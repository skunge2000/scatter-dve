# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 59 (`WU-42` — Phase 9's fourth production unit, PASS 2
(`src/core/resolve.hpp`/`.cpp`) reshaped from `Y`/`Cb`/`Cr` to `R`/`G`/`B`).

**Tag:** `wu-41-bgfix-red` was the newest real tag at this session's own
start (confirmed directly: `git fetch origin && git log --oneline -5`,
`git tag -l | tail -5`, `git rev-parse HEAD origin/main` — both
`c39a5ec`, `git describe --tags --exact-match HEAD` — `wu-41-bgfix-red`,
dereferencing to that same commit). `git status --short` read empty at
session start — clean tree, Session 58's own work already committed,
tagged and pushed by Steve, exactly as he reported. This session's own
nine-file change is not yet committed, tagged or pushed — that is Steve's
own next step, below.

## Before doing anything else in the next session

Run `git fetch origin`, `git log --oneline -5`, `git tag -l | tail -5`,
`git rev-parse HEAD origin/main`, `git describe --tags --exact-match HEAD`
and `git status --short` directly against `~/src/scatter-dve` — do not
trust this file's own account without checking it against the real
repository first. Also check `.git/index.lock` directly (`ls -la
.git/index.lock`) before assuming it is absent — see "Environment check"
below; this session's own opening verification found one, real state
disagreed with the prompt that started this session, and the session
correctly stopped and reported rather than proceeding, until Steve
confirmed it was safe to continue.

## This session in full

Opened as a direct continuation, in the same conversation as the aborted
first attempt: that attempt's own opening state-verification found a real,
live `.git/index.lock` (confirmed to actually block git — `git add
--dry-run -A` failed with "Unable to create index.lock: File exists" — not
merely present and harmless) and correctly stopped rather than proceeding,
per this project's own explicit gating instruction. Steve replied "Done";
this session re-ran the full verification rather than trusting that one
word, and this time found `HEAD == origin/main == c39a5ec`, the expected
commit message, `wu-41-bgfix-red` dereferencing to it, a clean tree, and
**no** `.git/index.lock` — state (a), genuinely confirmed this time, so
the session proceeded.

