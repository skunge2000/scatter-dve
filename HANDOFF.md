# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 62 (`WU-44b` — Phase 9's seventh unit, the second of `WU-44`'s
own lettered split: `tests/test_binner.cpp`/`test_splat.cpp`/`test_scan_
order_invariance.cpp`, PASS 1 fragment/splat colour fixtures, ADR-085).

**Tag:** `wu-44a-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both `d78a122`,
`git describe --tags --exact-match HEAD` — `wu-44a-red`, dereferencing to
that same commit). `git status --short` read empty at session start —
clean tree, Session 61's own work already committed, tagged and pushed by
Steve, exactly as its own HANDOFF.md account expected. `.git/index.lock`
was absent at the real first check (`ls -la .git/index.lock` — "No such
file or directory"). State (a), genuinely confirmed, so the session
proceeded. This session's own three-file change (`tests/test_binner.cpp`,
`WORK-UNITS.md`, this file) is not yet committed, tagged or pushed —
that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly before assuming
it is absent, and check whether it actually blocks git (`git add --dry-run
-A`) before concluding either way — see "Environment check" below.

## This session in full

Opened with a real state-verification step, per this session's own
opening instruction, before reading anything else. Found `HEAD ==
origin/main == d78a122`, the expected `WU-44a` commit message, `wu-44a-red`
dereferencing to that same commit, a clean tree, and no `.git/index.lock`.
State (a), genuinely confirmed, so the session proceeded.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 61's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own `WU-44` entry (the split rationale
and method), `WU-44a`'s own full entry (just landed), and `WU-44b`'s own
stub entry under Phase 9 (`todo`), all before touching anything.

**`WU-44b`'s own file list re-derived before trusting it, per its own
prior-session instruction that the list "may already be stale by then":**
`grep -rn '\bFrag\b\|\bAccumCell\b\|\bSourceRaster\b\|\.\(R\|G\|B\)\b'
tests/test_binner.cpp tests/test_splat.cpp tests/test_scan_order_
invariance.cpp` against a fresh clone of `wu-44a-red` reproduced the exact
same per-file hit counts Session 61 recorded — the file list was not
stale. `src/core/binner.cpp`/`.hpp` and `src/core/splat.cpp`/`.hpp` read in
full and confirmed unchanged since `WU-39`–`WU-42` landed.

**Real job, confirmed by direct inspection before any fixture was
touched, not assumed from the stub's own label:** `test_binner.cpp`'s own
two red checks (`:794`/`:795`, inside
`test_shading_multiplies_rgb_intensity_ahead_of_frag_construction()`) were
failing because that test's own independent mirror of `applyShading()`
(`mirrorToRgbBt601`/`mirrorFromRgbBt601`) was a full BT.601 YCbCr round
trip, predating `WU-41`'s RGB-native migration. `WU-41` changed both
sides of what that mirror was mirroring: `SourceRaster`'s r/g/b fields now
hold genuine RGB (not Y/Cb/Cr), and `core/binner.cpp`'s own real
`applyShading()` is now a bare `Colour{c.r*intensity, c.g*intensity,
c.b*intensity}` per-channel scale — confirmed by reading that function
directly, not assumed from its name. The test's own mirror was never
updated to match, so it was running the fixture's literal RGB values
(`R=20000, G=40000, B=25000`) through a formula that no longer describes
either the data or the production function. Verified numerically before
writing any fix: the stale mirror computes `(9109.264, 23058.617,
32815.104)` from those inputs — nothing close to the raw values
`applyShading()` actually scales directly.

**Fix: replaced the stale mirror with a direct RGB-domain one
(`mirrorApplyShadingRgb`), independently written — never calling
`core/binner.cpp`'s own private `applyShading()` — matching `WU-34b`/
ADR-084's "mirror the math independently" precedent and `WU-44a`'s own
application of it last session.** The fixture's three source planes
(`yPlane`/`cbPlane`/`crPlane`, stale names for literal RGB data) renamed to
`rPlane`/`gPlane`/`bPlane`; the dead YCbCr-domain intermediates and
`expectedYSample`/`expectedCbSample`/`expectedCrSample` removed. Repo-wide
grep confirmed no other reference to the removed names survives, and that
`kChromaZero` (still legitimately used elsewhere at the v210/chroma wire
boundary) is no longer referenced anywhere in `test_binner.cpp`.
`test_splat.cpp` and `test_scan_order_invariance.cpp` checked directly and
found to carry no equivalent staleness — `test_scan_order_invariance.cpp`'s
own `SignatureRaster` still names two filler planes `cb`/`cr` and seeds
them with `kChromaZero`, but only as an arbitrary bit-pattern filler for a
scan-order equivalence check that never derives an expected colour value
from them; a naming leftover, not a numerical error, left untouched
deliberately rather than renamed for cosmetic consistency alone.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, showed exactly the two files this account claims plus this
`HANDOFF.md` — `tests/test_binner.cpp`, `WORK-UNITS.md` — matching line
counts) before running anything.

## Build/test matrix — full twelve configurations (real source-code change)

Unlike `WU-43`'s docs-only check, this unit changes a test file — a real
source-code change — so the full matrix applies, matching `WU-39`–`WU-42`/
`WU-44a`'s own practice. Fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` into the cloud sandbox,
confirmed at `HEAD = wu-44a-red = d78a122` before any edit; this session's
own change applied from the verified-written real-repository copy
(`git diff --stat` against the clean clone: exactly `tests/test_binner.cpp`,
nothing else, before any build ran). GCC 13.3.0 and Clang 18.1.3, same
toolchain as prior sessions. Twelve configurations, matching `WU-44a`'s
own confirmed breakdown: GCC and Clang, Release and Debug, tile 4 and tile
5 (eight), plus GCC + ASan alone and GCC + UBSan alone, each at both tile
sizes (four more) — not two combined `-fsanitize=address,undefined`
builds.

