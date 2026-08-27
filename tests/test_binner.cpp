// WU-08: fragment generation and tile binning (architecture.md 4.3, 4.4,
// 4.6; ADR-024 for the choices left open).
//
// Checks WORK-UNITS.md's WU-08 accept criteria directly:
//
// 1. Fragment count equals source samples under compression: a uniformly
//    compressive affine lattice (pixel-space det J < 1 everywhere) must
//    produce exactly width*height primary fragments, none dropped, none
//    subdivided.
// 2. Boundary straddling replicates into exactly the right neighbours: an
//    exact pixel-space identity lattice makes every source pixel's
//    destination position an exact integer, so which tiles each pixel
//    should straddle into is knowable in closed form and checked against
//    an independent enumeration in this file, not against binner.cpp's
//    own logic.
// 3. No fragment lost or duplicated within a tile: every source pixel's
//    unique colour signature is decoded back out of every tile's bin and
//    checked against that independent enumeration -- no pixel missing,
//    none appearing twice in the same tile.
//
// Also checks 4.6's supersampling thresholds and cap in isolation (2x2,
// 4x4, the boundary values at exactly the configured thresholds, and the
// hard cap), and that a destination-raster-crossing map drops samples
// rather than emitting an out-of-range fragment.
//
// WU-28c (DECISIONS.md ADR-065) adds one more: generateFragmentsTagByFacing()
// gives a self-folding surface's front and back different Frag::tag values,
// checked against a real buildSphereLattice() self-fold (angleSpanH ==
// 2*pi) at the same two control vertices DECISIONS.md ADR-063 and
// tests/test_jacobian.cpp's own test_surface_normal_sign_matches_facing_
// convention() already hand-derive and check the *sign* of -- this test
// checks the *tag* generateFragmentsTagByFacing() assigns at those same two
// points instead.
//
// WU-23a2a (DECISIONS.md ADR-076) adds two more, for
// generateFragmentsFieldRows():
//
// 1. Equivalence to an independent ground truth: calling
//    generateFragmentsRowRange() once per row of one field's own parity
//    (each call covering exactly that one row, [py, py+1)) and accumulating
//    the results into one TileBins must produce output byte-for-byte
//    identical, tile by tile, fragment by fragment, in the same order, to
//    one generateFragmentsFieldRows() call covering the same field -- the
//    same "row-range/whole-raster equivalence" principle WU-16b/ADR-041
//    already established, extended from a contiguous range to a strided
//    one, and checked against generateFragmentsRowRange() specifically
//    because that entry point's own row-range/whole-raster equivalence is
//    already an accepted, tested fact, not something this test re-derives.
// 2. The actual bug DECISIONS.md ADR-075 found, demonstrated directly: the
//    naive alternative (extractField() a half-height field raster, then run
//    ordinary generateFragments() on it) computes a source pixel's v with
//    the wrong denominator (video/interlace.hpp, WU-23a -- reused here
//    purely as a comparison baseline; this unit does not modify that file).
//    Top field's own row 0 is frame row 0, so the naive approach happens to
//    get it right by coincidence (see the test below for why); Bottom
//    field's own row 0 is frame row 1, and the naive approach collapses it
//    back to the same v row 0 would give, erasing the half-line offset that
//    is the entire point of field mode. This test proves
//    generateFragmentsFieldRows() does not reproduce that bug where the
//    naive approach does.

#include "core/binner.hpp"
#include "core/coarse_shading.hpp"
#include "core/lighting.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"
#include "video/interlace.hpp"
#include "video/raster.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <set>
#include <utility>
#include <vector>

using namespace scatter;

namespace {

// An exact affine map in *pixel* space: dest.x = offX + scaleX * px,
// dest.y = offY + scaleY * py, for a lattice covering a src.width x
// src.height source raster (pixelToLattice()'s convention, binner.cpp).
// Catmull-Rom reproduces an affine function of its control points exactly
// everywhere, not just at the control vertices themselves (partition of
// unity plus tangents consistent with a constant slope), so this is exact
// for any continuous (u, v), the same property test_jacobian.cpp and
// test_ewa.cpp rely on when they build synthetic affine cases directly as
// a Jacobian rather than through a Lattice.
//
// "Exact" means algebraically; binner.cpp's pixelToLattice() maps a pixel
// index through px * kLatticeMax / (dim - 1) and back through the inverse
// scale factor, and IEEE 754 double rounding on that round trip is only
// guaranteed to cancel to the bit when (dim - 1) divides kLatticeMax (128)
// exactly -- e.g. dim - 1 in {..., 8, 16, 32, 64, 128}, which makes every
// intermediate a power-of-two fraction. Tests that need bit-exact integer
// destination positions (to know which tile a pixel lands in without
// tolerance) pick such a dim; see each test below for its choice.
Lattice makePixelAffineLattice(double scaleX, double scaleY, double offX,
                                double offY, int srcWidth, int srcHeight) {
    Lattice lat;
    const double su = (srcWidth  > 1) ? scaleX * double(srcWidth  - 1) / double(kLatticeMax) : 0.0;
    const double sv = (srcHeight > 1) ? scaleY * double(srcHeight - 1) / double(kLatticeMax) : 0.0;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            Vec3& p = lat.at(row, col);
            p.x = offX + su * double(col);
            p.y = offY + sv * double(row);
            p.z = 0.0;
        }
    }
    return lat;
}

