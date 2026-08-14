// WU-12a: page turn, transparent mode (core/shapes/shapes.hpp,
// DECISIONS.md ADR-028) — reproduces US 4,563,703 FIG. 5's transparent
// default (architecture.md 4.7 phase 1, section 9's own test-plan entry:
// "Transparent flap by default"). Checks WORK-UNITS.md's WU-12a accept
// criteria directly:
//
// 1. Every control vertex buildPageTurnLattice() writes is either exactly
//    flat (z == 0, on the spine-to-free-edge line) or lies exactly (to
//    double-precision tolerance) on the surface of the configured curl
//    cylinder, matching whichever side of the turnProgress-determined
//    flat/curl split it falls on — the same "on the surface" identity
//    ADR-027 established for buildCylinderLattice/buildSphereLattice,
//    checked at every one of the 129x129 control vertices.
// 2. The spine column (s == 0) never moves — x == centerX - width/2 and
//    z == 0 exactly, for any turnProgress — proving the hinge really is
//    fixed, not merely approximately so.
// 3. turnProgress == 0 reduces the whole lattice exactly to the flat
//    (affine) case: every vertex z == 0, x linear in s.
// 4. Lattice::jacobian()'s analytic derivatives agree with central
//    differences (WU-06's own method, reused) across a populated,
//    genuinely curling page-turn lattice — the same "does the interpolant
//    still differentiate correctly on this shape's own control data"
//    check WU-11 ran for cylinder/sphere, now for a piecewise (flat-then-
//    curled) shape for the first time.
// 5. Two independently generated fragment sets — a page-turn flap (one
//    colour) and a full-canvas flat "page behind" (another colour) — fed
//    into the *same* tile bins and splatted together produce, at every
//    destination cell, an AccumCell exactly equal (bit-for-bit, integer
//    addition being associative — I6) to the two layers' own AccumCells
//    added component-wise after being splatted separately. That is
//    architecture.md 4.7 phase 1's own "overlapping surfaces sum" stated
//    as a direct, checkable identity, not inferred from composited colour
//    alone. At the flap's own most solidly covered destination pixel (the
//    full-canvas page-behind layer covers everywhere, so any well-covered
//    flap pixel is guaranteed to overlap it), the combined layer's own
//    accumulated weight is strictly greater than the page-behind layer's
//    alone, and the combined, composited colour differs from the
//    page-behind-alone composited colour by more than ordinary rounding —
//    proving the flap is genuinely visible through pure accumulation, not
//    silently absorbed or ignored.
//
// Not tested here, deliberately: priority-tag opacity (WU-12b, not built
// yet — see HANDOFF.md and DECISIONS.md ADR-028's own scope note); any
// change to core/splat.cpp, core/resolve.cpp or core/pipeline.cpp (none
// needed for transparent mode — WU-06 through WU-11 already established
// the pipeline is shape-agnostic, and this unit's own accept criterion 5
// above is the direct proof that still holds for a *second*, independently
// generated fragment set sharing the same tile bins, not just a single
// shape as WU-11 checked).

#include "core/binner.hpp"
#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "core/splat.hpp"
#include "harness.hpp"
#include "video/raster.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;
using namespace scatter::shapes;

