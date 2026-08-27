# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 65 (`WU-44c3` — Phase 9's tenth unit, the third and last of
`WU-44c`'s own lettered split: `tests/test_pageturn.cpp` +
`tests/test_shapes.cpp`, page-turn/shape pipeline colour touches, ADR-085).

**Tag:** `wu-44c2-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5` — both `b9948d9`, `git rev-parse HEAD origin/main` —
both `b9948d9`, `git describe --tags --exact-match HEAD` — `wu-44c2-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 64's own work already committed,
tagged and pushed by Steve, exactly as its own HANDOFF.md account expected.
No `.git/index.lock` blocked this state-verification step itself (see
"Environment check" below for what showed up once real work started).
State (a), genuinely confirmed, so the session proceeded.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly before assuming
it is absent, and check whether it actually blocks git (`git add --dry-run
-A`) before concluding either way — see "Environment check" below, it was
present and genuinely blocking again this session, same as most sessions
since Session 55.

## This session in full

Opened with a real state-verification step before reading anything else:
`HEAD == origin/main == b9948d9`, the expected `WU-44c2` commit message,
`wu-44c2-red` dereferencing to that same commit, a clean tree. State (a),
genuinely confirmed.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 64's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own `WU-44` entry (split rationale),
`WU-44c` entry (the split into `WU-44c1`/`WU-44c2`/`WU-44c3`), `WU-44c1`'s
own full entry, `WU-44c2`'s own full entry, and `WU-44c3`'s own stub, all
before touching anything.

**`WU-44c3`'s own scope re-grepped directly this session, per this
project's own standing discipline that a scoping stub is a plan, not a
fact (C-027), rather than trusted from the stub:** the exact query the
stub itself named — `grep -n '\bAccumCell\b\|\bResolvedCell\b\|
\bCompositedCell\b\|\bBackground\b\|\.\(R\|G\|B\)\b' tests/test_pageturn.cpp
tests/test_shapes.cpp` — against the real repository, before anything
else. Matched the stub's own counts exactly, pattern for pattern:
`test_pageturn.cpp` 17 combined hits (`AccumCell`=9, `CompositedCell`=2,
`.R/.G/.B`=6), `test_shapes.cpp` 8 combined hits (`Background`=1,
`.R/.G/.B`=7) — no drift this time, unlike the small per-pattern bucketing
differences `WU-44c`'s split entry and `WU-44c2` each found against their
own stubs. `git log --oneline -- tests/test_pageturn.cpp tests/test_shapes.cpp
src/core/resolve.hpp src/core/resolve.cpp` confirmed the last commit
touching any of the four is still `WU-42` (`1297c19`) — nothing has moved
since `WU-44c2`'s own account. `src/core/resolve.hpp` and
`src/core/resolve.cpp` read in full this session (not assumed to still
match `WU-44c2`'s own account) before touching either test file.

**Confirmed the stub's own judgment that these two files need no further
lettered split, against real content rather than assuming the stub's
grouping guess was right:** both files read in full (432 and 480 lines).
Real content confirms both are genuinely lattice/Jacobian geometry files,
not just long by line count — `test_pageturn.cpp`'s five tests are four
pure-geometry checks plus one pipeline-scenario test; `test_shapes.cpp`'s
eight tests are five pure geometry checks plus three flat-source pipeline
runs. Combined 25 colour-relevant hits across 912 lines is the lightest
colour-fixture density of any `WU-44c` sub-unit (`WU-44c1`: 79 combined;
`WU-44c2`: 86 in one file alone) — real justification for one unit, not an
artefact of the stub's grouping.

**Every colour-relevant fixture hand-checked individually this session —
and neither file turned out to need the `WU-34b`/ADR-084 "mirror the math
independently" discipline at all, traced rather than assumed from the low
hit count.** `test_pageturn.cpp`'s only colour-bearing test
(`test_pipeline_pageturn_transparent_accumulates_over_page_behind`) checks
an exact componentwise `AccumCell` addition identity (I6) — never a
hand-derived expected *value*, so there is no independent mirror that
could have gone stale under the `Y/Cb/Cr` → `R/G/B` rename; its trailing
`composite()` calls check only a visibility inequality against production
`composite()` directly. `test_shapes.cpp`'s three pipeline tests
(`inHull()`/`near()`) check `runFrame()`'s real output against a
generous-margin hull between the test's own source and background colours
— also never a hand-computed expected colour. Confirmed the channel
correspondence itself is right (`dest.Y`↔`background.R`, `.Cb`↔`.G`,
`.Cr`↔`.B`), not merely assumed: `runFrame()` operates directly in RGB end
to end (`core/resolve.hpp`'s own file header), so `video::Raster444`'s
`.Y/.Cb/.Cr` field names (still carrying the standing `Raster444`-vs-
`RasterRGB` naming question, untouched, not this unit's job) genuinely do
line up channel-for-channel with `Background`'s real `.R/.G/.B` fields.
**No staleness found in either file** — see `WORK-UNITS.md`'s own
`WU-44c3` entry for the full per-test account.

**Two cosmetic-naming leftovers found, both decided this session rather
than left unaddressed by omission — neither a correctness bug:**

1. `test_shapes.cpp`'s `runFlatSourceThroughLattice()` sets
   `params.background = Background{fromCode10(64), fromCode10(512),
   fromCode10(512)}` — the exact pre-`WU-41` YCbCr-domain "legal black"
   triple (Session 63's own flag, carried forward by `WU-44c2`). Traced
   every reader this session: `isBackground`/`inHull()` compare the
   destination raster back against this same triple, componentwise, so the
   test's own pass/fail depends only on distinguishability from each
   test's own source colour, not on what the triple represents — not the
   C-032 numerical bug reproduced (self-referential test constant, never
   read by production code). **Decision: leave it as-is, not renamed** —
   matches this project's own established precedent (`WU-44b` on
   `test_scan_order_invariance.cpp`, `WU-44c1` on `test_kbuffer_storage.cpp`'s
   `y`/`cb`/`cr` parameter names) for this exact class of leftover.
2. New finding this session, not previously flagged: `test_pageturn.cpp`'s
   own visibility check names its delta variables `dY`/`dCb`/`dCr`
   (`:419-421`) while actually differencing `CompositedCell::R/G/B` — same
   cosmetic category as (1). **Decision: leave it as-is, not renamed,**
   same precedent-driven reasoning.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, expected to show exactly the two files this account claims —
`WORK-UNITS.md`, this `HANDOFF.md`) before running anything further this
session.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

This unit changes no `tests/` or `src/` file, but the full matrix was still
run — to empirically confirm the "no fixture change needed" finding above,
not merely trust the reading. Fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` into the cloud sandbox,
confirmed at `HEAD = wu-44c2-red = b9948d9` before any build; `git status
--short` in the sandbox stayed empty throughout (nothing was ever edited
there). GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and tile 5
(eight), plus GCC + ASan alone and GCC + UBSan alone, each at both tile
sizes (four more) — twelve total, not two combined
`-fsanitize=address,undefined` builds.