// A source raster where every sample's colour uniquely encodes its own
// (px, py): Y = px, Cb = py, Cr = 0. Decoding a Frag's colour back to
// (px, py) is then exact under an identity or near-identity affine map
// (no chroma resampling in this path to blur it -- WU-08 samples Y/Cb/Cr
// identically), which is what makes "no fragment lost or duplicated"
// checkable by set membership rather than only by counting.
struct SignatureRaster {
    int width, height;
    std::vector<Sample> y, cb, cr;

    explicit SignatureRaster(int w, int h) : width(w), height(h) {
        y.resize(std::size_t(w) * std::size_t(h));
        cb.resize(std::size_t(w) * std::size_t(h));
        cr.resize(std::size_t(w) * std::size_t(h));
        for (int py = 0; py < h; ++py) {
            for (int px = 0; px < w; ++px) {
                const std::size_t i = std::size_t(py) * std::size_t(w) + std::size_t(px);
                y[i]  = Sample(px);
                cb[i] = Sample(py);
                cr[i] = 0;
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

std::pair<int, int> decode(const Frag& f) noexcept {
    return {int(f.R), int(f.G)};
}

// Frag is trivially copyable, standard layout (core/types.hpp's own
// static_asserts) -- a plain memcmp is exactly "identical destination
// position, identical colour, identical everything", the same bit-exact
// comparison DECISIONS.md ADR-036's own checksum instrumentation used for a
// stronger claim than any field-by-field comparison could make cheaply.
bool sameFrag(const Frag& a, const Frag& b) noexcept {
    return std::memcmp(&a, &b, sizeof(Frag)) == 0;
}

// Destination position alone (tile-local SubPos x/y) -- deliberately not
// colour: WU-23a2a's own naive-extraction test below compares fragments
// whose source content genuinely differs (a field raster's own row 0 can
// hold a different frame row's colour than another field's row 0 does) but
// whose *destination* is the claim under test.
bool samePosition(const Frag& a, const Frag& b) noexcept {
    return a.x == b.x && a.y == b.y;
}

// Every fragment whose colour signature decodes to (px, srcRow), across
// every tile in `bins`. WU-23a2a's own tests below use this to locate one
// specific source sample's own fragment(s) in a TileBins built one of
// several different ways, for direct comparison.
std::vector<Frag> findFrags(const TileBins& bins, int px, int srcRow) {
    std::vector<Frag> found;
    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            for (const Frag& f : bins.tile(tx, ty)) {
                if (decode(f) == std::pair{px, srcRow}) found.push_back(f);
            }
        }
    }
    return found;
}

}  // namespace

// --- 1. fragment count equals source samples under compression ---------

static void test_fragment_count_matches_source_samples_under_compression() {
    const int W = 20, H = 15;
    SignatureRaster src(W, H);
    // Compress by 2x each axis in pixel space: pixel-space det J = 0.25,
    // well under the 2x2 threshold (1.0) everywhere, uniformly (affine).
    Lattice lat = makePixelAffineLattice(0.5, 0.5, 5.0, 5.0, W, H);

    TileBins bins(64, 64);
    SupersampleConfig ss;
    const BinStats stats =
        generateFragments(lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, bins);

    CHECK(stats.sourceSamples == std::size_t(W) * std::size_t(H));
    CHECK(stats.primaryFragments == std::size_t(W) * std::size_t(H));
    CHECK(stats.droppedOffRaster == 0);

    // Every tile's bin, taken together, must contain exactly one Frag per
    // source pixel (no loss, no duplication) -- checked independently of
    // how binning distributed them across tiles.
    std::multiset<std::pair<int, int>> seen;
    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            for (const Frag& f : bins.tile(tx, ty)) seen.insert(decode(f));
        }
    }
    CHECK(seen.size() == std::size_t(W) * std::size_t(H));
    bool allUnique = true;
    for (const auto& kv : seen) {
        if (seen.count(kv) != 1) { allUnique = false; break; }
    }
    CHECK(allUnique);
    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            CHECK_ONCE(seen.count({px, py}) == 1);
        }
    }
}

// --- 2/3. boundary straddling and no loss/duplication within a tile ----

