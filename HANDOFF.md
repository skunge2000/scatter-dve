# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 64 (`WU-44c2` — Phase 9's ninth unit, the second of `WU-44c`'s
own lettered split: `tests/test_layered_composite.cpp`, PASS 2
layered-composite colour fixtures, ADR-085).

**Tag:** `wu-44c1-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5` — both `6dd556e`, `git rev-parse HEAD origin/main` —
both `6dd556e`, `git describe --tags --exact-match HEAD` — `wu-44c1-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 63's own work already committed,
tagged and pushed by Steve, exactly as its own HANDOFF.md account expected.
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
`HEAD == origin/main == 6dd556e`, the expected `WU-44c1` commit message,
`wu-44c1-red` dereferencing to that same commit, a clean tree. State (a),
genuinely confirmed.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 63's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own `WU-44` entry (split rationale),
`WU-44c` entry (this session's predecessor's own split into
`WU-44c1`/`WU-44c2`/`WU-44c3`), `WU-44c1`'s own full entry, and `WU-44c2`'s
own stub, all before touching anything.

**`WU-44c2`'s own scope re-grepped directly this session, per this
project's own standing discipline that a scoping stub is a plan, not a
fact (C-027), rather than trusted from the stub:** the exact query the
stub itself named — `grep -n '\bAccumCell\b\|\bResolvedCell\b\|
\bCompositedCell\b\|\bBackground\b\|\.\(R\|G\|B\)\b'
tests/test_layered_composite.cpp` — against the real repository, before
anything else. 86 combined-line hits (matching the stub and `WU-44c`'s own
split entry exactly); per-pattern `AccumCell`=25 (stub said 24 — a
per-pattern/combined-count bucketing difference, not a sign the file
changed), `CompositedCell`=14, `Background`=7, `.R/.G/.B`=41 (both exactly
matching). `git log --oneline -- tests/test_layered_composite.cpp
src/core/resolve.hpp src/core/resolve.cpp` confirmed the last commit
touching any of the three is still `WU-42` (`1297c19`) — nothing has moved
since `WU-44c1`'s own account. `src/core/resolve.hpp` and
`src/core/resolve.cpp` read in full this session (not assumed to still
match `WU-44c1`'s own account) before touching the test file.

**Confirmed the stub's own judgment that this file needs no further
lettered split, against the real file rather than assuming it was right:**
411 lines, one clearly-scoped test file with two parts by its own header
(Part A: four direct hand-built-`AccumCell` unit tests of
`compositeLayered()`; Part B: two pipeline-scenario tests) — well inside
`SESSION-PROTOCOL.md`'s sizing rule for a single unit, not a five-way split
the way `test_layered_composite.cpp` being "the densest file in the whole
`WU-44` split" might have suggested on its own.

**Every fixture in the file hand-checked individually against
`resolve.cpp`'s real RGB-domain arithmetic this session — going beyond
`WU-44c1`'s own first pass, which checked only the file's three shared
helper re-derivations (`expectedDivide()`/`expectedBlend()`/
`expectedSum()`) and explicitly did not claim to have checked each test
function's own use of them.** All three helpers confirmed verbatim
duplicates of `resolve.cpp`'s file-local `divideRounded()`/`blend()`/
`sumCells()`, re-checked directly against the real `resolve.cpp` text read
this session. Then each of the six test functions traced individually
against the real `compositeLayered()`/`composite()`/`normaliseCell()`
code paths (four in Part A, two in Part B) — see `WORK-UNITS.md`'s own
`WU-44c2` entry for the full per-test account. **No staleness found in any
of the six** — every fixture either genuinely mirrors `resolve.cpp`'s real
math independently where the file claims to (Part A4's own hand-derived
blend, Part B's real-splatted-data checks), or legitimately cross-checks
against an already-independently-tested reference function (`composite()`
itself, tested elsewhere) for the branches that document doing so — the
same accepted strategy `WU-44c1`'s own Parts A/B used for
`compositeKBuffer()`. No `Y`/`Cb`/`Cr`-named field or pre-rename constant
found anywhere in the file — not a stale survivor of the `WU-41` migration
the way `test_binner.cpp` was. No `tests/` or `src/` file content changed
this session.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, showed exactly the two files this account claims —
`WORK-UNITS.md`, this `HANDOFF.md` — matching line counts) before running
anything.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

This unit changes no `tests/` or `src/` file, but the full matrix was still
run — to empirically confirm the "no fixture change needed" finding above,
not merely trust the reading. Fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` into the cloud sandbox,
confirmed at `HEAD = wu-44c1-red = 6dd556e` before any build; `git status
--short` in the sandbox stayed empty throughout (nothing was ever edited
there). GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and tile 5
(eight), plus GCC + ASan alone and GCC + UBSan alone, each at both tile
sizes (four more) — twelve total, not two combined
`-fsanitize=address,undefined` builds.

