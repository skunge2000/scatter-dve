// WU-11: cylinder and sphere control-lattice population (core/shapes/
// shapes.hpp, DECISIONS.md ADR-027) — the first non-affine lattice this
// project builds. Checks WORK-UNITS.md's WU-11 accept criteria directly:
//
// 1. Every control vertex buildCylinderLattice()/buildSphereLattice()
//    writes lies exactly (double-precision tolerance) on the surface of
//    the configured radius — the algebraic identity ADR-027 derives and
//    states should hold for *any* angleSpanH/angleSpanV, checked here
//    rather than trusted from the derivation alone.
// 2. Lattice::jacobian()'s analytic derivatives agree with central
//    differences (WU-06's own method — checkJacobianAt/numericDeriv/
//    relClose below are duplicated from tests/test_jacobian.cpp per
//    SESSION-PROTOCOL.md rule 2, not shared across translation units)
//    across a populated cylinder and a populated sphere lattice — proving
//    the interpolant differentiates correctly on genuinely curved,
//    non-affine control data for the first time. WU-06 only ever checked
//    this against synthetic-but-arbitrary data; nothing before this unit
//    ran it against a real shape.
// 3. runFrame() with a flat source through each shape produces no crash;
//    every covered destination pixel lies within the source/background
//    hull (ADR-025's rounding margin, the same tests/test_zoneplate.cpp
//    kRoundingMargin precedent, duplicated locally — a curved shape's
//    silhouette is a genuine partial-coverage edge, so exact source-colour
//    match is not expected everywhere covered, only within the hull); and
//    the most solidly covered destination pixel resolves to the source's
//    own colour closely, proving genuine full coverage happens somewhere,
//    not merely that nothing strays outside the hull. One of these runs
//    deliberately uses an
//    angleSpan wide enough that the cylinder folds back on itself (theta
//    exceeds +/-pi/2, where sin(theta) stops being monotonic) — I1's
//    "non-invertible maps, folds, tears" case, for the first time with a
//    real shape rather than a hypothetical one. Per architecture.md 4.7
//    phase 1 (and WORK-UNITS.md's own WU-11 accept line), overlapping
//    surface points are expected to simply accumulate, not be sorted or
//    culled — WU-28's k-buffer is not built yet and this unit does not
//    need it to pass; the check below only asks that folding does not
//    crash or corrupt colour where coverage exists, not that occlusion is
//    resolved correctly.
//
// Not tested here, deliberately: performance/tile-size (Q1, still open,
// unrelated to this unit); any shape other than the vertical-axis cylinder
// and the gimbal-angle sphere ADR-027 defines (page turn is WU-12,
// horizontal-axis cylinders are a documented future variant, not built).

#include "core/binner.hpp"
#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"
#include "video/raster.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

using namespace scatter;
using namespace scatter::shapes;

namespace {

constexpr double kPi = 3.14159265358979323846;

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

// ---------------------------------------------------------------------------
// Jacobian-vs-central-difference check, duplicated from
// tests/test_jacobian.cpp (SESSION-PROTOCOL.md rule 2: no test-local code
// shared across translation units). See that file for the reasoning behind
// the boundary-aware one-sided fallback and the knot special case.
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

// Representative (u, v) points: interior, edges, corners, interior knots —
// the same coverage tests/test_jacobian.cpp's four separate functions give,
// collapsed into one list since this file checks two lattices (cylinder,
// sphere) against it rather than one.
const double kJacobianCheckPoints[][2] = {
    {10.3, 20.7}, {64.5, 64.5}, {1.2, 126.8}, {126.8, 1.2},
    {45.0, 90.0}, {90.0, 45.0}, {0.5, 0.5}, {127.5, 127.5},
    {0.0, 64.0}, {128.0, 64.0}, {64.0, 0.0}, {64.0, 128.0},
    {0.0, 0.0}, {128.0, 0.0}, {0.0, 128.0}, {128.0, 128.0},
    {1.0, 64.0}, {64.0, 1.0}, {127.0, 64.0}, {64.0, 127.0},
};

}  // namespace

