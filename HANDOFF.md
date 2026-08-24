# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 51 (WU-27 continuation prompt — scoping session first, build
session second per the prompt's own instruction; both completed).

**Tag:** `wu-23b2b-green` was still the newest real tag as of this
session's own start (confirmed directly: `git tag --sort=creatordate`,
`git log --oneline -10` showing `HEAD` = `4eaf7de` = `origin/main`,
`git status --short` clean — no drift from the continuation prompt's own
snapshot). **Nothing is tagged yet by this session** — that is Steve's own
next step below, after his own real-terminal build/test confirms green.

## Before doing anything else in the next session

Run `git tag --sort=creatordate`, `git log --oneline -10`, `git rev-parse
HEAD origin/main` and `git status --short` directly against
`~/src/scatter-dve` — do not trust this file's own account without
checking it against the real repository first.

## This unit is core-only — a real cloud-sandbox build/test WAS possible, and was done

Unlike the two sessions before this one (WU-23b2a/WU-23b2b, both
DeckLink-linked), WU-27 touches no DeckLink-linked file at all. Every new
and changed file was written, then actually compiled and tested in this
session's own Linux cloud sandbox — not merely reasoned about from reading
the source, the situation the previous two sessions were stuck in.

## This session in full

Opened with a continuation prompt whose own job was: scope WU-27 first
(it had never carried real `Files:`/`Accept:`, only WU-36's first-cut
scoping note), build it second if time allowed. Confirmed repository state
directly before reading anything else — matched the continuation prompt's
own snapshot exactly, nothing to reconcile.

**Read directly, not from paraphrase:** `SESSION-PROTOCOL.md`,
`HANDOFF.md`, `INVARIANTS.md` (I10), `DECISIONS.md` (ADR-068 through
ADR-071, ADR-081 in full), `CORRECTIONS.md` (C-024, C-027, C-028, C-029),
`WORK-UNITS.md` (WU-27, WU-34, WU-37 entries), `docs/architecture.md` §3,
`docs/wu-audit-2026-08.md` (WU-27/WU-34 rows, Deliverable 4 and 5),
`docs/sources/WU-SM-01.md` §4.5, §4.6 and §8 in full (the actual
hand-worked fixture text, not the one-line gloss in
`tests/fixtures-historical.md`), `core/jacobian.hpp`, `core/lattice.hpp`,
`core/binner.hpp`/`.cpp`, `core/types.hpp`, `core/resolve.hpp`,
`CMakeLists.txt`, `tests/harness.hpp`, `tests/test_jacobian.cpp`.

**Scoping outcome, logged as `DECISIONS.md` ADR-082 (full account there):**

1. **WU-27 split from WU-34**, along "pure Phong evaluator" (this unit) vs.
   "wiring it into `core/binner.hpp` via a coarse-grid facet" (WU-34) —
   keeps WU-27 at 2 new source files, well under the 3-file cap, and
   respects ADR-070's own requirement that shading is evaluated per
   coarse-grid facet, not per source sample (a genuinely different
   mechanism from a call inside `core/binner.cpp`'s existing per-sample
   loop). `core/binner.hpp`/`.cpp` are **untouched** this session.
2. **Facet normal — raised with Steve directly, per ADR-070's own
   instruction: historical finite-difference reconstruction**, not WU-26's
   exact analytic `surfaceNormal()`. Matches this project's own demonstrated
   preference for faithful reproduction elsewhere. Binds WU-34's own build,
   not this session's code — logged here since it was decided during this
   unit's own scoping.
3. **`model(L, zone)` LUT — confirmed with Steve: build the pluggable
   interface now, with placeholder curves for all eight named models.**
   Matches the blocked-work register's own recommendation.
4. **The primary Nonweiler patent, EP 0248626/US 4,899,295, was obtained
   and read in full this session** (Steve supplied the PDF). It
   independently corroborates ADR-069's own closed-form formula from a
   second primary source, and adds a concrete "view and light coincident
   ⇒ B = 2A" relation not previously recorded — but it does **not** name or
   tabulate the eight specular models (Deliverable 5's blocking gap is
   unchanged; WU-37 remains blocked on a different, still-missing
   document), and it does **not** correct or reopen ADR-069's own `cos B`
   LUT claim (that claim is about a different document, S5/Cawley, already
   held — this patent describes a simpler, earlier, single-model system).
   Full reasoning in ADR-082, point 4.

**Built exactly per the scope above:**