static void test_boundary_straddling_replicates_into_right_neighbours() {
    // Two full tiles in each direction plus one extra pixel: W - 1 ==
    // 2 * kTileSize, which divides 128 exactly for both candidate tile
    // sizes (16 -> 32, 128/32 = 4; 32 -> 64, 128/64 = 2), giving a
    // bit-exact pixel-space identity map (see makePixelAffineLattice's
    // comment) so that which tile(s) each source pixel lands in is
    // knowable in closed form, not just up to floating-point tolerance.
    // The extra pixel makes a third, mostly-irrelevant tile column/row
    // exist; only the first two full tiles in each direction are checked
    // in detail below.
    const int W = 2 * kTileSize + 1, H = 2 * kTileSize + 1;
    SignatureRaster src(W, H);
    Lattice lat = makePixelAffineLattice(1.0, 1.0, 0.0, 0.0, W, H);

    TileBins bins(W, H);
    SupersampleConfig ss;
    const BinStats stats =
        generateFragments(lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, bins);

    CHECK(stats.sourceSamples == std::size_t(W) * std::size_t(H));
    CHECK(stats.primaryFragments == std::size_t(W) * std::size_t(H));
    CHECK(stats.droppedOffRaster == 0);
    CHECK(bins.tilesX() == tileCount(W) && bins.tilesY() == tileCount(H));

    // Independent reference: for an exact-integer identity landing, pixel
    // (px, py) is primary in tile (px/kTileSize, py/kTileSize) and
    // additionally replicated right/down/diagonally whenever its local
    // column/row is the tile's last (kTileSize - 1) and a further tile
    // exists in that direction -- architecture.md 4.4/4.5's base+1 /
    // base+stride / base+stride+1 corners.
    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            const int tx = px / kTileSize, ty = py / kTileSize;
            const int lx = px - tx * kTileSize, ly = py - ty * kTileSize;
            const bool wantRight = (lx == kTileSize - 1) && (tx + 1 < bins.tilesX());
            const bool wantDown  = (ly == kTileSize - 1) && (ty + 1 < bins.tilesY());

            for (int dty = 0; dty < bins.tilesY(); ++dty) {
                for (int dtx = 0; dtx < bins.tilesX(); ++dtx) {
                    const bool shouldBePresent =
                        (dtx == tx && dty == ty) ||
                        (wantRight && dtx == tx + 1 && dty == ty) ||
                        (wantDown  && dtx == tx && dty == ty + 1) ||
                        (wantRight && wantDown && dtx == tx + 1 && dty == ty + 1);

                    int count = 0;
                    for (const Frag& f : bins.tile(dtx, dty)) {
                        if (decode(f) == std::pair{px, py}) ++count;
                    }
                    CHECK_ONCE(count == (shouldBePresent ? 1 : 0));
                }
            }
        }
    }

    // Corner pixels (last row and column of the whole raster, hence of
    // the top-left tile) must replicate into all four tiles; interior
    // pixels away from any boundary must appear in exactly one.
    const int cornerCount = [&] {
        int n = 0;
        for (int ty = 0; ty < bins.tilesY(); ++ty)
            for (int tx = 0; tx < bins.tilesX(); ++tx)
                for (const Frag& f : bins.tile(tx, ty))
                    if (decode(f) == std::pair{kTileSize - 1, kTileSize - 1}) ++n;
        return n;
    }();
    CHECK(cornerCount == 4);

    const int interiorCount = [&] {
        int n = 0;
        for (int ty = 0; ty < bins.tilesY(); ++ty)
            for (int tx = 0; tx < bins.tilesX(); ++tx)
                for (const Frag& f : bins.tile(tx, ty))
                    if (decode(f) == std::pair{3, 3}) ++n;
        return n;
    }();
    CHECK(interiorCount == 1);

    // Total replica count matches the sum of the same independent
    // reference used above.
    std::size_t wantReplicas = 0;
    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            const int tx = px / kTileSize, ty = py / kTileSize;
            const int lx = px - tx * kTileSize, ly = py - ty * kTileSize;
            const bool wantRight = (lx == kTileSize - 1) && (tx + 1 < bins.tilesX());
            const bool wantDown  = (ly == kTileSize - 1) && (ty + 1 < bins.tilesY());
            if (wantRight) ++wantReplicas;
            if (wantDown) ++wantReplicas;
            if (wantRight && wantDown) ++wantReplicas;
        }
    }
    CHECK(stats.replicaFragments == wantReplicas);
}

// --- 4.6 supersampling thresholds and cap -------------------------------

static void test_supersampling_thresholds_and_cap() {
    // A 3-pixel-wide/tall raster: W - 1 == 2 divides 128 exactly
    // (128/2 == 64), the same bit-exactness reason as the other tests,
    // and its centre pixel (1, 1) maps to u == v == 64 -- the lattice's
    // midpoint, nowhere near its domain boundary. This matters here
    // specifically: ADR-022's edge handling (control-vertex lookups
    // outside [0, kLatticeMax] replicate the nearest edge vertex) gives
    // the *tangent* at u == 0 or u == kLatticeMax a clamped, one-sided
    // neighbour instead of a true central difference, so jacobian() at
    // the lattice's own boundary does not reproduce an affine field's
    // true constant slope -- correct, frozen WU-06 behaviour (its own
    // test_jacobian.cpp checks jacobian() against numeric differentiation
    // of eval() itself there, not against the affine field's slope), but
    // it means the two border pixels here (px/py == 0 or 2) are not a
    // clean test of chooseSupersample() in isolation the way the centre
    // pixel is. Offset keeps the centre pixel's destination comfortably
    // inside a tile's interior (away from kTileSize == 16 or 32's edge)
    // regardless of scale, so its fragments are never replicated and a
    // straight count of Frags decoding to (1, 1) is exactly its N * N.
    auto sampleCountFor = [](double scale, int maxSupersample) -> std::size_t {
        const int W = 3, H = 3;
        SignatureRaster src(W, H);
        Lattice lat = makePixelAffineLattice(scale, scale, 8.0, 8.0, W, H);
        TileBins bins(64, 64);
        SupersampleConfig ss;
        ss.maxSupersample = maxSupersample;
        generateFragments(lat, src.view(), /*maxK=*/1.0e6, ss, /*tag=*/0, bins);

        std::size_t n = 0;
        for (int ty = 0; ty < bins.tilesY(); ++ty)
            for (int tx = 0; tx < bins.tilesX(); ++tx)
                for (const Frag& f : bins.tile(tx, ty))
                    if (decode(f) == std::pair{1, 1}) ++n;
        return n;
    };

    // det J == scale^2 at the centre pixel. Strict "> 1" per
    // architecture.md 4.6's own words: scale == 1 (det == 1) must NOT
    // subdivide.
    CHECK(sampleCountFor(1.0, 4) == 1);             // det = 1: N = 1
    CHECK(sampleCountFor(std::sqrt(1.5), 4) == 4);  // det = 1.5: N = 2 -> 4 sub-samples
    CHECK(sampleCountFor(2.0, 4) == 4);             // det = 4 exactly: still N = 2 (ADR-024: strict >)
    CHECK(sampleCountFor(2.001, 4) == 16);          // det just over 4: N = 4 -> 16 sub-samples
    CHECK(sampleCountFor(5.0, 4) == 16);            // det = 25, deep into 4x4 territory

    // Hard cap: even far past the 4x4 threshold, maxSupersample limits N.
    CHECK(sampleCountFor(5.0, 2) == 4);
    CHECK(sampleCountFor(5.0, 1) == 1);
}

