// scatter-dve — WU-11: cylinder and sphere control-lattice population
// (architecture.md 4.1 "shape as a control lattice"; module layout section
// 8 names src/core/shapes/cylinder.cpp, sphere.cpp; DECISIONS.md ADR-027
// for the projection model, depth convention and both shapes' own
// parametrisations this session had to choose — the same kind of gap
// ADR-020/022/023/024/025/026 filled for earlier units.)
//
// This is the first shape to populate a Lattice's control vertices from a
// genuinely curved surface instead of a plane — WU-01 through WU-10 only
// ever exercised affine (planar) lattices, built test-locally
// (makeAffineLattice/makePixelAffineLattice; a shipped plane.cpp does not
// exist yet, per HANDOFF.md). Everything downstream of a populated Lattice
// — eval(), jacobian(), fragment generation, the four-bank splat,
// resolve/composite, runFrame()/runFrameFile() — is shape-agnostic already
// (it consumes whatever control vertices are written through Lattice::at())
// and needs no change here.
//
// Both build functions below write all kLatticeSize x kLatticeSize control
// vertices of a fresh Lattice and return it by value (guaranteed copy
// elision, same convention tests/test_zoneplate.cpp's own
// makeAffineLattice() already uses). Orthographic projection throughout
// (ADR-027): a shape's (x, y) is its 3D surface point's (x, y) directly,
// offset by a caller-supplied centre, not divided by depth or otherwise
// projected through a camera/lens model — no such model exists anywhere
// else in this pipeline (core/binner.cpp reads Vec3::x/y as literal
// destination-raster pixel coordinates), and z carries straight through as
// a depth scalar, zero at each shape's front-most (nearest-camera) point
// and increasing into the screen, matching every earlier (affine, z == 0
// uniformly) lattice's own convention. Neither function reads
// srcWidth/srcHeight: ADR-024 already normalises core/binner.cpp's
// pixelToLattice() so a lattice's own index space is resolution-
// independent, and radius/angleSpan/heightSpan are absolute (output
// pixels, radians), not per-source-pixel scale factors the way the
// test-local affine helpers' scaleX/scaleY are.
//
// Double precision throughout, same layering and rationale as
// core/lattice.hpp, core/jacobian.hpp and core/binner.hpp: this runs once
// per frame, at 16 641 control vertices (architecture.md 4.1), not in the
// fixed-point I4/I6 accumulation path.
#pragma once

#include "core/lattice.hpp"

namespace scatter::shapes {

// Cylinder, axis parallel to the destination raster's y axis — the
// standard vertical "roll" cylinder-DVE look (ADR-027). radius and
// heightSpan are output pixels; angleSpan is radians, the total horizontal
// wrap, centred on the front-facing point (destination-raster fraction 0.5
// across the source). centerX/centerY are the output-pixel position of
// that front-facing point at the lattice's vertical centre.
struct CylinderParams {
    double radius     = 200.0;
    double angleSpan  = 1.5707963267948966;  // pi/2: a quarter-turn total, an eighth each side of front
    double heightSpan = 400.0;
    double centerX    = 0.0;
    double centerY    = 0.0;
};

// Populates a fresh Lattice's control vertices with ADR-027's cylinder
// parametrisation: theta = (s - 0.5) * angleSpan, x = centerX +
// radius*sin(theta), z = radius*(1 - cos(theta)), y = centerY + (t - 0.5)
// * heightSpan, where s = col/kLatticeMax, t = row/kLatticeMax range over
// [0, 1] across the control-vertex grid. Every returned vertex satisfies
// (x - centerX)^2 + (z - radius)^2 == radius^2 exactly (to double-precision
// rounding) — a point on a cylinder of the given radius, by construction.
Lattice buildCylinderLattice(const CylinderParams& params);

// Sphere. radius is output pixels; angleSpanH/angleSpanV are radians, the
// total horizontal and vertical wrap, each centred on the front-facing
// point independently (ADR-027 — not a single solid angle, so a Mirage-
// style sphere effect can be wrapped differently per axis, the same
// independence the cylinder already has between angleSpan and heightSpan).
// centerX/centerY are the output-pixel position of the front-facing point.
struct SphereParams {
    double radius     = 200.0;
    double angleSpanH = 1.5707963267948966;  // pi/2
    double angleSpanV = 1.5707963267948966;  // pi/2
    double centerX    = 0.0;
    double centerY    = 0.0;
};

// Populates a fresh Lattice's control vertices with ADR-027's sphere
// parametrisation: phi = (s - 0.5) * angleSpanH, psi = (t - 0.5) *
// angleSpanV, x = centerX + radius*sin(phi)*cos(psi), y = centerY +
// radius*sin(psi), z = radius*(1 - cos(phi)*cos(psi)) — independent
// yaw/pitch angles, not textbook longitude/colatitude (see ADR-027 for the
// derivation and why it still lands exactly on the sphere). Setting
// angleSpanV to 0 reduces this exactly to buildCylinderLattice()'s own
// formula. Every returned vertex satisfies (x - centerX)^2 + (y -
// centerY)^2 + (z - radius)^2 == radius^2 exactly (to double-precision
// rounding), for any angleSpanH/angleSpanV.
Lattice buildSphereLattice(const SphereParams& params);

}  // namespace scatter::shapes
