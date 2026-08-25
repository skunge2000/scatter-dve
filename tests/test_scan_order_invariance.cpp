// scatter-dve — WU-32 (documentation-only historical-findings import): the
// one production-adjacent deliverable, a standing regression test, per
// docs/sources/WU-SM-02.md §8 fixture 29 ("scan-order invariance. Render
// one frame with the (u,v) walk in each of the four scan-direction
// combinations. Under M2 the output must be bit-identical. Under M1 it will
// not be. A cheap standing regression that also detects accidental order
// dependence.").
//
// WHAT THIS TEST ACTUALLY COVERS, AND WHY IT IS NARROWER THAN THE FIXTURE'S
// OWN NAME.
//
// The fixture as written asks for four combinations of the (u, v) walk:
// {forward, reverse} on each axis. This unit is documentation-only apart
// from this one test (its own opening brief); adding a column-traversal
// (u-direction) parameter to core/binner.cpp's per-sample loop would be a
// real, if small, production change to an existing, frozen entry point's
// internal behaviour, which is out of scope here — SESSION-PROTOCOL.md rule
// 2 already treats generateFragments()/generateFragmentsRowRange() as
// having a fixed contract, and this unit does not touch either function.
//
// What core/binner.hpp *does* already expose, unchanged, is row-range
// control (generateFragmentsRowRange(), WU-16b/ADR-041): a caller can
// request fragments for an arbitrary [rowStart, rowEnd) band, and — per
// ADR-040/041 — every row's own (u, v) parameterisation stays keyed to the
// whole raster regardless of which band it is requested in or when. That
// means the *row* (v) processing order is already a free variable through
// the existing public API, with no production code change needed: this
// test drives every source row through its own single-row
// generateFragmentsRowRange() call, in four distinct row orders, and checks
// that the fully-summed per-tile accumulator (splatTile() + sumBanks(),
// I4/I6's own order-independent arithmetic) is bit-identical across all
// four.
//
// This exercises the same class of bug the fixture exists to catch —
// accidental dependence on the order fragments are generated/binned in, the
// signature of a traversal-order (M1-style) mechanism creeping into this
// project's own deliberately order-independent (M2-style-safe) design —
// along the row axis. It does NOT exercise column (u) order, because no
// production entry point offers a way to do so without changing one. See
// tests/fixtures-historical.md's own row for fixture 29 and DECISIONS.md
// WU-32 (WORK-UNITS.md) for the same note.
#include "core/binner.hpp"
#include "core/shapes/shapes.hpp"
#include "core/splat.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cstdint>
#include <vector>

using namespace scatter;

namespace {

// A small, deliberately non-trivial warp — the same real shape generator
// (not a synthetic affine stand-in) tests/test_row_band.cpp's own Part 2
// uses for its pipeline-level check — so this test exercises real
// magnification/compression variety across the raster, not just a uniform
// scale where every source row would behave identically regardless of
// order.
Lattice buildWarpLattice(int destW, int destH) {
    shapes::CylinderParams cp;
    cp.radius = double(destW) * 0.6;
    cp.angleSpan = 1.4;
    cp.heightSpan = double(destH) * 0.9;
    cp.centerX = double(destW) / 2.0;
    cp.centerY = double(destH) / 2.0;
    return shapes::buildCylinderLattice(cp);
}

// Per-pixel signature so two different source samples are never
// accidentally colour-identical — the same construction
// tests/test_row_band.cpp's TinyWarpedFrame uses, duplicated locally per
// SESSION-PROTOCOL.md rule 2 (no cross-test-file sharing).
struct SignatureRaster {
    int width, height;
    std::vector<Sample> y, cb, cr;

    SignatureRaster(int w, int h) : width(w), height(h) {
        y.resize(std::size_t(w) * std::size_t(h));
        cb.assign(std::size_t(w) * std::size_t(h), kChromaZero);
        cr.assign(std::size_t(w) * std::size_t(h), kChromaZero);
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                const std::size_t i = std::size_t(py) * std::size_t(w) + std::size_t(px);
                y[i] = Sample((px * 997 + py * 131) % 65536);
            }
        }
    }

    SourceRaster view() const noexcept {
        SourceRaster r;
        r.width = width;
        r.height = height;
        r.r = y.data();
        r.g = cb.data();
        r.b = cr.data();
        return r;
    }
};

// Generates fragments for every row of `src`, one row at a time via
// generateFragmentsRowRange(), visited in `rowOrder` — an arbitrary
// permutation of [0, src.height). Merges into a single TileBins covering
// (destW, destH). No production code changed: this is exactly the existing
// row-range entry point, called once per row instead of once per band.
TileBins generateInRowOrder(const Lattice& lat, const SourceRaster& src,
                             int destW, int destH,
                             const std::vector<int>& rowOrder) {
    TileBins bins(destW, destH);
    for (int row : rowOrder) {
        generateFragmentsRowRange(lat, src, /*maxK=*/1000.0, SupersampleConfig{},
                                   /*tag=*/0, row, row + 1, bins);
    }
    return bins;
}

