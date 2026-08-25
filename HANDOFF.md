# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 57 (next sequential after Session 56's own `HANDOFF.md`; no
evidence of an intervening session — no later tag, no later commit, no
other `HANDOFF.md` account. WU-41, Phase 9: `sampleBilinear()` reads RGB
directly, `applyShading()` simplifies to a bare per-channel multiply,
`core/pipeline.cpp`'s six RGB-boundary blocks re-derived for the new
RGB-native `SourceRaster`, built.)

**Tag:** `wu-40-red` was the newest real tag at this session's own start
(confirmed directly: `git tag --list 'wu-*' --sort=-creatordate`, `git log
--oneline -8`, `git rev-parse HEAD origin/main`, `git rev-parse
wu-40-red^{commit}` to dereference the annotated tag, `git show wu-40-red
--no-patch` to read its own message — all agreed: `33671e9`, matching
Session 56's own WU-40 commit message exactly, tag message explicitly
saying "red as expected"). `git status --short` at this session's own
start read empty — clean tree. This session's own changes are not yet
committed, tagged or pushed — that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git tag --list 'wu-*' --sort=-creatordate`, `git log --oneline -8`,
`git rev-parse HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was WU-41: make
`sampleBilinear()` read RGB directly and simplify `applyShading()`, per
ADR-085 §6 item 3. Confirmed real repository state directly first (state
(a): `wu-40-red` dereferences to `HEAD` = `origin/main`, clean tree, no
stray lock at session start), then read `SESSION-PROTOCOL.md`,
`HANDOFF.md`, `INVARIANTS.md`, `DECISIONS.md` ADR-085 in full,
`CORRECTIONS.md` (C-001 through C-031, all still current — none of this
session's own findings warranted a new entry, see "Append to
CORRECTIONS.md" below), `WORK-UNITS.md`'s Phase 9 heading and its WU-41
entry (a `todo` stub, per this session's own opening instruction not to
trust it, WU-40's own HANDOFF.md "Next work unit" note, or the stub itself,
as a finished scope), then `src/core/binner.hpp`/`.cpp`,
`src/core/pipeline.cpp`, `src/video/raster.hpp`, `src/video/chroma.hpp`/
`.cpp` directly.

**Re-derived the real scope before writing anything**, per this session's
own opening instruction: `grep -n
'sampleBilinear\|applyShading\|coeffsFor\|ColourStandard'
src/core/binner.hpp src/core/binner.cpp`, `grep -rn 'SourceRaster'
src/core/pipeline.cpp src/core/binner.hpp`, `grep -rln
'RasterRGB\|ycbcrToRgbImage\|rgbToYcbcrImage' src/ tests/` — confirmed
`video::RasterRGB` and the four boundary-conversion functions exist
exactly as WU-40 left them (this unit does not change them, as planned),
and confirmed `core/pipeline.cpp`'s three production `SourceRaster` sites.

**Real `SourceRaster` construction-site count is much larger than three —
a genuine, useful correction of emphasis, not an error in WU-40's own
account.** Per this session's own opening instruction ("do not trust
WU-40's own HANDOFF.md account that there are exactly three... re-confirm
it directly"): `grep -rn 'SourceRaster' src/ tests/ tools/` finds exactly
three real *production* construction sites, all in `core/pipeline.cpp` —
WU-40's own claim was accurate for that specific question. But roughly a
dozen test files, plus `tools/coverage_view_demo.cpp`, also construct a
`SourceRaster` directly from synthetic data, bypassing the chroma boundary
entirely (WU-40's own "every test file... does so directly from synthetic
data, never through this boundary" was also true, and not about the same
question). Renaming `SourceRaster`'s own field names (this unit's job)
breaks every one of those sites at compile time regardless of whether they
go through the boundary, so this session updated all of them — see
`WORK-UNITS.md`'s own WU-41 entry for the full file list. Worth a future
session's attention if `SourceRaster`'s own shape changes again: "which
production functions build one" and "which files construct one at all"
are two different questions, and only the first was three.

