# WU-SM-02 — Surface Arbitration and the Front/Back Source Pair

**Project:** scatter-dve (Quantel Mirage emulation)
**Status:** Draft 0.2 — C1 answered from S1 p.11; M1 effectively excluded
**Date:** 2026-08-22
**Predecessor:** WU-SM-01 draft 0.3 (+ S7 addendum)
**Target generation:** Floating Viewpoint Mirage + Starlight
**Gates:** ADR-SM-017; informs WU-26, WU-27, WU-28
**Changes in 0.2.** S1 page 11 (State module, mode control) transcribed. It
answers Strand C outright: front/back overlap transparency is a **named,
continuously variable, operator-controlled facility with an explicit `Opaque`
override**, so arbitration is a defeatable stage rather than an emergent property
of write order. The `Opaque` / `Trail` mutual exclusion is a strong positive
signature of a depth plane (§3.4). ADR-SM-020 rewritten as a parameterised blend
rather than a binary decision. New question on the soft stencil (§7 A5) and on
`Auto Transp` "from stored shape" (§7 A6).

**Renumbering note:** WU-SM-01 §12 proposed "WU-SM-02 — the Starlight lighting
pipeline". That unit becomes **WU-SM-03**. This one takes 02 because it gates
work that WU-SM-03 assumes.

---

## 1. Why this unit exists

WU-SM-01 established that Mirage arbitrates between surface sheets — v1 sphere
effects are opaque, and S7 shows a cylinder whose far wall does not show through
the near one. It did not establish **how**, and no held source says.

That question was academic for the S1 machine, where `trans` projects offline and
the library track is a finished 2D warp field with occlusion already resolved for
one viewpoint. It stops being academic at FVP, which is what this project
targets: projection moves into the real-time path and hidden surface moves with
it. The FVP-era hardware must therefore have gained something the S1-era hardware
never needed, and that delta is a concrete thing to look for rather than an
abstract question.

This unit exists to answer it, or to establish that it cannot be answered from
available sources and that we are choosing rather than reconstructing.

### 1.1 Confidence tiers

As WU-SM-01 §1.1: **[A]** stated explicitly; **[B]** stated once or
inconsistently; **[C]** our inference; **[P]** proposal for scatter-dve.

---

## 2. A distinction WU-SM-01 was blurring

Three separate claims have been travelling together under "does Mirage have a
z-buffer". They need separating before anything else can be said.

| Level | Claim | Status |
|---|---|---|
| **Library** | The shape library track carries per-sample depth or normals | **Settled: no.** ADR-SM-003, WU-SM-01 §3.9 |
| **Run time** | A depth value exists per sample after transform and projection | **Trivially yes.** You computed z in order to divide by it; it is in hand at the splat |
| **Framestore** | The destination store carries a depth plane to compare against | **Open.** This is the actual question |

WU-SM-01 drafts 0.2 and 0.3 collapsed the first and third and concluded "Mirage
had no z-buffer" from a finding that only supports the first. Logged as a
correction in §9 below. **Nothing in ADR-SM-003 constrains the framestore.**

---

## 3. Evidence review

### 3.1 What is established