// Fully sums every tile's fragments (splatTile() + sumBanks(), the same
// order-independent I4/I6 arithmetic every other splat test in this suite
// checks) into one flat per-tile AccumCell grid, so two TileBins built from
// different fragment-arrival orders can be compared cell by cell.
std::vector<std::vector<AccumCell>> sumAllTiles(const TileBins& bins) {
    std::vector<std::vector<AccumCell>> out;
    out.reserve(std::size_t(bins.tilesX()) * std::size_t(bins.tilesY()));
    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            TileAccum accum;
            splatTile(bins.tile(tx, ty), accum);
            const std::size_t cellCount = std::size_t(kTilePixels);
            std::vector<AccumCell> cells(cellCount);
            sumBanks(accum, cells.data());
            out.push_back(std::move(cells));
        }
    }
    return out;
}

void checkAllTilesIdentical(const std::vector<std::vector<AccumCell>>& want,
                             const std::vector<std::vector<AccumCell>>& got,
                             const char* label) {
    CHECK(want.size() == got.size());
    for (std::size_t t = 0; t < want.size() && t < got.size(); ++t) {
        CHECK(want[t].size() == got[t].size());
        for (std::size_t c = 0; c < want[t].size() && c < got[t].size(); ++c) {
            const AccumCell& a = want[t][c];
            const AccumCell& b = got[t][c];
            CHECK_ONCE(a.R == b.R);
            CHECK_ONCE(a.G == b.G);
            CHECK_ONCE(a.B == b.B);
            CHECK_ONCE(a.w == b.w);
        }
    }
    (void)label;
}

// Four distinct row-visitation orders. Not literally "forward/reverse on
// each of two axes" (see file header) — four genuinely different
// permutations of the row (v) axis, chosen to catch different failure
// modes: (a) is the reference/natural order; (b) is a full reversal,
// the most obvious traversal-order dependence to catch; (c) interleaves
// non-adjacent rows, which a boundary-adjacent-only bug would miss; (d)
// reverses fixed-size blocks, which neither (b) nor (c) alone would catch
// (a bug tied to block-local ordering rather than global direction).
std::vector<int> rowsForward(int h) {
    const std::size_t n = std::size_t(h);
    std::vector<int> r(n);
    for (int i = 0; i < h; ++i) r[std::size_t(i)] = i;
    return r;
}

std::vector<int> rowsReverse(int h) {
    std::vector<int> r = rowsForward(h);
    std::reverse(r.begin(), r.end());
    return r;
}

std::vector<int> rowsEvenThenOdd(int h) {
    std::vector<int> r;
    r.reserve(std::size_t(h));
    for (int i = 0; i < h; i += 2) r.push_back(i);
    for (int i = 1; i < h; i += 2) r.push_back(i);
    return r;
}

std::vector<int> rowsBlockReversed(int h, int blockSize) {
    std::vector<int> r;
    r.reserve(std::size_t(h));
    std::vector<std::pair<int, int>> blocks;  // [start, end)
    for (int start = 0; start < h; start += blockSize) {
        blocks.emplace_back(start, std::min(start + blockSize, h));
    }
    for (auto it = blocks.rbegin(); it != blocks.rend(); ++it) {
        for (int row = it->first; row < it->second; ++row) r.push_back(row);
    }
    return r;
}

void test_scan_order_invariance_row_axis() {
    const int destW = 64, destH = 48;
    const int srcW = 37, srcH = 29;  // odd, coprime-ish dims: no accidental
                                      // symmetry between forward and reverse

    SignatureRaster src(srcW, srcH);
    const Lattice lat = buildWarpLattice(destW, destH);
    const SourceRaster view = src.view();

    const TileBins reference = generateInRowOrder(lat, view, destW, destH, rowsForward(srcH));
    const std::vector<std::vector<AccumCell>> referenceSummed = sumAllTiles(reference);

    // Sanity: the reference itself must contain fragments, or every check
    // below would trivially and uselessly pass on all-zero data.
    bool sawWeight = false;
    for (const auto& tile : referenceSummed) {
        for (const AccumCell& cell : tile) {
            if (cell.w != 0) { sawWeight = true; break; }
        }
        if (sawWeight) break;
    }
    CHECK(sawWeight);

    {
        const TileBins reversed = generateInRowOrder(lat, view, destW, destH, rowsReverse(srcH));
        checkAllTilesIdentical(referenceSummed, sumAllTiles(reversed), "reverse");
    }
    {
        const TileBins interleaved = generateInRowOrder(lat, view, destW, destH, rowsEvenThenOdd(srcH));
        checkAllTilesIdentical(referenceSummed, sumAllTiles(interleaved), "even-then-odd");
    }
    {
        const TileBins blockRev = generateInRowOrder(lat, view, destW, destH, rowsBlockReversed(srcH, 7));
        checkAllTilesIdentical(referenceSummed, sumAllTiles(blockRev), "block-reversed(7)");
    }
}

}  // namespace

int main() {
    test_scan_order_invariance_row_axis();
    return scatter::test::summary("test_scan_order_invariance");
}