**`ColourStandard`/`coeffsFor`'s own eventual shared home (ADR-085 §7):
resolved by deleting them, not promoting them.** Repository-wide grep
before deciding: their only real uses anywhere in the tree were
`core/binner.cpp`'s own `coeffsFor()` and `applyShading()`'s now-deleted
`standard` parameter, plus two `tests/test_binner.cpp` call sites (updated
this session). Once `applyShading()`'s own colour argument is already RGB,
a bare multiply needs no coefficient set — ADR-085 §7's own premise ("once
*both* shading and the I/O boundary need it") no longer holds, since
shading no longer needs one at all. `video/chroma.hpp`'s own RGB boundary
conversion (WU-40) already deliberately hardcodes BT.601 literals rather
than taking this enum; that duplication is unaffected. The enum and
`coeffsFor()` are deleted from `core/binner.hpp`/`.cpp` entirely — nothing
left in the tree needs a *shared* colour-standard type, and keeping one
alive against a hypothetical future caller would be the premature
module-placement decision ADR-085 §7 itself warned against. See
`WORK-UNITS.md`'s own WU-41 entry for the full reasoning; a future unit
that genuinely parameterises the I/O boundary's own coefficient choice can
reintroduce a shared type at that point, against a real caller.

**`core/pipeline.cpp`'s six RGB-boundary blocks: re-derived against the
real code, not implemented from this unit's own prior stub text or WU-40's
own "Next work unit" note — both described a shape that turns out to be
wrong for half the blocks.** Both accounts said, in effect, "delete the
convert-back-to-YCbCr half of each of the six blocks" (three call sites ×
input/output side). That is exactly right for the *input* side:
`chroma::ycbcrToRgbImage()` is kept, its result feeds `SourceRaster`
directly, and the `chroma::rgbToYcbcrImage()` call that used to convert it
straight back to YCbCr is deleted. It is **not** right for the *output*
side. Confirmed by reading `src/core/resolve.cpp` directly before
implementing anything (this file is not in this unit's own file scope, but
reading it was necessary to get the pipeline.cpp wiring right, and reading
a file to understand behaviour is not the same as editing it): PASS 2 is
still WU-42's own job, entirely untouched by this unit, and its normalise
step copies `AccumCell`'s `R`/`G`/`B` fields into a `Raster444`'s
`Y`/`Cb`/`Cr`-named fields *positionally*
(`out.Y = divideRounded(cell.R, cell.w)` etc.), with no channel-meaning
check either side — exactly the channel-agnostic arithmetic ADR-085 itself
describes PASS 2 as. Since PASS 1 now feeds it genuine RGB (this unit's
own change), `runFrame()`'s own `dest` (`warped` in `core/pipeline.cpp`)
comes back holding genuine RGB values *mislabelled* onto a `Raster444`'s
Y/Cb/Cr-named planes, not YCbCr — running the *forward*
`chroma::ycbcrToRgbImage()` call on it first, as the naive symmetric plan
would, would misinterpret already-RGB data as YCbCr and produce garbage.
What each output-side block actually needs, and what this unit ships, is
only the second half: a single `chroma::rgbToYcbcrImage()` call,
reinterpreting `warped`'s own Y/Cb/Cr-named planes as the R/G/B input they
actually hold, producing genuine YCbCr into a fresh `video::Raster444` for
chroma downsample. This mislabelling is known and temporary —
`video::Raster444` itself keeps its genuine YCbCr semantics everywhere
else (ADR-005, unaffected) — WU-42 resolves it for good. See
`core/pipeline.cpp`'s own rewritten file-header comment for the same
account in place. This is exactly the kind of "a scoping stub/prior
session's own account is a plan, not a fact already checked against the
real tree" finding C-027/C-028/C-029 exist to catch, caught here before
implementing the wrong, simpler shape rather than after.