// --- off-raster samples are dropped, not clamped into a bogus fragment -

static void test_off_raster_samples_are_dropped() {
    // W - 1 == 8 divides 128 exactly (128/8 == 16), for the same
    // bit-exactness reason as the other tests: the pixel that should land
    // precisely on the destination's x == 0 edge must not drift to either
    // side of it from rounding.
    const int W = 9, H = 9;
    SignatureRaster src(W, H);
    // Identity map offset so the lower half of the source raster lands to
    // the left of the destination raster's x = 0 edge.
    Lattice lat = makePixelAffineLattice(1.0, 1.0, -4.0, 0.0, W, H);

    TileBins bins(W, H);
    SupersampleConfig ss;
    const BinStats stats =
        generateFragments(lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, bins);

    CHECK(stats.sourceSamples == std::size_t(W) * std::size_t(H));
    CHECK(stats.droppedOffRaster == 4 * std::size_t(H));  // px in [0, 4) land at x < 0
    CHECK(stats.primaryFragments == stats.sourceSamples - stats.droppedOffRaster);

    std::size_t totalBinned = 0;
    for (int ty = 0; ty < bins.tilesY(); ++ty)
        for (int tx = 0; tx < bins.tilesX(); ++tx)
            totalBinned += bins.tile(tx, ty).size();
    CHECK(totalBinned == stats.primaryFragments + stats.replicaFragments);
}

// --- WU-28c: self-fold facing tag ---------------------------------------

static void test_self_fold_front_and_back_get_different_tags() {
    constexpr double kPi = 3.14159265358979323846;
    using scatter::shapes::SphereParams;
    using scatter::shapes::buildSphereLattice;

    // Same self-folding sphere DECISIONS.md ADR-063 and test_jacobian.cpp's
    // own test_surface_normal_sign_matches_facing_convention() already use:
    // angleSpanH widened to a full 2*pi wrap, everything else default
    // except centerX/centerY, offset here (unlike test_jacobian.cpp, which
    // only ever reads Jacobian, never destination position) so the sphere's
    // whole projected extent -- centered on (centerX, centerY), radius 200
    // in every direction the orthographic projection touches (ADR-027) --
    // lands inside a destination raster's non-negative bounds instead of
    // straddling x == 0 / y == 0.
    SphereParams p;
    p.angleSpanH = 2.0 * kPi;
    p.centerX = 300.0;
    p.centerY = 300.0;
    const Lattice lat = buildSphereLattice(p);

    // W - 1 == 2 and H - 1 == 2 both divide kLatticeMax (128) exactly, the
    // same bit-exactness reason every other test in this file picks its own
    // raster size: pixel (px, py) == (1, 1) lands exactly on (u, v) ==
    // (64, 64) -- phi == psi == 0, the front-most control vertex -- and
    // (0, 1) lands exactly on (u, v) == (0, 64) -- phi == -pi, psi == 0,
    // the antipodal fold-boundary vertex. Both are exactly the two points
    // DECISIONS.md ADR-063 derives by hand (front: Tv x Tu == (0, 0,
    // -radius^2); back: Tv x Tu == (0, 0, +radius^2)).
    const int W = 3, H = 3;
    SignatureRaster src(W, H);

    TileBins bins(700, 700);
    SupersampleConfig ss;
    constexpr std::uint8_t kFrontTag = 7, kBackTag = 13;
    const BinStats stats = generateFragmentsTagByFacing(
        lat, src.view(), /*maxK=*/1.0e6, ss, kFrontTag, kBackTag, bins);

    CHECK(stats.droppedOffRaster == 0);

    // Gather every fragment decoding back to the front-most source pixel
    // (1, 1) and to the antipodal fold-boundary source pixel (0, 1) --
    // there may be more than one of either if chooseSupersample() (4.6)
    // subdivided that source pixel, since this sphere's own Jacobian is far
    // from affine at this scale; every one of them must carry the same tag,
    // since facing is computed once per source pixel and reused across its
    // own sub-samples (core/binner.cpp's own comment on rawCentreJ/rawJ).
    int frontCount = 0, backCount = 0;
    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            for (const Frag& f : bins.tile(tx, ty)) {
                const auto sig = decode(f);
                if (sig == std::pair{1, 1}) {
                    CHECK_ONCE(f.tag == kFrontTag);
                    ++frontCount;
                } else if (sig == std::pair{0, 1}) {
                    CHECK_ONCE(f.tag == kBackTag);
                    ++backCount;
                }
            }
        }
    }
    CHECK(frontCount > 0);
    CHECK(backCount > 0);

    // generateFragmentsRowRangeTagByFacing() covering the same two rows in
    // one call must agree exactly with the whole-raster wrapper above (the
    // same row-range/whole-raster equivalence WU-16b/ADR-041 already
    // established for the plain-tag functions).
    TileBins rowRangeBins(700, 700);
    const BinStats rowRangeStats = generateFragmentsRowRangeTagByFacing(
        lat, src.view(), /*maxK=*/1.0e6, ss, kFrontTag, kBackTag, 0, H, rowRangeBins);
    CHECK(rowRangeStats.sourceSamples == stats.sourceSamples);
    CHECK(rowRangeStats.primaryFragments == stats.primaryFragments);
    CHECK(rowRangeStats.replicaFragments == stats.replicaFragments);
    CHECK(rowRangeStats.droppedOffRaster == stats.droppedOffRaster);
}

