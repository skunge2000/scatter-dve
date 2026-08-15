# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 20
**Tag:** `wu-15a-green` is still the most recent *tag* as of this file being
written — WU-16a is fully confirmed green at the real terminal (below) but
not yet tagged: `./tools/close.sh 16a` correctly refused (its own gate is
"any failure blocks tagging," ADR-035), because `test_decklink_device`'s
already-known full-duplex exception was present in the run. Steve's own
next step is to tag `wu-16a-green` by hand, the same way `wu-15a-green`
was — see "Next work unit," below, for the exact command.
**Phase:** 3 (SDI output) remains done in full, unchanged since session
18. Phase 4 (Threading and NEON): WU-16a (thread pool, QoS, per-worker
bin arenas — PASS 2's own tile-parallelism) is implemented, committed,
and now confirmed at the real terminal — the one piece this session's own
Linux cloud sandbox could not verify (`setWorkerQoS()`'s Apple-only QoS
branch) compiles clean on AppleClang.

**This session did real scoping before writing any code, per
`SESSION-PROTOCOL.md`'s own discipline and Steve's own brief**, then
implemented, verified in a Linux cloud sandbox, and committed WU-16a; a
second turn, after Steve ran the build at his own real terminal, closes
out what that sandbox could not check. See `DECISIONS.md` ADR-040 for the
full design.

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
picks it up next.

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

**Tests / Build — Linux cloud sandbox (this session's own first turn):**
all fifteen tests green (fourteen carried over unchanged, plus
`test_threading`) across Clang 18 and GCC 13, Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5 — eight configurations, zero warnings under
this project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set — plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (clean) and GCC 13 with `-fsanitize=thread` (clean — no data race;
run both across the full suite and standalone against `test_threading`).

**Tests / Build — real terminal, M1 Max, AppleClang (Steve's own second
turn, this session):** `cmake -B build && cmake --build build` succeeded
clean — `setWorkerQoS()`'s `#ifdef __APPLE__` branch
(`pthread_set_qos_class_self_np`, `<pthread/qos.h>`) compiles as written,
resolving this session's own last open question. Full suite: 16 of 17
passing. `test_threading` itself: green, all checks, 0.10-0.25s.
The one failure, `test_decklink_device.cpp:53`,
`test_at_least_one_device_is_full_duplex` (`foundDuplexDevice` staying
false) — this is **ADR-035's own already-named, already-accepted
exception**, unrelated to WU-16a: the UltraStudio Monitor 3G is
playback-only by design, so that check correctly reports no duplex
device found with it as the only attached device, exactly the same
"15/16" (now 16/17, one more test in the suite) pattern WU-15a's own
close.sh run hit. WU-16a touches no `src/io/`, `decklink_*` or
`test_decklink_*` file at all, so this is not a regression this unit
introduced — `./tools/close.sh 16a` correctly refused to tag (its own
gate cannot distinguish an accepted exception from a real failure, by
design), the same way it correctly refused for WU-15a.

## Where we are

Phase 3 (SDI output) remains done in full (WU-14/15a/15b, session 18).
Phase 4: WU-16a fully implemented and verified, both in the Linux cloud
sandbox and now at the real terminal — only blocked from an automatic
`green` tag by the same known, accepted `test_decklink_device` exception
ADR-035 already covers. WU-16b (PASS 1's own row-band parallelism) is
named in `WORK-UNITS.md` but not scoped or built. `DECISIONS.md` now runs
through ADR-040; `CORRECTIONS.md` is unchanged since C-014 (session 18).

**Corrections this session:** none.

**Delivery mechanics:** implementation and verification (writing
`pipeline.hpp`/`.cpp`, `resolve.hpp`, `test_threading.cpp`, and the full
eight-configuration + ASan/UBSan + TSan matrix) were done in a separate
Linux cloud sandbox, not the device bridge's own Linux VM; final files
were written to the real repository and committed via the device bridge.
One commit this session (`b052625`). A stale `.git/index.lock` (left over
from a previous session, no process holding it) blocked that commit
until Steve removed it by hand — device-bridge tools cannot delete files
on the connected machine, so that one step needed his own terminal.
Working tree is clean as of this handoff.

## Next work unit

**Steve's own next action: tag `wu-16a-green` by hand**, accepting the
ADR-035 exception himself exactly the way he did for `wu-15a-green` —
`close.sh` will not do this automatically, by design:

```
cd ~/src/scatter-dve
git tag -a wu-16a-green -m "WU-16a: build green, tests pass (test_decklink_device's full-duplex check is ADR-035's known exception, unrelated to WU-16a)"
git push origin HEAD --tags   # if you keep a remote; close.sh would have done this
```

After that: **WU-16b** (PASS 1 row-band parallelism, per-worker
generation-time bin arenas — see `DECISIONS.md` ADR-040 and
`WORK-UNITS.md`'s own WU-16b heading) is next in Phase 4, followed by
WU-17 (NEON v210 unpack/pack) and WU-18/19 as already listed.

## Open questions

Unchanged: Q1 (tile size), Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)). Q2 remains moot per ADR-037. ADR-037's own
follow-ups #1 (`test_decklink_device.cpp`'s full-duplex check — now hit
again by this session, still the same accepted exception, still not
resolved structurally) and #2 (genlock) remain open, unrelated to this
session's own work — Phase 5's problem, not Phase 4's.

Nothing new this session — `setWorkerQoS()`'s own open question (did the
Apple branch compile?) is resolved: yes.

## Blocked / red

Nothing red. WU-16a is fully green at both the Linux sandbox and the real
terminal; only the tag itself is outstanding, and that is Steve's own
manual step per "Next work unit," above.

## Environment check

Unchanged from session 18/19 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target — and, per this session's own real-
terminal run, still correctly reports as non-duplex (ADR-035, expected).
**UltraStudio Recorder 3G** is in hand, named (ADR-039) as Phase 5's own
input target, still untouched by any code. **UltraStudio 4K Mini**
remains on hold pending a PSU replacement. None of this is relevant to
WU-16a/16b's own work — pure `src/core/`, no DeckLink code touched.

## Append to DECISIONS.md

ADR-040 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-002, ADR-008, ADR-013, ADR-015,
ADR-017, ADR-024, ADR-026, ADR-029 or ADR-031 — see ADR-040's own closing
paragraph for the precise relationship to each. Not reopened by this
session's second turn either: the real-terminal run only confirms what
ADR-040 already flagged as unverified: it does not change the design.

## Append to CORRECTIONS.md

Nothing this session.

---

## What to run at your terminal

Already done, this session:

```
cd ~/src/scatter-dve
cmake -B build && cmake --build build     # clean — Apple QoS branch confirmed
ctest --test-dir build --output-on-failure   # 16/17, the known ADR-035 exception
./tools/close.sh 16a                      # correctly refused to tag — by design
```

Still to do — tag it by hand (see "Next work unit," above):

```
git tag -a wu-16a-green -m "WU-16a: build green, tests pass (test_decklink_device's full-duplex check is ADR-035's known exception, unrelated to WU-16a)"
git push origin HEAD --tags
```