**Files changed, all written to the real repository, re-staged and
grepped to confirm the intended content landed (device-bridge
`device_stage_files` worked without error throughout this session — no
`untrusted_device`/`auth_required` 403, so the fallback described in
`SESSION-PROTOCOL.md`/prior sessions' own accounts was not needed):**

- `src/core/binner.hpp` — `ColourStandard` enum deleted (replaced with a
  comment explaining why); `SourceRaster`'s `y`/`cb`/`cr` pointer fields →
  `r`/`g`/`b`, doc comment rewritten; `shadingStandard` parameter removed
  from all five public entry points, their doc comments updated.
- `src/core/binner.cpp` — `Kcoeffs` struct and `coeffsFor()` deleted;
  `applyShading()` simplified to `Colour{c.r*intensity, c.g*intensity,
  c.b*intensity}`, `standard` parameter removed; `sampleBilinear()`'s own
  local `Colour` struct renamed `y/cb/cr` → `r/g/b`, reads `src.r/g/b`
  directly; `Frag::R/G/B` construction reads `c.r/g/b` directly;
  `shadingStandard` removed from the shared templated loop
  (`generateFragmentsRowRangeImpl`) and all five public wrappers.
- `src/core/pipeline.cpp` — file-header comment rewritten to describe this
  unit's own asymmetric input/output shape (see above); all three
  `SourceRaster`-building functions' input-side blocks now feed
  `SourceRaster` from `video::RasterRGB` directly (the scratch `rgb`
  object promoted out of its old `{ }` scope so it outlives the
  `runFrame()` call `src` points into); all three output-side blocks
  replaced with a single `chroma::rgbToYcbcrImage()` call into a fresh
  `ycbcr` `Raster444`, feeding chroma downsample from that instead of from
  `warped` directly.
- `src/video/chroma.cpp`, `src/video/chroma.hpp` — comment-only: both
  files' own prose referencing `core/binner.cpp`'s `coeffsFor()`/
  `ColourStandard` (now deleted) corrected to past tense with a pointer to
  this unit's own reasoning; no behavioural change, the same "fix a stale
  comment while touching the surrounding context" precedent ADR-082/C-031
  already used.
- `WORK-UNITS.md` — WU-41 entry replaced (was a `todo` stub) with the real
  scope, design decisions, files, grep confirmations and Accept outcome.
- This `HANDOFF.md`.
- Mechanical field-rename only (`.y=`/`.cb=`/`.cr=` → `.r=`/`.g=`/`.b=` on
  `SourceRaster` instances; no fixture *values* changed), needed to keep
  the tree compiling after `SourceRaster`'s field rename, not counted
  against this unit's own file cap: `tests/test_threading.cpp`,
  `test_layered_composite.cpp`, `test_persistent_pool.cpp`,
  `test_pipeline_bytes.cpp`, `test_coverage_capture.cpp`,
  `test_row_band.cpp`, `test_pageturn.cpp`, `test_zoneplate.cpp`,
  `test_scan_order_invariance.cpp`, `test_binner.cpp` (plus its own two
  `ColourStandard::BT601` call-site arguments dropped),
  `test_shapes.cpp`, `test_field_pipeline.cpp`,
  `test_kbuffer_resolve.cpp`, `tools/coverage_view_demo.cpp`.

`INVARIANTS.md` and `DECISIONS.md` are **untouched** this session, per this
unit's own standing constraint — see "Flag for Steve, not resolved here"
below; I7's own wording question WU-40's session raised is still
unresolved as of this session's own read of the real `INVARIANTS.md`.

## Build/test matrix — ten configurations, genuinely red, honestly reported

Run in a fresh `git clone` of `skunge2000/scatter-dve` at `HEAD` `33671e9`
(confirmed matching the real repository before any edit) in this session's
own Linux cloud sandbox, then this session's 19 changed files copied over
and built/tested for real (GCC 13.3.0, Clang 18.1.3 both present):

| Compiler | Build type | `SCATTER_TILE_LOG2` | Result |
|---|---|---|---|
| GCC 13.3.0   | Release | 4 | 25/28 pass — `test_binner`, `test_zoneplate`, `test_pipeline_bytes` fail |
| GCC 13.3.0   | Release | 5 | 25/28 pass — same three fail |
| GCC 13.3.0   | Debug   | 4 | 25/28 pass — same three fail |
| GCC 13.3.0   | Debug   | 5 | 25/28 pass — same three fail |
| Clang 18.1.3 | Release | 4 | 25/28 pass — same three fail |
| Clang 18.1.3 | Release | 5 | 25/28 pass — same three fail |
| Clang 18.1.3 | Debug   | 4 | 25/28 pass — same three fail |
| Clang 18.1.3 | Debug   | 5 | 25/28 pass — same three fail |
| GCC 13.3.0 + ASan/UBSan | Debug | 4 | 25/28 pass — same three fail; no sanitizer findings |
| GCC 13.3.0 + ASan/UBSan | Debug | 5 | 25/28 pass — same three fail; no sanitizer findings |

**Identical failure set in all ten configurations — deterministic, not a
threading or optimisation-level artefact.** All ten compile clean with no
warnings surfaced in the build logs. Both sanitizer runs grepped
specifically for `AddressSanitizer`/`UndefinedBehaviorSanitizer`/"runtime
error" output beyond the expected `doctest` failure lines — none found.

**What actually fails, and why — verified directly against the real
`ctest`/binary output, not predicted in advance and left unchecked:**

- **`test_zoneplate` — 22 of 42537 checks fail, identical in count and
  content to WU-40's own already-red state.** `test_i7_identity_full_pipeline()`
  fails for the same three non-achromatic flat 10-bit codes (`kCode10Min=4`,
  `kCode10Black=64`, `kCode10Max=1019`) at both tested raster sizes, root
  cause unchanged from WU-40's own account (their implied RGB triple clips
  for real). This unit's own re-derived, asymmetric output-side conversion
  produces the *same* result for this fixture as WU-40's original
  symmetric round trip did — a reassuring cross-check, not proof of
  correctness on its own, but consistent with both being exact identities
  for achromatic content and empirically agreeing at the fixture's own
  clipped extremes too. Still `WU-44`'s own job to re-derive this fixture,
  not touched here.
