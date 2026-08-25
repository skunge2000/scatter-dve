// WU-28a: k-buffer storage, tag-keyed depth slots (DECISIONS.md ADR-059;
// see core/splat.hpp for the design note). Storage/accumulation only --
// resolve/composite (WU-28b) is not exercised here.
//
// Checks WORK-UNITS.md's WU-28a accept criteria:
// 1. "A cell where only one tag is ever present reproduces today's plain
//    sumBanks() AccumCell exactly": test_single_tag_matches_plain_sumBanks().
// 2. "same-tag ties at identical quantised z are order-independent across
//    fragment orders": test_same_tag_tied_z_order_independent().
// 3. "the >kBufferK-distinct-tags eviction case ... checked only for
//    run-to-run self-consistency at a given thread count, not against an
//    independent oracle": test_eviction_self_consistent_not_oracle_checked().
//    splatTileKBuffer()/sumBanksKBuffer() are not wired into runFrame()'s
//    thread pool yet (WU-28b's own job), so "a given thread count" is
//    stood in for here by "a given fragment processing order", the same
//    ordering discipline test_splat.cpp's own
//    test_splatTile_matches_reference_random() uses -- applied to repeated
//    runs of one fixed order rather than to which tags survive eviction
//    (the thing ADR-059 explicitly does not promise).
//
// All fragments below land at an exact grid position (fracX = fracY = 0),
// the same simplification test_splat.cpp's own
// test_exact_grid_fragment_lands_in_one_cell() uses: rawWeight is then
// exactly 256, so accumulateCorner()'s shift has no truncation and R/G/B
// contributions reduce to plain R*w/G*w/B*w -- letting these tests check
// accumulated values by hand without reproducing the bilinear-weight
// formula itself (already test_splat.cpp's own job).

#include "core/splat.hpp"
#include "harness.hpp"

#include <array>
#include <vector>

using namespace scatter;

namespace {

// Matches core/binner.cpp's encodeTileLocal() bias, same as test_splat.cpp's
// own file-local encodeLocal() helper.
SubPos encodeLocal(int base, int frac) noexcept {
    return SubPos((base + 1) * kSubPixelOne + frac);
}

Frag makeExactFrag(int bx, int by, Sample y, Sample cb, Sample cr, Weight w,
                    std::uint16_t z, std::uint8_t tag) noexcept {
    Frag f{};
    f.x = encodeLocal(bx, 0);
    f.y = encodeLocal(by, 0);
    f.R = y;
    f.G = cb;
    f.B = cr;
    f.w = w;
    f.z = z;
    f.tag = tag;
    f.reserved = 0;
    return f;
}

std::array<KSlot, kBufferK> kbufferCellAt(const std::array<KSlot, kBufferK>* grid, int x,
                                           int y) noexcept {
    return grid[std::size_t(y) * std::size_t(kTileSize) + std::size_t(x)];
}

int occupiedCount(const std::array<KSlot, kBufferK>& slots) noexcept {
    int n = 0;
    for (const KSlot& s : slots) {
        if (s.occupied) ++n;
    }
    return n;
}

const KSlot* findSlotForTag(const std::array<KSlot, kBufferK>& slots, std::uint8_t tag) noexcept {
    for (const KSlot& s : slots) {
        if (s.occupied && s.tag == tag) return &s;
    }
    return nullptr;
}

}  // namespace

// --- accept 1: single-tag cell matches plain sumBanks() exactly --------

