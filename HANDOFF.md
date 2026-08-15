# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 24
**Tag:** `wu-18-green` stands, confirmed — Steve's own `git show wu-18-green
--no-patch --format="%H %s"` vs `git log -1 --format="%H %s"` check (the one
loose end session 23's own handoff left) came back matching before this
session started work, per Steve's own brief at session open. No tag from
this session yet — WU-19a stays `wip` until Steve's own real-terminal build/
test/`close.sh` run, the same procedural reason every other unit's line has
had.
**Phase:** 4 (Threading and NEON) was already done in full going into this
session (WU-16a/16b, WU-17, WU-18 all green). This session opened WU-19, the
phase's last unit, and — after real scoping, per Steve's own brief — split
it into WU-19a (this session: a persistent, caller-owned `ThreadPool`) and
WU-19b (not this session: real-time measurement at 576i25 on the M1 Max,
Steve's own job). `DECISIONS.md` now runs through ADR-044; `CORRECTIONS.md`
is unchanged this session, still through C-015 (nothing was claimed and
found wrong this session).

**This session's own first job, per Steve's own brief and
`SESSION-PROTOCOL.md`'s own discipline: real scoping before any code.**
`WORK-UNITS.md`'s own WU-19 line was barer going in than any prior unit's
own line has been — not even WU-16's or WU-17's one-line accept criterion,
just a title ("Real time at 576i25"). Read `docs/architecture.md` section
10's own Phase 4 "done when" line ("8-thread output is bit-identical to
single-threaded, at frame rate"), section 6 (the threading model) and
section 11's own budget/splat discussion, plus `src/core/pipeline.cpp`/
`.hpp` (ADR-040/041's own current threading implementation) and
`src/video/v210.cpp`/`chroma.cpp` (ADR-042/043's own two NEON paths) — all
in full, before deciding what this unit actually covers, exactly as the
brief asked.

**The scoping question this session actually had to answer, unlike every
correctness-oriented unit before it: this unit's own accept criterion is
partly a real-hardware timing claim, and this project's own Linux cloud
sandbox CPU has no relationship to the M1 Max's actual throughput.** Every
prior Phase-4 unit's own accept criterion was pure correctness (bit-
identical output; bit-identical to scalar reference) — fully checkable in
this sandbox. "At frame rate" is not: it is a wall-clock claim against
576i25's own 40ms/frame budget, and nothing this sandbox measures says
anything true or false about whether the same code meets that budget on 8
M1 Max performance cores. See `DECISIONS.md` ADR-044's own opening section
for the full reasoning — this is a different *kind* of gap than ADR-031/
032's own "cannot compile here at all" (this sandbox compiles and runs this
unit's own code just fine; the number it would produce just would not mean
what it needs to mean).

**What this session actually built: the one concrete piece of
implementation work every prior Phase-4 ADR already named by number as
WU-19's own job.** ADR-040's own "Per-frame `ThreadPool` construction, not a
caller-owned persistent pool — deliberately left for WU-19": `runFrame()`
was spawning and joining `params.threads` OS threads on every single call.
This session built `PipelineParams::pool` (`core/resolve.hpp`) — an
optional, caller-owned, already-constructed `ThreadPool*` — so a caller can
build one once and reuse it across many `runFrame()` calls instead. This is
a correctness-preserving refactor, not a throughput claim: I6 still
guarantees bit-identical output to the `threads <= 1` oracle regardless of
whether `runFrame()` constructs its own pool or is handed one, and that is
exactly what this session's own new test checks, directly, not just reasons
through.

**Split, decided before writing any code, the same discipline ADR-028/032/
040 already used for WU-12a/b, WU-15a/b, WU-16a/b — though for a different
underlying reason this time (not the 3-file cap; WU-19a alone is nowhere
near it) — see `DECISIONS.md` ADR-044 for the full record:**

- **WU-19a (this session).** The persistent-pool mechanism, verified for
  correctness in full in this sandbox.
- **WU-19b (not this session — Steve's own real-time measurement at his own
  terminal).** Time `runFrame()`/`runFrameFile()` at 576i25 with a real,
  persistent pool on the real M1 Max, confirm the 40ms/frame budget — the
  same "hands-on verification, not a session's own job to assert from a
  terminal" category WU-15b was for its own hour-long endurance run. No new
  committed tooling for this — a `std::chrono` wrap at the terminal is
  enough; a benchmarking tool was considered this session and deliberately
  **not** built (its own numbers, from this sandbox's CPU, would mean
  nothing for the real question — see ADR-044).

**1. `src/core/resolve.hpp`, extended (not new).** One new field on
`PipelineParams`: `ThreadPool* pool = nullptr` (`ThreadPool` forward-
declared, not `#include`d — a pointer needs no complete type, and this
keeps the header's own dependency graph unchanged). Default `nullptr`:
every existing caller (WU-10 through WU-18) compiles and behaves exactly as
before. When non-null, `pool->size()` alone decides the worker count used;
`PipelineParams::threads` is not consulted at all in that branch — a
caller does not need to keep the two fields in sync, and there is no way
for a mismatch to silently corrupt output (I6; see ADR-044's own
"Precedence" section and this session's own dedicated regression test,
below).

