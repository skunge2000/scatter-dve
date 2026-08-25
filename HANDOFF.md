# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 61 (`WU-44a` — Phase 9's sixth unit and the first of `WU-44`'s
own lettered split, RGB boundary conversion test coverage for
`tests/test_chroma.cpp`, ADR-085).

**Tag:** `wu-43-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin && git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both
`f7dab4c`, `git describe --tags --exact-match HEAD` — `wu-43-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 60's own work already committed,
tagged and pushed by Steve, exactly as its own HANDOFF.md account
expected. `.git/index.lock` was absent at the real first check (a
follow-up `ls -la .git/index.lock` inside the same verification command
failed with "No such file or directory", which is the expected, harmless
outcome, not a blocking condition — see "Environment check" below). This
session's own two-file change (`WORK-UNITS.md`, `tests/test_chroma.cpp`)
plus this file is not yet committed, tagged or pushed — that is Steve's
own next step, below.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly before assuming
it is absent, and see "Environment check" below before assuming a lock
you do find is harmless.

## This session in full

Opened with a real state-verification step, per this session's own
opening instruction, before reading anything else. Found `HEAD ==
origin/main == f7dab4c`, the expected WU-43 commit message, `wu-43-red`
dereferencing to that same commit, a clean tree, and no `.git/index.lock`.
State (a), genuinely confirmed, so the session proceeded.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 60's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033), and `WORK-UNITS.md`'s own WU-44 entry (the stub — not trusted
as a finished single-session scope, per this session's own opening
instruction) and WU-43's own full entry (just landed), all before
touching anything.

**`WU-44` split into lettered sub-units before any code was written, per
this session's own opening instruction — see `WORK-UNITS.md`'s own new
`WU-44` entry for the full method and reasoning, summarised here:**
`WU-44`'s own stub (written WU-38, before `WU-39`–`WU-43` existed for
real) named five candidate clusters and a "~21 of 35" file estimate that
this session's own real, repository-wide grep against `tests/*.cpp` —
followed by a per-file `#include "core/..."` check to separate genuine
`Frag`/`AccumCell`/PASS1-PASS2 involvement from a wire-domain false
positive — did not confirm. Real result: of the grep's candidate files,
fourteen need no `WU-44` work at all (seven are wire-domain-only, like
`test_v210.cpp`; two, `test_ewa.cpp`/`test_jacobian.cpp`, have zero
signal; three, `test_row_band.cpp`/`test_smoke.cpp`/
`test_coverage_capture.cpp`, were already fully resolved by `WU-39`'s own
entry), three more (`test_lighting.cpp`, `test_coarse_shading.cpp`,
`test_morph.cpp`, the stub's own "lighting/coarse-shading" cluster) show
zero signal and are provisionally empty (`WU-44e`, flagged for
re-verification rather than closed outright), and `test_zoneplate.cpp` is
deliberately left unassigned to any sub-unit — its own three red checks
are the already-flagged I7 non-achromatic breakage, Steve's own call, not
any work unit's. `test_pipeline_bytes.cpp` (also currently red) is
flagged, not confirmed, as possibly the same root cause — see
`WORK-UNITS.md`'s own `WU-44` entry. Split into `WU-44a` (this session,
below) through `WU-44e` (scoped, not started — `WU-44c` in particular
flagged as likely still too large as scoped and needing its own further
split before anyone starts it).