namespace {

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

// ---------------------------------------------------------------------------
// Jacobian-vs-central-difference check, duplicated from
// tests/test_jacobian.cpp / tests/test_shapes.cpp (SESSION-PROTOCOL.md rule
// 2: no test-local code shared across translation units).
// ---------------------------------------------------------------------------

template <typename F>
double numericDeriv(F&& f, double x, double h, double xmin, double xmax) noexcept {
    const bool onInteriorKnot =
        x > xmin && x < xmax && std::fabs(x - std::round(x)) < 1e-9;

    if (!onInteriorKnot && x - h >= xmin && x + h <= xmax) {
        return (f(x + h) - f(x - h)) / (2.0 * h);
    }
    if (x + 2.0 * h <= xmax) {
        return (-3.0 * f(x) + 4.0 * f(x + h) - f(x + 2.0 * h)) / (2.0 * h);
    }
    return (3.0 * f(x) - 4.0 * f(x - h) + f(x - 2.0 * h)) / (2.0 * h);
}

void checkJacobianAt(const Lattice& lat, double u, double v) {
    constexpr double kH   = 1e-4;
    constexpr double kTol = 1e-6;
    constexpr double kMin = 0.0;
    constexpr double kMax = double(kLatticeMax);

    const Jacobian j = lat.jacobian(u, v);

    const double numDxDu = numericDeriv(
        [&](double uu) { return lat.eval(uu, v).x; }, u, kH, kMin, kMax);
    const double numDyDu = numericDeriv(
        [&](double uu) { return lat.eval(uu, v).y; }, u, kH, kMin, kMax);
    const double numDxDv = numericDeriv(
        [&](double vv) { return lat.eval(u, vv).x; }, v, kH, kMin, kMax);
    const double numDyDv = numericDeriv(
        [&](double vv) { return lat.eval(u, vv).y; }, v, kH, kMin, kMax);

    CHECK_ONCE(relClose(j.dxdu, numDxDu, kTol));
    CHECK_ONCE(relClose(j.dydu, numDyDu, kTol));
    CHECK_ONCE(relClose(j.dxdv, numDxDv, kTol));
    CHECK_ONCE(relClose(j.dydv, numDyDv, kTol));
}

// Representative (u, v) points, same coverage tests/test_shapes.cpp uses —
// interior, edges, corners, interior knots — plus a point straddling the
// flat/curl boundary for the params test_pageturn_jacobian_matches_central_
// difference() below uses (width 300, turnProgress 0.5: flatLen 150, which
// falls at lattice column 150/300*128 = 64 — {64.0, 64.0} and its immediate
// neighbours already appear below, so the boundary is exercised without a
// bespoke point).
const double kJacobianCheckPoints[][2] = {
    {10.3, 20.7}, {64.5, 64.5}, {1.2, 126.8}, {126.8, 1.2},
    {45.0, 90.0}, {90.0, 45.0}, {0.5, 0.5}, {127.5, 127.5},
    {0.0, 64.0}, {128.0, 64.0}, {64.0, 0.0}, {64.0, 128.0},
    {0.0, 0.0}, {128.0, 0.0}, {0.0, 128.0}, {128.0, 128.0},
    {63.0, 64.0}, {64.0, 64.0}, {65.0, 64.0}, {63.5, 64.0}, {64.5, 64.0},
};

// ---------------------------------------------------------------------------
// Affine lattice builder for the "page behind" layer — same technique as
// tests/test_zoneplate.cpp's own makeAffineLattice(), duplicated locally
// per SESSION-PROTOCOL.md rule 2.
// ---------------------------------------------------------------------------

Lattice makeAffineLattice(double scaleX, double scaleY, double offX,
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

}  // namespace

// ---------------------------------------------------------------------------
// 1. Every control vertex is exactly flat, or lies exactly on the curl
//    cylinder, per the flat/curl split turnProgress determines.
// ---------------------------------------------------------------------------

static void test_pageturn_flat_or_on_curl_cylinder() {
    PageTurnParams params;
    params.width        = 300.0;
    params.heightSpan   = 250.0;
    params.radius       = 50.0;
    params.turnProgress = 0.4;
    params.centerX      = 20.0;
    params.centerY      = -10.0;

    const Lattice lat = buildPageTurnLattice(params);
    const double spineX = params.centerX - 0.5 * params.width;
    const double flatLen = (1.0 - params.turnProgress) * params.width;
    const double rSq = params.radius * params.radius;

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& p = lat.at(row, col);
            const double s = double(col) / double(kLatticeMax);
            const double sx = s * params.width;

            if (sx <= flatLen) {
                CHECK_ONCE(p.z == 0.0);
                CHECK_ONCE(relClose(p.x, spineX + sx, 1e-9));
            } else {
                const double dx = p.x - (spineX + flatLen);
                const double dz = p.z - params.radius;
                CHECK_ONCE(relClose(dx * dx + dz * dz, rSq, 1e-9));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 2. The spine never moves, for any turnProgress.
// ---------------------------------------------------------------------------

static void test_pageturn_spine_never_moves() {
    PageTurnParams params;
    params.width      = 220.0;
    params.heightSpan = 180.0;
    params.radius     = 35.0;
    params.centerX    = 5.0;
    params.centerY    = 40.0;

    const double spineX = params.centerX - 0.5 * params.width;

    for (double progress : {0.0, 0.25, 0.6, 1.0}) {
        params.turnProgress = progress;
        const Lattice lat = buildPageTurnLattice(params);
        for (int row = 0; row < kLatticeSize; ++row) {
            const Vec3& p = lat.at(row, 0);  // s == 0: col 0 is exactly the spine
            CHECK_ONCE(p.x == spineX);
            CHECK_ONCE(p.z == 0.0);
        }
    }
}

// ---------------------------------------------------------------------------
// 3. turnProgress == 0 reduces exactly to the flat (affine) case.
// ---------------------------------------------------------------------------

static void test_pageturn_flat_at_zero_progress() {
    PageTurnParams params;
    params.width        = 260.0;
    params.heightSpan   = 200.0;
    params.radius       = 45.0;  // never used: flatLen == width for every sx in [0, width]
    params.turnProgress = 0.0;
    params.centerX      = -15.0;
    params.centerY      = 30.0;

    const Lattice lat = buildPageTurnLattice(params);
    const double spineX = params.centerX - 0.5 * params.width;

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& p = lat.at(row, col);
            const double s = double(col) / double(kLatticeMax);
            const double sx = s * params.width;
            CHECK_ONCE(p.z == 0.0);
            CHECK_ONCE(relClose(p.x, spineX + sx, 1e-9));
        }
    }
}

// ---------------------------------------------------------------------------
// 4. jacobian() agrees with central differences on a genuinely curling,
//    piecewise (flat-then-curled) lattice.
// ---------------------------------------------------------------------------

static void test_pageturn_jacobian_matches_central_difference() {
    PageTurnParams params;
    params.width        = 300.0;
    params.heightSpan   = 260.0;
    params.radius       = 55.0;
    params.turnProgress = 0.5;
    params.centerX      = 8.0;
    params.centerY      = -4.0;
    const Lattice lat = buildPageTurnLattice(params);

    for (const auto& pt : kJacobianCheckPoints) checkJacobianAt(lat, pt[0], pt[1]);
}

// ---------------------------------------------------------------------------
// 5. Two layers, one set of tile bins: accumulation sums exactly, and the
//    flap is genuinely visible through the full-canvas page behind it.
// ---------------------------------------------------------------------------

namespace {

// Pass 1 + pass 2 up to (but not including) normalise/composite — the same
// tile loop core/pipeline.cpp's runFrame() uses internally, duplicated here
// because runFrame() always composites before returning and this unit needs
// the raw per-cell AccumCell to check the accumulation-sums identity
// directly, not composited colour alone. Test-only scaffolding around
// already-frozen, already-tested public API (TileBins/generateFragments,
// TileAccum/splatTile/sumBanks); does not touch or duplicate any production
// arithmetic itself.
std::vector<AccumCell> accumFromBins(TileBins& bins, int destW, int destH) {
    std::vector<AccumCell> grid(std::size_t(destW) * std::size_t(destH));
    std::vector<AccumCell> tileCells(std::size_t(kTilePixels), AccumCell{});

    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            TileAccum accum;
            splatTile(bins.tile(tx, ty), accum);
            sumBanks(accum, tileCells.data());

            const int originX = tx * kTileSize;
            const int originY = ty * kTileSize;
            const int localWidth = std::min(kTileSize, destW - originX);
            const int localHeight = std::min(kTileSize, destH - originY);
            for (int ly = 0; ly < localHeight; ++ly) {
                for (int lx = 0; lx < localWidth; ++lx) {
                    const int dx = originX + lx;
                    const int dy = originY + ly;
                    grid[std::size_t(dy) * std::size_t(destW) + std::size_t(dx)] =
                        tileCells[std::size_t(ly) * std::size_t(kTileSize) + std::size_t(lx)];
                }
            }
        }
    }
    return grid;
}

