# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 7
**Tag:** `wu-07-green` — confirmed. `./tools/close.sh 07` ran clean on the
M1 Max with AppleClang (Release, tile 2^5, the config `close.sh` builds) and
tagged it.
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All seven green on the M1 Max: `test_smoke`, `test_v210`,
`test_chroma`, `test_ramp_roundtrip`, `test_testpat`, `test_jacobian` (six
unchanged from WU-06, none of their files touched) and `test_ewa`, new this
session (69 checks).

Before that, this session verified in a Linux cloud sandbox (no AppleClang
there), on Clang 18 and GCC 13, under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all green,
zero warnings — checked explicitly in the build logs, not just a successful
exit code), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug): clean, no ASan or UBSan report anywhere
— same practice as prior sessions.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on
AppleClang (M1 Max), Clang 18 and GCC 13.

## Where we are

WU-07 done and closed green. `src/core/jacobian.hpp` adds two pure
functions operating on the `Jacobian` struct `src/core/lattice.hpp` already
defines (unchanged this session): `densityCompensation(j, maxK)` —
`K = 1/|det J|`, clamped to a caller-supplied `maxK` — and `ewaFootprint(j)`
— the elliptical filter footprint architecture.md 4.2 describes, returned
as `EwaFootprint{majorAxis, minorAxis, majorAngle}`, from the closed-form
eigen-decomposition of the symmetric 2x2 matrix `J*J^T`. Header-only, no
`.cpp`: both functions are small, `noexcept`, pure functions of their
argument — the same doubles-throughout shape as `Lattice::eval()`/
`jacobian()` — and `WORK-UNITS.md`'s WU-07 file list itself only names the
header. `CMakeLists.txt`: `test_ewa` registered via `scatter_test()`; no new
file added to `scatter-core`'s sources, since `jacobian.hpp` is header-only.
`tests/test_ewa.cpp` checks `K` and the footprint's axes against pure
scale, pure rotation and shear cases with known closed-form values — a
second, independently-derived closed form written into the test file
itself (trace/determinant of `J^T J`, not the `J*J^T` decomposition
`jacobian.hpp` uses — see the test's own comment), the same relationship
`test_jacobian.cpp`'s `numericDeriv()` has to `lattice.cpp`'s analytic
differentiation — plus `K`'s clamp at a configured `maxK`, including the
degenerate `det J == 0` fold and a negative-determinant fold. 69 checks.
Matches WU-07's file list in `WORK-UNITS.md` exactly.

**Design choices this session had to make that `docs/architecture.md`
left open — now ADR-023 in `DECISIONS.md`:** the compression clamp
(`maxK`) is a caller-supplied parameter rather than a project-wide
constant, since architecture.md fixes neither its numeric value nor
whether it's compile-time or runtime, and nothing in WU-07 has grounds to
invent an operating point that properly belongs to a later work unit's
accumulation-stage configuration (WU-09 onward); and the concrete
representation of "the elliptical filter kernel" as `{majorAxis,
minorAxis, majorAngle}`, obtained from `J*J^T`'s eigen-decomposition
rather than `J^T*J`'s — the two have identical eigenvalues, but only
`J*J^T`'s eigenvectors describe directions in destination `(x, y)` space,
which is what a filter footprint needs, not source `(u, v)` space. Neither
reopens architecture.md, which left both open on purpose — same
relationship ADR-020 and ADR-022 have to it.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as session 6. All
implementation and the full verification matrix above ran first in a
disposable Linux cloud sandbox, never on this machine directly — nothing
was written here until it was already green there. Files were then written
to this machine via the bridge, and `git add -A && git commit` ran through
that same bridge; as in prior sessions it still cannot clean up its own
`index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on this
mount), so stale ones were moved into `_to_delete/` rather than removed —
safe to `rm -rf _to_delete/` by hand. Git identity was already set locally
on this mount from a prior session (`Stephen Neal
<stephenneal@Stephens-MacBook-Pro.local>`, confirmed against `git log`
before committing), so nothing needed reconfiguring. `./tools/close.sh 07`
was, as before, run by hand at the real terminal, in two steps this
session: an initial commit left WU-07 `wip` with everything above already
verified in the sandbox, then this closing update (`WORK-UNITS.md` to
`green`, this file) followed your pasted `close.sh` output confirming the
M1 Max/AppleClang build and tag.

## Next work unit

**WU-08 — Fragment generation and tile binning**, per `WORK-UNITS.md`.
**Files:** `src/core/binner.hpp`, `src/core/binner.cpp`, `tests/test_binner.cpp`.
**Accept:** fragment count equals source samples under compression; boundary
straddling replicates into exactly the right neighbours; no fragment lost
or duplicated within a tile. Unstarted.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Nothing. WU-07 closed green.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-08 and costs no session time.

## Append to DECISIONS.md

Nothing this update — ADR-023 was appended in full earlier this session; see
`DECISIONS.md`. Not reopened or amended now that the tag is confirmed.

## Append to CORRECTIONS.md

Nothing this session.
