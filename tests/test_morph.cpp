// WU-13: keyframed lattices, temporal interpolation ("morph") —
// core/lattice.hpp's morphLattice(), DECISIONS.md ADR-030. Checks
// WORK-UNITS.md's WU-13 accept criteria directly:
//
// 1. morphLattice(from, to, 0.0) reproduces `from` exactly, every control
//    vertex, `==` — not a tolerance — because the blend formula
//    from*(1-t) + to*t is specifically chosen (ADR-030) to be rounding-free
//    at t == 0.0: 1.0 - 0.0 == 1.0 exactly, so this reduces to
//    from*1.0 + to*0.0, both operations exact per CORRECTIONS.md C-012's
//    own "multiplying by an exact 0.0 or 1.0 introduces no rounding"
//    lesson.
// 2. morphLattice(from, to, 1.0) reproduces `to` exactly, by the same
//    argument at the other endpoint (1.0 - 1.0 == 0.0 exactly).
// 3. An interior t matches an independently-computed reference blend to a
//    tight relative tolerance, not `==` — per C-012's general lesson,
//    bit-exactness across two independently-written expressions (even the
//    "same" formula, evaluated in a different translation unit) is not
//    something to assert without a provably-rounding-free reason, and
//    interior t has none.
// 4. Lattice::jacobian()'s analytic derivatives agree with central
//    differences of eval() itself (WU-06's own method — checkJacobianAt/
//    numericDeriv/relClose below are duplicated from tests/test_jacobian.cpp
//    per SESSION-PROTOCOL.md rule 2, not shared across translation units)
//    on a lattice morphed from two distinct, genuinely curved keyframes — a
//    page turn mid-curl (which has its own flat/curl seam, ADR-028) blended
//    with a cylinder — proving the interpolant differentiates correctly on
//    real blended surface data, not just synthetic single-shape data.
//
// Not tested here, deliberately: any runFrame()-level check. This unit
// sits at WU-06's own layer (pure lattice mathematics, proven entirely
// against Lattice's own public API), not the shape layer WU-11/WU-12a sit
// at — see ADR-030's own reasoning for why no core/shapes/*, core/binner.cpp,
// core/splat.cpp, core/resolve.* or core/pipeline.cpp change is needed or
// exercised by this unit.

#include "core/lattice.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"

#include <cmath>

using namespace scatter;
using namespace scatter::shapes;

namespace {

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

// ---------------------------------------------------------------------------
// Jacobian-vs-central-difference check, duplicated from
// tests/test_jacobian.cpp (SESSION-PROTOCOL.md rule 2). See that file for
// the reasoning behind the boundary-aware one-sided fallback and the
// interior-knot special case.
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
// same coverage tests/test_jacobian.cpp, tests/test_shapes.cpp and
// tests/test_pageturn.cpp already use.
const double kJacobianCheckPoints[][2] = {
    {10.3, 20.7}, {64.5, 64.5}, {1.2, 126.8}, {126.8, 1.2},
    {45.0, 90.0}, {90.0, 45.0}, {0.5, 0.5}, {127.5, 127.5},
    {0.0, 64.0}, {128.0, 64.0}, {64.0, 0.0}, {64.0, 128.0},
    {0.0, 0.0}, {128.0, 0.0}, {0.0, 128.0}, {128.0, 128.0},
    {63.0, 64.0}, {64.0, 64.0}, {65.0, 64.0}, {63.5, 64.0}, {64.5, 64.0},
};

// ---------------------------------------------------------------------------
// Two distinct, genuinely curved keyframes: a page turn mid-curl (its own
// flat/curl seam falls at lattice column 150/300*128 = 64, same construction
// tests/test_pageturn.cpp's own Jacobian check uses) and a cylinder at a
// different radius, span and centre. Neither is affine, and they do not
// share a parametrisation, so a morph between them is a genuine blend of
// two independently authored surfaces, not a degenerate same-shape case.
// ---------------------------------------------------------------------------

Lattice makePageTurnKeyframe() {
    PageTurnParams params;
    params.width        = 300.0;
    params.heightSpan   = 250.0;
    params.radius       = 50.0;
    params.turnProgress = 0.5;
    params.centerX      = 20.0;
    params.centerY      = -10.0;
    return buildPageTurnLattice(params);
}

Lattice makeCylinderKeyframe() {
    CylinderParams params;
    params.radius     = 180.0;
    params.angleSpan  = 1.0471975511965976;  // pi/3
    params.heightSpan = 300.0;
    params.centerX    = -40.0;
    params.centerY    = 25.0;
    return buildCylinderLattice(params);
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. t == 0 reproduces `from` exactly.
// ---------------------------------------------------------------------------

static void test_morph_t0_reproduces_from_exactly() {
    const Lattice from = makePageTurnKeyframe();
    const Lattice to   = makeCylinderKeyframe();
    const Lattice morphed = morphLattice(from, to, 0.0);

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& want = from.at(row, col);
            const Vec3& got  = morphed.at(row, col);
            CHECK_ONCE(got.x == want.x);
            CHECK_ONCE(got.y == want.y);
            CHECK_ONCE(got.z == want.z);
        }
    }
}

