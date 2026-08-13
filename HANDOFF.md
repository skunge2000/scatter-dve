# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 7
**Tag:** none yet. WU-07 is `wip`, not `wu-07-green` — this session could
not run `./tools/close.sh`, which needs AppleClang on the M1 Max,
unreachable from here. **Run `./tools/close.sh 07` at the Mac terminal to
finish this work unit and tag it.**
**Phase:** 1 — portable core, file to file, 576p25, single-threaded

**Tests:** All seven green in the Linux cloud sandbox this session ran in:
`test_smoke`, `test_v210`, `test_chroma`, `test_ramp_roundtrip`,
`test_jacobian`, `test_testpat` (six unchanged from WU-06, none of their
files touched) and `test_ewa`, new this session (69 checks).

Verified there on Clang 18 and GCC 13, under the project's exact warning set
(`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`), Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all green,
zero warnings — checked explicitly in the build logs, not just a successful
exit code), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` (Debug): clean, no ASan or UBSan report anywhere.

**Not yet done, and the reason this session is wip, not green:** none of the
above ran on the M1 Max with AppleClang, and `./tools/close.sh` was not run
— this session had no access to a real terminal on that machine, only the
device-bridge file tools. **Next step for you: run `./tools/close.sh 07` at
the Mac terminal.** If it comes back green, tag as usual (`wu-07-green`)
and WU-08 is current. If AppleClang finds something Clang 18/GCC 13 didn't
(hasn't happened in six prior sessions, but WU-07 introduces `<algorithm>`'s
`std::min` and floating trig it hasn't used before), treat `close.sh`'s
result as the one that matters and report back here rather than trusting
this session's sandbox run over it.

**Build:** clean under `-Werror -Wconversion -Wsign-conversion` on Clang 18
and GCC 13. Not yet checked on AppleClang.

## Where we are

WU-07 implemented, not yet closed. `src/core/jacobian.hpp` adds two pure
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
file added to `scatter-core`'s sources, since `jacobian.hpp` is header-only
and `scatter-core`'s existing include path already covers it.
`tests/test_ewa.cpp` checks `K` and the footprint's axes against pure
scale, pure rotation and shear cases with known closed-form values — a
second, independently-derived closed form written into the test file
itself (trace/determinant of `J^T J`, not the `J*J^T` decomposition
`jacobian.hpp` uses — see the test's own comment), the same relationship
`test_jacobian.cpp`'s `numericDeriv()` has to `lattice.cpp`'s analytic
differentiation — plus `K`'s clamp at a configured `maxK`, including the
degenerate `det J == 0` fold and a negative-determinant fold. 69 checks.
Matches WU-07's file list in `WORK-UNITS.md` exactly.

**Design choices this session had to make that `docs/architecture.md` left
open — now ADR-023 in `DECISIONS.md`:** the compression clamp (`maxK`) is a
caller-supplied parameter rather than a project-wide constant, since
architecture.md fixes neither its numeric value nor whether it's
compile-time or runtime, and nothing in WU-07 has grounds to invent an
operating point that properly belongs to a later work unit's
accumulation-stage configuration (WU-09 onward); and the concrete
representation of "the elliptical filter kernel" as `{majorAxis, minorAxis,
majorAngle}`, obtained from `J*J^T`'s eigen-decomposition rather than
`J^T*J`'s — the two have identical eigenvalues, but only `J*J^T`'s
eigenvectors describe directions in destination `(x, y)` space, which is
what a filter footprint needs, not source `(u, v)` space. Neither reopens
architecture.md, which left both open on purpose — same relationship
ADR-020 and ADR-022 have to it.

**Delivery mechanics, not a design matter:** this session ran remotely, via
the device-bridge tools connecting to this machine, same as session 6. All
implementation and the full verification matrix above ran first in a
disposable Linux cloud sandbox, never on this machine directly — nothing
was written here until it was already green there. Files were then written
to this machine via the bridge, and `git add -A && git commit` ran through
that same bridge; as in prior sessions it still cannot clean up its own
`index.lock`/`HEAD.lock`/temp-object files afterward (unlink fails on this
mount), so stale ones — including one left over before this session touched
anything — were moved into `_to_delete/` rather than removed. Safe to
`rm -rf _to_delete/` by hand. Git identity was already set locally on this
mount from a prior session (`Stephen Neal <stephenneal@Stephens-MacBook-Pro.local>`,
confirmed against `git log` before committing), so nothing needed
reconfiguring this time.

## Next work unit

Still **WU-07** until `./tools/close.sh 07` is run at the Mac terminal and
comes back green. Only then does **WU-08 — Fragment generation and tile
binning** become current, per `WORK-UNITS.md`.
**Files:** `src/core/binner.hpp`, `src/core/binner.cpp`, `tests/test_binner.cpp`.
**Accept:** fragment count equals source samples under compression; boundary
straddling replicates into exactly the right neighbours; no fragment lost
or duplicated within a tile.

## Open questions

Unchanged: Q1 (tile size, WU-09), Q2 (4K Mini program outputs, WU-14), Q3
(macOS/Desktop Video version, WU-14).

## Blocked / red

Not red, but not closed: WU-07 needs `./tools/close.sh 07` run at your
terminal before it can tag `wu-07-green`. Nothing else outstanding.

## Environment check still outstanding

Unchanged from session 2 — Desktop Video / UltraStudio 4K Mini smoke test,
independent of WU-07/WU-08 and costs no session time.

## Append to DECISIONS.md

ADR-023 was appended in full this session; see `DECISIONS.md`.

## Append to CORRECTIONS.md

Nothing this session.
