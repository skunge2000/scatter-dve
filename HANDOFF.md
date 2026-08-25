# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 60 (`WU-43` — Phase 9's fifth unit, `docs/architecture.md`
rewritten for RGB-native colour, ADR-085).

**Tag:** `wu-42-red` was the newest real tag at this session's own start
(confirmed directly: `git fetch origin && git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both
`1297c19`, `git describe --tags --exact-match HEAD` — `wu-42-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 59's own work already committed,
tagged and pushed by Steve, exactly as its own HANDOFF.md account
expected. This session's own three-file change (`docs/architecture.md`,
`WORK-UNITS.md`, this file) is not yet committed, tagged or pushed —
that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly (`ls -la
.git/index.lock`) before assuming it is absent, and see "Environment
check" below before assuming a lock you do find is harmless — this
session's own opening verification found the same self-inflicted,
already-documented pattern Sessions 55–59 already logged (a lock left
behind by the device-bridge shell's own inability to delete files, not a
repository problem), confirmed by directly testing whether it blocked a
write-requiring git command rather than assumed either way.

## This session in full

Opened with a real state-verification step, per this session's own
opening instruction: `git fetch origin`, `git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main`,
`git describe --tags --exact-match HEAD`, `git status --short` and
`ls -la .git/index.lock`, all run directly against the real repository via
the device bridge before reading anything else. Found `HEAD == origin/main
== 1297c19`, the expected WU-42 commit message, `wu-42-red` dereferencing
to that same commit, a clean tree, and — at that first check — no
`.git/index.lock`. State (a), genuinely confirmed, so the session
proceeded. (A subsequent diagnostic *did* leave a stray lock behind later
in the session — see "Environment check" below; it did not affect
anything, since this unit never needed a write-requiring git command in
the device-bridge shell at all.)

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 59's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-032
and C-033, read in the order they appear), and `WORK-UNITS.md`'s own
WU-43 entry (the stub, not trusted as a finished scope) and WU-42's own
full entry (just landed, read to know what actually shipped), all before
touching anything, per this session's own opening instruction.

**Re-derived the real current scope directly against the code and the
current document, not from WU-43's own stub:** the stub named only
"Design Invariants table (§2) and signal-path diagram (§3)" — written
before `WU-39`–`WU-42` landed for real, and not itself the product of a
grep against the current document. This session read `docs/architecture.md`
in full, then `grep -n 'YCbCr\|Y/Cb/Cr\|\bCb\b\|\bCr\b' docs/architecture.md`
to find every line that might need updating rather than assuming §2/§3
were the whole scope — five real hits: §3's diagram, §4.3's `Frag` struct,
two chroma-filter bullets in §5, and §9's ramp-test row. Read
`src/core/types.hpp`, `src/core/binner.hpp`/`.cpp`, `src/core/resolve.hpp`,
`src/core/pipeline.cpp`, `src/video/chroma.hpp` and `src/video/raster.hpp`
in full to confirm the real, current shape of the already-landed pipeline
directly against the code — not against this file's or `WORK-UNITS.md`'s
own prose summaries of it, which are accounts, not ground truth — before
writing anything.

