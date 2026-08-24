// scatter-dve — WU-27: Starlight lighting, closed-form Phong evaluator
// (architecture.md's own [shading] step, section 3's signal-path diagram;
// DECISIONS.md ADR-068/069/070/071/082)
//
// This file is deliberately NOT wired into core/binner.hpp's per-sample
// loop -- that is WU-34's own job, once the coarse-grid facet-evaluation
// mechanism ADR-070 requires actually exists (ADR-070's own filtering
// ladder and grid-shift controls operate on a *coarse-grid* lattice of
// shading samples, not a call at every source sample; ADR-082 records why
// this session split "the pure illumination math" (WU-27, this file) from
// "wiring it into the raster via a coarse-grid facet" (WU-34) rather than
// building both in one unit). shade() below is a pure function of a
// surface point, a normal and a LightingScene -- it knows nothing about
// tiles, fragments or the coarse grid; tests/test_lighting.cpp is its only
// caller today.
//
// ADR-069 (S5 FIG. 3, docs/sources/WU-SM-01.md §4.6; corroborated this
// session by the primary Nonweiler patent EP 0248626/US 4,899,295 -- see
// ADR-082 for what that document does and does not confirm) gives the
// closed-form illumination equation this file implements:
//
//   I = Ia*Ka + sum over lights of (Ip/(r+k)) * (Kd*cosA + Ks*model(cosB))
//
// This is the original 1975 Phong formulation -- cosA between the surface
// normal and the direction to the light, cosB between the reflected ray
// and a *fixed* view direction (not Blinn's half-vector, not a per-pixel
// view vector) -- with linear-plus-constant distance falloff, not
// inverse-square. See shade()'s own comment below for the exact per-light
// mechanics and the design choices this session made where the sources
// leave a gap.
#pragma once

#include "core/lattice.hpp"  // Vec3

#include <cstdint>
#include <vector>

namespace scatter {

// How a light's direction and falloff are derived (docs/sources/WU-SM-01.md
// §4.6.3's own "Point / Beam / Parallel" table).
enum class LightType : std::uint8_t {
    Point,     // Light::position is a fixed world-space point; direction
               // and r are both derived per surface point.
    Beam,      // Light::direction is a fixed axis (not a ray to a point).
               // Illuminates both hemispheres of that axis symmetrically
               // -- fixture 13 ("beam bidirectionality"), WU-SM-01 §4.6.4's
               // own "beams explicitly emit out of both +z and -z" note.
               // No distance falloff, same as Parallel.
    Parallel,  // Light::direction is a fixed axis; no distance falloff
               // (WU-SM-01 §4.6.3: "parallel makes SP constant and drops
               // the 1/(r+k) term" -- implemented here as falloff == Ip
               // exactly, not Ip/(0+k)).
};

// One of Starlight's eight named per-zone specular shaping functions
// (WU-SM-01 §4.6.3, [C]: "almost certainly stage-35 look-up tables on cos
// B"). Model1..Model4 are cos^n B for four fixed exponents; the other four
// are non-power-law shapes named but not tabulated in any source held by
// this project.
//
// EVERY CURVE defaultSpecularCurve() RETURNS FOR THESE EIGHT IS A
// PROVISIONAL PLACEHOLDER, clearly arbitrary rather than a best-effort
// reconstruction -- real tabulated values are blocked on a document this
// project does not hold (docs/wu-audit-2026-08.md Deliverable 5; WU-37).
// The primary patent obtained this session (EP 0248626/US 4,899,295)
// corroborates the base Phong formula above but does not itself name or
// tabulate these eight models -- see DECISIONS.md ADR-082 for the full
// account of what it does and does not settle. Swapping in real curves
// later needs no interface change: only defaultSpecularCurve()'s own body,
// or a caller-supplied replacement of the same signature.
enum class SpecularModel : std::uint8_t {
    Model1,
    Model2,
    Model3,
    Model4,
    Ramp,
    Posterise,
    Ring2,
    Ring4,
};

// model(L, zone): cos B in [-1, 1] -> a non-negative shaping factor in
// [0, 1], clamped internally so a caller never needs to pre-clamp. See
// SpecularModel's own comment above -- every branch here is a provisional
// placeholder, not a historical reconstruction.
double defaultSpecularCurve(SpecularModel model, double cosB) noexcept;

// ADR-071: material lives on (light, zone), never on the surface. A Zone
// is nothing but a specular-model selector here -- Starlight's own
// two-zone product convention (docs/sources/WU-SM-01.md §4.4.2) is a
// caller-side choice (how many Zone entries a LightingScene carries),
// not something this file fixes at exactly two.
struct Zone {
    SpecularModel model = SpecularModel::Model1;
};

// One light source. ADR-069 (S5 FIG. 3; EP 0248626 p.6-7): Kd, Ks, Ka and
// the specular model are "per-setup constants," i.e. owned by the light
// (via its zone), never read per-sample from the surface -- ADR-071.
struct Light {
    LightType type = LightType::Point;

