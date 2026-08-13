// WU-07: K density compensation and EWA elliptical filter footprint from J.
//
// Checks two things against known affine cases (architecture.md 4.2, and
// WORK-UNITS.md's WU-07 accept criterion):
//
// 1. K = 1/|det J|: exact for pure scale, pure rotation and shear, and
//    clamps to a caller-chosen maxK under extreme compression, including
//    the degenerate det J == 0 fold.
// 2. ewaFootprint(): the ellipse's semi-axis lengths match closed-form
//    singular values computed independently in this file, from the
//    trace/determinant of J^T J rather than core/jacobian.hpp's own
//    J*J^T decomposition — a different algebraic route to the same
//    eigenvalues, so this is not the implementation checking itself, the
//    same relationship test_jacobian.cpp's numericDeriv() has to
//    lattice.cpp's analytic differentiation. Checked for pure scale, pure
//    rotation and shear; the major axis' direction is checked against the
//    analytic angle for pure scale and shear (a pure rotation's footprint
//    is a circle, where direction is undefined — see core/jacobian.hpp).
//
// As in test_jacobian.cpp, the Jacobian values here are synthetic affine
// cases chosen because their singular values and axis directions are
// known in closed form, not because they come from any particular shape
// (Phase 2, WU-11 onward).

#include "core/jacobian.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cmath>

using namespace scatter;

namespace {

constexpr double kPi = 3.14159265358979323846;

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

Jacobian makeJ(double dxdu, double dxdv, double dydu, double dydv) noexcept {
    Jacobian j;
    j.dxdu = dxdu;
    j.dxdv = dxdv;
    j.dydu = dydu;
    j.dydv = dydv;
    return j;
}

// Independent reference: singular values of J from the trace and
// determinant of J^T J (standard quadratic-formula eigenvalues of a 2x2
// matrix via its characteristic polynomial), rather than
// core/jacobian.hpp's J*J^T, p/q/r decomposition.
struct RefSingularValues {
    double sigma1;  // larger
    double sigma2;  // smaller
};

RefSingularValues referenceSingularValues(const Jacobian& j) noexcept {
    const double a = j.dxdu, b = j.dxdv, c = j.dydu, d = j.dydv;
    const double trace = a * a + b * b + c * c + d * d;  // tr(J^T J)
    const double det   = a * d - b * c;                  // det(J)
    // det(J^T J) = det(J^T) * det(J) = det(J)^2.
    const double disc  = std::max(trace * trace - 4.0 * det * det, 0.0);
    const double root  = std::sqrt(disc);
    const double lambda1 = 0.5 * (trace + root);
    const double lambda2 = 0.5 * (trace - root);
    return {std::sqrt(std::max(lambda1, 0.0)), std::sqrt(std::max(lambda2, 0.0))};
}

}  // namespace

static void test_k_pure_scale_and_rotation_and_shear() {
    const double kMaxK = 1.0e6;  // far above anything these cases produce

    // Pure scale: det J = sx * sy exactly.
    const double scales[][2] = {{2.0, 3.0}, {5.0, 5.0}, {0.25, 8.0}, {1.0, 1.0}};
    for (const auto& s : scales) {
        const Jacobian j = makeJ(s[0], 0.0, 0.0, s[1]);
        const double want = 1.0 / std::fabs(s[0] * s[1]);
        CHECK_ONCE(relClose(densityCompensation(j, kMaxK), want, 1e-12));
    }

    // Pure rotation: det J = 1 for any angle.
    const double angles[] = {0.0, 0.3, kPi / 4.0, kPi / 2.0, 2.1, -1.7};
    for (double theta : angles) {
        const Jacobian j = makeJ(std::cos(theta), -std::sin(theta),
                                  std::sin(theta), std::cos(theta));
        CHECK_ONCE(relClose(densityCompensation(j, kMaxK), 1.0, 1e-9));
    }

    // Shear: det J = 1 for [[1, k], [0, 1]] regardless of k.
    const double shears[] = {0.5, 1.0, 2.0, -1.5, 10.0};
    for (double k : shears) {
        const Jacobian j = makeJ(1.0, k, 0.0, 1.0);
        CHECK_ONCE(relClose(densityCompensation(j, kMaxK), 1.0, 1e-9));
    }
}

