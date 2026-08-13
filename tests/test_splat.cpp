// WU-09: the four-bank splat (architecture.md 4.5; ADR-025 for the choices
// left open).
//
// Checks WORK-UNITS.md's WU-09 accept criteria directly:
//
// 1. "Four-bank result identical to a single-accumulator reference
//    implementation": test_splatTile_matches_reference_random() drives a
//    large randomised batch of fragments -- interior, tile-edge and
//    replica-style (base cell == -1) positions all included -- through
//    both splatTile()+sumBanks() and splatTileReference(), and requires
//    every cell of every field to match exactly.
// 2. "int64 headroom verified at synthetic worst case": two tests, at two
//    scales. test_int64_headroom_full_pipeline() drives 25000 full-weight,
//    same-cell fragments through the real splatTile()/splatTileReference()
//    code path -- enough to overflow a hypothetical uint32 colour
//    accumulator many thousands of times over, while staying inside
//    AccumCell::w's int32 range (see CORRECTIONS.md's new entry this
//    session for why the literal "a million" from types.hpp's
//    kMaxFragContribution comment is not driven through the shared
//    Frag/weight-accumulator code path directly).
//    test_int64_headroom_million_fragment_arithmetic() then checks the
//    exact literal claim -- a million kMaxFragContribution-sized additions
//    -- against ColourAccum's own arithmetic in isolation.
//
// The remaining tests pin down ADR-025's concrete choices (bilinear
// corner weights, tile-edge corner dropping) against hand-computed
// expectations, independent of splat.cpp's own arithmetic, the same
// "don't check against the unit's own logic" discipline test_binner.cpp
// uses.

#include "core/splat.hpp"
#include "harness.hpp"

#include <cstdint>
#include <random>
#include <vector>

using namespace scatter;

namespace {

// Inverse of core/binner.cpp's encodeTileLocal(): packs a tile-local base
// cell (may be -1, a replica's footprint corner belonging to the tile on
// its other side -- see ADR-024) and a 0..kSubPixelOne-1 sub-pixel
// fraction into one biased SubPos, matching WU-08's own encoding exactly
// so splat.cpp's decode() is being exercised the same way real binner
// output would drive it.
SubPos encodeLocal(int base, int frac) noexcept {
    return SubPos((base + 1) * kSubPixelOne + frac);
}

Frag makeFrag(int baseX, int fracX, int baseY, int fracY, Sample y, Sample cb,
              Sample cr, Weight w) noexcept {
    Frag f{};
    f.x = encodeLocal(baseX, fracX);
    f.y = encodeLocal(baseY, fracY);
    f.Y = y;
    f.Cb = cb;
    f.Cr = cr;
    f.w = w;
    f.z = 0;
    f.tag = 0;
    f.reserved = 0;
    return f;
}

AccumCell cellAt(const AccumCell* grid, int x, int y) noexcept {
    return grid[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

}  // namespace

// --- exact grid position: all weight lands in one cell, via one corner --

static void test_exact_grid_fragment_lands_in_one_cell() {
    // Comfortably interior at either compile-time tile size (16 or 32).
    const int bx = 5, by = 5;
    const Frag f = makeFrag(bx, 0, by, 0, /*y=*/Sample(1000), /*cb=*/Sample(2000),
                             /*cr=*/Sample(3000), /*w=*/Weight(kWeightUnity));
    std::vector<Frag> frags{f};

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());

    // rawWeight is exactly 256 (16*16) at frac (0, 0) -- the >>8 in
    // accumulateCorner() is then an exact divide, no truncation.
    const AccumCell& c = out[std::size_t(by) * std::size_t(kTileSize) + std::size_t(bx)];
    CHECK(c.Y == ColourAccum(1000) * ColourAccum(kWeightUnity));
    CHECK(c.Cb == ColourAccum(2000) * ColourAccum(kWeightUnity));
    CHECK(c.Cr == ColourAccum(3000) * ColourAccum(kWeightUnity));
    CHECK(c.w == WeightAccum(kWeightUnity));

    // No bleed into any neighbour, including the three other corners of
    // this same fragment's nominal 2x2 footprint (each got rawWeight 0).
    CHECK(cellAt(out.data(), bx + 1, by).w == 0);
    CHECK(cellAt(out.data(), bx, by + 1).w == 0);
    CHECK(cellAt(out.data(), bx + 1, by + 1).w == 0);
    CHECK(cellAt(out.data(), bx - 1, by).w == 0);
}

// --- half-pixel position: bilinear split divides evenly across all four -

static void test_bilinear_split_at_half_pixel_divides_evenly() {
    const int bx = 4, by = 4;
    // kWeightUnity (32768) is divisible by 4, so at frac (8, 8) -- an
    // exact quarter to each corner, rawWeight 64 of 256 -- every corner's
    // >>8 divide is exact too, with nothing to round away.
    const Frag f = makeFrag(bx, 8, by, 8, /*y=*/Sample(4000), /*cb=*/Sample(0),
                             /*cr=*/Sample(0), /*w=*/Weight(kWeightUnity));
    std::vector<Frag> frags{f};

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());