- `src/core/lighting.hpp` (new): `LightType` (Point/Beam/Parallel),
  `SpecularModel` (the eight named models, all placeholder curves),
  `defaultSpecularCurve()`, `Zone`, `Light`, `LightingScene`, `shade()`.
- `src/core/lighting.cpp` (new): implementation — reflect/dot/normalize
  helpers, the eight placeholder curves, `shade()`'s own per-light
  summation loop.
- `tests/test_lighting.cpp` (new): fixtures 9, 12, 13, 15, 16 (adapted —
  see the file's own header comment for exactly what's adapted and why),
  17, 18, 26, plus `defaultSpecularCurve()` bounds/monotonicity checks.
  239 checks, all passing.
- `src/core/jacobian.hpp`: one comment line corrected (stale "WU-27's own
  two-sided Blinn-Phong shading" → "WU-27's own Phong shading... normalises
  internally" — `docs/wu-audit-2026-08.md` finding 4's own "natural moment
  to fix" note). No behavioural change.
- `CMakeLists.txt`: `src/core/lighting.cpp` added to `scatter-core`'s
  source list; `scatter_test(test_lighting)` added next to
  `test_jacobian`.
- `WORK-UNITS.md`: WU-27 rewritten with real `Files:`/`Accept:`,
  `Status: green`; WU-34 updated with the split confirmation and the
  facet-normal decision.
- `DECISIONS.md`: ADR-082 appended (full account of every decision above).

**Grepped the whole repository before closing out** (the lesson C-028/C-029
sharpened, applied here even though this unit is almost entirely additive):
`grep -rn "Blinn-Phong"` and `grep -rln "lighting"` across `src/`, `tests/`
and every `.md` file. Confirmed `src/core/jacobian.hpp:136` was the only
stale *code* comment (matching the wu-audit's own finding exactly, now
fixed); every other "Blinn-Phong" hit is inside `DECISIONS.md`/
`CORRECTIONS.md`/`docs/` correctly narrating the historical correction
itself, not a live stale claim. No other file references "lighting" —
confirms no naming collision with any prior partial work.

**No `CORRECTIONS.md` entry this session** — nothing the first-cut scoping
note or any earlier ADR assumed turned out wrong once the real code was
read; ADR-082 records one place a correction was *considered* (whether the
new patent contradicts ADR-069's `cos B` claim) and explains directly in
the ADR why it does not apply, rather than filing a correction for
something that wasn't actually wrong.

## Where we are

**WU-27 is written, built and tested green in the cloud sandbox — full
portable matrix, all ten configurations, no regressions, no sanitizer
findings:**

| Compiler | Build type | `SCATTER_TILE_LOG2` | Sanitizers | Result |
|---|---|---|---|---|
| GCC 13.3.0 | Release | 5 | — | 27/27 tests pass |
| GCC 13.3.0 | Debug | 5 | — | 27/27 tests pass |
| GCC 13.3.0 | Release | 4 | — | 27/27 tests pass |
| GCC 13.3.0 | Debug | 4 | — | 27/27 tests pass |
| Clang 18.1.3 | Release | 5 | — | 27/27 tests pass |
| Clang 18.1.3 | Debug | 5 | — | 27/27 tests pass |
| Clang 18.1.3 | Release | 4 | — | 27/27 tests pass |
| Clang 18.1.3 | Debug | 4 | — | 27/27 tests pass |
| GCC 13.3.0 | Debug | 5 | ASan+UBSan | 27/27 tests pass |
| GCC 13.3.0 | Debug | 4 | ASan+UBSan | 27/27 tests pass |

`test_lighting` itself: 239 checks, all passing, in every configuration.
This is real, verified-in-sandbox green — **not** the "written but never
compiled" situation the WU-23b2a/WU-23b2b sessions were in (those touched
DeckLink-linked files with no compiler in reach). Steve's own real-terminal
run is still the final word (`SESSION-PROTOCOL.md`'s own "sandbox edits are
not delivered until pushed" discipline) — see "Steve's own next steps"
below. `test_decklink_device`'s own `test_at_least_one_device_is_full_duplex`
check is expected to fail on Steve's own real-terminal run regardless (the
standing PSU/two-device exception, `CORRECTIONS.md` C-024) — not this
unit's own problem, and this unit doesn't touch anything DeckLink-related
at all.

All eight changed/created files are written to the real repository via the
device bridge — see the confirmation note below — but **not yet committed,
tagged or pushed**. That is Steve's own next step, after his own
real-terminal build/test confirms green.

## Next work unit

**WU-34** (coarse-grid shading: filtering ladder and grid shift) is now the
natural next pick — it wires `core/lighting.hpp`'s `shade()` into
`core/binner.hpp`'s per-sample loop via a coarse-grid facet, using the
historical finite-difference facet normal Steve chose this session
(`DECISIONS.md` ADR-082, extending ADR-070). Its own first open question,
not decided this session: coarse-grid cell size (no held source states it).
Everything else named in earlier sessions' own "Next work unit" sections
(WU-28d, WU-33, WU-35, WU-37) is unchanged and still pickable — WU-37
(specular model LUTs) remains blocked exactly as before; this session's
patent read did not unblock it (see ADR-082 point 4).

## Open questions

Unchanged from earlier sessions' own list (`kCaptureRingCapacity`, Q3, Q4,
Task A1, Task D6) — this session did not touch any of them. WU-27's own two
open design points (facet normal, LUT-interface plan) are now closed — see
ADR-082. WU-34's own first open question (coarse-grid cell size) is new,
not resolved here.