// ---------------------------------------------------------------------------
// 2. t == 1 reproduces `to` exactly.
// ---------------------------------------------------------------------------

static void test_morph_t1_reproduces_to_exactly() {
    const Lattice from = makePageTurnKeyframe();
    const Lattice to   = makeCylinderKeyframe();
    const Lattice morphed = morphLattice(from, to, 1.0);

    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& want = to.at(row, col);
            const Vec3& got  = morphed.at(row, col);
            CHECK_ONCE(got.x == want.x);
            CHECK_ONCE(got.y == want.y);
            CHECK_ONCE(got.z == want.z);
        }
    }
}

// ---------------------------------------------------------------------------
// 3. An interior t matches an independently-computed reference blend to a
//    tight relative tolerance (C-012: not `==` — this is a differently-
//    evaluated instance of the same formula, in a different translation
//    unit, not a provably rounding-free special case the way t == 0/1 are).
// ---------------------------------------------------------------------------

static void test_morph_interior_matches_reference_blend() {
    const Lattice from = makePageTurnKeyframe();
    const Lattice to   = makeCylinderKeyframe();
    const double t = 0.35;
    const Lattice morphed = morphLattice(from, to, t);

    const double oneMinusT = 1.0 - t;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& a = from.at(row, col);
            const Vec3& b = to.at(row, col);
            const Vec3& got = morphed.at(row, col);
            const double wantX = a.x * oneMinusT + b.x * t;
            const double wantY = a.y * oneMinusT + b.y * t;
            const double wantZ = a.z * oneMinusT + b.z * t;
            CHECK_ONCE(relClose(got.x, wantX, 1e-12));
            CHECK_ONCE(relClose(got.y, wantY, 1e-12));
            CHECK_ONCE(relClose(got.z, wantZ, 1e-12));
        }
    }
}

// ---------------------------------------------------------------------------
// 4. jacobian() agrees with central differences on a lattice morphed from
//    two distinct, genuinely curved keyframes, including at points that
//    straddle the page-turn keyframe's own flat/curl seam (lattice column
//    64, per makePageTurnKeyframe()'s params — same construction
//    tests/test_pageturn.cpp's own Jacobian check uses; ADR-028 already
//    established the Catmull-Rom expansion smooths across that seam with no
//    visible defect, and morphLattice() only blends control-vertex data, so
//    the same holds for the morphed lattice).
// ---------------------------------------------------------------------------

static void test_morph_jacobian_on_curved_blend() {
    const Lattice from = makePageTurnKeyframe();
    const Lattice to   = makeCylinderKeyframe();

    for (double t : {0.0, 0.3, 0.5, 0.7, 1.0}) {
        const Lattice morphed = morphLattice(from, to, t);
        for (const auto& pt : kJacobianCheckPoints) checkJacobianAt(morphed, pt[0], pt[1]);
    }
}

int main() {
    test_morph_t0_reproduces_from_exactly();
    test_morph_t1_reproduces_to_exactly();
    test_morph_interior_matches_reference_blend();
    test_morph_jacobian_on_curved_blend();
    return scatter::test::summary("test_morph");
}
