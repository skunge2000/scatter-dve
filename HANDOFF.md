# HANDOFF — Session 72

## Where we are

**Verified repo state at session start** (per this session's own standing
instruction to check, not trust, the incoming prompt): `HEAD` and
`origin/main` both at `60e2d285b954021ff3cf7d07ca27921be153ab41` (untagged,
`WU-35a2`). `git status --short` showed Session 71's own `WU-35a3` work
(`src/io/decklink_capture_consumer.{hpp,cpp}`, plus `HANDOFF.md` itself)
still uncommitted, exactly as Session 71's own `HANDOFF.md` said it would
be. `.git/index.lock` present, 0 bytes, no `git` process holding it
(`ps aux` checked) — stale, same as Session 71 left it. This matched the
prompt's own explicitly-flagged "real state may not match" scenario; no
correction needed to what follows, but flagging it here per instruction.

**WU-35a4 is done and sandbox-verified — status `green`.** This closes the
real-content gap `CORRECTIONS.md` C-020 and C-036 both describe:
`core/binner.hpp`'s `generateFragmentsTagByFacing()`/
`generateFragmentsRowRangeTagByFacing()` (`WU-28c`, built and `green` since
long before this session) were never actually called from
`core/pipeline.cpp`. They are now, in two of its three PASS-1 call sites,
gated by a new `PipelineParams::frontTag`/`backTag` pair — see
`DECISIONS.md` ADR-089 for the full design conversation, and
`WORK-UNITS.md`'s own `WU-35a4` entry (fully rewritten this session) for
the complete accept criteria and file list.

**New this session:**
- `DECISIONS.md` — ADR-089 appended (the design decision itself).
- `CORRECTIONS.md` — C-037 appended (the third-call-site correction to
  `WU-35a4`'s own prior, Session-71-written premise).
- `WORK-UNITS.md` — `WU-35a4`'s entry fully replaced (was a proposal, now a
  complete, verified close-out).
- `src/core/resolve.hpp` — `PipelineParams` gains `frontTag`/`backTag`
  (`std::uint8_t`, both default `0`).
- `src/core/pipeline.cpp` — `runThreaded()` and `runFrame()`'s
  `threads<=1` branch both gain the gated `generateFragmentsTagByFacing()`/
  `generateFragmentsRowRangeTagByFacing()` branch; `runFrameField()` gains
  an explanatory comment only (see "why not all three" below).
- `tests/test_kbuffer_resolve.cpp` — new Part E, six tests, exercising the
  real `runFrame()` path end to end on the real self-folding sphere, not
  hand-built `KSlot` arrays.
- `tests/test_decklink_live_sphere.cpp` — two-line opt-in
  (`params.frontTag = 1; params.backTag = 2;`) plus a header-comment
  correction, on top of Session 71's own still-uncommitted `WU-35a3` work
  in the same file. Blackmagic-SDK-linked; reasoned through and
  re-staged/diffed to confirm the write landed, but not buildable in this
  sandbox — same gap every DeckLink-touching unit in this project has
  named.

**Why only two of the three PASS-1 call sites changed:** checked directly
against the real code, not assumed. `runFrameField()`'s own
already-documented precondition (`ADR-077`) requires
`params.kBufferMode == Off` at every call through it, so the new gate would
always be false there by construction — and separately, no
`generateFragmentsFieldRowsTagByFacing()` sibling exists in
`core/binner.hpp`/`.cpp` for it to call even if it were reachable. `WU-28c`
never built one. Logged as `CORRECTIONS.md` C-037, since this entry's own
prior text (Session 71) had assumed "the same decision" would apply
"consistently" across all three call sites.

**Why the gate is `kBufferMode != Off && frontTag != backTag`, not
`kBufferMode != Off` alone:** the simpler gate was checked directly against
`tests/test_kbuffer_resolve.cpp`'s own already-`green` Part C before
deciding, not assumed — that test sets `kBufferMode == Blend` and `tag == 5`
together on real self-fold geometry. Gating on `kBufferMode` alone would
have silently stopped honouring that already-shipped caller's own `tag`
value the instant this unit landed. Full reasoning in `DECISIONS.md`
ADR-089.

## Build/test verification (this session, cloud sandbox)

Genuinely the first time in the whole `WU-35a`-numbered chain this has been
possible — `core/pipeline.cpp`, `core/binner.hpp`/`.cpp`, and
`core/resolve.hpp` link no Blackmagic SDK (`scatter-core` target). Fresh
clone of `origin/main` at `60e2d28` with this session's own files overlaid;
four configurations all green, zero warnings, zero sanitizer traps:

- GCC 13.3.0 Release, tile 2^5 — 28/28 `ctest` targets green.
- Clang 18.1.3 Release, tile 2^5 — 28/28 green.
- GCC 13.3.0 Debug, tile 2^4 — 28/28 green.
- GCC 13.3.0 Debug+ASan+UBSan, tile 2^5 — 28/28 green; `nm -D` confirmed
  genuine instrumentation linkage (28 `asan`/14 `ubsan` hits, matching
  `WU-35a1`'s own prior counts exactly).

`test_kbuffer_resolve` itself: 436190 checks passing (GCC Release t5),
including all six new Part E tests — each a real, non-vacuous assertion
(byte-identical-output checks for the not-opted-in path, genuine-difference
checks for the opted-in path, an I6 thread-count check specific to the new
branch). This is exactly the hole C-020 named as missing: "no test in
`tests/test_kbuffer_resolve.cpp` positioned to catch it."

Not yet built, run, tagged, or pushed at Steve's own real terminal — see
"What's next" below. `tests/test_decklink_live_sphere.cpp`'s own two-line
edit has no sandbox build path at all (Blackmagic SDK).

All writes to the seven changed files were made via the device bridge,
then re-staged from the Mac and diffed/grepped to confirm each landed
exactly as intended before this HANDOFF was written — including a final
brace/paren balance check on `tests/test_decklink_live_sphere.cpp`
(68/68 braces, 416/416 parens — balanced; the raw counts differ from
Session 71's own 68/68 braces, 410/410 parens because this session added
code, not because anything is unbalanced).

## What's next (Steve's own to run)

1. `rm -f .git/index.lock` (stale — no `git` process holds it, confirmed
   via `ps aux` this session).
2. Review the diff, especially `DECISIONS.md` ADR-089 and
   `WORK-UNITS.md`'s `WU-35a4` entry, alongside the code.
3. Commit. Session 71's own uncommitted `WU-35a3` work
   (`src/io/decklink_capture_consumer.{hpp,cpp}`) is still sitting there
   too — it needed zero changes for this unit (`PipelineParams` is copied
   wholesale into `CaptureConsumer`, so `frontTag`/`backTag` ride along
   automatically), but it's a separate, already-complete unit of work that
   this session did not touch and is not this session's to commit-message.
   Consider whether Steve wants it in the same commit or split.
4. Build and run the real test suite at a real terminal (this sandbox
   already did the portable subset; the DeckLink-linked targets need
   Steve's own Blackmagic-SDK-equipped machine).
5. `./tools/close.sh` for the normal path (auto-tags and auto-pushes on
   green), **or**, if `test_decklink_live_sphere` needs to stay excluded
   from the automated run the way prior DeckLink-touching sessions have
   done, the manual fallback — tag by hand, then push both explicitly,
   since the manual path does **not** auto-push:
   ```
   git tag -a wu-35a4-green -m "WU-35a4 green: pipeline.cpp TagByFacing wiring, ADR-089"
   git push origin main
   git push origin --tags
   ```
6. Real-hardware by-eye confirmation on `test_decklink_live_sphere`: the
   folding sphere's back half visibly occluded at `manualTransp == 0`,
   increasingly showing through as `T`/`t` sweeps toward `kWeightUnity`,
   on a real SDI monitor. This is the criterion the whole
   `WU-35a1`→`WU-35a4` chain has been building toward, and the one thing
   no cloud sandbox can ever check.

## What's broken / flagged, not fixed (out of this unit's own scope)

**`docs/wu-audit-2026-08.md` line 113 is now stale/inaccurate.** It reads:
"The real-content gap (single tag per call) was already found and logged
as C-020/ADR-062 before this sweep, and closed by WU-28c. Nothing new."
This is incorrect: `WU-28c` only ever *built* the TagByFacing functions —
`core/pipeline.cpp` never called them until this session's `WU-35a4`. The
gap was not closed until now. Found during this session's own
repo-wide grep sweep for anything this change might make inconsistent
elsewhere (per this unit's own standing instruction); left unfixed, per the
same instruction to name it rather than fix it as a side effect, since
touching an audit doc is outside `WU-35a4`'s own scope. `docs/architecture.md`
line 557's own similar-sounding reference to `WU-28a`–`WU-28d` was checked
too and is fine as written — it doesn't claim the gap is closed.

## Untouched, deliberately

`INVARIANTS.md` (I8, I9, I11 all read, all still hold, none touched, per
explicit instruction). `core/binner.hpp`/`.cpp` (the functions this unit
calls already existed and were already `green`). `src/io/decklink_capture_consumer.{hpp,cpp}`
(zero changes needed — confirmed, not assumed, by reading both files in
full). `ADR-059/062/065/085/086/087/088` (not reopened; `ADR-089` is new).
Field-mode k-buffer support (`runFrameField()`'s own precondition, plus a
new `core/binner.hpp`/`.cpp` entry point) — named above as a real future
unit's job, not started here.
