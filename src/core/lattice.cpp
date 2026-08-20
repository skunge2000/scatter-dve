// scatter-dve — WU-06: see core/lattice.hpp for the design note.
#include "core/lattice.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace scatter {

namespace {

// ---------------------------------------------------------------------------
// Uniform Catmull-Rom Hermite basis, one dimension.
//
// For a parameter t in [0, 1] between control points p1 and p2 (with
// neighbours p0 and p3 supplying the tangents), the standard Catmull-Rom
// matrix form is
//
//   q(t) = h0(t)*p0 + h1(t)*p1 + h2(t)*p2 + h3(t)*p3
//
//   h0(t) = -0.5t^3 +   t^2 - 0.5t
//   h1(t) =  1.5t^3 - 2.5t^2       + 1
//   h2(t) = -1.5t^3 +  2t^2 + 0.5t
//   h3(t) =  0.5t^3 - 0.5t^2
//
// q(0) = p1, q(1) = p2, q'(0) = (p2-p0)/2, q'(1) = (p3-p1)/2 — the uniform
// Catmull-Rom tangent choice, which is what makes adjacent cells agree on
// value and first derivative at their shared knot (C1 continuity): cell
// [p1,p2]'s q'(1) and cell [p2,p3]'s q'(0) are both (p3-p1)/2, the same
// expression evaluated from the same three points. basisDeriv below is
// dh/dt of the four polynomials above, term by term — jacobian() uses it
// in place of basisValue in whichever of (u, v) it is differentiating, so
// the derivative is exact for the identical surface eval() evaluates, not
// a separate approximation of it.
// ---------------------------------------------------------------------------

struct Basis {
    double h0, h1, h2, h3;
};

Basis basisValue(double t) noexcept {
    const double t2 = t * t;
    const double t3 = t2 * t;
    return Basis{
        -0.5 * t3 + 1.0 * t2 - 0.5 * t,
         1.5 * t3 - 2.5 * t2 + 1.0,
        -1.5 * t3 + 2.0 * t2 + 0.5 * t,
         0.5 * t3 - 0.5 * t2,
    };
}

Basis basisDeriv(double t) noexcept {
    const double t2 = t * t;
    return Basis{
        -1.5 * t2 + 2.0 * t - 0.5,
         4.5 * t2 - 5.0 * t,
        -4.5 * t2 + 4.0 * t + 0.5,
         1.5 * t2 - 1.0 * t,
    };
}

// A continuous lattice coordinate, split into the cell it falls in and the
// fractional offset within that cell. Clamps first, so u == kLatticeMax
// (the last vertex) resolves to the last cell at t = 1 rather than an
// out-of-range cell index, and any caller that over/undershoots gets the
// clamped edge behaviour rather than reading past the control vertex array.
struct Cell {
    int    index;  // in [0, kLatticeMax - 1]
    double frac;   // in [0, 1]
};

Cell locate(double u) noexcept {
    const double c = std::clamp(u, 0.0, double(kLatticeMax));
    int i = static_cast<int>(std::floor(c));
    if (i >= kLatticeMax) i = kLatticeMax - 1;  // u == kLatticeMax: last cell, t = 1
    return Cell{i, c - double(i)};
}

}  // namespace

Lattice::Lattice()
    : vertices_(std::size_t(kLatticeSize) * std::size_t(kLatticeSize)) {}

Vec3& Lattice::at(int row, int col) noexcept {
    return vertices_[std::size_t(row) * std::size_t(kLatticeSize) + std::size_t(col)];
}

const Vec3& Lattice::at(int row, int col) const noexcept {
    return vertices_[std::size_t(row) * std::size_t(kLatticeSize) + std::size_t(col)];
}

