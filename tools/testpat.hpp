// scatter-dve — WU-03: test pattern generation
//
// Header-only. tools/make_testpat.cpp and tests/test_testpat.cpp both need
// these functions and neither links the other — make_testpat is its own
// add_executable (CMakeLists.txt), not part of scatter-core — so the
// implementation lives here rather than split into a .cpp the way v210 is.
//
// Everything generated here already satisfies I2 by construction: every
// value handed to fromCode10 is within [kCode10Min, kCode10Max], so nothing
// downstream needs to clamp. v210::packRow remains the only clamp site in
// the whole pipeline; these generators do not add a second one.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

#include "core/types.hpp"
#include "video/v210.hpp"

namespace scatter::testpat {

// ---------------------------------------------------------------------------
// Frame — planar 4:2:2, tight-packed (stride == width / chromaWidth(width)).
// Mirrors the `Planes` helper private to tests/test_v210.cpp; duplicated
// rather than shared, since that one is local to its own translation unit
// (SESSION-PROTOCOL.md rule 2: no refactor across module boundaries to fold
// the two together).
// ---------------------------------------------------------------------------
struct Frame {
    std::vector<scatter::Sample> Y, Cb, Cr;
    int width  = 0;
    int height = 0;

    Frame(int w, int h)
        : Y(std::size_t(w) * std::size_t(h)),
          Cb(std::size_t(scatter::v210::chromaWidth(w)) * std::size_t(h)),
          Cr(std::size_t(scatter::v210::chromaWidth(w)) * std::size_t(h)),
          width(w), height(h) {}