**2. `src/core/pipeline.cpp`, extended.** The threaded PASS-1/PASS-2 body
WU-16b already wrote — per-worker generation-time bin arenas, the row-band
`runOnAll()` call, the barrier, the tile-parallel `runOnAll()` call — is
factored out unchanged into a new `runThreaded(..., ThreadPool&, ...)`
helper. `runFrame()` becomes a three-way branch: `pool == nullptr &&
threads <= 1` takes the untouched single-threaded oracle loop (WU-10's own
body, still never touched by anything since); `pool != nullptr` calls
`runThreaded()` against the caller's own pool; otherwise constructs a local
`ThreadPool` for exactly this one call and calls `runThreaded()` against
it — exactly reproducing WU-16a/16b's own prior behaviour, verified (not
just reasoned) by the full pre-existing test suite passing completely
unmoved against this refactor.

**3. `tests/test_persistent_pool.cpp`, new.** Three checks: (a) a
`ThreadPool` constructed once and reused across ten `runFrame()` calls
against the same warped-frame geometry matches the single-threaded oracle
on every call, and the pool tears down cleanly afterward; (b) a pool of
size 3 against a `PipelineParams::threads` field deliberately set to 99,
and separately to 1, both still produce output bit-identical to the
oracle — the direct regression check for the "pool size governs, not
`threads`" precedence rule, not just a comment claiming it; (c) one pool
reused across two different frame geometries in immediate sequence (and
back to the first again), proving no per-frame state leaks across the
shared pool. 2 562 778 checks.

**4. `CMakeLists.txt`.** `test_persistent_pool` added via the same
`scatter_test()` pattern as `test_threading`/`test_row_band`; top-of-file
history comment extended with a short WU-19a paragraph, same convention
every prior unit's own addition used there.

**Corrections this session:** none. `CORRECTIONS.md` unchanged, still
through C-015.

**Tests / Build — this session's own Linux cloud sandbox (Ubuntu 24.04,
Clang 18.1.3, GCC 13.3.0, cmake 3.28.3, ninja):**

Default matrix: Clang 18 and GCC 13, Release and Debug, `SCATTER_TILE_LOG2`
4 and 5 — eight configurations, all **seventeen** tests green (the sixteen
carried over unchanged plus `test_persistent_pool`), zero warnings under
this project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set. GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean. GCC 13 with
`-fsanitize=thread` at both tile sizes: clean, no data race, checked both
across the full suite and standalone against
`test_threading`/`test_row_band`/`test_persistent_pool` under
`TSAN_OPTIONS=halt_on_error=0` — checked with particular care since this is
the first unit to let a `ThreadPool` genuinely outlive a single `runFrame()`
call and be driven by more than one call in sequence.

This unit touches **no Apple-only surface at all** — unlike WU-14/WU-15a/
WU-17/WU-18, there is nothing here this sandbox could not already fully
verify. What remains for the real terminal is the same procedural
confirmation every unit needs (`SESSION-PROTOCOL.md`: "the assistant does
not run `close.sh`"), not a genuinely unverified code path.

**Tests / Build — real terminal, M1 Max, AppleClang:** not yet run this
session — see "What to run at your terminal," below.

## Where we are

Phase 3 (SDI output) and Phase 4's WU-16a/16b/WU-17/WU-18 remain done,
unchanged. WU-19a is implemented and fully verified in this session's own
sandbox, pending only Steve's own real-terminal confirmation (procedural,
not a genuine unverified-Apple-surface gap this time). WU-19b — the actual
"at frame rate" measurement — is named and scoped (`WORK-UNITS.md`) but not
started; it is Steve's own hands-on job, not a future session's coding
task, the same category WU-15b was. `DECISIONS.md` now runs through
ADR-044; `CORRECTIONS.md` unchanged, through C-015.

**Delivery mechanics:** implementation and verification were done in this
session's own Linux cloud sandbox; final files were written to the real
repository via the device bridge (`resolve.hpp`, `pipeline.cpp`,
`test_persistent_pool.cpp`, `CMakeLists.txt`, `DECISIONS.md`,
`WORK-UNITS.md`, this `HANDOFF.md`). Steve now commits, builds, tests and
(if green) tags at his own terminal — per the standing operational note,
device-bridge commits on this machine reliably leave stale
`.git/index.lock`/`HEAD.lock` files behind (the bridge can write files but
cannot unlink anything here), so the exact git commands are given below for
Steve to run himself rather than proxied through the bridge.

## Next work unit

**Steve's own immediate next action:**