// --- WU-23a2a: field-row visitation matches row-range ground truth ------

static void test_field_rows_match_row_range_ground_truth() {
    // Same compressive affine map and geometry as the very first test in
    // this file (2x compression each axis, well under the 2x2 threshold) --
    // this test's own point is checking rowStep's own plumbing, not
    // re-deriving generateFragmentsRowRange()'s own correctness (already
    // covered by WU-08/WU-16b's own tests above).
    const int W = 20, H = 15;
    SignatureRaster src(W, H);
    Lattice lat = makePixelAffineLattice(0.5, 0.5, 5.0, 5.0, W, H);
    SupersampleConfig ss;

    for (int rowOffset = 0; rowOffset <= 1; ++rowOffset) {
        TileBins fieldBins(64, 64);
        const BinStats fieldStats = generateFragmentsFieldRows(
            lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, rowOffset, fieldBins);

        // Ground truth: one generateFragmentsRowRange() call per row of
        // this field's own parity, each covering exactly [py, py + 1),
        // accumulated into one TileBins in the same ascending row order
        // generateFragmentsFieldRows() itself visits them in -- so if both
        // code paths are correct, not merely consistent with each other,
        // the two TileBins come out byte-for-byte identical, not just
        // equal as multisets.
        TileBins refBins(64, 64);
        BinStats refStats;
        for (int py = rowOffset; py < H; py += 2) {
            const BinStats rowStats = generateFragmentsRowRange(
                lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, py, py + 1, refBins);
            refStats.sourceSamples += rowStats.sourceSamples;
            refStats.primaryFragments += rowStats.primaryFragments;
            refStats.replicaFragments += rowStats.replicaFragments;
            refStats.droppedOffRaster += rowStats.droppedOffRaster;
        }

        CHECK(fieldStats.sourceSamples == refStats.sourceSamples);
        CHECK(fieldStats.primaryFragments == refStats.primaryFragments);
        CHECK(fieldStats.replicaFragments == refStats.replicaFragments);
        CHECK(fieldStats.droppedOffRaster == refStats.droppedOffRaster);

        CHECK(fieldBins.tilesX() == refBins.tilesX() && fieldBins.tilesY() == refBins.tilesY());
        for (int ty = 0; ty < fieldBins.tilesY(); ++ty) {
            for (int tx = 0; tx < fieldBins.tilesX(); ++tx) {
                const std::vector<Frag>& got = fieldBins.tile(tx, ty);
                const std::vector<Frag>& want = refBins.tile(tx, ty);
                CHECK_ONCE(got.size() == want.size());
                if (got.size() == want.size()) {
                    for (std::size_t i = 0; i < got.size(); ++i) {
                        CHECK_ONCE(sameFrag(got[i], want[i]));
                    }
                }
            }
        }
    }
}

// --- WU-23a2a: the naive per-field extraction bug, demonstrated ---------

