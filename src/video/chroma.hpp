// scatter-dve — chroma resampling, 4:2:2 <-> 4:4:4 (ADR-005, architecture.md
// section 5)
//
// Horizontal only. 4:2:2 already carries full vertical chroma resolution, so
// there is nothing to resample vertically here — see architecture.md section
// 5, "Vertical chroma is already full-rate in 4:2:2, so only field pairing
// is fiddly." Co-sited placement per Rec. 601 / Rec. 709: chroma sample i is
// co-located with luma sample 2*i.
//
// I2 applies here as everywhere in the pipeline: no clipping, no
// legalisation. Both filters below are symmetric with negative outer lobes
// and will ring on a step edge, which can push legal source content outside
// 64-940 (in 10-bit terms) in the intermediate 4:4:4 representation. That
// ringing passes through untouched — the only clamp anywhere in the pipeline
// is v210::pack, applying the v210 protocol limits alone. It is not
// duplicated, weakened or anticipated here.
//
// Both filters are integer, fixed-point and normalise by an exact power of
// two, so a flat field survives bit-exact with no rounding error, and the
// scalar reference here is the oracle WU-18's NEON path is diffed against —
// the same relationship v210.cpp already has to WU-17.
#pragma once

#include <cstddef>

#include "core/types.hpp"