| # | Finding | Tier | Source |
|---|---|---|---|
| E1 | Mirage arbitrates between sheets; v1 spheres are opaque | [A] | Observed product behaviour |
| E2 | Near wall exterior occludes far wall interior cleanly, with the near wall's top edge cutting the interior band | [A] | S7 |
| E3 | Back-facing surfaces are rendered, not culled, and carry the BACK video source | [A] | S7 + WU-SM-01 §3.9.4.3 |
| E4 | Collision under minification accumulates and normalises — this is resampling, and it is correct | [A] | UK 2,119,594 as recited in S5 |
| E5 | S5's skin-store machine arbitrates by a 1-deep soft Z: replace / discard / blend-when-approximately-equal | [A] | S5 masking circuit 78 |
| E6 | S5's picture store gains "an additional plane" to hold the Z | [A] | S5 |
| E7 | Facing is available from the sign of the surface normal, independently of any arbitration | [A/C] | S5 FIG. 3 circuit 32; WU-SM-01 §4.6.2 |
| E8 | Neither UK 2,119,594 nor UK 2,158,671, **as recited by S5**, mentions depth comparison | [A] | S5 background |
| E9 | **"front/back overlap" is a first-class named concept** in the machine | [A] | S1 p.11 |
| E10 | `Manual Transp` "controls the transparency of front/back overlaps" — a continuous analogue parameter on a Joystick pot | [A] | S1 p.11 |
| E11 | `Auto Transp` "controls transparency from stored shape. Replaces Manual." | [A] | S1 p.11 |
| E12 | `Opaque` "makes front/back overlap opaque. **Overrides** Auto and Manual." | [A] | S1 p.11 |
| E13 | `Opaque`: "**Not in trail.**" / `Trail`: "selects non-decaying frozen trail. **Not in Opaque.**" | [A] | S1 p.11 |
| E14 | `Ext. Key` "uses an external video input as both a logo key **and transparency control**" — per-pixel transparency from an external source | [A] | S1 p.11 |
| E15 | S5's skin store carries "an S component defining a keying or stencil signal, **2 to 4 bits**" | [A] | S5 |

E8 is an argument from silence in a second-hand recital. It carries very little
weight and Task A1 exists to replace it.

### 3.2 The two candidate mechanisms

**M1 — traversal-order painter's algorithm.** The address map is walked over the
(u,v) lattice in a known order; later writes win. Correct if the walk is
back-to-front. For convex parametric shapes that is a matter of flipping the u
and/or v scan direction from the sign of the viewing direction — a two-bit
decision per field, no extra store, entirely plausible for the period.

**M2 — depth plane and compare**, as S5. More capable, needs framestore width,
demonstrably within Quantel's repertoire by 1987.

**M3 — both.** Ordering for the library shapes, depth for the awkward ones.

### 3.3 Why M1 now looks weak

The shipped library (WU-SM-01 §3.2) is the argument. It contains, as
**as-shipped defaults**, shapes that no single scan order renders correctly:

- `PAGE TURN LH` / `PAGE TURN RH` — the curl occludes the flat behind it
- `VORTEX`, `STARBURST 1`, `STARBURST 2`, `SPINY NORMAN`
- `TILE UNIVERSE true 3d` — the name is Quantel's, and it is pointed
- `EVOLVING MOBIUS STRIP twistup` and `EVOLVING MOBIUS STRIP turn inside-out`

A Möbius strip is about as hostile to M1 as a parametric sheet can be: there is
no scan direction that is back-to-front everywhere, by construction. Somebody
wrote it, tested it and shipped it in the default library.

**Caveat, and it is a real one.** WU-SM-01 §3.2 flags that `sh 55` appears twice
in the S2 library table — once as the second half of `54/55` and once alone — and
marks it a probable draft slip [B]. The Möbius evidence rests partly on an entry
with a known transcription oddity. Two 33-track runs are allocated either way,
and the argument does not depend on the Möbius entries alone: VORTEX, the
STARBURSTs and TILE UNIVERSE are independent instances. But the strongest single
example is the least clean one, and that should be said.

**The cost argument against M2 was also overstated.** Mirage's splat is *already*
a read-modify-write — it accumulates colour and weight per destination pixel. Adding
depth widens an existing RMW; it does not introduce a new access pattern. The
hard part at sample rate was built and paid for in the base machine. [C]

### 3.4 S1 p.11 answers Strand C, and the `Trail` exclusion discriminates

Strand C asked whether transparency between sheets is an explicit, defeatable
stage or something emergent from write order. **It is explicit** (E9–E12).
Mirage names the front/back overlap, puts a continuously variable transparency on
it under a Joystick pot, offers a second mode that derives it from the shape, and
provides an `Opaque` override that beats both.

