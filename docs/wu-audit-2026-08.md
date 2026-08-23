# Work-unit re-plan audit — August 2026

Companion to WU-32 (import of `docs/sources/WU-SM-01.md`/`WU-SM-02.md` into
`DECISIONS.md`/`INVARIANTS.md`/`CORRECTIONS.md`). This is WU-36 (see
`WORK-UNITS.md`'s Cross-cutting documentation units section): a full
work-unit-sequence audit against the ten findings WU-32 landed, plus a
revised Phase 7 plan, a green-tag review-candidate list, a dependency graph,
and a blocked-work register. Documentation-only — no production code
touched, no green tag reopened.

**Findings audited against** (see `DECISIONS.md` ADR-066–074,
`INVARIANTS.md` I8–I11, `CORRECTIONS.md` C-022/C-023 for the full text):

- **F1** Starlight patent misidentified; real one (EP 0248626 / US 4,899,295) not yet obtained
- **F2** Lighting is Phong, not Blinn-Phong
- **F3** Shading evaluated per coarse grid, filtering ladder + grid shift
- **F4** Lighting composited write-side, before projection
- **F5** Address map runs forwards
- **F6** Two video sources, front and back, independently freezable
- **F7** Nothing is culled; back faces always splat
- **F8** Sheet arbitration is a parameterised blend, coefficient `T`
- **F9** Mechanism probably a depth plane — still open [C]
- **F10** ∂z in `Lattice::jacobian()` gates three consumers

Total units audited: **53** (WU-01 through WU-35, plus WU-30/31/32). Two
further units are proposed at the end of this document (WU-36, this sweep;
WU-37, specular-model LUTs) — not folded into the 53, since they did not
exist in `WORK-UNITS.md` at the start of this sweep.

---

## Deliverable 1 — Audit register

Verdicts: **unaffected** · **plan amended** · **acceptance criteria wrong** ·
**needs rework** · **superseded** · **new unit required**.

### Phase 1 — Portable core (WU-01…10)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-01 | Repo skeleton | green | none | unaffected | None. Predates any lattice/shading/arbitration concept. |
| WU-02 | v210 unpack/pack | green | none | unaffected | None. |
| WU-03 | Test pattern generator | green | none | unaffected | None. |
| WU-04 | Chroma resampling | green | none | unaffected | None. |
| WU-05 | File I/O, identity passthrough | green | F5 (tangentially) | unaffected | Grepped: no address-map-direction language anywhere in this unit's own files or accept text. I7/I1 already state "forward scatter" correctly. |
| WU-06 | Lattice and Jacobian | green | F10 (predecessor) | unaffected | This unit's own `jacobian()` is exactly what WU-26 later extends with ∂z — no change to WU-06 itself, its own accept criteria (2D derivatives only) stand unchanged. |
| WU-07 | K and EWA footprint | green | none | unaffected | None. |
| WU-08 | Fragment generation, tile binning | green | F7 | unaffected, confirmed by source read | Grepped all of `src/` for cull/early-out/back-face-reject/discard: zero hits that are generation-time rejections. `generateFragments()` has no facing check at all — I8 holds structurally, not just by absence of a bug report. |
| WU-09 | Four-bank splat | green | F7 (indirectly) | unaffected | Splat has no notion of facing; doubled contention from I8 is a cost property of upstream fragment volume, not something this unit's own accept criteria (bit-exact vs. reference, int64 headroom) claim anything about. |
| WU-10 | Normalise/composite, first affine warp | green | F4 (indirectly) | unaffected | `composite()` has no shading hook and none is implied by its accept criteria; consistent with I10 (shading is pre-projection, not a resolve-time operation). |

### Phase 2 — Shapes (WU-11…13)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-11 | Cylinder and sphere | green | F7 | unaffected, because already correct | This unit's own accept text *already* says "not testing self-occlusion or back-face correctness... overlapping surface points are expected to simply accumulate, not be sorted or culled" — written before F7 existed, and F7 confirms it was right. Textbook "unaffected, because X." |
| WU-12a | Page turn, transparent mode | green | none | unaffected | None. |
| WU-12b | Page turn, priority-tag opaque | green | F8, F9, F10 (via ADR-074) | **unaffected, but see green-tag candidate below** | `compositeLayered()` is a second, shipped, caller-order-driven arbitration mechanism, distinct from WU-28's k-buffer. ADR-074's "single swappable interface" binding requirement doesn't mention it. Not wrong on its own terms (page-turn opaque mode, correctly built and scoped at the time) — but WU-35's own scope should explicitly decide whether to fold it behind the future swappable interface or document it as a permanently separate, page-turn-specific mechanism. See green-tag review candidates. |
| WU-13 | Keyframed lattices (morph) | green | none | unaffected | Pure lattice mathematics, no shading/arbitration/facing surface. |

### Phase 3 — SDI output (WU-14, 15a, 15b)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-14 | DeckLink enumeration | green | none | unaffected | None. |
| WU-15a | Scheduled playback, one frame | green | none | unaffected | None. |
| WU-15b | Scheduled playback endurance | green | none | unaffected | None. |

### Phase 4 — Threading and NEON (WU-16a…19b)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-16a | Thread pool PASS 2 | green | none | unaffected | None. |
| WU-16b | Thread pool PASS 1 row-band | wip | none | unaffected | None. |
| WU-17 | NEON v210 | green (pending tag) | none | unaffected | None. |
| WU-18 | NEON chroma | green (pending tag) | none | unaffected | None. |
| WU-19a | Persistent ThreadPool | wip | none | unaffected | None. |
| WU-19b | Real-time measurement, 576i25 | green | F7 (context), C-023 | **unaffected — see green-tag candidate below** | This unit's own claim is 576i25 only, measured directly on real hardware, not extrapolated — it is not undermined. C-023 (which the continuation prompt names alongside this unit) targets ADR-007's *separate* 1080p50 headroom conclusion, not WU-19b's own 576i25 numbers. Flagged explicitly below per the prompt's own instruction, with this distinction spelled out so it isn't miscited as needing rework. |

### Phase 5 — Live capture (WU-20a…22c)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-20a | Ring buffer | green | none | unaffected | None. |
| WU-20b | DeckLink capture, format detection | green | F6 (forward context) | unaffected | `CaptureSource` wraps exactly one `IDeckLinkInput`. Correct and complete for what it claimed (single-source capture); F6 means a second instance of this class (or an equivalent) is part of WU-33's real footprint. No rework — see WU-33's amended scope below. |
| WU-21a | `runFrameBytes()` | green | F6 (forward context) | unaffected | Single `SourceRaster` signature, as designed. Same note as WU-20b: WU-33 will need to extend `runFrame`/`runFrameBytes`/`runFrameFile`'s signatures, not just add new functions beside them — flagged there, not here. |
| WU-21b | Capture-side pixel read | green | none | unaffected | None. |
| WU-21c | Continuous SDI re-output | green | none | unaffected | None. |
| WU-21d | Cold-start black fill | wip | none | unaffected | None. |
| WU-21e | Live sphere demo (static geometry) | superseded | none | superseded | Self-superseded by WU-21f before being tagged; unrelated to these findings. |
| WU-21f | Rotating live sphere (two-axis) | superseded | none | superseded | Self-superseded by WU-21g; unrelated to these findings. |
| WU-21g | Full pole-to-pole wrap | superseded | F7 (context) | superseded | Self-superseded by WU-21h. Its own text already names the un-occluded front/back overlap as a backlog item for WU-28 — consistent with, not contradicted by, F7/I8. |
| WU-21h | Interactive UI, cursor+shift | superseded | none | superseded | Self-superseded by WU-21i (broken shift+cursor sequence, C-018); unrelated to these findings. |
| WU-21i | Letter-key manual controls | green (reasoned-through) | none | unaffected | None. |
| WU-22a | Coverage-view weight capture | green | none | unaffected | None. |
| WU-22b | Coverage-view Metal window | green | none | unaffected | None. |
| WU-22c | Coverage-view wired into live pipeline | green | none directly; internal doc defect found | **unaffected — see green-tag candidate below** | Not related to F1–F10, but this unit's own entry is internally self-contradictory (see below) and the continuation prompt named it explicitly. |

### Phase 6 — Scale up (WU-23…25)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-23 | Interlace and field mode | todo | none | unaffected | Unscoped; nothing in F1–F10 bears on field mode. |
| WU-24 | Adaptive supersampling | todo | none | unaffected | Unscoped; unrelated. |
| WU-25 | 1080p50, tile-size tuning | todo | C-023 | **plan amended** | Whoever scopes WU-25 must re-derive `architecture.md` §11's performance budget against I8 (doubled contention), ADR-070 (cheaper coarse-grid shading) and ADR-072/074's future arbitration cost, per C-023's own Task D6, before repeating ADR-007's "factor of ten headroom" conclusion for 1080p50. Added as an explicit predecessor note; no `Files:`/`Accept:` existed to be "wrong," so this is a plan amendment, not a correction. |

### Phase 7 — Starlight (WU-26…35)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-26 | Normals from lattice | wip | F10 | **unaffected — already fully absorbs the finding** | WU-26's own WU-32-session note already names all three downstream consumers (WU-27 normal, WU-28c/WU-33 facing sign, WU-35 depth gradient) and states ∂z is a hard predecessor. Nothing left to amend. One cosmetic issue found by source read: `src/core/jacobian.hpp` line 136's comment still says "WU-27's own two-sided Blinn-Phong shading" — stale, predates ADR-069. See green-tag candidates (cosmetic). |
| WU-27 | Phong, linear light, two-sided | todo | F1, F2, F3, F4 | **plan amended** | Never had `Files:`/`Accept:` in the repo; this sweep adds a first-cut scoping below (Deliverable 2), tiered [C] where it goes beyond ADR-069/070/071's own text. Still has a live, unresolved design fork (ADR-070's facet-normal-vs-WU-26-normal question) that whoever builds this must decide, not this sweep. |
| WU-28a | k-buffer storage | green | F7, F8, F9 (context) | unaffected | Its own accept criteria (synthetic multi-tag byte-identity, I6) are unaffected. The real-content gap (single tag per call) was already found and logged as C-020/ADR-062 before this sweep, and closed by WU-28c. Nothing new. |
| WU-28b | k-buffer resolve | green | F8, F9 (context) | unaffected | Same reasoning. ADR-074 already documents, in the same session that found F8/F9, that this unit's shipped blend formula does not implement the `T`-coefficient rule and is not behind a swappable interface — tracked at WU-35, not reopened here. |
| WU-28c | Self-fold facing tag | green | F10 (consumer) | unaffected | Built on WU-26's normal exactly as intended; no discrepancy found. |
| WU-28d | Wire self-fold occlusion into live demo | todo | none new | unaffected | Scope unchanged by F1–F10. |
| WU-29 | Environment map | todo | F2 | **plan amended** | F2's fixed (orthographic) view vector is a real premise change for an environment map, which conventionally implies a *reflection* direction that depends on view position. Whoever scopes WU-29 must decide whether a historically-faithful Starlight environment map is even meaningful under a fixed view vector, or whether this becomes a deliberate, flagged departure. Noted as a predecessor-scoping question, not resolved here (would require a design decision this documentation-only sweep is not permitted to make). |
| WU-33 | Front/back source pair | todo | F6 | **plan amended** | Confirmed by reading `core/binner.hpp/.cpp`, `core/resolve.hpp`, `core/pipeline.cpp`, `io/decklink_capture_consumer.hpp/.cpp`: every ingest entry point takes exactly one `SourceRaster`/one `Lattice`. This unit's real footprint includes modifying those already-green files (new `SourceRaster` parameter threaded through `runFrame`/`runFrameBytes`/`runFrameFile`/`generateFragments*`, and a second live capture path), not just adding new ones beside them. Scope note added below (Deliverable 2); **not** moved earlier in the sequence per Ground Rule 3 (no renumbering) — flagged instead as the single most consequential open scoping question in Phase 7. |
| WU-34 | Coarse-grid shading | todo | F3 | unaffected | Already correctly scoped by WU-32 against ADR-070, including the open facet-normal-vs-WU-26-normal note. Nothing to add. |
| WU-35 | Sheet arbitration v2 | todo | F8, F9 | **plan amended** | Already correctly scoped against ADR-072/074 by WU-32. This sweep adds one scope item: WU-35 should explicitly decide WU-12b's `compositeLayered()`'s relationship to the swappable interface (fold it in, or document it as permanently separate) — see green-tag candidates. |

