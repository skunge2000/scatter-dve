# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 63 (`WU-44c1` — Phase 9's eighth unit, the first of `WU-44c`'s
own new lettered split: `tests/test_kbuffer_storage.cpp`/
`test_kbuffer_resolve.cpp`, PASS 1/2 k-buffer colour fixtures, ADR-085).

**Tag:** `wu-44b-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5` — both `b679969`, `git rev-parse HEAD origin/main` —
both `b679969`, `git describe --tags --exact-match HEAD` — `wu-44b-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 62's own work already committed,
tagged and pushed by Steve, exactly as its own HANDOFF.md account expected.
`.git/index.lock` was absent at the real first check (`ls -la
.git/index.lock` — "No such file or directory"). `mcp__remote-devices__
device_stage_files` worked normally this session (tested against
`SESSION-PROTOCOL.md` first) — Session 62's own `HTTP 403 untrusted_device`
did not recur; whatever device-trust issue that was, it appears resolved,
not re-flagged here since this session had no trouble to report. State (a),
genuinely confirmed, so the session proceeded. This session's own two-file
change (`WORK-UNITS.md`, this file) is not yet committed, tagged or pushed
— that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly before assuming
it is absent, and check whether it actually blocks git (`git add --dry-run
-A`) before concluding either way.

## This session in full

Opened with a real state-verification step before reading anything else:
`HEAD == origin/main == b679969`, the expected `WU-44b` commit message,
`wu-44b-red` dereferencing to that same commit, a clean tree, no
`.git/index.lock`. State (a), genuinely confirmed.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 62's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own `WU-44` entry (split rationale),
`WU-44a`'s and `WU-44b`'s own full entries, and `WU-44c`'s own stub, all
before touching anything.

**`WU-44c`'s own stub, already flagged by two prior sessions as "likely
too large for one session as scoped," re-grepped directly rather than
trusted:** the exact query the stub itself named —
`grep -rn '\bAccumCell\b\|\bResolvedCell\b\|\bCompositedCell\b\|
\bBackground\b\|\.\(R\|G\|B\)\b' tests/test_kbuffer_resolve.cpp tests/
test_kbuffer_storage.cpp tests/test_layered_composite.cpp tests/
test_pageturn.cpp tests/test_shapes.cpp` — against a fresh clone of
`wu-44b-red`. Combined per-file hits: `test_kbuffer_resolve.cpp`=62,
`test_kbuffer_storage.cpp`=17, `test_layered_composite.cpp`=86,
`test_pageturn.cpp`=17, `test_shapes.cpp`=8. `git log --oneline -- ` all
five confirmed the last commit touching any of them is `WU-42`
(`1297c19`) — nothing has moved since. `src/core/resolve.hpp` and
`src/core/resolve.cpp` read in full (`normaliseCell()`, `composite()`,
`compositeLayered()`, `compositeKBuffer()` and their file-local
`divideRounded()`/`blend()`/`sumCells()`/`asBackground()`/`nearerThan()`
helpers) before hand-checking any fixture against them.

**Split `WU-44c` into `WU-44c1`/`WU-44c2`/`WU-44c3` in `WORK-UNITS.md`,
based on the real re-grep above, not a pre-guessed grouping** — the same
convention `WU-44` itself used for its own split (and `WU-23a2a`/`b`,
`WU-28a`–`d` before it): `WU-44c1` (`test_kbuffer_storage.cpp` +
`test_kbuffer_resolve.cpp`, the k-buffer pair — WU-28a's storage-only file
naturally paired with WU-28b's own resolve file rather than left alone),
`WU-44c2` (`test_layered_composite.cpp` alone — still the single densest
file in the whole `WU-44` split, 86 hits), `WU-44c3` (`test_pageturn.cpp` +
`test_shapes.cpp` — long in line count, light in real colour-fixture
content, mostly lattice/Jacobian geometry). Only `WU-44c1` built this
session, per this project's own one-cluster-per-session discipline —
`WU-44c2`/`WU-44c3` are scoped in `WORK-UNITS.md`, not started, even
though this session's own scoping pass read both files in full (see
`WORK-UNITS.md`'s own `WU-44c2`/`WU-44c3` entries for what that reading
already found, and what still needs each sub-unit's own formal build/test
close-out).

**`WU-44c1`'s real job, confirmed by inspection before any file was
touched: neither `test_kbuffer_storage.cpp` nor `test_kbuffer_resolve.cpp`
needed a fixture fix.** `test_kbuffer_storage.cpp` (WU-28a, PASS 1 storage/
accumulation only, confirmed by its own file header and by zero
`ResolvedCell`/`CompositedCell`/`Background` hits) checks k-buffer storage
against an already-tested production path (`sumBanks()`) or against itself
(order-independence, eviction self-consistency) — never an independent
colour-formula mirror, so there was no stale mirror to look for.
`test_kbuffer_resolve.cpp` (WU-28b, PASS 2 k-buffer resolve) does use
independent derivations in places — hand-checked all of them directly
against `resolve.cpp`'s own real arithmetic this session: Parts A/B mostly
cross-check `compositeKBuffer()` against `composite()`/`compositeLayered()`
themselves (a legitimate, already-documented strategy in the file's own
header, since those two functions are independently tested elsewhere, not
`compositeKBuffer()`'s own arithmetic circularly validating itself), and
`test_blend_known_ratio()` hand-derives the blend formula directly and
matches `resolve.cpp`'s own `blend()` exactly. **No staleness found in
either file — unlike `WU-44b`'s own `test_binner.cpp` finding, this
cluster's "currently green" tests turn out to be green for the right
reasons, not by coincidence.** No `tests/` or `src/` file content changed
this session.

**New finding, not previously logged:** `test_kbuffer_storage`'s own total
check count depends on `SCATTER_TILE_LOG2` (314 at tile 4, 1082 at tile 5,
identical across every configuration below) — the same tile-dependence
`CORRECTIONS.md` C-033 already documented for `test_binner`.
`test_kbuffer_resolve`'s own total (186944) is genuinely tile-invariant in
every configuration — its own checks are either tile-agnostic hand-built
`KSlot` arrays or a fixed-size `runFrame()` output compared across thread
counts. Not a correction to any existing claim (nobody had asserted either
count before), just a fresh, directly-checked baseline.

**Flagged, not fixed, for whoever picks up `WU-44c3`:** `test_shapes.cpp`'s
own `runFlatSourceThroughLattice()` sets `params.background =
Background{fromCode10(64), fromCode10(512), fromCode10(512)}` — the exact
pre-`WU-41` YCbCr-domain "legal black" triple `core/resolve.hpp`'s own
comment and `CORRECTIONS.md` C-032 describe, reused here as an arbitrary
distinguishable background constant. Every check that reads it compares
the destination raster back against that same value, so this is not the
C-032 numerical bug reproduced — the test's own pass/fail does not depend
on what the triple actually is — but it is a confusing thing for a future
reader to trip over. Whether to rename/replace it for clarity is
`WU-44c3`'s own call, a cosmetic question, not a correctness one — see
`WORK-UNITS.md`'s own `WU-44c3` entry.

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
confirmed at `HEAD = wu-44b-red = b679969` before any build; `git status
--short` in the sandbox stayed empty throughout (nothing was ever edited
there). GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and tile 5
(eight), plus GCC + ASan alone and GCC + UBSan alone, each at both tile
sizes (four more) — twelve total, not two combined
`-fsanitize=address,undefined` builds.

| Configuration | Build | `ctest` | `test_kbuffer_storage` | `test_kbuffer_resolve` |
|---|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 314 checks | PASS, 186944 checks |
| GCC, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 314 checks | PASS, 186944 checks |
| GCC, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 1082 checks | PASS, 186944 checks |
| GCC, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 1082 checks | PASS, 186944 checks |
| Clang, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 314 checks | PASS, 186944 checks |
| Clang, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 314 checks | PASS, 186944 checks |
| Clang, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 1082 checks | PASS, 186944 checks |
| Clang, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 1082 checks | PASS, 186944 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 314 checks | PASS, 186944 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 1082 checks | PASS, 186944 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 314 checks | PASS, 186944 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 1082 checks | PASS, 186944 checks |

Twelve rows, zero warnings, zero sanitizer traps — grepped every `ctest
--output-on-failure` log for `AddressSanitizer`/`UndefinedBehaviorSanitizer`/
`runtime error:` (zero hits), and confirmed via `nm` that the ASan/UBSan
binaries for both test executables genuinely carry sanitizer
instrumentation (26/29 and 7/14 case-insensitive `asan`/`ubsan` symbol
hits respectively), not inferred from a clean `ctest` result alone.
`test_zoneplate` (22 of 42537) and `test_pipeline_bytes` (3 of 42) fail
identically to `wu-44b-red`'s own baseline in every configuration — this
unit touched neither file. **26 of 28 tests pass in every configuration,
identical to `wu-44b-red`'s own count** — this unit changes no test's
pass/fail state, only confirms two already-passing tests are passing for
the right reasons.

## Flag for Steve, not resolved here

**Carried forward unchanged:** the `video::Raster444`-vs-`video::RasterRGB`
question and I7's non-achromatic round-trip breakage are both still open,
both still Steve's own call, neither touched this session.
`test_kbuffer_resolve.cpp`'s own Part C (`test_kbuffer_pipeline_threads_1_
matches_threads_8`) still constructs its source via `w.y`/`w.cb`/`w.cr`
naming and reads `video::Raster444`'s own `.Y`/`.Cb`/`.Cr` fields — the same
open naming question, not re-opened here since it does not affect that
part's own correctness (it only checks bit-identity across thread counts,
never derives an expected colour value). `test_pipeline_bytes.cpp`'s own
possible link to the same root cause (`WU-44d`'s own job to check) is also
unchanged.

## Where we are

`WU-44c1` (`test_kbuffer_storage.cpp`/`test_kbuffer_resolve.cpp` PASS 1/2
k-buffer colour fixtures) is built, verified against the full twelve-
configuration matrix, and closed out here — both files confirmed already
correct, no fixture change needed. `WU-44` as a whole is now `WU-44a`
(done) + `WU-44b` (done) + `WU-44c1` (done) through `WU-44c3`/`WU-44d`/
`WU-44e` (scoped, not started — see `WORK-UNITS.md`). The suite's
build/test state is unchanged from `wu-44b-red`: `test_zoneplate` and
`test_pipeline_bytes` are still genuinely red, neither in this unit's own
scope. "Green after every unit" (ADR-085 §5) still has not resumed.

## Next work unit

**Do not start `WU-44c2` this session — this session's own instruction was
to stop at one `WU-44c` sub-cluster and hand off, even with budget left,
and that instruction was followed.** Whoever starts next: `WU-44c2`
(`test_layered_composite.cpp` alone) is scoped in `WORK-UNITS.md` with
this session's own hand-check of its `expectedDivide()`/`expectedBlend()`/
`expectedSum()` re-derivations already done (found to match `resolve.cpp`
exactly) — that reading is not a substitute for `WU-44c2`'s own formal
build/test verification and close-out, which is still that session's own
job. `WU-44c3` (`test_pageturn.cpp` + `test_shapes.cpp`) is scoped with the
`test_shapes.cpp` background-constant flag above already noted for it.
`WU-44d` (`test_pipeline_bytes.cpp` and three others) remains the sub-unit
most likely to need real investigation. `WU-44e` remains flagged likely
empty.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause is
also not this session's to resolve. Whether to rename/replace
`test_shapes.cpp`'s pre-migration-looking background constant is
`WU-44c3`'s own call, not decided here.

## Blocked / red

**Red, identically to `wu-44b-red`'s own state — nothing in this session
changed which tests pass or fail.** `test_zoneplate` and
`test_pipeline_bytes` fail identically to `wu-44b-red`'s own baseline (same
lines, same counts, in all twelve configurations checked this session).
Every other test (26 of 28) passes. `ctest` was not run on Steve's own real
terminal yet this session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. This
session used a genuinely fresh `git clone` of the already-tagged
`wu-44b-red` commit, confirmed identical to Steve's own real repository
before any build; no `tests/` or `src/` file was ever edited in the
sandbox, since this unit's own real job turned out to be confirmation, not
repair.

**A stray `.git/index.lock` appeared this session, the same self-inflicted,
already-documented Sessions-55–62 pattern.** `.git/index.lock` was absent
at this session's own first check (`ls -la .git/index.lock` — "No such
file or directory"). By the time this session's own close-out
verification ran (`git add --dry-run -A`, through the device bridge, to
check the repo-wide grep step below), that same command's own output
included `warning: unable to unlink '.../.git/index.lock': Operation not
permitted` — the device-bridge shell cannot delete files by default, so
git's own internal cleanup unlink silently failed on one of this
session's own `git status`/`git diff`/`git add --dry-run` calls, exactly
as documented. Confirmed directly that this one actually blocks git, not
assumed either way: a follow-up `git add --dry-run -A` failed outright
with "Another git process seems to be running" / "Unable to create...
File exists." The close-out block below makes `rm -f .git/index.lock` its
own first line, and it is not just precautionary — it is expected to be
genuinely needed, the same as prior sessions' own close-outs found.

`mcp__remote-devices__device_stage_files` was tested at this session's own
start (staging `SESSION-PROTOCOL.md`) and worked normally — Session 62's
own `HTTP 403 untrusted_device` did not recur. All of this session's own
staging and re-staging (docs read at the start, `WORK-UNITS.md`/
`HANDOFF.md` re-staged after writing) used the normal stage-and-diff path,
not the `device_bash`-only fallback Session 62 needed.

## Append to DECISIONS.md

None this session. No new architectural decision was made — this unit
confirmed existing fixtures correct rather than changing any behaviour or
scope.

## Append to CORRECTIONS.md

None this session. No claim in any state file was found wrong — the new
tile-dependent check-count finding for `test_kbuffer_storage` is a fresh
baseline, not a correction of an existing claim (nobody had stated a count
for it before), so it is logged in `WORK-UNITS.md`'s own `WU-44c1` entry
rather than here.

## Closed out this session

**`WU-44c1` (`tests/test_kbuffer_storage.cpp`/`test_kbuffer_resolve.cpp`):
both files hand-checked against `core/resolve.hpp`/`.cpp`'s real RGB-domain
arithmetic (read in full this session) and confirmed already correct — no
stale fixture found, no `tests/` or `src/` file changed. Full twelve-
configuration matrix run to confirm empirically: both files fully green in
all twelve (314/1082 checks depending on tile for `test_kbuffer_storage`,
186944 checks tile-invariant for `test_kbuffer_resolve`), zero warnings,
zero sanitizer traps, no regression to the two tests still genuinely red
(`test_zoneplate`, `test_pipeline_bytes`), both outside this unit's own
scope. `WU-44c` split into `WU-44c1`/`WU-44c2`/`WU-44c3` in `WORK-UNITS.md`;
only `WU-44c1` built. Not yet committed.** Two files (`WORK-UNITS.md`,
this `HANDOFF.md`).

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show exactly the same 26/28 pass state `wu-44b-red` already had,
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
test, including `test_kbuffer_storage` and `test_kbuffer_resolve`, should
pass exactly as it already did at `wu-44b-red` — if anything differs from
that, stop and report exactly what, rather than assuming it is safe to tag
past, since this session's own cloud sandbox found no change in any test's
pass/fail state across all twelve configurations checked.

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
git commit -m "WU-44c1: tests/test_kbuffer_storage.cpp and test_kbuffer_resolve.cpp confirmed already correct for RGB-domain resolve.cpp math (ADR-085); no fixture change needed; WU-44c split into WU-44c1-c3, see HANDOFF.md"
git tag -a wu-44c1-red -m "test_kbuffer_storage.cpp/test_kbuffer_resolve.cpp hand-checked against core/resolve.cpp's real RGB-domain arithmetic and confirmed already correct in all twelve configurations (314/1082 checks depending on tile for storage, 186944 tile-invariant for resolve); no stale fixture found, no tests/src file changed; test_zoneplate (22/42537) and test_pipeline_bytes (3/42) unchanged, both outside this unit's own scope; WU-44c2/c3/WU-44d/WU-44e remain, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44c1-red`, not `wu-44c1-green`** — this sub-unit's own
two files are fully green, but the suite as a whole is not: `test_zoneplate`
and `test_pipeline_bytes` are both still red, both outside this unit's own
scope, and ADR-085 §5's "green after every unit" resumption is a
whole-`WU-44`-phase property, not a per-test one. `WU-44c2`, `WU-44c3`,
`WU-44d` and `WU-44e` remain.

This exact list of 2 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 2 `M`/`??` lines (`WORK-UNITS.md`
modified, `HANDOFF.md` modified) and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
