# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 53 (WU-34b continuation prompt — scoping session first, build
session second per the prompt's own instruction; both completed, WU-34b
built and green; a new WU-34c logged as the deferred follow-up).

**Tag:** `wu-34a-green` was the newest real tag as of this session's own
start (confirmed directly: `git tag --sort=creatordate`, `git log --oneline
-10` showing `HEAD` = `83d420a` = `origin/main`, `git status --short`
clean, commit message matching the prior session's own exact text — Steve
had already run his own real-terminal close-out for WU-34a before this
session started). **Nothing is tagged yet by this session** — that is
Steve's own next step below, after his own real-terminal build/test
confirms green.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This unit is core-only — a real cloud-sandbox build/test WAS possible, and was done

Like WU-27, WU-34a and WU-32 before it, WU-34b touches no DeckLink-linked
file. Every changed file was written, then actually compiled and tested in
this session's own Linux cloud sandbox (a fresh `git clone` of
`skunge2000/scatter-dve` at `HEAD` `83d420a`, confirmed matching the real
repository before any edit) — not merely reasoned about from reading the
source.

## This session in full

Opened with a continuation prompt whose own job was: confirm which of two
possible real repository states applied (Steve's own WU-34a close-out
already run, or not) before doing anything else — confirmed the former
directly, then scope WU-34b's own three open questions before writing any
code, per the prompt's own "scoping session first, build session second"
instruction.

