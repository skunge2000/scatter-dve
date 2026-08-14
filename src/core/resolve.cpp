// scatter-dve — WU-10: see core/resolve.hpp for the design note and
// DECISIONS.md ADR-026 for the choices this file makes that
// architecture.md leaves open.
#include "core/resolve.hpp"

#include <algorithm>
#include <cstdint>

namespace scatter {

namespace {

// One channel of architecture.md 4.8's divide: colour / weight, rounded to
// nearest via "add half the divisor, then divide" (the same convention
// core/types.hpp's toCode10 and ADR-020's chroma filters already use).
// colour and weight are both non-negative by construction (every
// contribution accumulateCorner() makes in core/splat.cpp is a product of
// non-negative quantities), so this is plain non-negative integer
// division, no sign handling needed.
//
// Cannot overflow Sample on the way back down: colour is a sum of
// per-corner contributions each of the form (sample * frag.w * rawWeight)
// >> 8 (core/splat.cpp's accumulateCorner()), and sample is itself already
// bounded by Sample's own range, so colour <= sample_max * weight for any
// sample_max attainable -- the coverage-weighted average this divide
// computes cannot exceed the largest sample that contributed to it, and
// the rounding term (weight / 2) added before dividing cannot push an
// exact quotient of sample_max past sample_max either, since weight / 2 <
// weight for any weight >= 1. See resolve.hpp's own comment on
// normaliseCell() for why this needs no clamp (I2).
Sample divideRounded(ColourAccum colour, WeightAccum weight) noexcept {
    const std::int64_t w = std::int64_t(weight);
    const std::int64_t r = (colour + w / 2) / w;
    return Sample(r);
}

}  // namespace

ResolvedCell normaliseCell(const AccumCell& cell) noexcept {
    if (cell.w <= 0) {
        return ResolvedCell{};
    }
    ResolvedCell out;
    out.Y  = divideRounded(cell.Y, cell.w);
    out.Cb = divideRounded(cell.Cb, cell.w);
    out.Cr = divideRounded(cell.Cr, cell.w);
    out.covered = true;
    return out;
}

namespace {

// One channel of the composite: a convex blend of `colour` and `bg` by
// `alpha` / kWeightUnity, rounded to nearest the same way divideRounded()
// is. alpha is already clamped to [0, kWeightUnity] by the caller
// (composite() below), so (unity - alpha) is never negative and this is,
// like divideRounded(), a blend of two non-negative Sample-range values --
// it cannot leave that range either, so again no clamp (I2).
Sample blend(Sample colour, Sample bg, std::int64_t alpha) noexcept {
    const std::int64_t unity = std::int64_t(kWeightUnity);
    const std::int64_t sum = std::int64_t(colour) * alpha +
                              std::int64_t(bg) * (unity - alpha) +
                              unity / 2;
    return Sample(sum / unity);
}

}  // namespace

CompositedCell composite(const AccumCell& cell, const Background& bg) noexcept {
    const ResolvedCell resolved = normaliseCell(cell);
    if (!resolved.covered) {
        return CompositedCell{bg.Y, bg.Cb, bg.Cr};
    }
    const std::int64_t alpha = std::clamp<std::int64_t>(
        std::int64_t(cell.w), std::int64_t(0), std::int64_t(kWeightUnity));

    CompositedCell out;
    out.Y  = blend(resolved.Y, bg.Y, alpha);
    out.Cb = blend(resolved.Cb, bg.Cb, alpha);
    out.Cr = blend(resolved.Cr, bg.Cr, alpha);
    return out;
}

}  // namespace scatter