namespace scatter::chroma {

// ---------------------------------------------------------------------------
// Upsample: 4:2:2 -> 4:4:4, one row
//
//   out[2i]   = in[i]                                       (co-sited, exact)
//   out[2i+1] = (-in[i-1] + 9*in[i] + 9*in[i+1] - in[i+2] + 8) >> 4
//
// Coefficients (-1, 9, 9, -1)/16: the standard 4-tap half-sample
// interpolator (the same weights used for half-pel luma interpolation in
// several video codecs). Symmetric, unity DC gain — the taps sum to 16, an
// exact power of two, so a flat field upsamples with no rounding error —
// and negative outer lobes, so it rings on a step rather than smearing it.
//
// Edge handling: the source index is clamped to [0, chromaWidth(width) - 1]
// (edge replication) for taps that would otherwise read outside the plane.
//
// Rounding: round-half-up (ties away from zero's usual meaning, here toward
// +infinity), via "add half the divisor, then arithmetic shift" — the same
// convention core/types.hpp's toCode10 uses. C++20 guarantees signed right
// shift is arithmetic (floor division by a power of two, two's-complement),
// so this rounds correctly for the negative partial sums the outer lobes
// produce, not just positive ones.
//
// No range clamp is applied to the result beyond what Sample (a 16-bit
// unsigned container) can hold. A partial sum can go negative when nearby
// samples are near the bottom of the representable range — most plainly for
// an isolated impulse against a zero background, which is how this file's
// test verifies the coefficients including their sign. Converting a
// negative or over-65535 mathematical result to Sample wraps modulo 65536,
// per the standard's rule for narrowing to an unsigned type: well-defined,
// not undefined behaviour, and not a clamp — I2 is not violated because
// nothing is being pulled back toward a legal range, the container is just
// finite. In practice chroma content sits near kChromaZero with headroom on
// both sides, and the two filters here overshoot a step by at most 1/16 and
// 22/512 of its size respectively (worked in tests/test_chroma.cpp), so this
// does not arise for realistic signals; it has not been exercised for
// content driven all the way to the v210 protocol limits.
//
// Preconditions, none checked at runtime: width even and >= 2; `in` holds at
// least v210::chromaWidth(width) samples; `out` holds at least `width`.
// ---------------------------------------------------------------------------
void upsampleRow(const Sample* in, int width, Sample* out) noexcept;

// ---------------------------------------------------------------------------
// Downsample: 4:4:4 -> 4:2:2, one row
//
//   out[i] = ( 3*in[2i-5] - 25*in[2i-3] + 150*in[2i-1]
//            + 256*in[2i]
//            + 150*in[2i+1] - 25*in[2i+3] + 3*in[2i+5] + 256 ) >> 9
//
// Coefficients (3, -25, 150, 256, 150, -25, 3)/512: a standard truncated,
// windowed half-band low-pass. Symmetric, unity DC gain (taps sum to 512,
// an exact power of two) and negative lobes, same rationale as the
// upsampler. This is the filter that must run before decimation to 4:2:2:
// the warp synthesises high-frequency chroma content at the non-co-sited
// positions the source never had, and skipping this filter aliases that
// content back as an artefact indistinguishable from a bug in the splat.
//
// The half-band property — every tap at a nonzero *even* offset from the
// centre is exactly zero — means only the centre sample and its odd-offset
// neighbours (themselves interpolated, non-co-sited 4:4:4 samples) ever
// enter the sum. An impulse placed at an even (co-sited) 4:4:4 position
// therefore passes straight through to the one corresponding output sample
// and nowhere else; verified in tests/test_chroma.cpp alongside the
// coefficients proper, which need an impulse at an *odd* position to
// exercise.
//
// Edge handling, rounding and the absence of a result-range clamp: as
// upsampleRow. The overshoot on a step is at most 22/512 of the step size
// (worked in tests/test_chroma.cpp), smaller than the upsampler's 1/16.
//
// Preconditions, none checked at runtime: width even and >= 2; `in` holds at
// least `width` samples; `out` holds at least v210::chromaWidth(width).
// ---------------------------------------------------------------------------
void downsampleRow(const Sample* in, int width, Sample* out) noexcept;

// ---------------------------------------------------------------------------
// Image operations — apply the row primitive to every row independently.
// Sample strides may exceed the natural row width, exactly as in v210.hpp;
// neither may be negative here — that is the caller's problem, not this
// layer's.
//
// `inStrideSamples`/`outStrideSamples` are row strides for whichever side of
// the call is 4:2:2-width (chromaWidth(width) samples/row) versus
// 4:4:4-width (`width` samples/row) — upsampleImage's input is the former
// and its output the latter; downsampleImage is the reverse.
// ---------------------------------------------------------------------------
void upsampleImage(const Sample* in, std::ptrdiff_t inStrideSamples,
                   int width, int height,
                   Sample* out, std::ptrdiff_t outStrideSamples) noexcept;

void downsampleImage(const Sample* in, std::ptrdiff_t inStrideSamples,
                     int width, int height,
                     Sample* out, std::ptrdiff_t outStrideSamples) noexcept;

// ---------------------------------------------------------------------------
// NEON operations — WU-18, ADR-043
//
// Present only when __ARM_NEON is defined (AArch64 always defines it), same
// guard v210.hpp already uses (ADR-042) so scatter-core's object list and
// every existing test are unaffected on x86_64.
//
// Same signatures, same preconditions as the scalar functions above. Unlike
// v210's own NEON path — a fixed per-group bit-field interleave, uniform
// across every group regardless of width — this filter's own irregularity
// is a sliding window's boundary clamp (ADR-020's edge-replication choice),
// present only at a handful of indices near either end of a row and absent
// everywhere in the interior. The NEON siblings below vectorise exactly the
// interior — four lanes of int32 multiply-accumulate per batch, matching
// the scalar reference's own std::int32_t accumulator width, coefficients
// and rounding exactly, lane for lane; the edge indices where a tap's
// clampIndex() call actually replicates a boundary sample are computed
// scalar, calling the identical clampIndex()/roundShift() helpers the
// scalar row functions already use — the same "vectorise the uniform part,
// leave the genuinely irregular part scalar" discipline ADR-042 established
// for v210's own bit-interleave, applied here to a boundary-clamp
// irregularity instead. Bit-identical to the scalar reference is this
// unit's entire accept criterion (WORK-UNITS.md) — diffed against a call to
// the same function (upsampleRow/downsampleRow) over the same input, never
// a round trip through both filters, which C-006 already established is
// not bit-exact for non-flat content and is not what this unit claims.
// tests/test_chroma_neon.cpp checks this directly. See DECISIONS.md
// ADR-043 for the full design.
// ---------------------------------------------------------------------------

#if defined(__ARM_NEON)

void upsampleRowNeon(const Sample* in, int width, Sample* out) noexcept;

void downsampleRowNeon(const Sample* in, int width, Sample* out) noexcept;

void upsampleImageNeon(const Sample* in, std::ptrdiff_t inStrideSamples,
                       int width, int height,
                       Sample* out, std::ptrdiff_t outStrideSamples) noexcept;

void downsampleImageNeon(const Sample* in, std::ptrdiff_t inStrideSamples,
                         int width, int height,
                         Sample* out, std::ptrdiff_t outStrideSamples) noexcept;

#endif  // __ARM_NEON

}  // namespace scatter::chroma