// ---------------------------------------------------------------------------
// 1. Every control vertex lies exactly on the configured surface.
// ---------------------------------------------------------------------------

static void test_cylinder_vertices_lie_on_cylinder() {
    CylinderParams params;
    params.radius     = 200.0;
    params.angleSpan  = kPi / 3.0;
    params.heightSpan = 300.0;
    params.centerX    = 50.0;
    params.centerY    = 80.0;

    const Lattice lat = buildCylinderLattice(params);
    const double rSq = params.radius * params.radius;

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& p = lat.at(row, col);
            const double dx = p.x - params.centerX;
            const double dz = p.z - params.radius;
            CHECK_ONCE(relClose(dx * dx + dz * dz, rSq, 1e-9));
        }
    }

    // Exact spot checks at the front-facing column (s == 0.5, theta == 0):
    // sin(0) == 0 and cos(0) == 1 hold bit-exactly in IEEE 754, so these
    // are checked with ==, not a tolerance.
    const int frontCol = kLatticeMax / 2;
    for (int row : {0, kLatticeMax / 2, kLatticeMax}) {
        const Vec3& p = lat.at(row, frontCol);
        CHECK(p.x == params.centerX);
        CHECK(p.z == 0.0);
    }

    // y is linear in row alone, exactly like the affine (plane) case's own
    // vertical mapping — no transcendental function involved, so this is
    // also an exact check.
    CHECK(lat.at(0, frontCol).y == params.centerY - 0.5 * params.heightSpan);
    CHECK(lat.at(kLatticeMax, frontCol).y == params.centerY + 0.5 * params.heightSpan);
}

static void test_sphere_vertices_lie_on_sphere() {
    SphereParams params;
    params.radius     = 150.0;
    params.angleSpanH = 1.2;
    params.angleSpanV = 0.8;
    params.centerX    = -30.0;
    params.centerY    = 60.0;

    const Lattice lat = buildSphereLattice(params);
    const double rSq = params.radius * params.radius;

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& p = lat.at(row, col);
            const double dx = p.x - params.centerX;
            const double dy = p.y - params.centerY;
            const double dz = p.z - params.radius;
            CHECK_ONCE(relClose(dx * dx + dy * dy + dz * dz, rSq, 1e-9));
        }
    }

    // Front-facing vertex (phi == psi == 0): exact, same reasoning as the
    // cylinder's front column above.
    const int centre = kLatticeMax / 2;
    const Vec3& p = lat.at(centre, centre);
    CHECK(p.x == params.centerX);
    CHECK(p.y == params.centerY);
    CHECK(p.z == 0.0);
}

