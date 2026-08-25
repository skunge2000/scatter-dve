# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 55 (next sequential after Session 54's own `HANDOFF.md`; no
evidence of an intervening session — no later tag, no later commit, no
other `HANDOFF.md` account. WU-39, Phase 9: `core/types.hpp`'s
`Frag`/`AccumCell` `Y`/`Cb`/`Cr` → `R`/`G`/`B` rename, built).

**Tag:** `wu-38-green` was the newest real tag at this session's own
start (confirmed directly: `git tag --sort=creatordate`, `git log
--oneline -10` showing `HEAD` = `944b102` = `origin/main`, matching
Session 54's own WU-38 commit message exactly — Steve had already run
his own real-terminal close-out for WU-38, including the manual tag and
push, before this session started). `git status --short` at this
session's own start read empty — clean tree, per Session 54's own
"Steve's own next steps" block having been followed. This session's own
changes are not yet committed, tagged or pushed — that is Steve's own
next step, below.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was WU-39: rename
`Frag::Y/Cb/Cr` and `AccumCell::Y/Cb/Cr` to `R/G/B` in `core/types.hpp`
and update every real reference across the tree so it still compiles and
links, under Phase 9's standing "green not required" exception
(ADR-085 §5). Confirmed real repository state directly first (git
tag/log/status, above — state (a), clean tree, matched WU-38's own
close-out exactly), then read `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md`
(C-024 through C-031, all still current), `WORK-UNITS.md`'s Phase 9
heading and its WU-39 entry, and `src/core/types.hpp`'s real current
`Frag`/`AccumCell` definitions, per `SESSION-PROTOCOL.md` rule 6 and this
session's own opening instruction not to trust the WU-39 stub as a
finished file list.

**Re-ran the grep from scratch before touching anything.** `grep -rlw
'Frag'`/`'AccumCell'` across `src/` and `tests/`, cross-checked against
each match's real context (including re-confirming `src/io/com_ptr.hpp`
is still comment-only): came back **identical** to WU-38's own 22-file
list (10 production, 2 comment-only; 12 tests) — no drift since that
scoping session. Reported here per this session's own standing
instruction to say so either way, per C-027/C-028/C-029's own "a scoping
stub is a plan, not a fact" lesson.