static void test_field_rows_reject_naive_half_height_extraction_bug() {
    // W - 1 == 8 and H - 1 == 16 both divide kLatticeMax (128) exactly, the
    // same bit-exactness reason every other test in this file picks its own
    // geometry -- this test compares two computed destinations for exact
    // equality or exact inequality, never against a hand-derived numeric
    // value, but bit-exactness still matters so floating-point noise near a
    // tile or pixel boundary cannot masquerade as either outcome.
    const int W = 9, H = 17;
    Lattice lat = makePixelAffineLattice(1.0, 1.0, 0.0, 0.0, W, H);
    SupersampleConfig ss;
    // Forced to 1: pixelJacobian() (core/binner.cpp) scales by src.height
    // too, not only pixelToLattice() -- so the naive extraction's own wrong
    // src.height also perturbs chooseSupersample()'s det J and would
    // otherwise trigger a *different* N than the honest fix gets for the
    // very same source pixel, a real but separate consequence of ADR-024's
    // own design (K/supersampling depend on src.height by construction,
    // documented there) that would otherwise blur this test's own point --
    // the erased v-parameter phase, not the (already-understood) K
    // sensitivity to src.height.
    ss.maxSupersample = 1;
    constexpr std::uint8_t tag = 0;
    const int px = 4;  // an interior column, away from any tile edge

    // A full interlaced frame, video::Raster444, with the same (Y = px,
    // Cb = py, Cr = 0) signature SignatureRaster uses above -- built
    // directly rather than through SignatureRaster, since extractField()
    // (video/interlace.hpp, WU-23a, unmodified by this unit) operates on
    // Raster444, not on SourceRaster's raw pointers.
    video::Raster444 frame(W, H);
    for (int py = 0; py < H; ++py) {
        for (int x = 0; x < W; ++x) {
            const std::size_t i = std::size_t(py) * std::size_t(W) + std::size_t(x);
            frame.Y[i] = Sample(x);
            frame.Cb[i] = Sample(py);
            frame.Cr[i] = 0;
        }
    }

    auto toSourceRaster = [](const video::Raster444& r) noexcept {
        SourceRaster s;
        s.width = r.width;
        s.height = r.height;
        s.r = r.Y.data();
        s.g = r.Cb.data();
        s.b = r.Cr.data();
        return s;
    };

    // Honest fix: generateFragmentsFieldRows() on the full frame.
    TileBins topHonestBins(64, 64), bottomHonestBins(64, 64);
    generateFragmentsFieldRows(lat, toSourceRaster(frame), /*maxK=*/1000.0, ss, tag,
                                /*rowOffset=*/0, topHonestBins);
    generateFragmentsFieldRows(lat, toSourceRaster(frame), /*maxK=*/1000.0, ss, tag,
                                /*rowOffset=*/1, bottomHonestBins);

    // Naive: extractField() a half-height field raster, then run ordinary
    // generateFragments() against it -- the exact "first-cut plan"
    // DECISIONS.md ADR-075 found wrong.
    video::Raster444 topField(W, video::fieldRowCount(H, video::FieldParity::Top));
    video::Raster444 bottomField(W, video::fieldRowCount(H, video::FieldParity::Bottom));
    video::extractField(frame, video::FieldParity::Top, topField);
    video::extractField(frame, video::FieldParity::Bottom, bottomField);

    TileBins topNaiveBins(64, 64), bottomNaiveBins(64, 64);
    generateFragments(lat, toSourceRaster(topField), /*maxK=*/1000.0, ss, tag, topNaiveBins);
    generateFragments(lat, toSourceRaster(bottomField), /*maxK=*/1000.0, ss, tag, bottomNaiveBins);

    // Top field's own row 0 is frame row 0: pixelToLattice(0, anything) is
    // 0 regardless of which (right or wrong) denominator is used, so the
    // naive extraction happens to land this one field's own row 0 at the
    // same destination by coincidence -- worth checking directly, not
    // assumed, since it is exactly the kind of accidental agreement that
    // could otherwise hide the real bug below. Position only, not the whole
    // Frag: pixelJacobian()'s own K/weight computation still depends on
    // src.height independently of pixelToLattice() (see ss.maxSupersample
    // above), so frag.w legitimately differs between the two even at this
    // matching position -- that is ADR-024's own documented src.height
    // sensitivity, not this test's own concern.
    const std::vector<Frag> topHonest = findFrags(topHonestBins, px, /*srcRow=*/0);
    const std::vector<Frag> topNaive = findFrags(topNaiveBins, px, /*srcRow=*/0);
    CHECK(topHonest.size() == 1);
    CHECK(topNaive.size() == 1);
    if (topHonest.size() == 1 && topNaive.size() == 1) {
        CHECK(samePosition(topHonest[0], topNaive[0]));
    }

    // Bottom field's own row 0 is frame row 1, not row 0. The honest fix
    // (full frame height kept as the v-parameter's own denominator) places
    // it one pixel below frame row 0's own destination -- offX == offY ==
    // 0, scaleX == scaleY == 1.0 above make this an exact pixel-space
    // identity map (see makePixelAffineLattice's own comment), so this is
    // the correct half-line phase between the two fields, not an
    // approximation.
    const std::vector<Frag> bottomHonest = findFrags(bottomHonestBins, px, /*srcRow=*/1);
    CHECK(bottomHonest.size() == 1);
    if (bottomHonest.size() == 1 && topHonest.size() == 1) {
        CHECK(!samePosition(bottomHonest[0], topHonest[0]));
    }

    // The naive extraction's own half-height denominator instead maps
    // Bottom's own row 0 back to v == 0, exactly the same v Top's own row 0
    // gets -- both fields' fragments for "their own row 0" land at the
    // identical destination position under the naive approach, erasing the
    // half-line phase ADR-075 names directly.
    const std::vector<Frag> bottomNaive = findFrags(bottomNaiveBins, px, /*srcRow=*/1);
    CHECK(bottomNaive.size() == 1);
    if (bottomNaive.size() == 1 && topNaive.size() == 1) {
        CHECK(samePosition(bottomNaive[0], topNaive[0]));
    }

    // Put the two together: generateFragmentsFieldRows() does NOT reproduce
    // the naive extraction's own bug for the Bottom field, where a
    // hypothetical implementation built the wrong way (extract-then-
    // generateFragments) would.
    if (bottomHonest.size() == 1 && bottomNaive.size() == 1) {
        CHECK(!samePosition(bottomHonest[0], bottomNaive[0]));
    }
}