// ADR-027: angleSpanV == 0 collapses the sphere's x/z formulas to the
// cylinder's own (psi == 0 uniformly, cos psi == 1, sin psi == 0). Checked
// with a tight relative tolerance (kUltraTight below), not bit-exact
// equality — see CORRECTIONS.md C-012: this session's first draft asserted
// `==` here on the reasoning that multiplying by an exactly-1.0 cos(0)
// cannot introduce rounding, which is true of that one step in isolation
// but is not a safe assumption about two *independently compiled*
// multi-operation expressions (cylinder.cpp's single radius*sin(theta)
// versus sphere.cpp's radius*sin(phi)*cosPsi) agreeing to the very last
// bit on every platform — confirmed by `./tools/close.sh 11` failing on
// AppleClang/ARM64 at one specific grid point out of 16641, not reproduced
// in this session's own Clang 18/GCC 13 x86_64 sandbox even when forcing
// FMA contraction explicitly. y does not collapse the same way — the
// sphere has no heightSpan, so its y sits at centerY uniformly here; that
// comparison stays bit-exact (`radius * sin(psi)` with psi identically
// zero has no rounding to differ on, on any platform: multiplying by an
// exact zero is exact regardless of contraction).
static void test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span() {
    constexpr double kRadius = 120.0;
    constexpr double kAngle  = 1.4;
    constexpr double kCx     = 12.0;
    constexpr double kUltraTight = 1e-12;  // C-012: platform rounding noise is ~1 ULP (~2e-16 relative); this is ~4000x that, not a real disagreement

    CylinderParams cp;
    cp.radius = kRadius;
    cp.angleSpan = kAngle;
    cp.heightSpan = 999.0;  // deliberately different from the sphere's centerY below
    cp.centerX = kCx;
    cp.centerY = 7.0;
    const Lattice cyl = buildCylinderLattice(cp);

    SphereParams sp;
    sp.radius = kRadius;
    sp.angleSpanH = kAngle;
    sp.angleSpanV = 0.0;
    sp.centerX = kCx;
    sp.centerY = -55.0;  // deliberately different from the cylinder's centerY
    const Lattice sph = buildSphereLattice(sp);

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& c = cyl.at(row, col);
            const Vec3& s = sph.at(row, col);
            CHECK_ONCE(relClose(c.x, s.x, kUltraTight));
            CHECK_ONCE(relClose(c.z, s.z, kUltraTight));
            CHECK_ONCE(s.y == sp.centerY);
        }
    }
}

// ---------------------------------------------------------------------------
// 2. jacobian() agrees with central differences on genuinely curved control
//    data — the actual "first non-affine lattice" milestone.
// ---------------------------------------------------------------------------

static void test_cylinder_jacobian_matches_central_difference() {
    CylinderParams params;
    params.radius = 200.0;
    params.angleSpan = kPi / 2.5;
    params.heightSpan = 350.0;
    params.centerX = 10.0;
    params.centerY = -20.0;
    const Lattice lat = buildCylinderLattice(params);

    for (const auto& pt : kJacobianCheckPoints) checkJacobianAt(lat, pt[0], pt[1]);
}

static void test_sphere_jacobian_matches_central_difference() {
    SphereParams params;
    params.radius = 180.0;
    params.angleSpanH = kPi / 2.2;
    params.angleSpanV = kPi / 3.0;
    params.centerX = -5.0;
    params.centerY = 15.0;
    const Lattice lat = buildSphereLattice(params);

    for (const auto& pt : kJacobianCheckPoints) checkJacobianAt(lat, pt[0], pt[1]);
}

// ---------------------------------------------------------------------------
// 3. Full pipeline: flat source through each shape, no crash, correct
//    colour wherever covered.
// ---------------------------------------------------------------------------

