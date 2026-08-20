// WU-06: lattice expansion and analytic Jacobian. WU-26 (DECISIONS.md
// ADR-063) extends this file with dz/du, dz/dv and surfaceNormal().
//
// Checks:
//
// 1. eval() reproduces control vertices exactly at integer lattice
//    coordinates. q(0) = p1 is the defining property of a Catmull-Rom
//    spline (core/lattice.cpp's basisValue(0) is (0,1,0,0) exactly) --
//    a Jacobian test built on top of a broken interpolant would not mean
//    anything.
// 2. jacobian()'s analytic dx/du, dx/dv, dy/du, dy/dv, dz/du, dz/dv agree
//    with central (or, at the domain boundary, one-sided) differences of
//    eval() to 1e-6 relative, across the lattice interior, at its edges and
//    corners, and straddling interior cell knots -- WORK-UNITS.md's WU-06
//    accept criterion (dz/du, dz/dv added by WU-26 using the identical
//    method, in checkJacobianAt() below).
// 3. (WU-26) surfaceNormal()'s cross-product z-component always equals
//    -(2x2 Jacobian determinant), independent of dz/du, dz/dv -- an
//    algebraic fact about 3D cross products, not specific to this
//    implementation, checked against real (non-synthetic) lattice content.
// 4. (WU-26) surfaceNormal()'s sign convention -- front-facing means
//    normal.z < 0, back-facing (self-folded) means normal.z >= 0 -- against
//    a real buildSphereLattice() lattice at its front-most control vertex
//    and at the antipodal control vertex of a full (angleSpanH == 2*pi)
//    self-fold, both hand-derived in DECISIONS.md ADR-063.
//
// The control vertex data checkJacobianAt() and most tests below use is
// synthetic and arbitrary. This checks that jacobian() differentiates
// whatever eval() computes, for any lattice content -- not that either
// matches some particular "shape". Real shapes (plane, cylinder, sphere,
// ...) arrive at WU-11 onward; this unit is the interpolant and its
// derivative (and, since WU-26, the normal built from it) alone. The two
// WU-26 sign-convention tests are the deliberate exception, using a real
// buildSphereLattice() lattice because the sign convention is a statement
// about this project's own front/back convention (ADR-027), not about the
// interpolant in the abstract.

#include "core/jacobian.hpp"
#include "core/lattice.hpp"
#include "core/shapes/shapes.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cmath>

using namespace scatter;

namespace {

// Deterministic, smooth and non-planar in both directions, so the
// interpolant actually curves and the derivative comparison exercises the
// cubic terms rather than degenerating to a constant slope.
Lattice makeTestLattice() {
    Lattice lat;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const double r = double(row);
            const double c = double(col);
            Vec3& p = lat.at(row, col);
            p.x = 6.0 * c + 40.0 * std::sin(0.05 * r + 0.03 * c) + 0.01 * r * c;
            p.y = 4.0 * r + 30.0 * std::cos(0.04 * r - 0.06 * c) - 0.0002 * r * r * c;
            p.z = 10.0 * std::sin(0.02 * (r + c));
        }
    }
    return lat;
}

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

