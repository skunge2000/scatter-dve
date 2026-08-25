// scatter-dve — WU-09: see core/splat.hpp for the design note and
// DECISIONS.md ADR-025 for the choices this file makes that
// architecture.md leaves open.
#include "core/splat.hpp"

#include <algorithm>
#include <cstdint>

namespace scatter {

TileAccum::TileAccum() {
    // std::array::fill() copies its argument into every element, giving
    // each bank its own independently-owned, already-zeroed vector (POD
    // AccumCell value-initialises to all-zero fields).
    banks_.fill(std::vector<AccumCell>(std::size_t(kTilePixels)));
}

void TileAccum::clear() noexcept {
    for (auto& bank : banks_) {
        std::fill(bank.begin(), bank.end(), AccumCell{});
    }
}

AccumCell& TileAccum::bank(int bankIdx, int x, int y) noexcept {
    return banks_[std::size_t(bankIdx)]
                 [std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

const AccumCell& TileAccum::bank(int bankIdx, int x, int y) const noexcept {
    return banks_[std::size_t(bankIdx)]
                 [std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

// Value-initialises every cell's KSlot array via KSlot's own default
// member initialisers (core/types.hpp) -- every slot starts unoccupied,
// the same "zero-initialised on construction" convention TileAccum uses.
TileKBufferAccum::TileKBufferAccum() : cells_(std::size_t(kTilePixels)) {}

void TileKBufferAccum::clear() noexcept {
    std::fill(cells_.begin(), cells_.end(), std::array<KSlot, kBufferK>{});
}

std::array<KSlot, kBufferK>& TileKBufferAccum::cell(int x, int y) noexcept {
    return cells_[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

const std::array<KSlot, kBufferK>& TileKBufferAccum::cell(int x, int y) const noexcept {
    return cells_[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

namespace {

// architecture.md 4.5's four corners, named to match its own prose exactly
// ("Bank A addressed with the base cell, B with base+1, C with
// base+stride, D with base+stride+1"). dx/dy are the offset from a
// fragment's base cell; bank is which of TileAccum's four banks that
// corner's contribution goes to.
constexpr int kBankA = 0, kBankB = 1, kBankC = 2, kBankD = 3;

struct Corner {
    int dx, dy, bank;
};

constexpr Corner kCorners[4] = {
    {0, 0, kBankA},
    {1, 0, kBankB},
    {0, 1, kBankC},
    {1, 1, kBankD},
};

// A fragment's tile-local destination, decoded back out of Frag::x/y
// (SubPos, 12.4 fixed) after undoing WU-08/ADR-024's one-pixel bias
// (encodeTileLocal() in core/binner.cpp biases every stored coordinate by
// +kSubPixelOne so a replica's position, up to one pixel negative relative
// to its new tile, still fits in an unsigned field).
//
// baseX/baseY are the integer cell the fragment's fractional position
// floors to, in the range [-1, kTileSize - 1]: -1 only for a replica whose
// footprint's low corner belongs to the tile on its other side (see
// splatCorners() below). fracX/fracY are the remaining sub-pixel fraction,
// in [0, kSubPixelOne), i.e. sixteenths of a pixel.
//
// Widening to int32_t before subtracting the bias is required, not
// defensive: SubPos is unsigned, and a replica's biased value can be
// smaller than kSubPixelOne, which would underflow in unsigned arithmetic.
// The right shift and bitwise-and below rely on C++20's guaranteed
// two's-complement, floor-rounding signed right shift to decompose a
// possibly-negative value into (floor, non-negative remainder) in one
// step, the same trick as an unsigned fixed-point decode but signed.
struct DecodedPos {
    std::int32_t baseX, baseY;
    std::int32_t fracX, fracY;
};

DecodedPos decode(SubPos x, SubPos y) noexcept {
    const std::int32_t ux = std::int32_t(x) - std::int32_t(kSubPixelOne);
    const std::int32_t uy = std::int32_t(y) - std::int32_t(kSubPixelOne);
    DecodedPos d{};
    d.baseX = ux >> kSubPixelBits;
    d.baseY = uy >> kSubPixelBits;
    d.fracX = ux & (kSubPixelOne - 1);
    d.fracY = uy & (kSubPixelOne - 1);
    return d;
}

// One fragment's contribution to one corner's AccumCell, given that
// corner's bilinear weight `rawWeight` (0..kSubPixelOne*kSubPixelOne,
// i.e. 0..256 — the product of the two 4-bit axis weights below). This is
// the ONE place fragment -> accumulator arithmetic is written; splatTile()
// and splatTileReference() both call it, through splatCorners(), so they
// cannot drift apart.
//
// Right-shifting by 2*kSubPixelBits (8) after the multiply divides by
// kSubPixelOne^2 (256), the bilinear weight's fixed-point scale; any
// fractional remainder below one part in 256 truncates towards zero
// (per-corner, not across a cell's many contributors), which is
// deterministic and order-independent, all I6 requires -- it does not
// require exact weight conservation across a splat's four corners.
//
// Overflow: frag.w and each colour channel are already bounded by
// core/types.hpp's kMaxFragContribution static_asserts (R or G or B
// (renamed from Y/Cb/Cr, WU-39), times w, both up to 65535); rawWeight
// <= 256 only ever *reduces* that
// product before the shift restores the scale, so no corner-write here
// can exceed kMaxFragContribution regardless of rawWeight. The intermediate
// product before the shift, up to 65535 * 65535 * 256 (~1.1e12), and the
// running sum across many fragments in one cell (I4's whole reason for
// int64) both stay well inside int64's range -- see test_splat.cpp's
// synthetic worst case, matching CORRECTIONS.md C-001.
void accumulateCorner(AccumCell& cell, const Frag& f,
                       std::int32_t rawWeight) noexcept {
    const std::int64_t w  = std::int64_t(f.w);
    const std::int64_t rw = std::int64_t(rawWeight);
    const std::int64_t weightContribution = (w * rw) >> (2 * kSubPixelBits);

    cell.R += (std::int64_t(f.R) * w * rw) >> (2 * kSubPixelBits);
    cell.G += (std::int64_t(f.G) * w * rw) >> (2 * kSubPixelBits);
    cell.B += (std::int64_t(f.B) * w * rw) >> (2 * kSubPixelBits);
    cell.w += WeightAccum(weightContribution);
}

// Visits the (up to four) corners of one fragment's 2x2 splat footprint
// that fall inside this tile's own [0, kTileSize) x [0, kTileSize) range,
// and calls sink(bank, cellX, cellY, rawWeight) for each.
//
// A corner outside that range is simply skipped -- not clamped, not
// wrapped. This is the direct, necessary consequence of WU-08's already-
// frozen replication scheme (ADR-024), not an independent choice: every
// fragment's footprint can only ever reach one pixel beyond its base cell
// (4.5's base+1/base+stride/base+stride+1), and core/binner.cpp already
// replicates any fragment whose footprint would cross into a neighbouring
// tile into that neighbour's own bin, with tile-local coordinates recomputed
// relative to it. So of the two fragment copies (or four, at a tile's
// corner) that can result, each one's footprint corners that fall outside
// its OWN tile belong to a copy in a different tile's bin instead -- or,
// at the destination raster's own edge, to no valid destination pixel at
// all, since there was no neighbour to replicate into and the true 2x2
// footprint genuinely extends past the raster boundary there. Either way,
// nothing here needs writing. See ADR-025.
void splatCorners(const Frag& f, auto&& sink) {
    const DecodedPos d = decode(f.x, f.y);
    for (const Corner& c : kCorners) {
        const std::int32_t cellX = d.baseX + c.dx;
        const std::int32_t cellY = d.baseY + c.dy;
        if (cellX < 0 || cellX >= kTileSize || cellY < 0 || cellY >= kTileSize) {
            continue;
        }
        const std::int32_t wx = (c.dx == 0) ? (kSubPixelOne - d.fracX) : d.fracX;
        const std::int32_t wy = (c.dy == 0) ? (kSubPixelOne - d.fracY) : d.fracY;
        sink(c.bank, int(cellX), int(cellY), wx * wy);
    }
}

// One fragment's contribution to one cell's k-buffer, per ADR-059's tag
// routing (WU-28a): find `f.tag`'s existing slot and accumulate into it
// via accumulateCorner() above (unchanged, order-independent, I6); else
// claim a free slot; else evict the farthest-so-far occupied slot (by
// firstSeenZ -- larger is farther, Frag::z's own "near = 0" convention)
// and claim that one instead. The one place this policy is written.
void routeIntoKBuffer(std::array<KSlot, kBufferK>& slots, const Frag& f,
                      std::int32_t rawWeight) noexcept {
    for (KSlot& slot : slots) {
        if (slot.occupied && slot.tag == f.tag) {
            accumulateCorner(slot.cell, f, rawWeight);
            return;
        }
    }
    for (KSlot& slot : slots) {
        if (!slot.occupied) {
            slot.occupied = true;
            slot.tag = f.tag;
            slot.firstSeenZ = f.z;
            slot.cell = AccumCell{};
            accumulateCorner(slot.cell, f, rawWeight);
            return;
        }
    }
    // All kBufferK slots occupied by other tags: evict the farthest-so-far
    // (ADR-059's accepted caveat -- a firstSeenZ tie resolves to whichever
    // this scan reaches first: deterministic per run, not claimed
    // order-independent across fragment orders).
    KSlot* farthest = &slots[0];
    for (KSlot& slot : slots) {
        if (slot.firstSeenZ > farthest->firstSeenZ) {
            farthest = &slot;
        }
    }
    farthest->occupied = true;
    farthest->tag = f.tag;
    farthest->firstSeenZ = f.z;
    farthest->cell = AccumCell{};
    accumulateCorner(farthest->cell, f, rawWeight);
}

}  // namespace

void splatTile(const std::vector<Frag>& frags, TileAccum& accum) {
    for (const Frag& f : frags) {
        splatCorners(f, [&](int bankIdx, int x, int y, std::int32_t rawWeight) {
            accumulateCorner(accum.bank(bankIdx, x, y), f, rawWeight);
        });
    }
}

void sumBanks(const TileAccum& accum, AccumCell* out) {
    for (int y = 0; y < kTileSize; ++y) {
        for (int x = 0; x < kTileSize; ++x) {
            AccumCell sum{};
            for (int b = 0; b < kBanks; ++b) {
                const AccumCell& c = accum.bank(b, x, y);
                sum.R += c.R;
                sum.G += c.G;
                sum.B += c.B;
                sum.w += c.w;
            }
            out[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)] = sum;
        }
    }
}

void splatTileReference(const std::vector<Frag>& frags, AccumCell* out) {
    for (const Frag& f : frags) {
        splatCorners(f, [&](int bankIdx, int x, int y, std::int32_t rawWeight) {
            (void)bankIdx;  // single accumulator: every corner lands in the
                             // same array regardless of which bank it would
                             // have used in splatTile().
            accumulateCorner(out[std::size_t(y) * std::size_t(kTileSize) +
                                  std::size_t(x)],
                              f, rawWeight);
        });
    }
}

void splatTileKBuffer(const std::vector<Frag>& frags, TileKBufferAccum& accum) {
    for (const Frag& f : frags) {
        splatCorners(f, [&](int bankIdx, int x, int y, std::int32_t rawWeight) {
            (void)bankIdx;  // no bank split for the k-buffer -- see
                             // TileKBufferAccum's own comment in splat.hpp.
            // A rawWeight-0 corner (the other three corners of an
            // exact-grid-position fragment, or any fragment whose
            // fractional position is exactly 0 on one axis) is harmless
            // in the plain path -- accumulateCorner() adds zero to an
            // AccumCell that may never be looked at again. It is NOT
            // harmless here: routeIntoKBuffer() below would still claim a
            // free slot (or evict an occupied one) for a tag that made no
            // real contribution to this cell. Skip zero-weight corners
            // entirely so a k-buffer touch always means a genuine one.
            if (rawWeight == 0) return;
            routeIntoKBuffer(accum.cell(x, y), f, rawWeight);
        });
    }
}

void sumBanksKBuffer(const TileKBufferAccum& accum, std::array<KSlot, kBufferK>* out) {
    for (int y = 0; y < kTileSize; ++y) {
        for (int x = 0; x < kTileSize; ++x) {
            out[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)] = accum.cell(x, y);
        }
    }
}

}  // namespace scatter
