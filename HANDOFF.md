# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 58 (a narrow, immediate follow-up on top of Session 57's own
`wu-41-red`, not a new numbered work unit — WU-41 itself stays the current
unit; this session patches one production bug found in it right after its
own close-out, at Steve's own explicit direction, and hands off again with
WU-42 still next.)

**Tag:** `wu-41-red` was the newest real tag at this session's own start
(confirmed directly: `git log --oneline -3`, `git tag -l | tail`, `git
fetch origin && git rev-parse HEAD origin/main` — both `6cb8818`, matching
Session 57's own WU-41 commit exactly; `git rev-parse wu-41-red` returns
the *annotated tag object*'s own SHA, `dd2c456`, which is expected and not
a mismatch — `git describe --tags --exact-match HEAD` resolves through it
and correctly prints `wu-41-red`). `git status --short` at this session's
own start read empty — clean tree, Session 57's own work already
committed, tagged and pushed by Steve, exactly as he reported. This
session's own one-file change is not yet committed, tagged or pushed —
that is Steve's own next step, below.

## Before doing anything else in the next session

Run `git log --oneline -3`, `git tag -l | tail`, `git fetch origin && git
rev-parse HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This session in full

Opened as a live follow-up in the same conversation as Session 57 (WU-41),
not a fresh continuation prompt: Steve reported "I now have a cyan/blue
background on test-sphere" — `tests/test_decklink_live_sphere.cpp`, a live
DeckLink demo he runs on his own Mac against real capture hardware, never
built or run in this project's own cloud sandbox (no `BLACKMAGIC_SDK_DIR`
there, and this repo's own ten-configuration matrix does not include it).

Re-verified real state first, per this project's own standing discipline,
rather than trusting Session 57's own account or Steve's own summary of
what he'd done: `HEAD` and `origin/main` both `6cb8818`, `wu-41-red`
dereferences to it, `git status --short` empty, no `.git/index.lock`
present yet. Confirmed Session 57's own close-out really had landed before
touching anything further.

**Root cause, traced directly against the real, already-tagged code** (not
assumed from Session 57's own account): `core/resolve.hpp`'s `Background`/
`kDefaultBackground` — read in full this session (it was only read to line
140 live, mid-diagnosis, before this session formally opened; read in full
here, along with `core/resolve.cpp` in full, satisfying this project's own
"never edit a file you have not been shown this session" rule before
touching it) — still held its pre-ADR-085 value:
`{Y = kBlack, Cb = kChromaZero, Cr = kChromaZero}`, genuine YCbCr legal
black, untouched by WU-39/40/41 because `core/resolve.hpp`/`.cpp` are
explicitly named as `WU-42`'s own future scope in every one of those
units' own accounts.

`resolve.cpp`'s `composite()` writes `bg`'s three fields, completely
unconverted, into any destination cell with `cell.w <= 0` (uncovered) or
partial coverage (`blend()`, a per-channel convex blend against `bg`).
WU-41's own `core/pipeline.cpp` output-side change (Session 57) applies one
blanket `chroma::rgbToYcbcrImage()` reinterpretation to *all* of `warped`'s
`Y`/`Cb`/`Cr`-named content on the way out, correct for every pixel whose
colour actually came from a splatted source fragment (genuine RGB, by
construction, since WU-39/WU-41) — but `Background`'s own fallback content
is a second, independent source feeding that same buffer, one Session 57's
own repo-wide grep never checked because it was framed as "every caller of
`SourceRaster`/every reader of `core/pipeline.cpp`'s own change," not
"every writer into `warped`, from any source." See `CORRECTIONS.md` C-032
for the general lesson, logged this session.

**Verified numerically**, not just reasoned about: a small standalone
program (`/tmp/verify_bg.cpp`, this session's own cloud sandbox, not
committed — links `core/resolve.cpp` + `video/chroma.cpp` directly, no
test harness) runs `composite(AccumCell{}, bg)` (an uncovered cell) through
the exact same `chroma::rgbToYcbcrImage()` reinterpretation
`core/pipeline.cpp`'s real output-side code applies:

- **Old value** (`Cb = Cr = kChromaZero`, `wu-41-red`'s real shipped
  behaviour): final genuine YCbCr comes out `Y=24195, Cb=37606, Cr=18432`
  — Cb well above `kChromaZero` (32768), Cr well below it: exactly the
  cyan/blue tint Steve reported, reproduced outside the live-hardware path
  entirely.
- **New value** (`Cb = Cr = kBlack`, this session's own fix): final
  genuine YCbCr comes out `Y=4096, Cb=32768, Cr=32768` — exactly legal
  black, matching `kBlack`/`kChromaZero`/`kChromaZero` (the same numeric
  constants, now reached via the RGB round trip rather than assigned
  directly).

**Fix**, `core/resolve.hpp` only: `Background`'s default-member
initialisers changed from `Sample Y = kBlack, Cb = kChromaZero, Cr =
kChromaZero;` to `Sample Y = kBlack, Cb = kBlack, Cr = kBlack;` — per I3's
own new text ("R, G and B are all full-range... no channel needs a
mid-point offset any more"), black is `kBlack` on every channel once every
channel is genuinely RGB. `kDefaultBackground` (a `constexpr` default
member initialiser away) and `PipelineParams::background`'s own default
(`= kDefaultBackground`) both pick this up automatically — no other edit
needed for the fix itself. The struct's own field names (`Y`/`Cb`/`Cr`)
are untouched — that rename is still `WU-42`'s own job, not started here;
this session changes only the *value*, an explicit, narrow exception to
WU-41's own file scope, authorised directly by Steve.

Grepped the whole repository for every other reader/setter of
`Background`/`kDefaultBackground` before considering the fix complete:
`tests/test_layered_composite.cpp` (two uses of `kDefaultBackground`
directly) both compute their own expected values dynamically from the same
`bg` variable at test time (`composite(lower, bg)`, `expectedBlend(...)`)
rather than a hardcoded number baked in from the old constant — unaffected
by the value change, still correct either way.
`tests/test_zoneplate.cpp:436` and `tests/test_shapes.cpp:367` each
construct their own explicit, independent `Background` (one a bare
`{kBlack, kChromaZero, kChromaZero}` literal used only as an arbitrary
pure-blend-math fixture for the I5 fringe test, unrelated to
`kDefaultBackground`; one a custom `fromCode10(...)` triple) — neither
references the constant, neither needed touching. No other production or
test file constructs a `Background` or reads `kDefaultBackground`.

## Build/test matrix — ten configurations, re-run against this one-file fix

Fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` into
the cloud sandbox (not a copy of Session 57's own leftover, uncommitted
working tree, which was still sitting in this sandbox from before — used
deliberately as the verified starting point instead, confirmed at
`HEAD = wu-41-red = 6cb8818` before any edit), the same `core/resolve.hpp`
change applied, diffed against the file already written to Steve's own
real repository (`git diff --stat` — one file, 25 insertions, 3 deletions
— matches exactly). Same GCC 13.3.0 / Clang 18.1.3 × Release/Debug ×
`SCATTER_TILE_LOG2` 4/5, plus GCC + ASan/UBSan at both tile sizes, all
configured, built and `ctest`-run for real.

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

