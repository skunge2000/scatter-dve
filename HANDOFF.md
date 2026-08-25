# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 56 (next sequential after Session 55's own `HANDOFF.md`; no
evidence of an intervening session — no later tag, no later commit, no
other `HANDOFF.md` account. WU-40, Phase 9: RGB boundary conversion in
`src/video/chroma.hpp`/`.cpp` plus a new `video::RasterRGB` container, wired
into `core/pipeline.cpp`'s three real production callers, built.)

**Tag:** `wu-39-green` was the newest real tag at this session's own start
(confirmed directly: `git tag --list 'wu-*' --sort=-creatordate`, `git log
--oneline -8`, `git rev-parse HEAD origin/main`, `git cat-file -t
wu-39-green` + `git rev-parse wu-39-green^{commit}` to dereference the
annotated tag — all three equal `db20a77`, matching Session 55's own WU-39
commit message exactly). `git status --short` at this session's own start
read empty — clean tree. This session's own changes are not yet committed,
tagged or pushed — that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git tag --list 'wu-*' --sort=-creatordate`, `git log --oneline -8`,
`git rev-parse HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This session in full

Opened with a continuation prompt whose own job was WU-40: add the RGB
boundary conversion ADR-085 describes, at the v210/chroma boundary, both
directions. Confirmed real repository state directly first (state (a):
`wu-39-green` dereferences to `HEAD` = `origin/main`, clean tree, no stray
lock at session start), then read `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`INVARIANTS.md`, `DECISIONS.md` ADR-085 in full, `CORRECTIONS.md` (C-001
through C-031, all still current), `WORK-UNITS.md`'s Phase 9 heading and
its WU-40 entry (a `todo` stub, per this session's own opening instruction
not to trust it as a finished scope), then `src/video/raster.hpp`,
`v210.hpp`/`.cpp`, `chroma.hpp`/`.cpp` and `src/core/types.hpp` directly.

**Re-derived the real scope before writing anything**, per this session's
own opening instruction: `grep -rln 'v210'`/`'chroma'`/`'Raster444\|Raster422'`
across `src/`/`tests/` (broad, as expected — these are load-bearing types
touched by many files); confirmed directly against the real
`src/video/raster.hpp` that `Raster444`/`Raster422` still carry their own
`.Y/.Cb/.Cr` field names, untouched by WU-39's rename (a distinct struct —
matches WU-40's own stub text exactly, checked rather than assumed).

**This unit's own first design questions, resolved this session:**

1. **New RGB container shape: a new struct, `video::RasterRGB`**
   (`src/video/raster.hpp`), not a repurposed `Raster444`. Same reasoning
   WU-39 already used for `Frag`/`AccumCell`: `CORRECTIONS.md` (C-027
   onward) has repeatedly found bugs from two differently-typed things
   sharing one accessor spelling, and `Raster444` needs to keep meaning
   YCbCr unconditionally (it is still what `chroma::upsampleImage`/
   `downsampleImage` produce/consume either side of the new conversion).
2. **Coefficients: hardcoded BT.601** (`Kr=0.299, Kg=0.587, Kb=0.114`),
   duplicated as literals directly in `chroma.cpp`, not read from
   `core/binner.hpp`'s `ColourStandard`/`coeffsFor()`. Deliberate, not an
   oversight: ADR-085 §7 explicitly leaves "where `ColourStandard`/
   `coeffsFor` should live once both shading and the I/O boundary need
   them" open, for whoever starts WU-41 — this session does not decide it,
   and does not invert this project's own core-depends-on-video layering
   (every existing include edge runs core → video, never the reverse) to
   borrow it early. Matches every real caller's own existing default.
3. **I2's clip site: confirmed unaffected, as the stub flagged.** The new
   conversion's own quantisation clamps to `Sample`'s own representable
   range `[0, 65535]` only — the same container-limit clamp
   `core/binner.cpp`'s `applyShading()`/`toSample()` already use for
   exactly this reason, duplicated here for the same layering reason as
   the coefficients above — never I2's v210 protocol clamp
   (`[kCode10Min, kCode10Max]`), which stays exactly where it already is,
   at `v210::packRow` alone.

**Wiring: real, not inert.** Grepped the whole repository for every real
`SourceRaster` construction site before deciding where to wire the new
conversion in (C-027/C-028/C-029's own "a scoping stub is a plan, not a
fact" lesson) — exactly three, all in `src/core/pipeline.cpp`
(`runFrameBytes()`, `runFrameBytesDeinterlaced()`, `runFrameFile()`); every
test file that builds its own `SourceRaster` does so directly from
synthetic data, never through this boundary. Wired the new conversion into
all three, on both sides: input side, immediately before `SourceRaster` is
built (after `deinterlacer.push()` in the deinterlaced path — deinterlace
itself still runs on genuine, unperturbed chroma-upsampled YCbCr,
unaffected); output side, immediately after `runFrame()` returns, before
chroma downsample. The round trip goes YCbCr → RGB → YCbCr at each site,
not YCbCr → RGB → (into `runFrame()`): `SourceRaster`/`sampleBilinear()`
still expect YCbCr-labelled `Raster444` planes (`WU-41`'s own job to
change that), so this unit's own job is limited to proving the new
conversion is genuinely wired in and exercised at the real boundary, not
to reshaping what runs between the two conversions — recorded explicitly
in `core/pipeline.cpp`'s own new file-header comment so a future session
does not mistake the round trip for a stray no-op.

**Files changed, all written to the real repository, re-staged and grepped
to confirm the intended content landed (see "Environment check" below for
why full byte-checksum diffing was skipped this session):**

- `src/video/raster.hpp` — new `struct RasterRGB` (R/G/B planes, same
  shape/constructor/`Plane` accessor convention as `Raster444`), added
  after `Raster444`'s own definition, with a comment explaining why it is
  a new struct and directly answering WU-40's own stub design question.
- `src/video/chroma.hpp` — new declarations: `ycbcrToRgbRow`/`Image`,
  `rgbToYcbcrRow`/`Image`, with a long design comment covering the
  algebra (identical derivation to `core/binner.cpp`'s `applyShading()`),
  coefficient choice and why it's not shared code, quantisation and why
  it's not I2's clamp, threading (none needed, stateless per pixel), and
  preconditions.
- `src/video/chroma.cpp` — implementation: a private `kKr`/`kKg`/`kKb`
  triple and `toSampleClamped()` helper, then the four functions, matching
  the file's own existing plain-scalar, `noexcept`, row-then-image style.
  New includes: `<algorithm>`, `<cmath>`, `<limits>` (for `std::clamp`,
  `std::round`, `std::numeric_limits`).
- `src/core/pipeline.cpp` — file-header comment extended to describe the
  new step; all three `SourceRaster`-building functions get the round
  trip on both sides (six new call-site blocks total, each building a
  scratch `video::RasterRGB` and converting through it).
- `src/core/resolve.hpp` — comment-only: the "complete signal path" prose
  ahead of `runFrameBytes()`/`runFrameBytesDeinterlaced()`/`runFrameFile()`'s
  own declarations updated to mention the new step; the
  `runFrameFile()` doc comment's own I7 claim specifically qualified with
  a direct account of which cases the real test now finds non-exact and
  why (see Build/test below) — not removed, since I7 still holds for
  every case except the three non-achromatic flat codes, and the
  distinction is worth stating precisely rather than hand-waved.
- `WORK-UNITS.md` — WU-40 entry replaced (was a `todo` stub) with the real
  scope, design decisions, files, grep confirmations and Accept outcome.
- This `HANDOFF.md`.

`INVARIANTS.md` and `DECISIONS.md` are **untouched** this session, per
this unit's own standing constraint — see "Flag for Steve, not resolved
here" below for why `INVARIANTS.md`'s I7 text is worth Steve's own
attention regardless.

## Build/test matrix — ten configurations, genuinely red, honestly reported

Run in a fresh `git clone` of `skunge2000/scatter-dve` at `HEAD` `db20a77`
(confirmed matching the real repository before any edit) in this session's
own Linux cloud sandbox, then this session's five changed source files
copied over and built/tested for real (GCC 13.3.0, Clang 18.1.3 both
present):

| Compiler | Build type | `SCATTER_TILE_LOG2` | Result |
|---|---|---|---|
| GCC 13.3.0   | Release | 4 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| GCC 13.3.0   | Release | 5 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| GCC 13.3.0   | Debug   | 4 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| GCC 13.3.0   | Debug   | 5 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| Clang 18.1.3 | Release | 4 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| Clang 18.1.3 | Release | 5 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| Clang 18.1.3 | Debug   | 4 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| Clang 18.1.3 | Debug   | 5 | 27/28 pass — `test_zoneplate` fails (22/42537 checks) |
| GCC 13.3.0 + ASan/UBSan | Debug | 4 | 27/28 pass — `test_zoneplate` fails (22/42537 checks); no sanitizer findings |
| GCC 13.3.0 + ASan/UBSan | Debug | 5 | 27/28 pass — `test_zoneplate` fails (22/42537 checks); no sanitizer findings |

**Identical failure in all ten configurations — deterministic, not a
threading or optimisation-level artefact.** All ten compile clean with no
warnings surfaced in the build logs. Both sanitizer runs grepped
specifically for `AddressSanitizer`/`UndefinedBehaviorSanitizer`/"runtime
error" output beyond the expected `doctest` failure lines — none found.

**What actually fails, and why — verified directly against the real
`ctest`/binary output, not predicted in advance and left unchecked:**
`test_zoneplate.cpp`'s `test_i7_identity_full_pipeline()` (which drives
`runFrameFile()`) fails for three of its four `chromaExpectedExact=true`
flat-pattern codes — `kCode10Min` (4), `kCode10Black` (64), `kCode10Max`
(1019) — at both tested raster sizes (8×2, 128×65); `kCode10ChromaZero`
(512) stays exact. `test_zoneplate.cpp`'s own `makeFlat()` sets
`Cb=Cr=Y` (the same 10-bit code on all three planes) — achromatic (zero
chroma delta from `kChromaZero`) only for code 512. The other three codes
are flat but not achromatic; their implied RGB triple falls outside
`Sample`'s representable range and clips for real on the forward
conversion — e.g. code 4: `Y=Cb=Cr=256`, chroma delta `256 - 32768 =
-32512`, implied `R ≈ 256 + 2×0.701×(-32512) ≈ -45334`, clamped to 0. This
also perturbs luma (`test_zoneplate.cpp:209`, not just `:212`/`:213`),
which the test's own pre-existing comment ("luma never enters a chroma
filter... must survive exactly, always") did not anticipate, because that
comment predates a conversion step that couples Y to chroma — worth a
future session's attention if that comment is ever revisited, though this
session did not edit `test_zoneplate.cpp` itself (fixture/test-content
re-derivation is `WU-44`'s own explicit job, not cut short here to force
a false green, per ADR-085 §5).

**`test_pipeline_bytes.cpp` stays green, verified why rather than assumed:**
its own I7 check (`runFrameBytes()`) and its `referenceRunFrameBytesDeinterlaced()`
byte-exact cross-check both use `testpat::makeZonePlate()`, whose own file
header holds chroma flat at exactly `kChromaZero` — the one flat-chroma
case that is a genuine algebraic fixed point of the new conversion
regardless of luma's own (non-flat, genuine zone-plate) value: achromatic
input makes `R=B=Y` exactly and `G` round to `Y` for any `Y`, confirmed
directly against the conversion's own arithmetic, not merely observed to
pass by coincidence. `test_chroma.cpp`, `test_chroma_neon.cpp` and
`test_ramp_roundtrip.cpp` call `chroma::upsampleImage`/`downsampleImage`
directly, never through `core/pipeline.cpp`'s orchestration, so they never
see the new conversion at all — confirmed by grep. No other test touched
or affected; the DeckLink-linked production caller
(`src/io/decklink_capture_consumer.cpp`) does not assert byte-exactness on
its own output, only consumes it — confirmed by grep, not built here (no
`BLACKMAGIC_SDK_DIR` in this sandbox, matching every prior session).

## Flag for Steve, not resolved here

**`INVARIANTS.md`'s I7 text ("Input v210 equals output v210, byte for
byte, illegal excursions included... This is the foundation test") is now
demonstrably not exact for three specific inputs, and this session did
not touch `INVARIANTS.md`, per this unit's own standing constraint.**
ADR-085 itself already anticipated this in general terms ("I7 in
particular becomes the critical regression test for this whole migration
and needs re-proving against the new representation, not assumed to still
hold") — so this is not a surprise or an error, but I7's own frozen text
is now stated more absolutely than what this session found to actually be
true, and I do not think it is my call to reword it. Worth your own
decision: reword I7 to scope it (e.g. "for content whose implied RGB stays
within [0, 65535] on this conversion" or similar), leave it as-is and treat
the gap as a known, tracked exception until `WU-44` re-derives the
affected fixtures anyway, or something else you'd rather do. Not acted on
this session either way.

## Where we are

**WU-40 is written, built and delivered — genuinely red in the cloud
sandbox, exactly as ADR-085 §5's standing exception allows, not forced
green and not treated as suspicious for being red.** All five changed
files (`src/video/raster.hpp`, `chroma.hpp`, `chroma.cpp`,
`core/pipeline.cpp`, `core/resolve.hpp`) plus `WORK-UNITS.md` (WU-40 entry
updated to `red` with the real outcome above) and this `HANDOFF.md` are
written to the real repository via the device bridge and re-staged to
confirm the intended content landed (see "Environment check" below) —
**not yet committed, tagged or pushed**. That is Steve's own next step,
after his own real-terminal build/test confirms the same outcome (expected
to be 27/28, `test_zoneplate` the one failure, since the cloud sandbox
already confirmed this across ten configurations, but per
`SESSION-PROTOCOL.md` still worth Steve's own real run first).

## Next work unit

**`WU-41`** (`src/core/binner.cpp`/`.hpp`: `sampleBilinear()` reads RGB;
`applyShading()` simplifies) is Phase 9's own next pick, per
`WORK-UNITS.md`'s own dependency ordering (depends on `WU-39`, `WU-40`,
both now landed). Per ADR-085 §5 and this phase's own standing exception,
`WU-41` is also not expected to leave the build green at the end. It
inherits a real, useful head start from this session: `video::RasterRGB`
already exists and is already exercised at the boundary, so `WU-41`'s own
job is narrower than it might otherwise have been — change
`SourceRaster`'s own `y`/`cb`/`cr` pointers to `r`/`g`/`b` (or similar),
change `sampleBilinear()` to read them directly, and in `core/pipeline.cpp`
delete the "convert back to YCbCr immediately" half of each of this
session's six new blocks, feeding the `RasterRGB` straight into
`SourceRaster` instead — the conversion functions themselves,
`ycbcrToRgbImage()`/`rgbToYcbcrImage()`, should not need to change. Also:
`ColourStandard`/`coeffsFor()`'s own eventual shared home (ADR-085 §7,
this session's own "Coefficients" note above) is explicitly `WU-41`'s own
first design question now, not decided here.

**Per this session's own standing instruction: do not proceed to WU-41
even with session budget left.** Each Phase 9 unit is knowingly
larger-than-normal (ADR-085 §5) and this session stops here, the same
restraint WU-34b/WU-38/WU-39's own sessions used deferring their own next
units.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. ADR-085 §7's
own open sub-questions: I4 already resolved (WU-38); where
`ColourStandard`/`coeffsFor` should live is now explicitly `WU-41`'s own
first design question (this session's own "Coefficients" note above);
per-frame boundary-conversion parameterisation and the fixture-value
re-derivation strategy remain open, `WU-41`/`WU-44`'s own concern. New
this session: the `INVARIANTS.md` I7-wording question under "Flag for
Steve, not resolved here" above.

## Blocked / red

**Red, genuinely and expectedly — see Build/test matrix above.**
`test_zoneplate` fails identically in all ten sandbox configurations (22
of 42537 checks, `test_i7_identity_full_pipeline()`, three non-achromatic
flat codes). Every other test passes in every configuration. `ctest` was
not run on Steve's own real terminal yet this session — see "Steve's own
next steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang++ --version` directly before
building); the full ten-configuration matrix ran for real, per this
unit's own brief (production code, not docs-only). A fresh `git clone` of
`https://github.com/skunge2000/scatter-dve.git` was used for the sandbox
build (not a copy staged through the device bridge), confirmed matching
`HEAD db20a77` before any edit. The device-bridge shell had ordinary
read/write access this session for every file this session touched,
confirmed by re-staging each of the five changed files after writing them
and grepping the re-staged copies for the specific new content (struct/
function names, wiring-block comments) rather than a full line-for-line
diff against a separately retained pre-write copy, which this session did
not keep — `device_stage_files` itself worked without error throughout
(no `untrusted_device`/`auth_required` 403 this session, unlike the
session referenced in this unit's own opening instructions — that fallback
was not needed). `git status --short`/`git diff --stat` run directly
against the real repository after writing confirmed exactly the five
intended files modified and nothing else. **A stray, empty
`.git/index.lock` was found this session** (same as Session 55's own
account) while running `git status --short` — the device-bridge shell
could not remove it (`rm`/`unlink` on a bridge-mounted file returns
"Operation not permitted" there by design, unchanged from last session).
The close-out block below checks for it and removes it first regardless.
No `git commit` or `git push` attempted this session — nothing pushed.
C-024's standing condition (PSU out, `tools/close.sh` cannot succeed on
Steve's own real terminal for any unit) checked directly against
`CORRECTIONS.md` this session and found unchanged — `./tools/close.sh`
must not be run. (No DeckLink target exists in the cloud sandbox at all,
so the duplex-check exception itself does not arise there.)

