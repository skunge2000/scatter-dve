# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 11
**Tag:** none yet. WU-11 is `wip` in `WORK-UNITS.md`, not `green` —
everything below is verified in a Linux cloud sandbox only. `./tools/close.sh
11` still needs to run on the M1 Max with AppleClang, the same two-step
close WU-10 used: this session's own commit leaves WU-11 `wip` with
everything below already verified in the sandbox; running `close.sh 11` at
the real terminal, pasting its output back, and a small follow-up commit
moving `WORK-UNITS.md` to `green` and tagging is what's left. **The
MacBook Pro was offline for most of this session by arrangement** — nothing
here has run on real hardware yet, only in the cloud sandbox, same
delivery mechanics as sessions 6 through 10 (see "Delivery mechanics"
below).
**Phase:** 2 — Shapes. WU-11 (cylinder and sphere) is the first unit in
it, in progress.

**Tests:** All eleven green in the cloud sandbox: the ten carried over
unchanged from WU-10 (`test_smoke`, `test_v210`, `test_chroma`,
`test_ramp_roundtrip`, `test_jacobian`, `test_ewa`, `test_binner`,
`test_splat`, `test_zoneplate`, `test_testpat`, none of their files
touched) and `test_shapes`, new this session, checking WU-11's own three
accept criteria directly: every control vertex `buildCylinderLattice()`/
`buildSphereLattice()` writes lies exactly on the configured surface
(`test_cylinder_vertices_lie_on_cylinder`, `test_sphere_vertices_lie_on_sphere`,
plus `test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span`
checking the two shapes' formulas agree at their shared degenerate case);
`Lattice::jacobian()` matches central differences on a populated cylinder
and a populated sphere lattice (`test_cylinder_jacobian_matches_central_difference`,
`test_sphere_jacobian_matches_central_difference`, reusing WU-06's own
method); and `runFrame()` with a flat source through a cylinder, a folded
(self-overlapping) cylinder, and a sphere all produce coverage, stay
within the source/background hull, and resolve close to the source colour
at the most solidly covered point (`test_pipeline_flat_source_through_cylinder`,
`test_pipeline_flat_source_through_folded_cylinder`,
`test_pipeline_flat_source_through_sphere`).

Verified in the Linux cloud sandbox (no AppleClang there — same caveat
every session since WU-06), on Clang 18 and GCC 13, under the project's
exact warning set (`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror`), Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all green, zero warnings — checked explicitly in the build
logs, not just exit codes), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug) at both tile sizes: clean, no ASan or
UBSan report anywhere. **Not yet run on the M1 Max with AppleClang, and
not yet run through `./tools/close.sh`** — that is what's left before this
unit can close.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13 in the cloud sandbox. AppleClang unverified this session (see
above).

## Where we are

WU-11 adds `src/core/shapes/shapes.hpp`, `src/core/shapes/cylinder.cpp` and
`src/core/shapes/sphere.cpp` — the first shapes to populate a `Lattice`'s
control vertices from a genuinely curved surface instead of a plane, the
first non-affine lattice this project builds. Nothing WU-06 through WU-10
built needed to change: `Lattice::eval()`/`jacobian()`, fragment
generation, the four-bank splat, resolve/composite and
`runFrame()`/`runFrameFile()` are all shape-agnostic already, consuming
whatever control vertices are written through `Lattice::at()` — exactly
what `HANDOFF.md`'s own "Next work unit" note going into this session
expected.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-027 in `DECISIONS.md`:** orthographic projection, not
a perspective camera (nothing else in the pipeline has a lens model —
`core/binner.cpp` already reads `Vec3::x`/`y` as literal destination-raster
pixel coordinates); a depth convention (`z == 0` at each shape's
front-most point, increasing into the screen, matching every earlier
affine lattice's own `z == 0`); the cylinder's parametrisation
(vertical axis, `theta = (s - 0.5) * angleSpan`, `x`/`z` a standard
circular cross-section, `y` linear); the sphere's parametrisation
(independent yaw/pitch gimbal angles rather than textbook
longitude/colatitude, chosen so horizontal and vertical wrap can be sized
independently — verified algebraically and in `tests/test_shapes.cpp` to
still land exactly on the sphere); why neither shape function takes
`srcWidth`/`srcHeight` (ADR-024 already normalises the lattice's own index
space); and a shared `core/shapes/shapes.hpp` rather than one header per
shape, to stay within SESSION-PROTOCOL.md's three-source-file cap (same
judgement call ADR-021 and ADR-026 already made for other units).