**Read directly, not from paraphrase:** `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`INVARIANTS.md` (I3, I9, I10), `DECISIONS.md` (ADR-068 through ADR-071,
ADR-082, ADR-083 in full), `CORRECTIONS.md` (in full), `WORK-UNITS.md`
(WU-34b's own entry), `docs/architecture.md` §3/§6, `docs/sources/
WU-SM-01.md` §3.9.2, `core/coarse_shading.hpp`, `core/lighting.hpp`,
`core/types.hpp`, `core/binner.hpp`/`.cpp`, `core/pipeline.hpp`/`.cpp`,
`core/resolve.hpp`, `tests/test_binner.cpp`, `tests/test_coarse_shading.cpp`.

**Scoping outcome, logged as `DECISIONS.md` ADR-084 (full account there):**

1. **The RGB-vs-YCbCr application question — raised with Steve directly,
   settled by his own domain knowledge, not defaulted.** Mirage was
   RGB-native internally throughout; the 4:2:2 YCbCr and analogue/composite
   I/O were both converted to/from RGB at the boundary alone. So "multiply
   RGB by `I`" (S5 FIG. 1's own `STARLIGHT (RGB × I)` stage) is implemented
   as a real RGB round trip — convert this one sample's Y/Cb/Cr to RGB,
   multiply by `I`, convert back — not a YCbCr approximation of one. This
   also corrected this session's own initial cost estimate: the conversion
   sits entirely in the per-sample loop's own already-existing
   double-precision stretch (ahead of quantisation), so it is a small,
   fixed function, not a new subsystem, and does not touch I3/I4/I6.
2. **Coefficients: BT601, Steve's own explicit choice ("this is an SD
   machine"), with BT709 built and selectable for later HD work.** A
   general `(Kr, Kg, Kb)`-parametrised conversion, not BT601's numbers
   hard-coded — `core/binner.hpp`'s new `ColourStandard` enum.
3. **Threading: resolved directly against `core/pipeline.cpp`'s real
   current code — no new synchronisation needed.** Found in the process
   that `core/resolve.hpp`'s own `PipelineParams::threads` comment was
   stale (claimed PASS 1 is "always single-threaded," true of WU-16a but
   not of WU-16b's real row-band threading, shipped two units ago and never
   backfilled into this comment) — logged as `CORRECTIONS.md` C-031, fixed
   the same session (comment only, no behavioural change). The real answer:
   `CoarseShadingGrid` follows the same caller-owned, concurrently-read
   pattern `Lattice` itself already uses — nothing new needed in
   `core/binner.cpp`.
4. **Insertion point: confirmed exactly as anticipated** —
   `generateFragmentsRowRangeImpl()`'s shared loop, immediately after
   `sampleBilinear()` returns and before `Frag` construction.
5. **Split confirmed: WU-34b stays narrow.** Owning a real per-frame
   `LightingScene`/`CoarseShadingConfig` in `core/pipeline.cpp` is new
   WU-34c (`WORK-UNITS.md`), deferred — this unit's own file budget
   (`binner.hpp`/`.cpp` alone) had no room for it, exactly as WU-34b's own
   prior scoping note anticipated.

**Built exactly per the scope above:**

- `src/core/binner.hpp` (edited): forward-declares `CoarseShadingGrid`;
  adds `ColourStandard` (`BT601`, `BT709`); adds
  `const CoarseShadingGrid* shadingGrid = nullptr` and
  `ColourStandard shadingStandard = ColourStandard::BT601` as new optional
  trailing parameters on all five public entry points.
- `src/core/binner.cpp` (edited): `applyShading()` (the RGB round-trip) and
  its own `coeffsFor()` helper; the multiply call site ahead of `Frag`
  construction; the same two parameters threaded through
  `generateFragmentsRowRangeImpl()` and all five wrappers.
- `tests/test_binner.cpp` (edited, doesn't count against the cap): two new
  tests — the shading-multiply correctness check (independent hand-mirrored
  BT601 RGB round trip, not calling `applyShading()`) and the
  explicit-nullptr-matches-implicit-default regression guard.
- `src/core/resolve.hpp` (edited, one comment only — C-031, no behavioural
  change).
- `WORK-UNITS.md`: WU-34b's entry replaced (`Files:`/`Accept:`/
  `Status: green`); new WU-34c entry added (scoped, `todo`).
- `DECISIONS.md`: ADR-084 appended (full account of every decision above).
- `CORRECTIONS.md`: C-031 appended.
- No `CMakeLists.txt` change — both changed source files were already
  registered.

**Grepped the whole repository before closing out** (`CORRECTIONS.md`
C-028/C-029's own sharpened lesson, and doubly warranted here since this
unit edits `core/binner.cpp`'s existing, shared, heavily-reused per-sample
loop rather than only adding new files): every real call site of all five
`generateFragments*()` entry points (`core/pipeline.cpp` and roughly a
dozen test files) confirmed to use ordinary call syntax, none via a
function pointer that the two new defaulted trailing parameters could
break; `git diff --stat` confirms exactly four files changed
(`src/core/binner.hpp`, `src/core/binner.cpp`, `src/core/resolve.hpp`,
`tests/test_binner.cpp`) and no other file references the new symbols
(`ColourStandard`, `applyShading`, `Kcoeffs`, `coeffsFor`) or collides with
the test file's own new `RGB` struct name.

**Verified the new test actually catches a regression, not just passes
vacuously:** temporarily disabled the multiply in `core/binner.cpp` (while
keeping `shadingGrid`/`shadingStandard` referenced so the build stayed
clean under `-Werror`), rebuilt, and confirmed
`test_shading_multiplies_rgb_intensity_ahead_of_frag_construction()` failed
with exactly its three expected colour-channel checks; restored the real
implementation, rebuilt, confirmed all 28 tests pass again before treating
this as done.

## Where we are

**WU-34b is written, built and tested green in the cloud sandbox — full
portable matrix, all ten configurations, no regressions, no sanitizer
findings:**

| Compiler | Build type | `SCATTER_TILE_LOG2` | Sanitizers | Result |
|---|---|---|---|---|
| GCC 13.3.0 | Release | 5 | — | 28/28 tests pass |
| GCC 13.3.0 | Debug | 5 | — | 28/28 tests pass |
| GCC 13.3.0 | Release | 4 | — | 28/28 tests pass |
| GCC 13.3.0 | Debug | 4 | — | 28/28 tests pass |
| Clang 18.1.3 | Release | 5 | — | 28/28 tests pass |
| Clang 18.1.3 | Debug | 5 | — | 28/28 tests pass |
| Clang 18.1.3 | Release | 4 | — | 28/28 tests pass |
| Clang 18.1.3 | Debug | 4 | — | 28/28 tests pass |
| GCC 13.3.0 | Debug | 5 | ASan+UBSan | 28/28 tests pass |
| GCC 13.3.0 | Debug | 4 | ASan+UBSan | 28/28 tests pass |

`test_binner` itself: 39139 checks, all passing, in every configuration.
This is real, verified-in-sandbox green — Steve's own real-terminal run is
still the final word (`SESSION-PROTOCOL.md`'s own "sandbox edits are not
delivered until pushed" discipline) — see "Steve's own next steps" below.
`test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check is expected to fail on Steve's own real-terminal run regardless (the
standing PSU/two-device exception, `CORRECTIONS.md` C-024) — not this
unit's own problem, and this unit doesn't touch anything DeckLink-related
at all.