- **`test_binner` — newly fails 2 of 39139 checks, in
  `test_shading_multiply_applies_rgb_round_trip_before_quantisation`.**
  This test's own hand-derived expected values
  (`mirrorToRgbBt601`/`mirrorFromRgbBt601`, an independent
  YCbCr→RGB→multiply→RGB→YCbCr mirror) assume `applyShading()`'s own
  pre-WU-41 round-trip behaviour; the real, simplified `applyShading()`
  (bare multiply, no round trip) genuinely differs now. Not a defect —
  this test's own fixture is exactly what WU-44 exists to re-derive, and
  this session did not shortcut that by loosening or removing the check.
- **`test_pipeline_bytes` — newly fails 3 of 42 checks, all three inside
  its own deinterlaced-path reference-comparison tests**
  (`test_deinterlaced_matches_reference_and_first_push_is_a_noop`,
  `test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace`,
  `test_deinterlaced_sd_geometry_sanity`). Each compares real
  `runFrameBytesDeinterlaced()` output against this test file's own
  hand-rolled `referenceRunFrameBytesDeinterlaced()`/
  `referenceWithExplicitReinterlace()` — neither of which this unit
  touched beyond the mechanical field rename needed to keep them
  compiling. Those references still feed `SourceRaster` directly from
  YCbCr with no RGB boundary conversion at all, so they now diverge from
  real production, which does apply it. Same class of
  reference-implementation staleness `test_zoneplate`'s own I7 check
  already exposed under WU-40 (a hand-rolled reference not updated for
  ADR-085's cutover), now visible in a second file. Re-deriving these two
  reference functions to mirror the real boundary conversion independently
  (WU-34b's own "mirror the math, never call the production function"
  precedent) is `WU-44`'s own job.
- **Stay green, verified why rather than assumed:**
  `test_runframebytes_identity_round_trips_exactly` (the non-deinterlaced
  I7 check via `runFrameBytes()`) and
  `test_deinterlaced_anchor_rows_survive_identity_round_trip` both pass in
  every configuration — neither depends on the stale reference functions
  above, and both exercise achromatic zone-plate content, which this
  unit's own re-derived conversion (like WU-40's) round-trips exactly.
  Every other test (25 of 28) passes in every configuration.

## Flag for Steve, not resolved here