**`WU-44a` implemented this session — real scope re-derived directly, not
taken from the stub's "v210/chroma" label by assumption:**
`grep -rn 'ycbcrToRgb\|rgbToYcbcr' tests/ src/` before writing anything
found `chroma::ycbcrToRgbRow`/`rgbToYcbcrRow`/`ycbcrToRgbImage`/
`rgbToYcbcrImage` (`src/video/chroma.hpp`/`.cpp`, landed WU-40) — real
production code, called at every pipeline boundary crossing
(`src/core/pipeline.cpp`) since WU-40 — had **zero test coverage
anywhere in the repository.** That is this unit's real job: not a stale
fixture to re-derive (the rest of `test_chroma.cpp`, the resampling
filters, was already green, wire-domain, and untouched), but a coverage
gap in untested RGB-domain production code. `test_v210.cpp`,
`test_v210_neon.cpp`, `test_chroma_neon.cpp`, `test_ramp_roundtrip.cpp`
confirmed unaffected and left untouched (wire-domain, no `Frag`/
`AccumCell`/RGB-internal involvement — same reasoning `docs/
architecture.md`'s own §9 note already established).

Six new test functions added to `tests/test_chroma.cpp`, every expected
value hand-derived independently from chroma.hpp's own stated BT.601
formula (Kr=0.299, Kg=0.587, Kb=0.114), cross-checked numerically before
being written, never by calling the production function and trusting the
result (WU-34b/ADR-084's own "mirror the math independently" precedent,
ADR-085's own stated preference): achromatic identity (five points,
including the finding that Kr+Kg+Kb = 0.9999999999999999 in IEEE double —
not bit-exact 1.0 as the real-number formula would suggest — checked
directly rather than assumed, and confirmed not to disturb the exact
integer round trip at any point checked); one off-achromatic known vector,
forward and round-trip, no clamp engaged; a forward clamp on the low side
(`G`) and the high side (`B`); a reverse clamp (three pure-primary RGB
triples, two clamping, one not); and an Image-wrapper stride test built
only from already-verified-exact vectors, so it can only catch an
indexing bug, not raise a fresh arithmetic question. No bug found in
`chroma.cpp`'s own implementation — every hand-derived value matched the
production function's own output exactly, in every case checked.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed to confirm the write landed**
(`git diff --stat` against the real repository, staged and re-read after
the write, showed exactly the two files this account claims —
`tests/test_chroma.cpp`, `WORK-UNITS.md` — matching line counts) before
running anything.

## Build/test matrix — full twelve configurations (real source-code change)

**Unlike WU-43's deliberately scoped-down single-configuration check (a
documentation-only change), this unit changes a test file — a real
source-code change per SESSION-PROTOCOL.md's own build-configuration
section — so the full matrix applies again, matching WU-39–WU-42's own
practice.** Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git`
into the cloud sandbox, confirmed at `HEAD = wu-43-red = f7dab4c` before
any edit; this session's own change applied from the verified-written
real-repository copy (`git diff --stat` against the clean clone: exactly
the two files above, nothing else, before any build ran). GCC 13.3.0 and
Clang 18.1.3, same toolchain as prior sessions. Twelve configurations, not
assumed from this session's own first guess: `WORK-UNITS.md`'s own WU-42
entry was checked directly for what its "twelve rows" actually means
before building anything sanitizer-related — GCC and UBSan **alone**
(`-fsanitize=undefined`, no ASan) are two of the twelve rows, separate
from GCC and ASan **alone** (`-fsanitize=address`, no UBSan), each at both
tile sizes — four sanitizer configurations, not two combined
(`-fsanitize=address,undefined`) ones, matching WU-42's own explicit "in
either GCC + ASan or GCC + UBSan, at either tile size" wording once
re-read carefully rather than pattern-matched.

| Configuration | Build | `ctest` | `test_chroma` |
|---|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| GCC, Debug, tile 4 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| GCC, Release, tile 5 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| GCC, Debug, tile 5 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| Clang, Release, tile 4 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| Clang, Debug, tile 4 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| Clang, Release, tile 5 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| Clang, Debug, tile 5 | clean, no warnings | 25/28 pass | PASS, 21465 checks |
| GCC + ASan only, tile 4 | clean, no warnings | 25/28 pass, no sanitizer trap | PASS, 21465 checks |
| GCC + ASan only, tile 5 | clean, no warnings | 25/28 pass, no sanitizer trap | PASS, 21465 checks |
| GCC + UBSan only, tile 4 | clean, no warnings | 25/28 pass, no sanitizer trap | PASS, 21465 checks |
| GCC + UBSan only, tile 5 | clean, no warnings | 25/28 pass, no sanitizer trap | PASS, 21465 checks |

Twelve rows, matching WU-39–WU-42's own real count.

Per-test check counts on the three genuinely red tests, diffed directly
against Session 60's own stated baseline rather than assumed to carry
over (`CORRECTIONS.md` C-033's own general lesson) — identical in every
one of the ten configurations above:

- `test_binner`: 2 of 39139 checks fail at tile 5 (`test_binner.cpp:794`,
  `:795`), 2 of 10963 at tile 4 — matches Session 60's own baseline and
  C-033's own tile-4/tile-5 figures exactly.
- `test_zoneplate`: 22 of 42537 checks fail (`test_zoneplate.cpp:209`,
  `:212`, `:213`) — matches exactly, tile-invariant as before.
- `test_pipeline_bytes`: 3 of 42 checks fail (`test_pipeline_bytes.cpp:406`,
  `:452`, `:552`) — matches exactly.

Every other test (25 of 28) passes in every configuration, exactly as
Session 60 left it. No ASan or UBSan trap fired in any of the four
sanitizer configurations, on any test, checked directly (grepped every
test binary's own output for `AddressSanitizer`/
`UndefinedBehaviorSanitizer`/`runtime error:`, not inferred from ctest's
own pass/fail alone).

## Flag for Steve, not resolved here

**Carried forward unchanged from Sessions 56–60:** the `video::Raster444`-
vs-`video::RasterRGB` question and I7's non-achromatic round-trip breakage
are both still open, both still Steve's own call, neither touched this
session.

**New this session:** `test_pipeline_bytes.cpp`'s own three red checks
may be the same I7/non-achromatic root cause as `test_zoneplate.cpp`
rather than an ordinary stale `WU-44` fixture — both compare production
output against a reference over `testpat::makeZonePlate()` content (a
non-achromatic pattern), and both involve the RGB boundary conversion's
clamp somewhere in the chain. Not confirmed either way this session (that
would be real investigation into whether production and reference reach
the clamp by different paths, not scoping) — flagged in `WORK-UNITS.md`'s
own `WU-44d` entry for whoever starts it to check directly before
assuming their own fixture work can turn this file green.

## Where we are

`WU-44a` (`tests/test_chroma.cpp` RGB boundary conversion coverage) is
built, verified against the full twelve-configuration matrix, and closed
out here. `WU-44` as a whole is split into `WU-44a` (done) through
`WU-44e` (scoped, not started — see `WORK-UNITS.md`). The suite's
build/test state is unchanged from `wu-43-red`: still genuinely red,
`test_binner`/`test_zoneplate`/`test_pipeline_bytes`, none of which this
unit's own scope could affect (it added test coverage for previously-
untested functions; it touched no file any of the three red tests
depends on).

## Next work unit

**Do not start `WU-44b` this session — this session's own instruction was
to stop at one `WU-44` cluster and hand off, even with budget left, and
that instruction was followed.** Whoever starts next: `WU-44b`
(`test_binner.cpp`/`test_splat.cpp`/`test_scan_order_invariance.cpp`) is
the sub-unit most likely to turn a currently-red test
(`test_binner`, via its own shading-mirror fixture at `test_binner.cpp:794`/
`:795`) fully green — but re-grep first per `WORK-UNITS.md`'s own `WU-44b`
entry, this session's own file list may already be stale by the time
someone starts it. `WU-44c` is flagged as likely needing its own further
split before anyone starts it (five files, two of them substantial).

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both carried forward, for Steve's own decision.
`test_pipeline_bytes.cpp`'s own possible link to the same root cause,
flagged above, is also not this session's to resolve.

## Blocked / red

**Red, genuinely and expectedly, unchanged in substance from Session 60
— see Build/test matrix above.** `test_binner`, `test_zoneplate` and
`test_pipeline_bytes` fail identically to Session 60's own baseline (same
lines, same counts, in all ten configurations checked this session).
Every other test (25 of 28, including `test_chroma` with its own new
checks) passes. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions. This
session used a genuinely fresh `git clone` of the already-tagged
`wu-43-red` commit, confirmed identical to Steve's own real repository
before any edit, with this session's own changed file
(`tests/test_chroma.cpp` — the only file the build/test matrix above
actually depends on; `WORK-UNITS.md` and this file are documentation,
copied in afterward and not part of what was built) copied in from the
verified-written real-repository copy — `git diff --stat` against the
clean clone confirmed exactly that one file changed, nothing else, before
any build ran.

**A stray `.git/index.lock` did appear this session, the same
self-inflicted, already-documented pattern Sessions 55–60 logged.** This
session's own opening state-verification found the repository genuinely
clean (`ls -la .git/index.lock` — "No such file or directory"). This
session's own *closing* `git status --short` (run through the device
bridge, immediately before the close-out block below was written) left a
fresh, empty `.git/index.lock` behind — the device-bridge shell cannot
delete files by default, so git's own internal cleanup unlink silently
failed, exactly as documented. Confirmed directly that this one actually
blocks git, not assumed harmless: `git add --dry-run -A` failed with
"Another git process seems to be running" / "Unable to create... File
exists." The close-out block below makes `rm -f .git/index.lock` its own
first line, and this time it is not just precautionary — it is expected
to be genuinely needed.

## Append to DECISIONS.md

None this session. `WU-44a` implements ADR-085's own already-accepted
scope (closing a test-coverage gap the boundary conversion's own WU-40
landing left open); no new architectural decision was made. The
`Raster444`-vs-`RasterRGB` question and the `test_pipeline_bytes.cpp`
flag above are real but not this session's decision to make or pre-empt
with a new ADR.

## Append to CORRECTIONS.md

None this session. This session verified rather than assumed one
arithmetic claim of its own before writing it down (Kr+Kg+Kb's own exact
value in IEEE double, see "This session in full" above) — that is this
session's own new work checked before being trusted, not a correction to
an earlier session's claim, so it is documented in `WORK-UNITS.md`'s own
`WU-44a` entry rather than logged here as a correction.

## Closed out this session

**`WU-44` split into `WU-44a`–`WU-44e` in `WORK-UNITS.md`, with real,
re-derived file lists per sub-unit (see that file for the full method and
per-cluster reasoning) — `WU-44a` (`tests/test_chroma.cpp`: six new test
functions covering the previously-untested RGB boundary conversion
functions, `ycbcrToRgbRow`/`rgbToYcbcrRow`/`ycbcrToRgbImage`/
`rgbToYcbcrImage`) built and verified against the full twelve-
configuration matrix, identical red state to `wu-43-red` (same three
tests, same lines, same counts), no new warning, no new sanitizer trap.
Not yet committed.** Two files (`tests/test_chroma.cpp`,
`WORK-UNITS.md`), this `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **still
expected to be red, exactly as `wu-43-red` was, not newly green**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect the same failures `wu-43-red` already had: `test_decklink_device`'s
own duplex check (standing PSU exception, unrelated), plus `test_binner`
(2 checks), `test_zoneplate` (22 checks), `test_pipeline_bytes`
(3 checks). `test_chroma` itself should now report a higher check count
than before (21465, up from whatever your own last local build reported)
and still PASS — if it fails, stop and report exactly what failed rather
than assuming it is safe to tag past, since this session's own cloud
sandbox found it green in all twelve configurations checked.

