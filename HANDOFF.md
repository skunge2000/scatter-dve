# HANDOFF — Session 73

## Where we are

**Verified repo state at session start, not trusted from the incoming
prompt — and it disagreed with what the prompt claimed, in exactly the way
`SESSION-PROTOCOL.md`'s own standing instruction exists to catch.** The
prompt asserted `HEAD == origin/main`, one commit ahead of `60e2d28`,
tagged `wu-35a4-green`, working tree clean. Checked directly: `HEAD` and
`origin/main` were **both** `60e2d28` itself (`WU-35a2`'s own commit, not
one ahead of it); `wu-35a4-green` was a real, pushed, annotated tag, but it
resolved to that same `60e2d28`, not to a commit containing `WU-35a4`'s own
work; `git status --short` listed all ten of `WU-35a4`'s own changed files
as ordinary uncommitted working-tree modifications. This is exactly the
state `CORRECTIONS.md` C-038 already describes finding — but C-038's own
"Resolution" paragraph claims that state was fixed "this same
conversation." It had not been: no trace of the described fix anywhere
(`git reflog`, `git stash list` both checked, both empty of it). Logged as
**C-039**: a correction's own past-tense "fixed" is a claim, not evidence,
and needs the same direct verification any other claim in this project
gets before being trusted.

**Fixed for real, this session, before anything else:** the misplaced
`wu-35a4-green` (`9cb7f88`, pointing at `60e2d28`) deleted locally; the
real ten-file `WU-35a2`/`WU-35a3`/`WU-35a4` diff committed for real
(`00aa6a6`); `wu-35a4-green` recreated pointing at `00aa6a6`. Could not be
pushed from this session's own device-bridge shell — `git push` fails
immediately with "could not read Username for 'https://github.com'", no
stored credentials there unlike Steve's own real terminal. Left as an
explicit command block below, per this project's own standing discipline
(`C-021`/`C-038`) that a tag must never be handed over without its commit
already confirmed directly first — here, the commit is not just confirmed,
it is already made.

**Job 0 (doc-only, done first):** Steve's own real-hardware confirmation
that `T`/`t` sweeps the live sphere demo correctly (this session) satisfies
`WU-35a2`'s own `Accept:` line in full and `WU-35a3`'s own
mechanical-correctness confirmation. Both entries' status lines corrected
from `todo`/not-`green` to `green`, citing `00aa6a6` — see
`WORK-UNITS.md`. Doc-only commit `ccf6e29`. `WU-35a1` and `WU-35a4`'s own
entries left untouched, per this session's own standing instruction.

