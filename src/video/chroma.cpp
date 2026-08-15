// scatter-dve — chroma resampling, scalar reference
//
// Deliberately plain, same spirit as v210.cpp: this is the oracle WU-18's
// NEON path is diffed against, so it is written to be read and believed
// rather than to be fast. See chroma.hpp for the filter design and
// docs/architecture.md section 5 for the rationale.

#include "video/chroma.hpp"

#include <cstdint>

#include "video/v210.hpp"

#if defined(__ARM_NEON)
#include <arm_neon.h>  // file scope, not nested in any namespace -- WU-18, ADR-043
#endif

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

// ---------------------------------------------------------------------------
// NEON — WU-18, ADR-043
//
// Unlike v210's own NEON path (WU-17, ADR-042) -- a fixed per-group bit
// interleave, identically shaped for every group regardless of width --
// this filter's own irregularity is a sliding window's boundary clamp
// (clampIndex() above, ADR-020's edge-replication choice), present at a
// handful of indices near either end of a row and completely absent in the
// interior. The functions below vectorise exactly the interior: four lanes
// of int32 multiply-accumulate per batch, matching the scalar functions'
// own std::int32_t accumulator width, tap coefficients and round-shift
// exactly, lane for lane. The edge indices where a tap's clampIndex() call
// actually replicates a boundary sample -- upsample: index 0 and the last
// couple of indices; downsample: the first three and the last couple,
// since its footprint is wider -- are computed scalar, calling
// clampIndex()/roundShift() directly, unmodified from the scalar functions
// above. Same "vectorise the genuinely uniform part, leave the irregular
// part scalar" discipline ADR-042 already used for v210's own bit-field
// placement, applied here to a boundary-clamp irregularity instead of a
// bit-interleave one.
//
// Reordering the seven (downsample) or four (upsample) terms of an exact,
// non-overflowing int32 sum -- which is all NEON's own multiply-accumulate
// instruction selection can ever do relative to the scalar functions' own
// left-to-right sum -- changes nothing: integer addition is associative and
// commutative when no term overflows, which chroma.hpp's own worst-case
// bounds (1/16 of a step for upsample, 22/512 for downsample, both far
// under int32's range) already establish holds throughout Sample's whole
// 16-bit domain. This is not the C-012 hazard: that lesson is specific to
// floating point, where two differently-shaped expressions for the same
// real number are not guaranteed to round identically; there is no
// equivalent hazard in exact integer arithmetic, so this unit's NEON MAC
// order is guaranteed identical to the scalar sum's own result, not merely
// close to it.
// ---------------------------------------------------------------------------

#if defined(__ARM_NEON)

void upsampleRowNeon(const Sample* in, int width, Sample* out) noexcept {
    const int cw = v210::chromaWidth(width);

    // Co-sited samples never need boundary clamping -- in[i] is always in
    // range for i in [0, cw) -- so this is a plain scalar copy. There is no
    // arithmetic here to diverge on, and vectorising a copy buys nothing a
    // decent compiler does not already do for a linear loop.
    for (int i = 0; i < cw; ++i) {
        out[2 * i] = in[i];
    }

    // Halfway samples, out[2i+1]. scalarHalfway is upsampleRow's own loop
    // body verbatim, used for index 0 and, below, whichever tail near cw's
    // own edge does not fill a 4-lane interior batch.
    const auto scalarHalfway = [&](int idx) noexcept {
        const Sample a = in[clampIndex(idx - 1, cw)];
        const Sample b = in[idx];
        const Sample c = in[clampIndex(idx + 1, cw)];
        const Sample d = in[clampIndex(idx + 2, cw)];
        const std::int32_t sum = -std::int32_t(a) + 9 * std::int32_t(b) +
                                 9 * std::int32_t(c) - std::int32_t(d);
        out[2 * idx + 1] = roundShift(sum, 8, 4);
    };

    int i = 0;
    if (cw >= 1) {
        scalarHalfway(0);
        i = 1;
    }
    // Interior: 1 <= i <= cw-3, where clampIndex(i-1,cw)==i-1 and
    // clampIndex(i+2,cw)==i+2 -- every tap reads a genuinely adjacent
    // element, no boundary replication. A batch of 4 needs its last lane
    // (i+3) to still satisfy i+3 <= cw-3, i.e. i <= cw-6, i.e. i+4 <= cw-2.
    for (; i + 4 <= cw - 2; i += 4) {
        const uint16x4_t a16 = vld1_u16(in + i - 1);
        const uint16x4_t b16 = vld1_u16(in + i);
        const uint16x4_t c16 = vld1_u16(in + i + 1);
        const uint16x4_t d16 = vld1_u16(in + i + 2);

        const int32x4_t a32 = vreinterpretq_s32_u32(vmovl_u16(a16));
        const int32x4_t b32 = vreinterpretq_s32_u32(vmovl_u16(b16));
        const int32x4_t c32 = vreinterpretq_s32_u32(vmovl_u16(c16));
        const int32x4_t d32 = vreinterpretq_s32_u32(vmovl_u16(d16));

        int32x4_t sum = vnegq_s32(a32);
        sum = vmlaq_n_s32(sum, b32, 9);
        sum = vmlaq_n_s32(sum, c32, 9);
        sum = vsubq_s32(sum, d32);
        const int32x4_t rounded = vaddq_s32(sum, vdupq_n_s32(8));
        const int16x4_t halfway = vmovn_s32(vshrq_n_s32(rounded, 4));

        std::uint16_t lanes[4];
        vst1_u16(lanes, vreinterpret_u16_s16(halfway));
        out[2 * (i + 0) + 1] = lanes[0];
        out[2 * (i + 1) + 1] = lanes[1];
        out[2 * (i + 2) + 1] = lanes[2];
        out[2 * (i + 3) + 1] = lanes[3];
    }
    for (; i < cw; ++i) {
        scalarHalfway(i);
    }
}