| Configuration | Build | `ctest` | `test_binner` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 10963 checks |
| GCC, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 10963 checks |
| GCC, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 39139 checks |
| GCC, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 39139 checks |
| Clang, Release, tile 4 | clean, no warnings | 26/28 pass | PASS, 10963 checks |
| Clang, Debug, tile 4 | clean, no warnings | 26/28 pass | PASS, 10963 checks |
| Clang, Release, tile 5 | clean, no warnings | 26/28 pass | PASS, 39139 checks |
| Clang, Debug, tile 5 | clean, no warnings | 26/28 pass | PASS, 39139 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 10963 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 39139 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 10963 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 26/28 pass, no sanitizer trap | PASS, 39139 checks |

Twelve rows. **`test_binner` is fully green in all twelve: 0 of 10963
checks fail at tile 4, 0 of 39139 at tile 5** — both totals match
`CORRECTIONS.md` C-033's own tile-dependent baseline exactly, confirming
this fix changed only the correctness of the six shading checks, not the
fixture's own shape or count. No ASan or UBSan trap fired in any of the
four sanitizer configurations, on any test — grepped every `ctest
--output-on-failure` log directly for `AddressSanitizer`/
`UndefinedBehaviorSanitizer`/`runtime error:` (zero hits), and separately
confirmed via `nm | grep -i asan`/`ubsan` that the ASan/UBSan binaries
genuinely carried sanitizer instrumentation, not inferred from a clean
`ctest` result alone.

Per-test check counts on the two still-red tests, diffed directly against
`wu-44a-red`'s own baseline rather than assumed to carry over — identical
in every one of the twelve configurations above:

- `test_zoneplate`: 22 of 42537 checks fail (`test_zoneplate.cpp:209`,
  `:212`, `:213`) — matches exactly, tile-invariant as before.
- `test_pipeline_bytes`: 3 of 42 checks fail (`test_pipeline_bytes.cpp:406`,
  `:452`, `:552`) — matches exactly.

**26 of 28 tests now pass, up from 25 of 28 at `wu-44a-red` — `test_binner`
is the one that moved.** Every other test passes in every configuration,
exactly as `wu-44a-red` left them.

## Flag for Steve, not resolved here

