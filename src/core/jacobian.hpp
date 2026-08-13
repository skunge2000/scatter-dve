// scatter-dve — WU-07: density compensation (K) and EWA filter footprint
// from J (architecture.md section 4.2)
//
// architecture.md 4.2 lists three things the Jacobian yields "at once,
// which is why it is the centre of the design": density compensation K,
// the elliptical filter footprint, and (WU-26) the surface normal. This
// file is the first two, built purely from the Jacobian struct
// core/lattice.hpp already defines — it knows nothing about the lattice
// itself, only the 2x2 linear map at one (u, v).
//
// K = 1 / |det J| — architecture.md's own formula — "clamped to a
// configured maximum compression". architecture.md fixes the formula but
// not the clamp's value or where it lives; see densityCompensation()'s
// comment for the choice made here, and DECISIONS.md ADR-023.
//
// Filter footprint: "J *is* the elliptical filter kernel" — the image of
// the unit circle in source parameter space {(du, dv) : du^2 + dv^2 = 1}
// under the linear map J is an ellipse in destination (x, y) space, and
// that ellipse is the footprint one source sample should splat across at
// the destination. Its semi-axis lengths are J's singular values; its
// major axis' direction is the corresponding left singular vector.
// ewaFootprint() gets both from the closed-form eigen-decomposition of the
// symmetric 2x2 matrix J*J^T below, exact for 2x2 — there is no larger
// matrix anywhere in this path to justify a general SVD routine. See
// ADR-023 for why J*J^T rather than J^T*J.
//
// Double precision throughout, same rationale as core/lattice.hpp: this
// runs once per destination pixel per frame, not in the fixed-point I4/I6
// accumulation path, and every function here is a pure function of its
// arguments — nothing here sums across threads or tiles for evaluation
// order to disturb.
#pragma once

#include "core/lattice.hpp"

#include <algorithm>
#include <cmath>

namespace scatter {

// The elliptical filter footprint J implies at one destination pixel: the
// image of the unit source-parameter circle under J (architecture.md 4.2).
//
// majorAxis and minorAxis are the ellipse's semi-axis lengths in
// destination pixels, majorAxis >= minorAxis >= 0 — J's two singular
// values. majorAngle is the major axis' direction, as an angle in radians
// from the destination +x axis, in (-pi/2, pi/2]. When the footprint is a
// circle (majorAxis == minorAxis, e.g. a pure rotation, or any J whose two
// singular values coincide) every direction is a major axis and
// majorAngle is left at 0 rather than computed from a degenerate
// eigenvector problem.
struct EwaFootprint {
    double majorAxis  = 0.0;
    double minorAxis  = 0.0;
    double majorAngle = 0.0;
};

// architecture.md 4.2: K = 1 / |det J|, clamped to a configured maximum
// compression. The clamp is the caller-supplied parameter maxK, not a
// project-wide constant: architecture.md leaves its numeric value open
// (ADR-023), and unlike kLatticeSize or the tile size it is not a property
// of a data structure this project fixes once — it belongs to whichever
// accumulation-stage configuration (WU-09 onward) chooses an operating
// point, and nothing in WU-07 has grounds to pick a number nobody has
// decided yet.
//
// |det J| == 0 — a fold collapsing an area to a line or point, which I1
// permits — makes raw K infinite; IEEE 754 double division by zero yields
// +inf here, well-defined and not UB, and clamps to maxK exactly like any
// other extreme compression.
inline double densityCompensation(const Jacobian& j, double maxK) noexcept {
    const double detJ = j.dxdu * j.dydv - j.dxdv * j.dydu;
    const double rawK = 1.0 / std::abs(detJ);
    return std::min(rawK, maxK);
}

// architecture.md 4.2's elliptical filter kernel: the image of the unit
// circle in (du, dv) under J. Semi-axis lengths are J's singular values,
// obtained from the eigenvalues of J*J^T in closed form — exact for 2x2,
// not an iterative SVD. J*J^T (not J^T*J, whose eigenvalues are identical
// but whose eigenvectors describe directions in source (u, v) space) is
// used because its eigenvectors give the axis directions in destination
// (x, y) space, which is what a filter footprint needs.
//
// With J = [dxdu dxdv; dydu dydv], J*J^T = [[p, r], [r, q]] where
// p = dxdu^2 + dxdv^2, q = dydu^2 + dydv^2, r = dxdu*dydu + dxdv*dydv —
// row_x . row_x, row_y . row_y, row_x . row_y respectively. This
// symmetric matrix's eigenvalues are (p+q)/2 +/- sqrt(((p-q)/2)^2 + r^2),
// the standard 2x2 symmetric-eigenvalue formula, and the singular values
// are their square roots. The eigenvector angle for the larger eigenvalue
// is the standard principal-axis formula for a symmetric 2x2 matrix,
// 0.5 * atan2(2r, p - q).
inline EwaFootprint ewaFootprint(const Jacobian& j) noexcept {
    const double p = j.dxdu * j.dxdu + j.dxdv * j.dxdv;
    const double q = j.dydu * j.dydu + j.dydv * j.dydv;
    const double r = j.dxdu * j.dydu + j.dxdv * j.dydv;

    const double halfDiff = 0.5 * (p - q);
    const double f = std::sqrt(halfDiff * halfDiff + r * r);
    const double e = 0.5 * (p + q);

    EwaFootprint out;
    out.majorAxis = std::sqrt(std::max(e + f, 0.0));
    out.minorAxis = std::sqrt(std::max(e - f, 0.0));
    if (f > 0.0) {
        out.majorAngle = 0.5 * std::atan2(2.0 * r, p - q);
    }
    return out;
}

}  // namespace scatter