An emergent write-order result cannot be overridden — write order is write order.
So M1 in its plain form is excluded. What survives is **M1′ — painter's ordering
with alpha blending**, where later writes blend over earlier ones with
coefficient `T`, and `Opaque` sets `T = 0`. That is a real candidate and must be
kept on the table.

**E13 excludes M1′ as well, and it is worth stating carefully because it is the
strongest single inference available.**

`Opaque` and `Trail` are declared mutually exclusive, in both directions. That
exclusion has to be explained by some shared resource.

- **Under M1′** there is nothing to explain. `Trail` means not clearing the
  framestore between fields; write ordering within a field is untouched. The two
  facilities are orthogonal and there would be no reason to forbid the
  combination.
- **Under M2** the conflict is immediate. `Trail` retains picture content from
  previous fields, but the **depth associated with that content is stale**.
  Depth-testing this field's samples against last field's depth plane either
  lets the trail occlude the live picture or causes the live picture to be
  wrongly rejected. Either way the combination is broken, so the manual forbids
  it.

An exclusion that is inexplicable under one mechanism and forced under the other
is exactly the kind of evidence this unit was created to find. **[C], but strong
— promoted above everything in Strand A.**

### 3.5 The error in WU-SM-01 0.3, inverted

WU-SM-01's second pass claimed Mirage was a machine "where the ghosting is the
point"; that was corrected on the observation that v1 spheres are opaque. S1 p.11
shows both statements describe real behaviour of the same machine, and clarifies
which is the default.

The accumulator's **natural** output is the blend: accumulate two sheets,
normalise, get 50/50. That is E4 operating without arbitration. `Opaque` is the
thing that had to be added, and it is what an operator selects for a solid shape.
The opaque sphere is an `Opaque` sphere.

This also re-reads WU-SM-01 §3.6, which records `Opaque` as **gone** in FVP.
Rather than a removed facility, it is most likely `Opaque` collapsing into the
zero end of S2's single continuous `TRANSP` control — the same collapse S2 applied
to `Manual Transp` / `Auto Transp`. [C]

---

## 4. The reframe: arbitration is a parameterised blend

The productive way to read E5 is that S5's three-mode compare is not a
compromised z-buffer. It is **the arbitration rule a splatting engine actually
requires**.

In a scatter engine two things collide at a destination pixel and they need
opposite treatment:

- **Adjacent source samples from the same sheet** land on overlapping footprints
  at nearly equal depth. These *must* accumulate — E4, ADR-001, correct
  resampling under minification.
- **A different sheet** lands at a materially different depth. This must *not*
  accumulate — E1, E2.

S5's three modes map onto exactly that: nearer sheet wins, farther sheet loses,
**same sheet accumulates**. The equal-depth blend band is the sheet
discriminator. This reconciles ADR-001 with occlusion instead of setting them
against each other, and it is what WU-28 is for.

### 4.0 The transparency coefficient

Draft 0.1 framed the between-sheet decision as binary. E10–E12 show it is not:
the nearer sheet wins **by `(1 − T)`** and the farther contributes `T`, where `T`
is the front/back overlap transparency. `Opaque` is `T = 0`; the unarbitrated
accumulator is `T = 0.5` after normalisation.

`T` has three documented sources, and they are exclusive of one another:

| Mode | Source of `T` | S1 wording |
|---|---|---|
| `Manual Transp` | a Joystick pot, one value for the whole picture | "controls the transparency of front/back overlaps. Replaces Auto." |
| `Auto Transp` | the shape | "controls transparency from stored shape. Replaces Manual." |
| `Ext. Key` | an external video input, per pixel | "uses an external video input as both a logo key and transparency control" |
| `Opaque` | forced to zero | "Overrides Auto and Manual." |

So the resolve rule is:

