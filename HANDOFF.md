# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 52 (WU-34 continuation prompt — scoping session first, build
session second per the prompt's own instruction; both completed, split into
WU-34a (built, green) and WU-34b (scoped, deferred)).

**Tag:** `wu-27-green` was still the newest real tag as of this session's
own start (confirmed directly: `git tag --sort=creatordate`, `git log
--oneline -10` showing `HEAD` = `e2fbea7` = `origin/main`, `git status
--short` clean — no drift from the continuation prompt's own snapshot).
**Nothing is tagged yet by this session** — that is Steve's own next step
below, after his own real-terminal build/test confirms green.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This unit is core-only — a real cloud-sandbox build/test WAS possible, and was done

Like WU-27 before it, WU-34a touches no DeckLink-linked file. Every new and
changed file was written, then actually compiled and tested in this
session's own Linux cloud sandbox (a fresh `git clone` of
`skunge2000/scatter-dve` at `HEAD` `e2fbea7`, confirmed matching the real
repository before any edit) — not merely reasoned about from reading the
source.

## This session in full

Opened with a continuation prompt whose own job was: scope WU-34 first (its
own first open question, coarse-grid cell size, was left unresolved by
every held source across WU-32 and WU-27's own sessions), build it second
if the scope held together within `SESSION-PROTOCOL.md`'s 3-file cap.
Confirmed repository state directly before reading anything else — matched
the continuation prompt's own snapshot exactly (newest tag `wu-27-green`,
`HEAD` `e2fbea7` == `origin/main`, clean tree), including an initial
`git tag --sort=creatordate` surprise (a `wu-36-green` tag sorting *before*
`wu-27-green` by creation date, since WU-36 was a documentation-only
re-plan sweep created the same day as WU-32/WU-28c, before WU-27's own
build session) — verified `wu-36-green` is a real ancestor of `HEAD`, not
drift, before proceeding.

**Read directly, not from paraphrase:** `SESSION-PROTOCOL.md`, `HANDOFF.md`,
`INVARIANTS.md` (I9, I10), `DECISIONS.md` (ADR-068 through ADR-071,
ADR-082 in full), `CORRECTIONS.md` (C-024 through C-029),
`WORK-UNITS.md` (WU-27 and WU-34 entries), `docs/architecture.md` §3/§4.1/
§4.2, `docs/sources/WU-SM-01.md` §3.9.1, §3.9.2, §4.6.3, §6.3 and §8 in
full (not the one-line gloss in `tests/fixtures-historical.md`),
`core/lighting.hpp`/`.cpp`, `core/lattice.hpp`/`.cpp`, `core/jacobian.hpp`,
`core/binner.hpp`/`.cpp`, `core/types.hpp`, `tests/harness.hpp`,
`tests/test_lighting.cpp`, `tests/test_jacobian.cpp`, `CMakeLists.txt`.

**Scoping outcome, logged as `DECISIONS.md` ADR-083 (full account there):**

1. **Coarse-grid cell size — the existing 129×129 `core::Lattice` geometry
   lattice, not a second grid.** `docs/sources/WU-SM-01.md` §3.9.1 ties the
   shading coarse grid to the same "coarse grid" S1's own shape diagnostics
   name (§6.3); `docs/architecture.md` §4.1 already identifies that concept
   with the 129×129 lattice. No source states a shading-specific number, and
   nothing suggests a second mesh. Judged (per the continuation prompt's own
   tiering) an ordinary "no source states a number" engineering call, not a
   "which historical behaviour to reproduce" tradeoff — did not escalate to
   Steve, consistent with the prompt's own suggested reading.
