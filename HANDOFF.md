# Handoff
Overwritten at the end of every session. This is the first thing to read.

---

**Session:** 40 (WU-32, historical-findings import — documentation-only,
per its own opening brief; no production source file touched, one new
standing regression test added). WU-28c/WU-21d's own real-terminal
close-out status is unchanged from Session 39 — not this session's job,
not touched.

**Tag:** no new tag exists yet. `wu-32-green` is Steve's own next action —
see "Steve's own next steps" below. Unlike every WU-28-adjacent unit before
it, this one changes no `scatter-core`/`scatter` source file at all (only
one new test file plus `CMakeLists.txt`'s own registration of it), so the
real-terminal step is materially smaller than usual: rebuild, run the new
test, confirm nothing else moved, tag.

## Before doing anything else in the next session

Run `git tag`, `git log --oneline -10`, `git status --short` and `git status
-sb` directly against `~/src/scatter-dve` via the device bridge, the same as
every session before this one — do not trust this file's own account of
tag/commit state without checking it against the real repository first.

## This session in full

Opened from a continuation prompt (pasted by Steve) asking to import two
research documents' findings — `WU-SM-01` (Mirage shape/UI/storage model,
draft 0.3) and `WU-SM-02` (surface arbitration and the front/back source
pair, draft 0.1), both derived from primary Quantel sources — into this
repository's own state files. The prompt was explicit that it might be
working from a stale summary of repository state and instructed verifying
`DECISIONS.md`/`INVARIANTS.md`/`WORK-UNITS.md`/`HANDOFF.md`/`docs/
gpu-route-assessment.md` before writing anything, and to ask before choosing
this unit's own number rather than assume.

Requested device-bridge access to `~/src/scatter-dve` and read
`SESSION-PROTOCOL.md`, `HANDOFF.md` (Session 39's), `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md`, `INVARIANTS.md`, `docs/architecture.md`
and `docs/workflow.md` in full before writing anything, per standing
discipline.

**Verification found the continuation prompt's premises did not fully match
real repository state, in several concrete ways — surfaced to Steve before
any file was written, per the prompt's own instruction:**

1. `docs/gpu-route-assessment.md` does not exist in this repository (`find`
   confirmed directly, not assumed). The equivalent content — the Starlight
   phase description, the transparency-mechanism design note, the patent
   provenance list — lives in `docs/architecture.md` §0/§4.7/§10/§13.
   Corrections targeted that file instead.
2. `docs/architecture.md` §13 already cited **US 5,103,217 (Cawley)**, not
   EP0320166A1, as its Starlight provenance — there was no wrong-patent
   citation in this repository to fix. The wrong-attribution history
   `WU-SM-01.md` §2.1 describes belongs to an earlier scatter-dve working
   assumption, not to this file's current text. See CORRECTIONS.md C-022.
3. `WORK-UNITS.md`'s WU-27 ("Blinn-Phong, linear light, two-sided") has
   never carried `Files:`/`Accept:` content past its one-line backlog
   title — there was no existing acceptance criteria to rewrite, only a
   title to correct and a scoping note to leave for whoever picks it up.
4. `WORK-UNITS.md`'s WU-28 is not `todo` — it is already split into
   WU-28a/WU-28b (`green`, a k-buffer resolve with `Opaque`/`Blend` modes,
   built and sandbox-tested across several sessions, `DECISIONS.md`
   ADR-059–ADR-061), WU-28c (`wip`, per-fragment facing tags), and WU-28d
   (`todo`, wiring into the live demo). Its shipped blend formula
   (`ADR-061`) does not implement the transparency-coefficient rule the
   research documents describe (`ADR-072`), and it was built — ADR-059
   says so explicitly — as an invented-from-scratch mechanism, because the
   real Mirage patent discloses no general multi-surface mechanism. "WU-28
   becomes sheet arbitration" (the continuation prompt's own framing) does
   not match this — WU-28a–d are not to be touched, renumbered or
   reinterpreted as unbuilt.
5. Two of the nine `DECISIONS.md` items the prompt asked for ("address map
   plus soft stencil", and two others) were not fully explained by the
   findings pasted into the prompt itself; `WU-SM-01`/`WU-SM-02` had not
   been attached. Flagged to Steve, who then attached both — all nine items
   were promoted with real provenance once they arrived. One correction
   surfaced in the process: the source documents' own Task A5 (mask width:
   one bit or a wider soft stencil) is explicitly **unresolved**, so
   "address map plus soft stencil" overstated what is actually settled —
   promoted as "address map plus validity mask, width open" instead
   (`ADR-066`).

Two decisions were put to Steve before writing, per the prompt's own "ask
before choosing this unit's own number" instruction and the scale of the
WU-28/research divergence above:

- **This unit's own number: WU-32, main sequence** (not a new `WU-Dn`
  documentation series, Steve's explicit choice over the recommended
  alternative).