Read `SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 58's own entry),
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (at
least C-028, C-029, C-032, read in the order they appear), and
`WORK-UNITS.md`'s own WU-42 entry (the stub, not trusted as a finished
scope) and WU-41's own entry including its addendum, all before touching
anything, per this session's own opening instruction.

**Re-derived the real current scope directly against the code, not from
`WORK-UNITS.md`'s own WU-42 stub or WU-41/HANDOFF's own forward-looking
notes about it:** read `src/core/resolve.hpp`/`.cpp` and
`src/core/pipeline.cpp` in full this session (satisfying "never edit a
file you have not been shown this session" even though both were already
read fully the prior session while diagnosing the WU-41 background-colour
bug), then grepped the whole repository twice — once narrowly
(`ResolvedCell\|CompositedCell\|\.Y\s*=\|\.Cb\s*=\|\.Cr\s*=\|
normaliseCell(\|composite(\|compositeLayered(\|compositeKBuffer(` across
`src/core/`) and once broadly (`ResolvedCell\|CompositedCell\|Background\b`
across every `.cpp`/`.hpp` in the repository) — before deciding what
needed to change.

**The rename itself, `src/core/resolve.hpp`/`.cpp`:** `ResolvedCell`,
`Background`/`kDefaultBackground` and `CompositedCell`'s own `Y`/`Cb`/`Cr`
fields renamed to `R`/`G`/`B`, matching WU-39's `Frag`/`AccumCell`
precedent and WU-41's `SourceRaster` precedent; `normaliseCell()`,
`composite()`, `asBackground()` and `compositeKBuffer()` updated to
read/write the renamed fields. Pure rename — the arithmetic shape
(`divideRounded()`/`blend()`, a straight positional divide/blend) is
unchanged, confirmed by the identical build/test matrix below.
`kDefaultBackground`'s own *value* (already RGB-domain black,
`{kBlack, kBlack, kBlack}`, since Session 58's own WU-41 follow-up fix)
was not touched again — this unit's own job was only the field-name
rename on top of that already-correct value.

**`core/pipeline.cpp` needed touching after all — confirmed, not assumed,
per this session's own opening instruction to check the real shape of its
output-side block before deciding.** Two real, compile-breaking sites:
`resolveOneTile()`'s two `CompositedCell`-field reads
(`dest.Y[idx] = out.Y;`/`.Cb`/`.Cr`, one in the plain path, one in the
k-buffer path) reference the now-renamed fields, fixed to
`out.R`/`out.G`/`out.B` — mechanical, values unchanged; `dest.Y`/`.Cb`/
`.Cr` themselves (the `video::Raster444` side) are untouched. Beyond that
mechanical fix, **no further change was needed or made** to
`pipeline.cpp`: `runFrame()`'s own `dest` parameter and `video::Raster444`
stay genuinely YCbCr-*typed* throughout, exactly the first of the two
possibilities this session's own opening instruction posed — `Raster444`
is not one of this unit's own files, and reshaping it would break its own
genuine YCbCr role either side of chroma up/downsample (ADR-005,
unaffected) — so every `chroma::rgbToYcbcrImage()`/`chroma::ycbcrToRgbImage()`
call in `pipeline.cpp` is unchanged, and `dest`'s own `.Y`/`.Cb`/`.Cr`
planes still hold genuine RGB positionally, mislabelled under
`Raster444`'s permanently-YCbCr-named fields.

**One correction to WU-41's own forward-looking note, found while writing
this up, not before:** WU-41's own file-header comment in `pipeline.cpp`
said "WU-42 resolves it for good by reshaping `core/resolve.cpp`/`.hpp` to
`R`/`G`/`B` throughout." That overstates what actually changes. The
mislabelling WU-41 described was really two independent layers —
PASS-2-internal types (`ResolvedCell`/`CompositedCell`/`Background`, now
honestly `R`/`G`/`B`) *and* `video::Raster444` itself (permanently
`Y`/`Cb`/`Cr`-named, reused mid-pipeline to carry RGB content) — and this
unit only resolves the first layer. The second is not eliminated; it is
`Raster444`'s own permanent role as a generic three-plane container, not a
temporary state any future unit is expected to resolve away (reshaping
`Raster444` itself would break its own real, unrelated YCbCr use either
side of chroma up/downsample). Updated `pipeline.cpp`'s own file-header
comment in place with a new, dated paragraph (not by rewriting WU-41's own
historical text, per this project's own layered-comment convention) —
flagged for Steve below, not treated as a defect anywhere in this account.

**Repository-wide grep before closing out (C-028/C-029/C-032's own
standard):** `grep -rln 'ResolvedCell\|CompositedCell\|Background\b'
--include="*.cpp" --include="*.hpp" .` found exactly eleven files: this
unit's own three, plus eight test files. Read each of the eight in full.
`tests/test_threading.cpp` and `tests/test_persistent_pool.cpp` matched
only via comment prose ("Background per PipelineParams' own default",
local names `sawNonBackground`/`hasNonBackground`) — no real field access,
confirmed, no edit needed. The other six needed real field-name edits
(values unchanged throughout — every `Background{...}` aggregate
construction is positional, not name-based, so only named-field *reads*
on `CompositedCell`/`Background` broke at compile time):
`tests/test_layered_composite.cpp`, `tests/test_kbuffer_resolve.cpp`,
`tests/test_pageturn.cpp`, `tests/test_field_pipeline.cpp`,
`tests/test_shapes.cpp`, `tests/test_zoneplate.cpp`. Re-grepped after
editing (`(bg|out|acc|resolved|got|expected|actual|step|afterRead|
afterBack|afterMid|afterB)\.(Y|Cb|Cr)`, across every touched file): no
matches remain; every surviving `.Y`/`.Cb`/`.Cr` site is a genuine
`video::Raster444` (or `testpat::Frame`) field, traced by declared type
before being left alone. `tools/` and `docs/` grepped too: no matches.

**Every changed file written to Steve's own real repository via the
device bridge, then re-staged and grepped to confirm the write landed**
(spot-checked the renamed struct definitions in `resolve.hpp`, the
renamed field assignments in `resolve.cpp`, the `out.R`/`out.G`/`out.B`
lines in `pipeline.cpp`, and re-ran the same "no stale `.Y`/`.Cb`/`.Cr`
on a composited/background variable" grep across all nine files against
the re-staged copies) before building anything.

## Build/test matrix — twelve configurations (this project's own
"ten-configuration matrix," by name; see Environment check below for why
the name and the row count disagree)

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox, confirmed at `HEAD = wu-41-bgfix-red = c39a5ec` before
any edit, then this session's own nine changed files copied in from the
verified-written real-repository copies (`git diff --stat` against the
clean clone: exactly the nine files this session touched, nothing else).
Same GCC 13.3.0 / Clang 18.1.3 toolchain as prior sessions. GCC/Clang ×
Release/Debug × `SCATTER_TILE_LOG2` 4/5 (8 configurations), plus GCC +
ASan and GCC + UBSan at both tile sizes (4 more, **12 total** — see
Environment check below), all configured, built and `ctest`-run for real.

| Configuration | Build | `ctest` |
|---|---|---|
| GCC, Release, tile 4 | clean, no warnings | 25/28 pass — `test_binner`, `test_zoneplate`, `test_pipeline_bytes` fail |
| GCC, Release, tile 5 | clean, no warnings | same three fail |
| GCC, Debug, tile 4 | clean, no warnings | same three fail |
| GCC, Debug, tile 5 | clean, no warnings | same three fail |
| Clang, Release, tile 4 | clean, no warnings | same three fail |
| Clang, Release, tile 5 | clean, no warnings | same three fail |
| Clang, Debug, tile 4 | clean, no warnings | same three fail |
| Clang, Debug, tile 5 | clean, no warnings | same three fail |
| GCC + ASan, tile 4 | clean, no warnings | same three fail, no ASan finding |
| GCC + ASan, tile 5 | clean, no warnings | same three fail, no ASan finding |
| GCC + UBSan, tile 4 | clean, no warnings | same three fail, no UBSan finding |
| GCC + UBSan, tile 5 | clean, no warnings | same three fail, no UBSan finding |

Per-test check counts — **one genuine finding here, not noise; see
CORRECTIONS.md C-033 for the full account:**

- `test_binner`: 2 of 39139 checks fail at `SCATTER_TILE_LOG2=5`
  (`test_binner.cpp:794`, `:795`, the pre-existing, already-documented
  shading-mirror-fixture staleness, `WU-44`'s own job) — matching Session
  58's own baseline exactly. At `SCATTER_TILE_LOG2=4`, the same two checks
  fail, but the total is **2 of 10963**, not 39139. Isolated before
  logging: a fresh, unmodified clone of the already-tagged
  `wu-41-bgfix-red` commit, built at tile 4 with none of this session's
  own changes applied, reproduces the identical `10963` figure — this is
  not a regression `WU-42` introduced, it is a correction to Session
  57/58's own "identical across all ten configurations" claim, which
  turns out to have been true for two of the three tests, not all three.
- `test_zoneplate`: 22 of 42537 checks fail, identically at both tile
  sizes (`test_zoneplate.cpp:209`, `:212`, `:213` — the pre-existing,
  already-documented I7 non-achromatic round-trip breakage, ADR-085 §5's
  own accepted phase-9 exception) — matches Session 58's own baseline
  exactly.
- `test_pipeline_bytes`: 3 of 42 checks fail, identically at both tile
  sizes (`test_pipeline_bytes.cpp:406`, `:452`, `:552` — the pre-existing,
  already-documented deinterlaced-path reference-function staleness,
  `WU-44`'s own job) — matches Session 58's own baseline exactly.

Every other test (25 of 28) passes in every one of the twelve
configurations, exactly as Session 58 left it — **this unit's own rename
changes no test's outcome**: same three tests, same failing check lines,
same counts (modulo the tile-4 `test_binner` correction above, which
reproduces identically with or without this unit's own changes).

## Flag for Steve, not resolved here

**Carried forward unchanged from Sessions 56–58:** I7 ("Input v210 equals
output v210, byte for byte, illegal excursions included") is not exactly
true post-ADR-085 for non-achromatic flat content — `test_zoneplate.cpp`'s
own `test_i7_identity_full_pipeline()` is the check that surfaces this,
one of the three tests already counted as red above. `INVARIANTS.md`'s own
I7 wording has still not been revisited to say so explicitly — flagged
again, not touched (this project's own standing rule: flag concerns about
`INVARIANTS.md`, never edit it directly).

**New this session:** `video::Raster444`'s own permanent role as a
YCbCr-*named*-but-sometimes-RGB-*valued* container mid-pipeline
(`runFrame()`'s own `dest`/`warped`/`full`/`progressive` locals in
`core/pipeline.cpp`) is not something any future unit resolves away — it
is a deliberate, permanent design characteristic of reusing one
three-plane struct for two different roles, not a temporary state on the
way to something cleaner. WU-41's own note that "WU-42 resolves it for
good" was optimistic; corrected in `pipeline.cpp`'s own file-header
comment this session (see "This session in full" above). Worth Steve's
own explicit sign-off on whether this is acceptable as a permanent shape,
or whether a future unit should give `runFrame()`'s own output-side RGB
content a genuinely RGB-named type (`video::RasterRGB`, which already
exists, WU-40) instead of continuing to borrow `Raster444` for it — not
decided or started here, since `Raster444` is not one of `WU-42`'s own
files and touching it would be a real, unscoped reshape, not a rename.

## Where we are

WU-42 (Phase 9, `src/core/resolve.hpp`/`.cpp` reshaped to `R`/`G`/`B`) is
built, matrix-verified across twelve sandbox configurations, and closed
out here — genuinely red, per the standing ADR-085 §5 exception, not
forced green. `WU-43` (`docs/architecture.md` rewrite) and `WU-44` (~21
dependent test files re-derived for RGB, the phase's own last unit,
"green after every unit" resumes once it lands) are both still open,
`WU-44` now unblocked (`WU-39` through `WU-42` are all landed).

## Next work unit

**Do not start `WU-43` or `WU-44` this session — this session's own
instruction was to stop at `WU-42` and hand off, even with budget left,
and that instruction was followed.** Whoever starts next: `WU-43`
(`docs/architecture.md`'s Design Invariants table and signal-path diagram,
rewritten for RGB) is documentation-only against now-fully-landed code
(`WU-39`–`WU-42`); `WU-44` (the ~21 dependent test files, re-derived
fixture-by-fixture, not mechanically transformed, per ADR-085 §6/§5) is
the phase's own last unit and the one that actually turns this red state
green — re-grep for the real file list rather than trusting either this
note or ADR-085's own "21 of 35" estimate, per `WORK-UNITS.md`'s own
WU-44 entry.

## Open questions

The `video::Raster444`-vs-`video::RasterRGB` question flagged above, for
Steve's own decision, not this session's.

## Blocked / red

**Red, genuinely and expectedly, unchanged in substance from Session
58 — see Build/test matrix above.** `test_binner`, `test_zoneplate` and
`test_pipeline_bytes` fail identically in all twelve sandbox
configurations (same lines, same values, modulo the tile-4 `test_binner`
denominator correction in CORRECTIONS.md C-033). Every other test (25 of
28) passes in every configuration. `ctest` was not run on Steve's own real
terminal yet this session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as prior sessions
(`nproc` = 2 this session). This session used a genuinely fresh `git
clone` of the already-tagged `wu-41-bgfix-red` commit, confirmed identical
to Steve's own real repository before any edit, with this session's own
nine files copied in from the verified-written real-repository copies
(not built directly from the sandbox's own edited-in-place copy) —
`git diff --stat` against the clean clone confirmed exactly nine files
changed, nothing else, before the matrix build began.

**"Ten-configuration matrix" is this project's own name for what has
always actually been twelve build/test rows — noted, not changed, this
session.** Re-checked directly against Session 58's own HANDOFF.md table
(the most recent full one): it lists GCC/Clang × Release/Debug × tile 4/5
(8 rows) plus GCC+ASan and GCC+UBSan **each at both tile sizes** (4 more
rows, not 2) — 12 rows total, tabulated in full in that session's own
account, under the same "ten-configuration" name this session's own
opening instruction also used. This session ran the identical 12 rows
Session 58 did, under the same name, rather than second-guessing which
count is "correct" — a naming-vs-count mismatch, not a build coverage
gap, and not this session's place to rename a standing project convention
without being asked.

**A stray `.git/index.lock` appeared again this session, self-inflicted
this time, not a repository problem this session found waiting for
it.** This session's own opening state-verification (after Steve's
"Done") found the repository genuinely clean — `ls -la .git/index.lock`
returned "No such file or directory." This session's own follow-up
diagnostic (`git add --dry-run -A`, run to positively confirm the absence
of a lock actually meant git could perform a write-requiring operation,
not just that the file didn't exist) left a fresh, empty
`.git/index.lock` behind: git creates a temporary lock for a dry-run add
and normally removes it again on exit, but the device-bridge shell this
session runs in cannot delete files by default (confirmed: both `rm -f`
and `mv` on the lock file itself returned "Operation not permitted"), so
git's own internal cleanup unlink silently failed and the lock was left
in place, blocking any subsequent write-requiring git command
(`git add`) with the same "Another git process seems to be running"
message Sessions 55–58 already documented for the same underlying cause.
Read-only commands (`log`, `status`, `describe`, `fetch`) are unaffected
and were used throughout the rest of this session without issue. This
session did **not** attempt to remove the lock again after the first
failed `rm`/`mv` pair, to avoid compounding the problem — the close-out
block below makes `rm -f .git/index.lock` its own first line regardless,
per this project's own standing convention, and it is expected to
actually be needed this time, not a no-op precaution.

## Append to DECISIONS.md

None this session. This unit implements ADR-085's own already-accepted
Phase 9 scope (the PASS 2 field rename WU-38's own work breakdown already
named), not a new architectural decision. The `Raster444`-vs-`RasterRGB`
question flagged above is a real open question but not this session's
decision to make or pre-empt with a new ADR.

## Append to CORRECTIONS.md

**C-033 added** — HANDOFF.md's own Session 57/58 claim that per-test check
counts are "identical across all ten configurations" was not quite right:
`test_binner`'s own total genuinely depends on `SCATTER_TILE_LOG2` (10963
at tile 4, 39139 at tile 5), confirmed independent of this session's own
changes via a fresh, unmodified clone of the already-tagged
`wu-41-bgfix-red` commit. Full account, general lesson, in
`CORRECTIONS.md` itself.

## Closed out this session

**WU-42 — `src/core/resolve.hpp`/`.cpp`'s PASS 2 reshaped from `Y`/`Cb`/
`Cr` to `R`/`G`/`B` (`ResolvedCell`, `Background`/`kDefaultBackground`,
`CompositedCell`), matching WU-39/WU-41's own precedent; `src/core/
pipeline.cpp`'s two `CompositedCell`-field reads fixed to match
(mechanical, values unchanged), plus its own file-header comment corrected
about what this unit does and does not resolve; six test files
mechanically updated to keep the tree compiling. Built and matrix-verified
across twelve fresh cloud-sandbox configurations: identical red state to
Session 58 (same three tests, same check lines, same counts except one
tile-4-specific `test_binner` denominator correction logged as C-033), no
new warning, no sanitizer finding. Not yet committed.** Three source files
(`src/core/resolve.hpp`, `src/core/resolve.cpp`, `src/core/pipeline.cpp`),
six test files (`tests/test_layered_composite.cpp`,
`tests/test_kbuffer_resolve.cpp`, `tests/test_pageturn.cpp`,
`tests/test_field_pipeline.cpp`, `tests/test_shapes.cpp`,
`tests/test_zoneplate.cpp`), `WORK-UNITS.md` (WU-42 entry replaced in
full), `CORRECTIONS.md` (C-033 appended), this `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **still
expected to be red, exactly as `wu-41-bgfix-red` was, not newly green**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect the same failures `wu-41-bgfix-red` already had:
`test_decklink_device`'s own duplex check (standing PSU exception,
unrelated), plus `test_binner` (2 checks), `test_zoneplate` (22 checks),
`test_pipeline_bytes` (3 checks) — your own local build uses whatever
`SCATTER_TILE_LOG2` your `build/` directory was configured with (this
project's own settled default is 5, ADR-045), so `test_binner`'s own
total check count in your output may read `39139` (tile 5) rather than
`10963` (tile 4) — both are correct, per CORRECTIONS.md C-033, not a sign
anything is wrong.

The `rm -f .git/index.lock` above is not a precaution this time — this
session's own diagnostics left a real, empty lock file behind that the
device-bridge shell could not remove (see "Environment check" above); if
it is already gone by the time you run this, the `rm -f` is a silent
no-op, but do not skip it.

**Do not run `./tools/close.sh`** — same reasoning as every session since
`wu-39-green`: this state does not pass `ctest` cleanly (by design, the
standing Phase 9 exception), and `close.sh` refuses to tag past any
failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the ones named above, close out with the **manual tag path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/core/resolve.hpp src/core/resolve.cpp src/core/pipeline.cpp \
        tests/test_layered_composite.cpp tests/test_kbuffer_resolve.cpp \
        tests/test_pageturn.cpp tests/test_field_pipeline.cpp \
        tests/test_shapes.cpp tests/test_zoneplate.cpp \
        WORK-UNITS.md CORRECTIONS.md HANDOFF.md
git commit -m "WU-42: core/resolve.hpp/.cpp PASS 2 Y/Cb/Cr -> R/G/B rename (ADR-085); red, see HANDOFF.md"
git tag -a wu-42-red -m "PASS 2 (ResolvedCell/CompositedCell/Background) reshaped to R/G/B; test_binner/test_zoneplate/test_pipeline_bytes still red as expected (unrelated, pre-existing, WU-44's own job), see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-42-red`, not `wu-42-green`** — this unit's own build/test
matrix is genuinely red (three pre-existing failures, none of them this
unit's own job to fix, per ADR-085 §5's own standing exception), so a
green tag here would misreport the phase's own real state; `WU-44` is
still the unit expected to bring the suite back to green.

This exact list of 12 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written — it should read exactly 12 `M` lines (nine changed files plus
`WORK-UNITS.md`, `CORRECTIONS.md`, `HANDOFF.md`) and nothing else. Still
worth a quick `git status --short` yourself before pasting this block,
since time has passed since that check.