**`INVARIANTS.md`'s I7 text is unchanged since WU-40's own session flagged
it, and this session did not touch `INVARIANTS.md`, per this unit's own
standing constraint — checked directly against the real, current file
before writing this, not assumed either way.** Still reads "Input v210
equals output v210, byte for byte, illegal excursions included... This is
the foundation test," still demonstrably not exact for the same three
non-achromatic flat 10-bit codes WU-40's session found (this session's own
`test_zoneplate` run confirms the identical 22 failing checks, unchanged).
Not a new finding — carried forward exactly as WU-40's own session left
it. Still your own call: reword I7 to scope it, leave it as a known,
tracked exception until `WU-44` re-derives the affected fixtures anyway,
or something else you'd rather do. Not acted on this session either way.

## Where we are

**WU-41 is written, built and delivered — genuinely red in the cloud
sandbox, exactly as ADR-085 §5's standing exception allows, not forced
green and not treated as suspicious for being red.** All 19 changed files
(three production, two comment-only, thirteen test files plus one tool
file per the field-rename note above) plus `WORK-UNITS.md` (WU-41 entry
updated to `red` with the real outcome above) and this `HANDOFF.md` are
written to
the real repository via the device bridge and re-staged/re-grepped to
confirm the intended content landed (see "Environment check" below) — **not
yet committed, tagged or pushed**. That is Steve's own next step, after his
own real-terminal build/test confirms the same outcome (expected to be
25/28, `test_binner`/`test_zoneplate`/`test_pipeline_bytes` the three
failures, since the cloud sandbox already confirmed this across ten
configurations, but per `SESSION-PROTOCOL.md` still worth Steve's own real
run first).

## Next work unit