- **The WU-28/`ADR-072` gap: a new forward-looking unit (WU-35), not
  touching WU-28a/WU-28b/WU-28c/WU-28d's own files or status**, promoting
  `ADR-072`/`ADR-074` as documentation and leaving the actual
  reconciliation — a swappable arbitration interface, the transparency-
  coefficient blend formula, retiring or superseding WU-28b's placeholder —
  as a scoped-but-not-built future unit (Steve's explicit choice, the
  recommended option).

**Deliverables, once both documents were in hand:**

1. `DECISIONS.md` — nine ADRs appended, ADR-066 through ADR-074, each with
   a provenance line back to `docs/sources/WU-SM-01.md`/`WU-SM-02.md`'s own
   section and confidence tier ([A]/[B]/[C]/[P] preserved, not flattened).
   Real next number confirmed directly (`grep -oE "ADR-[0-9]+"`, max was
   ADR-065) before use, per the prompt's own instruction not to guess.
2. `INVARIANTS.md` — four additions, I8–I11 (nothing culled; no per-sample
   normal/depth in the lattice format; shading pre-projection; within-sheet
   accumulate vs. between-sheet blend as separate stages).
3. `CORRECTIONS.md` — two entries, C-022 (`docs/architecture.md`'s
   Blinn-Phong/per-pixel-evaluation text, and the non-existent wrong-patent
   citation — see point 2 above) and C-023 (the ADR-007 1080p50 headroom
   figure predates I8/`ADR-070`/`ADR-072` and cannot be inherited without
   re-deriving it — no replacement figure computed here, per Task D6 in
   `WU-SM-02.md` §5.2, left for whoever does that re-derivation).
4. `WORK-UNITS.md` — WU-26 gained a note that its own `∂z` gates three
   consumers (shading normal, facing sign, future depth-gradient
   tolerance), not touching its `Files:`/`Accept:`/`Status:`; WU-27 renamed
   Phong (was Blinn-Phong) with a scoping note, no invented `Files:`/
   `Accept:`; three new units added, WU-33 (front/back source pair), WU-34
   (coarse-grid shading), WU-35 (sheet arbitration v2, forward-looking
   only); this unit's own WU-32 entry added under a new "Cross-cutting
   documentation units" heading (numbers do not track document position
   for these three new units — WU-33/34/35 sit inside the Phase 7 section
   by topic, ahead of Phase 8's lower-numbered WU-30/31, flagged plainly
   rather than silently reordered).
5. `docs/architecture.md` — §0 ("per-pixel lighting" → coarse-grid-
   evaluated), §10 (Phase 7's Blinn-Phong line corrected, cross-referenced
   to the new ADRs and work units), §13 (the S6 Starlight-patent citation
   added; a note that no wrong-patent citation existed to fix).
6. `docs/sources/WU-SM-01.md`, `docs/sources/WU-SM-02.md` — dropped in
   verbatim, own numbering intact. `docs/sources/README.md` — new, the
   `ADR-SM-nnn → ADR-0nn` mapping table, and an explicit list of what was
   **not** promoted this session (`ADR-SM-001/002/004–008/010/013/019/021`
   remain proposals in the source documents only).
7. `tests/fixtures-historical.md` — new, all 32 fixtures from both
   documents, fixture → owning WU → status. Most rows are `not runnable`
   (the owning unit doesn't exist yet) — the table itself is the
   deliverable, as the opening brief said it would be.
8. `tests/test_scan_order_invariance.cpp` — new, the one actual test.
   **Narrower than the fixture's own name**, honestly: covers the row (v)
   traversal axis only (four distinct row-visitation orders — forward,
   reverse, even-then-odd, block-reversed — through the existing
   `generateFragmentsRowRange()` public API, checked bit-identical via
   `splatTile()`/`sumBanks()`), not the full four-way `(u, v)` combination
   the fixture names, because covering the column (u) axis would need a
   traversal-direction parameter `core/binner.cpp` does not expose today —
   adding one is production code beyond a documentation-only unit's scope.
   Flagged in the test file's own header, in `tests/fixtures-historical.md`
   fixture 29's row, and here, rather than silently claiming full coverage.
   `CMakeLists.txt` gained its `scatter_test()` registration, same pattern
   as every other test target.

Delivered every changed/new file to the real repository via `SendUserFile`
+ `device_commit_files`, then **re-staged each one from the device and
diffed against this session's own edited copy before writing this
sentence** — `SESSION-PROTOCOL.md`'s own rule 8, not inferred from the
write call returning without error alone.

## Where we are

Phase 6/Phase 5 unchanged from Session 39. Phase 7: WU-26 `wip` (unchanged,
gained a documentation-only note), WU-27 renamed and still `todo` (no
`Files:`/`Accept:`, unchanged in substance), WU-28a/WU-28b `green`, WU-28c
`wip`, WU-28d `todo` (none touched — status, files and content all
unchanged from Session 39), WU-29 `todo` (unchanged), WU-33/WU-34/WU-35
new, all `todo`, none scoped past a note. `DECISIONS.md` runs through
ADR-074. `INVARIANTS.md` runs through I11. `CORRECTIONS.md` runs through
C-023. `WORK-UNITS.md` gained the "Cross-cutting documentation units"
section, WU-32.

## Next work unit

**Steve's own real-terminal close-out for WU-32** — see below; small,
since no `scatter-core`/`scatter` source file changed. After that, several
independently pickable units are now real: WU-27 (rename done, real scoping
still needed against ADR-069/070/071), WU-33 (front/back source pair),
WU-34 (coarse-grid shading), WU-35 (sheet arbitration v2, blocked in
substance on Task A1 — UK 2,158,671 in full — per `docs/sources/
WU-SM-02.md` §7, not blocked procedurally). None of WU-28a/WU-28b/WU-28c/
WU-28d's own next-step guidance (Session 39's HANDOFF, preserved above by
implication) has changed.