static void test_single_tag_matches_plain_sumBanks() {
    const int bx = 5, by = 5;
    const std::uint8_t tag = 3;
    std::vector<Frag> frags{
        makeExactFrag(bx, by, Sample(1000), Sample(2000), Sample(3000), Weight(kWeightUnity),
                      /*z=*/100, tag),
        makeExactFrag(bx, by, Sample(500), Sample(100), Sample(50), Weight(10000), /*z=*/200, tag),
        makeExactFrag(bx, by, Sample(0), Sample(0), Sample(0), Weight(5000), /*z=*/300, tag),
    };

    TileAccum plainAccum;
    splatTile(frags, plainAccum);
    std::vector<AccumCell> plainOut((std::size_t(kTilePixels)));
    sumBanks(plainAccum, plainOut.data());
    const AccumCell& plainCell = plainOut[std::size_t(by) * std::size_t(kTileSize) + std::size_t(bx)];

    TileKBufferAccum kAccum;
    splatTileKBuffer(frags, kAccum);
    std::vector<std::array<KSlot, kBufferK>> kOut((std::size_t(kTilePixels)));
    sumBanksKBuffer(kAccum, kOut.data());
    const std::array<KSlot, kBufferK> kCell = kbufferCellAt(kOut.data(), bx, by);

    CHECK(occupiedCount(kCell) == 1);
    const KSlot* slot = findSlotForTag(kCell, tag);
    CHECK(slot != nullptr);
    if (slot == nullptr) return;

    // Same accumulateCorner() arithmetic, same fragments/order -- exact.
    CHECK(slot->cell.R == plainCell.R);
    CHECK(slot->cell.G == plainCell.G);
    CHECK(slot->cell.B == plainCell.B);
    CHECK(slot->cell.w == plainCell.w);
    CHECK(slot->firstSeenZ == 100);  // first fragment's own z, arrival order

    CHECK(occupiedCount(kbufferCellAt(kOut.data(), bx + 1, by)) == 0);
    CHECK(occupiedCount(kbufferCellAt(kOut.data(), bx, by + 1)) == 0);
}

// --- accept 2: same-tag ties at identical z, order-independent ---------

static void test_same_tag_tied_z_order_independent() {
    const int bx = 8, by = 8;
    const std::uint8_t tag = 7;
    const std::uint16_t tiedZ = 4242;  // every fragment shares this exact
                                        // quantised z -- a genuine tie, per
                                        // ADR-059's own I6/tied-z risk.

    std::vector<Frag> forward{
        makeExactFrag(bx, by, Sample(1000), Sample(200), Sample(300), Weight(9000), tiedZ, tag),
        makeExactFrag(bx, by, Sample(2000), Sample(400), Sample(600), Weight(12000), tiedZ, tag),
        makeExactFrag(bx, by, Sample(3000), Sample(600), Sample(900), Weight(6000), tiedZ, tag),
        makeExactFrag(bx, by, Sample(4000), Sample(800), Sample(1200), Weight(15000), tiedZ, tag),
    };
    std::vector<Frag> reversed(forward.rbegin(), forward.rend());
    std::vector<Frag> shuffled{forward[2], forward[0], forward[3], forward[1]};

    auto runOnce = [&](const std::vector<Frag>& frags) {
        TileKBufferAccum accum;
        splatTileKBuffer(frags, accum);
        std::vector<std::array<KSlot, kBufferK>> out((std::size_t(kTilePixels)));
        sumBanksKBuffer(accum, out.data());
        return kbufferCellAt(out.data(), bx, by);
    };

    const std::array<KSlot, kBufferK> cellForward = runOnce(forward);
    const std::array<KSlot, kBufferK> cellReversed = runOnce(reversed);
    const std::array<KSlot, kBufferK> cellShuffled = runOnce(shuffled);

    CHECK(occupiedCount(cellForward) == 1);
    CHECK(occupiedCount(cellReversed) == 1);
    CHECK(occupiedCount(cellShuffled) == 1);

    const KSlot* sForward = findSlotForTag(cellForward, tag);
    const KSlot* sReversed = findSlotForTag(cellReversed, tag);
    const KSlot* sShuffled = findSlotForTag(cellShuffled, tag);
    CHECK(sForward != nullptr && sReversed != nullptr && sShuffled != nullptr);
    if (sForward == nullptr || sReversed == nullptr || sShuffled == nullptr) return;

    // Integer addition is commutative/associative (I6): order-independent.
    CHECK(sForward->cell.R == sReversed->cell.R);
    CHECK(sForward->cell.R == sShuffled->cell.R);
    CHECK(sForward->cell.G == sReversed->cell.G);
    CHECK(sForward->cell.G == sShuffled->cell.G);
    CHECK(sForward->cell.B == sReversed->cell.B);
    CHECK(sForward->cell.B == sShuffled->cell.B);
    CHECK(sForward->cell.w == sReversed->cell.w);
    CHECK(sForward->cell.w == sShuffled->cell.w);

    // Every fragment shares the exact same z, so first-seen z is identical
    // regardless of which one arrives first.
    CHECK(sForward->firstSeenZ == tiedZ);
    CHECK(sReversed->firstSeenZ == tiedZ);
    CHECK(sShuffled->firstSeenZ == tiedZ);
}

