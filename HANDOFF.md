# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 68 (`WU-45` — I7 narrowed to in-gamut content, resolving
Phase 9's own deferred decision; DECISIONS.md ADR-086, INVARIANTS.md I7).

**Tag:** `wu-44e-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both `72bbdac`,
`git describe --tags --exact-match HEAD` — `wu-44e-red`, dereferencing to
that same commit). `git status --short` read empty at session start —
clean tree, Session 67's own work already committed, tagged and pushed by
Steve, exactly as its own HANDOFF.md account expected. State (a), genuinely
confirmed, so the session proceeded.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly (this session's
own recurring quirk, again present at close — see "Environment check"
below) and whether it actually blocks git (`git add --dry-run -A`) before
concluding either way.

## This session in full

Steve made the real call this session, in conversation, after the prior
session (67) laid out the `test_zoneplate`/I7 options rather than resolving
them: **Option A — accept the RGB boundary conversion's own clamp as
permanent, narrow I7 to in-gamut content, fix the test fixture.** Not
Option B (widen `Frag`'s own colour storage through the whole splat/
resolve hot path to preserve I7 exactly as originally written) — that
option's own real cost (a second, `Frag`-size-breaking migration
comparable to Phase 9 itself) was laid out alongside Option A, and Steve
picked A.

**Investigated before touching anything, and found broader than the prior
session's own framing.** `core/resolve.hpp`'s own `runFrameFile()`
comment (written at WU-40) already called this "the honestly-reportable
breakage ADR-085 Section 5 accepts for this phase" — a provisional
judgment explicitly scoped to Phase 9's own duration. `video/chroma.hpp`'s
RGB boundary conversion (`ycbcrToRgbRow()`/`rgbToYcbcrRow()`) clamps to
`Sample`'s own `[0, 65535]` range; not every I2-legal YCbCr triple's
implied RGB fits, and the clip is real on both channels, including luma
(Y is a linear combination of R, G and B, so a clip on any one of R/G/B
moves the recovered Y too). `test_zoneplate.cpp`'s `makeFlat()` sets Y,
Cb and Cr to the same code for three of its four tested values, which is
saturated and non-achromatic (I3's centre is `kCode10ChromaZero=512`),
not achromatic — those three fail on both channels.

**Checked directly whether ramp/excursion were really exempt, per this
project's own standing discipline (C-027) — they were not**, and the
prior session's own characterisation of them as immune (offered before
this session existed) did not hold up. Verified against a fresh,
unmodified clone of `wu-44e-red` with a temporary diagnostic before
making any change: `makeRamp()`'s own `rampCode(0, span)` and
`makeExcursion()`'s own `kCycle[0]` are both `kCode10Min`, aligning luma
and the nearest chroma sample at their shared `x=0` on the same extreme
combination `flat`'s bad codes hit. Both patterns' own luma check was
already failing there in the pristine baseline — the original 22-check
failure count already included it, previously undercounted in scope
(misread as confined to `flat`), not in total.

**Why accepted rather than fixed, checked directly against the real
code.** A genuine fix (carry full precision through the RGB path, clamp
only at `v210::packRow` the way I2 already licenses) collides with
`core/types.hpp`'s `Frag` struct: `static_assert(sizeof(Frag) == 16)`,
explicitly load-bearing per its own comment (fragment traffic is pass 1's
dominant memory cost). Widening `Frag::R/G/B` to survive out-of-range
values breaks that budget on the hottest struct in the pipeline — a
second migration comparable to Phase 9 itself, not undertaken.

**Fixed: one file, `tests/test_zoneplate.cpp`.** `testI7Pattern()` now
takes independent `lumaExpectedExact`/`chromaExpectedExact` flags; both
`true` only for the genuinely achromatic flat code (512), legal-range-only
otherwise — `flat`'s other three codes, `ramp` and `excursion` all move to
this treatment on both channels. No `src/` file touched. `INVARIANTS.md`'s
I7 superseded directly; `DECISIONS.md` gains `ADR-086`.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(re-read directly after each write, expected to show exactly four files —
`DECISIONS.md`, `INVARIANTS.md`, `tests/test_zoneplate.cpp`,
`WORK-UNITS.md` — plus this `HANDOFF.md`) before running anything further
this session.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox, confirmed at `HEAD = wu-44e-red = 72bbdac` before any
build; only `tests/test_zoneplate.cpp` was ever edited there — `git status
--short` in the sandbox read exactly that one modified file throughout.
GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and tile 5 (eight),
plus GCC + ASan alone and GCC + UBSan alone, each at both tile sizes (four
more) — twelve total.

| Configuration | Build | `ctest` | `test_zoneplate` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| GCC, Debug, tile 4 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| GCC, Release, tile 5 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| GCC, Debug, tile 5 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| Clang, Release, tile 4 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| Clang, Debug, tile 4 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| Clang, Release, tile 5 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| Clang, Debug, tile 5 | clean, no warnings | 28/28 pass | PASS, 109203 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 109203 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 109203 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 109203 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 28/28 pass, no sanitizer trap | PASS, 109203 checks |

Twelve rows, zero real warnings, zero sanitizer traps (grepped every
`ctest --output-on-failure` log — zero hits), sanitizer instrumentation
confirmed genuinely present (`nm -D`/`ldd` against `test_zoneplate`: 28
`asan` hits/`libasan.so.8` linked, 14 `ubsan` hits/`libubsan.so.1`
linked). `test_zoneplate`'s own 109203 checks are tile-invariant. **28 of
28 tests pass in every configuration — the whole suite, fully green, for
the first time since `wu-38` closed Phase 9's own opening unit.**

## Flag for Steve, not resolved here

None carried forward — this was the one open item, and it is now closed.
If a future session ever wants to pursue the `Frag`-widening fix (the
real I7-preserving alternative, Option B), it needs its own proposal and
sign-off, the same way ADR-085 itself got one — not a quiet reopening of
ADR-086.

## Where we are

**The suite is fully green: 28 of 28 tests pass.** `test_zoneplate` moved
from the suite's sole red test (22 of 42537) to fully passing (109203 of
109203) via a genuine, Steve-approved narrowing of I7 to in-gamut content,
not a fixture patch papering over a real bug. Phase 9 (`WU-38`–`WU-44e`)
and its own deferred decision (`WU-45`) are both complete. "Green after
every unit" (ADR-085 §5) resumes for real starting here.

## Next work unit

None scoped. With the suite fully green and Phase 9's own last open
question closed, there is no standing work item in `WORK-UNITS.md` beyond
this entry. Whatever comes next is a fresh scoping decision for Steve, not
inherited from this session.

## Open questions

None. The `video::Raster444`-vs-`video::RasterRGB` naming question
(`HANDOFF.md`'s own long-carried item) is unaffected by this session's
work — it was always a naming/typing observation about `Raster444`'s own
mid-pipeline reuse, not the I7 breakage itself, and remains exactly as
documented in `core/pipeline.cpp`'s own WU-41/WU-42 comments: a permanent,
accepted design choice, not a defect, not blocking anything.

## Blocked / red

None. `ctest` was not run on Steve's own real terminal yet this session —
see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. Fresh
`git clone` of the already-tagged `wu-44e-red` commit, confirmed identical
to Steve's own real repository before any build; only
`tests/test_zoneplate.cpp` was ever edited in the sandbox.

**`.git/index.lock` was present in the real repository at both this
session's own opening and closing checks** — the same recurring,
already-documented quirk since Session 55, checked directly rather than
assumed: `git add --dry-run -A` failed with "Another git process seems to
be running" / "File exists," confirming it genuinely blocks git in the
device-bridge shell, and the shell could not remove it itself
("Operation not permitted"). This did **not** block this session's own
work: all building and testing happened in a fresh cloud-sandbox clone,
and the changed-file writes below used the device bridge's own
Python-based file-write path, not `git`. The close-out block's own first
line, `rm -f .git/index.lock`, is expected to be genuinely needed on
Steve's own real terminal.

File writes and re-reads this session used `device_bash` directly (Python
read-modify-write against `DECISIONS.md`, `INVARIANTS.md` and
`WORK-UNITS.md`, replaying the exact same edits made and verified in the
cloud sandbox against `tests/test_zoneplate.cpp`), confirmed by re-reading
and grepping the real files afterward, not inferred from the write calls
succeeding alone.

## Append to DECISIONS.md

`ADR-086` — added in full this session. See `DECISIONS.md` itself; not
re-quoted here since the state-file discipline is "read the real file,"
not "trust the handoff's paraphrase of it."

## Append to CORRECTIONS.md

None this session. Nothing shipped or misled a decision — the prior
session's own "ramp/excursion look exempt" characterisation was offered
as an in-conversation working hypothesis, checked and corrected within
this same session before anything was written to `WORK-UNITS.md` or
`DECISIONS.md`, not carried forward as a stale claim into a state file.
Per this project's own standing convention (e.g. `WU-44d`'s own
`test_field_pipeline.cpp` count drift), a correction goes to
`CORRECTIONS.md` when something already shipped or misled a real decision
on record — a conversational hypothesis corrected before it reached a
state file does not meet that bar.

## Closed out this session

**`WU-45` (`tests/test_zoneplate.cpp`, `DECISIONS.md` `ADR-086`,
`INVARIANTS.md` I7): I7 narrowed to in-gamut YCbCr content, resolving
Phase 9's own deferred `test_zoneplate` breakage as a genuine, Steve-
approved design decision, not a fixture patch.** Full twelve-configuration
matrix run to confirm empirically: `test_zoneplate` newly green (109203/
109203 checks) in all twelve, zero warnings, zero sanitizer traps.
**28 of 28 tests pass — the suite is fully green.** Not yet committed.
Four files (`DECISIONS.md`, `INVARIANTS.md`, `tests/test_zoneplate.cpp`,
`WORK-UNITS.md`), plus this `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show 28/28 pass in the cloud sandbox's own terms; your own real
terminal may show 29 with `test_decklink_device`'s own standing duplex-
check exception (ADR-035, only present when built against the Blackmagic
SDK — unrelated to this session's own work) still passing separately,
or 28 if not built against the SDK**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: every test passes. If anything differs from that — including
`test_zoneplate` — stop and report exactly what, rather than assuming it
is safe to tag past, since this session's own cloud sandbox found a clean
28/28 across all twelve configurations checked.

**`./tools/close.sh` can be run this time** — this is the first session
since `wu-38` where the tree is genuinely, fully green — but the manual
path below is given regardless, since this session did not run it and
cannot confirm what it would do on your own real terminal.

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add DECISIONS.md INVARIANTS.md tests/test_zoneplate.cpp tests/test_pipeline_bytes.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-45: I7 narrowed to in-gamut YCbCr content (DECISIONS.md ADR-086, INVARIANTS.md I7); tests/test_zoneplate.cpp's own three non-achromatic flat codes, plus ramp/excursion (both found to also touch the same extreme combination at their own x=0), moved to legal-range-only on both luma and chroma, matching what the RGB boundary conversion (ADR-085/WU-40) already actually does. No src/ file touched -- the conversion is confirmed correct as built. Suite fully green: 28/28, see HANDOFF.md. Also fixes a stale comment in tests/test_pipeline_bytes.cpp (line ~184) that still named the old single-parameter testI7Pattern(..., chromaExpectedExact=true) signature and called flat chroma exactness a general convention -- corrected to name the new two-parameter signature and pin the exactness to kChromaZero specifically (I3's achromatic centre), the one flat code that stays in-gamut; comment-only, no logic change, sanity-rebuilt and test_pipeline_bytes re-run clean."
git tag -a wu-45-green -m "I7 (INVARIANTS.md) narrowed to in-gamut YCbCr content, formalising WU-40's own provisional 'accepted for this phase' judgment (core/resolve.hpp's runFrameFile() comment) now that Phase 9 (WU-38-WU-44e) is complete -- Steve's own explicit decision (Option A over Option B: accept the RGB boundary conversion's real, documented clamp rather than a second Frag-widening migration comparable in scope to Phase 9 itself, which core/types.hpp's own static_assert(sizeof(Frag) == 16) makes a real, unbudgeted cost, not a quick fix). tests/test_zoneplate.cpp's own test_i7_identity_full_pipeline()/testI7Pattern() gained independent lumaExpectedExact/chromaExpectedExact flags; only the genuinely achromatic flat code (kCode10ChromaZero=512) is exact on both channels now. Checked directly, not assumed: ramp and excursion were found to ALSO touch the same extreme combination at their own x=0 (rampCode(0,span) and kCycle[0] are both kCode10Min), against a fresh unmodified wu-44e-red clone before any change -- the original 22-check failure count already included this, previously undercounted in scope. No src/ file touched. Full twelve-configuration matrix (GCC 13.3.0/Clang 18.1.3, Release/Debug, tile 4/5, plus GCC+ASan-only and GCC+UBSan-only at both tile sizes): 28 of 28 tests pass in every configuration, zero warnings, zero sanitizer traps, sanitizer instrumentation confirmed genuinely linked (nm -D/ldd). The suite is fully green for the first time since wu-38 opened Phase 9; 'green after every unit' (ADR-085 section 5) resumes for real."
git push origin main
git push origin --tags
```

**Tag name is `wu-45-green`** — the first real `-green` tag since
`wu-39-green`, and the first time this project's own "green after every
unit" property (ADR-085 §5) has held since Phase 9 opened. This is a real
green, directly confirmed via the cloud sandbox's own full `ctest` run
across all twelve configurations, not assumed.

This exact list of 6 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (a second pass, after the repo-wide "did this change make
anything else inconsistent" grep surfaced the stale comment in
tests/test_pipeline_bytes.cpp fixed above) — `.git/index.lock` was
still present at that same moment but, as before, did not block `git
status --short` itself (confirmed both ways, not assumed either implies
the other); the command ran clean and read exactly 6 `M` lines:
`DECISIONS.md`, `HANDOFF.md`, `INVARIANTS.md`, `WORK-UNITS.md`,
`tests/test_pipeline_bytes.cpp`, `tests/test_zoneplate.cpp` — matching
this list exactly. `git diff --stat` at the same moment: 507
insertions(+), 246 deletions(-) across those 6 files. Still worth a
quick `git status --short` yourself before pasting this block, since
time has passed since that check.
