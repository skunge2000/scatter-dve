// scatter-dve — WU-09: the four-bank splat (architecture.md section 4.5;
// module layout section 8 names this core/splat.hpp/.cpp)
//
// This is pass 2's first half (section 3's signal path: "PASS 2: tile
// resolve -> four-bank splat -> accumulate -> normalise"). It consumes
// exactly what WU-08's core/binner.hpp produces -- one std::vector<Frag>
// per tile, from TileBins::tile() -- and turns it into one fully-summed
// kTileSize x kTileSize grid of AccumCell per tile. Normalising (dividing
// by accumulated weight) and compositing are WU-10's core/resolve.hpp/.cpp,
// a different unit and a different "resolve" than 4.5's bank-resolve this
// file implements -- see sumBanks() below, deliberately not named
// resolve() to keep the two apart for whoever writes WU-10.
//
// architecture.md 4.5, verbatim on the mechanism:
//
//   Bank A addressed with the base cell, B with base+1, C with
//   base+stride, D with base+stride+1. Each fragment performs exactly one
//   read-modify-write per bank. On resolve, all four banks are addressed
//   identically and summed. This reconstitutes the full 2x2 footprint for
//   every output cell.
//
// "base" is a fragment's own tile-local integer destination cell; "stride"
// is the Y axis (base+stride is the row below). Reading 4.5 together with
// section 13's provenance note -- "forward scatter with fractional
// addresses supplying splat weights" -- fixes what 4.5 alone leaves open:
// a fragment's sub-pixel *fractional* position (core/types.hpp's SubPos,
// carried in Frag::x/y) supplies bilinear weights across the four corners,
// not a full, unweighted copy of the fragment into all four. ADR-025 in
// DECISIONS.md records this and the two other concrete choices
// architecture.md leaves open here: the fixed-point bilinear weight
// formula, and what happens when a corner falls outside this tile (it is
// simply not written -- see splat.cpp).
//
// Integer arithmetic throughout (I4, I6): this is the fixed-point
// accumulation path proper, unlike core/binner.cpp's fragment *generation*
// which stays in double until the point a Frag is built. Never introduce
// floating point here.
#pragma once

#include "core/types.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace scatter {

// Per-tile four-bank accumulator storage (architecture.md 4.5's table):
// one full kTileSize x kTileSize grid of AccumCell per bank, four banks,
// each independently addressable so a fragment's four read-modify-writes
// -- one per bank -- do not serialise on each other. The modern equivalent
// of the patent's four parallel framestores (C-002 / ADR-002): the
// serialisation problem, not the bandwidth problem, is what the bank split
// solves; tile binning (WU-08) already solved bandwidth by making the
// accumulator cache-resident.
//
// Zero-initialised on construction. Not resized after construction --
// kTileSize is a compile-time constant (SCATTER_TILE_LOG2), so one
// TileAccum always matches the tile size the whole binary was built with.
class TileAccum {
public:
    TileAccum();

    // Zeroes all four banks, e.g. for reuse across tiles (WU-16's
    // per-worker bin arenas will want this).
    void clear() noexcept;

    // bankIdx in [0, kBanks), x and y in [0, kTileSize). Out-of-range
    // arguments are the caller's bug, not checked here -- same convention
    // as Lattice::at() (core/lattice.hpp).
    AccumCell&       bank(int bankIdx, int x, int y) noexcept;
    const AccumCell& bank(int bankIdx, int x, int y) const noexcept;

private:
    std::array<std::vector<AccumCell>, kBanks> banks_;
};

// Splats every fragment in `frags` (one tile's bin, e.g. from
// TileBins::tile() -- see core/binner.hpp) into `accum`'s four banks, per
// architecture.md 4.5 and ADR-025. Does not clear `accum` first: call
// TileAccum::clear() between tiles if reusing one, the same convention
// TileBins itself does not impose clearing between frames.
void splatTile(const std::vector<Frag>& frags, TileAccum& accum);