    std::ptrdiff_t yStride() const noexcept { return width; }
    std::ptrdiff_t cStride() const noexcept { return scatter::v210::chromaWidth(width); }
};

// ---------------------------------------------------------------------------
// Ramp — full-range sweep, code 4 to 1019, on Y, Cb and Cr independently.
//
// Column 0 and the last column of each plane land on kCode10Min and
// kCode10Max exactly, by construction of the rounded-division formula
// below — not by approaching them asymptotically — so the endpoints are
// exact regardless of plane width. Identical on every row.
// ---------------------------------------------------------------------------

// `span` is the sample count the sweep crosses (a plane width). Requires
// span >= 2 to have two distinct endpoints; span == 1 has nowhere to put a
// sweep and returns kCode10Min. Not exercised by any width this project
// uses: isSupportedWidth requires width >= 2, and every width used so far
// (8 upward) keeps chromaWidth(width) >= 2 as well.
inline std::uint16_t rampCode(int x, int span) noexcept {
    if (span <= 1) return scatter::kCode10Min;
    const std::uint32_t range = std::uint32_t(scatter::kCode10Max - scatter::kCode10Min);  // 1015
    const std::uint32_t num   = std::uint32_t(x) * range;
    const std::uint32_t den   = std::uint32_t(span - 1);
    const std::uint32_t step  = (num + den / 2) / den;  // rounded, not truncated
    return std::uint16_t(scatter::kCode10Min + step);
}

inline Frame makeRamp(int width, int height) {
    Frame f(width, height);
    const int cw = scatter::v210::chromaWidth(width);

    // Sized with a named variable, not `vector<T> v(std::size_t(width))`
    // directly: the latter parses as a function declaration ("most vexing
    // parse" — std::size_t(width) reads as a redundantly-parenthesised
    // parameter named width) and -Werror turns the resulting warning into a
    // build failure.
    const std::size_t widthN = std::size_t(width);
    const std::size_t cwN    = std::size_t(cw);
    std::vector<scatter::Sample> rowY(widthN);
    for (int x = 0; x < width; ++x) {
        rowY[std::size_t(x)] = scatter::fromCode10(rampCode(x, width));
    }
    std::vector<scatter::Sample> rowC(cwN);
    for (int x = 0; x < cw; ++x) {
        rowC[std::size_t(x)] = scatter::fromCode10(rampCode(x, cw));
    }

    for (int y = 0; y < height; ++y) {
        std::copy(rowY.begin(), rowY.end(), f.Y.begin() + std::ptrdiff_t(y) * width);
        std::copy(rowC.begin(), rowC.end(), f.Cb.begin() + std::ptrdiff_t(y) * cw);
        std::copy(rowC.begin(), rowC.end(), f.Cr.begin() + std::ptrdiff_t(y) * cw);
    }
    return f;
}

// ---------------------------------------------------------------------------
// Zone plate — concentric rings of increasing spatial frequency toward the
// edge, luma only. Chroma held flat at the achromatic code: WU-10's aliasing
// check is about the chroma resampler's response to synthesised high-
// frequency content, and this pattern needs to carry that content in the
// plane the resampler will later act on, not already be coloured.
//
// Amplitude is bounded to the nominal legal range [kCode10Black,
// kCode10WhiteNominal], not the full protocol range: this is a resolution
// stimulus, not an I2 excursion test (makeExcursion is that). Ring density
// (kZoneCycles) is a starting point, not a tuned value — WU-10 adjusts it
// against the actual resampler's cutoff, per HANDOFF.md's scope note for
// this unit.
// ---------------------------------------------------------------------------

inline constexpr double kPi = 3.14159265358979323846;
inline constexpr double kZoneCycles = 8.0;

inline Frame makeZonePlate(int width, int height) {
    Frame f(width, height);
    const double cx = double(width  - 1) / 2.0;
    const double cy = double(height - 1) / 2.0;
    const double maxR2 = cx * cx + cy * cy;
    const double k = (maxR2 > 0.0) ? (kZoneCycles * kPi / maxR2) : 0.0;

    const double kCenter    = (double(scatter::kCode10WhiteNominal) + double(scatter::kCode10Black)) / 2.0;
    const double kAmplitude = (double(scatter::kCode10WhiteNominal) - double(scatter::kCode10Black)) / 2.0;

    for (int y = 0; y < height; ++y) {
        const double dy = double(y) - cy;
        for (int x = 0; x < width; ++x) {
            const double dx = double(x) - cx;
            const double r2 = dx * dx + dy * dy;
            double v = kCenter + kAmplitude * std::cos(k * r2);
            // Defensive only: cos() is bounded to [-1, 1], so v is already
            // within [kCode10Black, kCode10WhiteNominal] mathematically;
            // this guards against float rounding at the extremes.
            if (v < double(scatter::kCode10Black))       v = double(scatter::kCode10Black);
            if (v > double(scatter::kCode10WhiteNominal)) v = double(scatter::kCode10WhiteNominal);
            const std::uint16_t code = std::uint16_t(std::lround(v));
            f.Y[std::size_t(y) * std::size_t(width) + std::size_t(x)] = scatter::fromCode10(code);
        }
    }
    std::fill(f.Cb.begin(), f.Cb.end(), scatter::kChromaZero);
    std::fill(f.Cr.begin(), f.Cr.end(), scatter::kChromaZero);
    return f;
}

// ---------------------------------------------------------------------------
// Excursion — deliberate sub-black and super-white codes, applied to Y, Cb
// and Cr alike. Not a legal-signal pattern (I2 does not require legality,
// only that codes 0-3 and 1020-1023 never appear, and this pattern keeps
// clear of those). The point is to prove nothing downstream of pack/unpack
// legalises what I2 says must pass through untouched.
// ---------------------------------------------------------------------------

// 4 and 1019 are the protocol limits; 20 is footroom (below kCode10Black,
// 64); 1000 is super-white (above kCode10WhiteNominal, 940). Cycle length 6
// needs plane width >= 6 for every code to appear at least once — true for
// every width this project uses (720/1920 luma, 360/960 chroma).
inline constexpr std::uint16_t kExcursionCycle[] = {
    scatter::kCode10Min, 20, scatter::kCode10Black, scatter::kCode10WhiteNominal, 1000, scatter::kCode10Max,
};
inline constexpr int kExcursionCycleLen =
    int(sizeof(kExcursionCycle) / sizeof(kExcursionCycle[0]));

inline Frame makeExcursion(int width, int height) {
    Frame f(width, height);
    const int cw = scatter::v210::chromaWidth(width);

    const std::size_t widthN = std::size_t(width);
    const std::size_t cwN    = std::size_t(cw);
    std::vector<scatter::Sample> rowY(widthN);
    for (int x = 0; x < width; ++x) {
        rowY[std::size_t(x)] = scatter::fromCode10(kExcursionCycle[x % kExcursionCycleLen]);
    }
    std::vector<scatter::Sample> rowC(cwN);
    for (int x = 0; x < cw; ++x) {
        rowC[std::size_t(x)] = scatter::fromCode10(kExcursionCycle[x % kExcursionCycleLen]);
    }

    for (int y = 0; y < height; ++y) {
        std::copy(rowY.begin(), rowY.end(), f.Y.begin() + std::ptrdiff_t(y) * width);
        std::copy(rowC.begin(), rowC.end(), f.Cb.begin() + std::ptrdiff_t(y) * cw);
        std::copy(rowC.begin(), rowC.end(), f.Cr.begin() + std::ptrdiff_t(y) * cw);
    }
    return f;
}

// ---------------------------------------------------------------------------
// File output — raw .v210, no header. Stride is rowBytesMin(width): the
// right default for files (docs/workflow.md), and it coincides with the
// aligned figure at both 720 and 1920 anyway, so the two never disagree at
// the widths this project actually uses.
// ---------------------------------------------------------------------------

inline bool writeV210(const Frame& f, const std::string& path) {
    const std::size_t stride = scatter::v210::rowBytesMin(f.width);
    std::vector<std::uint8_t> packed(stride * std::size_t(f.height));

    scatter::v210::packImage(f.Y.data(), f.yStride(),
                             f.Cb.data(), f.Cr.data(), f.cStride(),
                             f.width, f.height,
                             packed.data(), std::ptrdiff_t(stride));

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(packed.data()),
              std::streamsize(packed.size()));
    return bool(out);
}

}  // namespace scatter::testpat