namespace {

// Weighted sum over the 4x4 control-vertex neighbourhood around cell
// (uIndex, vIndex): separable bicubic blend, one basis (bv) for the v
// direction and one (bu) for the u direction. eval() passes basisValue for
// both; jacobian() passes basisDeriv for whichever direction it is
// differentiating and basisValue for the other, which is exactly what
// differentiating the double sum below term by term produces (product
// rule, holding the untouched direction's basis fixed).
//
// row/col range over [index-1, index+2]; at a boundary cell that runs
// outside [0, kLatticeMax], so each coordinate is clamped independently to
// the nearest edge control vertex before the lookup — edge replication,
// the same choice ADR-020 makes for chroma resampling's filter taps,
// applied here to the lattice's control-vertex stencil instead of a
// sample row.
Vec3 blend(const Lattice& lattice, const Basis& bv, const Basis& bu,
           int vIndex, int uIndex) noexcept {
    const double wv[4] = {bv.h0, bv.h1, bv.h2, bv.h3};
    const double wu[4] = {bu.h0, bu.h1, bu.h2, bu.h3};
    Vec3 out;
    for (int a = 0; a < 4; ++a) {
        const int row = vIndex - 1 + a;
        for (int b = 0; b < 4; ++b) {
            const int col = uIndex - 1 + b;
            const double w = wv[a] * wu[b];
            const Vec3& p = lattice.at(std::clamp(row, 0, kLatticeMax),
                                        std::clamp(col, 0, kLatticeMax));
            out.x += w * p.x;
            out.y += w * p.y;
            out.z += w * p.z;
        }
    }
    return out;
}

}  // namespace

Vec3 Lattice::eval(double u, double v) const noexcept {
    const Cell cu = locate(u);
    const Cell cv = locate(v);
    const Basis bu = basisValue(cu.frac);
    const Basis bv = basisValue(cv.frac);
    return blend(*this, bv, bu, cv.index, cu.index);
}

// WU-13: see core/lattice.hpp for the design note and DECISIONS.md
// ADR-030 for the full rationale, in particular why the blend is written
// as from*(1-t) + to*t rather than the algebraically equivalent
// from + t*(to-from) -- this form alone is bit-exact at t == 0.0 and
// t == 1.0.
Lattice morphLattice(const Lattice& from, const Lattice& to, double t) {
    Lattice out;
    const double oneMinusT = 1.0 - t;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3& a = from.at(row, col);
            const Vec3& b = to.at(row, col);
            Vec3& o = out.at(row, col);
            o.x = a.x * oneMinusT + b.x * t;
            o.y = a.y * oneMinusT + b.y * t;
            o.z = a.z * oneMinusT + b.z * t;
        }
    }
    return out;
}

Jacobian Lattice::jacobian(double u, double v) const noexcept {
    const Cell cu = locate(u);
    const Cell cv = locate(v);
    const Basis bu  = basisValue(cu.frac);
    const Basis bv  = basisValue(cv.frac);
    const Basis dbu = basisDeriv(cu.frac);
    const Basis dbv = basisDeriv(cv.frac);

    // Lattice-coordinate cells have unit width, so dt/du == dt/dv == 1 and
    // no chain-rule scale factor is needed beyond substituting the
    // differentiated basis for the direction being differentiated.
    const Vec3 du = blend(*this, bv, dbu, cv.index, cu.index);   // d/du
    const Vec3 dv = blend(*this, dbv, bu, cv.index, cu.index);   // d/dv

    Jacobian j;
    j.dxdu = du.x;
    j.dydu = du.y;
    j.dxdv = dv.x;
    j.dydv = dv.y;
    // WU-26 (DECISIONS.md ADR-063): du/dv above are already the full Vec3
    // blend (blend() sums x, y and z alike), so their own z components are
    // exactly d(eval().z)/du and d(eval().z)/dv — no extra lattice
    // evaluation needed, reusing what was already computed above rather
    // than duplicating it (the same reuse-not-duplicate reasoning
    // DECISIONS.md ADR-062 already applied when rejecting a
    // finite-difference shortcut around this addition).
    j.dzdu = du.z;
    j.dzdv = dv.z;
    return j;
}

}  // namespace scatter
