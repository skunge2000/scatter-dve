# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 67 (`WU-44e` — Phase 9's twelfth and final unit:
`tests/test_lighting.cpp`, `test_coarse_shading.cpp`, `test_morph.cpp`,
confirmed empty, ADR-085).

**Tag:** `wu-44d-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both `8b9d1c0`,
`git describe --tags --exact-match HEAD` — `wu-44d-red`, dereferencing to
that same commit). `git status --short` read empty at session start —
clean tree, Session 66's own work already committed, tagged and pushed by
Steve, exactly as its own HANDOFF.md account expected. No `.git/index.lock`
present at this state-verification step (`ls -la .git/index.lock` — no
such file). State (a), genuinely confirmed, so the session proceeded.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly before assuming
it is absent or present either way, and check whether it actually blocks
git (`git add --dry-run -A`) before concluding either way — it was
genuinely absent at this session's own opening check but genuinely
present and genuinely blocking by this session's own close (see
"Environment check" below), the same recurring pattern Sessions 55-66
already documented.

## This session in full

Opened with a real state-verification step before reading anything else:
`HEAD == origin/main == 8b9d1c0`, the expected `WU-44d` commit message,
`wu-44d-red` dereferencing to that same commit, a clean tree, no
`.git/index.lock`. State (a), genuinely confirmed.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 66's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), `WORK-UNITS.md`'s own `WU-44` top-level entry (split
rationale) and `WU-44d`'s own full entry, and `WU-44e`'s own stub under
Phase 9, all before touching anything.

**`WU-44e`'s own scope re-verified directly this session, per this
project's own standing discipline (C-027 — a scoping stub is a plan, not
a fact, and this holds exactly as much for a stub predicting emptiness as
one predicting work), rather than skipped on the stub's own "likely
empty" say-so.** Re-grepped `tests/test_lighting.cpp`,
`tests/test_coarse_shading.cpp` and `tests/test_morph.cpp` with the
stub's own query (`ResolvedCell|CompositedCell|SourceRaster|Frag|
AccumCell|YCbCr|.(R|G|B)|.(Y|Cb|Cr)`): zero hits, matching the stub
exactly. `#include` check confirms none of the three reaches a `core/`
header that could carry `Frag`/`AccumCell`/colour fields. `git log
--oneline --` for all three shows their most recent commits are `WU-34a`,
`WU-27` and `WU-13` respectively — all three predate `WU-38`/ADR-085
entirely.

**All three files also read in full this session, not left at the
grep-plus-includes check alone.** `test_lighting.cpp` (410 lines) and
`test_coarse_shading.cpp` (314 lines) both operate entirely on scalar
`double` lighting intensities (`shade()`, `CoarseShadingGrid::sample()`);
`test_morph.cpp` (235 lines) operates entirely on `Vec3` geometry
(`morphLattice()`, `Lattice::jacobian()`). None of the three has any code
path that could go stale under ADR-085's RGB rename — confirmed by
reading, not inferred from the zero-hit grep alone. The stub's own
prediction held in full; no work found in either direction (it was not
under-scoped, unlike some of `WU-44d`'s own file-count drift, and not
over-scoped either).

**Confirmed empty — no fixture change needed, the same "confirmed already
correct" shape `WU-44c1`/`c2`/`c3` used**, not `WU-44d`'s own investigative
shape: there was no known failing check here for `WU-44` to flag, and
none was found. No `src/` or `tests/` file touched this session.

**`WU-44` (`WU-44a` through `WU-44e`) is now complete.** `WORK-UNITS.md`'s
own top-level `WU-44` entry updated this session with a status note
recording this; `WU-44e`'s own stub entry replaced with its full built
account, matching every other `WU-44` sub-unit's own entry shape.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, expected to show exactly the two files this account claims —
`WORK-UNITS.md`, this `HANDOFF.md` — no `tests/` file this session, since
this unit found no fixture work) before running anything further this
session.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox, confirmed at `HEAD = wu-44d-red = 8b9d1c0` before any
build; `git status --short` in the sandbox read empty throughout — no
file was ever edited there, since this unit found no fixture work. GCC
13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and tile 5 (eight),
plus GCC + ASan alone and GCC + UBSan alone, each at both tile sizes
(four more) — twelve total, not two combined
`-fsanitize=address,undefined` builds.

| Configuration | Build | `ctest` | `test_lighting`/`test_coarse_shading`/`test_morph` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| GCC, Debug, tile 4 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| GCC, Release, tile 5 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| GCC, Debug, tile 5 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| Clang, Release, tile 4 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| Clang, Debug, tile 4 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| Clang, Release, tile 5 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| Clang, Debug, tile 5 | clean, no warnings | 27/28 pass | PASS, PASS, PASS |
| GCC + ASan only, tile 4 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, PASS, PASS |
| GCC + ASan only, tile 5 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, PASS, PASS |
| GCC + UBSan only, tile 4 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, PASS, PASS |
| GCC + UBSan only, tile 5 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, PASS, PASS |

