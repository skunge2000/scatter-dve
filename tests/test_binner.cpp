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

#include "core/binner.hpp"
#include "harness.hpp"

#include <cmath>
#include <cstdint>
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
        r.y = y.data();
        r.cb = cb.data();
        r.cr = cr.data();
        return r;
    }
};

std::pair<int, int> decode(const Frag& f) noexcept {
    return {int(f.Y), int(f.Cb)};
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

int main() {
    test_fragment_count_matches_source_samples_under_compression();
    test_boundary_straddling_replicates_into_right_neighbours();
    test_supersampling_thresholds_and_cap();
    test_off_raster_samples_are_dropped();
    return scatter::test::summary("test_binner");
}