| Configuration | Build | `ctest` | `test_pageturn` | `test_shapes` |
|---|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| GCC, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| GCC, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| GCC, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| Clang, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| Clang, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| Clang, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| Clang, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 126512 checks | PASS, 83385 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 126512 checks | PASS, 83385 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 126512 checks | PASS, 83385 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 126512 checks | PASS, 83385 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 126512 checks | PASS, 83385 checks |

Twelve rows, zero real warnings (one harmless `CMake Warning:
Manually-specified variables were not used by the project:
CMAKE_C_COMPILER` at configure time in every configuration — expected, not
a compiler diagnostic, this is a pure C++ project with no C sources), zero
sanitizer traps — grepped every `ctest --output-on-failure` log for
`AddressSanitizer`/`UndefinedBehaviorSanitizer`/`runtime error:` (zero
hits), and confirmed via `nm` that the ASan/UBSan `test_pageturn`/
`test_shapes` binaries genuinely carry sanitizer instrumentation (26/29 and
11/14 case-insensitive `asan`/`ubsan` symbol hits respectively), not
inferred from a clean `ctest` result alone. `test_zoneplate` (22 of 42537)
and `test_pipeline_bytes` (3 of 42) fail identically to `wu-44c2-red`'s own
baseline in every configuration — this unit touched neither file. Both
`test_pageturn` (126512 checks) and `test_shapes` (83385 checks) are
genuinely tile-invariant, confirmed directly by running each binary
standalone in every configuration, unlike `test_binner`'s (C-033) or
`test_kbuffer_storage`'s (`WU-44c1`) tile-dependent counts. **26 of 28
tests pass in every configuration, identical to `wu-44c2-red`'s own
count** — this unit changes no test's pass/fail state, only confirms two
already-passing tests are passing for the right reasons.