```
if   |z_new − z_stored| < sheet_tolerance      →  accumulate            (same sheet)
elif z_new nearer                              →  new·(1−T) + stored·T
else                                           →  stored·(1−T) + new·T
```

with `T = 0` reproducing hard occlusion. **[P]** for the exact form, **[A]** for
the requirement that a coefficient exists at all.

### 4.1 Tolerance from the Jacobian **[P]**

The open parameter is the band width, and there is a principled source for it.
Two contributions belong to the same sheet if their depth difference is within
what the **local depth gradient** predicts across a footprint.

`Lattice::jacobian()` is already computed for density compensation and
anisotropic footprints. Once it carries ∂z, the tolerance is a by-product rather
than a new term:

```
same_sheet(z_new, z_stored)  ⇔  |z_new − z_stored|  <  κ · ‖∂z/∂(u,v)‖ · footprint
```

Behaviour at the two regimes that matter:

- **Grazing angles**, where Δz per source sample is large: a fixed scalar
  tolerance is too narrow and produces seams. The gradient-scaled one widens
  automatically.
- **Silhouettes**, where two sheets converge in depth: a fixed tolerance wide
  enough for grazing angles bleeds the far wall through. The gradient-scaled one
  does not, because the sheets' gradients diverge there.

`κ` is a single tunable. This is [P] — it is what we would do, not what Quantel
did — but it degrades gracefully to a scalar if evidence says otherwise.

---

## 5. Consequences already forced

Two design questions that were open close as a consequence of ADR-SM-014 and E3,
without needing this unit's investigation to complete.

**5.1 Shade at splat time, not resolve time.** ADR-SM-014 puts lighting
write-side, in view space, before projection (WU-SM-01 §3.9.2). The source sample
is shaded and the shaded value is splatted. Resolve-time shading is not available
without carrying normals into the accumulator, which ADR-SM-003 and the
write-side finding jointly rule out. This closes one of the two design questions
raised in the GPU route assessment.

**5.2 There is no early-out, and the budget must be re-run.** Back faces are
visible and carry the back source (E3), so nothing can be culled. Consequences:

- Splat count stays fixed at source-raster size — reassuring, and it means the
  workload does not grow with shape complexity.
- Destination contention roughly **doubles** wherever two sheets overlap, which
  for a closed shape is most of its area.
- Every splat performs a **wider RMW** — colour, weight and now depth.

The WU-19b extrapolation (6.868 ms/frame at 576i25, 8 threads) predates all
three. It should be re-derived, not inherited. Task D6.

---

## 6. The gating prerequisite

**∂z in `Lattice::jacobian()`.** Previously flagged as a WU-26 prerequisite for
normals. It is in fact the gate on three separate features:

| Consumer | Needs |
|---|---|
| Starlight shading (WU-27) | surface **normal** |
| Front/back source select (ADR-SM-018), `Front 1 Back 2` zoning | normal **sign** |
| Sheet arbitration (WU-28, ADR-SM-020) | depth **gradient** |

That makes it the obvious first unit in Phase 7 and it is cheap. Nothing else in
this document can be built without it.

---

## 7. Investigation plan

### Strand A — documents