| Configuration | Build | `ctest` | `test_layered_composite` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| GCC, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| GCC, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| GCC, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| Clang, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| Clang, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| Clang, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| Clang, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 28 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 28 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 28 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 28 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 28 checks |

Twelve rows, zero real warnings (one harmless `CMake Warning:
Manually-specified variables were not used by the project:
CMAKE_C_COMPILER` at configure time in every configuration — this is a
pure C++ project with no C sources, so the flag going unconsumed is
expected, not a compiler diagnostic or code-quality signal), zero
sanitizer traps — grepped every `ctest --output-on-failure` log for
`AddressSanitizer`/`UndefinedBehaviorSanitizer`/`runtime error:` (zero
hits), and confirmed via `nm` that the ASan/UBSan `test_layered_composite`
binaries genuinely carry sanitizer instrumentation (26 and 11
case-insensitive `asan`/`ubsan` symbol hits respectively), not inferred
from a clean `ctest` result alone. `test_zoneplate` (22 of 42537) and
`test_pipeline_bytes` (3 of 42) fail identically to `wu-44c1-red`'s own
baseline in every configuration — this unit touched neither file. `28
checks` for `test_layered_composite` in every configuration — genuinely
tile-invariant, confirmed directly, unlike `test_binner`'s (C-033) or
`test_kbuffer_storage`'s (`WU-44c1`) tile-dependent counts. **26 of 28
tests pass in every configuration, identical to `wu-44c1-red`'s own
count** — this unit changes no test's pass/fail state, only confirms one
already-passing test is passing for the right reasons.

## Flag for Steve, not resolved here

