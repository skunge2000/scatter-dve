# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 20
**Tag:** `wu-15a-green` is still the most recent *tag* — this session's own
work (WU-16a) is implemented, committed and verified in a Linux cloud
sandbox, but is **not tagged `wu-16a-green` yet**: one piece is
unverified (see below), and per this project's own established practice,
the assistant does not run `./tools/close.sh` — that, and the tag it
produces, is Steve's own action at the real terminal.
**Phase:** 3 (SDI output) remains done in full, unchanged since session
18. Phase 4 (Threading and NEON) is now under way: this session scoped
and built WU-16a (thread pool, QoS, per-worker bin arenas — PASS 2's own
tile-parallelism), the first unit of the phase.

**This session did real scoping before writing any code, per
`SESSION-PROTOCOL.md`'s own discipline and Steve's own brief.**
`WORK-UNITS.md`'s WU-16 line was bare going in — a title and one accept
criterion, no `Files:` — and `docs/architecture.md` section 6 describes a
fuller two-pass design (PASS 1 row-band parallelism with per-worker
generation-time bin arenas, *and* PASS 2 tile-parallelism) than fits
`SESSION-PROTOCOL.md`'s "3 source files" cap without reopening
`core/binner.hpp`/`.cpp` (WU-08, frozen). Split the same way this project
has split an over-scoped unit before (ADR-028's WU-12a/WU-12b, ADR-032's
WU-15a/WU-15b): **WU-16a (this session) is PASS 2's tile-parallelism
alone; WU-16b (not this session, not yet scoped with `Files:`/`Accept:`)
is PASS 1's row-band parallelism**, named in `WORK-UNITS.md` for whoever
picks it up next. See `DECISIONS.md` ADR-040 for the full design and
reasoning — it is long, because this is the first unit in the project
with genuine concurrency inside `scatter-core` and there was real design
surface to cover (thread pool shape, per-worker arena reuse, tile
partitioning, why the single-threaded oracle path stays completely
separate from the new machinery, the QoS platform guard).

**1. `src/core/pipeline.hpp`, new.** `ThreadPool` (persistent worker
threads, a generation-counter dispatch that doubles as a barrier across
two consecutive `runOnAll()` calls) and `setWorkerQoS()` (Apple-only
`pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`, a no-op
elsewhere). Arrives exactly where ADR-026 (WU-10) said it would.

