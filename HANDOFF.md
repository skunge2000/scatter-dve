# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 66 (`WU-44d` — Phase 9's eleventh unit: `tests/test_pipeline_bytes.cpp`,
`test_threading.cpp`, `test_persistent_pool.cpp`, `test_field_pipeline.cpp`,
full-pipeline integration fixtures, ADR-085).

**Tag:** `wu-44c3-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both `9c3a1ba`,
`git describe --tags --exact-match HEAD` — `wu-44c3-red`, dereferencing to
that same commit). `git status --short` read empty at session start —
clean tree, Session 65's own work already committed, tagged and pushed by
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
genuinely absent at this session's own opening check, unlike most
sessions since Session 55, but that is not a reason to skip checking.

## This session in full

Opened with a real state-verification step before reading anything else:
`HEAD == origin/main == 9c3a1ba`, the expected `WU-44c3` commit message,
`wu-44c3-red` dereferencing to that same commit, a clean tree, no
`.git/index.lock`. State (a), genuinely confirmed.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 65's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own `WU-44` entry (split rationale,
including its own flag that `test_pipeline_bytes.cpp`'s three failing
checks may share `test_zoneplate.cpp`'s own I7/non-achromatic root
cause), the `WU-44c` entry, and `WU-44c1`/`WU-44c2`/`WU-44c3`'s own full
entries, and `WU-44d`'s own stub under Phase 9, all before touching
anything.

**`WU-44d`'s own scope re-grepped directly this session, per this
project's own standing discipline (C-027), rather than trusted from the
stub:** matched the stub's own counts exactly for `test_pipeline_bytes.cpp`
(15), `test_threading.cpp` (7); `test_persistent_pool.cpp` came out 15
here vs. the stub's 14 (a `-oE` occurrence-vs-line bucketing difference,
one line has two `Raster444` hits — not a sign of drift).
`test_field_pipeline.cpp` came out genuinely different — 27 here vs. the
stub's 30, and not just a count difference: the stub's own `Background`=1
and `YCbCr`=6 do not exist in the file any more, and the stub never
counted the file's own real `CompositedCell`=1 and three `.R`/`.G`/`.B`
hits. Root-caused: `git log --oneline -- tests/test_field_pipeline.cpp`
shows `WU-42` (`1297c19`) touched this file — the stub was written before
that rename reached it. `src/core/resolve.hpp`, `src/core/resolve.cpp`
and `src/core/pipeline.cpp` all read in full this session (`pipeline.cpp`
specifically, not read by any prior `WU-44c` sub-unit) — nothing has
moved since `WU-42` in any of the three. See `WORK-UNITS.md`'s own
`WU-44d` entry for the full per-file breakdown; this drift did not go to
`CORRECTIONS.md` (nothing shipped or misled a real decision on it, only a
scoping stub's own count).

**Split judgment: four files confirmed to genuinely belong in one unit,
against real content, not assumed from the stub's own grouping.**
`test_threading.cpp` and `test_persistent_pool.cpp` never derive an
independent colour value — both check bit-identity across thread
counts/pool configurations against a same-call `threads == 1` reference,
structurally incapable of going stale under the ADR-085 rename (the same
class `WU-44c1`'s own Part C already found for `test_kbuffer_storage.cpp`).
`test_field_pipeline.cpp` likewise never hand-derives a colour: an exact
bit-for-bit identity round-trip, plus a cross-check against production's
own already-tested `composite()`/`splatTile()`/`sumBanks()` primitives
directly. Only `test_pipeline_bytes.cpp` carried real risk. Three of the
four files were cheap, structurally-safe confirmations; one needed the
real investigation this unit's own brief asked for — retrospectively
justifying the four-file grouping, not merely permitting it.

**This unit's own real job — investigate, don't just confirm, per
`WU-44`'s own explicit instruction for this file:** traced whether
`test_pipeline_bytes.cpp`'s three failing checks (`:406`, `:452`, `:552`)
share `test_zoneplate.cpp`'s own I7/non-achromatic root cause, rather than
assumed either way. **Confirmed a different, fixable stale-fixture
cause, not the same root cause.** All three failing checks compare
production `runFrameBytesDeinterlaced()` (`core/pipeline.cpp`, read in
full) against this file's own two hand-written reference functions
(`referenceRunFrameBytesDeinterlaced()`, `referenceWithExplicitReinterlace()`),
never a hand-derived colour value. Production's real function calls
`chroma::ycbcrToRgbImage()`/`rgbToYcbcrImage()` (WU-40/WU-41's own RGB
boundary conversion) either side of `runFrame()`; this file's own two
reference functions — written before WU-40 existed — call neither,
feeding YCbCr planes straight in as if they were RGB. Since this test's
own content (`testpat::makeZonePlate()`, flat chroma) is achromatic
throughout, the RGB clamp that actually causes I7's own breakage never
engages here — the real divergence is structural: skipping the
conversion means the reference composites its uncovered/partial cells
against `kBlack` directly instead of against achromatic-RGB-black
converted back to genuine `kChromaZero`-centred YCbCr, two numerically
distinct codes. A genuine, ordinary stale fixture, fixed the normal way:
both reference functions gained the same two conversion calls
production's own function makes, hand-matched against `core/pipeline.cpp`'s
real body line for line, not copied blind. See `WORK-UNITS.md`'s own
`WU-44d` entry for the full derivation.

**Fixed, one file:** `tests/test_pipeline_bytes.cpp` (96 insertions, 20
deletions — well inside `SESSION-PROTOCOL.md`'s own sizing rule). No
`src/` file touched — `core/pipeline.cpp` was already correct, confirmed
by tracing it. `test_pipeline_bytes` is now fully green, 42 of 42 checks,
in every one of this session's own twelve configurations (was 39 of 42
before this fix).

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, expected to show exactly the three files this account claims
— `tests/test_pipeline_bytes.cpp`, `WORK-UNITS.md`, this `HANDOFF.md`)
before running anything further this session.

## Build/test matrix — full twelve configurations (run to confirm, not assumed)

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox, confirmed at `HEAD = wu-44c3-red = 9c3a1ba` before any
build; only `tests/test_pipeline_bytes.cpp` was ever edited there —
`git status --short` in the sandbox read exactly that one modified file
throughout. GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 4 and
tile 5 (eight), plus GCC + ASan alone and GCC + UBSan alone, each at both
tile sizes (four more) — twelve total, not two combined
`-fsanitize=address,undefined` builds.

| Configuration | Build | `ctest` | `test_pipeline_bytes` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| GCC, Debug, tile 4 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| GCC, Release, tile 5 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| GCC, Debug, tile 5 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| Clang, Release, tile 4 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| Clang, Debug, tile 4 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| Clang, Release, tile 5 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| Clang, Debug, tile 5 | clean, no warnings | 27/28 pass | PASS, 42 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, 42 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, 42 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, 42 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 27/28 pass, no sanitizer trap | PASS, 42 checks |

Twelve rows, zero real warnings (the same harmless configure-time
`CMAKE_C_COMPILER` warning every prior `WU-44` sub-unit's own matrix
already reports), zero sanitizer traps — grepped every `ctest
--output-on-failure` log for `AddressSanitizer`/`UndefinedBehaviorSanitizer`/
`runtime error:` (zero hits) — and confirmed via `nm -D` (the dynamic
symbol table, not `nm`'s default static view, which shows nothing for a
dynamically-linked PIE executable's own sanitizer interceptors — a
wrinkle this session found and worked around) that the ASan/UBSan
binaries genuinely carry sanitizer instrumentation (28 and 14
case-insensitive `asan`/`ubsan` dynamic-symbol hits respectively, `ldd`
confirming `libasan.so.8`/`libubsan.so.1` actually linked). `test_threading`
(1497677 checks), `test_persistent_pool` (2562778 checks) and
`test_field_pipeline` (27654 checks) are all genuinely tile-invariant and
fully green in every configuration, unchanged by this unit (confirmed,
not merely left alone by omission). `test_zoneplate` (22 of 42537) fails
identically to `wu-44c3-red`'s own baseline in every configuration — this
unit touched neither `test_zoneplate.cpp` nor anything it depends on.
**27 of 28 tests pass in every configuration — one more than
`wu-44c3-red`'s own 26/28: `test_pipeline_bytes` moves from failing to
passing, `test_zoneplate` remains the suite's own sole red test.**

## Flag for Steve, not resolved here

**Carried forward unchanged:** the `video::Raster444`-vs-`video::RasterRGB`
question and I7's non-achromatic round-trip breakage are both still open,
both still Steve's own call, neither touched this session.
`test_pipeline_bytes.cpp`'s own possible link to that root cause — the
one thing `WU-44`'s own top-level entry asked this unit to check
directly — is now resolved: **not the same root cause.** Its own three
failing checks were a genuine, ordinary stale fixture (two reference
functions missing the RGB boundary conversion), now fixed; `test_zoneplate.cpp`'s
own I7 breakage is untouched, unrelated, and still exactly Steve's own
call.

## Where we are

`WU-44d` (`tests/test_pipeline_bytes.cpp`, `test_threading.cpp`,
`test_persistent_pool.cpp`, `test_field_pipeline.cpp`) is built, verified
against the full twelve-configuration matrix, and closed out here.
`test_pipeline_bytes` moves from red to green — a genuine fixture fix,
not a confirmation, unlike every `WU-44c` sub-unit. `test_threading`,
`test_persistent_pool` and `test_field_pipeline` were all already
correct, confirmed rather than assumed. `WU-44` as a whole is now
`WU-44a` (done) + `WU-44b` (done) + `WU-44c` (done, all three sub-units)
+ `WU-44d` (done, this session) through `WU-44e` (scoped, not started —
see `WORK-UNITS.md`). The suite's build/test state: 27 of 28 pass —
`test_zoneplate` is now the *only* test still genuinely red, neither in
`WU-44d`'s own scope nor `WU-44e`'s own (found empty, per its own stub).
"Green after every unit" (ADR-085 §5) still has not resumed — one test
away.

## Next work unit

**Do not start `WU-44e` this session — this session's own instruction was
to stop at `WU-44d` and hand off, even with budget left, and that
instruction was followed.** Whoever starts next: `WU-44e`
(`tests/test_lighting.cpp`, `test_coarse_shading.cpp`, `test_morph.cpp`)
is scoped in `WORK-UNITS.md`, flagged there as likely empty (this
project's own standing discipline — C-027 — still means it should be
re-verified directly, not skipped on the stub's own say-so). If `WU-44e`
confirms empty and closes with no fixture change, `WU-44` as a whole is
complete and only `test_zoneplate.cpp`'s own I7 non-achromatic breakage —
Steve's own call, the `video::Raster444`-vs-`video::RasterRGB` question —
stands between the suite and "green after every unit" resuming for real.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision —
now the only thing keeping the suite from fully green, once `WU-44e`
closes. `test_pipeline_bytes.cpp`'s own possible link to that root cause
is no longer open — resolved this session as a different, unrelated,
already-fixed stale fixture.

## Blocked / red

**`test_zoneplate` alone, 22 of 42537 checks — identical to `wu-44c3-red`'s
own baseline in every configuration checked this session.** Every other
test (27 of 28) passes, including `test_pipeline_bytes`, newly green this
session. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. Fresh
`git clone` of the already-tagged `wu-44c3-red` commit, confirmed
identical to Steve's own real repository before any build; only
`tests/test_pipeline_bytes.cpp` was ever edited in the sandbox.

**No `.git/index.lock` was present in the real repository at this
session's own state-verification step** — checked directly (`ls -la
.git/index.lock`: no such file), unlike most sessions since Session 55.
Not assumed absent for the rest of the session on that basis alone: a
later `git add --dry-run -A` run during this session's own pre-close-out
sweep (checking for anything this unit's change might have made
inconsistent elsewhere) created exactly the same self-inflicted,
already-documented Sessions-55–64 lock — checked directly, not assumed:
`ls -la .git/index.lock` afterward showed the file present (0 bytes,
today's date), and a follow-up `git add --dry-run -A` failed outright
with "Another git process seems to be running" / "Unable to create...
File exists," confirming it genuinely blocks git in the device-bridge
shell (the device-bridge shell cannot unlink files by default, so git's
own internal cleanup silently fails, the same mechanism every recent
session's own account already gives). Per this project's own standing
finding, this did **not** block this session's own work — all building
and testing happened in a fresh cloud-sandbox clone, never in the
device-bridge-mounted working tree, and the changed-file writes below
used the device bridge's own file-write path, not `git`. The close-out
block's own first line, `rm -f .git/index.lock`, is expected to be
genuinely needed on Steve's own real terminal.

`mcp__remote-devices__device_stage_files` was tested at this session's
own start (staging the six state files, then again for the four
`WU-44d` test files, `src/core/resolve.hpp`/`.cpp`/`pipeline.cpp`, and
`src/video/raster.hpp`/`chroma.hpp`) and worked normally throughout — no
`HTTP 403 untrusted_device` recurrence this session.

## Append to DECISIONS.md

None this session. No new architectural decision was made — this unit
fixed a stale test fixture, it did not change any production behaviour
or scope, and it deliberately did not touch the `video::Raster444`-vs-
`video::RasterRGB` question.

## Append to CORRECTIONS.md

None this session. The `WU-44d` stub's own drifted `test_field_pipeline.cpp`
count (see "This session in full" above) is logged in `WORK-UNITS.md`'s
own `WU-44d` entry, matching `WU-44c`'s split entry's own precedent for
small stub-vs-real-grep drift — not escalated to `CORRECTIONS.md` since
nothing shipped or misled a real decision on it, only mis-scoped a
not-yet-started stub.

## Closed out this session

**`WU-44d` (`tests/test_pipeline_bytes.cpp`, `test_threading.cpp`,
`test_persistent_pool.cpp`, `test_field_pipeline.cpp`): investigated, not
merely confirmed, per this unit's own explicit brief.
`test_pipeline_bytes.cpp`'s three failing checks traced and confirmed to
be a genuine, ordinary stale fixture (two reference functions missing
WU-40/WU-41's own RGB boundary conversion) — unrelated to
`test_zoneplate.cpp`'s I7 non-achromatic root cause — and fixed the
normal way, hand-matched against `core/pipeline.cpp`'s real body. The
other three files read in full and confirmed already correct, no
staleness possible by construction. Full twelve-configuration matrix run
to confirm empirically: `test_pipeline_bytes` newly green (42/42 checks)
in all twelve, the other three files' own already-passing counts
unchanged and tile-invariant, zero warnings, zero sanitizer traps, no
regression to `test_zoneplate` (still 22/42537, outside this unit's own
scope). **27 of 28 tests pass — `test_zoneplate` is now the suite's own
sole remaining red test.** Not yet committed.** Three files
(`tests/test_pipeline_bytes.cpp`, `WORK-UNITS.md`, this `HANDOFF.md`).

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **expected
to show 27/28 pass, one more than `wu-44c3-red`'s own 26/28:
`test_pipeline_bytes` newly passing, only `test_zoneplate` still red**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect: `test_decklink_device`'s own standing duplex-check exception
(unrelated, only present when built against the Blackmagic SDK on your
own Mac — this session's cloud sandbox has no SDK, so its own ctest
total there is 28, not 29), plus `test_zoneplate` (22 checks) failing
exactly as before. Every other test, including `test_pipeline_bytes`
(now genuinely fixed, not merely re-confirmed), should pass — if
anything differs from that, stop and report exactly what, rather than
assuming it is safe to tag past, since this session's own cloud sandbox
found the same result across all twelve configurations checked.

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
git add tests/test_pipeline_bytes.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-44d: tests/test_pipeline_bytes.cpp fixture fixed (RGB boundary conversion missing from two reference functions, unrelated to I7); test_threading.cpp/test_persistent_pool.cpp/test_field_pipeline.cpp confirmed already correct, no fixture change needed; test_zoneplate now the suite's sole remaining red test, see HANDOFF.md"
git tag -a wu-44d-red -m "tests/test_pipeline_bytes.cpp's own three failing checks traced and fixed: two hand-written reference functions were missing WU-40/WU-41's own RGB boundary conversion (chroma::ycbcrToRgbImage/rgbToYcbcrImage), confirmed unrelated to test_zoneplate.cpp's I7 non-achromatic root cause (this test's own content is achromatic throughout); now 42/42 checks, tile-invariant, in all twelve configurations. test_threading.cpp/test_persistent_pool.cpp/test_field_pipeline.cpp read in full and confirmed already correct, no staleness possible by construction (self-consistency/independent-recomputation checks only, no hand-derived colour value). 27 of 28 tests pass; test_zoneplate (22/42537) is now the suite's own sole remaining red test, Steve's own call (video::Raster444-vs-video::RasterRGB, I7 non-achromatic breakage), outside WU-44's own scope. WU-44e (likely empty) remains, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44d-red`, not `wu-44d-green`** — this sub-unit's own
four files are fully green (one newly fixed, three confirmed), but the
suite as a whole is not: `test_zoneplate` is still red, outside this
unit's own scope, and ADR-085 §5's "green after every unit" resumption
is a whole-`WU-44`-phase property, not a per-unit one. `WU-44e` remains.

This exact list of 3 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 3 `M`/`??` lines
(`tests/test_pipeline_bytes.cpp` modified, `WORK-UNITS.md` modified,
`HANDOFF.md` modified) and nothing else. Still worth a quick `git status
--short` yourself before pasting this block, since time has passed since
that check.
