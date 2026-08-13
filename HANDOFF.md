# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 6
**Tag:** `wu-06-green` — confirmed. `./tools/close.sh 06` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All six green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat` (all five unchanged
from WU-05, none of their files touched) and `test_jacobian`, new this
session (411 checks).

Before that, this session verified in a Linux sandbox (no AppleClang
there), on Clang 18 and GCC 13, under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all green),
plus the full suite under GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug): clean, no ASan or UBSan report anywhere
— same practice as prior sessions.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max), Clang 18 and GCC 13.

## Where we are

WU-06 done and closed green. `src/core/lattice.hpp`/`lattice.cpp` add
`Lattice`: a 129×129 grid of `Vec3` control vertices (`x`, `y`, `z`,
`double`), `eval(u, v)` (bicubic Catmull-Rom expansion) and `jacobian(u, v)`
(analytic `dx/du`, `dx/dv`, `dy/du`, `dy/dv`, differentiating the identical
basis polynomials `eval()` uses, term by term — not a separate
approximation). `tests/test_jacobian.cpp` checks the analytic Jacobian
against central differences of `eval()` to 1e-6 relative, across the
lattice interior, its four edges, its four corners, and interior integer
coordinates (cell knots) — the WU-06 accept criterion — plus two sanity
checks: `eval()` reproduces control vertices exactly at integer
coordinates, and both `eval()`/`jacobian()` clamp rather than misbehave on
out-of-range input. `CMakeLists.txt`: `src/core/lattice.cpp` added to
`scatter-core`'s sources, `test_jacobian` registered via `scatter_test()`.
Matches WU-06's file list in `WORK-UNITS.md` exactly — no deviation, unlike
WU-05.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-022 in `DECISIONS.md`:** uniform (not
centripetal/chordal) Catmull-Rom parametrisation, chosen because the
lattice's own indexing is uniform; edge replication for control-vertex
lookups outside the lattice, the same choice ADR-020 makes for chroma's
filter taps; and that `jacobian()` stays 2×2 (`x`, `y` only) even though
`eval()` carries `z` too, per architecture.md 4.2's own stated definition
of `J` — `dz/du`/`dz/dv` are deferred to WU-26's surface normals. None of
this reopens architecture.md, which left these open on purpose.

**A real finding, not just a design choice — worth reading before writing
more finite-difference tests against anything in `src/core/`:** a Catmull-
Rom spline is C1 at its knots (value and first derivative agree from
either side — provable directly from the basis polynomials) but generally
**not** C2. A symmetric central difference straddling a knot mixes two
cells with different second derivatives, degrading the usual O(h²) central-
difference truncation error to O(h) — about four orders of magnitude worse
at `h = 1e-4`, comfortably enough to blow through a 1e-6 relative
tolerance. This surfaced as `test_jacobian.cpp` failing at exactly the
points you'd least expect a bug to hide (integer lattice coordinates).
Confirmed by hand that it is not a bug in `jacobian()`: both cells'
formulas reduce to the identical expression `(P[i+1] - P[i-1]) / 2` at a
shared knot. `tests/test_jacobian.cpp`'s `numericDeriv()` now detects an
interior-knot input and switches to forward differencing, staying inside
the same cell `Lattice::eval()`'s own convention (`locate()` in
`lattice.cpp`) picks for that point, rather than working around a symptom
of the seam.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, no separate laptop
session open for most of it. Files were written directly, and `git add -A
&& git commit` ran through that same bridge; as in prior sessions it still
cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink fails on this mount), so stale ones — including one left
over from the end of session 5, present before this session touched
anything — were moved into `_to_delete/` rather than removed. Safe to
`rm -rf _to_delete/` by hand. This session also had to set a *local* (not
`--global`) git identity on the bridge's sandboxed VM, which had none
configured — separate from the normal Terminal environment, where
`./tools/close.sh 06` was, as before, run by hand; that is the one step
this session could not do itself, having no access to a real terminal on
this machine.

## Next work unit

**WU-07 — K and EWA footprint from J**, per `WORK-UNITS.md`.
**Files:** `src/core/jacobian.hpp`, `tests/test_ewa.cpp`.
**Accept:** `K = 1/|det J|` correct for known affine cases; ellipse axes
match analytic values for pure scale, pure rotation and shear; clamping at
the configured maximum compression behaves. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-06 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-07 and costs no session time.

## Append to DECISIONS.md

ADR-022 was appended in full this session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session. (Two bugs surfaced while writing `test_jacobian.cpp`
— a row/col-vs-u/v transposition and the knot central-difference issue
above — but both were caught and fixed before anything was committed, so
neither reached a state file or a claim anyone believed; see prior
session's note in git history if useful, not repeated here.)