**Repository-wide grep before writing, not just the two named sections
(C-028/C-029's own standard, applied here to documentation instead of
code):** `grep -rln 'YCbCr\|Y/Cb/Cr' --include="*.md" --include="*.hpp"
--include="*.cpp"` across the whole repository, excluding the five
state files (which name the pre-ADR-085 design deliberately, as history)
and `docs/sources/`/`docs/proposals/`/`docs/wu-audit-2026-08.md` (source
material and a past audit, not live documentation). Every remaining hit
is either a genuine, ADR-085-unaffected `Raster422`/`Raster444`/v210/
chroma-boundary reference (confirmed by tracing each variable's own
declared type, the same discipline `WU-39`/`WU-41`/`WU-42` already used
for the analogous risk in their own renames) or `tools/
coverage_view_demo.cpp`'s own already-known staleness: `WU-41`'s own
`WORK-UNITS.md` entry already updated that file's field names to compile
after `SourceRaster`'s rename, explicitly without re-deriving its fixture
*values* (`WU-44`'s own job) — not a new finding, not re-logged as a
correction here, and out of scope for a docs-only unit to touch even if
it were new (it lives in `tools/`, not `docs/`).

**What changed, `docs/architecture.md` only (see `WORK-UNITS.md`'s own
WU-43 entry for the itemised list against each of the five grep hits and
each table row):** §1's "4:4:4 or RGB I/O" out-of-scope line gained one
clause making explicit that it describes the wire transport, not the
(now RGB) internal representation. §2's Design Invariants table: row 3
(I3) rewritten to the RGB-native text, matching `INVARIANTS.md`'s own I3
in substance; row 4 (I4) gained one sentence noting the bound was
re-derived under ADR-085 and is channel-agnostic, matching
`INVARIANTS.md`'s own I4 addition; rows 1, 2, 5, 6, 7 untouched
(colour-space-agnostic, per ADR-085's own "What does not change"). §3's
signal-path diagram gained two new stages (RGB boundary conversion,
input side after chroma upsample/de-interlace and output side before
chroma downsample), matching `core/pipeline.cpp`'s real call sequence
exactly, plus a new paragraph stating plainly that `video::Raster444`
(`runFrame()`'s own `dest`) carries genuine RGB content in permanently
YCbCr-named planes between PASS 2 and the second boundary conversion —
this project's own already-flagged `Raster444`-vs-`RasterRGB` open
question (Session 59) is named here as a fact about the diagram, not
resolved; the note states what is real today and points to this file for
the open decision. §4.3's `Frag` struct: `Y, Cb, Cr` → `R, G, B`, matching
`core/types.hpp` exactly. §5 gained one clarifying paragraph (the
4:2:2↔4:4:4 resampling is a geometry argument, unaffected by which three
channels are being warped — existing bullets left untouched, still
accurate) and a new "RGB boundary conversion (ADR-085)" subsection
describing `video/chroma.hpp`'s `ycbcrToRgbImage()`/`rgbToYcbcrImage()`:
hardcoded BT.601 coefficients, the quantisation clamp to `Sample`'s own
[0, 65535] range (explicitly distinguished from I2's v210-protocol
clamp), and — stated as an observed fact, not resolved as settled either
way — that a non-achromatic flat field is not guaranteed to round-trip
bit-exact through the full pipeline any more, pointing here and to
`CORRECTIONS.md` for current status. §9's ramp-test row (`Y, Cb, Cr`
wording): confirmed, not edited — that row describes the v210
*wire*-domain ramp, unaffected by ADR-085; changing it to match §3/§4.3's
internal-representation renames would have been a wrong edit born of
pattern-matching the grep hit rather than reading what the row actually
describes.

**Not touched, deliberately:** `INVARIANTS.md` (standing rule — concerns
flagged below, not edited). `DECISIONS.md` ADR-085 (standing rule — not
reopened; no new ADR proposed, none needed for this unit's own work).
§8 Module layout (confirmed via the grep above to contain no relevant
hits — a separate, pre-existing, ADR-085-unrelated gap noticed in passing
is flagged below, not fixed). `tools/coverage_view_demo.cpp` (out of
scope for a docs-only unit regardless of its own known staleness). No
source file (`src/`, `tests/`) touched anywhere this session — confirmed
directly, not assumed, by the verification below.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and diffed/grepped to confirm the write
landed** (`git diff --stat docs/architecture.md` against the real
repository read exactly one file, 87 insertions/8 deletions, matching
what was intended; re-staged the written copy and grepped it for every
new string this session's account above claims was added — `RGB boundary
conversion`, `Frag::R/G/B`, `RasterRGB`, `ADR-085` — all present at the
expected locations) before running anything.

## Build/test matrix — one representative configuration, scoped down
deliberately for a docs-only change

**Not the full twelve-configuration matrix `WU-39`–`WU-42` each ran: a
documentation-only change touching no source file has no compiler,
sanitizer or tile-size axis for its own edit to vary across, so running
all twelve would re-verify nothing this session's own change could
plausibly have affected.** One representative build instead: fresh
`git clone` of `https://github.com/skunge2000/scatter-dve.git` into the
cloud sandbox, confirmed at `HEAD = wu-42-red = 1297c19` before any edit,
then this session's own single changed file (`docs/architecture.md`)
copied in from the verified-written real-repository copy (`git diff
--stat` against the clean clone: exactly that one file, nothing else).
Same GCC 13.3.0 toolchain as prior sessions. GCC, Release,
`SCATTER_TILE_LOG2=5` (this project's own settled default, ADR-045) —
configured, built and `ctest`-run for real.

| Configuration | Build | `ctest` |
|---|---|---|
| GCC, Release, tile 5 | clean, no warnings | 25/28 pass — `test_binner`, `test_zoneplate`, `test_pipeline_bytes` fail |

Per-test check counts, diffed directly against Session 59's own stated
baseline rather than assumed to carry over (`CORRECTIONS.md` C-033's own
general lesson):

- `test_binner`: 2 of 39139 checks fail (`test_binner.cpp:794`, `:795`) —
  matches Session 59's own tile-5 baseline exactly.
- `test_zoneplate`: 22 of 42537 checks fail (`test_zoneplate.cpp:209`,
  `:212`, `:213`) — matches Session 59's own baseline exactly.
- `test_pipeline_bytes`: 3 of 42 checks fail (`test_pipeline_bytes.cpp:406`,
  `:452`, `:552`) — matches Session 59's own baseline exactly.

Every other test (25 of 28) passes, exactly as Session 59 left it — this
unit's own documentation-only change changed nothing about the build/test
outcome, confirmed rather than assumed.

## Flag for Steve, not resolved here

**Carried forward unchanged from Sessions 56–59:** the `video::Raster444`-
vs-`video::RasterRGB` question (whether `runFrame()`'s own output side
should eventually be given a genuinely RGB-named container instead of
continuing to borrow `Raster444`, YCbCr-named, for RGB content) is still
open. This session named the underlying fact plainly in
`docs/architecture.md` itself, for the first time — not just in this
file — but did not decide it, per this project's own standing instruction
not to resolve it here.

**Carried forward unchanged from Sessions 56–59:** I7 ("Input v210 equals
output v210, byte for byte, illegal excursions included") is not exactly
true post-ADR-085 for non-achromatic flat content — the same
`test_zoneplate.cpp` breakage counted as red above. `INVARIANTS.md`'s own
I7 wording has still not been revisited to say so explicitly — flagged
again, not touched. `docs/architecture.md`'s own new "RGB boundary
conversion" section (§5) now states the underlying fact (the RGB
boundary's clamp can clip a non-achromatic YCbCr triple's implied RGB)
without asserting whether or how I7's wording should change — the same
restraint applied to the `Raster444`/`RasterRGB` question above.

**New this session, minor, unrelated to ADR-085:** `docs/architecture.md`
§2's Design Invariants table still lists only I1–I7; `INVARIANTS.md`
itself has grown to I1–I11 (I8 back-face splat, I9 no stored normal/
depth, I10 pre-projection shading, I11 within/between-sheet resolve)
since this table was last touched, well before this phase. Not this
unit's own job to fix (its own scope is the ADR-085 colour-representation
change, not a general architecture.md audit) — worth a future
documentation unit. Similarly, §8's own module-layout `video/` file list
omits `raster.hpp` (added WU-05, also unrelated to ADR-085) — same
disposition, flagged not fixed.

## Where we are

WU-43 (`docs/architecture.md` rewritten for RGB-native colour) is built,
verified against one representative build/test configuration, and closed
out here — the build/test state is unchanged from WU-42 (still genuinely
red, per the standing ADR-085 exception), since a documentation-only
change could not have altered it. `WU-44` (~21 dependent test files
re-derived for RGB, the phase's own last unit, "green after every unit"
resumes once it lands) is the only unit left in Phase 9, now unblocked
(`WU-39`–`WU-43` are all landed; `WU-44` only actually depends on
`WU-39`–`WU-42`, the production code).

## Next work unit

**Do not start `WU-44` this session — this session's own instruction was
to stop at `WU-43` and hand off, even with budget left, and that
instruction was followed.** Whoever starts next: `WU-44` is a large unit
(~21 dependent test files, re-derived fixture-by-fixture, not
mechanically transformed, per ADR-085 §6/§5) and the phase's own last
unit — re-grep for the real file list rather than trusting
`WORK-UNITS.md`'s own WU-44 stub or ADR-085's own "21 of 35" estimate, per
`WORK-UNITS.md`'s own WU-44 entry.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question and I7's
non-achromatic breakage, both flagged above, for Steve's own decision,
not this session's.

## Blocked / red

**Red, genuinely and expectedly, unchanged in substance from Session
59 — see Build/test matrix above.** `test_binner`, `test_zoneplate` and
`test_pipeline_bytes` fail identically to Session 59's own baseline (same
lines, same counts). Every other test (25 of 28) passes. `ctest` was not
run on Steve's own real terminal yet this session — see "Steve's own next
steps" below.

## Environment check

Same GCC 13.3.0 cloud sandbox as prior sessions. This session used a
genuinely fresh `git clone` of the already-tagged `wu-42-red` commit,
confirmed identical to Steve's own real repository before any edit, with
this session's own one changed file copied in from the verified-written
real-repository copy — `git diff --stat` against the clean clone
confirmed exactly one file changed, nothing else, before the build ran.

**A stray `.git/index.lock` appeared again this session, the same
self-inflicted, already-documented pattern Sessions 55–59 logged — not a
repository problem this session found waiting for it, and this time it
did not matter.** This session's own opening state-verification found the
repository genuinely clean — `ls -la .git/index.lock` returned "No such
file or directory". A later diagnostic this session ran specifically to
confirm whether a lock would actually block git (`git add --dry-run -A`,
the same check this project's own state-verification instruction asks
for) left a fresh, empty `.git/index.lock` behind: git creates a
temporary lock for a dry-run add and normally removes it again on exit,
but the device-bridge shell this session runs in cannot delete files by
default, so git's own internal cleanup unlink silently failed. Confirmed
this actually blocks a write-requiring git command (`git add --dry-run
-A` itself failed the second time it was tried, with "Another git process
seems to be running") — read-only commands (`log`, `status`, `describe`,
`rev-parse`, `fetch`) are unaffected and were used throughout the rest of
this session without issue, and this unit never needed a write-requiring
git command in the device-bridge shell at all (every file write this
session went through `device_commit_files`, not a shell `git` command),
so the lock had no practical effect on this session's own work. The
close-out block below makes `rm -f .git/index.lock` its own first line
regardless, per this project's own standing convention, since it is
expected to actually be needed by Steve's own `git add`/`git commit`
below.

## Append to DECISIONS.md

None this session. This unit documents ADR-085's own already-accepted
Phase 9 scope; no new architectural decision was made. The
`Raster444`-vs-`RasterRGB` question flagged above is real but not this
session's decision to make or pre-empt with a new ADR.

## Append to CORRECTIONS.md

None this session. Repository-wide grep before writing (see "This session
in full" above) found `tools/coverage_view_demo.cpp`'s own stale fixture
values, but that staleness was already logged by `WU-41`'s own
`WORK-UNITS.md` entry — re-finding an already-known, already-flagged
issue is not a new correction to log, and this file already says so above
rather than re-logging it as if new.

## Closed out this session

**WU-43 — `docs/architecture.md` rewritten for RGB-native colour
(ADR-085): §1's out-of-scope line clarified, §2's Design Invariants table
rows 3/4 updated, §3's signal-path diagram given two new RGB-boundary
stages plus an explanatory paragraph on `Raster444`'s own permanent
mislabelling, §4.3's `Frag` struct fields renamed to match
`core/types.hpp`, §5 given a new "RGB boundary conversion" subsection,
§9 confirmed unchanged (wire-domain, correctly). Verified against one
representative build/test configuration in a fresh cloud-sandbox clone:
identical red state to Session 59 (same three tests, same check lines,
same counts), no new warning. Not yet committed.** One source-adjacent
file (`docs/architecture.md` — no `src/`/`tests/` file touched, confirmed
above), `WORK-UNITS.md` (WU-43 entry replaced in full), this `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **still
expected to be red, exactly as `wu-42-red` was, not newly green**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect the same failures `wu-42-red` already had: `test_decklink_device`'s
own duplex check (standing PSU exception, unrelated), plus `test_binner`
(2 checks), `test_zoneplate` (22 checks), `test_pipeline_bytes`
(3 checks) — your own local build uses whatever `SCATTER_TILE_LOG2` your
`build/` directory was configured with (this project's own settled
default is 5, ADR-045), so `test_binner`'s own total check count in your
output may read `39139` (tile 5) or `10963` (tile 4) — both are correct,
per `CORRECTIONS.md` C-033, not a sign anything is wrong.

The `rm -f .git/index.lock` above is a precaution this time (this
session's own device-bridge shell never ran a write-requiring `git`
command against your repository), but it costs nothing to run and matches
this project's own standing convention — do not skip it.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly (by design, the
standing Phase 9 exception), and `close.sh` refuses to tag past any
failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the ones named above, close out with the **manual tag path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add docs/architecture.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-43: docs/architecture.md rewritten for RGB-native colour (ADR-085); red, see HANDOFF.md"
git tag -a wu-43-red -m "docs/architecture.md rewritten for RGB-native colour, ADR-085 (Design Invariants table, signal-path diagram, Frag struct, chroma-handling section); build/test state unchanged from wu-42-red (test_binner/test_zoneplate/test_pipeline_bytes still red, pre-existing, WU-44's own job), see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-43-red`, not `wu-43-green`** — this unit's own change is
documentation-only and could not have turned the suite green; it carries
forward the exact red state `wu-42-red` already had, honestly reported,
per the standing ADR-085 exception. `WU-44` is still the unit expected to
bring the suite back to green.

This exact list of 3 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 3 `M` lines (`docs/architecture.md`,
`WORK-UNITS.md`, `HANDOFF.md`) and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