    const WeightAccum quarterW = WeightAccum(kWeightUnity / 4);
    const ColourAccum quarterY = ColourAccum(4000) * ColourAccum(kWeightUnity) / 4;

    CHECK(cellAt(out.data(), bx, by).w == quarterW);
    CHECK(cellAt(out.data(), bx + 1, by).w == quarterW);
    CHECK(cellAt(out.data(), bx, by + 1).w == quarterW);
    CHECK(cellAt(out.data(), bx + 1, by + 1).w == quarterW);
    CHECK(cellAt(out.data(), bx, by).Y == quarterY);
    CHECK(cellAt(out.data(), bx + 1, by + 1).Y == quarterY);

    // Perfect divisibility here means no weight is lost to truncation:
    // the four corners sum back to exactly the fragment's own weight.
    const WeightAccum total =
        WeightAccum(cellAt(out.data(), bx, by).w + cellAt(out.data(), bx + 1, by).w +
                     cellAt(out.data(), bx, by + 1).w + cellAt(out.data(), bx + 1, by + 1).w);
    CHECK(total == WeightAccum(kWeightUnity));
}

// --- corners that fall outside this tile are dropped, not clamped ------

// A fragment whose base cell is the tile's last column: its base+1 corner
// (architecture.md 4.5's "bank B") would land at column kTileSize, outside
// this tile's own accumulator. ADR-025: simply not written -- either
// core/binner.cpp already replicated this fragment into the neighbour
// tile's own bin (a straddling case), or there was no neighbour (the
// raster's own edge) and that corner never corresponded to a real
// destination pixel. Either way, splat.cpp has no cell here to write to,
// and this test's job is only to confirm that "no cell to write to" means
// "silently contributes nothing", not a crash or a wrapped/clamped index.
static void test_out_of_bounds_corner_dropped_at_tile_high_edge() {
    const int bx = kTileSize - 1, by = 5;
    const Frag f = makeFrag(bx, /*fracX=*/8, by, /*fracY=*/0, /*y=*/Sample(100),
                             /*cb=*/Sample(0), /*cr=*/Sample(0), /*w=*/Weight(40000));
    std::vector<Frag> frags{f};

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());

    // rawWeight at (dx=0,dy=0) is (16-8)*(16-0) = 128 of 256, i.e. exactly
    // half -- the only corner inside this tile's bounds with any weight
    // (dy=1's corners get rawWeight 0 from fracY == 0, and dx=1's corners
    // are off the tile regardless of weight).
    const std::int64_t expectedWeight = std::int64_t(40000) * 128 / 256;
    CHECK(cellAt(out.data(), bx, by).w == WeightAccum(expectedWeight));
    CHECK(cellAt(out.data(), bx, by + 1).w == 0);  // rawWeight 0, not dropped

    // Total weight landed anywhere in the tile is exactly the one valid
    // corner's contribution: half of the fragment's weight is genuinely
    // lost off the tile edge, not redistributed or clamped in.
    std::int64_t totalW = 0;
    for (int i = 0; i < kTilePixels; ++i) totalW += out[std::size_t(i)].w;
    CHECK(totalW == expectedWeight);
}

// Mirror of the above on the low side: base cell -1 is exactly what a
// replica fragment's footprint corner looks like once it has crossed into
// the tile it was replicated into (ADR-024) -- so this also stands in for
// "splat.cpp correctly decodes a replica's biased-but-unbiased-to-negative
// position", not just an isolated edge case.
static void test_out_of_bounds_corner_dropped_at_tile_low_edge() {
    const int by = 5;
    const Frag f = makeFrag(/*baseX=*/-1, /*fracX=*/8, by, /*fracY=*/0, /*y=*/Sample(100),
                             /*cb=*/Sample(0), /*cr=*/Sample(0), /*w=*/Weight(40000));
    std::vector<Frag> frags{f};

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());

    const std::int64_t expectedWeight = std::int64_t(40000) * 128 / 256;
    CHECK(cellAt(out.data(), 0, by).w == WeightAccum(expectedWeight));

    std::int64_t totalW = 0;
    for (int i = 0; i < kTilePixels; ++i) totalW += out[std::size_t(i)].w;
    CHECK(totalW == expectedWeight);
}