## Blocked / red

Not blocked. `ctest` was not run on Steve's own real terminal yet this
session — see "Steve's own next steps" below.

## Environment check

This session had both GCC 13.3.0 and Clang 18.1.3 in its own cloud
sandbox (confirmed via `gcc --version`/`clang --version` directly before
building) — the full portable matrix ran for real, not by inference. The
device-bridge sandbox used for reading/writing files had ordinary
read/write access this session; no `.git/index.lock` stray files
encountered issuing `git status`/`git tag`/`git log` reads through it. No
`git commit` or `git push` attempted this session — nothing pushed. C-024's
standing condition (PSU out, `tools/close.sh` cannot succeed on Steve's own
real terminal for any unit because of the PSU/two-device-architecture
mismatch) is unchanged and unaffected by this session.

## Append to DECISIONS.md

**ADR-082**, appended this session — see above and the real `DECISIONS.md`
for the full text. Covers the WU-27/WU-34 split, the facet-normal decision,
the LUT-placeholder confirmation, and the full account of what the
Nonweiler patent (EP 0248626/US 4,899,295) does and does not settle.

## Append to CORRECTIONS.md

Nothing this session — see "This session in full" above for why (one
place a correction was considered and explicitly ruled out, reasoned
through in ADR-082 rather than filed here).

## Closed out this session

**WU-27, built, tested green in the cloud sandbox (full portable matrix),
not yet committed.** `src/core/lighting.hpp` (new), `src/core/lighting.cpp`
(new), `tests/test_lighting.cpp` (new), `src/core/jacobian.hpp` (one
comment line), `CMakeLists.txt` (two additions), `WORK-UNITS.md` (WU-27
rewritten, WU-34 updated), `DECISIONS.md` (ADR-082 appended). This
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
new `test_lighting`, should pass — if anything else fails, that is real
feedback for the next session, not a reason to force a tag past it. **Do
not run `./tools/close.sh`** — see this project's own standing note in
`CORRECTIONS.md` C-024: it treats any `ctest` failure as blocking and
refuses to tag, and the duplex-check exception means it can never succeed
here regardless of which unit is being closed.

**Only once you've confirmed a real, green (modulo the standing duplex
exception) build and test run**, close out with:

```
cd ~/src/scatter-dve
git add CMakeLists.txt DECISIONS.md HANDOFF.md WORK-UNITS.md src/core/jacobian.hpp src/core/lighting.cpp src/core/lighting.hpp tests/test_lighting.cpp
git commit -m "WU-27: Phong lighting evaluator, no coarse-grid wiring (ADR-082)"
git tag -a wu-27-green -m "WU-27: Phong lighting evaluator, no coarse-grid wiring (ADR-082)"
git push origin main
git push origin --tags
```

This exact list of eight paths was checked against a real `git status
--short` run through the device bridge immediately before this block was
written (`CORRECTIONS.md` C-026's own general lesson) — it read exactly:
`M CMakeLists.txt`, `M DECISIONS.md`, `M HANDOFF.md`, `M WORK-UNITS.md`,
`M src/core/jacobian.hpp`, `?? src/core/lighting.cpp`,
`?? src/core/lighting.hpp`, `?? tests/test_lighting.cpp`, and nothing
else — `CORRECTIONS.md` is clean, confirming no entry was needed this
session. Still worth a quick `git status --short` yourself before pasting
this block, since time has passed since that check.