// --- WU-34b (DECISIONS.md ADR-084): coarse-grid shading multiplied into --
// --- the sample ahead of Frag construction -------------------------------

namespace {

// RGB triple used by this fixture's own independent shading mirror below.
struct RGB {
    double r, g, b;
};

// Independent reimplementation of applyShading()'s own real formula
// (core/binner.cpp) -- not calling into that file's own private helper, the
// same discipline tests/test_coarse_shading.cpp already uses for its own
// production-formula mirrors. This replaces a stale pre-ADR-085 BT.601
// YCbCr round-trip mirror (`mirrorToRgbBt601`/`mirrorFromRgbBt601`) that
// this test carried from before WU-41's RGB-native migration: WU-41 made
// `SourceRaster`'s own r/g/b fields genuine RGB, not Y/Cb/Cr, and
// applyShading() itself became a bare per-channel scale with no conversion
// and no coefficient choice left in it at all (see core/binner.cpp's own
// comment above applyShading() for the full reasoning) -- so the mirror
// here is the same bare per-channel scale, independently written rather
// than calling applyShading() itself, matching WU-34b/ADR-084's own
// "mirror the math independently" precedent and WU-44a's application of it
// this phase (CORRECTIONS.md C-033, WORK-UNITS.md's own WU-43/WU-44/WU-44b
// entries all flagged this fixture's own staleness as WU-44b's job).
RGB mirrorApplyShadingRgb(const RGB& c, double intensity) noexcept {
    return RGB{c.r * intensity, c.g * intensity, c.b * intensity};
}

// Same fixture scene tests/test_coarse_shading.cpp's own
// oneParallelLightScene() uses (redefined here -- that one lives in a
// different translation unit's own anonymous namespace): a Parallel light
// has no distance falloff and no dependence on the surface point P, so
// combined with a flat (z == 0 everywhere) lattice -- a constant facet
// normal across the whole coarse grid -- CoarseShadingGrid::sample()
// returns the identical value I everywhere, making this test's own ground
// truth unambiguous regardless of which coarse-grid vertex a given source
// pixel lands nearest.
LightingScene mirrorParallelLightScene() {
    LightingScene s;
    s.Ia = 0.0;
    s.Ka = 1.0;
    Light light;
    light.type = LightType::Parallel;
    light.direction = Vec3{0.3, 0.2, 0.9};
    light.intensity = 2.0;
    light.Kd = 1.0;
    light.Ks = 0.0;
    s.lights.push_back(light);
    return s;
}

Sample clampRoundToSample(double v) noexcept {
    const double lo = 0.0, hi = double(std::numeric_limits<Sample>::max());
    return Sample(std::round(std::clamp(v, lo, hi)));
}

}  // namespace