The `rm -f .git/index.lock` above is not just a precaution this time: this
session's own final `git status --short` (run through the device bridge,
immediately before this block was written) left a stray, empty
`.git/index.lock` behind — the same self-inflicted, already-documented
Sessions-55–60 pattern (the device-bridge shell cannot unlink files by
default, so git's own internal cleanup unlink silently fails). Confirmed
directly, not assumed, that this one actually blocks git: `git add
--dry-run -A` failed with "Another git process seems to be running" /
"Unable to create... File exists." Your own real terminal has no such
restriction, so `rm -f .git/index.lock` removes it there without issue —
but it must run before `git add` below, not be skipped.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly (by design, the
standing Phase 9 exception), and `close.sh` refuses to tag past any
failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the ones named above, close out with the **manual tag path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add tests/test_chroma.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-44a: tests/test_chroma.cpp RGB boundary conversion coverage; WU-44 split into WU-44a-e (ADR-085); red, see HANDOFF.md"
git tag -a wu-44a-red -m "RGB boundary conversion (ycbcrToRgbRow/rgbToYcbcrRow/Image) test coverage added, six hand-derived test functions, zero prior coverage closed; WU-44 split into lettered sub-units, real file lists re-derived; build/test state unchanged from wu-43-red (test_binner/test_zoneplate/test_pipeline_bytes still red, none in this unit's own scope), see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-44a-red`, not `wu-44a-green`** — this unit closes a
test-coverage gap in already-correct production code; it does not touch,
and cannot have fixed, any of the three genuinely red tests. `WU-44b`
(most likely `test_binner`) through `WU-44e` remain, and Phase 9's own
"green after every unit" resumption still waits on the last of them.

This exact list of 3 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 3 `M`/`??` lines (`tests/test_chroma.cpp`
modified, `WORK-UNITS.md` modified, `HANDOFF.md` modified) and nothing
else. Still worth a quick `git status --short` yourself before pasting
this block, since time has passed since that check.