// --- accept 3: >kBufferK distinct tags -- self-consistency, no oracle --

// Six distinct tags land in one cell, each with a distinct, well-separated
// z, one fragment per tag. ADR-059's own accepted caveat: which kBufferK
// of the six survive is not proven order-independent across threading
// paths, so this test does NOT assert a specific hand-derived surviving
// set -- only: the hard cap holds (never more than kBufferK occupied);
// whichever tags did survive accumulated correctly for their own single
// fragment; and running the identical fragment order twice is
// byte-identical both times (the "self-consistency" property itself).
static void test_eviction_self_consistent_not_oracle_checked() {
    const int bx = 12, by = 12;
    std::vector<Frag> frags;
    for (std::uint8_t tag = 0; tag < 6; ++tag) {
        const auto n = Sample(tag) + 1;
        frags.push_back(makeExactFrag(bx, by, Sample(100 * n), Sample(0), Sample(0),
                                       Weight(1000 * n), std::uint16_t(1000 * n), tag));
    }

    auto runOnce = [&] {
        TileKBufferAccum accum;
        splatTileKBuffer(frags, accum);
        std::vector<std::array<KSlot, kBufferK>> out((std::size_t(kTilePixels)));
        sumBanksKBuffer(accum, out.data());
        return kbufferCellAt(out.data(), bx, by);
    };

    const std::array<KSlot, kBufferK> run1 = runOnce();
    const std::array<KSlot, kBufferK> run2 = runOnce();

    CHECK(occupiedCount(run1) == kBufferK);
    CHECK(occupiedCount(run2) == kBufferK);

    for (int i = 0; i < kBufferK; ++i) {
        CHECK(run1[std::size_t(i)].occupied == run2[std::size_t(i)].occupied);
        CHECK(run1[std::size_t(i)].tag == run2[std::size_t(i)].tag);
        CHECK(run1[std::size_t(i)].firstSeenZ == run2[std::size_t(i)].firstSeenZ);
        CHECK(run1[std::size_t(i)].cell.R == run2[std::size_t(i)].cell.R);
        CHECK(run1[std::size_t(i)].cell.w == run2[std::size_t(i)].cell.w);
    }

    // Whichever tags survived: numerically correct for their own single
    // fragment (rawWeight 256 here, so R*w with no truncation).
    for (const KSlot& s : run1) {
        if (!s.occupied) continue;
        const Frag& f = frags[s.tag];
        CHECK(s.cell.R == ColourAccum(f.R) * ColourAccum(f.w));
        CHECK(s.cell.w == WeightAccum(f.w));
        CHECK(s.firstSeenZ == f.z);
    }
}

// --- TileKBufferAccum::clear() ------------------------------------------

static void test_clear_frees_all_slots() {
    const int bx = 3, by = 3;
    std::vector<Frag> frags{
        makeExactFrag(bx, by, Sample(1), Sample(2), Sample(3), Weight(kWeightUnity), 0, 1)};

    TileKBufferAccum accum;
    splatTileKBuffer(frags, accum);
    accum.clear();

    std::vector<std::array<KSlot, kBufferK>> out((std::size_t(kTilePixels)));
    sumBanksKBuffer(accum, out.data());
    for (int i = 0; i < kTilePixels; ++i) {
        CHECK_ONCE(occupiedCount(out[std::size_t(i)]) == 0);
    }
}

int main() {
    test_single_tag_matches_plain_sumBanks();
    test_same_tag_tied_z_order_independent();
    test_eviction_self_consistent_not_oracle_checked();
    test_clear_frees_all_slots();
    return scatter::test::summary("test_kbuffer_storage");
}