**2. `src/core/pipeline.cpp`, extended.** `runFrame()` now branches on
`PipelineParams::threads`: `<= 1` runs the exact WU-10 loop, refactored to
share a new `resolveOneTile()` helper but otherwise untouched and never
touching `ThreadPool` at all (the oracle, ADR-015, stays independently
simple); `> 1` constructs a local `ThreadPool`, gives each worker its own
persistent `TileAccum` + `AccumCell` scratch buffer (the "per-worker bin
arena" this unit's own scope covers), and statically partitions
`TileBins`' own tiles across workers by `tileIndex % threads`.

**3. `src/core/resolve.hpp`, one field added.** `PipelineParams::threads`,
default `1` — additive, every existing caller unchanged.

**4. `tests/test_threading.cpp`, new.** Direct `ThreadPool` checks
(dispatch reaches every worker exactly once per round, clean teardown)
plus the literal accept criterion: a genuinely warped (cylinder over a
zone plate), multi-tile, non-tile-size-multiple frame run at
`threads` in `{0, -3, 1, 2, 3, 5, 8, 16}`, every one checked bit-for-bit
against the `threads == 1` reference; a second, differently-shaped
geometry checked at `threads == 1` vs `8` specifically.

**5. `CMakeLists.txt`.** `find_package(Threads REQUIRED)`,
`Threads::Threads` linked into `scatter-core` (first unit that needed
it), `test_threading` added.

**No corrections this session.** Nothing earlier was found wrong.

**Tests / Build:** all fifteen tests green (fourteen carried over
unchanged, plus `test_threading`) across Clang 18 and GCC 13, Release and
Debug, `SCATTER_TILE_LOG2` 4 and 5 — eight configurations, zero warnings
under this project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set — plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (clean) and GCC 13 with `-fsanitize=thread` (clean — no data race;
run both across the full suite and standalone against `test_threading`
under `TSAN_OPTIONS=halt_on_error=0` to surface every report rather than
stop at the first). All of this ran in a Linux cloud sandbox (a full
Ubuntu 24.04 environment, not the device bridge's own more limited Linux
VM), the same kind of verification WU-01 through WU-13 describe doing
before their own real-terminal `close.sh` run.

**What is NOT verified, and why this line stays `wip`:**
`setWorkerQoS()`'s `#ifdef __APPLE__` branch — the actual
`pthread_set_qos_class_self_np` call and its `<pthread/qos.h>` header —
has never been compiled against real Apple headers; no AppleClang/Xcode
toolchain exists anywhere this session had access to. Same gap
ADR-031/032 already named for WU-14/WU-15a's own first Apple-only
surfaces, same resolution: needs the real terminal.

## Where we are

Phase 3 (SDI output) remains done in full (WU-14/15a/15b, session 18).
Phase 4: WU-16a implemented, committed, verified except for one
Apple-only function (above) — `wip`, not `green`. WU-16b (PASS 1's own
row-band parallelism) is named in `WORK-UNITS.md` but not scoped or
built. `DECISIONS.md` now runs through ADR-040; `CORRECTIONS.md` is
unchanged since C-014 (session 18).

**Corrections this session:** none.

**Delivery mechanics:** ran remotely via the device-bridge tools, same as
every session since WU-14, for reading the repository's state and for
this handoff's own final commit — but the actual implementation and
verification work (writing `pipeline.hpp`/`.cpp`, `resolve.hpp`,
`test_threading.cpp`, and the full eight-configuration + ASan/UBSan +
TSan matrix above) was done in a separate Linux cloud sandbox, not the
device bridge's own Linux VM, because that sandbox has the compilers and
`nproc` headroom this unit's own verification needed. Final files were
written back to the real repository via the device bridge and committed
from there. One commit this session — see `git log` for its actual hash.
Working tree is clean as of this handoff.

## Next work unit

**Steve's own next action is at the real terminal, not a new work unit
yet:** pull this session's commit, `cmake -B build && cmake --build
build`, confirm `setWorkerQoS()`'s Apple branch actually compiles
(`<pthread/qos.h>`, `pthread_set_qos_class_self_np`'s exact signature —
flagged unverified above), then `./tools/close.sh 16a`. If it builds and
the full suite passes, tag `wu-16a-green` by hand the same way
`wu-15a-green` was (ADR-035's own precedent for a session-flagged
exception a script cannot itself judge) — though here there is no known
exception to accept, just an unverified-until-now compile to confirm; if
it is red, record the failure verbatim here per `SESSION-PROTOCOL.md` and
the next session starts there instead of at WU-16b.

Once WU-16a is confirmed green: **WU-16b** (PASS 1 row-band parallelism,
per-worker generation-time bin arenas — see `DECISIONS.md` ADR-040 and
`WORK-UNITS.md`'s own WU-16b heading) is next in Phase 4, followed by
WU-17 (NEON v210 unpack/pack) and WU-18/19 as already listed.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 remains moot per ADR-037. ADR-037's own
follow-ups #1 (`test_decklink_device.cpp`'s full-duplex check) and #2
(genlock) remain open, unrelated to this session's own work — Phase 5's
problem, not Phase 4's.

New, this session: whether `setWorkerQoS()`'s Apple branch compiles
as written (above) — the one thing standing between WU-16a and `green`.

## Blocked / red

Nothing red. WU-16a is `wip` pending the real-terminal confirmation
above, not blocked — every portable-C++ part of it is fully verified.

## Environment check

Unchanged from session 18/19 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target, still untouched by
any code. **UltraStudio 4K Mini** remains on hold pending a PSU
replacement. None of this is relevant to WU-16a/16b — pure `src/core/`
work, no DeckLink, no hardware.

## Append to DECISIONS.md

ADR-040 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-002, ADR-008, ADR-013, ADR-015,
ADR-017, ADR-024, ADR-026, ADR-029 or ADR-031 — see ADR-040's own closing
paragraph for the precise relationship to each.

## Append to CORRECTIONS.md

Nothing this session.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
git log -1                       # confirm this session's commit landed
cmake -B build && cmake --build build
```

Watch specifically for whether `src/core/pipeline.cpp`'s
`#if defined(__APPLE__)` block (the `setWorkerQoS()` implementation, near
the top of the file) compiles — `<pthread/qos.h>`,
`QOS_CLASS_USER_INTERACTIVE`, `pthread_set_qos_class_self_np`'s exact
signature are all unverified against real Apple headers (see above). If
it does not compile as written, the fix is local to that one `#ifdef`
block; nothing else in this unit depends on its exact shape.

If it builds:

```
ctest --test-dir build --output-on-failure
```

then `./tools/close.sh 16a` if that's green, and report the result back
(paste any failure verbatim per `SESSION-PROTOCOL.md` if it's red — the
next session starts there rather than at WU-16b).