**Job 1 (`WU-46`, built and `green` this session):**
`generateFragmentsFieldRowsTagByFacing()` added to `core/binner.hpp`/`.cpp`
— the sibling function `CORRECTIONS.md` C-037/`DECISIONS.md` ADR-089 both
named as missing (field mode's own `resolveOneParity()` call site has no
facing-tag-aware fragment generator to call, even setting aside its own
`kBufferMode == Off` precondition). `DECISIONS.md` ADR-090 has the full
design conversation: checked directly against the real code first, this
turned out to need no new per-sample logic at all — every one of the four
existing `generateFragments*` entry points is already a thin wrapper
around one shared, anonymous-namespace template
(`generateFragmentsRowRangeImpl()`) templated on the tag policy and
parameterised on the row range independently, so the new function is a
pure composition of `generateFragmentsRowRangeTagByFacing()`'s own facing
lambda with `generateFragmentsFieldRows()`'s own row-visitation arguments.
Two new tests in `tests/test_binner.cpp`: a ground-truth equivalence check
(mirroring `WU-23a2a`'s own field-rows test) and a real self-fold sphere
check (mirroring `WU-28c`'s own facing-sign test), reusing both files'
already-established fixtures rather than inventing new ones.

**`WU-47` (wiring this into `runFrameField()`) scoped, not built —
`DECISIONS.md` ADR-090, `WORK-UNITS.md`'s own new entry.** Split from
`WU-46` purely on file-count sizing (`core/resolve.hpp` +
`core/pipeline.cpp` on top of `core/binner.hpp`/`.cpp` would be four
source files for one unit, past `SESSION-PROTOCOL.md`'s three-file cap),
not because the remaining work is a large open design question — ADR-090
worked through it directly: each of `resolveOneParity()`'s two per-field-
parity calls already runs a complete, independent PASS-1/PASS-2 cycle
producing its own full raster, so a k-buffer resolve fits entirely inside
one such call with no cross-parity interaction to arbitrate, and relaxing
`runFrameField()`'s own `kBufferMode == Off` precondition is therefore
mechanical, not a new design choice. `weightOut`'s own precondition is a
genuinely different, harder question (one caller-supplied buffer, written
once per call, ambiguous across the two per-frame calls field mode makes)
that ADR-090 does not resolve and leaves exactly as `ADR-077` put it.
`ADR-077` is narrowed, not reopened.

## Build/test verification (this session, cloud sandbox)

Built and tested independently of anything Session 72 reported, from a
fresh checkout — not merely trusted from `HANDOFF.md`'s own prior claims.
Baseline (before any change, `00aa6a6`): 28/28 `ctest` targets green, GCC
13.3.0 Release tile 2^5 — confirms Session 72's own sandbox report was
correct, checked directly rather than assumed. After `WU-46`'s three
changed files, four configurations, all green:

- GCC 13.3.0 Release, tile 2^5 — 28/28 `ctest` targets green.
- Clang (Ubuntu's default `clang++`) Release, tile 2^5 — 28/28 green.
- GCC 13.3.0 Debug, tile 2^4 — 28/28 green.
- GCC 13.3.0 Debug+ASan+UBSan, tile 2^5 — 28/28 green; `nm -D` confirms
  genuine instrumentation linkage (25 `asan`/11 `ubsan` hits); `ldd`
  confirms `libasan.so.8`/`libubsan.so.1` actually linked.

Zero compiler warnings in any configuration. `test_binner` itself: 39698
checks passing, including both new tests. All three changed files (
`src/core/binner.hpp`, `src/core/binner.cpp`, `tests/test_binner.cpp`)
written back to the real repository via the device bridge, then
re-staged and diffed to confirm each landed exactly as intended — brace
counts balanced in every file; raw paren counts checked as a *delta*
against each file's own `HEAD`-before-edit baseline (133/137 vs. a
pre-existing 115/119 baseline for `binner.hpp`, etc.) rather than as a
raw same-file balance, since this codebase's own prose-heavy comments
already carry a nonzero raw paren imbalance before any edit — confirmed
directly (`git show HEAD:<file>` vs. working tree), not assumed. The
build-then-test result above is the authoritative check regardless; the
paren delta is a secondary sanity check, not a substitute for it.

Not yet built, run, tagged or pushed at Steve's own real terminal.

## What's next (Steve's own to run)

1. Review the diff — `git log --oneline -5`, `git show 00aa6a6`,
   `git show ccf6e29`, and this session's own upcoming `WU-46` commit
   (see below).
2. Push everything this session committed locally but could not push
   (no `git` credentials in the device-bridge shell). Three things ride
   together here: the `C-038` git-history correction (`00aa6a6`, already
   tagged `wu-35a4-green` locally), the doc-only `WU-35a2`/`WU-35a3`/
   `C-039` commit (`ccf6e29`), and this session's own `WU-46` commit
   (tagged `wu-46-green` locally, see below — commit hash not known until
   after this HANDOFF is written; check `git log --oneline -3` first).
   The misplaced tag must be deleted on `origin` before the corrected one
   can be pushed — `git push` refuses to overwrite an existing remote tag
   under the same name:
   ```
   cd ~/src/scatter-dve
   git status --short
   git log --oneline -5 --decorate
   git push origin --delete wu-35a4-green
   git push origin main
   git push origin --tags
   ```
   The last command pushes both `wu-35a4-green` (recreated on `00aa6a6`
   this session) and `wu-46-green` (this session's own new tag) in one
   step — both already exist locally, created directly against verified
   commits, not blindly re-run.
3. Build and run the real test suite at a real terminal for the parts
   this sandbox cannot reach (`test_decklink_live_sphere`,
   `test_decklink_device`) — this session's own sandbox verification
   covers `scatter-core` only, same limitation every session has named.
4. `WU-47` (field-mode k-buffer wiring) is scoped, ready to pick up next
   — see `WORK-UNITS.md`'s own entry and `DECISIONS.md` ADR-090.

## What's broken / flagged, not fixed (out of this session's own scope)

**`docs/wu-audit-2026-08.md` line 113 is still stale — re-checked directly
this session, not fixed, per the same standing instruction Session 72
already followed.** It still reads: "The real-content gap (single tag per
call) was already found and logged as C-020/ADR-062 before this sweep,
and closed by WU-28c. Nothing new." Still incorrect: `WU-28c` only ever
*built* the TagByFacing functions; the gap was not actually closed until
`WU-35a4` (now genuinely committed, `00aa6a6`) wired them into
`core/pipeline.cpp`. Left unfixed — touching an audit doc is outside this
session's own scope (`core/binner.hpp`/`.cpp`, `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md`), same restraint Session 72 already
applied.

## Untouched, deliberately

`INVARIANTS.md` (not touched, not read for a change — no invariant this
session's own work bears on). `ADR-059/062/065/077/085/086/087/088/089`
(not reopened; `ADR-090` narrows one clause of `ADR-077`'s own precondition,
explicitly, rather than reopening it). `WU-35a1`/`WU-35a4`'s own
`WORK-UNITS.md` entries (left exactly as Session 72 wrote them, per this
session's own standing instruction — this session's own verification found
nothing wrong in either). `src/core/resolve.hpp`, `src/core/pipeline.cpp`
(read, not edited — `WU-47`'s own job, not this session's). `WU-35`'s own
rump scope and `WU-37` (Specular LUTs, still blocked — `docs/sources/WU-SM-01.md`
line 45 re-checked directly this session, EP 0248626/US 4,899,295 still
"NOT YET HELD") both considered as this session's own Job 1 candidate and
not picked, in favour of `WU-46` — see this session's own scoping
reasoning, `DECISIONS.md` ADR-090.