**Carried forward unchanged:** the `video::Raster444`-vs-`video::RasterRGB`
question and I7's non-achromatic round-trip breakage are both still open,
both still Steve's own call, neither touched this session.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause
(`WU-44d`'s own job to check) is also unchanged. Nothing new to flag from
this unit's own file — `test_layered_composite.cpp` carries no
`Y`/`Cb`/`Cr`-named field or pre-migration-looking constant the way
`test_shapes.cpp` does (flagged for `WU-44c3` in Session 63's own entry).

## Where we are

`WU-44c2` (`test_layered_composite.cpp` PASS 2 layered-composite colour
fixtures) is built, verified against the full twelve-configuration matrix,
and closed out here — the file confirmed already correct, no fixture
change needed. `WU-44` as a whole is now `WU-44a` (done) + `WU-44b` (done)
+ `WU-44c1` (done) + `WU-44c2` (done) through `WU-44c3`/`WU-44d`/`WU-44e`
(scoped, not started — see `WORK-UNITS.md`). The suite's build/test state
is unchanged from `wu-44c1-red`: `test_zoneplate` and `test_pipeline_bytes`
are still genuinely red, neither in this unit's own scope. "Green after
every unit" (ADR-085 §5) still has not resumed.

## Next work unit

**Do not start `WU-44c3` this session — this session's own instruction was
to stop at `WU-44c2` and hand off, even with budget left, and that
instruction was followed.** Whoever starts next: `WU-44c3`
(`test_pageturn.cpp` + `test_shapes.cpp`) is scoped in `WORK-UNITS.md`,
with `test_shapes.cpp`'s own pre-migration-looking `Background` constant
already flagged for it (Session 63's own entry) — a cosmetic question, not
a correctness one, per that entry. `WU-44d` (`test_pipeline_bytes.cpp` and
three others) remains the sub-unit most likely to need real investigation.
`WU-44e` remains flagged likely empty.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause is
also not this session's to resolve. Whether to rename/replace
`test_shapes.cpp`'s pre-migration-looking background constant is
`WU-44c3`'s own call, not decided here.

## Blocked / red

**Red, identically to `wu-44c1-red`'s own state — nothing in this session
changed which tests pass or fail.** `test_zoneplate` and
`test_pipeline_bytes` fail identically to `wu-44c1-red`'s own baseline
(same lines, same counts, in all twelve configurations checked this
session). Every other test (26 of 28) passes. `ctest` was not run on
Steve's own real terminal yet this session — see "Steve's own next steps"
below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. This
session used a genuinely fresh `git clone` of the already-tagged
`wu-44c1-red` commit, confirmed identical to Steve's own real repository
before any build; no `tests/` or `src/` file was ever edited in the
sandbox, since this unit's own real job turned out to be confirmation, not
repair.

**A stray `.git/index.lock` was present in the real repository this
session, the same self-inflicted, already-documented Sessions-55–63
pattern.** Checked directly, not assumed either way: `ls -la
.git/index.lock` showed the file present at this session's own
state-verification step (0 bytes, today's date); `git status --short`
through the device bridge produced `warning: unable to unlink
'.../.git/index.lock': Operation not permitted` (the device-bridge shell
cannot delete files by default, so git's own internal cleanup unlink
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
start (staging `SESSION-PROTOCOL.md`, then `WORK-UNITS.md` for editing)
and worked normally throughout — no `HTTP 403 untrusted_device` recurrence
this session.

## Append to DECISIONS.md

None this session. No new architectural decision was made — this unit
confirmed existing fixtures correct rather than changing any behaviour or
scope.

## Append to CORRECTIONS.md

None this session. No claim in any existing state file was found wrong —
the per-pattern `AccumCell` count (25 here vs. the stub's 24) and the
tile-invariant `28`-check baseline for `test_layered_composite` are fresh,
directly-checked findings, not corrections of any prior claim (nobody had
asserted either figure before this unit), so both are logged in
`WORK-UNITS.md`'s own `WU-44c2` entry rather than here.

## Closed out this session

**`WU-44c2` (`tests/test_layered_composite.cpp`): all six test functions
(four in Part A, two in Part B) hand-checked individually against
`core/resolve.hpp`/`.cpp`'s real RGB-domain arithmetic (read in full this
session) and confirmed already correct — no stale fixture found, no
`tests/` or `src/` file changed. Full twelve-configuration matrix run to
confirm empirically: the file fully green in all twelve (28 checks,
tile-invariant), zero warnings, zero sanitizer traps, no regression to the
two tests still genuinely red (`test_zoneplate`, `test_pipeline_bytes`),
both outside this unit's own scope. Not yet committed.** Two files
(`WORK-UNITS.md`, this `HANDOFF.md`).

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show exactly the same 26/28 pass state `wu-44c1-red` already had,
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
test, including `test_layered_composite`, should pass exactly as it
already did at `wu-44c1-red` — if anything differs from that, stop and
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
git commit -m "WU-44c2: tests/test_layered_composite.cpp confirmed already correct for RGB-domain resolve.cpp math (ADR-085); no fixture change needed; every fixture hand-checked individually, see HANDOFF.md"
git tag -a wu-44c2-red -m "tests/test_layered_composite.cpp (Part A's four unit tests, Part B's two pipeline-scenario tests) hand-checked against core/resolve.cpp's real RGB-domain arithmetic and confirmed already correct in all twelve configurations (28 checks, tile-invariant); no stale fixture found, no tests/src file changed; test_zoneplate (22/42537) and test_pipeline_bytes (3/42) unchanged, both outside this unit's own scope; WU-44c3/WU-44d/WU-44e remain, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44c2-red`, not `wu-44c2-green`** — this sub-unit's own
file is fully green, but the suite as a whole is not: `test_zoneplate` and
`test_pipeline_bytes` are both still red, both outside this unit's own
scope, and ADR-085 §5's "green after every unit" resumption is a
whole-`WU-44`-phase property, not a per-test one. `WU-44c3`, `WU-44d` and
`WU-44e` remain.

This exact list of 2 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 2 `M`/`??` lines (`WORK-UNITS.md`
modified, `HANDOFF.md` modified) and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