| # | Task | Accept |
|---|---|---|
| **A1** | **UK 2,158,671 / US app 713,028 in full.** Top priority, above S6. This is the FVP patent — the one that moves projection into the real-time path — so if run-time hidden surface is described anywhere, it is there. We have only seen it recited second-hand through S5. | A statement either way on run-time hidden surface, **or** a positive finding that it is silent. Silence is a result: it makes M2-by-inheritance less likely and licenses a [P] choice. |
| **A2** | **US 4,563,703 in full.** Base write pipeline and framestore word format. | Whether the accumulator word has spare width, and what the four-bank decomposition stores per pixel. |
| **A3** | **DVM8000/1 service or engineering manual**, any generation. | Framestore organisation. Would answer the depth-plane question outright. |
| **A4** | **S6 — US 4,899,295 / EP 0248626 B1.** Carried from WU-SM-01 §11.3 item 1. Primarily a lighting target, but a shading patent must address facing and may address occlusion. | The eight spectral models; secondarily, any statement on facing or arbitration. |
| **A5** | **Is the library's "validity mask" one bit or a soft stencil?** E15 shows S5 carrying a 2–4 bit keying/stencil component per surface element. If that is the house pattern, WU-SM-01's one-bit mask is wrong. | A bit width. Refines ADR-SM-003 rather than threatening it. |
| **A6** | **What does `Auto Transp` "from stored shape" actually read?** Three readings: (a) derived at run time from shape geometry — depth or facing driven, harmless; (b) a scalar per shape, harmless; (c) a **per-sample transparency channel in the library track**, which would qualify ADR-SM-003. | Which of the three. Pairs with A5 — if the stencil is 2–4 bits, (c) becomes likely and benign. |

### Strand B — rendered evidence

Ascending order of discrimination. Each is a falsifier for M1.

| # | Shape | What it decides |
|---|---|---|
| **B1** | Rotating cylinder, high bitrate | **Silhouette seam → soft band (M2-like); clean edge → hard decision.** Settles whether the equal-depth blend band exists in Mirage or is S5-only. The single most informative capture. |
| **B2** | `PAGE TURN LH` | Does the curl occlude the flat correctly? A partial M1 failure would show as the flat punching through the curl. |
| **B3** | `STARBURST` / `SPINY NORMAN` | Non-convex self-occlusion at multiple depths. |
| **B4** | `EVOLVING MOBIUS STRIP` | If this renders correctly, M1 is dead. |
| **B5** | Slow rotation through the angle at which a scan order would have to flip | **A pop or a one-field glitch there is a positive signature of M1.** Its absence is weak evidence; its presence is strong. |

S7 (the cylinder frame already held) satisfies none of these — it is consistent
with M1 and with M2 equally, and is too compressed at the silhouette for B1.

### Strand C — the decisive experiment

| # | Task | Why it is worth more than the rest |
|---|---|---|
| ~~**C1**~~ | ~~Does the S2 `TRANSP` state make a sphere genuinely see-through?~~ | **CLOSED — yes.** Answered from S1 p.11 (E9–E12), §3.4. Arbitration is an explicit, defeatable stage. No machine or operator testimony needed. |
| **C2** | Confirm the reading of E13 with anyone who operated a Mirage: **why** could you not use `Trail` and `Opaque` together? | An operator's account of the symptom — what actually went wrong if you tried — would confirm or refute the stale-depth explanation directly, and it is the sort of thing people remember. Now the highest-value human question, replacing C1. |

### Strand D — decisions we make regardless

| # | Decision | Note |
|---|---|---|
| **D1** | **∂z in `Lattice::jacobian()`** | §6. Gating. Code. Feeds WU-26. Do this first and unconditionally. |
| **D2** | Arbitration behind **one swappable interface** | So M1, M2 and M3 can be substituted without touching the splat. Non-negotiable while the question is open. |
| **D3** | Tolerance: scalar vs Jacobian-derived | §4.1 recommends Jacobian-derived with a single `κ`. Decide after D1. |
| **D4** | k=1 soft-z first, or the full k-buffer | k=1 with a blend band reproduces E5 exactly and is cheaper. The k-buffer is a superset. Recommend k=1 first, k-buffer as WU-28 proper. |
| **D5** | Validation shape set | Sphere, open cylinder, page turn, Möbius. Behind D2's interface so all three mechanisms can be scored against the same set. |
| **D6** | Re-derive the performance extrapolation | §5.2. Wider RMW, doubled contention, no early-out. Affects the 1080p50 conclusion and the GPU route assessment. |

---

## 8. Fixtures

Carried from WU-SM-01 §8 and extended. Fixtures 21–25 there already cover the
opaque sphere, minification-vs-occlusion, visible back faces, non-convex
self-occlusion and front/back source switching. Added here:

27. **Same-sheet accumulation at a grazing angle.** A plane rotated to near
    edge-on. Adjacent source samples land at large Δz but belong to one sheet and
    must accumulate. A fixed scalar tolerance fails this; the Jacobian-derived
    one should not. Directly exercises D3.
28. **Two sheets at a silhouette.** A cylinder at the left and right edges, where
    near and far walls converge in depth. Must not blend. The complement of 27:
    together they bracket `κ`.
29. **Scan-order invariance.** Render one frame with the (u,v) walk in each of
    the four scan-direction combinations. Under M2 the output must be
    bit-identical. Under M1 it will not be. A cheap standing regression that also
    detects accidental order dependence.
30. **Transparency sweep.** Sphere with `T` from 0 to 1. At `T = 0` the far
    hemisphere must be invisible; at `T = 0.5` the result must match what an
    unarbitrated accumulate-then-normalise produces; at `T = 1` the near
    hemisphere must vanish behind the far one. Exercises ADR-SM-020 end to end.
31. **Opaque and Trail are exclusive.** Whatever mechanism is built, selecting
    both must either be refused at the UI or produce the historical failure. Do
    not silently make the combination work — reproducing the constraint is
    evidence the mechanism is right.
32. **Möbius closure.** `EVOLVING MOBIUS STRIP` through a full evolution with no
    depth-sorting hints. No self-punch-through at any frame.

---

## 9. ADRs

| ID | Decision | Status |
|----|----------|--------|
| ADR-SM-003 | Library carries address map + validity mask only | Accepted; **unaffected** — see §2, it does not constrain the framestore |
| ADR-SM-014 | Lighting composited write-side, pre-projection | Accepted; forces §5.1 |
| ADR-SM-016 | Accumulate-then-normalise **within a sheet** | Proposed |
| ADR-SM-017 | Between-sheet arbitration mechanism | **M1 excluded** (§3.4); M1′ excluded by E13 [C strong]; **M2 favoured**, pending A1 |
| ADR-SM-018 | Two video sources, front and back, selected per sample by facing | Proposed; firm up under A2/A3 |
| **ADR-SM-019** | The three-level depth distinction of §2 is recorded explicitly, so that "no stored normals" is never again read as "no run-time depth" | **Proposed (new)** |
| **ADR-SM-020** | Arbitration by three-mode compare with a **transparency coefficient `T`** — same sheet accumulates; between sheets the nearer contributes `(1−T)` and the farther `T`; `T = 0` is `Opaque`. Sheet test from the Jacobian depth gradient | **Proposed (new)**; the coefficient is [A], the sheet test [P] |
| **ADR-SM-022** | `T` sourced per WU-SM-02 §4.0 — global scalar (`Manual`), from the shape (`Auto`), or per-pixel from an external key input (`Ext. Key`), mutually exclusive | **Proposed (new)** |
| **ADR-SM-021** | Shading at splat time, not resolve time | **Proposed (new)**, forced by ADR-SM-014 |

---

## 10. Risks and falsifiers

Stated up front so the unit can be shown to have failed rather than quietly
drifting.

| Risk | Falsifier | Response |
|---|---|---|
| The E13 inference is over-read | A1, A3 or C2 shows `Opaque`/`Trail` were exclusive for some unrelated reason (a shared register, a control-panel interlock) | M1′ returns to the table; D2's interface makes the fallback cheap |
| ADR-SM-020's sheet test is our design, not Quantel's | A1 shows ordering | Fall back behind D2's interface; keep M2 as the default rendering path |
| The Möbius argument rests on a [B] library entry | B4 unavailable, and §3.2's `sh 55` duplication turns out to be a real error | Argument weakens but does not collapse — VORTEX, STARBURST, TILE UNIVERSE are independent |
| Jacobian-derived tolerance is over-engineering | Fixtures 27 and 28 both pass with a scalar | Adopt the scalar; the Jacobian term costs nothing to remove |
| No footage, no manual, no machine | All of A1–A3, B1–B5, C1 return nothing | Declare the mechanism unrecoverable, choose M2 explicitly as [P], and record that scatter-dve is at that point emulating an inferred behaviour. Say so in the repo, do not let it pass as reconstruction |
| Budget regression | D6 shows 576i25 no longer fits with arbitration | Revisit the GPU route assessment; programmable blending handles depth arbitration natively and this is exactly the workload it was assessed for |

