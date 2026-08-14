// scatter-dve — WU-11: cylinder and sphere control-lattice population
// (architecture.md 4.1 "shape as a control lattice"; module layout section
// 8 names src/core/shapes/cylinder.cpp, sphere.cpp; DECISIONS.md ADR-027
// for the projection model, depth convention and both shapes' own
// parametrisations this session had to choose — the same kind of gap
// ADR-020/022/023/024/025/026 filled for earlier units.)
//
// WU-12a adds PageTurnParams/buildPageTurnLattice below (module layout
// section 8 names src/core/shapes/pageturn.cpp) — see DECISIONS.md ADR-028
// for the page-turn surface's own parametrisation, and for why its
// turn-progress fraction is this shape's own parameter, not
// architecture.md 4.1's shape-function t (still WU-13's, keyframed
// lattices/temporal interpolation between whole lattices — a different
// mechanism). Same shared-header rationale as ADR-027 gave for cylinder
// and sphere: one params struct and one build function per shape, declared
// together here rather than in per-shape headers, to stay within
// SESSION-PROTOCOL.md's three-source-file cap.
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

// Page turn (US 4,563,703 FIG. 5; ADR-028). A rectangular sheet, hinged
// along a vertical spine that never moves, the rest of which lies flat
// against the destination raster for the portion nearest the spine and
// rolls into a partial cylinder of the configured radius for the portion
// that has turned — the split point between flat and curled is
// turnProgress, not a fixed geometric boundary. width and heightSpan are
// output pixels: width spans spine to free edge when fully flat
// (turnProgress == 0); heightSpan is the spine's own (straight, uncurved)
// extent, exactly like the cylinder's own heightSpan. radius is the curl's
// cylinder radius, output pixels. turnProgress in [0, 1]: 0 leaves the
// whole sheet flat (reduces exactly to the affine/plane case); 1 curls the
// sheet starting from the spine itself, with no flat portion left.
// centerX/centerY are the output-pixel position of the flat sheet's own
// centre — the spine itself sits at (centerX - width/2), for any
// turnProgress, since it never moves.
struct PageTurnParams {
    double width        = 400.0;
    double heightSpan   = 400.0;
    double radius       = 60.0;
    double turnProgress = 0.0;
    double centerX      = 0.0;
    double centerY      = 0.0;
};

// Populates a fresh Lattice's control vertices with ADR-028's page-turn
// parametrisation. s = col/kLatticeMax, t = row/kLatticeMax range over
// [0, 1] across the control-vertex grid, as ADR-027 already establishes;
// spineX = centerX - width/2; flatLen = (1 - turnProgress) * width is how
// far from the spine, measured along the flat sheet, the flat/curl split
// falls; sx = s * width is a control vertex's own distance from the spine
// along that same measure.
//
// For sx <= flatLen (still flat): x = spineX + sx, z = 0.
// For sx > flatLen (turned): a = sx - flatLen (arc-length distance into
// the curl — the sheet does not stretch as it feeds into the roll), theta
// = a / radius, x = spineX + flatLen + radius*sin(theta), z = radius*(1 -
// cos(theta)) — the same cylinder cross-section ADR-027 already derives
// for buildCylinderLattice, reused here for the curled portion, arc-length
// parametrised (rather than a fixed angular span) so the flat and curled
// pieces meet with matching position and tangent at sx == flatLen for any
// turnProgress: at theta == 0, dx/da == cos(0) == 1 and dz/da == sin(0) ==
// 0, exactly the flat piece's own dx/dsx == 1, dz/dsx == 0 — see ADR-028
// for the full derivation. y = centerY + (t - 0.5) * heightSpan
// throughout, unchanged along the spine direction, exactly like the
// cylinder's own vertical mapping.
//
// Every control vertex in the curled region satisfies (x - (spineX +
// flatLen))^2 + (z - radius)^2 == radius^2 exactly (to double-precision
// rounding) — a point on a cylinder of the given radius, by construction,
// the same identity ADR-027 states for buildCylinderLattice. The spine
// column (s == 0) satisfies x == spineX and z == 0 exactly, for any
// turnProgress — sx == 0 is never greater than flatLen (flatLen >= 0
// always, since turnProgress is expected in [0, 1]), so the spine is
// always in the flat branch, and radius never enters the calculation
// there regardless of its value. radius must be positive whenever
// turnProgress > 0 (theta divides by it wherever the curled branch is
// reached) — not checked here, the same unchecked-precondition convention
// this codebase already uses for Lattice::at()'s row/col bounds.
Lattice buildPageTurnLattice(const PageTurnParams& params);

}  // namespace scatter::shapes