static void test_k_clamps_at_maximum_compression() {
    const double kMaxK = 1000.0;

    // Heavy but nonzero compression: det J well below 1/maxK.
    const Jacobian j = makeJ(1e-4, 0.0, 0.0, 1e-4);  // det = 1e-8, raw K = 1e8
    CHECK(densityCompensation(j, kMaxK) == kMaxK);

    // Degenerate fold: det J == 0 exactly (the whole footprint collapses
    // to a point — permitted by I1). Raw K is IEEE 754 +inf, well-defined,
    // and must still clamp rather than propagate infinity or NaN.
    const Jacobian collapsed = makeJ(0.0, 0.0, 0.0, 0.0);
    const double kCollapsed = densityCompensation(collapsed, kMaxK);
    CHECK(kCollapsed == kMaxK);
    CHECK(std::isfinite(kCollapsed));

    // A near-collapse that should NOT clamp: raw K just under maxK.
    const Jacobian nearly = makeJ(0.1, 0.0, 0.0, 0.02);  // det = 0.002, raw K = 500
    CHECK(relClose(densityCompensation(nearly, kMaxK), 500.0, 1e-9));

    // A fold with negative determinant (orientation flip, still permitted
    // by I1): K depends on |det J| only, sign must not matter.
    const Jacobian folded = makeJ(1e-5, 0.0, 0.0, -1e-5);  // det = -1e-10
    CHECK(densityCompensation(folded, kMaxK) == kMaxK);
}

static void test_footprint_pure_scale() {
    const double scales[][2] = {{2.0, 3.0}, {3.0, 2.0}, {5.0, 5.0}, {0.5, 4.0}};
    for (const auto& s : scales) {
        const Jacobian j = makeJ(s[0], 0.0, 0.0, s[1]);
        const EwaFootprint f = ewaFootprint(j);
        const double wantMajor = std::max(std::fabs(s[0]), std::fabs(s[1]));
        const double wantMinor = std::min(std::fabs(s[0]), std::fabs(s[1]));
        CHECK_ONCE(relClose(f.majorAxis, wantMajor, 1e-9));
        CHECK_ONCE(relClose(f.minorAxis, wantMinor, 1e-9));

        const RefSingularValues ref = referenceSingularValues(j);
        CHECK_ONCE(relClose(f.majorAxis, ref.sigma1, 1e-9));
        CHECK_ONCE(relClose(f.minorAxis, ref.sigma2, 1e-9));

        if (s[0] != s[1]) {
            // Major axis aligned with whichever of x/y has the larger
            // scale: angle 0 if x, +-pi/2 if y.
            const double wantAngle =
                (std::fabs(s[0]) > std::fabs(s[1])) ? 0.0 : (kPi / 2.0);
            CHECK_ONCE(relClose(std::fabs(f.majorAngle), wantAngle, 1e-9));
        }
    }
}

static void test_footprint_pure_rotation_is_a_circle() {
    const double angles[] = {0.0, 0.3, kPi / 4.0, kPi / 2.0, 2.1, -1.7};
    for (double theta : angles) {
        const Jacobian j = makeJ(std::cos(theta), -std::sin(theta),
                                  std::sin(theta), std::cos(theta));
        const EwaFootprint f = ewaFootprint(j);
        CHECK_ONCE(relClose(f.majorAxis, 1.0, 1e-9));
        CHECK_ONCE(relClose(f.minorAxis, 1.0, 1e-9));
    }
}

static void test_footprint_shear() {
    const double shears[] = {0.5, 1.0, 2.0, -1.5, 10.0};
    for (double k : shears) {
        const Jacobian j = makeJ(1.0, k, 0.0, 1.0);
        const EwaFootprint f = ewaFootprint(j);
        const RefSingularValues ref = referenceSingularValues(j);
        CHECK_ONCE(relClose(f.majorAxis, ref.sigma1, 1e-9));
        CHECK_ONCE(relClose(f.minorAxis, ref.sigma2, 1e-9));
        // Product of the axes is |det J| = 1 for this family, independent
        // of the eigen-decomposition route either side took to get there.
        CHECK_ONCE(relClose(f.majorAxis * f.minorAxis, 1.0, 1e-9));
    }
}

static void test_footprint_total_collapse_is_a_point() {
    // Degenerate J: both axes zero, angle left at the documented default.
    const Jacobian collapsed = makeJ(0.0, 0.0, 0.0, 0.0);
    const EwaFootprint f = ewaFootprint(collapsed);
    CHECK(f.majorAxis == 0.0);
    CHECK(f.minorAxis == 0.0);
    CHECK(f.majorAngle == 0.0);
}

int main() {
    test_k_pure_scale_and_rotation_and_shear();
    test_k_clamps_at_maximum_compression();
    test_footprint_pure_scale();
    test_footprint_pure_rotation_is_a_circle();
    test_footprint_shear();
    test_footprint_total_collapse_is_a_point();
    return scatter::test::summary("test_ewa");
}
