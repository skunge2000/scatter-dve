// scatter-dve — chroma resampling, scalar reference
//
// Deliberately plain, same spirit as v210.cpp: this is the oracle WU-18's
// NEON path is diffed against, so it is written to be read and believed
// rather than to be fast. See chroma.hpp for the filter design and
// docs/architecture.md section 5 for the rationale.

#include "video/chroma.hpp"

#include <cstdint>

#include "video/v210.hpp"

namespace scatter::chroma {
namespace {

// Edge handling: replicate the boundary sample for taps that would read
// outside [0, n-1]. Same choice for both filters.
inline int clampIndex(int i, int n) noexcept {
    return i < 0 ? 0 : (i >= n ? n - 1 : i);
}

// Round-half-up via "add half the divisor, then arithmetic shift" — see
// chroma.hpp for why this rounds correctly for negative sums too, and why
// the plain conversion to Sample below is not a clamp.
inline Sample roundShift(std::int32_t sum, std::int32_t halfDivisor,
                         int shift) noexcept {
    return Sample((sum + halfDivisor) >> shift);
}

}  // namespace

void upsampleRow(const Sample* in, int width, Sample* out) noexcept {
    const int cw = v210::chromaWidth(width);

    for (int i = 0; i < cw; ++i) {
        out[2 * i] = in[i];
    }
    for (int i = 0; i < cw; ++i) {
        const Sample a = in[clampIndex(i - 1, cw)];
        const Sample b = in[i];
        const Sample c = in[clampIndex(i + 1, cw)];
        const Sample d = in[clampIndex(i + 2, cw)];
        const std::int32_t sum = -std::int32_t(a) + 9 * std::int32_t(b) +
                                 9 * std::int32_t(c) - std::int32_t(d);
        out[2 * i + 1] = roundShift(sum, 8, 4);
    }
}

void downsampleRow(const Sample* in, int width, Sample* out) noexcept {
    const int cw = v210::chromaWidth(width);

    for (int i = 0; i < cw; ++i) {
        const int c0 = 2 * i;
        const Sample m5 = in[clampIndex(c0 - 5, width)];
        const Sample m3 = in[clampIndex(c0 - 3, width)];
        const Sample m1 = in[clampIndex(c0 - 1, width)];
        const Sample p0 = in[c0];
        const Sample p1 = in[clampIndex(c0 + 1, width)];
        const Sample p3 = in[clampIndex(c0 + 3, width)];
        const Sample p5 = in[clampIndex(c0 + 5, width)];
        const std::int32_t sum = 3 * std::int32_t(m5) - 25 * std::int32_t(m3) +
                                 150 * std::int32_t(m1) +
                                 256 * std::int32_t(p0) +
                                 150 * std::int32_t(p1) -
                                 25 * std::int32_t(p3) + 3 * std::int32_t(p5);
        out[i] = roundShift(sum, 256, 9);
    }
}

void upsampleImage(const Sample* in, std::ptrdiff_t inStrideSamples,
                   int width, int height,
                   Sample* out, std::ptrdiff_t outStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        upsampleRow(in + r * inStrideSamples, width, out + r * outStrideSamples);
    }
}

void downsampleImage(const Sample* in, std::ptrdiff_t inStrideSamples,
                     int width, int height,
                     Sample* out, std::ptrdiff_t outStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        downsampleRow(in + r * inStrideSamples, width,
                      out + r * outStrideSamples);
    }
}

}  // namespace scatter::chroma