**`WU-42`** (`src/core/resolve.hpp`/`.cpp`: PASS 2 reshaped from
`Y`/`Cb`/`Cr` to `R`/`G`/`B`) is Phase 9's own next pick, per
`WORK-UNITS.md`'s own dependency ordering (depends on `WU-39`, landed).
Per ADR-085 §5 and this phase's own standing exception, `WU-42` is also not
expected to leave the build green at the end. It inherits a real, useful
head start from this session: `AccumCell`/`Frag` are already `R`/`G`/`B`
(WU-39) and PASS 1 already produces genuine RGB into them (this session),
so `WU-42`'s own job is a rename of `core/resolve.hpp`/`.cpp`'s own
`Y`/`Cb`/`Cr`-shaped local variables, struct fields (`CompositedCell`,
`Background`, `ResolvedCell` — confirm the real current field names
directly, do not assume from this prose) and arithmetic to `R`/`G`/`B`,
with the arithmetic *shape* itself unchanged (ADR-085's own "the arithmetic
shape does not change, only what the three channels mean does"). Once
`WU-42` lands, `core/pipeline.cpp`'s own `warped`/`dest` locals will
genuinely hold `Y`/`Cb`/`Cr`-*shaped-but-actually-RGB* data no more — the
mislabelling this session's own output-side blocks work around
(`chroma::rgbToYcbcrImage()` reinterpreting `warped.Y/Cb/Cr` as `R/G/B`)
will need revisiting once `warped` (or whatever `WU-42` renames it to) is
genuinely `RasterRGB`-shaped rather than a relabelled `Raster444` — flagged
here explicitly so `WU-42`'s own session does not miss that
`core/pipeline.cpp`'s six boundary blocks are a second file this unit's own
reshape has downstream consequences for, beyond `core/resolve.cpp`/`.hpp`
themselves. Also: `WU-44`'s own fixture re-derivation should re-derive
`test_binner.cpp`'s shading-mirror test and `test_pipeline_bytes.cpp`'s two
deinterlaced-path reference functions, both newly red this session, in
addition to `test_zoneplate`'s own already-red fixtures from WU-40.

**Per this session's own standing instruction: do not proceed to WU-42
even with session budget left.** Each Phase 9 unit is knowingly
larger-than-normal (ADR-085 §5) and this session stops here, the same
restraint WU-34b/WU-38/WU-39/WU-40's own sessions used deferring their own
next units.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. ADR-085 §7's
own open sub-questions: I4 already resolved (WU-38); where
`ColourStandard`/`coeffsFor` should live is now resolved (this session —
deleted, not relocated, see above); per-frame boundary-conversion
parameterisation and the fixture-value re-derivation strategy remain open,
`WU-44`'s own concern. The `INVARIANTS.md` I7-wording question under "Flag
for Steve, not resolved here" above remains open, unchanged from WU-40's
own session.

## Blocked / red

**Red, genuinely and expectedly — see Build/test matrix above.**
`test_binner`, `test_zoneplate` and `test_pipeline_bytes` fail identically
in all ten sandbox configurations (2/39139, 22/42537 and 3/42 checks
respectively). Every other test (25 of 28) passes in every configuration.
`ctest` was not run on Steve's own real terminal yet this session — see
"Steve's own next steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang++ --version` directly before
building); the full ten-configuration matrix ran for real, per this
unit's own brief (production code, not docs-only). A fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` was used for the sandbox
build (not a copy staged through the device bridge), confirmed matching
`HEAD 33671e9` before any edit. The device-bridge shell had ordinary
read/write access this session for every file this session touched;
`device_stage_files` worked without error throughout (no
`untrusted_device`/`auth_required` 403 this session, so the fallback
`SESSION-PROTOCOL.md` describes was not needed). Every changed file was
re-staged after writing and grepped for the specific new content
(struct/field names, deleted-symbol comments, the rewritten
`core/pipeline.cpp` blocks) to confirm the write landed; a full
byte-for-byte checksum diff against a separately retained pre-write copy
was not additionally performed, the same trade-off WU-40's own session
made and for the same reason (re-grep after re-stage is the practical
per-file confirmation this device bridge supports). `git status
--short`/`git diff --stat` run directly against the real repository after
writing confirmed exactly the 19 intended files modified and nothing
else (`git diff --stat`'s own output listed above under "Files changed").
**A stray, empty `.git/index.lock` appeared this session** (same as
Sessions 55 and 56's own accounts) — not present at session start
(`ls -la .git/index.lock` returned "No such file or directory" during this
session's own opening verification), but created by an ordinary
`git status`/`git diff` invocation partway through this session and left
behind; the device-bridge shell could not remove it
(`rm -f .git/index.lock` returned "Operation not permitted", unchanged
from the last two sessions). The close-out block below checks for it and
removes it first regardless. No `git commit` or `git push` attempted this
session — nothing pushed. C-024's standing condition (PSU out,
`tools/close.sh` cannot succeed on Steve's own real terminal for any unit)
checked directly against `CORRECTIONS.md` this session and found
unchanged — `./tools/close.sh` must not be run. (No DeckLink target exists
in the cloud sandbox at all, so the duplex-check exception itself does not
arise there.)

## Append to DECISIONS.md

None this session. ADR-085 (WU-38) already covers this unit's own scope;
this unit's own design decisions (deleting rather than relocating
`ColourStandard`/`coeffsFor`; the asymmetric input/output shape for
`core/pipeline.cpp`'s six boundary blocks) are implementation choices
within ADR-085's already-accepted scope, not new architectural decisions —
recorded in `WORK-UNITS.md`'s own WU-41 entry instead, matching how
WU-39/WU-40 recorded their own findings there rather than in a new ADR. No
superseding ADR proposed; no reason this session found to propose one — if
you'd like a dedicated ADR recording the "ColourStandard/coeffsFor
deleted, not relocated" decision specifically (it is a real, if small,
architectural call — reversing ADR-085 §7's own implicit expectation that
somewhere would need a shared home), that's your own call to make, not
assumed here.

## Append to CORRECTIONS.md

None this session. This session's own two "the real scope was bigger/
different than the prior account" findings (the `SourceRaster`
construction-site count; the asymmetric output-side conversion) are not
logged as corrections because neither prior account was actually wrong
about what it claimed — WU-40's "exactly three" was about production
callers specifically and was accurate for that; its own "Next work unit"
sketch of `core/pipeline.cpp`'s six blocks was an upfront, explicitly
tentative plan ("this most likely means..."), not a claim checked against
`core/resolve.cpp` at the time it was written, and this session's own
opening instructions already anticipated it might not survive contact
with the real code ("confirm this plan against the real current code
before implementing it, not against this paragraph"). Both are recorded
in `WORK-UNITS.md`'s own WU-41 entry as re-derivations, matching how
WU-39/WU-40 recorded their own re-scoping there.

## Closed out this session

**WU-41 — `sampleBilinear()` reads RGB directly, `applyShading()`
simplifies to a bare per-channel multiply, `ColourStandard`/`coeffsFor`
deleted, `core/pipeline.cpp`'s six RGB-boundary blocks re-derived for the
new asymmetric input/output shape. Built, genuinely red in all ten
cloud-sandbox configurations (`test_binner`/`test_zoneplate`/
`test_pipeline_bytes`, 2/39139 + 22/42537 + 3/42 checks, understood and
documented root causes), not yet committed.** Five production files plus
two comment-only files (listed above under "Files changed"), fourteen
test/tool files (mechanical field-rename only), `WORK-UNITS.md` (WU-41
entry updated to `red`), this `HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **this
unit is expected to be red, not green**:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect `test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check to fail regardless (the standing PSU/two-device exception,
`DECISIONS.md` ADR-034/035/037, `CORRECTIONS.md` C-024) — not this unit's
own problem. Expect `test_binner`, `test_zoneplate` and
`test_pipeline_bytes` to also fail — specifically 2, 22 and 3 of their own
checks respectively, all traced above. Every other test should pass — the
cloud sandbox already confirmed 25/28 `scatter-core` tests pass across ten
configurations, so a real failure anywhere other than those four named
tests (DeckLink-adjacent aside) would be genuinely surprising and worth a
fresh session investigating before tagging.
**Do not run `./tools/close.sh`** — see `CORRECTIONS.md` C-024: it treats
any `ctest` failure as blocking and refuses to tag, and this unit is
*expected* to fail `ctest`, on top of the standing duplex-check exception.

**Once you've confirmed the build succeeds and `ctest`'s failures are
exactly the four expected ones (duplex check, `test_binner`,
`test_zoneplate`, `test_pipeline_bytes`)**, close out with the **manual tag
path** (`close.sh` cannot be used for a red unit regardless of the duplex
exception):

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/core/binner.hpp src/core/binner.cpp src/core/pipeline.cpp src/video/chroma.hpp src/video/chroma.cpp tests/test_threading.cpp tests/test_layered_composite.cpp tests/test_persistent_pool.cpp tests/test_pipeline_bytes.cpp tests/test_coverage_capture.cpp tests/test_row_band.cpp tests/test_pageturn.cpp tests/test_zoneplate.cpp tests/test_scan_order_invariance.cpp tests/test_binner.cpp tests/test_shapes.cpp tests/test_field_pipeline.cpp tests/test_kbuffer_resolve.cpp tools/coverage_view_demo.cpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-41: sampleBilinear reads RGB, applyShading simplifies, pipeline boundary re-derived (ADR-085); red, see HANDOFF.md"
git tag -a wu-41-red -m "WU-41: RGB-native sampleBilinear/applyShading (ADR-085); test_binner/test_zoneplate/test_pipeline_bytes red as expected, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-41-red`, not `wu-41-green`** — matching the naming
choice you made for `wu-40-red` (this project's own tag convention,
`wu-NN-green`, had never named a red unit before that one); rename it to
whatever you'd prefer before running the command if you'd rather keep the
`-green` suffix regardless of colour, or use a different scheme entirely.
The `rm -f .git/index.lock` is a precaution, not a sign anything is
currently wrong: a stray, empty `index.lock` appeared on the real
repository partway through this session (see "Environment check" above)
and could not be removed from the device-bridge shell. If it is already
gone by the time you run this, the `rm -f` is a silent no-op; if it is
still there, this clears it before `git add` needs to create it for real.

This exact list of 21 paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it should read
exactly 21 `M` lines: the 19 source/test/tool files above (three
production, two comment-only, thirteen test files, one tool file), plus
`WORK-UNITS.md` and `HANDOFF.md`, and nothing else. Still worth a quick
`git status
--short` yourself before pasting this block, since time has passed since
that check.