std::vector<AccumCell> accumOneLayer(const Lattice& lat, const SourceRaster& src,
                                      std::uint8_t tag, double maxK,
                                      const SupersampleConfig& ss,
                                      int destW, int destH) {
    TileBins bins(destW, destH);
    generateFragments(lat, src, maxK, ss, tag, bins);
    return accumFromBins(bins, destW, destH);
}

}  // namespace

static void test_pipeline_pageturn_transparent_accumulates_over_page_behind() {
    constexpr int kDest = 128;
    constexpr int kSrc  = 128;

    // "Page behind": a flat affine map covering the entire destination
    // canvas at 1:1, uniform colour B — full coverage everywhere, by
    // construction, so any well-covered flap pixel is guaranteed to
    // overlap it.
    const Lattice pageLat = makeAffineLattice(1.0, 1.0, 0.0, 0.0, kSrc, kSrc);
    std::vector<Sample> pageY(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(200));
    std::vector<Sample> pageCb(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(600));
    std::vector<Sample> pageCr(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(300));
    SourceRaster pageSrc;
    pageSrc.width = kSrc;
    pageSrc.height = kSrc;
    pageSrc.y = pageY.data();
    pageSrc.cb = pageCb.data();
    pageSrc.cr = pageCr.data();

    // The flap: a genuinely curling page turn, sized comfortably inside the
    // destination canvas, with a larger source raster than its own on-screen
    // footprint so compression dominates rather than magnification — the
    // same C-011 precaution WU-11's own pipeline checks apply.
    PageTurnParams flapParams;
    flapParams.width        = 80.0;
    flapParams.heightSpan   = 80.0;
    flapParams.radius       = 40.0;
    flapParams.turnProgress = 0.5;
    flapParams.centerX      = 64.0;
    flapParams.centerY      = 64.0;
    const Lattice flapLat = buildPageTurnLattice(flapParams);
    std::vector<Sample> flapY(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(850));
    std::vector<Sample> flapCb(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(450));
    std::vector<Sample> flapCr(std::size_t(kSrc) * std::size_t(kSrc), fromCode10(900));
    SourceRaster flapSrc;
    flapSrc.width = kSrc;
    flapSrc.height = kSrc;
    flapSrc.y = flapY.data();
    flapSrc.cb = flapCb.data();
    flapSrc.cr = flapCr.data();

    // Tags chosen now in anticipation of WU-12b's own priority-tag opacity
    // mechanism (see HANDOFF.md and DECISIONS.md ADR-028's scope note); not
    // read by anything yet, matching core/binner.hpp's own "not otherwise
    // used here" comment on Frag::tag.
    constexpr std::uint8_t kPageTag = 0;
    constexpr std::uint8_t kFlapTag = 1;
    constexpr double kMaxK = 1000.0;
    const SupersampleConfig ss{};

    const std::vector<AccumCell> pageOnly =
        accumOneLayer(pageLat, pageSrc, kPageTag, kMaxK, ss, kDest, kDest);
    const std::vector<AccumCell> flapOnly =
        accumOneLayer(flapLat, flapSrc, kFlapTag, kMaxK, ss, kDest, kDest);

    TileBins combinedBins(kDest, kDest);
    generateFragments(pageLat, pageSrc, kMaxK, ss, kPageTag, combinedBins);
    generateFragments(flapLat, flapSrc, kMaxK, ss, kFlapTag, combinedBins);
    const std::vector<AccumCell> combined = accumFromBins(combinedBins, kDest, kDest);

    // Accept criterion 5's core identity: architecture.md 4.7 phase 1,
    // "overlapping surfaces sum", checked exactly (I6: integer addition is
    // associative) at every destination cell, not just the well-covered
    // one below.
    for (std::size_t i = 0; i < combined.size(); ++i) {
        CHECK_ONCE(combined[i].Y == pageOnly[i].Y + flapOnly[i].Y);
        CHECK_ONCE(combined[i].Cb == pageOnly[i].Cb + flapOnly[i].Cb);
        CHECK_ONCE(combined[i].Cr == pageOnly[i].Cr + flapOnly[i].Cr);
        CHECK_ONCE(combined[i].w == pageOnly[i].w + flapOnly[i].w);
    }

    // The flap's own most solidly covered destination pixel — found
    // empirically, the same "do not assume where coverage is densest"
    // discipline CORRECTIONS.md C-011 established, rather than hand-picking
    // a coordinate.
    std::size_t bestIdx = 0;
    WeightAccum bestW = -1;
    for (std::size_t i = 0; i < flapOnly.size(); ++i) {
        if (flapOnly[i].w > bestW) {
            bestW = flapOnly[i].w;
            bestIdx = i;
        }
    }
    CHECK(bestW > 0);

    // The page-behind layer has full coverage everywhere by construction
    // (1:1 affine map over the whole canvas), so this pixel is a genuine
    // overlap of both layers.
    CHECK(pageOnly[bestIdx].w > 0);

    // The flap visibly adds weight on top of the page behind it — pure
    // accumulation, nothing occluded.
    CHECK(combined[bestIdx].w > pageOnly[bestIdx].w);

    const CompositedCell pageAloneComposited = composite(pageOnly[bestIdx]);
    const CompositedCell combinedComposited = composite(combined[bestIdx]);

    // The combined colour differs from the page-behind-alone colour by more
    // than ordinary fixed-point rounding (CORRECTIONS.md C-010's own
    // rounding-margin precedent) — proof the flap is genuinely visible
    // through it, not silently absorbed.
    constexpr int kRoundingMargin = 8;
    const int dY = std::abs(int(combinedComposited.Y) - int(pageAloneComposited.Y));
    const int dCb = std::abs(int(combinedComposited.Cb) - int(pageAloneComposited.Cb));
    const int dCr = std::abs(int(combinedComposited.Cr) - int(pageAloneComposited.Cr));
    CHECK(dY > kRoundingMargin || dCb > kRoundingMargin || dCr > kRoundingMargin);
}

int main() {
    test_pageturn_flat_or_on_curl_cylinder();
    test_pageturn_spine_never_moves();
    test_pageturn_flat_at_zero_progress();
    test_pageturn_jacobian_matches_central_difference();
    test_pipeline_pageturn_transparent_accumulates_over_page_behind();
    return scatter::test::summary("test_pageturn");
}