// --- resolve sums contributions from different fragments' corners ------

// The actual point of the four-bank design: two different fragments, each
// contributing to the SAME output cell via a different corner of their own
// footprint (and so, inside splatTile(), via different banks). Checked
// purely through the public splatTile()/sumBanks() API -- which bank
// number either contribution actually used is splat.cpp's own business,
// not part of the contract.
static void test_cross_fragment_summation_matches_hand_computation() {
    const Frag f1 = makeFrag(5, 0, 5, 0, /*y=*/Sample(1000), Sample(0), Sample(0),
                              /*w=*/Weight(kWeightUnity));
    // Its "base+1" corner (dx=1, dy=0) lands on cell (5, 5) with rawWeight
    // 15 * 16 = 240.
    const Frag f2 = makeFrag(4, 15, 5, 0, /*y=*/Sample(500), Sample(0), Sample(0),
                              /*w=*/Weight(kWeightUnity));
    std::vector<Frag> frags{f1, f2};

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());

    const ColourAccum contrib1 = ColourAccum(1000) * ColourAccum(kWeightUnity) * 256 / 256;
    const ColourAccum contrib2 = ColourAccum(500) * ColourAccum(kWeightUnity) * 240 / 256;
    const AccumCell& c = out[std::size_t(5) * std::size_t(kTileSize) + std::size_t(5)];
    CHECK(c.Y == contrib1 + contrib2);
}

// --- TileAccum::clear() ------------------------------------------------

static void test_clear_zeroes_all_banks() {
    TileAccum accum;
    std::vector<Frag> frags{makeFrag(3, 0, 3, 0, Sample(1), Sample(2), Sample(3),
                                      Weight(kWeightUnity))};
    splatTile(frags, accum);
    accum.clear();
    std::vector<AccumCell> out((std::size_t(kTilePixels)));
    sumBanks(accum, out.data());
    for (int i = 0; i < kTilePixels; ++i) {
        CHECK_ONCE(out[std::size_t(i)].Y == 0);
        CHECK_ONCE(out[std::size_t(i)].w == 0);
    }
}

// --- accept criterion 1: four-bank result == single-accumulator result -

static void test_splatTile_matches_reference_random() {
    std::mt19937 rng(0x5CA77E12u);
    std::uniform_int_distribution<int> baseDist(-1, kTileSize - 1);
    std::uniform_int_distribution<int> fracDist(0, kSubPixelOne - 1);
    std::uniform_int_distribution<int> sampleDist(0, 65535);
    std::uniform_int_distribution<int> weightDist(0, 65535);

    std::vector<Frag> frags;
    const int kCount = 50000;
    frags.reserve(std::size_t(kCount));
    for (int i = 0; i < kCount; ++i) {
        frags.push_back(makeFrag(baseDist(rng), fracDist(rng), baseDist(rng),
                                  fracDist(rng), Sample(sampleDist(rng)),
                                  Sample(sampleDist(rng)), Sample(sampleDist(rng)),
                                  Weight(weightDist(rng))));
    }

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> banked((std::size_t(kTilePixels)));
    sumBanks(accum, banked.data());

    std::vector<AccumCell> reference((std::size_t(kTilePixels)));  // zero-initialised
    splatTileReference(frags, reference.data());

    for (int i = 0; i < kTilePixels; ++i) {
        const AccumCell& a = banked[std::size_t(i)];
        const AccumCell& b = reference[std::size_t(i)];
        CHECK_ONCE(a.Y == b.Y);
        CHECK_ONCE(a.Cb == b.Cb);
        CHECK_ONCE(a.Cr == b.Cr);
        CHECK_ONCE(a.w == b.w);
    }
}

// --- accept criterion 2a: int64 headroom through the real splat path ---