void downsampleRowNeon(const Sample* in, int width, Sample* out) noexcept {
    const int cw = v210::chromaWidth(width);

    // downsampleRow's own loop body verbatim -- used for the first three
    // indices, below, and whichever tail near cw's own edge does not fill a
    // 4-lane interior batch.
    const auto scalarAt = [&](int idx) noexcept {
        const int c0 = 2 * idx;
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
        out[idx] = roundShift(sum, 256, 9);
    };

    // Interior: 3 <= i <= cw-3, where every one of the seven taps (offsets
    // -5,-3,-1,0,+1,+3,+5 from 2i) reads a genuinely adjacent element --
    // derived the same way upsampleRowNeon's own bound is, from the widest
    // tap (+-5) needing 2i-5 >= 0 and 2i+5 <= width-1. A batch's last lane
    // (i+3) needs i+3 <= cw-3, i.e. i+4 <= cw-2.
    int i = 0;
    for (; i < 3 && i < cw; ++i) scalarAt(i);

    for (; i + 4 <= cw - 2; i += 4) {
        // Six deinterleaving loads cover the seven tap offsets: an even
        // offset's value is a load's val[0] at that offset, an odd
        // offset's is the val[1] of a load starting one element earlier --
        // offset 0 and offset +1 share one load. Some redundancy (twelve
        // total sub-loads for seven distinct tap positions) in exchange for
        // a design simple enough to verify by direct per-lane arithmetic,
        // matching this unit's own "correctness over throughput" scope
        // (WU-19's job, not this one's).
        const uint16x4x2_t lm6 = vld2_u16(in + 2 * i - 6);  // val[1]: tap -5
        const uint16x4x2_t lm4 = vld2_u16(in + 2 * i - 4);  // val[1]: tap -3
        const uint16x4x2_t lm2 = vld2_u16(in + 2 * i - 2);  // val[1]: tap -1
        const uint16x4x2_t l0  = vld2_u16(in + 2 * i);      // val[0]: tap 0, val[1]: tap +1
        const uint16x4x2_t lp2 = vld2_u16(in + 2 * i + 2);  // val[1]: tap +3
        const uint16x4x2_t lp4 = vld2_u16(in + 2 * i + 4);  // val[1]: tap +5

        const int32x4_t t_5 = vreinterpretq_s32_u32(vmovl_u16(lm6.val[1]));
        const int32x4_t t_3 = vreinterpretq_s32_u32(vmovl_u16(lm4.val[1]));
        const int32x4_t t_1 = vreinterpretq_s32_u32(vmovl_u16(lm2.val[1]));
        const int32x4_t t0  = vreinterpretq_s32_u32(vmovl_u16(l0.val[0]));
        const int32x4_t tp1 = vreinterpretq_s32_u32(vmovl_u16(l0.val[1]));
        const int32x4_t tp3 = vreinterpretq_s32_u32(vmovl_u16(lp2.val[1]));
        const int32x4_t tp5 = vreinterpretq_s32_u32(vmovl_u16(lp4.val[1]));

        int32x4_t sum = vmulq_n_s32(t_5, 3);
        sum = vmlsq_n_s32(sum, t_3, 25);
        sum = vmlaq_n_s32(sum, t_1, 150);
        sum = vmlaq_n_s32(sum, t0, 256);
        sum = vmlaq_n_s32(sum, tp1, 150);
        sum = vmlsq_n_s32(sum, tp3, 25);
        sum = vmlaq_n_s32(sum, tp5, 3);

        const int32x4_t rounded = vaddq_s32(sum, vdupq_n_s32(256));
        const int16x4_t result = vmovn_s32(vshrq_n_s32(rounded, 9));
        vst1_u16(out + i, vreinterpret_u16_s16(result));
    }

    for (; i < cw; ++i) scalarAt(i);
}

void upsampleImageNeon(const Sample* in, std::ptrdiff_t inStrideSamples,
                       int width, int height,
                       Sample* out, std::ptrdiff_t outStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        upsampleRowNeon(in + r * inStrideSamples, width,
                        out + r * outStrideSamples);
    }
}

void downsampleImageNeon(const Sample* in, std::ptrdiff_t inStrideSamples,
                         int width, int height,
                         Sample* out, std::ptrdiff_t outStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        downsampleRowNeon(in + r * inStrideSamples, width,
                          out + r * outStrideSamples);
    }
}

#endif  // __ARM_NEON

}  // namespace scatter::chroma