**One correction found this session, in `CORRECTIONS.md` as C-011** — not
a defect in any production code, and not in any earlier, already-frozen
unit's own work: a mistaken assumption in this session's own first draft
of `tests/test_shapes.cpp`. That draft assumed a shape's front-facing
point is a safe place to expect solid, near-source-colour coverage;
actually the front-facing point is exactly where magnification peaks (the
surface is most face-on to the camera there, so `d(x)/d(theta)` is
largest), meaning it can be the *sparsest*-covered part of the frame, not
the densest — confirmed directly (a 64x64 source's front-facing pixel
resolved within a few hundred codes of pure background, not source).
Fixed within this unit's own test file before it shipped: the pipeline
checks use a larger source raster so compression dominates the shape's
footprint instead of magnification, and search for whichever destination
pixel is empirically farthest from the background rather than asserting a
hand-picked coordinate. See C-011 for the full account — worth reading
before writing any future test (WU-12's page turn included) that assumes
where on a curved or folded surface coverage will be densest.

**Delivery mechanics, not a design matter:** this session ran remotely,
via the device-bridge tools connecting to this machine, same as sessions 6
through 10. All implementation and the full verification matrix above ran
first in a disposable Linux cloud sandbox, never on this machine directly
— nothing was written here until it was already green there. Files were
then written to this machine via the bridge, and `git add -A && git
commit` ran through that same bridge; as in prior sessions it still cannot
clean up its own `index.lock`/`HEAD.lock`/temp-object files afterward
(unlink fails on this mount), so stale ones were moved into `_to_delete/`
rather than removed — safe to `rm -rf _to_delete/` by hand. Git identity
was confirmed against `git log`/`git config` before committing, matching
prior sessions' `Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`.
**This session did not run `./tools/close.sh`** — it needs AppleClang on
the M1 Max, unreachable from here, and the Mac was offline for most of the
session by arrangement. The commit this session makes leaves WU-11 `wip`.

## Next steps (before WU-11 can close)

1. At the real terminal, on the M1 Max: `./tools/close.sh 11`. This
   builds Release, tile 2^5 (its own fixed config) with AppleClang and
   runs the suite, including the new `test_shapes`.
2. Paste the output back. If green, the next session (or this one,
   resumed) updates `WORK-UNITS.md`'s WU-11 line to `green`, appends a
   short confirmation to this file the way WU-10's own session-10 entry
   did, and tags `wu-11-green`.
3. If anything is red on AppleClang that was not caught by Clang
   18/GCC 13 in the sandbox, that is itself worth a `CORRECTIONS.md` entry
   — nothing like that is expected (nothing in `core/shapes/` uses
   anything platform-specific), but it has not happened before either.

## Next work unit (after WU-11 closes)

**WU-12 — Page turn, transparent and priority-tag opaque**, per
`WORK-UNITS.md`'s Phase 2. **Accept:** reproduces US 4,563,703 FIG. 5 in
both modes. Not scoped yet — `WORK-UNITS.md`'s WU-12 entry is currently a
bare heading with only that one accept line, no **Files:**. Per
SESSION-PROTOCOL.md, the next session should read `docs/architecture.md`
4.7 (transparency, both phases) and section 13's provenance note again for
FIG. 5 itself, work out the page-turn lattice parametrisation and the
priority-tag mechanism `core/types.hpp`'s `Frag::tag` already carries but
nothing yet reads meaningfully, fill in WU-12's own **Files:**/**Accept:**
lines, and only then implement — the same discipline this session's own
brief asked for going into WU-11. `core/shapes/shapes.hpp` may or may not
be the right place for a page-turn's own params struct and build function
(`buildPageTurnLattice()`?, `pageturn.cpp` per architecture.md 8's module
layout) — worth a deliberate look rather than an assumption either way,
the same "does this need its own header" question WU-11 asked of itself.

## Open questions

Unchanged from WU-10: Q1 (tile size), Q2 (4K Mini program outputs), Q3
(macOS/Desktop Video version) — all still open, none blocking. Q4
(`core/lattice.cpp`'s `jacobian()` edge damping, C-008(a)) — still open,
still not urgent; nothing in WU-11's own accept criteria exercised the
lattice's edge region in a way that hit it (this unit's own tests query
`jacobian()` at representative interior/edge/corner/knot points the same
way `test_jacobian.cpp` does, not by running a source raster larger than
`kLatticeSize` through the full pipeline the way WU-10's own C-008
discovery did).

No new open question from this session beyond what C-011 already records.

## Blocked / red

Nothing red. WU-11 is `wip`, verified in the cloud sandbox, waiting on the
M1 Max/AppleClang close described above.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-11 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-027 was appended in full earlier this session;
see `DECISIONS.md`. Not reopened now.

## Append to CORRECTIONS.md

Nothing this update — C-011 was appended in full earlier this session; see
`CORRECTIONS.md`. Not reopened now.