## Append to DECISIONS.md

None this session. ADR-085 (WU-38) already covers this unit's own scope;
this unit's own design decisions (new `RasterRGB` struct, hardcoded BT.601,
round-trip-back wiring shape, I2 clip-site confirmation) are implementation
choices within ADR-085's already-accepted scope, not new architectural
decisions — recorded in `WORK-UNITS.md`'s own WU-40 entry instead, matching
how WU-39 recorded its own findings there rather than in a new ADR. No
superseding ADR proposed.

## Append to CORRECTIONS.md

None this session. Nothing this session found was a prior session's
error — ADR-085 itself already anticipated that I7 would need re-proving
under the new representation (quoted above under "Flag for Steve"), so
this session's own finding confirms that anticipation rather than
correcting a mistaken claim.

## Closed out this session

**WU-40 — RGB boundary conversion (`video::RasterRGB`, `chroma::ycbcrToRgbImage()`/
`rgbToYcbcrImage()`), wired into all three real `core/pipeline.cpp`
callers, both directions. Built, genuinely red in all ten cloud-sandbox
configurations (`test_zoneplate` only, 22/42537 checks, understood and
documented root cause), not yet committed.** Five source files (listed
above under "Where we are"), `WORK-UNITS.md` (WU-40 entry updated to
`red`), this `HANDOFF.md`.

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
own problem. Expect `test_zoneplate` to also fail — specifically 22 of its
checks, all inside `test_i7_identity_full_pipeline()`, all traced above to
the three non-achromatic flat 10-bit codes (4, 64, 1019). Every other test
should pass — the cloud sandbox already confirmed 27/28 `scatter-core`
tests pass across ten configurations, so a real failure anywhere other
than those two named tests (DeckLink-adjacent aside) would be genuinely
surprising and worth a fresh session investigating before tagging.
**Do not run `./tools/close.sh`** — see `CORRECTIONS.md` C-024: it treats
any `ctest` failure as blocking and refuses to tag, and this unit is
*expected* to fail `ctest`, on top of the standing duplex-check exception.

