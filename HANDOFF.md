# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 11
**Tag:** `wu-11-green` — confirmed. `./tools/close.sh 11` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds)
and tagged it, on the second attempt (see "Two close.sh attempts" below).
**Phase:** 2 — Shapes. WU-11, its first unit, is done. WU-12 (page turn)
starts next; see below.

**Tests:** All eleven green on the M1 Max: the ten carried over unchanged
from WU-10 (`test_smoke`, `test_v210`, `test_chroma`, `test_ramp_roundtrip`,
`test_jacobian`, `test_ewa`, `test_binner`, `test_splat`, `test_zoneplate`,
`test_testpat`, none of their files touched) and `test_shapes`, new this
session, checking all three of WU-11's own accept criteria directly: every
control vertex `buildCylinderLattice()`/`buildSphereLattice()` writes lies
exactly on the configured surface
(`test_cylinder_vertices_lie_on_cylinder`, `test_sphere_vertices_lie_on_sphere`,
plus `test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span`
checking the two shapes' formulas agree at their shared degenerate case);
`Lattice::jacobian()` matches central differences on a populated cylinder
and a populated sphere lattice (`test_cylinder_jacobian_matches_central_difference`,
`test_sphere_jacobian_matches_central_difference`, reusing WU-06's own
method); and `runFrame()` with a flat source through a cylinder, a folded
(self-overlapping) cylinder, and a sphere all produce coverage, stay
within the source/background hull, and resolve close to the source colour
at the most solidly covered point.

Before that, this session verified in a Linux cloud sandbox (no
AppleClang there), on Clang 18 and GCC 13, under the project's exact
warning set (`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror`), Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all green, zero warnings — checked explicitly in the
build logs, not just exit codes), plus GCC 13 with `-fsanitize=address,
undefined -fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no
ASan or UBSan report anywhere — same practice as prior sessions.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max), Clang 18 and GCC 13.

## Where we are

WU-11 adds `src/core/shapes/shapes.hpp`, `src/core/shapes/cylinder.cpp`
and `src/core/shapes/sphere.cpp` — the first shapes to populate a
`Lattice`'s control vertices from a genuinely curved surface instead of a
plane, the first non-affine lattice this project builds. Nothing WU-06
through WU-10 built needed to change: `Lattice::eval()`/`jacobian()`,
fragment generation, the four-bank splat, resolve/composite and
`runFrame()`/`runFrameFile()` are all shape-agnostic already, consuming
whatever control vertices are written through `Lattice::at()`.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-027 in `DECISIONS.md`:** orthographic projection, not
a perspective camera (nothing else in the pipeline has a lens model —
`core/binner.cpp` already reads `Vec3::x`/`y` as literal destination-raster
pixel coordinates); a depth convention (`z == 0` at each shape's
front-most point, increasing into the screen, matching every earlier
affine lattice's own `z == 0`); the cylinder's parametrisation (vertical
axis, `theta = (s - 0.5) * angleSpan`, `x`/`z` a standard circular
cross-section, `y` linear); the sphere's parametrisation (independent
yaw/pitch gimbal angles rather than textbook longitude/colatitude, chosen
so horizontal and vertical wrap can be sized independently — verified
algebraically and in `tests/test_shapes.cpp` to still land exactly on the
sphere); why neither shape function takes `srcWidth`/`srcHeight` (ADR-024
already normalises the lattice's own index space); and a shared
`core/shapes/shapes.hpp` rather than one header per shape, to stay within
SESSION-PROTOCOL.md's three-source-file cap.

**Two corrections found this session, in `CORRECTIONS.md` as C-011 and
C-012** — both in this session's own test-writing, neither a defect in
`core/shapes/cylinder.cpp` or `sphere.cpp`, and neither in any earlier,
already-frozen unit's own work:

- **C-011** — a mistaken assumption in this session's own first draft of
  `tests/test_shapes.cpp`: that a shape's front-facing point is a safe
  place to expect solid, near-source-colour coverage. Actually the
  front-facing point is exactly where magnification peaks (the surface is
  most face-on to the camera there), meaning it can be the *sparsest*-
  covered part of the frame, not the densest. Caught and fixed before
  shipping: the pipeline checks use a larger source raster so compression
  dominates the shape's footprint, and search for whichever destination
  pixel is empirically farthest from the background rather than asserting
  a hand-picked coordinate.
- **C-012** — found by real hardware, not the cloud sandbox (see "Two
  close.sh attempts" below): asserting bit-exact (`==`) equality between
  two *differently-shaped* floating-point expressions (`cylinder.cpp`'s
  one-multiply `radius*sin(theta)` versus `sphere.cpp`'s two-multiply
  `radius*sin(phi)*cosPsi`) is not safe across compilers/platforms, even
  when the algebra guarantees they compute the same real number — FMA
  contraction, reassociation and transcendental rounding can all legally
  differ. Fixed with a tight relative tolerance (`1e-12`) in place of `==`
  for that one comparison; every other floating-point check in the file
  was swept for the same risk and left as-is (either already tolerance-
  based, or providably rounding-free — see C-012's own account).

## Two `close.sh 11` attempts

The first attempt was red: `test_shapes.cpp:230`, `c.x == s.x`, 1 of 83385
checks. That is C-012 above. The fix touched only
`tests/test_shapes.cpp` and `CORRECTIONS.md` — `core/shapes/cylinder.cpp`
and `sphere.cpp` were never touched, and the fix was re-verified across
the full eight-configuration-plus-sanitizers matrix in the cloud sandbox
before asking for a second `close.sh 11` run, which came back green. This
is exactly what `SESSION-PROTOCOL.md`'s cloud-sandbox-first process is
for: it does not catch everything (a platform-specific floating-point
rounding difference is precisely the kind of thing it can miss, having no
AppleClang/ARM64 of its own to test against), but it means a red result on
real hardware is rare and, when it happens, isolated enough to fix and
re-verify quickly rather than being an actual design defect.

**Delivery mechanics, not a design matter:** this session ran remotely,
via the device-bridge tools connecting to this machine, same as sessions 6
through 10. All implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly.
Files were then written to this machine via the bridge, and `git add -A
&& git commit` ran through that same bridge, twice (once for the initial
WU-11 commit, once for the C-012 fix); as in prior sessions it still
cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones were moved into
`_to_delete/` rather than removed — safe to `rm -rf _to_delete/` by hand.
Git identity was already set locally on this mount from a prior session
(`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`, confirmed
against `git log`/`git config` before committing), so nothing needed
reconfiguring. This machine was offline for part of the session by
arrangement; nothing here ran locally until it reconnected. `./tools/
close.sh 11` was, as before, run by hand at the real terminal — twice,
per the paragraph above.

## Next work unit

**WU-12 — Page turn, transparent and priority-tag opaque**, per
`WORK-UNITS.md`'s Phase 2. **Accept:** reproduces US 4,563,703 FIG. 5 in
both modes. **Files/Accept:** not yet scoped beyond that one accept line
— `WORK-UNITS.md`'s WU-12 entry is currently a bare heading plus the
accept line alone, unlike WU-02 through WU-11's, which all had **Files:**
filled in before their session started. Per SESSION-PROTOCOL.md, the next
session should:

- Re-read `docs/architecture.md` 4.7 in full (both transparency phases —
  phase 1's pure accumulation, already built, and phase 2's k-buffer,
  still WU-28's) and section 13's provenance note again specifically for
  FIG. 5 (US 4,563,703) — a page turn's own geometry and its transparent-
  vs-opaque behaviour are the actual patent claim this unit reproduces,
  not just a shape parametrisation exercise the way WU-11 was.
- Work out the page-turn lattice parametrisation from first principles
  the same way WU-11 worked out cylinder/sphere — likely a rolling or
  folding surface parametrised by a "turn" progress fraction (possibly
  the unit's own `t` parameter mentioned in architecture.md 4.1's "a
  shape is a function of `(u, v, t)`", not used by any shape yet — WU-11's
  cylinder/sphere are both static, no time-varying `t` — this may be the
  first unit that actually needs it, or `t` may still be WU-13's
  (keyframed lattices/morph) to introduce; worth a deliberate look rather
  than assuming either way going in).
- Work out the priority-tag mechanism: `core/types.hpp`'s `Frag::tag`
  already exists (copied through unchanged by `core/binner.cpp` since
  WU-08) but nothing yet reads it meaningfully — "opaque with priority tag
  set" (architecture.md's own test-plan entry, section 9) is the first
  place this unit's accept criterion needs it to actually do something.
  Given Phase 1's transparency is pure accumulation with no k-buffer
  (ADR-009, unchanged), "opaque" here almost certainly means something
  narrower than real depth-sorted occlusion — worth being precise about
  what it *can* mean without WU-28, rather than either under- or
  over-building it.
- Decide whether `core/shapes/shapes.hpp` is the right home for a
  page-turn's own params struct and build function
  (`buildPageTurnLattice()`?, `pageturn.cpp` per architecture.md 8's
  module layout), or whether it needs its own header — the same "does
  this need its own header" judgement call WU-11 made of itself, not to
  be assumed either way without looking.
- Fill in WU-12's own **Files:**/**Accept:** lines, respecting the sizing
  cap, and only then implement.

## Open questions

Unchanged from WU-10: Q1 (tile size), Q2 (4K Mini program outputs), Q3
(macOS/Desktop Video version) — all still open, none blocking. Q4
(`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; nothing in WU-11's own accept criteria exercised the
lattice's edge region in a way that hit it.

No new open question from this session beyond what C-011/C-012 already
record — both are closed corrections, not open questions.

## Blocked / red

Nothing. WU-11 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-12 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-027 was appended in full earlier this session;
see `DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this update — C-011 and C-012 were appended in full earlier this
session; see `CORRECTIONS.md`. Not reopened or amended now that the tag is
confirmed.
