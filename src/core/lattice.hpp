// scatter-dve — WU-06: control lattice, Catmull-Rom expansion, analytic
// Jacobian (architecture.md sections 4.1, 4.2)
//
// architecture.md 4.1: a shape is a function of (u, v, t) producing
// (x, y, z) in output raster space, sampled onto a coarse lattice once per
// frame — "coarse" meaning 129x129 control vertices, the direct descendant
// of the Mirage address map's every-8th-pixel addresses. This file is the
// lattice storage and its two consumers, eval() and jacobian(); it knows
// nothing about what a "shape" is. Populating the control vertices (plane,
// cylinder, sphere, page turn, ...) is Phase 2, WU-11 onward — this unit
// only needs *some* control vertex data to expand and differentiate, and
// tests/test_jacobian.cpp supplies synthetic data for that purpose.
//
// architecture.md 4.2: "Expand the lattice to per-pixel destinations with a
// Catmull-Rom (cubic) interpolant. Differentiate the same interpolant
// analytically to obtain
//
//   J = [ dx/du  dx/dv ]
//       [ dy/du  dy/dv ]
//
// eval() is that expansion — bicubic (separable) Catmull-Rom, uniform
// parameterisation, tangent at each control vertex (p_next - p_prev) / 2.
// jacobian() differentiates the identical basis polynomials term by term,
// not a separate approximation of the same surface — see lattice.cpp. This
// is what tests/test_jacobian.cpp checks against central differences of
// eval() itself.
//
// z rides along in eval() because 4.1 says a shape produces (x, y, z), but
// Jacobian is deliberately 2x2: 4.2 defines it on x and y only, because it
// drives K = 1/|det J| and the destination-raster filter footprint, both
// properties of where a source sample lands in the 2D output raster. dz/du
// and dz/dv are not needed until WU-26's surface normals (cross product of
// the two tangent vectors) and can be added there without changing this
// interface.
//
// Double precision throughout: this is evaluated once per frame on one
// thread (16 641 control vertices, architecture.md 4.1) plus one Catmull-Rom
// expansion per output pixel, not the per-fragment accumulation path I4 and
// I6 constrain to fixed-point integers. Nothing here sums across threads or
// tiles — each output pixel's position and Jacobian are a pure function of
// (u, v) and the control vertices, so ADR-015's determinism oracle is
// unaffected: IEEE 754 double arithmetic is deterministic for a fixed
// sequence of operations regardless of which thread runs it, and there is
// no cross-thread reduction here for evaluation order to disturb.
#pragma once

#include <vector>

namespace scatter {

// Control vertices per lattice edge (architecture.md 4.1).
inline constexpr int kLatticeSize = 129;
inline constexpr int kLatticeMax  = kLatticeSize - 1;  // 128, last valid index

// A control vertex, or eval()'s result: destination raster position (x, y)
// plus depth (z).
struct Vec3 {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

// architecture.md 4.2. Named for the (x, y) pair it differentiates, not for
// (u, v): dxdu reads as "d x, d u".
struct Jacobian {
    double dxdu = 0.0;
    double dxdv = 0.0;
    double dydu = 0.0;
    double dydv = 0.0;
};

class Lattice {
public:
    // kLatticeSize x kLatticeSize control vertices, zero-initialised.
    Lattice();

    // Row-major control vertex access. row is the v index, col is the u
    // index, both in [0, kLatticeMax]; out-of-range indices are the
    // caller's bug, not checked here. Populating these is a shape
    // function's job (Phase 2 onward) or a test's; this class only stores
    // and interpolates whatever is written through at().
    Vec3&       at(int row, int col) noexcept;
    const Vec3& at(int row, int col) const noexcept;

    // Bicubic Catmull-Rom expansion at continuous lattice coordinates
    // (u, v). Inputs are clamped to [0, kLatticeMax] first, so u ==
    // kLatticeMax lands exactly on the last vertex rather than one cell
    // past the end, and out-of-range callers get the clamped edge value
    // rather than undefined behaviour.
    Vec3 eval(double u, double v) const noexcept;

    // Analytic d(eval().x)/du, d(eval().x)/dv, d(eval().y)/du, d(eval().y)/dv
    // at the same (u, v), obtained by differentiating eval()'s cubic basis
    // term by term — see tests/test_jacobian.cpp for the central-difference
    // check this must agree with, interior and at the lattice edges, to
    // 1e-6 relative (WORK-UNITS.md WU-06 accept criterion).
    Jacobian jacobian(double u, double v) const noexcept;

private:
    std::vector<Vec3> vertices_;  // kLatticeSize * kLatticeSize, row-major
};

}  // namespace scatter