### Phase 8 — Authoring (WU-30, 31) and cross-cutting (WU-32)

| WU | Name | Status | Findings touching it | Verdict | Action |
|---|---|---|---|---|---|
| WU-30 | Embedded Lua shape programs | todo | none | unaffected | None. |
| WU-31 | OSC/WebSocket control | todo | none | unaffected | None. |
| WU-32 | Import WU-SM-01/02 findings | green | — | unaffected | This is the unit that produced F1–F10; trivially unaffected by its own output. |

**Tally: 44 unaffected · 5 plan amended · 4 superseded · 0 acceptance criteria wrong · 0 needs rework · 0 new unit required** (within the 53 audited; two additions proposed separately below).

---

## Deliverable 3 — Green-tag review candidates

Ordered by severity, not unit number. None of these are being reopened —
this is the review-candidate list Ground Rule 2 asks for.

1. **WU-22c — self-contradictory status text.** *Severity: documentation.*
   What was validated: the entry's header and opening paragraph correctly
   state `wu-22c-green` exists in the real repository, confirmed via `git
   tag`/`git describe` against commit `a40e403` (verified again this
   session, on the real device). What undermines it: the entry's own later
   `*Status:*` paragraph still reads "**UNVERIFIED IN FULL**... Needs
   `cmake --build build`, `ctest`... before this unit can be called
   `green`" — stale text from mid-session, before Steve's own build/tag/push
   landed, never struck. Not a finding from F1–F10; found by reading the
   entry itself end to end. Cost to fix: edit one paragraph in
   `WORK-UNITS.md`, no re-validation of any code needed — the tag is
   genuinely green.