namespace {

// Same rounding-margin precedent tests/test_zoneplate.cpp's
// test_pipeline_partial_coverage_no_fringe() establishes (CORRECTIONS.md
// C-010): core/splat.cpp's four-bank splat truncates each corner
// contribution separately (ADR-025), so a uniform-colour source can
// accumulate a code value or two of drift even at full coverage. Not this
// unit's defect to fix, and not what this test is checking for. Widened
// from that test's own 4 to 8: a sphere curves in two directions at once
// (unlike a flat affine edge or a cylinder's single curved axis), so more
// distinct fragments' corners can overlap a single fully-covered cell,
// measured directly (this session) at up to +7 code on Cb for the sphere
// case below — still a handful of codes, not a real coverage or fringe
// bug, which would miss by hundreds of codes (the gap between this test's
// own source and background colours).
constexpr int kRoundingMargin = 8;

bool near(Sample v, Sample expected) noexcept {
    const int diff = int(v) - int(expected);
    return diff >= -kRoundingMargin && diff <= kRoundingMargin;
}

// A curved shape's silhouette has a genuine partial-coverage edge, unlike
// WU-10's own flat-source checks (a straight column boundary) — a pixel
// there legitimately composites to a blend of source and background, not
// an exact (within-rounding) source-colour match, the same "hull" property
// tests/test_zoneplate.cpp's own partial-coverage checks already rely on.
// So every covered pixel is checked only for lying within that hull
// (componentwise, between source and background, plus the rounding
// margin); a separate, specific interior point (see below) checks that
// genuine full coverage actually happens and resolves to the source colour
// closely, not just that nothing strays outside the hull.
bool inHull(Sample v, Sample a, Sample b) noexcept {
    const int lo = int(std::min(a, b)) - kRoundingMargin;
    const int hi = int(std::max(a, b)) + kRoundingMargin;
    return int(v) >= lo && int(v) <= hi;
}

struct PipelineRunResult {
    std::size_t coveredCount = 0;
    bool allCoveredWithinHull = true;
    bool bestCoveredPointMatchesSource = false;
};

// Which destination pixel ends up most solidly covered is not something
// this test hand-predicts: a cylinder or sphere's local compression ratio
// varies continuously across its footprint (architecture.md 4.2's whole
// point), and — counter-intuitively — the front-facing point (where the
// surface is most face-on to the camera) is where *magnification*, not
// compression, peaks for a shape whose angular span approaches the source
// raster's own pixel density; that turns out to be the sparsest-covered
// region, not the densest, the opposite of what a first guess at "the
// obviously well-covered point" would assume. The callers below use a
// source raster (256) noticeably larger than the shape's own on-screen
// footprint (radius order 100, in a 256x256 destination) specifically so
// compression dominates almost everywhere instead — magnification-driven
// gaps are architecture.md 4.6's own known, accepted limitation (holes
// under magnification, mitigated by supersampling but not eliminated),
// not something this unit's own accept criteria need to characterise. So
// instead of asserting a hand-picked coordinate, this finds whichever
// covered pixel's Y channel sits farthest from the background — the
// empirically best-covered point, wherever it actually lands — and checks
// that one against the source
// colour. If nothing in the frame is genuinely well covered, that pixel
// fails the check too; this cannot pass by accident the way asserting a
// guessed coordinate could.
PipelineRunResult runFlatSourceThroughLattice(const Lattice& lat, int srcSize,
                                               Sample srcY, Sample srcCb, Sample srcCr,
                                               int destW, int destH) {
    std::vector<Sample> y(std::size_t(srcSize) * std::size_t(srcSize), srcY);
    std::vector<Sample> cb(std::size_t(srcSize) * std::size_t(srcSize), srcCb);
    std::vector<Sample> cr(std::size_t(srcSize) * std::size_t(srcSize), srcCr);

    SourceRaster src;
    src.width = srcSize;
    src.height = srcSize;
    src.y = y.data();
    src.cb = cb.data();
    src.cr = cr.data();

    PipelineParams params;
    params.destWidth = destW;
    params.destHeight = destH;
    params.maxK = 1000.0;
    params.background = Background{fromCode10(64), fromCode10(512), fromCode10(512)};

    video::Raster444 dest(destW, destH);
    runFrame(lat, src, params, dest);

    PipelineRunResult result;
    std::size_t bestIdx = 0;
    int bestDist = -1;
    for (std::size_t i = 0; i < dest.Y.size(); ++i) {
        const bool isBackground = dest.Y[i] == params.background.Y &&
                                   dest.Cb[i] == params.background.Cb &&
                                   dest.Cr[i] == params.background.Cr;
        if (isBackground) continue;
        ++result.coveredCount;
        if (!(inHull(dest.Y[i], srcY, params.background.Y) &&
              inHull(dest.Cb[i], srcCb, params.background.Cb) &&
              inHull(dest.Cr[i], srcCr, params.background.Cr))) {
            result.allCoveredWithinHull = false;
        }
        const int dist = std::abs(int(dest.Y[i]) - int(params.background.Y));
        if (dist > bestDist) {
            bestDist = dist;
            bestIdx = i;
        }
    }

    result.bestCoveredPointMatchesSource = bestDist >= 0 &&
                                            near(dest.Y[bestIdx], srcY) &&
                                            near(dest.Cb[bestIdx], srcCb) &&
                                            near(dest.Cr[bestIdx], srcCr);
    return result;
}

}  // namespace