// architecture.md 4.5: "on resolve, all four banks are addressed
// identically and summed" -- reconstitutes each output cell's true total
// from the (up to) four banks its contributing fragments' corners were
// spread across. `out` must point to at least kTilePixels AccumCells,
// row-major (y * kTileSize + x); every entry is overwritten, not
// accumulated into.
void sumBanks(const TileAccum& accum, AccumCell* out);

// The oracle WORK-UNITS.md's WU-09 accept line asks for: "four-bank result
// identical to a single-accumulator reference implementation". Same
// fragment -> weighted-corner-contribution arithmetic as
// splatTile()/sumBanks() (both call the same internal helper in splat.cpp,
// so there is exactly one place that arithmetic is written), but against a
// single kTileSize x kTileSize accumulator with no bank split at all.
// Integer addition is associative (I6), so this must be bit-identical to
// splatTile() followed by sumBanks() for the same fragments regardless of
// fragment order or how many fragments land in a shared cell via different
// banks.
//
// `out` must point to at least kTilePixels AccumCells, already zeroed
// (e.g. value-initialised, `std::vector<AccumCell>(kTilePixels)`) --
// unlike sumBanks(), this function accumulates into `out` (read-modify-
// write, matching a real single-accumulator implementation), it does not
// overwrite it.
void splatTileReference(const std::vector<Frag>& frags, AccumCell* out);

// ---------------------------------------------------------------------------
// K-buffer accumulation (WU-28a; DECISIONS.md ADR-059) -- storage/
// accumulation only, new and additive alongside -- never replacing --
// TileAccum/splatTile()/sumBanks() above. Resolve/composite (depth-sorting
// the occupied slots, opaque vs blend outcomes) is WU-28b's own unit,
// consuming this one's output; nothing here decides how a multi-tag cell
// is finally composited.
// ---------------------------------------------------------------------------

// Per-tile k-buffer storage: one array of up to kBufferK tag-keyed KSlot
// per cell, kTilePixels cells. Unlike TileAccum, NOT split across kBanks
// independent banks: the plain path's four-bank split exists to pipeline
// store-to-load latency (ADR-002), a performance concern ADR-059's own
// "correctness first" scope does not need yet -- and banking a
// tag-keyed, eviction-order-sensitive structure would additionally
// require a cross-bank merge order ADR-059 never specifies. Still keeps
// the plain path's two-step accumulate/extract-to-flat-array shape below,
// so a later unit could add banking without changing either contract.
class TileKBufferAccum {
public:
    TileKBufferAccum();

    // Zeroes all cells (all slots unoccupied) -- same convention as
    // TileAccum::clear().
    void clear() noexcept;

    // x and y in [0, kTileSize); out-of-range is the caller's bug, not
    // checked here, same convention as TileAccum::bank().
    std::array<KSlot, kBufferK>&       cell(int x, int y) noexcept;
    const std::array<KSlot, kBufferK>& cell(int x, int y) const noexcept;

private:
    std::vector<std::array<KSlot, kBufferK>> cells_;
};

// Splats every fragment in `frags` into `accum`, tag-routed per ADR-059: a
// fragment whose tag already occupies a slot accumulates into it via
// exactly accumulateCorner() (unchanged, order-independent, I6); a
// fragment whose tag has no slot claims a free one; if none are free, the
// farthest-so-far occupied slot (by first-seen z) is evicted and claimed
// instead -- ADR-059's accepted non-order-independence caveat for the
// rare >kBufferK-distinct-tags case, not something this function fixes.
// Does not clear `accum` first, same convention as splatTile().
void splatTileKBuffer(const std::vector<Frag>& frags, TileKBufferAccum& accum);

// Copies each cell's up to kBufferK KSlot entries out to `out`, row-major
// (y * kTileSize + x), overwritten not accumulated into (sumBanks()'s own
// contract shape) even though there is only one internal store to copy
// from -- see TileKBufferAccum's own comment. `out` must point to at
// least kTilePixels entries.
void sumBanksKBuffer(const TileKBufferAccum& accum, std::array<KSlot, kBufferK>* out);

}  // namespace scatter