2. **WU-19b — required flag, not actually undermined.** *Severity:
   documentation (risk of misreading, not a real gap).* What was validated:
   576i25, 720×576, real M1 Max hardware, four thread counts, both tile
   sizes — a direct measurement, not an extrapolation. What might seem to
   undermine it: C-023's note that ADR-007's 1080p50 headroom conclusion
   predates F3/F4/F7/F8. The two are not the same claim: WU-19b never
   asserts anything about 1080p50, and none of F3/F4/F7/F8 change what was
   actually measured (I8 was already true when this ran; nothing pushes
   the 576i25 number to be wrong). Included here only because the
   continuation prompt named it explicitly; recommended action is a
   one-line cross-reference in WU-25's entry (already added, Deliverable 1)
   pointing future readers away from citing WU-19b for 1080p50 confidence,
   not a revalidation of WU-19b itself.

3. **WU-12b — a second, untracked arbitration mechanism.** *Severity:
   behavioural (nothing is wrong today, but a future correctness question
   is currently unowned).* What was validated: `compositeLayered()`'s
   read-replace-write opaque mode, exactly reproducing patent FIG. 5's
   two-layer case — still correct for that case. What undermines it: F8/F9
   and ADR-072/074 describe a general sheet-arbitration requirement
   (swappable M1/M2/hybrid interface) and don't mention this mechanism at
   all, even though it is a second, shipped, order-driven arbitration path
   alongside WU-28's k-buffer. Re-validation cost: none required now — this
   is a scope-completeness gap for WU-35 to close (decide whether
   `compositeLayered()` moves behind the future interface or stays a
   documented exception), not a defect in WU-12b's own delivered code.