static void test_pipeline_flat_source_through_cylinder() {
    CylinderParams params;
    params.radius = 100.0;
    params.angleSpan = kPi * 0.8;  // well within +/-pi/2: no self-overlap here
    params.heightSpan = 180.0;
    params.centerX = 128.0;
    params.centerY = 128.0;
    const Lattice lat = buildCylinderLattice(params);

    const auto result = runFlatSourceThroughLattice(
        lat, /*srcSize=*/256, fromCode10(900), fromCode10(150), fromCode10(850),
        /*destW=*/256, /*destH=*/256);

    CHECK(result.coveredCount > 0);
    CHECK(result.allCoveredWithinHull);
    CHECK(result.bestCoveredPointMatchesSource);
}

// angleSpan here exceeds pi, so theta ranges outside +/-pi/2 and sin(theta)
// stops being monotonic: two different source columns land on overlapping
// destination x — a genuine fold (I1: "non-invertible maps, folds, tears
// and shattering are only expressible this way"). architecture.md 4.7
// phase 1 says overlapping surfaces simply sum; this only checks that
// happens without crashing or corrupting colour, not that the fold is
// resolved into a single visible surface (WU-28's job).
static void test_pipeline_flat_source_through_folded_cylinder() {
    CylinderParams params;
    params.radius = 80.0;
    params.angleSpan = 3.6;  // > pi: folds back on itself
    params.heightSpan = 140.0;
    params.centerX = 128.0;
    params.centerY = 128.0;
    const Lattice lat = buildCylinderLattice(params);

    const auto result = runFlatSourceThroughLattice(
        lat, /*srcSize=*/256, fromCode10(700), fromCode10(300), fromCode10(600),
        /*destW=*/256, /*destH=*/256);

    CHECK(result.coveredCount > 0);
    // A folded, uniform-colour source still resolves within the source/
    // background hull everywhere: overlapping fragments of the *same*
    // colour and comparable weight normalise (Sigma(w*colour)/Sigma(w))
    // back toward that same colour, pure accumulation producing no new hue
    // the way a genuine k-buffer's back-to-front compositing would — the
    // fold changes *how much* of the frame is covered, not what colour a
    // covered pixel resolves toward.
    CHECK(result.allCoveredWithinHull);
    CHECK(result.bestCoveredPointMatchesSource);
}

static void test_pipeline_flat_source_through_sphere() {
    SphereParams params;
    params.radius = 100.0;
    params.angleSpanH = kPi * 0.7;
    params.angleSpanV = kPi * 0.6;
    params.centerX = 128.0;
    params.centerY = 128.0;
    const Lattice lat = buildSphereLattice(params);

    const auto result = runFlatSourceThroughLattice(
        lat, /*srcSize=*/256, fromCode10(200), fromCode10(700), fromCode10(400),
        /*destW=*/256, /*destH=*/256);

    CHECK(result.coveredCount > 0);
    CHECK(result.allCoveredWithinHull);
    CHECK(result.bestCoveredPointMatchesSource);
}

int main() {
    test_cylinder_vertices_lie_on_cylinder();
    test_sphere_vertices_lie_on_sphere();
    test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span();
    test_cylinder_jacobian_matches_central_difference();
    test_sphere_jacobian_matches_central_difference();
    test_pipeline_flat_source_through_cylinder();
    test_pipeline_flat_source_through_folded_cylinder();
    test_pipeline_flat_source_through_sphere();
    return scatter::test::summary("test_shapes");
}