**Carried forward unchanged from Sessions 56–61:** the `video::Raster444`-
vs-`video::RasterRGB` question and I7's non-achromatic round-trip breakage
are both still open, both still Steve's own call, neither touched this
session. `test_pipeline_bytes.cpp`'s own possible link to that same root
cause (flagged Session 61, `WU-44d`'s own job to check) is also
unchanged — not investigated this session, since it is outside `WU-44b`'s
own scope (neither `test_zoneplate.cpp` nor `test_pipeline_bytes.cpp` is
one of `WU-44b`'s three files, and this session did not touch either).

## Where we are

`WU-44b` (`test_binner.cpp`/`test_splat.cpp`/`test_scan_order_
invariance.cpp` PASS 1 colour fixtures) is built, verified against the
full twelve-configuration matrix, and closed out here. `test_binner`
itself is now fully green — the one sub-unit `WU-44a`'s own handoff
flagged as "most likely to turn a currently-red test fully green" did so.
`WU-44` as a whole is now `WU-44a` (done) + `WU-44b` (done) through
`WU-44e` (scoped, not started — see `WORK-UNITS.md`). The suite's
build/test state: `test_zoneplate` and `test_pipeline_bytes` are still
genuinely red, neither in `WU-44b`'s own scope to fix (both flagged
Steve's own call or `WU-44d`'s own job, per `WU-44`'s entry). "Green after
every unit" (ADR-085 §5) still has not resumed — it is a whole-`WU-44`-
phase property, not per-test, and two of the three originally-red tests
remain red.

## Next work unit

**Do not start `WU-44c` this session — this session's own instruction was
to stop at one `WU-44` cluster and hand off, even with budget left, and
that instruction was followed.** Whoever starts next: `WU-44c`
(`test_kbuffer_resolve.cpp`, `test_kbuffer_storage.cpp`,
`test_layered_composite.cpp`, `test_pageturn.cpp`, `test_shapes.cpp`) is
flagged in `WORK-UNITS.md`'s own entry as likely too large for one session
as scoped (five files, two substantial) — re-grep first, per this
project's own standing discipline, and split into `WU-44c1`/`WU-44c2` (etc)
before writing any fixture rather than assuming it fits. `WU-44d`
(`test_pipeline_bytes.cpp` and three others) is the sub-unit most likely to
need real investigation rather than a mechanical fixture fix — see
`WU-44`'s own entry for why its own three red checks may share
`test_zoneplate.cpp`'s I7 root cause. `WU-44e` is flagged likely empty
(verify, don't assume).

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause,
flagged last session, is also not this session's to resolve.

## Blocked / red

**Red, genuinely and expectedly — narrower than `wu-44a-red`'s own red
state, not identical to it.** `test_zoneplate` and `test_pipeline_bytes`
fail identically to `wu-44a-red`'s own baseline (same lines, same counts,
in all twelve configurations checked this session). `test_binner`, the
third test that was red at `wu-44a-red`, is now green in every
configuration. Every other test (26 of 28) passes. `ctest` was not run on
Steve's own real terminal yet this session — see "Steve's own next steps"
below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. This
session used a genuinely fresh `git clone` of the already-tagged
`wu-44a-red` commit, confirmed identical to Steve's own real repository
before any edit, with this session's own changed file
(`tests/test_binner.cpp` — the only file the build/test matrix above
actually depends on; `WORK-UNITS.md` and this file are documentation,
copied in afterward and not part of what was built) copied in from the
verified-written real-repository copy — `git diff --stat` against the
clean clone confirmed exactly that one file changed, nothing else, before
any build ran.

**A stray `.git/index.lock` appeared this session, the same self-inflicted,
already-documented Sessions-55–61 pattern.** This session's own opening
state-verification found the repository genuinely clean (`ls -la
.git/index.lock` — "No such file or directory"). By the time this
session's own close-out verification ran (`git status --short`/`git diff
--stat`, through the device bridge, after the three changed files were
written back and before this block was written), a fresh, empty
`.git/index.lock` had appeared — the device-bridge shell cannot delete
files by default, so git's own internal cleanup unlink silently failed on
one of the intervening `git status`/`git diff` calls, exactly as
documented. Confirmed directly that this one actually blocks git, not
assumed either way: `git add --dry-run -A` failed with "Another git
process seems to be running" / "Unable to create... File exists." The
close-out block below makes `rm -f .git/index.lock` its own first line,
and it is not just precautionary this time — it is expected to be
genuinely needed, the same as Session 61's own close-out found.

**Separately: `mcp__remote-devices__device_stage_files` returned `HTTP 403
untrusted_device` this session** when re-staging the three written files
from the Mac to confirm the writes landed (the assistant's own device
sign-in read as stale to the bridge). Write confirmation for this
session's three files was done a different way instead — direct `git
status --short`/`git diff --stat`/`wc -l`/`grep` checks against the real
repository through `device_bash` (which kept working throughout) rather
than a re-stage-and-diff — and all of it matched what was intended: `git
diff --stat` showed exactly the three files this account claims, line
counts matched exactly, and every real function name this session
introduced or removed was confirmed present/absent by direct `grep`
against the real file. Steve may need to re-sign-in to the Claude desktop
app on this Mac (a sign-in banner was reportedly raised there) before the
next session's own file-staging works normally again — flagged here, not
fixed, since it is a device-trust action only Steve can take.

## Append to DECISIONS.md

None this session. `WU-44b` implements ADR-085's own already-accepted
scope (fixing a stale pre-migration test fixture the `WU-41` RGB-native
cutover left behind); no new architectural decision was made.

## Append to CORRECTIONS.md

None this session. The staleness this unit fixed was already correctly
anticipated and logged by `CORRECTIONS.md` C-033 and `WORK-UNITS.md`'s own
`WU-43`/`WU-44` entries ("shading-mirror fixture staleness — WU-44's own
job") — this session confirmed and fixed the anticipated issue rather than
discovering a new one, so it is documented in `WORK-UNITS.md`'s own
`WU-44b` entry rather than logged here as a fresh correction.

## Closed out this session

**`WU-44b` (`tests/test_binner.cpp`/`test_splat.cpp`/`test_scan_order_
invariance.cpp`): the stale pre-`WU-41` YCbCr-domain shading mirror in
`tests/test_binner.cpp`'s own `test_shading_multiplies_rgb_intensity_
ahead_of_frag_construction()` replaced with a direct RGB-domain mirror,
independently derived from `applyShading()`'s own real (bare per-channel
multiply) formula without calling it. `test_binner` turns fully green (0
of 10963 checks fail at tile 4, 0 of 39139 at tile 5, matching C-033's own
baseline totals) in all twelve configurations, with no regression to any
other test. `test_splat.cpp` and `test_scan_order_invariance.cpp` checked
directly and confirmed to need no fixture work of their own. Not yet
committed.** Two files (`tests/test_binner.cpp`, `WORK-UNITS.md`), this
`HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show `test_binner` newly green, with `test_zoneplate` and
`test_pipeline_bytes` still red exactly as `wu-44a-red` had them**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: `test_decklink_device`'s own duplex check (standing PSU exception,
unrelated, only present when built against the Blackmagic SDK on your own
Mac — this session's cloud sandbox has no SDK and does not build that
target at all, so its own ctest total there is 28, not 29; your own real
build's total may differ by exactly that one target and that is expected,
not a discrepancy to chase), plus `test_zoneplate` (22 checks) and
`test_pipeline_bytes` (3 checks). `test_binner` itself should now PASS
with no failing checks (10963 or 39139 depending on your own
`SCATTER_TILE_LOG2` setting) — if it still shows the old `:794`/`:795`
failures, stop and report exactly what failed rather than assuming it is
safe to tag past, since this session's own cloud sandbox found it green in
all twelve configurations checked.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly (`test_zoneplate`/
`test_pipeline_bytes` still red, by design, the standing Phase 9
exception), and `close.sh` refuses to tag past any failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the two named above (plus your own real Mac's `test_decklink_device`
duplex exception, if built with the SDK), close out with the **manual tag
path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add tests/test_binner.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-44b: tests/test_binner.cpp shading-mirror fixture fixed for RGB-native applyShading (ADR-085); test_binner fully green, test_zoneplate/test_pipeline_bytes still red, see HANDOFF.md"
git tag -a wu-44b-red -m "Stale pre-WU-41 YCbCr shading-mirror fixture (test_binner.cpp:794/:795) replaced with a direct RGB-domain mirror; test_binner fully green in all twelve configurations (0/10963 at tile 4, 0/39139 at tile 5); test_zoneplate (22/42537) and test_pipeline_bytes (3/42) unchanged, both outside this unit's own scope; WU-44c-e remain, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44b-red`, not `wu-44b-green`** — `test_binner` itself is
fully green, but the suite as a whole is not: `test_zoneplate` and
`test_pipeline_bytes` are both still red, both outside this unit's own
scope, and ADR-085 §5's "green after every unit" resumption is a
whole-`WU-44`-phase property, not a per-test one. `WU-44c` through `WU-44e`
remain.

This exact list of 3 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 3 `M`/`??` lines (`tests/test_binner.cpp`
modified, `WORK-UNITS.md` modified, `HANDOFF.md` modified) and nothing
else. Still worth a quick `git status --short` yourself before pasting
this block, since time has passed since that check.