4. **WU-26 — stale terminology in a source comment.** *Severity:
   cosmetic.* What was validated: `surfaceNormal()`'s sign convention and
   derivation, still correct. What undermines it: `src/core/jacobian.hpp`
   line 136 still reads "WU-27's own two-sided Blinn-Phong shading" —
   written before ADR-069 corrected the model to Phong, never updated. No
   behavioural effect (the comment doesn't drive any logic). Cost to fix:
   one comment-line edit, whenever WU-27 is actually built (natural moment
   to touch this file again) — not urgent enough to justify reopening
   WU-26 on its own.

5. **`docs/architecture.md` §4.7 — "nearest 8 depth-sorted layers" vs.
   shipped `kBufferK = 4`.** *Severity: documentation.* Not a WU-32 finding
   and not one of F1–F10 — found incidentally while cross-checking WU-28a's
   own design against the architecture doc. `architecture.md` still
   describes an 8-deep k-buffer; `src/core/types.hpp:166` ships
   `kBufferK = 4`, confirmed by ADR-059/060 as the deliberate, scoped
   choice. Not attached to any WU's own status (architecture.md is
   reference material, not a work unit); flagged here so a future session
   updates the prose rather than assuming 8 is still current.

---

## Deliverable 4 — Dependency graph

The repository has no existing dependency-graph artifact (`WORK-UNITS.md`
states predecessors in prose, per unit). Rendered here as a plain tree; the
change of substance is exactly what WU-26's own WU-32-session note already
recorded — ∂z becomes a hard predecessor of three (now four, counting
WU-34's soft dependency) previously-sibling units, and arbitration
(WU-35) and shading (WU-27/WU-34) are independent of each other once ∂z
exists, so they can proceed in parallel.

```
WU-06 (Lattice/Jacobian, green)
  └─ WU-26 (∂z / surfaceNormal, wip) ── hard predecessor of:
       ├─ WU-27 (Phong shading)              ─┐
       ├─ WU-28c (facing tag, green) ──┐       │  independent of each other
       │    └─ WU-28d (live-demo wire) │       │  once WU-26 exists —
       │    └─ WU-33 (front/back src)──┴───────┤  may proceed in parallel
       ├─ WU-34 (coarse-grid shading, soft dep,│
       │         see ADR-070's open note)     ─┤
       └─ WU-35 (sheet arbitration v2, depth   │
                 gradient) ─────────────────────┘

WU-28a (k-buffer storage, green)
  └─ WU-28b (k-buffer resolve, green)
       └─ WU-28c (facing tag, green) [see above]
            └─ WU-28d, WU-33 [see above]

WU-33 (front/back source pair)
  ├─ requires WU-26, WU-28c (facing signal) — already stated
  └─ ALSO requires touching (not just extending) already-green:
       core/binner.hpp/.cpp, core/resolve.hpp, core/pipeline.cpp,
       io/decklink_input.*, io/decklink_capture_consumer.* — new finding
       this sweep, not previously noted in WU-33's own scope line.

WU-35 (sheet arbitration v2)
  ├─ requires WU-26 (depth gradient) — already stated
  ├─ requires WU-28a/WU-28b (k-buffer substrate) — already stated
  ├─ soft-requires WU-33 (fixtures 21/23/27/28/30 need real front/back
  │   content, not just tags) — already stated
  └─ should also resolve WU-12b's compositeLayered() — new scope item,
      this sweep.

WU-25 (1080p50 tuning)
  └─ should re-derive architecture.md §11's budget against I8/ADR-070/
      ADR-072/074 (C-023, Task D6) before repeating ADR-007's headroom
      claim — new predecessor note, this sweep.
```

---

## Deliverable 5 — Blocked-work register

| Blocked unit(s) | Blocking document | What can be built around the gap now |
|---|---|---|
| Specular model LUTs (proposed WU-37, see below); any exact Starlight highlight-shape verification for WU-27 | **EP 0248626 / US 4,899,295** (Nonweiler) — the real Starlight patent, identified (F1) but not yet obtained | Stub all eight named models (`Model 1`…`4`, `Ramp`, `Posterise`, `2 ring`, `4 ring`) as a pluggable `model(L, zone)` lookup-table interface with placeholder curves now, per ADR-069's own description of the mechanism shape (LUT on `cos B`) — swap in the real tabulated values once the patent arrives, no interface change needed. WU-27 itself is *not* blocked: ADR-069's closed-form illumination equation, falloff, and fixed view vector are already settled [A] independent of the LUT contents. |
| WU-35 (sheet arbitration v2) — specifically, resolving F9 (M1 vs. M2 vs. hybrid) | **UK 2,158,671** in full (only excerpted today), or an operator's first-hand account of the historical `Trail`/`Opaque` mutual-exclusion failure mode | Build the swappable arbitration interface itself now (ADR-074's binding requirement doesn't depend on which mechanism wins), with M1 (painter's-order) as a cheap placeholder implementation behind it — already known to be insufficient alone (fixtures 24/32) but useful for exercising the interface boundary and unblocking WU-33/WU-34 integration work without waiting on M2's confirmation. Do not ship M1 as the final answer; ADR-074 already forbids treating this placeholder as settling F9. |
| Fixture 31 (`Opaque`/`Trail` mutual exclusion) | No `Trail` facility exists in scatter-dve at all yet; no owning unit | Nothing to build around yet — this fixture stays unrunnable until a future Trail unit is scoped, independent of WU-35. Tracked here only so it isn't lost, per fixtures-historical.md's own note not to silently make the combination work once both exist. |
| Fixture 32 (Möbius closure) | No Möbius-strip shape generator in `src/core/shapes/` | Independent of any patent gap — this is a missing shape unit, not a blocked-on-research one. Not scheduled; noted so a future Phase 2/8 session doesn't conflate "no shape yet" with "blocked on a document." |
| ADR-067's WORLD-rooted axis tree; fixtures 1–8 | No blocking document — these are simply not yet scoped as any work unit (Phase 8, loosely) | Nothing to build around; genuinely just unscheduled, not blocked. Listed for completeness since fixtures-historical.md marks them "unscoped," not "blocked" — different category, included here only to avoid the two being conflated in a future sweep. |