## Flag for Steve, not resolved here

**Carried forward unchanged:** the `video::Raster444`-vs-`video::RasterRGB`
question and I7's non-achromatic round-trip breakage are both still open,
both still Steve's own call, neither touched this session.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause
(`WU-44d`'s own job to check) is also unchanged. Two cosmetic-naming
leftovers found and decided (left as-is) this session — see "This session
in full" above and `WORK-UNITS.md`'s own `WU-44c3` entry; neither is a
correctness question needing Steve's own decision, but both are logged so
a future reader does not have to rediscover them.

## Where we are

`WU-44c3` (`test_pageturn.cpp` + `test_shapes.cpp` page-turn/shape pipeline
colour touches) is built, verified against the full twelve-configuration
matrix, and closed out here — both files confirmed already correct, no
fixture change needed. **`WU-44c` as a whole (`WU-44c1` + `WU-44c2` +
`WU-44c3`) is now complete.** `WU-44` as a whole is now `WU-44a` (done) +
`WU-44b` (done) + `WU-44c` (done, all three sub-units) through `WU-44d`/
`WU-44e` (scoped, not started — see `WORK-UNITS.md`). The suite's
build/test state is unchanged from `wu-44c2-red`: `test_zoneplate` and
`test_pipeline_bytes` are still genuinely red, neither in this unit's own
scope. "Green after every unit" (ADR-085 §5) still has not resumed.

## Next work unit

**Do not start `WU-44d` this session — this session's own instruction was
to stop at `WU-44c3` and hand off, even with budget left, and that
instruction was followed.** Whoever starts next: `WU-44d`
(`tests/test_pipeline_bytes.cpp`, `test_threading.cpp`,
`test_persistent_pool.cpp`, `test_field_pipeline.cpp`) is scoped in
`WORK-UNITS.md` — flagged there as the sub-unit most likely to need real
investigation, since `test_pipeline_bytes.cpp`'s own three failing checks
may share the same I7/non-achromatic root cause as `test_zoneplate.cpp`
rather than be a fixable stale fixture; check before assuming either way.
`WU-44e` remains flagged likely empty.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause is
also not this session's to resolve. `test_shapes.cpp`'s pre-migration-
looking background constant and `test_pageturn.cpp`'s `dY`/`dCb`/`dCr`
variable naming were this unit's own call and both were decided (left
as-is) — not open any more.

## Blocked / red

**Red, identically to `wu-44c2-red`'s own state — nothing in this session
changed which tests pass or fail.** `test_zoneplate` and
`test_pipeline_bytes` fail identically to `wu-44c2-red`'s own baseline
(same lines, same counts, in all twelve configurations checked this
session). Every other test (26 of 28) passes. `ctest` was not run on
Steve's own real terminal yet this session — see "Steve's own next steps"
below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. This
session used a genuinely fresh `git clone` of the already-tagged
`wu-44c2-red` commit, confirmed identical to Steve's own real repository
before any build; no `tests/` or `src/` file was ever edited in the
sandbox, since this unit's own real job turned out to be confirmation, not
repair.