Full per-test check counts (`test_lighting` 239, `test_coarse_shading`
305, `test_morph` 150189 — all confirmed tile-invariant) in
`WORK-UNITS.md`'s own `WU-44e` entry. Twelve rows, zero real warnings,
zero sanitizer traps (grepped every `ctest --output-on-failure` log for
`AddressSanitizer`/`UndefinedBehaviorSanitizer`/`runtime error:` — zero
hits), sanitizer instrumentation confirmed genuinely present (`nm -D` +
`ldd` against `test_morph`: 16 `asan` hits / `libasan.so.8` linked, 8
`ubsan` hits / `libubsan.so.1` linked). `test_zoneplate` (22 of 42537)
fails identically to `wu-44d-red`'s own baseline in every configuration —
this unit touched neither `test_zoneplate.cpp` nor anything it depends
on. **27 of 28 tests pass in every configuration, unchanged from
`wu-44d-red`'s own baseline — no change to the suite's pass/fail state,
since this unit found no work to do.**

## Flag for Steve, not resolved here

**Carried forward unchanged:** the `video::Raster444`-vs-`video::RasterRGB`
question and I7's non-achromatic round-trip breakage are both still open,
both still Steve's own call, neither touched this session, and neither
touched by any `WU-44` sub-unit.

## Where we are

`WU-44e` (`tests/test_lighting.cpp`, `test_coarse_shading.cpp`,
`test_morph.cpp`) is confirmed empty and closed out here — no fixture
change needed, verified against the full twelve-configuration matrix.
**`WU-44` as a whole (`WU-44a` through `WU-44e`) is now complete.** The
suite's build/test state is unchanged from `wu-44d-red`: 27 of 28 pass —
`test_zoneplate` remains the suite's own sole red test, outside every
`WU-44` sub-unit's own scope. "Green after every unit" (ADR-085 §5) does
not resume with `WU-44`'s own completion: that resumption is a
whole-project property, not a whole-phase one, and `test_zoneplate`'s own
I7 non-achromatic breakage — Steve's own call — still stands between the
suite and it.

## Next work unit

**None scoped in `WORK-UNITS.md` beyond `WU-44e`.** Phase 9 (`WU-38`
through `WU-44e`, the ADR-085 RGB-native migration) is now complete in
full. This session's own instruction was to stop at `WU-44e` and hand
off regardless of remaining budget, and that instruction was followed —
no `WU-45` or later unit was scoped, started, or even sketched this
session. What comes next is genuinely Steve's own call: the only thing
standing between the suite and "green after every unit" resuming is
`test_zoneplate.cpp`'s own I7 non-achromatic breakage, which needs the
`video::Raster444`-vs-`video::RasterRGB` question resolved first — no
work unit has been scoped for that, and none should be until Steve
decides which way that question goes.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision —
now the *only* thing keeping the suite from fully green, with `WU-44`
complete.

## Blocked / red

**`test_zoneplate` alone, 22 of 42537 checks — identical to `wu-44d-red`'s
own baseline in every configuration checked this session.** Every other
test (27 of 28) passes. `ctest` was not run on Steve's own real terminal
yet this session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. Fresh
`git clone` of the already-tagged `wu-44d-red` commit, confirmed identical
to Steve's own real repository before any build; no file was ever edited
in the sandbox this session (no fixture work found).

