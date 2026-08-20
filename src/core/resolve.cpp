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

namespace {

// compositeLayered()'s own "read" step (resolve.hpp) hands its
// CompositedCell result to the "write" step's composite() call as a
// Background. Both structs are the same three Sample fields (Y, Cb, Cr) by
// construction -- this is a lossless relabelling, not a conversion, the
// same way ResolvedCell and CompositedCell are also the same three fields
// with different names for different pipeline stages.
Background asBackground(const CompositedCell& c) noexcept {
    return Background{c.Y, c.Cb, c.Cr};
}

// WU-12a's own accumulation-sums default (architecture.md 4.7 phase 1):
// component-wise sum of two AccumCells, exact (I6: integer addition is
// associative). AccumCell::reserved (core/types.hpp) is unused padding in
// both inputs, so the summed cell's own reserved is left at its
// value-initialised 0 rather than summing it too -- nothing reads it.
AccumCell sumCells(const AccumCell& a, const AccumCell& b) noexcept {
    AccumCell out{};
    out.Y  = a.Y + b.Y;
    out.Cb = a.Cb + b.Cb;
    out.Cr = a.Cr + b.Cr;
    out.w  = a.w + b.w;
    return out;
}

}  // namespace

CompositedCell compositeLayered(const AccumCell& lower, const AccumCell& upper,
                                 std::uint8_t upperTag, std::uint8_t opaqueTag,
                                 const Background& bg) noexcept {
    if (upperTag == opaqueTag) {
        const CompositedCell afterRead = composite(lower, bg);
        return composite(upper, asBackground(afterRead));
    }
    return composite(sumCells(lower, upper), bg);
}

namespace {

// Total order over occupied KSlots for compositeKBuffer()'s own two modes
// (WU-28b): primarily by firstSeenZ ascending (smaller = nearer, KSlot's
// own "near = 0" convention), ties (routine, not defensive -- see
// compositeKBuffer()'s own comment in resolve.hpp) broken by smallest tag.
bool nearerThan(const KSlot& a, const KSlot& b) noexcept {
    if (a.firstSeenZ != b.firstSeenZ) return a.firstSeenZ < b.firstSeenZ;
    return a.tag < b.tag;
}

}  // namespace

CompositedCell compositeKBuffer(const std::array<KSlot, kBufferK>& slots,
                                 KBufferResolveMode mode,
                                 const Background& bg) noexcept {
    std::array<const KSlot*, kBufferK> occupied{};
    int n = 0;
    for (const KSlot& s : slots) {
        if (s.occupied) {
            occupied[std::size_t(n)] = &s;
            ++n;
        }
    }
    if (n == 0) {
        return composite(AccumCell{}, bg);
    }

    if (mode == KBufferResolveMode::Opaque) {
        const KSlot* nearest = occupied[0];
        for (int i = 1; i < n; ++i) {
            if (nearerThan(*occupied[std::size_t(i)], *nearest)) {
                nearest = occupied[std::size_t(i)];
            }
        }
        return composite(nearest->cell, bg);
    }

    // Blend: sort the occupied pointers nearest-first -- a hand-written
    // insertion sort, not std::sort: kBufferK is at most 4, small enough
    // that GCC 13's own -Warray-bounds mistakes std::sort's internal
    // insertion-sort threshold for a real out-of-bounds access against
    // `occupied`'s static capacity (a known false positive at this size,
    // confirmed by building; not a real bug in the sort or a reason to
    // suppress the warning instead). Then composite farthest-to-nearest --
    // occupied[n-1] against bg first, each nearer slot composited over the
    // accumulated result, exactly compositeLayered()'s own mechanism above.
    for (int i = 1; i < n; ++i) {
        const KSlot* key = occupied[std::size_t(i)];
        int j = i - 1;
        while (j >= 0 && nearerThan(*key, *occupied[std::size_t(j)])) {
            occupied[std::size_t(j + 1)] = occupied[std::size_t(j)];
            --j;
        }
        occupied[std::size_t(j + 1)] = key;
    }

    Background acc = bg;
    for (int i = n - 1; i >= 0; --i) {
        const CompositedCell step = composite(occupied[std::size_t(i)]->cell, acc);
        acc = asBackground(step);
    }
    return CompositedCell{acc.Y, acc.Cb, acc.Cr};
}

}  // namespace scatter