**The real work: a field-by-field diff per file, not a blind substitution.**
Several of the 22 files hold *both* `Frag`/`AccumCell` (this unit's own
rename target) and one or more of `ResolvedCell`/`Background`/
`CompositedCell` (`core/resolve.hpp`, all three explicitly out of scope —
`WU-42`'s own job) or `video::Raster444`/`video::Raster422` (out of
scope — `WU-40`'s own job), and all of these unrelated structs use the
*identical* `.Y`/`.Cb`/`.Cr` accessor spelling `Frag`/`AccumCell` used
before this rename. A text-level `s/\.Y/\.R/g` across any of these files
would have silently renamed the wrong struct's fields and still compiled
(same field name, wrong type) — exactly the kind of drift this project's
own `CORRECTIONS.md` (C-027 onward) keeps finding. Every `.Y`/`.Cb`/`.Cr`
occurrence in every one of the 22 files was traced to its own variable's
real declared type before deciding whether to touch it. Result: of the
22 files, only **9 needed a real code edit** — the other 13 either
declare `Frag`/`AccumCell` as a type without ever reading a colour field
(`pipeline.cpp`, `pipeline.hpp`, `binner.hpp`, `splat.hpp`,
`test_coverage_capture.cpp`, `test_field_pipeline.cpp`, `test_smoke.cpp`)
or turned out, on inspection, to only ever touch the type via `.w`
(unrenamed) or by passing whole cells to `composite()`/`compositeLayered()`
without reading a channel directly.

**Files changed, with what and why (all nine plus the three comment-only
production files, full outcomes):**

- `src/core/types.hpp` — the rename itself: `Frag::Y, Cb, Cr` →
  `R, G, B`; `AccumCell::Y, Cb, Cr` → `R, G, B`. `kMaxFragContribution`'s
  own derivation/static_asserts untouched (I4's bound is field-name-
  independent). `kChromaZero` left exactly as defined — see below.
- `src/core/resolve.cpp` — `normaliseCell()`: `cell.Y/Cb/Cr` →
  `cell.R/G/B` (reads `AccumCell`); `out.Y/Cb/Cr` in that function is
  `ResolvedCell`, **unchanged**. `sumCells()` (anonymous namespace,
  `compositeLayered()`'s transparent-sum branch): `a`/`b`/`out` are all
  `AccumCell` — all six field accesses renamed. `composite()` itself
  needed no change: it never reads an `AccumCell` field directly, only
  passes `cell` through to `normaliseCell()`.
- `src/core/binner.cpp` — the shared per-sample loop's `Frag`
  construction: `frag.Y/Cb/Cr = toSample(c.y/cb/cr)` → `frag.R/G/B =
  ...`. `c` is the file-local (lowercase) `Colour` struct, **unchanged**
  — it still carries YCbCr-domain doubles from `sampleBilinear()`/
  `applyShading()`, both untouched by this unit (that reshaping is
  `WU-41`'s job). One stale comment fixed: "`I3/I4/I6` govern the
  *stored* `Y/Cb/Cr`" → "the *stored* `Frag` fields (`R/G/B`, renamed
  from `Y/Cb/Cr`...)", since `Frag`'s stored fields are the actual
  referent and are now named `R/G/B`.
- `src/core/splat.cpp` — `accumulateCorner()`: `cell.Y/Cb/Cr` and
  `f.Y/Cb/Cr` → `R/G/B` (both `AccumCell`/`Frag`). `sumBanks()`:
  `sum.Y/Cb/Cr`, `c.Y/Cb/Cr` → `R/G/B` (both `AccumCell`). One comment
  fixed ("`Y` or `Cb` or `Cr`, times `w`" → "`R` or `G` or `B`...").
- `src/core/resolve.hpp` — no field access at all (only declares
  functions taking `const AccumCell&`); one comment fixed: "`AccumCell`'s
  `Y/Cb/Cr` fields are premultiplied" → "`R/G/B` fields ... (renamed
  from `Y/Cb/Cr`...)". The other `Y/Cb/Cr` mention in this file (line 71,
  about `ResolvedCell`) is correctly **unchanged**.
- `src/core/coarse_shading.hpp` — comment-only mention, "`Frag::Y/Cb/Cr`"
  → "`Frag::R/G/B` (renamed from `Y/Cb/Cr`...)", exactly the mention
  WU-38's own scoping flagged.
- `src/core/binner.hpp` — one stale comment predating this unit (from
  WU-38's own I3 acceptance, never fixed there): "This project's own
  internal colour (I3) is `Y/Cb/Cr`, never RGB" was already false the
  moment WU-38 landed ADR-085. Corrected in place to say I3 moved to RGB
  at WU-38, while making clear this function's own YCbCr-domain
  arithmetic is unaffected by that alone (still `WU-41`'s job). Not a
  `CORRECTIONS.md` entry — nothing this session wrote was wrong; this is
  a pre-existing staleness from a prior session's own doc-only unit,
  fixed opportunistically because this session was already in the file
  for the rename and leaving a comment that contradicts `INVARIANTS.md`
  right next to the code being renamed is exactly the kind of drift this
  project's own `CORRECTIONS.md` keeps finding later.
- `src/core/pipeline.cpp`, `src/core/pipeline.hpp`, `src/core/binner.hpp`
  (structural), `src/core/splat.hpp` — no code change; `AccumCell`/`Frag`
  appear only as types or via `.w`, never `.Y`/`.Cb`/`.Cr`.

**Test files — 7 needed edits, 5 needed none:**

- `tests/test_binner.cpp` — `decode()` helper: `f.Y, f.Cb` → `f.R, f.G`
  (`Frag`). The shading-multiply test's `unshaded[i]`/`shaded[i]` (both
  `Frag`, from `TileBins`): six `.Y/.Cb/.Cr` → `.R/.G/.B`. The
  `frame.Y/Cb/Cr` (line ~580, `video::Raster444`) and
  `s.y = r.Y.data()` (`SourceRaster`/`Raster444`) occurrences nearby are
  correctly **unchanged**.
- `tests/test_kbuffer_storage.cpp` — every `.Y/.Cb/.Cr` in this file is a
  `Frag` or `AccumCell` (via `KSlot::cell`) access; all renamed
  (`makeExactFrag()`, both accept-criteria tests, the eviction test).
  Two comments fixed to match ("`Y/Cb/Cr` contributions reduce to plain
  `Y*w/Cb*w/Cr*w`" → `R/G/B`/`R*w/G*w/B*w`; "`Y*w` with no truncation" →
  "`R*w`...").
- `tests/test_kbuffer_resolve.cpp` — only `makeUniformCell()`'s `c.Y/Cb/Cr`
  (`AccumCell`) renamed. Every other `.Y/.Cb/.Cr` in this file (`got`,
  `expected`, `bg`, `step1`/`afterBack`, etc.) is `CompositedCell`/
  `Background` — correctly **unchanged**.
- `tests/test_layered_composite.cpp` — three spots: `expectedSum()`
  (`a`/`b`/`out`, all `AccumCell`), `makeUniformCell()` (`c`, `AccumCell`),
  and `upper.Y/Cb/Cr` inside
  `test_pipeline_pageturn_opaque_flap_hides_page_behind()` (`upper` is
  `const AccumCell&` from `layers.flapOnly`). Every `CompositedCell`
  comparison in this file (`actual`, `expected`, `afterRead`,
  `transparentResult`, and the `a`/`b`/`c` locals in the full-alpha test,
  which are `CompositedCell` results despite reusing the same letters as
  `expectedSum()`'s unrelated `AccumCell` parameters) is **unchanged**.
- `tests/test_pageturn.cpp` — only the accumulation-sums identity check
  (`combined[i].Y/Cb/Cr == pageOnly[i].Y/Cb/Cr + flapOnly[i].Y/Cb/Cr`, all
  `std::vector<AccumCell>`) renamed. The nearby
  `combinedComposited.Y/Cb/Cr`/`pageAloneComposited.Y/Cb/Cr`
  (`CompositedCell`) are **unchanged**.
- `tests/test_row_band.cpp` — `decode()` and `fragBitIdentical()`
  (`Frag`) renamed; see the dedicated note below on the open question
  WU-38's own scoping flagged for this file. The `dest.Y/Cb/Cr`/
  `reference.Y/Cb/Cr` (`video::Raster444`) in the threaded-pipeline test
  are **unchanged**.
- `tests/test_scan_order_invariance.cpp` — `checkAllTilesIdentical()`'s
  `a`/`b` (both `const AccumCell&`) renamed; the `cell.w` check nearby is
  untouched (not a renamed field).
- `tests/test_splat.cpp` — every `.Y/.Cb/.Cr` in this file is `Frag` or
  `AccumCell`; renamed throughout (`makeFrag()`, all eight test
  functions).
- `tests/test_zoneplate.cpp` — only
  `test_composite_partial_coverage_no_green_fringe()`'s hand-built
  `cell.Y/Cb/Cr` (`AccumCell`) and the one `cell.Cr` reference inside its
  own "wrong answer" derivation, renamed. Every other `.Y/.Cb/.Cr` in this
  32 KB file (`f.Y` on `video::Raster422` in the three pattern
  generators, `result.Y`/`src.Y`/`dest.Y`/`zp.Y`/`params.background.Y`,
  all `Raster444`/`Raster422`/`testpat::Frame`/`Background`) is
  **unchanged** — confirmed by tracing every declaration in the file, not
  by pattern alone.
- `tests/test_coverage_capture.cpp`, `tests/test_field_pipeline.cpp`,
  `tests/test_smoke.cpp` — no change; see above.

**`tests/test_row_band.cpp`'s `decode()`-by-signature helper — WU-38's
own flagged open question, resolved.** WU-38's scoping asked whether this
helper (`f.Y`, `f.Cb`, C-015) "needs a shape change beyond the rename."
It does not: `decode()` only reads back whatever two `Frag` fields
`SignatureRaster`'s own `(px, py)` signature got written into by
`binner.cpp`'s per-sample loop — that loop's own numeric content is
completely unaffected by what the destination fields are called, so
renaming `f.Y`→`f.R`, `f.Cb`→`f.G` is a pure label change with no logic
implication. Fixed as a plain substitution; no further investigation
needed.

**`kChromaZero` (`types.hpp`): kept, not removed.** Checked directly this
session with a repository-wide grep before deciding, rather than going
with WU-38's own framing at face value: WU-38's WU-39 stub said
`kChromaZero` "becomes dead once no channel needs an achromatic
mid-point offset any more" — true of the *eventual* end state once the
whole of Phase 9 lands, but not of this unit alone. `kChromaZero` is
still read by `src/video/v210.cpp`/`.hpp` (chroma plane fill), 
`src/video/chroma.hpp` (its own doc comment), `src/core/binner.cpp`'s
`applyShading()` (its YCbCr-domain round trip, entirely untouched by
this unit — `WU-41`'s job), `src/core/resolve.hpp`'s `Background`
default, and well over a dozen test files (`test_v210.cpp`,
`test_chroma.cpp`, `test_zoneplate.cpp`, `test_threading.cpp`,
`test_persistent_pool.cpp`, `test_coverage_capture.cpp`,
`test_scan_order_invariance.cpp`, `test_kbuffer_resolve.cpp`,
`test_field_pipeline.cpp`, `test_testpat.cpp`, `test_binner.cpp`, and
more) — none of which this unit's own file list touches, since they all
operate on `Raster444`/`Raster422`'s still-YCbCr planes or the still-YCbCr
v210 boundary, both `WU-40`'s own job. Removing `kChromaZero` now would
simply fail to compile roughly a dozen unrelated files. Left exactly as
defined in `types.hpp`, no comment change needed there (the staleness was
entirely in `WORK-UNITS.md`'s own framing, now corrected).

## Build/test matrix — all ten configurations green

Run in a fresh `git clone` of `skunge2000/scatter-dve` at `HEAD`
`944b102` (confirmed matching the real repository before any edit) in
this session's own Linux cloud sandbox, then this session's 16 changed
files copied over and built/tested for real (GCC 13.3.0, Clang 18.1.3
both present):

| Compiler | Build type | `SCATTER_TILE_LOG2` | Result |
|---|---|---|---|
| GCC 13.3.0   | Release | 4 | 28/28 tests pass |
| GCC 13.3.0   | Release | 5 | 28/28 tests pass |
| GCC 13.3.0   | Debug   | 4 | 28/28 tests pass |
| GCC 13.3.0   | Debug   | 5 | 28/28 tests pass |
| Clang 18.1.3 | Release | 4 | 28/28 tests pass |
| Clang 18.1.3 | Release | 5 | 28/28 tests pass |
| Clang 18.1.3 | Debug   | 4 | 28/28 tests pass |
| Clang 18.1.3 | Debug   | 5 | 28/28 tests pass |
| GCC 13.3.0 + ASan/UBSan | Debug | 4 | 28/28 tests pass |
| GCC 13.3.0 + ASan/UBSan | Debug | 5 | 28/28 tests pass |

**This is a real, accepted-as-reported green, not a forced or lucky one,
and not evidence the standing "green not required" exception (ADR-085 §5)
somehow did not apply here.** Every single reference this unit touched
was a pure field-label rename with the underlying numeric data flow
completely unchanged (confirmed file by file, above) — nothing in this
unit's own scope had a chance to disagree with anything else. The actual
semantic reshaping ADR-085 describes — `SourceRaster`, the local `Colour`
struct, and `ResolvedCell`/`Background`/`CompositedCell` actually
carrying RGB triples instead of YCbCr ones — is `WU-40` through `WU-42`'s
own job, still entirely ahead, and that is where this phase's own
green-suspension exception is genuinely expected to bite. Reported
exactly as it came out, per this unit's own standing instruction to
report red or green honestly either way.

No DeckLink-linked target exists in this sandbox (no Apple toolchain, no
`BLACKMAGIC_SDK_DIR`) — `test_decklink_device`'s own duplex-check
exception (ADR-034/035, C-024) does not arise here at all; only the
28-test `scatter-core` suite ran, matching every prior sandbox session's
own experience (WU-32/34b/36/38).

## Where we are

**WU-39 is written, built and delivered, green in the cloud sandbox.**
All sixteen changed files (`src/core/types.hpp`, `binner.cpp`,
`binner.hpp`, `coarse_shading.hpp`, `resolve.cpp`, `resolve.hpp`,
`splat.cpp`; `tests/test_binner.cpp`, `test_kbuffer_storage.cpp`,
`test_kbuffer_resolve.cpp`, `test_layered_composite.cpp`,
`test_pageturn.cpp`, `test_row_band.cpp`, `test_scan_order_invariance.cpp`,
`test_splat.cpp`, `test_zoneplate.cpp`) plus `WORK-UNITS.md` (WU-39 entry
updated to `green` with the real outcomes above) and this `HANDOFF.md`
are written to the real repository via the device bridge, re-staged and
diff/checksum-confirmed byte for byte against what this session intended
— see the confirmation note below — but **not yet committed, tagged or
pushed**. That is Steve's own next step, after his own real-terminal
build/test confirms green (expected, since the cloud sandbox already
confirmed it across ten configurations, but per `SESSION-PROTOCOL.md`
still worth Steve's own real run before tagging).

`DECISIONS.md` and `INVARIANTS.md` are **untouched** this session, per
this unit's own explicit brief (I3/I4 already finalised at WU-38; no ADR
reopened).

## Next work unit

**`WU-40`** (`src/video/v210.cpp`/`chroma.cpp`/`.hpp`: RGB boundary
conversion, both directions) is Phase 9's own next pick, per
`WORK-UNITS.md`'s own dependency ordering (depends on `WU-39`, now
landed). Per ADR-085 §5 and this phase's own standing exception, `WU-40`
is also not expected to leave the build green at the end — unlike
`WU-39`, it is expected to actually touch `SourceRaster`'s/`Raster444`'s
own YCbCr-vs-RGB boundary, which is exactly where downstream breakage
(callers still expecting YCbCr semantics from what `WU-40` is turning
into RGB) becomes plausible for the first time in this phase. Everything
named in earlier sessions' own "Next work unit" sections outside Phase 9
(WU-28d, WU-29, WU-33, WU-35, WU-37) is unchanged and still pickable —
this session did not touch any of them.

**Per this session's own standing instruction: do not proceed to WU-40
even with session budget left.** Each Phase 9 unit is knowingly
larger-than-normal (ADR-085 §5) and this session stops here, the same
restraint WU-34b's own session used deferring WU-34c, and WU-38's own
session used deferring WU-39 itself.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. ADR-085 §7's
own open sub-questions are unchanged from WU-38's own account (I4's
magnitude bound already resolved there): where `ColourStandard`/
`coeffsFor` should live; per-frame boundary-conversion parameterisation;
fixture-value re-derivation strategy. All three remain `WU-40`/`WU-41`'s
own concern, not touched by this unit's pure rename.

## Blocked / red

Not blocked. Not red — see the build/test matrix above. `ctest` was not
run on Steve's own real terminal yet this session — see "Steve's own next
steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang++ --version` directly
before building); the full ten-configuration matrix ran for real, per
this unit's own brief (WU-34b's own precedent, not the
documentation-only four-configuration one WU-32/WU-36/WU-38 used, since
this unit touches real production source). A fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` was used for the sandbox
build (not a copy staged through the device bridge), confirmed matching
`HEAD 944b102` before any edit. The device-bridge sandbox used for
reading/writing files on the real Mac repository had ordinary read/write
access this session for the tracked files themselves, but a stray,
empty `.git/index.lock` **was** found this session while running `git
status --short` to confirm the file list below (unlike every prior
session's own account, which found none) — the device-bridge shell could
not remove it itself (`rm`/`unlink` on a bridge-mounted file returns
"Operation not permitted" there by design). This may or may not be
present by the time Steve reaches his own real terminal; the close-out
block below now checks for it and removes it first regardless, since a
stale `index.lock` would otherwise make Steve's own `git add`/`git
commit` fail with "Unable to create '.git/index.lock': File exists." No
`git commit` or `git push` attempted this session — nothing pushed.
C-024's standing condition (PSU out, `tools/close.sh` cannot succeed on
Steve's own real terminal for any unit) checked directly against
`CORRECTIONS.md` this session and found unchanged — `./tools/close.sh`
must not be run. (No DeckLink target exists in the cloud sandbox at all,
so the duplex-check exception itself does not arise there — it is purely
a real-terminal, real-hardware concern for Steve's own next steps below.)

## Append to DECISIONS.md

None this session. ADR-085 (WU-38) already covers this unit's own scope
in full; this unit's own findings (field-by-field diff, the
`decode()`/`kChromaZero` resolutions) are recorded in `WORK-UNITS.md`'s
own WU-39 entry instead, matching how WU-38 itself recorded its own
scoping detail there rather than in a new ADR.

## Append to CORRECTIONS.md

None this session. The `binner.hpp` stale-comment fix (above) is not a
`CORRECTIONS.md` entry: nothing this session wrote was wrong, and the
staleness itself predates this session (it was a doc gap left over from
WU-38's own I3 acceptance, never a claim this session made or relied on)
— fixed opportunistically while already in the file, not logged as a
correction of this session's own work.

## Closed out this session

**WU-39 — `core/types.hpp`'s `Frag`/`AccumCell` `Y`/`Cb`/`Cr` → `R`/`G`/`B`
rename, built, green in all ten cloud-sandbox configurations, not yet
committed.** Sixteen source/test files (listed above under "Where we
are"), `WORK-UNITS.md` (WU-39 entry updated to `green`), this
`HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real, green (modulo the standing
duplex exception, which only applies once `scatter-decklink`/
`test_decklink_device` are actually built with the SDK configured) build
and test run:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect `test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check to fail regardless (the standing PSU/two-device exception,
`DECISIONS.md` ADR-034/035/037, `CORRECTIONS.md` C-024) — not this unit's
own problem. Every other test should pass — the cloud sandbox already
confirmed all 28 `scatter-core` tests pass across ten configurations
(GCC/Clang × Release/Debug × tile 4/5, plus GCC+ASan/UBSan at both tile
sizes), so a real failure here on your own machine (DeckLink-adjacent
tests aside) would be genuinely surprising and worth a fresh session
investigating before tagging. **Do not run `./tools/close.sh`** — see
`CORRECTIONS.md` C-024: it treats any `ctest` failure as blocking and
refuses to tag, and the duplex-check exception means it can never succeed
here regardless of which unit is being closed.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/core/types.hpp src/core/binner.cpp src/core/binner.hpp src/core/coarse_shading.hpp src/core/resolve.cpp src/core/resolve.hpp src/core/splat.cpp tests/test_binner.cpp tests/test_kbuffer_resolve.cpp tests/test_kbuffer_storage.cpp tests/test_layered_composite.cpp tests/test_pageturn.cpp tests/test_row_band.cpp tests/test_scan_order_invariance.cpp tests/test_splat.cpp tests/test_zoneplate.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-39: core/types.hpp Frag/AccumCell Y/Cb/Cr -> R/G/B rename (ADR-085)"
git tag -a wu-39-green -m "WU-39: core/types.hpp Frag/AccumCell Y/Cb/Cr -> R/G/B rename (ADR-085)"
git push origin main
git push origin --tags
```

The `rm -f .git/index.lock` is a precaution, not a sign anything is
currently wrong: a stray, empty `index.lock` was found on the real
repository this session (see "Environment check" above) and could not be
removed from the device-bridge shell. If it is already gone by the time
you run this, the `rm -f` is a silent no-op; if it is still there, this
clears it before `git add` needs to create it for real.

This exact list of eighteen paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it should read
exactly sixteen `M` lines for the source/test files above, plus
`M WORK-UNITS.md` and `M HANDOFF.md`, and nothing else. Still worth a
quick `git status --short` yourself before pasting this block, since time
has passed since that check.