**`.git/index.lock` was absent from the real repository at this session's
own state-verification step, but was found present and genuinely
blocking by the time this session ran its own pre-close-out checks** —
the same recurring, already-documented Sessions-55-66 quirk, checked
directly rather than assumed either way: `ls -la .git/index.lock` at
close showed the file present (0 bytes, today's date), a follow-up `git
add --dry-run -A` failed outright with "Another git process seems to be
running" / "Unable to create... File exists" (genuinely blocking git in
the device-bridge shell), and the device-bridge shell could not remove it
itself (`rm -f .git/index.lock` there returns "Operation not permitted" —
the shell cannot unlink files under the mounted folder by default). Per
this project's own standing finding, this did **not** block this
session's own work: all building and testing happened in a fresh
cloud-sandbox clone, never in the device-bridge-mounted working tree, and
the changed-file writes below used the device bridge's own file-write
path, not `git`. The close-out block's own first line, `rm -f
.git/index.lock`, is expected to be genuinely needed on Steve's own real
terminal.

`mcp__remote-devices__device_stage_files` was not needed this session
(no file needed staging into the sandbox — the sandbox was a fresh clone
from `origin`, and no real-repository file needed reading into the
sandbox for comparison). File writes and re-reads this session used
`device_bash` directly (Python read-modify-write against `WORK-UNITS.md`,
full-file rewrite against this `HANDOFF.md`), confirmed by re-grepping
and re-reading the real files afterward, not inferred from the write
call succeeding alone.

## Append to DECISIONS.md

None this session. No new architectural decision was made — this unit
confirmed three test files carry no colour-relevant content and need no
change; it did not touch production behaviour or scope. (No new ADR is
proposed for anything found this session — nothing here rose to that
level.)

## Append to CORRECTIONS.md

None this session. No drift was found between the `WU-44e` stub and this
session's own re-grep/full-read — unlike `WU-44d`'s own
`test_field_pipeline.cpp` count drift, the stub's "likely empty"
prediction held exactly, so there is nothing to log as a correction.

## Closed out this session

**`WU-44e` (`tests/test_lighting.cpp`, `test_coarse_shading.cpp`,
`test_morph.cpp`): re-verified directly, not skipped on the stub's own
say-so. Confirmed empty — zero colour-relevant content, zero `core/`
header exposure beyond what that predicts, confirmed by both a direct
re-grep and a full read of all three files. No fixture change needed.**
Full twelve-configuration matrix run to confirm empirically: all three
files pass in all twelve configurations (239/305/150189 checks
respectively, tile-invariant), zero warnings, zero sanitizer traps, no
regression to `test_zoneplate` (still 22/42537, outside this unit's own
scope). **27 of 28 tests pass — unchanged from `wu-44d-red`. `WU-44`
(`WU-44a` through `WU-44e`) is now complete.** Not yet committed. Two
files (`WORK-UNITS.md`, this `HANDOFF.md`).

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show 27/28 pass, identical to `wu-44d-red`'s own last real-terminal
result: only `test_zoneplate` red**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: `test_decklink_device`'s own standing duplex-check exception
(unrelated, only present when built against the Blackmagic SDK on your
own Mac — this session's cloud sandbox has no SDK, so its own ctest total
there is 28, not 29), plus `test_zoneplate` (22 checks) failing exactly
as before. Every other test should pass — if anything differs from that,
stop and report exactly what, rather than assuming it is safe to tag
past, since this session's own cloud sandbox found the same result across
all twelve configurations checked.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly (`test_zoneplate`
alone still fails), and `close.sh` refuses to tag past any failure.

Once you've confirmed the build and that `ctest`'s only failure is
`test_zoneplate` (plus your own real Mac's `test_decklink_device` duplex
exception, if built with the SDK), close out with the **manual tag
path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add WORK-UNITS.md HANDOFF.md
git commit -m "WU-44e: tests/test_lighting.cpp, test_coarse_shading.cpp, test_morph.cpp confirmed empty (zero colour-relevant content, re-grepped and read in full, matching prior session's stub); no fixture change needed. WU-44 (WU-44a-WU-44e) now complete; test_zoneplate remains the suite's sole red test, outside WU-44's scope, see HANDOFF.md"
git tag -a wu-44e-red -m "WU-44e's own three files (tests/test_lighting.cpp, test_coarse_shading.cpp, test_morph.cpp) re-grepped directly (zero hits, matching the prior session's stub exactly) and read in full: all three operate on scalar double intensities (test_lighting/test_coarse_shading, via shade()/CoarseShadingGrid) or pure Vec3 geometry (test_morph, via morphLattice()/Lattice::jacobian()) -- no Frag/AccumCell/SourceRaster/colour-channel content anywhere, no core/ header beyond core/lighting.hpp, core/coarse_shading.hpp, core/lattice.hpp, core/shapes/shapes.hpp. Confirmed empty, no fixture change needed -- WU-44 (WU-44a through WU-44e) is now complete. Full twelve-configuration matrix (GCC 13.3.0/Clang 18.1.3, Release/Debug, tile 4/5, plus GCC+ASan-only and GCC+UBSan-only at both tile sizes) run to confirm empirically: 27 of 28 tests pass identically in every configuration, zero warnings, zero sanitizer traps, sanitizer instrumentation confirmed genuinely linked (nm -D/ldd). test_zoneplate (22/42537) remains the suite's sole red test, unchanged, outside every WU-44 sub-unit's own scope -- Steve's own call (video::Raster444-vs-video::RasterRGB, I7 non-achromatic breakage). Tag is -red, not -green: ADR-085 section 5's 'green after every unit' resumption is a whole-project property, not a per-unit or per-phase one, and test_zoneplate still stands between the suite and it."
git push origin main
git push origin --tags
```

**Tag name is `wu-44e-red`, not `wu-44e-green`** — this sub-unit's own
three files were confirmed empty, requiring no change, but the suite as a
whole is not green: `test_zoneplate` is still red, outside this unit's
own scope (and outside every `WU-44` sub-unit's own scope), and ADR-085
§5's "green after every unit" resumption is a whole-project property, not
a per-unit or per-phase one. With `WU-44` now complete, nothing further
is scoped to fix it — that is Steve's own next decision.

This exact list of 2 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 2 `M`/`??` lines (`WORK-UNITS.md`
modified, `HANDOFF.md` modified) and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
