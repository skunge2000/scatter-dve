# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 42 (WU-36 — work-unit re-plan sweep against WU-32's own
findings; documentation-only, no production code touched, no green tag
reopened).

**Tag:** none yet — this session's own close-out (`wu-36-green`) is Steve's
own next action; see "Steve's own next steps" below.

## Before doing anything else in the next session

Run `git tag`, `git log --oneline -10`, `git status --short` and `git status
-sb` directly against `~/src/scatter-dve` via the device bridge, the same as
every session before this one — do not trust this file's own account of
tag/commit state without checking it against the real repository first.

## This session in full

Opened by verifying real repository state directly via the device bridge:
`git tag` listed `wu-28c-green` as the newest tag; `git log --oneline -5`
showed `HEAD` at `4c548cd` ("WU-28c: self-fold facing tag from surface
normals (ADR-065)"); `git status -sb` read `## main...origin/main` with no
ahead/behind marker; `git status --short` showed a clean tree. Matches
Session 41's own account exactly.

This was the second of two prompts Steve ran back to back: the first landed
WU-32 (ten historical findings, F1–F10, into `DECISIONS.md`/`INVARIANTS.md`/
`CORRECTIONS.md`/`WORK-UNITS.md`'s Phase 7); this session's own job was to
go back over the *entire* 53-unit sequence — not just Phase 7 — and give
every unit a verdict against those findings, systematically rather than
discovered one unit at a time.

Read `WORK-UNITS.md` in full (all 2157 lines, all 53 units, every phase),
`DECISIONS.md` ADR-066–074 in full, `INVARIANTS.md` I8–I11, `CORRECTIONS.md`
in full (all 23 corrections, for context on what's already been caught),
`tests/fixtures-historical.md`, `docs/architecture.md` in full, and
`SESSION-PROTOCOL.md`. For the load-bearing questions the prompt flagged as
requiring a real source read rather than reasoning from the ADRs alone,
grepped the actual tree:

- **F5 (address-map direction) leakage:** grepped all of `src/`, `tests/`,
  `DECISIONS.md`, `WORK-UNITS.md`, `INVARIANTS.md` for
  backward/inverse/gather language describing the address map's own
  direction. Found none current — `docs/architecture.md` and
  `INVARIANTS.md` I1 already state "forward scatter, never inverse gather"
  correctly, and the historical inverted-direction language only survives,
  correctly, inside `docs/sources/WU-SM-01.md`'s own verbatim research
  record (as the thing being corrected, not a live leak). Verdict:
  unaffected, confirmed rather than assumed.
- **F6 (single-source ingest) architecture:** grepped `core/binner.hpp`/
  `.cpp`, `core/resolve.hpp`, `core/pipeline.cpp`: every entry point
  (`runFrame`/`runFrameBytes`/`runFrameFile`, `generateFragments*`) takes
  exactly one `SourceRaster`. Confirms the prompt's own suspicion: WU-33
  (front/back source pair) has to modify these already-green files, plus
  the live-capture stack (`io/decklink_capture_consumer.*`, `CaptureConsumer`
  holding one `Lattice`), not just add new files beside them. Recorded as a
  scope amendment on WU-33's own entry, not a status change on anything
  green.
- **F4 (shading pipeline order):** `docs/architecture.md` §3's own signal-
  path diagram already places `[shading]` as the last step of PASS 1
  (fragment generation), ahead of PASS 2's splat — already correct per
  ADR-068/I10, not a structural problem. Grepped `src/` for any
  resolve-time shading hook: none exists (nothing to build shading
  post-splat has been written yet). Verdict: unaffected, already correct.
- **F7 (culling) invariant:** grepped all of `src/` for
  cull/early-out/back-face-reject/discard-on-facing logic: zero hits that
  are generation-time rejections (the only "discard" hits are WU-28b's own
  resolve-time `Opaque` mode discarding losing k-buffer slots, which is
  arbitration, not culling, and is exactly what I8 permits). WU-11's own
  accept text already anticipated this correctly before F7 existed.
  Verdict: unaffected, confirmed by source read.
- **F10 (∂z ordering):** WU-26 already ships `dz/du`/`dz/dv` and
  `surfaceNormal()`, already consumed by WU-28c, and WU-26's own WU-32-era
  note already names all three downstream consumers (WU-27, WU-28c/WU-33,
  WU-35) as gated on it. Nothing left to add beyond rendering it as an
  explicit dependency-graph tree (Deliverable 4).
- **Any unit whose accept criteria reference per-pixel shading:** none
  found in `WORK-UNITS.md` — the only "per pixel" hits are already-correct,
  post-WU-32 text (WU-27's own note, WU-28b's I6 check). `docs/
  architecture.md`'s own pre-WU-32 "per-pixel lighting" line was already
  corrected by WU-32/C-022.

Two findings turned up that are outside F1–F10 but worth recording (per
Ground Rule 1's own "a deliberate unaffected, because X is the useful
output, not silence" — these are the "not silence" part): `src/core/
jacobian.hpp` line 136's own comment still reads "WU-27's own two-sided
Blinn-Phong shading," predating ADR-069's Phong correction — cosmetic, no
behavioural effect, not fixed (would reopen WU-26). `docs/architecture.md`
§4.7 still describes "nearest 8 depth-sorted layers" while
`core/types.hpp:166` ships `kBufferK = 4` (ADR-059/060's own deliberate,
scoped choice) — a pre-existing documentation mismatch, unrelated to this
session's findings, flagged for a future session to reconcile.

Also found, reading `WU-12b`'s own entry against ADR-072/074: a second,
already-shipped, order-driven arbitration mechanism (`compositeLayered()`)
that ADR-074's swappable-interface binding requirement doesn't mention at
all. Added as a scope item to WU-35, not a correction to WU-12b.

Also found, reading `WU-22c`'s own entry end to end: its header/opening
paragraph correctly say `wu-22c-green` is real (confirmed again this
session, `git describe --tags a40e403` → `wu-22c-green`), but a later
`*Status:*` paragraph still reads "UNVERIFIED IN FULL... needs
`cmake --build`... before this unit can be called `green`" — stale text
from mid-session, contradicting the entry's own corrected opening. Recorded
as a green-tag review candidate (documentation severity), not corrected in
place — Ground Rule 2.

Produced `docs/wu-audit-2026-08.md` (new): the full 53-row audit register
(one row per unit, all six verdict categories represented in the tally —
44 unaffected, 5 plan amended, 4 superseded, 0 acceptance-criteria-wrong, 0
needs-rework, 0 new-unit-required within the 53), the green-tag
review-candidate list (WU-22c, WU-19b, WU-12b, WU-26, plus the
architecture.md k-buffer-depth mismatch — ordered by severity, none
reopened), a dependency-graph tree (∂z as the hard predecessor of WU-27/
WU-28c-family/WU-34(soft)/WU-35, with WU-33's real footprint and WU-35's
`compositeLayered()` reconciliation item marked), and a blocked-work
register (specular-LUT tables and the arbitration-mechanism choice, each
with what can be built around the gap now).

Amended `WORK-UNITS.md`'s Phase 7: gave WU-27 its first-ever `Files:`/
first-cut scoping (tiered [C], explicitly not frozen — whoever builds it
re-verifies against real code as every other unit's own build session
does), added scope notes to WU-29 (F2's fixed view vector vs. an
environment map's usual premise), WU-33 (real footprint per the F6 source
read above) and WU-35 (the continuation prompt's own suggested "k=1 soft-z"
Phase 7 shape is superseded by the already-shipped k=4 design — WU-35
remains the correct reconciliation vehicle, not a WU-28 restructure; plus
the `compositeLayered()` item), and appended one new unit, **WU-37**
(specular model LUTs, stubbed pending the real Starlight patent) — proposed
numbering, per Ground Rule 3. Did not renumber or reopen anything.

Delivered `docs/wu-audit-2026-08.md`, `WORK-UNITS.md` and this file to the
real repository via `SendUserFile` + `device_commit_files`, then re-staged
all three from the device and diffed against this session's own edited
copies before writing this sentence — `SESSION-PROTOCOL.md`'s own rule 8.

## Where we are

No unit's `Status:`/tag changed — this was a documentation-only sweep.
Phase 7 entries gained scope notes (WU-27, WU-29, WU-33, WU-35) and one new
unit (WU-37); nothing green was reopened. `DECISIONS.md` still runs through
ADR-074, `INVARIANTS.md` through I11, `CORRECTIONS.md` through C-023 — none
touched this session (no new finding rose to a correction; see above for
the two cosmetic/documentation items recorded in the audit doc instead).

## Next work unit

Unchanged in substance from WU-32's/Session-41's own handoff: **WU-28d**
remains the natural next pick if Steve wants to see self-fold occlusion on
real hardware (DeckLink-linked, reasoned-through-only from this sandbox).
**WU-27** is now scoped for the first time (see `docs/wu-audit-2026-08.md`
and this session's own `WORK-UNITS.md` amendment) and is a strong
alternative pick — "the single highest-value line change in the repo," per
the continuation prompt that opened WU-32. **WU-33** and **WU-35** are both
now flagged as materially bigger than their one-line `todo` entries implied
before this sweep; either is pickable, but whoever does should read
`docs/wu-audit-2026-08.md`'s Deliverable 1/4 entries for that unit first,
not just its own `WORK-UNITS.md` line. **WU-37** (specular LUTs) can
proceed in parallel with WU-27, per the blocked-work register's own
unblock-the-mechanism-not-the-curves reasoning.

## Open questions

Unchanged from WU-32's own handoff, plus one addition:
`kCaptureRingCapacity` = 8, Q3 (macOS/Desktop Video version), Q4 (lattice
edge damping, C-008(a)), Task A1 (UK 2,158,671 in full, gates WU-35), Task
D6 (re-derive the 1080p50 budget, C-023 — now also a named predecessor note
on WU-25 itself), ADR-070's coarse-grid-facet-normal-vs-WU-26-normal open
question. **New this session:** whether WU-35 should fold WU-12b's
`compositeLayered()` behind the future swappable arbitration interface, or
document it as a permanently separate mechanism — not decided, deliberately
(this is design work, out of scope for a documentation-only sweep).

## Blocked / red

Nothing red. Nothing newly blocked — WU-27's closed-form/falloff/view-vector
work and WU-37's LUT interface are both confirmed *not* blocked on the
Starlight patent, only the exact tabulated curve values are (see the
blocked-work register in `docs/wu-audit-2026-08.md`).

## Environment check

Unchanged from Session 41 — nothing in this session touched a source file,
so nothing needed rebuilding. The sandbox's own four-configuration matrix
was not re-run because nothing build-affecting changed (`docs/
wu-audit-2026-08.md` is new but untracked by any CMake target;
`WORK-UNITS.md`/`HANDOFF.md` are not compiled).

## Append to DECISIONS.md

Nothing this session — no design decision was made; two design *questions*
were raised (WU-29's environment-map premise, WU-35's `compositeLayered()`
reconciliation) and left open, deliberately, for whoever scopes those units.

## Append to CORRECTIONS.md

Nothing this session — the two documentation issues found (the stale
Blinn-Phong comment, the architecture.md k-buffer-depth mismatch) are
pre-existing slips, not errors made by this session, and are cosmetic/
documentation-severity rather than a corrected claim; recorded in
`docs/wu-audit-2026-08.md`'s green-tag review-candidate list instead, per
that deliverable's own purpose.

## Closed out this session

**WU-36 — work-unit re-plan sweep against WU-32's findings.** Documentation
only: `docs/wu-audit-2026-08.md` (new), `WORK-UNITS.md` (Phase 7 amendment),
this file. Ready for Steve's own build (trivially green, nothing compiled
changed), commit, tag and push.

## Steve's own next steps

**1. Confirm the tree at your own real terminal.**

```
cd ~/src/scatter-dve
git status --short
```

Expected: `docs/wu-audit-2026-08.md` untracked (new file), `WORK-UNITS.md`
and `HANDOFF.md` modified. Nothing else.

**2. Build and test, to confirm nothing regressed (expected: trivially
green, since no source file changed).**

```
cd ~/src/scatter-dve
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: the same pass count as your last run, including the same
already-accepted `test_decklink_device`/`foundDuplexDevice` exception
(ADR-035) if the DeckLink SDK is configured.

**3. Commit, tag and push.**

```
cd ~/src/scatter-dve
git add docs/wu-audit-2026-08.md WORK-UNITS.md HANDOFF.md
git commit -m "WU-36: work-unit re-plan sweep against WU-32's findings"
git tag -a wu-36-green -m "WU-36: work-unit re-plan sweep, documentation-only, four-config matrix unchanged"
git push origin main
git push origin --tags
```

**4. Verify it landed correctly:**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`, carrying
`wu-36-green`; `git status -sb` should read `## main...origin/main` with no
ahead marker and no modified files listed at all.