All four changed files are written to the real repository via the device
bridge — see the confirmation note below — but **not yet committed, tagged
or pushed**. That is Steve's own next step, after his own real-terminal
build/test confirms green.

## Next work unit

**WU-34c** (own a per-frame `LightingScene`/`CoarseShadingConfig` in
`core/pipeline.cpp`, wire a real `runFrame()` caller) is the natural next
pick — its own dependency (WU-34b) is now green, and `WORK-UNITS.md`'s own
WU-34c entry names the shape this session's own scoping already worked
out (new `PipelineParams` fields, following the same
`pool`/`weightOut`/`kBufferMode` convention). Everything else named in
earlier sessions' own "Next work unit" sections (WU-28d, WU-33, WU-35,
WU-37) is unchanged and still pickable — WU-37 (specular model LUTs)
remains blocked exactly as before.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. WU-34b's own
three open questions are now closed — see ADR-084. WU-34c's own scope
(above) is new, not resolved here.

## Blocked / red

Not blocked. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang --version` directly before
building) — the full portable matrix ran for real, not by inference. A
fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` was
used for the sandbox build (not a copy staged through the device bridge),
confirmed matching `HEAD 83d420a` before any edit. The device-bridge
sandbox used for reading/writing files on the real Mac repository had
ordinary read/write access this session; no `.git/index.lock` stray files
encountered. No `git commit` or `git push` attempted this session —
nothing pushed. C-024's standing condition (PSU out, `tools/close.sh`
cannot succeed on Steve's own real terminal for any unit) is unchanged and
unaffected by this session.

## Append to DECISIONS.md

**ADR-084**, appended this session — see above and the real `DECISIONS.md`
for the full text. Covers the RGB-vs-YCbCr application decision (Steve's
own direct call), the BT601/BT709 coefficient decision, the threading
finding, the insertion-point confirmation, and the WU-34b/WU-34c split.

## Append to CORRECTIONS.md

**C-031**, appended this session — see above and the real `CORRECTIONS.md`
for the full text. `core/resolve.hpp`'s own `PipelineParams::threads`
comment claimed PASS 1 is "always single-threaded," stale since WU-16b
gave it real row-band threading two units ago; fixed (comment only, no
behavioural change).

## Closed out this session

**WU-34b, built, tested green in the cloud sandbox (full portable matrix),
not yet committed.** `src/core/binner.hpp` (edited), `src/core/binner.cpp`
(edited), `tests/test_binner.cpp` (edited), `src/core/resolve.hpp` (edited,
comment only), `WORK-UNITS.md` (WU-34b entry replaced, WU-34c added),
`DECISIONS.md` (ADR-084 appended), `CORRECTIONS.md` (C-031 appended). This
`HANDOFF.md`.

## Steve's own next steps

At your own real terminal, confirm a real, green (modulo the standing
duplex exception) build and test run:

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expect `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check to fail regardless (the
standing PSU/two-device exception, `DECISIONS.md` ADR-034/035/037,
`CORRECTIONS.md` C-024) — not this unit's own problem, and this unit
touches nothing DeckLink-related at all. Every other test, including
`test_binner`'s own two new checks, should pass — if anything else fails,
that is real feedback for the next session, not a reason to force a tag
past it. **Do not run `./tools/close.sh`** — see `CORRECTIONS.md` C-024: it
treats any `ctest` failure as blocking and refuses to tag, and the
duplex-check exception means it can never succeed here regardless of which
unit is being closed.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
git add CORRECTIONS.md DECISIONS.md HANDOFF.md WORK-UNITS.md src/core/binner.cpp src/core/binner.hpp src/core/resolve.hpp tests/test_binner.cpp
git commit -m "WU-34b: coarse-grid shading wired into core/binner.hpp per-sample loop, real RGB round-trip (ADR-084); C-031"
git tag -a wu-34b-green -m "WU-34b: coarse-grid shading wired into core/binner.hpp per-sample loop, real RGB round-trip (ADR-084); C-031"
git push origin main
git push origin --tags
```

This exact list of eight paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it should read
exactly: `M CORRECTIONS.md`, `M DECISIONS.md`, `M HANDOFF.md`,
`M WORK-UNITS.md`, `M src/core/binner.cpp`, `M src/core/binner.hpp`,
`M src/core/resolve.hpp`, `M tests/test_binner.cpp`, and nothing else.
Still worth a quick `git status --short` yourself before pasting this
block, since time has passed since that check.