**Once you've confirmed the build succeeds and `ctest`'s failures are
exactly the two expected ones (duplex check, `test_zoneplate`)**, close
out with the **manual tag path** (`close.sh` cannot be used for a
red unit regardless of the duplex exception):

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/video/raster.hpp src/video/chroma.hpp src/video/chroma.cpp src/core/pipeline.cpp src/core/resolve.hpp WORK-UNITS.md HANDOFF.md
git commit -m "WU-40: RGB boundary conversion at v210/chroma boundary, both directions (ADR-085); red, see HANDOFF.md"
git tag -a wu-40-red -m "WU-40: RGB boundary conversion (ADR-085); test_zoneplate red as expected, see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is `wu-40-red`, not `wu-40-green`** — this project's own tag
convention (`wu-NN-green`, every prior tag) has never named a red unit
before; `-red` here is this session's own suggestion to keep the tag
honest about what it points at, not an established convention — rename it
to whatever you'd prefer before running the command if you'd rather keep
the `-green` suffix regardless of colour, or use a different scheme
entirely. The `rm -f .git/index.lock` is a precaution, not a sign anything
is currently wrong: a stray, empty `index.lock` was found on the real
repository this session (see "Environment check" above) and could not be
removed from the device-bridge shell. If it is already gone by the time
you run this, the `rm -f` is a silent no-op; if it is still there, this
clears it before `git add` needs to create it for real.

This exact list of seven paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it should read
exactly five `M` lines for the source files above, plus `M WORK-UNITS.md`
and `M HANDOFF.md`, and nothing else. Still worth a quick `git status
--short` yourself before pasting this block, since time has passed since
that check.