---

## 11. Exit criteria

This unit closes when all of:

1. **D1 delivered** — ∂z in `Lattice::jacobian()`, green-tagged.
2. **ADR-SM-017 resolved** to M1, M2 or M3, with the evidence recorded and the
   tier stated honestly — [A] if a source says so, [P] if we chose.
3. **ADR-SM-019, 020, 021 accepted or rejected.**
4. **D2 in place** — the arbitration interface exists and at least one mechanism
   is behind it.
5. **Fixtures 21–25 and 27–30** specified as executable tests, whether or not
   they all pass yet.
6. **D6 complete** — the performance picture re-derived, and the GPU route
   assessment annotated with the result rather than superseded.

Strands A, B and C may all return nothing. That is an acceptable outcome provided
criterion 2 records it as a choice.

---

## 12. Corrections carried in from WU-SM-01

| Ref | Correction |
|-----|------------|
| WU-SM-01 0.2/0.3 | "Mirage had no z-buffer" conflated the library-level finding (ADR-SM-003, correct) with the framestore-level question (open). Separated in §2 here; ADR-SM-019 proposed to keep it separated. |
| WU-SM-01 0.3, second pass | The claim that depth arbitration "would change the look" because Mirage is a machine "where the ghosting is the point". Wrong — v1 spheres are opaque. Corrected in WU-SM-01 §3.9.4.1; §4 here is the constructive replacement. |
| WU-SM-01 0.3, second pass, again | The correction itself was half right. "Ghosting is the point" was wrong as a description of the *default look*, but the accumulator's unarbitrated output **is** the blend, and `Opaque` is the added override. Both observations describe the same machine in different modes. §3.5. |
| WU-SM-02 0.1 | Framed between-sheet arbitration as a binary decision. It is a parameterised blend with an operator-controlled coefficient. §4.0, ADR-SM-020 rewritten. |
| WU-SM-01 0.3, cost reasoning | Treated a z-buffer as exotic for the period. The splat is already a read-modify-write; depth widens it rather than introducing it. §3.3. |

---

## 13. Handoff

1. **Start D1.** It is cheap, it gates everything, and it is useful under every
   outcome of Strands A–C.
2. **Chase A1 next** — UK 2,158,671 in full. It is the FVP patent and it is the
   most likely single document to contain the answer.
3. **B1 is the highest-value capture.** One clean rotating cylinder decides
   whether the blend band is real.
4. **C2 replaces C1 as the human question.** Ask anyone who operated a Mirage
   what actually went wrong if you selected `Trail` and `Opaque` together. If the
   answer describes trails occluding live picture, or live picture vanishing into
   old trail, the stale-depth reading of E13 is confirmed and ADR-SM-017 closes
   on M2.
5. Do not build the arbitration stage before D2's interface exists.
6. **WU-SM-03 — the Starlight lighting pipeline** (WU-SM-01 §7.6) can proceed in
   parallel. It depends on D1 but not on the outcome of ADR-SM-017.
7. A **cross-reference pass over WU-SM-01** is worth adding to its own §12. The
   front/back finding of §3.9.4.3 sat in three unconnected sections across three
   drafts because the handoff protocol says "diff only, do not re-derive". That
   discipline is right for throughput and wrong for synthesis; one pass reading
   the transcribed material against itself, rather than against the next source,
   would have caught it.

---

*End of WU-SM-02 draft 0.1.*