## Open questions

Unchanged from Session 35–39: `kCaptureRingCapacity` = 8 (WU-20a/20b,
ADR-046), Q3 (macOS/Desktop Video version), Q4 (lattice edge damping,
C-008(a)). New this session, carried from the research documents rather
than this project's own prior sessions: Task A1 (UK 2,158,671 in full —
gates `ADR-074`'s M1/M2/M3 choice and therefore WU-35), Task D6
(re-derive the 1080p50 performance budget — C-023), and the open design
question `ADR-070` records (coarse-grid facet normal vs. WU-26's exact
analytic normal — whichever of WU-27/WU-34 gets scoped first should settle
it, not silently pick one).

## Blocked / red

Nothing red. Nothing newly blocked. WU-35 is blocked *in substance* on Task
A1 (not a repository-state block); noted above, not new to this session's
own scope.

## Environment check

Unchanged from Session 39. Nothing this session touched any DeckLink-linked
or Cocoa-linked file, or any `scatter-core`/`scatter` source file at all.

## Append to DECISIONS.md

ADR-066 through ADR-074 — appended in full this session; see `DECISIONS.md`.
Does not reopen any existing ADR, including ADR-059/060/061 (WU-28a/WU-28b's
own k-buffer design and build) and ADR-007 (1080p50 headroom, corrected only
via CORRECTIONS.md C-023, not reopened or rewritten).

## Append to CORRECTIONS.md

C-022 (`docs/architecture.md`'s Blinn-Phong/per-pixel text, and the
non-existent wrong-patent citation) and C-023 (the ADR-007 1080p50 headroom
figure needs re-deriving, not inheriting) — appended in full this session;
see `CORRECTIONS.md`.

## Closed out this session

Nothing tagged this session — WU-32 needs Steve's own real-terminal
`build`/`ctest` to confirm `test_scan_order_invariance` before a tag is
appropriate, consistent with "the assistant does not run `close.sh`" and
does not tag units on cloud-sandbox evidence alone.

## Steve's own next steps

**1. Rebuild and test WU-32 at your own real terminal.** No hardware setup
needed — every changed/new file is either documentation or a
`scatter-core`-only test.

```
rm -f ~/src/scatter-dve/.git/index.lock
cd ~/src/scatter-dve
git status --short
cmake --build build
ctest --test-dir build --output-on-failure
```

Expected: everything green except `test_decklink_device`'s own
`foundDuplexDevice` check — ADR-035's already-accepted exception, which
fires on every real-terminal run regardless of what a given unit touches.
`test_scan_order_invariance` should appear as a new passing target; every
other test's own result should be unchanged from your last real-terminal
run, since no `scatter-core`/`scatter` source file was touched.

**If anything else fails, stop here — don't tag, don't proceed to step 2 —
and paste me the full `--output-on-failure` output.**

**2. Commit, tag and push.** Same manual-tag fallback every DeckLink-adjacent
close-out has used since ADR-035 (`close.sh` refuses to tag past that known
exception) — `git add`/`git commit` before `git tag`, so the tag lands on
the commit that actually contains these changes (the exact mistake C-021
recorded, avoided here):

```
cd ~/src/scatter-dve
git add DECISIONS.md INVARIANTS.md CORRECTIONS.md WORK-UNITS.md HANDOFF.md \
        docs/architecture.md docs/sources/WU-SM-01.md docs/sources/WU-SM-02.md \
        docs/sources/README.md tests/fixtures-historical.md \
        tests/test_scan_order_invariance.cpp CMakeLists.txt
git commit -m "WU-32: import WU-SM-01/WU-SM-02 historical findings (ADR-066..074)"
git tag -a wu-32-green -m "WU-32: historical findings import green (test_decklink_device/foundDuplexDevice is ADR-035's known exception)"
git push origin main
git push origin --tags
```

**3. Verify it landed correctly:**

```
cd ~/src/scatter-dve
git log --oneline -3
git tag | tail -5
git status -sb
```

`git log --oneline -3` should show your own new commit at `HEAD`, carrying
`wu-32-green`; `git status -sb` should read `## main...origin/main` with no
ahead marker and no modified files listed at all.