// Boundary-aware first derivative of a scalar function of one variable:
// central (O(h^2)) where there is room on both sides, otherwise a 3-point
// one-sided formula (also O(h^2)) toward whichever side has room. This is
// what makes a single check function work uniformly at interior points and
// at the lattice edges and corners -- the caller never has to special-case
// where in the domain (u, v) sits.
//
// One more case needs care: an *interior* coordinate that lands exactly on
// a lattice integer is a knot, the boundary between two Catmull-Rom cells.
// core/lattice.cpp's locate() always resolves that point to the cell
// starting there (t = 0 of the cell to the right) -- see its comment.
// Catmull-Rom is C1 at a knot (value and first derivative agree from
// either side, by construction: core/lattice.cpp's basis-function comment)
// but generally *not* C2, so a symmetric central difference spanning the
// knot mixes two cells whose second derivatives differ, which degrades the
// usual O(h^2) truncation error to O(h) -- not a bug in jacobian(), just an
// invalid use of a central-difference formula across a C1-only seam.
// Forward-differencing from the knot instead stays inside the same cell
// locate() selects, matching what jacobian() actually differentiates.
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
    // Domain width (128) is vastly larger than h, so if there was no room
    // forward there is always room backward.
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
    // WU-26: same method, extended to z.
    const double numDzDu = numericDeriv(
        [&](double uu) { return lat.eval(uu, v).z; }, u, kH, kMin, kMax);
    const double numDzDv = numericDeriv(
        [&](double vv) { return lat.eval(u, vv).z; }, v, kH, kMin, kMax);

    CHECK_ONCE(relClose(j.dxdu, numDxDu, kTol));
    CHECK_ONCE(relClose(j.dydu, numDyDu, kTol));
    CHECK_ONCE(relClose(j.dxdv, numDxDv, kTol));
    CHECK_ONCE(relClose(j.dydv, numDyDv, kTol));
    CHECK_ONCE(relClose(j.dzdu, numDzDu, kTol));
    CHECK_ONCE(relClose(j.dzdv, numDzDv, kTol));
}

}  // namespace

static void test_eval_reproduces_control_points() {
    Lattice lat = makeTestLattice();
    // A representative sample, not all 16 641 -- interior, edges, corners.
    const int rows[] = {0, 1, 40, 64, 100, 127, 128};
    const int cols[] = {0, 1, 55, 64, 90, 127, 128};
    for (int row : rows) {
        for (int col : cols) {
            const Vec3 want = lat.at(row, col);
            // at()'s row is the v index, col the u index (core/lattice.hpp);
            // eval() takes (u, v), so the arguments are (col, row).
            const Vec3 got  = lat.eval(double(col), double(row));
            CHECK_ONCE(got.x == want.x);
            CHECK_ONCE(got.y == want.y);
            CHECK_ONCE(got.z == want.z);
        }
    }
}

static void test_jacobian_interior() {
    Lattice lat = makeTestLattice();
    const double pts[][2] = {
        {10.3, 20.7}, {64.5, 64.5}, {1.2, 126.8}, {126.8, 1.2},
        {45.0, 90.0}, {90.0, 45.0}, {0.5, 0.5}, {127.5, 127.5},
    };
    for (const auto& p : pts) checkJacobianAt(lat, p[0], p[1]);
}

static void test_jacobian_edges() {
    Lattice lat = makeTestLattice();
    // Several points along each of the four edges, not just one.
    const double along[] = {0.0, 0.001, 32.25, 64.0, 96.75, 127.999, 128.0};
    for (double t : along) {
        checkJacobianAt(lat, 0.0, t);
        checkJacobianAt(lat, 128.0, t);
        checkJacobianAt(lat, t, 0.0);
        checkJacobianAt(lat, t, 128.0);
    }
}

static void test_jacobian_corners() {
    Lattice lat = makeTestLattice();
    checkJacobianAt(lat, 0.0, 0.0);
    checkJacobianAt(lat, 128.0, 0.0);
    checkJacobianAt(lat, 0.0, 128.0);
    checkJacobianAt(lat, 128.0, 128.0);
}

static void test_jacobian_across_interior_knots() {
    Lattice lat = makeTestLattice();
    // Exact integer lattice coordinates in the interior are knots, where
    // numericDeriv above switches to forward differencing to stay on the
    // same side locate() does (see its comment for why).
    const double knots[] = {1.0, 30.0, 64.0, 97.0, 127.0};
    for (double u : knots) {
        for (double v : knots) checkJacobianAt(lat, u, v);
    }
}