static void test_shading_multiplies_rgb_intensity_ahead_of_frag_construction() {
    const int W = 3, H = 3;
    Lattice lat = makePixelAffineLattice(1.0, 1.0, 20.0, 20.0, W, H);

    const LightingScene scene = mirrorParallelLightScene();
    CoarseShadingConfig cfg;
    cfg.filter = ShadingFilter::Full;
    cfg.gridShift = 0;
    const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);
    const double expectedI = grid.sample(64.0, 64.0);  // any (u, v): constant grid

    // A uniform, non-degenerate source colour -- avoids bilinear
    // interpolation entirely and isolates the shading math itself. WU-41:
    // SourceRaster's own r/g/b fields are genuine RGB, not Y/Cb/Cr, so
    // these three planes are literal R/G/B values with no chroma-zero
    // offset to account for.
    std::vector<Sample> rPlane(std::size_t(W) * std::size_t(H), Sample(20000));
    std::vector<Sample> gPlane(std::size_t(W) * std::size_t(H), Sample(40000));
    std::vector<Sample> bPlane(std::size_t(W) * std::size_t(H), Sample(25000));
    SourceRaster src;
    src.width = W;
    src.height = H;
    src.r = rPlane.data();
    src.g = gPlane.data();
    src.b = bPlane.data();

    SupersampleConfig ss;

    TileBins unshadedBins(64, 64);
    generateFragments(lat, src, /*maxK=*/1000.0, ss, /*tag=*/0, unshadedBins);

    TileBins shadedBins(64, 64);
    generateFragments(lat, src, /*maxK=*/1000.0, ss, /*tag=*/0, shadedBins,
                       &grid);

    // Independent expected value: scale the known source RGB colour by
    // expectedI directly -- a separate reimplementation of applyShading()'s
    // own math (a bare per-channel multiply, WU-41/ADR-085), not a call
    // into it.
    const RGB rgb{20000.0, 40000.0, 25000.0};
    const RGB scaled = mirrorApplyShadingRgb(rgb, expectedI);
    const Sample expectedRSample = clampRoundToSample(scaled.r);
    const Sample expectedGSample = clampRoundToSample(scaled.g);
    const Sample expectedBSample = clampRoundToSample(scaled.b);

    // Both TileBins were built from the identical lattice/raster -- only
    // colour should differ between them, fragment for fragment, in the same
    // order.
    bool foundAny = false;
    CHECK(unshadedBins.tilesX() == shadedBins.tilesX() &&
          unshadedBins.tilesY() == shadedBins.tilesY());
    for (int ty = 0; ty < unshadedBins.tilesY(); ++ty) {
        for (int tx = 0; tx < unshadedBins.tilesX(); ++tx) {
            const std::vector<Frag>& unshaded = unshadedBins.tile(tx, ty);
            const std::vector<Frag>& shaded = shadedBins.tile(tx, ty);
            CHECK_ONCE(unshaded.size() == shaded.size());
            const std::size_t n = std::min(unshaded.size(), shaded.size());
            for (std::size_t i = 0; i < n; ++i) {
                foundAny = true;
                // Unshaded path: exactly the uniform source colour,
                // untouched -- proves the null-grid call this test also
                // makes is unaffected by this unit.
                CHECK_ONCE(unshaded[i].R == Sample(20000));
                CHECK_ONCE(unshaded[i].G == Sample(40000));
                CHECK_ONCE(unshaded[i].B == Sample(25000));

                // Shaded path: matches the independent RGB shading mirror
                // above, within +/-1 code of independent rounding.
                CHECK_ONCE(std::fabs(double(shaded[i].R) - double(expectedRSample)) <= 1.0);
                CHECK_ONCE(std::fabs(double(shaded[i].G) - double(expectedGSample)) <= 1.0);
                CHECK_ONCE(std::fabs(double(shaded[i].B) - double(expectedBSample)) <= 1.0);

                // Position, weight, depth and tag are untouched by shading --
                // only colour differs between the two calls.
                CHECK_ONCE(shaded[i].x == unshaded[i].x);
                CHECK_ONCE(shaded[i].y == unshaded[i].y);
                CHECK_ONCE(shaded[i].w == unshaded[i].w);
                CHECK_ONCE(shaded[i].z == unshaded[i].z);
                CHECK_ONCE(shaded[i].tag == unshaded[i].tag);
            }
        }
    }
    CHECK(foundAny);

    // expectedI must not be ~1.0 and the shaded colour must genuinely differ
    // from the unshaded one -- otherwise this test would not actually be
    // exercising the multiply (applyShading(c, 1.0, ...) is an identity up
    // to rounding, so a near-1.0 intensity could pass even with the
    // multiply silently skipped).
    CHECK(std::fabs(expectedI - 1.0) > 0.05);
    CHECK(expectedRSample != Sample(20000) || expectedGSample != Sample(40000) ||
          expectedBSample != Sample(25000));
}

static void test_shading_grid_defaults_to_null_and_preserves_existing_output() {
    // Every pre-existing test in this file already calls generateFragments()
    // (and its siblings) without the trailing shadingGrid parameter
    // (WU-41: the shadingStandard parameter this comment used to also
    // mention is gone -- applyShading() no longer takes one), exercising
    // the default nullptr path throughout -- this test checks the default
    // explicitly, once: an explicit nullptr call and the implicit-default
    // call must produce byte-for-byte identical bins.
    const int W = 20, H = 15;
    SignatureRaster src(W, H);
    Lattice lat = makePixelAffineLattice(0.5, 0.5, 5.0, 5.0, W, H);
    SupersampleConfig ss;

    TileBins implicitBins(64, 64);
    generateFragments(lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, implicitBins);

    TileBins explicitBins(64, 64);
    generateFragments(lat, src.view(), /*maxK=*/1000.0, ss, /*tag=*/0, explicitBins,
                       /*shadingGrid=*/nullptr);

    CHECK(implicitBins.tilesX() == explicitBins.tilesX() &&
          implicitBins.tilesY() == explicitBins.tilesY());
    for (int ty = 0; ty < implicitBins.tilesY(); ++ty) {
        for (int tx = 0; tx < implicitBins.tilesX(); ++tx) {
            const std::vector<Frag>& a = implicitBins.tile(tx, ty);
            const std::vector<Frag>& b = explicitBins.tile(tx, ty);
            CHECK_ONCE(a.size() == b.size());
            const std::size_t n = std::min(a.size(), b.size());
            for (std::size_t i = 0; i < n; ++i) {
                CHECK_ONCE(sameFrag(a[i], b[i]));
            }
        }
    }
}

int main() {
    test_fragment_count_matches_source_samples_under_compression();
    test_boundary_straddling_replicates_into_right_neighbours();
    test_supersampling_thresholds_and_cap();
    test_off_raster_samples_are_dropped();
    test_self_fold_front_and_back_get_different_tags();
    test_field_rows_match_row_range_ground_truth();
    test_field_rows_reject_naive_half_height_extraction_bug();
    test_shading_multiplies_rgb_intensity_ahead_of_frag_construction();
    test_shading_grid_defaults_to_null_and_preserves_existing_output();
    return scatter::test::summary("test_binner");
}