2. **Facet-normal finite-difference stencil — P plus its immediate forward
   +u/+v neighbours (backward at the lattice's own last row/column),
   attributed to P.** No source names the real stencil; grid shift's own
   horizontal-only documented behaviour is the only available (consistent,
   not confirmatory) constraint. Same escalation tier as point 1.
3. **Filtering ladder and grid shift reconstructed as an explicit
   raw → grid-shift → filter pipeline**, with grid shift applied to the raw
   field before the filtering ladder (not after) — the order that keeps
   grid shift's own documented purpose intact. Flat1/Flat2x2/Flat3x3 are
   block-anchor lookups (no bilinear across a block seam); Smooth1/Smooth2
   are a box blur of radius 1/2 (explicit placeholder radius, same tier as
   `defaultSpecularCurve()`).
4. **Split: WU-34a is `CoarseShadingGrid` alone; WU-34b is wiring it into
   `core/binner.hpp`'s per-sample loop.** Verified directly against the
   real current source before deciding (`core/binner.hpp`/`.cpp` are large,
   already-dense files whose per-sample loop is shared by five public entry
   points) — matches the continuation prompt's own anticipated split.
5. **MULTIPLY-vs-ADD scope: MULTIPLY only this session, ADD left as
   documented future work** — matches the well-evidenced base circuit
   (S5 FIG. 1's own `STARLIGHT (RGB × I)` stage). Affects WU-34b's own
   scope, logged here since it was decided during this unit's own scoping.

**Built exactly per the scope above:**

- `src/core/coarse_shading.hpp` (new): `ShadingFilter`, `CoarseShadingConfig`,
  `CoarseShadingGrid` (`build()`, `sample()`).
- `src/core/coarse_shading.cpp` (new): implementation — finite-difference
  facet normal, grid-shift column offset, the filtering-ladder
  post-process, bilinear/block-anchor `sample()`.
- `tests/test_coarse_shading.cpp` (new): fixtures 10, 11, 19, each checked
  against an independently hand-mirrored reimplementation of the production
  formula (not by calling coarse_shading.cpp's own internal helpers) —
  305 checks, all passing.
- `CMakeLists.txt`: `src/core/coarse_shading.cpp` added to `scatter-core`'s
  source list; `scatter_test(test_coarse_shading)` added next to
  `test_lighting`.
- `WORK-UNITS.md`: the single WU-34 entry replaced with WU-34a
  (`Files:`/`Accept:`/`Status: green`) and WU-34b (scoped, `todo`).
- `DECISIONS.md`: ADR-083 appended (full account of every decision above).
- `tests/fixtures-historical.md`: rows for fixtures 9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19 and 26 updated to "Runnable, passing" — 10/11/19 are
  this session's own WU-34a fixtures; 9/12–18/26 are WU-27's, fixed here as
  a correction to that prior session's own incomplete close-out (see
  `CORRECTIONS.md` C-030 below).
- `CORRECTIONS.md`: C-030 appended.

**Grepped the whole repository before closing out** (`CORRECTIONS.md`
C-028/C-029's own sharpened lesson): `grep -rln "coarse_shading\|
CoarseShadingGrid\|ShadingFilter\|CoarseShadingConfig"` across `src/`,
`tests/` and `CMakeLists.txt` — confirms the new names appear only in the
three new files plus `CMakeLists.txt`'s own two additions, no naming
collision with any prior partial work; `git diff --stat` confirms
`core/binner.hpp`/`.cpp`, `core/lighting.hpp`/`.cpp` and every other
existing file are byte-for-byte untouched except `CMakeLists.txt`'s own
two-line addition — this unit is purely additive.

**`CORRECTIONS.md` entry this session — C-030**, not about this session's
own new code but about a gap discovered in a prior session's close-out
(`tests/fixtures-historical.md` never updated when WU-27 went green) —
found while updating the same table's own WU-34 rows and noticing the
inconsistency sitting next to them. Full account in `CORRECTIONS.md`.

## Where we are

**WU-34a is written, built and tested green in the cloud sandbox — full
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

`test_coarse_shading` itself: 305 checks, all passing, in every
configuration. This is real, verified-in-sandbox green — Steve's own
real-terminal run is still the final word (`SESSION-PROTOCOL.md`'s own
"sandbox edits are not delivered until pushed" discipline) — see "Steve's
own next steps" below. `test_decklink_device`'s own
`test_at_least_one_device_is_full_duplex` check is expected to fail on
Steve's own real-terminal run regardless (the standing PSU/two-device
exception, `CORRECTIONS.md` C-024) — not this unit's own problem, and this
unit doesn't touch anything DeckLink-related at all.

All eight changed/created files are written to the real repository via the
device bridge — see the confirmation note below — but **not yet committed,
tagged or pushed**. That is Steve's own next step, after his own
real-terminal build/test confirms green.

## Next work unit

**WU-34b** (wire `CoarseShadingGrid` into `core/binner.hpp`'s per-sample
loop) is the natural next pick — its own dependencies (WU-34a, WU-27) are
both green, and `WORK-UNITS.md`'s own WU-34b entry names its three real
open questions (per-frame `LightingScene`/`CoarseShadingConfig` ownership;
whether `CoarseShadingGrid::build()` needs its own threading treatment;
`core/binner.hpp`/`.cpp`'s real current size/shape once re-read directly).
Everything else named in earlier sessions' own "Next work unit" sections
(WU-28d, WU-33, WU-35, WU-37) is unchanged and still pickable — WU-37
(specular model LUTs) remains blocked exactly as before.

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. WU-34's own
first open question (coarse-grid cell size) is now closed — see ADR-083.
WU-34b's own three open questions (above) are new, not resolved here.

## Blocked / red

Not blocked. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang --version` directly before
building) — the full portable matrix ran for real, not by inference. A
fresh `git clone` of `https://github.com/skunge2000/scatter-dve.git` was
used for the sandbox build (not a copy staged through the device bridge),
confirmed matching `HEAD e2fbea7` before any edit. The device-bridge
sandbox used for reading/writing files on the real Mac repository had
ordinary read/write access this session; no `.git/index.lock` stray files
encountered. No `git commit` or `git push` attempted this session — nothing
pushed. C-024's standing condition (PSU out, `tools/close.sh` cannot
succeed on Steve's own real terminal for any unit) is unchanged and
unaffected by this session.

## Append to DECISIONS.md

**ADR-083**, appended this session — see above and the real `DECISIONS.md`
for the full text. Covers the coarse-grid cell-size decision, the
facet-normal stencil, the filtering-ladder/grid-shift reconstruction, the
WU-34a/WU-34b split, and the MULTIPLY-only scope decision.

## Append to CORRECTIONS.md

**C-030**, appended this session — see above and the real `CORRECTIONS.md`
for the full text. Not about this session's own new code: `tests/
fixtures-historical.md` was never updated when WU-27 went green in Session
51, discovered this session while updating the same table's own WU-34 rows.

## Closed out this session

**WU-34a, built, tested green in the cloud sandbox (full portable matrix),
not yet committed.** `src/core/coarse_shading.hpp` (new), `src/core/
coarse_shading.cpp` (new), `tests/test_coarse_shading.cpp` (new),
`CMakeLists.txt` (two additions), `WORK-UNITS.md` (WU-34 split into WU-34a/
WU-34b), `DECISIONS.md` (ADR-083 appended), `tests/fixtures-historical.md`
(fixtures 9–19, 26 rows updated), `CORRECTIONS.md` (C-030 appended). This
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
touches nothing DeckLink-related at all. Every other test, including the
new `test_coarse_shading`, should pass — if anything else fails, that is
real feedback for the next session, not a reason to force a tag past it.
**Do not run `./tools/close.sh`** — see `CORRECTIONS.md` C-024: it treats
any `ctest` failure as blocking and refuses to tag, and the duplex-check
exception means it can never succeed here regardless of which unit is
being closed.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
git add CMakeLists.txt CORRECTIONS.md DECISIONS.md HANDOFF.md WORK-UNITS.md src/core/coarse_shading.cpp src/core/coarse_shading.hpp tests/fixtures-historical.md tests/test_coarse_shading.cpp
git commit -m "WU-34a: coarse-grid shading field, filtering ladder and grid shift (ADR-083); C-030"
git tag -a wu-34a-green -m "WU-34a: coarse-grid shading field, filtering ladder and grid shift (ADR-083); C-030"
git push origin main
git push origin --tags
```

This exact list of nine paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it read exactly:
`M CMakeLists.txt`, `M CORRECTIONS.md`, `M DECISIONS.md`, `M HANDOFF.md`,
`M WORK-UNITS.md`, `M tests/fixtures-historical.md`,
`?? src/core/coarse_shading.cpp`, `?? src/core/coarse_shading.hpp`,
`?? tests/test_coarse_shading.cpp`, and nothing else. Still worth a quick
`git status --short` yourself before pasting this block, since time has
passed since that check.