**A stray `.git/index.lock` was present in the real repository this
session, the same self-inflicted, already-documented Sessions-55–64
pattern.** Checked directly, not assumed either way: `ls -la
.git/index.lock` showed the file present (0 bytes, today's date); `git
status --short` through the device bridge produced `warning: unable to
unlink '.../.git/index.lock': Operation not permitted` (the device-bridge
shell cannot delete files by default, so git's own internal cleanup unlink
silently fails); a direct follow-up `git add --dry-run -A` failed outright
with "Another git process seems to be running" / "Unable to create...
File exists" — confirming it genuinely blocks git in the device-bridge
shell, not merely present and harmless. Per this project's own standing
finding, this did **not** block this session's own work, since all
building and testing happened in a fresh cloud-sandbox clone, never in the
device-bridge-mounted working tree — only reading and re-staging state
files and writing back `WORK-UNITS.md`/`HANDOFF.md` touched the real
mounted tree, and neither of those needs git. The close-out block below
makes `rm -f .git/index.lock` its own first line, expected to be genuinely
needed on Steve's own real terminal, the same as every recent session's
own close-out has found.

`mcp__remote-devices__device_stage_files` was tested at this session's own
start (staging the six state files plus `resolve.hpp`/`.cpp`/
`test_pageturn.cpp`/`test_shapes.cpp`) and worked normally throughout — no
`HTTP 403 untrusted_device` recurrence this session.

## Append to DECISIONS.md

None this session. No new architectural decision was made — this unit
confirmed existing fixtures correct rather than changing any behaviour or
scope. (The two cosmetic-naming leftovers were decided as this sub-unit's
own scoped call, not an ADR-level decision.)

## Append to CORRECTIONS.md

None this session. No claim in any existing state file was found wrong —
the exact match between this session's own re-grep and the `WU-44c3` stub
is a confirmation, not a correction (unlike `WU-44c`'s split entry and
`WU-44c2`, which each found small per-pattern drift against their own
stubs); the tile-invariant `126512`/`83385`-check baselines for
`test_pageturn`/`test_shapes` are fresh, directly-checked findings, not
corrections of any prior claim (nobody had asserted either figure before
this unit), so both are logged in `WORK-UNITS.md`'s own `WU-44c3` entry
rather than here.

## Closed out this session

**`WU-44c3` (`tests/test_pageturn.cpp`, `tests/test_shapes.cpp`): every
colour-relevant fixture hand-checked individually against
`core/resolve.hpp`/`.cpp`'s real RGB-domain arithmetic (read in full this
session) and confirmed already correct — no stale fixture found, no
`tests/` or `src/` file changed. Two cosmetic-naming leftovers found and
decided (left as-is), logged above and in `WORK-UNITS.md`. Full
twelve-configuration matrix run to confirm empirically: both files fully
green in all twelve (126512 and 83385 checks respectively, tile-invariant),
zero warnings, zero sanitizer traps, no regression to the two tests still
genuinely red (`test_zoneplate`, `test_pipeline_bytes`), both outside this
unit's own scope. **`WU-44c` as a whole is now complete.** Not yet
committed.** Two files (`WORK-UNITS.md`, this `HANDOFF.md`).

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show exactly the same 26/28 pass state `wu-44c2-red` already had,
nothing newly green or newly red**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: `test_decklink_device`'s own standing duplex-check exception
(unrelated, only present when built against the Blackmagic SDK on your own
Mac — this session's cloud sandbox has no SDK, so its own ctest total
there is 28, not 29), plus `test_zoneplate` (22 checks) and
`test_pipeline_bytes` (3 checks) failing exactly as before. Every other
test, including `test_pageturn` and `test_shapes`, should pass exactly as
it already did at `wu-44c2-red` — if anything differs from that, stop and
report exactly what, rather than assuming it is safe to tag past, since
this session's own cloud sandbox found no change in any test's pass/fail
state across all twelve configurations checked.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly, and `close.sh`
refuses to tag past any failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the two named above (plus your own real Mac's `test_decklink_device`
duplex exception, if built with the SDK), close out with the **manual tag
path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add WORK-UNITS.md HANDOFF.md
git commit -m "WU-44c3: tests/test_pageturn.cpp and test_shapes.cpp confirmed already correct for RGB-domain resolve.cpp math (ADR-085); no fixture change needed; WU-44c (c1+c2+c3) now complete, see HANDOFF.md"
git tag -a wu-44c3-red -m "tests/test_pageturn.cpp and tests/test_shapes.cpp hand-checked against core/resolve.cpp's real RGB-domain arithmetic and confirmed already correct in all twelve configurations (126512 and 83385 checks respectively, tile-invariant); no stale fixture found, no tests/src file changed; two cosmetic YCbCr-era naming leftovers found and left as-is (test_shapes.cpp Background constant, test_pageturn.cpp dY/dCb/dCr variables); test_zoneplate (22/42537) and test_pipeline_bytes (3/42) unchanged, both outside this unit's own scope; WU-44c complete, WU-44d/WU-44e remain, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44c3-red`, not `wu-44c3-green`** — this sub-unit's own
two files are fully green, but the suite as a whole is not: `test_zoneplate`
and `test_pipeline_bytes` are both still red, both outside this unit's own
scope, and ADR-085 §5's "green after every unit" resumption is a
whole-`WU-44`-phase property, not a per-test one. `WU-44d` and `WU-44e`
remain.

This exact list of 2 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 2 `M`/`??` lines (`WORK-UNITS.md`
modified, `HANDOFF.md` modified) and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