static void test_eval_and_jacobian_clamp_out_of_range_input() {
    Lattice lat = makeTestLattice();
    // core/lattice.cpp's locate() clamps: querying beyond the lattice
    // reads the boundary value/derivative rather than extrapolating or
    // reading past the control vertex array.
    const Vec3 atMin    = lat.eval(0.0, 0.0);
    const Vec3 belowMin = lat.eval(-5.0, -5.0);
    const Vec3 atMax    = lat.eval(128.0, 128.0);
    const Vec3 aboveMax = lat.eval(133.0, 133.0);
    CHECK(belowMin.x == atMin.x && belowMin.y == atMin.y && belowMin.z == atMin.z);
    CHECK(aboveMax.x == atMax.x && aboveMax.y == atMax.y && aboveMax.z == atMax.z);

    const Jacobian jMin   = lat.jacobian(0.0, 0.0);
    const Jacobian jBelow = lat.jacobian(-5.0, -5.0);
    CHECK(jBelow.dxdu == jMin.dxdu && jBelow.dxdv == jMin.dxdv);
    CHECK(jBelow.dydu == jMin.dydu && jBelow.dydv == jMin.dydv);
    CHECK(jBelow.dzdu == jMin.dzdu && jBelow.dzdv == jMin.dzdv);
}

// WU-26 (DECISIONS.md ADR-063): surfaceNormal(j).z depends only on j's x/y
// fields (a 3D cross product's z-component never involves either input's
// own z component), so it must always equal -(the existing 2x2 Jacobian
// determinant) -- independent of dz/du, dz/dv, and for any lattice content,
// not just the sphere used by the sign-convention test below.
static void test_surface_normal_z_matches_2x2_determinant() {
    Lattice lat = makeTestLattice();
    const double pts[][2] = {
        {10.3, 20.7}, {64.5, 64.5}, {1.2, 126.8}, {126.8, 1.2},
        {0.0, 0.0}, {128.0, 128.0}, {64.0, 64.0},
    };
    for (const auto& p : pts) {
        const Jacobian j = lat.jacobian(p[0], p[1]);
        const Vec3 n = surfaceNormal(j);
        const double detJ = j.dxdu * j.dydv - j.dxdv * j.dydu;
        CHECK_ONCE(relClose(n.z, -detJ, 1e-9));
    }
}

// WU-26 (DECISIONS.md ADR-063): surfaceNormal()'s sign convention against a
// real shape, not a synthetic lattice -- this project's own front/back
// convention (core/shapes/shapes.hpp, ADR-027) is what fixes which sign is
// "correct" here, so the check needs a real buildSphereLattice() lattice,
// not arbitrary control data.
static void test_surface_normal_sign_matches_facing_convention() {
    constexpr double kPi = 3.14159265358979323846;
    using scatter::shapes::SphereParams;
    using scatter::shapes::buildSphereLattice;

    SphereParams p;
    p.angleSpanH = 2.0 * kPi;  // full wrap: the sphere self-folds (WU-28c's
                                // own motivating case, HANDOFF.md Session 36).
    const Lattice lat = buildSphereLattice(p);
    const double mid = double(kLatticeMax) / 2.0;  // 64.0: phi == psi == 0

    // Front-most control vertex (phi == psi == 0): front-facing per this
    // project's convention. DECISIONS.md ADR-063 derives dP/du parallel to
    // (radius, 0, 0), dP/dv parallel to (0, radius, 0) here, giving
    // Tv x Tu == (0, 0, -radius^2) -- negative z.
    {
        const Jacobian j = lat.jacobian(mid, mid);
        const Vec3 n = surfaceNormal(j);
        CHECK(n.z < 0.0);
    }

    // Antipodal control vertex at the fold boundary (col == 0, phi == -pi;
    // row == mid, psi == 0): the literal back of the sphere, folded away
    // from the camera. Back-facing per this project's convention.
    // DECISIONS.md ADR-063 derives dP/du parallel to (-radius, 0, 0),
    // dP/dv parallel to (0, radius, 0) here, giving Tv x Tu ==
    // (0, 0, +radius^2) -- non-negative z.
    {
        const Jacobian j = lat.jacobian(0.0, mid);
        const Vec3 n = surfaceNormal(j);
        CHECK(n.z >= 0.0);
    }
}

int main() {
    test_eval_reproduces_control_points();
    test_jacobian_interior();
    test_jacobian_edges();
    test_jacobian_corners();
    test_jacobian_across_interior_knots();
    test_eval_and_jacobian_clamp_out_of_range_input();
    test_surface_normal_z_matches_2x2_determinant();
    test_surface_normal_sign_matches_facing_convention();
    return scatter::test::summary("test_jacobian");
}