```
cd ~/src/scatter-dve
git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_persistent_pool.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-19a: persistent, caller-owned ThreadPool (ADR-044), verified in the cloud sandbox"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure   # expect 20/21 -- ADR-035's known exception (test_decklink_device duplex check)
./tools/close.sh 19a                          # will likely refuse to tag automatically, same as every unit since WU-15a
git tag -a wu-19a-green -m "WU-19a: persistent ThreadPool, verified"   # only if the above is the one known exception, nothing else
```

After that: **WU-19b** — time `runFrame()`/`runFrameFile()` at 576i25 with a
real, persistent `ThreadPool` (this session's own `PipelineParams::pool`) on
the real M1 Max, and confirm per-frame wall-clock time stays under 40ms
(25fps) — architecture.md 10's own "at frame rate." No code to write for
this by default; a simple `std::chrono::steady_clock` wrap around a
`runFrame()`/`runFrameFile()` call at your own terminal is enough to get a
real number (see `DECISIONS.md` ADR-044 for why a committed benchmarking
tool was considered and deliberately not built this session). If that
measurement shows the splat or either NEON path (WU-17/18) is actually the
bottleneck, their own already-named deferred refinements — v210's denser
`vld4q_u32` scheme, chroma's `downsampleRowNeon` load-count reduction — or
something not yet anticipated, become a future unit's own job, with real
evidence behind them for the first time. Once WU-19b reports a number,
Phase 4 (Threading and NEON) is done in full and Phase 5 (Live capture,
WU-20 onward) is next — WU-20 already has its own hardware target named
(UltraStudio Recorder 3G, ADR-039) but no `Files:`/`Accept:` scoping yet.

## Open questions

Unchanged: Q1 (tile size — still open; WU-19b's own real timing numbers,
once they exist, are the first evidence this project will have had that
could actually settle it, though settling it is not itself WU-19b's own
job), Q3 (macOS/Desktop Video version), Q4 (lattice edge damping, C-008(a)).
Q2 remains moot per ADR-037. ADR-037's own follow-ups #1
(`test_decklink_device.cpp`'s full-duplex check) and #2 (genlock) remain
open, unrelated to Phase 4/WU-19.

New this session, named in `DECISIONS.md` ADR-044 and `WORK-UNITS.md`'s own
WU-19b line, not decided or scoped: whether WU-17's own denser `vld4q_u32`
v210 scheme or WU-18's own `downsampleRowNeon` load-count reduction are
worth building at all — entirely dependent on WU-19b's own real
measurement (or later profiling), which has not happened yet.

## Blocked / red

Nothing red. WU-19a is green in this session's own sandbox across the full
matrix (including TSAN, both tile sizes). Only the real-terminal build/
test/`close.sh` confirmation is outstanding, per "Next work unit," above —
the same procedural step every unit needs, not a genuine open question this
time (no Apple-only surface in this unit at all).

## Environment check

Unchanged from sessions 18–23 (ADR-037/039): **UltraStudio Monitor 3G** is
the active, confirmed output target. **UltraStudio Recorder 3G** is in
hand, named (ADR-039) as Phase 5's own input target, still untouched by any
code. **UltraStudio 4K Mini** remains on hold pending a PSU replacement.
None of this is relevant to WU-19a's own work — pure `src/core/`, no
DeckLink code touched, no hardware needed to verify it. WU-19b, when Steve
runs it, needs no DeckLink hardware either — it is a file-to-file/in-memory
timing measurement of `runFrame()`/`runFrameFile()`, not a live-capture or
SDI-output test.

## Append to DECISIONS.md

ADR-044 was appended in full this session; see `DECISIONS.md`. Does not
reopen `docs/architecture.md`, ADR-015, ADR-021, ADR-024, ADR-026, ADR-030,
ADR-031, ADR-032, ADR-038, ADR-040, ADR-041, ADR-042 or ADR-043 — see
ADR-044's own closing paragraph for the precise relationship to each.

## Append to CORRECTIONS.md

Nothing appended this session — see "Corrections this session," above.

---

## What to run at your terminal

```
cd ~/src/scatter-dve
git add src/core/resolve.hpp src/core/pipeline.cpp tests/test_persistent_pool.cpp \
        CMakeLists.txt DECISIONS.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-19a: persistent, caller-owned ThreadPool (ADR-044), verified in the cloud sandbox"
cmake -B build && cmake --build build
ctest --test-dir build --output-on-failure
./tools/close.sh 19a
```

If the full suite is 20/21 with the one failure being exactly
`test_decklink_device`'s own full-duplex check (ADR-035's already-accepted
exception, unrelated to this unit), tag it by hand the same way you have
for every unit since WU-15a:

```
git tag -a wu-19a-green -m "WU-19a: persistent, caller-owned ThreadPool, verified"
git push origin HEAD --tags   # if you keep a remote
```

If anything *else* fails, stop and paste the output back rather than
tagging — that would be a real regression this session's own sandbox
somehow missed, not the known exception.

When you're ready for WU-19b, it needs no new code from a session by
default — see "Next work unit," above, for the `std::chrono` measurement
itself. Report back whatever per-frame time you get (and at how many
threads/what tile size) and the next session can pick up from there.