// 25000 full-weight fragments, all landing exactly on one cell (so every
// one contributes the full kMaxFragContribution to that cell's colour
// accumulators via a single corner, rawWeight 256, no truncation). Chosen
// as a scale that is (a) already far beyond what a 32-bit colour
// accumulator could hold -- kMaxFragContribution alone is one uint32 short
// of overflowing it twice over (C-001) -- while (b) staying under
// AccumCell::w's int32 capacity at full weight (INT32_MAX / 65535 =~
// 32768); see CORRECTIONS.md for why this test does not push weight all
// the way to the "a million" scale kMaxFragContribution's own comment in
// core/types.hpp uses for colour.
static void test_int64_headroom_full_pipeline() {
    const int kCount = 25000;
    const int bx = kTileSize / 2, by = kTileSize / 2;
    std::vector<Frag> frags;
    frags.reserve(std::size_t(kCount));
    for (int i = 0; i < kCount; ++i) {
        frags.push_back(makeFrag(bx, 0, by, 0, /*y=*/65535, /*cb=*/65535,
                                  /*cr=*/65535, /*w=*/Weight(kWeightMax)));
    }

    TileAccum accum;
    splatTile(frags, accum);
    std::vector<AccumCell> banked((std::size_t(kTilePixels)));
    sumBanks(accum, banked.data());

    std::vector<AccumCell> reference((std::size_t(kTilePixels)));
    splatTileReference(frags, reference.data());

    const ColourAccum expectedColour = ColourAccum(kCount) * kMaxFragContribution;
    const WeightAccum expectedWeight = WeightAccum(kCount) * WeightAccum(kWeightMax);

    const AccumCell& banked_c = banked[std::size_t(by) * std::size_t(kTileSize) + std::size_t(bx)];
    const AccumCell& ref_c = reference[std::size_t(by) * std::size_t(kTileSize) + std::size_t(bx)];

    CHECK(banked_c.Y == expectedColour);
    CHECK(banked_c.Cb == expectedColour);
    CHECK(banked_c.Cr == expectedColour);
    CHECK(banked_c.w == expectedWeight);
    CHECK(ref_c.Y == expectedColour);
    CHECK(ref_c.w == expectedWeight);

    // The point: this is already far beyond a hypothetical 32-bit colour
    // accumulator's range, many times over -- exactly what I4/C-001 say
    // int64 is for.
    CHECK(expectedColour > ColourAccum(INT32_MAX));
    CHECK(expectedColour > ColourAccum(UINT32_MAX));
    // And weight stayed safely inside its own (smaller, int32) capacity,
    // by design of this test's chosen count -- not by accident.
    CHECK(expectedWeight > 0);  // did not wrap negative
    CHECK(std::int64_t(expectedWeight) < std::int64_t(INT32_MAX));
}

// --- accept criterion 2b: the literal "a million" claim, in isolation --

// core/types.hpp: "static_assert(kMaxFragContribution <= INT64_MAX /
// 1000000, ...)" -- a compile-time claim about ColourAccum's own capacity,
// independent of AccumCell::w. This test drives that exact claim at
// runtime, through the identical arithmetic shape splat.cpp's
// accumulateCorner() uses (multiply-then-implicit-accumulate), but against
// a bare ColourAccum rather than routing a million real Frags through the
// shared weight-accumulator code path -- see CORRECTIONS.md for why.
static void test_int64_headroom_million_fragment_arithmetic() {
    ColourAccum acc = 0;
    const std::int64_t kCount = 1'000'000;
    for (std::int64_t i = 0; i < kCount; ++i) {
        acc += kMaxFragContribution;
    }
    CHECK(acc == kCount * kMaxFragContribution);
    CHECK(acc > ColourAccum(INT32_MAX));
    CHECK(acc > ColourAccum(UINT32_MAX));
    // Comfortably inside int64 (~4.29e15 vs ~9.22e18), with room to spare
    // -- the "headroom" WORK-UNITS.md's accept line asks for, not just a
    // near miss.
    CHECK(acc < INT64_MAX / 1000);
}

int main() {
    test_exact_grid_fragment_lands_in_one_cell();
    test_bilinear_split_at_half_pixel_divides_evenly();
    test_out_of_bounds_corner_dropped_at_tile_high_edge();
    test_out_of_bounds_corner_dropped_at_tile_low_edge();
    test_cross_fragment_summation_matches_hand_computation();
    test_clear_zeroes_all_banks();
    test_splatTile_matches_reference_random();
    test_int64_headroom_full_pipeline();
    test_int64_headroom_million_fragment_arithmetic();
    return scatter::test::summary("test_splat");
}
