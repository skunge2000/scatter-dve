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
// Jacobian was originally 2x2: 4.2 defines K = 1/|det J| and the
// destination-raster filter footprint purely from x and y, both properties
// of where a source sample lands in the 2D output raster, so those two
// consumers (core/jacobian.hpp's densityCompensation()/ewaFootprint(), and
// core/binner.cpp's pixelJacobian()) never needed dz/du, dz/dv and are
// unchanged by WU-26 adding them below. WU-26 (DECISIONS.md ADR-063) is
// architecture.md 4.2's third Jacobian-derived quantity, the surface normal
// (cross product of the two tangent vectors) — see core/jacobian.hpp's
// surfaceNormal(), which reads dzdu/dzdv alongside the original four fields.
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

    // WU-26 (DECISIONS.md ADR-063): dP/du's and dP/dv's own z components —
    // together with the x/y fields above, the full 3D tangent vectors
    // core/jacobian.hpp's surfaceNormal() needs. Additive: every existing
    // reader of this struct (densityCompensation(), ewaFootprint(),
    // pixelJacobian()) reads only the four original fields by name and is
    // unaffected by these two being appended.
    double dzdu = 0.0;
    double dzdv = 0.0;
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

    // Analytic d(eval().x)/du, d(eval().x)/dv, d(eval().y)/du, d(eval().y)/dv,
    // and (WU-26) d(eval().z)/du, d(eval().z)/dv, at the same (u, v),
    // obtained by differentiating eval()'s cubic basis term by term — see
    // tests/test_jacobian.cpp for the central-difference check this must
    // agree with, interior and at the lattice edges, to 1e-6 relative
    // (WORK-UNITS.md WU-06 accept criterion, extended to dz/du, dz/dv by
    // WU-26 using the identical method).
    Jacobian jacobian(double u, double v) const noexcept;

private:
    std::vector<Vec3> vertices_;  // kLatticeSize * kLatticeSize, row-major
};

// WU-13: architecture.md 4.1's own "temporal interpolation between shape
// lattices... Mirage's morph" — DECISIONS.md ADR-030 for the full design.
// Returns a new Lattice whose every control vertex is the componentwise
// blend from.at(row,col)*(1-t) + to.at(row,col)*t of the two inputs' own
// vertices, covering (x, y, z) alike. Exactly two keyframes, not an
// ordered sequence (ADR-030); the caller is responsible for selecting
// which two keyframes bracket a given frame and for reducing that to this
// single blend fraction t, exactly as a page-turn caller is responsible
// for reducing "how far turned" to PageTurnParams::turnProgress before
// calling buildPageTurnLattice() (ADR-028) — this function takes no frame
// number or timestamp of its own.
//
// The blend formula is deliberately from*(1-t) + to*t, not the
// algebraically equivalent from + t*(to-from): at t == 0.0, 1-t == 1.0
// exactly, so this reduces to from*1.0 + to*0.0, exactly from (both
// operations rounding-free per CORRECTIONS.md C-012); at t == 1.0,
// 1-t == 0.0 exactly, so this reduces to exactly to. t is not clamped or
// validated -- the same unchecked-precondition convention Lattice::at()'s
// row/col bounds and PageTurnParams::turnProgress already use -- so a t
// outside [0, 1] linearly extrapolates past whichever keyframe it
// overshoots rather than being sanitised away.
//
// Not noexcept: default-constructs a fresh Lattice (a std::vector<Vec3>
// allocation that can throw), the same reason buildCylinderLattice()/
// buildSphereLattice()/buildPageTurnLattice() are not noexcept either.
Lattice morphLattice(const Lattice& from, const Lattice& to, double t);

}  // namespace scatter
