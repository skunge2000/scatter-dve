# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 6
**Tag:** none yet. `wu-06-green` is **not** created — this session ran
without access to a real terminal on the M1 Max (remote, via the
device-bridge tools described below), so `./tools/close.sh 06` has not run.
Do that first, at the real terminal: `cd ~/src/scatter-dve && ./tools/close.sh 06`.
On a clean AppleClang build and green `ctest` it tags `wu-06-green`; then
flip `WORK-UNITS.md`'s WU-06 line from `wip` to `green` and replace its
`*So far:*` note with a `*Done:*` one, same as every prior unit.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** Not run on the M1 Max / AppleClang this session — see above. In a
Linux sandbox (Clang 18, GCC 13), all six green: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat` (all five unchanged
from WU-05, none of their files touched) and `test_jacobian`, new this
session, 411 checks. Verified Release and Debug, `SCATTER_TILE_LOG2` 4 and 5
(four configurations × two compilers, all green — eight total), all under
the project's exact warning set (`-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror`), same practice as prior sessions. Also ran the
full suite under GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug): clean, no ASan or UBSan report anywhere.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13. Not yet built with AppleClang this session.

## Where we are

WU-06 implemented and verified everywhere available to this session, but
**not closed** — see Tag, above. `src/core/lattice.hpp`/`lattice.cpp` add
`Lattice`: a 129×129 grid of `Vec3` control vertices (`x`, `y`, `z`,
`double`), `eval(u, v)` (bicubic Catmull-Rom expansion) and `jacobian(u, v)`
(analytic `dx/du`, `dx/dv`, `dy/du`, `dy/dv`, differentiating the identical
basis polynomials `eval()` uses, term by term — not a separate
approximation). `tests/test_jacobian.cpp` checks the analytic Jacobian
against central differences of `eval()` to 1e-6 relative, across the
lattice interior, at its four edges, at the four corners, and at interior
integer coordinates (cell knots) — the WU-06 accept criterion, plus two
sanity checks: `eval()` reproduces control vertices exactly at integer
coordinates, and both `eval()`/`jacobian()` clamp rather than misbehave on
out-of-range input. `CMakeLists.txt`: `src/core/lattice.cpp` added to
`scatter-core`'s sources, `test_jacobian` registered via `scatter_test()`.
No headers/tests outside this list touched; matches WU-06's file list in
`WORK-UNITS.md` exactly, unlike WU-05's declared deviation.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-022, appended in full to `DECISIONS.md`:** which
Catmull-Rom parametrisation (uniform — the lattice's own indexing is
uniform, so this is the variant that's actually well-behaved here), how
control-vertex lookups outside the lattice behave (edge replication, same
choice ADR-020 makes for chroma), and that `jacobian()` is 2×2 (`x`, `y`
only) even though `eval()` carries `z` too, per architecture.md 4.2's own
stated definition of `J`. None of this reopens architecture.md, which left
these open on purpose (reference document, not an ADR) — same relationship
ADR-020 already has to it for chroma's filter taps.

**A real finding, not just a design choice — worth reading before writing
more finite-difference tests against anything in `src/core/`:** a Catmull-
Rom spline is C1 at its knots (value and first derivative agree from either
side — provable directly from the basis polynomials, and `lattice.cpp`'s
header comment now says so) but generally **not** C2. A symmetric central
difference straddling a knot mixes two cells with different second
derivatives, which degrades the usual O(h²) central-difference truncation
error to O(h) — about four orders of magnitude worse at `h = 1e-4`, comfortably
enough to blow through a 1e-6 relative tolerance. This first showed up as
`test_jacobian.cpp` failing at exactly the points you'd least expect a
subtle bug to hide (integer lattice coordinates), which is what made it
worth writing down. Not a bug in `jacobian()` — confirmed by hand, both
cells' formulas reduce to the identical expression `(P[i+1] - P[i-1]) / 2`
at a shared knot — purely a property of finite-differencing a C1-only
function across the seam. `tests/test_jacobian.cpp`'s `numericDeriv()`
detects an interior-knot input and switches to forward differencing, which
stays inside the same cell `Lattice::eval()`'s own convention (`locate()`
in `lattice.cpp`) picks for that point, avoiding the seam rather than
working around a symptom of it.

A second issue, unrelated: the first draft of `test_jacobian.cpp`'s
control-vertex-reproduction check called `eval(row, col)` where it should
have called `eval(col, row)` — `Lattice::at(row, col)` documents `row` as
the *v* index and `col` as the *u* index, and `eval` takes `(u, v)`. Caught
by the same central-difference failure investigation above (both bugs
surfaced in the same first test run); fixed before anything was written to
the Mac or committed, so it never reached a state file and isn't a
`CORRECTIONS.md` entry — flagging it here only because it's exactly the
kind of transposition a future session extending this file should watch
for.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine (no separate laptop
session was open). Files were written directly, and `git add -A && git
commit` was run through that same bridge — same as prior sessions, this
still cannot clean up its own `index.lock`/`HEAD.lock`/temp-object files
afterward (unlink still fails on this mount), so `mv` into `_to_delete/`
was used again, both for a stale `index.lock` left over from end of session
5 (present at the start of this session, before anything here touched the
tree) and for what this session's own commit left behind. Safe to `rm -rf
_to_delete/` by hand at the real terminal, as before. Unlike prior
sessions, this one could not reach a real terminal at all — no build, no
`ctest`, no `close.sh`, no tag came from this machine; everything under
Tests/Build above ran in a separate Linux sandbox, and the M1 Max /
AppleClang leg that every previous session's Handoff reports is simply
missing this time. Do that first next session, or before, at the terminal.

## Next work unit

Do not start WU-07 until WU-06 is tagged `wu-06-green`. Once it is:

**WU-07 — K and EWA footprint from J**, per `WORK-UNITS.md`.
**Files:** `src/core/jacobian.hpp`, `tests/test_ewa.cpp`.
**Accept:** `K = 1/|det J|` correct for known affine cases; ellipse axes
match analytic values for pure scale, pure rotation and shear; clamping at
the configured maximum compression behaves. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing red. WU-06 is implemented and verified short of the M1/AppleClang
leg and the tag — see Tag, above; that is the one blocking step before
WU-07 can start.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-06/WU-07 and costs no session time.

## Append to DECISIONS.md

ADR-022 was appended in full during this session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session — see the transposition note above under "Where we
are": caught and fixed before it reached any state file or commit.
