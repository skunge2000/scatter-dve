// scatter-dve — WU-23b1: Weston 3-field de-interlace, filter core. See
// video/deinterlace.hpp for the class contract and DECISIONS.md
// ADR-078/ADR-079 (and CORRECTIONS.md C-025) for the full design account.
//
// Coefficients and row-offset structure below are transcribed as numeric
// constants and re-derived arithmetic, not FFmpeg's own source text:
// libavfilter/vf_w3fdif.c is BBC R&D/FFmpeg's own original implementation
// of a published algorithm (Martin Weston's process for BBC R&D,
// PH-2071), and this file is this project's own original implementation
// of the same published algorithm, reproducing its numeric coefficients
// (which the algorithm itself fixes) without reproducing the real
// source's own code structure or comments.
#include "video/deinterlace.hpp"

#include <algorithm>
#include <array>
#include <cstdint>

namespace scatter::video {

namespace {

// ---------------------------------------------------------------------------
// Coefficient sets -- DECISIONS.md ADR-079, re-verified directly against
// libavfilter/vf_w3fdif.c's own coef_lf[]/coef_hf[] (PH-2071, scaled by
// 256*128 = 2^15). The static_asserts below are this unit's own
// build-time re-verification of the two properties the real source's own
// comment calls out (n_coef_lf even, n_coef_hf odd) plus the two
// properties the algorithm's own design intent requires (unity low-pass
// gain, zero high-pass gain) -- not merely copied from the source, but
// checked here every time this file is compiled.
// ---------------------------------------------------------------------------

inline constexpr int kMaxLowTaps = 4;
inline constexpr int kMaxHighTaps = 5;

struct CoefSet {
    int nLow;
    std::array<std::int32_t, kMaxLowTaps> low;
    int nHigh;
    std::array<std::int32_t, kMaxHighTaps> high;
};

inline constexpr CoefSet kSimpleCoefs{
    2, {16384, 16384, 0, 0},
    3, {-2048, 4096, -2048, 0, 0},
};

inline constexpr CoefSet kComplexCoefs{
    4, {-852, 17236, 17236, -852},
    5, {1016, -3801, 5570, -3801, 1016},
};

static_assert(kSimpleCoefs.nLow % 2 == 0 && kComplexCoefs.nLow % 2 == 0,
              "n_coef_lf must be even -- vf_w3fdif.c's own comment on why");
static_assert(kSimpleCoefs.nHigh % 2 == 1 && kComplexCoefs.nHigh % 2 == 1,
              "n_coef_hf must be odd -- vf_w3fdif.c's own comment on why");

static_assert(kSimpleCoefs.low[0] + kSimpleCoefs.low[1] == 32768,
              "simple low-pass taps must sum to unity (2^15)");
static_assert(kComplexCoefs.low[0] + kComplexCoefs.low[1] + kComplexCoefs.low[2] +
                  kComplexCoefs.low[3] == 32768,
              "complex low-pass taps must sum to unity (2^15)");
static_assert(kSimpleCoefs.high[0] + kSimpleCoefs.high[1] + kSimpleCoefs.high[2] == 0,
              "simple high-pass taps must sum to zero (no DC contribution)");
static_assert(kComplexCoefs.high[0] + kComplexCoefs.high[1] + kComplexCoefs.high[2] +
                  kComplexCoefs.high[3] + kComplexCoefs.high[4] == 0,
              "complex high-pass taps must sum to zero (no DC contribution)");

const CoefSet& coefsFor(DeinterlaceCoefficients c) noexcept {
    return c == DeinterlaceCoefficients::Simple ? kSimpleCoefs : kComplexCoefs;
}

// ---------------------------------------------------------------------------
// Row-offset structure and edge reflection -- re-derived directly from
// libavfilter/vf_w3fdif.c's own deinterlace_plane_slice(), not assumed:
// for a to-be-reconstructed row r, low-pass tap j reads cur at row
// reflect((r+1) + 2j - nLow); high-pass tap j reads both cur and prev at
// row reflect((r+1) + 2j - nHigh). Every low-pass offset is odd relative
// to r (always an anchor-parity row, real data in cur); every high-pass
// offset is even relative to r (always a missing-parity row, real data in
// both cur and prev) -- adding or subtracting 2 preserves parity, so
// reflection never crosses from one to the other.
// ---------------------------------------------------------------------------

constexpr int reflectRow(int y, int height) noexcept {
    while (y < 0) y += 2;
    while (y >= height) y -= 2;
    return y;
}

// Round-half-up via "add half the divisor, then shift" -- the same idiom
// core/types.hpp's toCode10() and video/chroma.cpp's roundShift() already
// use, here on a signed 64-bit sum (I4/I6): C++20 guarantees a signed
// right shift is arithmetic (floor division by a power of two,
// two's-complement), so this rounds correctly for the negative sums the
// high-pass taps' own negative lobes produce, not just positive ones.
// Narrowing to Sample below wraps modulo 65536 for an out-of-range result
// -- not a clamp -- exactly video/chroma.hpp's own documented precedent
// for a negative-lobe integer filter, honouring I2 ("filter ringing...
// passes through untouched").
inline Sample roundShift15(std::int64_t sum) noexcept {
    return Sample((sum + (std::int64_t(1) << 14)) >> 15);
}

// One plane (Y, Cb or Cr); cur and prev are both that same plane, of the
// same weave-frame shape. anchorOffset is 0 if row 0 is the anchor parity
// (FieldParity::Top), 1 if row 1 is (FieldParity::Bottom) --
// video/interlace.hpp's own FieldParity convention (Top owns row 0).
void reconstructPlane(const Sample* curPlane, const Sample* prevPlane,
                      Sample* outPlane, int width, int height,
                      int anchorOffset, const CoefSet& coefs) noexcept {
    for (int r = 0; r < height; ++r) {
        const std::size_t rowBase = std::size_t(r) * std::size_t(width);

        if ((r & 1) == anchorOffset) {
            // Anchor-parity row: cur's own real data, copied through
            // unchanged -- the real source's own "copy unchanged the
            // lines of the field" step.
            std::copy_n(curPlane + rowBase, std::size_t(width), outPlane + rowBase);
            continue;
        }

        // Missing-parity row: spatial low-pass over cur's own nearby
        // anchor-parity rows, plus a temporal high-pass summing cur's and
        // prev's own missing-parity rows at the same reflected positions.
        const Sample* lowRows[kMaxLowTaps];
        for (int j = 0; j < coefs.nLow; ++j) {
            const int yin = reflectRow((r + 1) + 2 * j - coefs.nLow, height);
            lowRows[j] = curPlane + std::size_t(yin) * std::size_t(width);
        }
        const Sample* highCurRows[kMaxHighTaps];
        const Sample* highPrevRows[kMaxHighTaps];
        for (int j = 0; j < coefs.nHigh; ++j) {
            const int yin = reflectRow((r + 1) + 2 * j - coefs.nHigh, height);
            const std::size_t base = std::size_t(yin) * std::size_t(width);
            highCurRows[j] = curPlane + base;
            highPrevRows[j] = prevPlane + base;
        }

        Sample* outRow = outPlane + rowBase;
        for (int x = 0; x < width; ++x) {
            std::int64_t sum = 0;
            for (int j = 0; j < coefs.nLow; ++j) {
                sum += std::int64_t(lowRows[j][std::size_t(x)]) * coefs.low[std::size_t(j)];
            }
            for (int j = 0; j < coefs.nHigh; ++j) {
                sum += std::int64_t(highCurRows[j][std::size_t(x)]) * coefs.high[std::size_t(j)];
                sum += std::int64_t(highPrevRows[j][std::size_t(x)]) * coefs.high[std::size_t(j)];
            }
            outRow[x] = roundShift15(sum);
        }
    }
}

void reconstruct(const Raster444& cur, const Raster444& prev, FieldParity anchorParity,
                 const CoefSet& coefs, Raster444& out) noexcept {
    const int anchorOffset = (anchorParity == FieldParity::Bottom) ? 1 : 0;
    reconstructPlane(cur.Y.data(),  prev.Y.data(),  out.Y.data(),  cur.width, cur.height, anchorOffset, coefs);
    reconstructPlane(cur.Cb.data(), prev.Cb.data(), out.Cb.data(), cur.width, cur.height, anchorOffset, coefs);
    reconstructPlane(cur.Cr.data(), prev.Cr.data(), out.Cr.data(), cur.width, cur.height, anchorOffset, coefs);
}

}  // namespace

Deinterlacer::Deinterlacer(FieldParity anchorParity, DeinterlaceCoefficients coeffs) noexcept
    : anchorParity_(anchorParity), coeffs_(coeffs) {}

bool Deinterlacer::push(const Raster444& weaveFrame, Raster444& outFrame) {
    // Shift the history exactly as libavfilter/vf_w3fdif.c's own
    // filter_frame() does (DECISIONS.md ADR-079, CORRECTIONS.md C-025):
    // prev <- cur, cur <- next, next <- the newly-pushed frame.
    prev_ = std::move(cur_);
    cur_ = std::move(next_);
    next_ = weaveFrame;

    // Stream start: the very first frame ever pushed has no cur yet, so
    // it becomes its own cur too -- exactly the real source's own
    // av_frame_clone(next)-when-cur-is-null convention. This can only
    // happen on the first call: next_ is set unconditionally on every
    // call, so from the second call onward cur_ <- next_ above is always
    // already engaged.
    if (!cur_.has_value()) {
        cur_ = next_;
    }

    // No prev yet: only the very first push ever reaches here without
    // one (every push from the second onward has a cur already, which
    // became this push's own prev above).
    if (!prev_.has_value()) {
        return false;
    }

    reconstruct(*cur_, *prev_, anchorParity_, coefsFor(coeffs_), outFrame);
    return true;
}

}  // namespace scatter::video