    // Point only: world-space position. Ignored for Beam/Parallel.
    Vec3 position{0.0, 0.0, 0.0};

    // Beam/Parallel only: a unit-length axis pointing FROM any surface
    // point TOWARD the light (not the ray's own direction of travel) --
    // caller-normalised; shade() does not renormalise it, the same
    // unchecked-precondition convention core/lattice.hpp's Lattice::at()
    // already uses for its own callers. Ignored for Point.
    Vec3 direction{0.0, 0.0, 1.0};

    double intensity = 0.0;  // Ip, signed -- fixture 12, negative-intensity
                              // cancellation; Mirage's own -400%..+400%
                              // control range is a caller-side scale, not
                              // enforced here.
    double Kd = 1.0;         // diffuse reflectivity
    double Ks = 0.0;         // specular reflectivity
    double k  = 1.0;         // S5's own "empirical constant" in the
                              // 1/(r+k) falloff (Point only)

    // Selects one entry of LightingScene::zones below (ADR-071). Unchecked
    // precondition: must index a valid entry whenever Ks != 0.0 -- the
    // same "caller's own bug, not guarded against here" convention this
    // codebase already uses throughout (e.g. Lattice::at()'s row/col).
    int zoneIndex = 0;
};

// Everything shade() needs besides the surface point and normal: the
// ambient floor, every light, the zone table those lights index into, and
// the one fixed view direction every light's specular term shares
// (ADR-069: "the line of sight LP may be assumed to be fixed" -- an
// explicit orthographic-view simplification, not a per-pixel vector;
// fixture 26 is the direct behavioural test of this).
struct LightingScene {
    double Ia = 0.0;  // ambient light intensity
    double Ka = 1.0;  // ambient reflection coefficient

    std::vector<Light> lights;
    std::vector<Zone>  zones;

    // Unit vector from any surface point TOWARD the viewer, caller-
    // normalised. Default (0, 0, -1) matches this project's own
    // z-increases-into-screen convention (core/shapes/shapes.hpp, ADR-027;
    // core/jacobian.hpp's surfaceNormal() comment) -- the viewer sits on
    // the -z side of the scene.
    Vec3 viewDirection{0.0, 0.0, -1.0};
};

// ADR-069's closed-form illumination equation (S5 FIG. 3 §4.6.1; EP
// 0248626 p.6, ADR-082), evaluated at one surface point P with surface
// normal N. N need not be unit length (core/jacobian.hpp's surfaceNormal()
// is not -- see its own comment, "a future consumer needing a unit normal
// ... normalises this result itself"; this is that consumer, and
// normalises internally).
//
//   I = Ia*Ka + sum over lights of (Ip/(r+k)) * (Kd*cosA + Ks*model(cosB))
//
// Design choices this session made where the sources leave a real gap,
// stated here rather than left silent (SESSION-PROTOCOL.md rule 6):
//
// - Per-light summation, single (not per-light) ambient floor: WU-SM-01
//   §4.6.4 flags this as genuinely open ("straight summation... is the
//   obvious reading, but it is a reading" -- [C]). Chosen because it is
//   the only reading fixtures 9 and 12 are consistent with (six lights,
//   five at zero intensity, must reduce to exactly one light's own
//   contribution; two lights differing only in intensity sign must cancel
//   to exactly zero) -- tests/test_lighting.cpp checks both directly.
// - cosA/cosB clamping: Point and Parallel lights clamp both cosines to
//   [0, inf) before use -- an ordinary one-sided Lambertian/specular
//   surface, dark to a light it does not face or reflect toward. Beam
//   lights use |cosA| and |cosB| instead, unclamped -- this is what makes
//   a Beam bidirectional (fixture 13). WU-SM-01 §4.6.4 flags the general
//   clamping question as genuinely unresolved by any source ("nothing is
//   said... a guess"), but Beam's own documented symmetric-illumination
//   behaviour is not a guess, so that much is implemented directly. [C],
//   this session's own reasoned default (not one of the two points the
//   continuation prompt named for Steve) -- see ADR-082.
// - The specular term is gated by the same (already-clamped) cosA being
//   strictly positive, the ordinary Phong convention S5 itself quotes
//   (Phong 1975): no highlight from a light a surface does not face
//   (Point/Parallel) or is exactly grazing (Beam). [C], same tier as
//   above.
//
// Returns the light intensity factor I, not a colour -- multiplying I
// into RGB, or any ADD-mode alternative to that (S3's own "Spectral
// MULTIPLY vs ADD" control), is deliberately out of this function's own
// scope; see this file's own header comment and WORK-UNITS.md's WU-34
// entry.
double shade(const LightingScene& scene, const Vec3& P, const Vec3& N) noexcept;

}  // namespace scatter