Per-test check counts, identical across all ten configurations and
identical to Session 57's own matrix (this fix changes no test's outcome):

- `test_binner`: 2 of 39139 checks fail (`test_binner.cpp:794`,
  `:795` — the pre-existing, already-documented shading-mirror-fixture
  staleness, `WU-44`'s own job).
- `test_zoneplate`: 22 of 42537 checks fail (`test_zoneplate.cpp:209`,
  `:212`, `:213` — the pre-existing, already-documented I7 non-achromatic
  round-trip breakage, ADR-085 §5's own accepted phase-9 exception).
- `test_pipeline_bytes`: 3 of 42 checks fail
  (`test_pipeline_bytes.cpp:406`, `:452`, `:552` — the pre-existing,
  already-documented deinterlaced-path reference-function staleness,
  `WU-44`'s own job).

Every other test (25 of 28) passes in every configuration, exactly as
Session 57 left it — **this fix is orthogonal to the standing WU-41 red
state**: it touches only `Background`'s compositing-fallback path
(uncovered/partially-covered destination cells), none of which any of
these three failing tests' own checks exercise (all three fail on genuine,
fully-covered source content going through the RGB round trip, not on
background pixels).

## Flag for Steve, not resolved here

**Carried forward unchanged from Session 57 (and Session 56 before it):**
I7 ("Input v210 equals output v210, byte for byte, illegal excursions
included") is not exactly true post-ADR-085 for non-achromatic flat
content — `test_zoneplate.cpp`'s own `test_i7_identity_full_pipeline()` is
the check that surfaces this, and it is one of the three tests already
counted as red above. `INVARIANTS.md`'s own I7 wording has still not been
revisited to say so explicitly — flagged again, not touched (this
project's own standing rule: flag concerns about `INVARIANTS.md`, never
edit it directly).

**New this session:** none. The cyan/blue background bug itself is now
fixed, not merely flagged — see above.

## Where we are

WU-41 (Phase 9, `sampleBilinear()`/`applyShading()` RGB-native,
`core/pipeline.cpp`'s boundary conversion) is closed, tagged `wu-41-red`,
pushed — and now has this session's own narrow follow-up fix to
`core/resolve.hpp`'s `kDefaultBackground`, an explicit, acknowledged
exception to that unit's own file scope, not a reopening of it. `WU-42`
(PASS 2 reshaped to `R`/`G`/`B`) is still next, genuinely unstarted beyond
this one field's *value* (its own field names, and the rest of
`core/resolve.cpp`/`.hpp`'s reshape, remain entirely open).

## Next work unit

**WU-42 — `src/core/resolve.hpp`/`.cpp`: PASS 2 reshaped to R/G/B.**
Unchanged from Session 57's own account, with one addition: this session's
own fix already moved `Background`'s *value* into the RGB domain
(`kBlack` on every channel), so `WU-42` inherits a `Background` whose
default is already correct for genuine RGB — its own remaining job is the
field-name rename (`Y`/`Cb`/`Cr` → `R`/`G`/`B`, matching WU-39's own
`Frag`/`AccumCell` precedent and WU-41's own `SourceRaster` precedent) and
the rest of PASS 2's reshape (`normaliseCell()`, `composite()`,
`compositeLayered()`, `compositeKBuffer()`'s own internal Y/Cb/Cr-named
locals and doc comments), not a second look at what the constant should
equal. `core/pipeline.cpp`'s output-side block (the single
`rgbToYcbcrImage()` call this session's own predecessor, WU-41, built)
will also need revisiting once `warped` becomes genuinely RGB-shaped by
construction rather than RGB-mislabelled-as-YCbCr — flagged already in
Session 57's own account, unchanged here.

## Open questions

None new this session.

## Blocked / red

**Red, genuinely and expectedly, unchanged from Session 57 — see Build/test
matrix above.** `test_binner`, `test_zoneplate` and `test_pipeline_bytes`
fail identically in all ten sandbox configurations, exact same checks as
before this session's own fix. Every other test (25 of 28) passes in every
configuration. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

Same GCC 13.3.0 / Clang 18.1.3 cloud sandbox as Session 57. This session
used a genuinely fresh `git clone` (not Session 57's own leftover,
uncommitted working tree still present in this sandbox from before, which
was deliberately left alone and not reused, to build against the real,
already-tagged `wu-41-red` commit rather than a stale local copy that
might have drifted from it). The one-file fix was written to Steve's own
real repository via the device bridge, then re-staged and grepped
(`Sample Y = kBlack, Cb = kBlack, Cr = kBlack`) to confirm the write
landed, and `git diff --stat` confirmed exactly one file changed
(`src/core/resolve.hpp`) before this session's own fix was copied into the
separate cloud-sandbox clone for the matrix build — the two copies verified
identical by direct `diff` before building. **A stray, empty
`.git/index.lock` appeared again this session** (same as Sessions 55, 56
and 57's own accounts) — created by this session's own opening
`git status --short`/`git diff --stat` verification, not present before
it, and the device-bridge shell still cannot remove it
(`rm -f .git/index.lock` still returns "Operation not permitted"). The
close-out block below checks for it and removes it first regardless. No
`git commit` or `git push` attempted this session — nothing pushed.
`./tools/close.sh` was not run — C-024's standing PSU/duplex-check
exception is unchanged, and this fix does not reach a green `ctest` run
regardless (the three pre-existing failures are untouched by it).

## Append to DECISIONS.md

None this session. This is an implementation bug fix within ADR-085's
already-accepted scope (a compositing constant left in the wrong colour
domain), not a new architectural decision — I3's own already-accepted text
already says what RGB black should be on every channel; this session only
made `core/resolve.hpp` actually agree with it.

## Append to CORRECTIONS.md

**C-032 added** — WU-41's own C-028/C-029-style repo-wide check, before
closing out `wu-41-red`, covered every real call site that constructs or
reads `SourceRaster`, but missed that `core/resolve.hpp`'s `Background`/
`kDefaultBackground` is an independent source of `Y`/`Cb`/`Cr`-labelled
content reaching the same `warped` raster that unit's own
`core/pipeline.cpp` change reinterprets wholesale — a real bug that shipped
in the tagged, pushed `wu-41-red` commit, caught only because Steve
happened to run real DeckLink hardware against a test this repo's own
matrix cannot exercise. Full account, general lesson ("audit every writer
into a reinterpreted buffer, not only the writer(s) this unit itself
touched, even when another writer's file is on paper a different unit's
scope") in `CORRECTIONS.md` itself.

## Closed out this session

**WU-41 follow-up fix — `core/resolve.hpp`'s `kDefaultBackground` moved
from YCbCr-domain legal black (`Cb = Cr = kChromaZero`) to RGB-domain legal
black (`Cb = Cr = kBlack`), fixing a genuine cyan/blue background tint
Steve observed on real DeckLink hardware. Built, re-verified across all
ten cloud-sandbox configurations against a fresh clone of the already-
tagged `wu-41-red` commit: identical red state to Session 57's own matrix
(same three tests, same exact check counts), no new warning, no sanitizer
finding, fix verified numerically outside the live-hardware path. Not yet
committed.** One production file (`src/core/resolve.hpp`), `WORK-UNITS.md`
(WU-41 entry, addendum appended), `CORRECTIONS.md` (C-032 appended), this
`HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real build and test run — **still
expected to be red, exactly as `wu-41-red` was, not newly green**:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect the same failures `wu-41-red` already had: `test_decklink_device`'s
own duplex check (standing PSU exception, unrelated), plus `test_binner`
(2 checks), `test_zoneplate` (22 checks), `test_pipeline_bytes` (3 checks).
If you re-run `tests/test_decklink_live_sphere.cpp` against your own
DeckLink hardware after this build, the background should now read as
genuine black rather than cyan/blue — worth confirming visually, since
that live path is exactly what this session's own cloud sandbox cannot
test.

**Do not run `./tools/close.sh`** — same reasoning as `wu-41-red`: this
state does not pass `ctest` cleanly (by design, the standing Phase 9
exception), and `close.sh` refuses to tag past any failure.

Once you've confirmed the build and that `ctest`'s failures are exactly
the ones named above, close out with the **manual tag path**:

```
cd ~/src/scatter-dve
rm -f .git/index.lock
git add src/core/resolve.hpp WORK-UNITS.md CORRECTIONS.md HANDOFF.md
git commit -m "WU-41 follow-up: core/resolve.hpp kDefaultBackground moved to RGB-domain black (ADR-085); fixes cyan/blue background on live DeckLink output, see HANDOFF.md"
git tag -a wu-41-bgfix-red -m "core/resolve.hpp Background default fixed to RGB-domain black; test_binner/test_zoneplate/test_pipeline_bytes still red as expected (unrelated, pre-existing), see HANDOFF.md"
git push origin main
git push origin --tags
```

**Tag name is a suggestion, `wu-41-bgfix-red`** — not a new numbered work
unit (WU-42 is still next and still unstarted), so it doesn't fit the
plain `wu-NN-{red,green}` pattern cleanly; rename it to whatever you'd
prefer before running the command (`wu-41b-red` and `wu-41-red-2` are two
other reasonable choices) if this one doesn't read right to you. The
`rm -f .git/index.lock` is a precaution, not a sign anything is currently
wrong — same situation as Session 57's own close-out block: a stray, empty
`index.lock` appeared on the real repository partway through this session
(see "Environment check" above) and could not be removed from the
device-bridge shell. If it is already gone by the time you run this, the
`rm -f` is a silent no-op.

This exact list of 4 paths was checked against a real `git status --short`
run through the device bridge immediately before this block was written
(`CORRECTIONS.md` C-026's own general lesson) — it should read exactly 4
`M` lines: `src/core/resolve.hpp`, `WORK-UNITS.md`, `CORRECTIONS.md`,
`HANDOFF.md`, and nothing else. Still worth a quick `git status --short`
yourself before pasting this block, since time has passed since that
check.
