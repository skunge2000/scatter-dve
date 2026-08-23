# WU-SM-01 — Mirage Shape, UI and Storage Model

**Project:** scatter-dve (Quantel Mirage emulation)
**Status:** Draft 0.3 — Starlight patent chain identified; lighting model recovered
**Date:** 2026-08-21
**Scope:** Consolidated data model, parameter semantics, UI model and storage schema
derived from five primary Quantel documents spanning three machine generations,
plus one identified but not yet held.

**Changes in 0.3.** US 5,103,217 (S5) ingested. Three consequences, in
descending order of importance. (a) **The lighting mathematics is recovered** —
S5 gives Quantel's starlight illumination formula in closed form, a Phong-family
model, and shows the circuit that evaluates it (§4.6). (b) **A patent
misattribution is corrected**: S5 is the *skin-store / chisel* patent, in which
starlight appears as an already-commercial building block; the actual Starlight
patent is **EP 0248626 / US 4,899,295** (Nonweiler), which S5 cites by name
(§2, §10 S5). (c) **ADR-SM-014 resolves** — lighting is applied in 3D view space
*before* projection to 2D (§3.9.2), and the address map is confirmed as a
**forward/scatter** map, not a reverse map (§3.9.4). Also new: fixtures 17–22,
ADR-SM-015 and 016, and a revised §11 ledger. Draft 0.2's content is retained
except where explicitly annotated.

**Changes in 0.2.** Starlight operator's manual (S3) and the Starlight sales
brochure (S4) ingested. The blocking question of draft 0.1 — whether the shape
library frame grew depth or normal channels at Starlight — is **answered no**
(§3.9), closing ADR-SM-003. New material: §3.9 storage impact, §4.5 the lighting
model, §5.6 the control tree and new VDU pages, §6.5 Contour and Corner Pinning,
§7.6 the lighting pipeline, five new ADRs, S3/S4 errata, and a rewritten question
ledger (§11). Nothing in §§3.1–3.8, 4.1–4.4, 5.1–5.5 or 6.1–6.4 has been
re-derived; those sections stand as written in 0.1 except where explicitly
annotated.

---

## 1. Sources and provenance

| ID | Document | Date / rev | Machine generation | Confidence in transcription |
|----|----------|-----------|--------------------|------------------------------|
| **S1** | *Quantel Mirage Provisional Operator's Manual* | Cover letter 16 Mar 1984; copy annotated "Groucho" | Standard Mirage, modular control panel, HP A700 host | High (typeset + clean typescript) |
| **S2** | *Floating Viewpoint Mirage — Operating Instructions for V1.0 Software*, doc 240/50 | DRAFT 2, undated (post-S1) | FVP Mirage (marketed as Floating Viewpoint Control); "Encore" named once, see §2 | High body text; library table partly degraded |
| **S3** | *Starlight operator's manual*, chapters 1–5 | Undated; cover sheets annotated **"OUTDATED"** in manuscript, scanned in two parts marked "1 of 2" / "2 of 2" | FVP Mirage + Starlight crate; running software **V4.02** | Medium–high. Body text clean, but this is an **incomplete draft**: §2.1.1, §2.1.2 and §5.2.5 have headings and no content, and the lighting mathematics promised in §1.2.1 is absent entirely |
| **S4** | Quantel sales brochure, *Mirage Starlight — real-time lighting and shading system for Mirage* | Undated; references Harry, Encore and the Digital Production Center, and Quantel's Newbury address → c. 1988–89 | Same generation as S3 | High (typeset print, two sides) |
| **S5** | **US 5,103,217**, *Electronic image processing*, Robin A. Cawley, Quantel Ltd | Priority GB 8728836, 10 Dec 1987; filed 29 Nov 1988; granted 7 Apr 1992 | Quantel **skin-store / chisel** machine — *not* Mirage, but reuses Mirage's starlight, floating-viewpoint and colour-processing circuits as named commercial components | High (granted text); OCR of the PDF is poor in places, see §10 |
| **S7** | Off-air frame of a Mirage CYLINDER effect (screen capture) | Undated; v1-era | Standard or FVP Mirage | Direct evidence of rendered behaviour; heavily compressed, so fine detail at silhouettes is not readable |
| **S6** | **EP 0248626 A2/B1 = US 4,899,295**, *Video signal processing*, B. R. Nonweiler, Quantel Ltd | Priority GB 8613447, 3 Jun 1986; EP filed 1 Jun 1987, published 9 Dec 1987, granted 16 Oct 1991 | **The Starlight patent proper** | **NOT YET HELD** — identified in 0.3, cited by S5 as the basis of its starlight system |

S5/S6 provenance detail: S5 §"description of the preferred embodiment" states
that floating viewpoint circuits, starlight systems and colour processing
circuits of the kinds it describes "are currently available articles of commerce
being incorporated for example in video processing systems, sold by the
Applicant/Assignee of the present patent application, **under the trade mark
Mirage**". S5 therefore describes Mirage's Starlight as prior art it consumes,
and points at S6 for the detail: "The system 76 is based on that described in our
European Patent Application No. 248626A (equivalent to U.S. patent application
Ser. No. 052,464)". The EPO record for EP 0248626 gives the abstract as a system
that reshapes a video frame into a three-dimensional object and shades it from an
imaginary light source, in which **the address data is processed to produce light
intensity factors and each pixel is adjusted in response to a light intensity
factor** — an exact description of Mirage Starlight, and independent confirmation
of §3.9.

S1 provenance detail: transmittal from **Dave Scammell, Head of Applications Group,
Quantel Limited** to **Mike Luckwell, Moving Picture Company**, enclosing the
provisional manual plus "a printed copy of all the help files delivered with the
Mirage computer". Distribution list in manuscript: Dave Throssell, Phil Graham,
Martin Hicks, Groucho, Bob, Alan, John B (×5). The letter explicitly warns the
help files are subject to change before delivery.

S1 therefore contains **two strata**: the typeset panel description (front) and
four verbatim on-line help files (back) — STARTUP/RTE-A, MTEST, SHAPE 3D moves,
and shape-generation linking. The help files are the authoritative machine-facing
grammar; the panel description is the operator-facing view of the same model.

### 1.1 Confidence tiers used below

- **[A]** Stated explicitly and unambiguously in a source.
- **[B]** Stated but internally inconsistent, or stated once without corroboration.
- **[C]** Inferred by us from the sources; not stated.
- **[P]** Proposal for scatter-dve; not evidence about the historical machine.

---

## 2. Machine generations

Four states of the machine are now in view. S1 and S2 describe the first two
production generations; S3 and S4 describe the Starlight upgrade to the second,
which is a fitted option rather than a generation in its own right.

1. **Pre-panel Mirage (NAB 1982).** Referenced in S1 §"MIRAGE: A new concept in
   flexibility and control" as the machine unveiled at NAB '82, controlled by
   "previous Quantel control boxes" (DPE 5000 pre-select, DLS 6000 stack). Not
   documented here. [A]
2. **Standard Mirage (S1, 1984).** HP A700 host running RTE-A. Modular control
   panel: Transition, Joystick, Numeric, State, Selector. Shape library is a set
   of **baked 2D warp fields**; perspective is applied offline at bake time. [A]
3. **Floating Viewpoint Mirage (S2, V1.0).** Trackball-driven. Control system
   named **Encore**. HP host no longer visible to the operator; a micro floppy
   provides effects backup and system software. Shape library becomes
   **viewpoint-independent**; projection moves into the real-time path. [A]
4. **Starlight (S3/S4, running software V4.02).** Not a new machine. A
   **field-fittable upgrade to a Floating Viewpoint Mirage** — S3 §1.2 opens "If
   Starlight is to be implemented on an existing Floating Viewpoint Mirage", and
   §1.1 states it "may be fitted to any Mirage in the field". Everything in the
   S2 model therefore carries forward unchanged unless S3 contradicts it. The kit
   is: a Starlight hardware crate; a replacement border/soft-switch card in the
   **DPU**; V4.02 running software on two floppies; copymir V3.01 (required
   because V4.02 no longer fits on one floppy); and a demonstration-effects
   floppy. [A]

   Architecturally the crate sits **beside** the existing signal path, not in it:
   it takes shape information and control commands **from the ESU** and delivers
   lighting calculations **to the DPU**, where the modified border/soft-switch
   card applies the pattern digitally to the video. Lighting is computed **per
   field**. [A]

   The axis model is generalised from the S2 REF/OBJECT pair into a full
   **control tree rooted at WORLD** (§5.6.1), explicitly borrowed from the
   structure used on Quantel's multi-channel machines.

### 2.1 Correction: which patent is the Starlight patent

Earlier scatter-dve sessions have worked from **EP 0320166 A1** on the
understanding that it is the Starlight patent. That attribution does not survive
S5.

- **S5 = US 5,103,217** (Cawley, priority GB 8728836 of 10 Dec 1987, filed 29 Nov
  1988) is about a **skin store** — a semiconductor store holding RGB + stencil +
  24-bit XYZ per surface element — and a **"chisel"** for deforming it
  interactively. It is a 3D paint/sculpt machine, not a DVE. Its priority date
  and filing window match EP 0320166 A1's, so the two are **probably the same
  family**; that identity is unverified and should be checked, but it does not
  change the conclusion. [B]
- Within S5, "starlight" is a **named subsystem incorporated from elsewhere**, and
  S5 says where: **EP 0248626 A / US Ser. No. 052,464**, granted as **US
  4,899,295** to **Nonweiler**, priority **3 June 1986**. That is S6, and it is
  the Starlight patent. [A]

Two things follow. First, the "per-pixel lighting per EP0320166A1" claim carried
in earlier sessions rested on the wrong document; see §3.9.1 for what S5 actually
supports. Second, S6 has a **1986** priority — a year and a half before S5, and
well before the Starlight product shipped — which fits Starlight being an
engineering programme that predates its productisation on Mirage.

**Action:** obtain US 4,899,295 / EP 0248626 B1. It is the highest-value
outstanding document alongside a non-outdated S3.

The name "Encore" appearing in S2 §2.4 ("stored in Encore's memory") is a single
occurrence and may be a drafting slip from a sibling product manual. **S4
strengthens the drafting-slip reading**: it lists Encore as a distinct Quantel
product that Mirage interoperates with via the optional Combiner, and names
Mirage's own control system as **Floating Viewpoint Control**. A sibling Encore
manual to paste from therefore demonstrably existed. Still **[B]**, but the
balance has shifted against Encore being the name of the FVP control system.

---

## 3. Storage model

### 3.1 The shape library (both generations)

A flat, **track-addressed** store on a dedicated library disk. Addressing is by
integer track number; there is no filesystem in the runtime path. [A]

A *shape* is not stored as geometry at runtime. It is stored as a run of
consecutive tracks, each track holding one sampled position of the shape. [A]

**S1 (standard Mirage):** `trans` takes the array written into shared EMA by the
shape-generating program, applies the concatenated 3D move, perspectives it, and
projects to 2D, writing the result to the library. Only one object-generator
program need be written per shape because the 3D→2D reduction is done offline. [A]
Both worked examples produce **33 positions** (tracks 50–82 for the cylinder,
83–115 for the flat). [A]

**S2 (FVP):** static shapes collapse to **one track each** (`Size 1`). Only shapes
that genuinely *change shape* during a sweep retain `Size 33`. S2 §2.3.1 states
the reason directly: with FVP, storing evolving shapes so the viewer can see the
object from another side is no longer necessary, so shape storage is much more
efficient. [A]

**Consequence [C]:** the library track has changed meaning between generations.
In S1 it is a finished 2D warp field for one viewpoint. In S2 it is a
viewpoint-independent 3D address map, reprojected live. This is the single most
important structural delta between the two documents, and the one most likely to
change again at Starlight.

`33` in S2 is therefore a sampling of the **shape-evolution parameter only**,
no longer a viewpoint sampling. [C]

### 3.2 FVP library map (S2 §2.3.1, verbatim structure)

Columns: `Size | Start | End | Move | Title`.

| Size | Start | End | Move | Title |
|-----:|------:|----:|------|-------|
| 3 | 0 | 2 | — | **CONTROL TRACKS — DO NOT WRITE IN THIS AREA** |
| 1 | 3 | 3 | sh 01 | FLAT |
| 1 | 4 | 4 | sh 04 | SPHERE |
| 1 | 5 | 5 | sh 02 | CYLINDER |
| 1 | 6 | 6 | sh 03 | CONE |
| 1 | 7 | 7 | sh 05 | OPEN ENDED BOX |
| 1 | 8 | 8 | sh 06 | CLOSED BOX |
| 1 | 9 | 9 | sh 07 | EXPLODE |
| 1 | 10 | 10 | sh 08 | 4-SIDED PYRAMID, open bottom |
| 1 | 11 | 11 | sh 09 | ROUND TOROID |
| 1 | 12 | 12 | sh 10 | SQUARE TOROID |
| 1 | 13 | 13 | sh 11 | 6-SIDED PRISM, open top and bottom |
| 1 | 14 | 14 | sh 12 | 6-SIDED PYRAMID, open bottom |
| 1 | 15 | 15 | sh 13 | 6-SIDED SPHERE |
| 1 | 16 | 16 | sh 14 | 6-SIDED CAN, open top, flat bottom |
| 1 | 17 | 17 | sh 15 | 6-SIDED CAN, flat top, flat bottom |
| 1 | 18 | 18 | sh 16 | 6-SIDED CAN, pointed top, flat bottom |
| 1 | 19 | 19 | sh 17 | 6-SIDED CAN, pointed top and bottom |
| 1 | 20 | 20 | sh 18 | 6-SIDED CAN, domed top, flat bottom |
| 1 | 21 | 21 | sh 19 | 6-SIDED CAN, domed top and bottom |
| 1 | 22 | 22 | sh 20 | 5-POINT STAR BADGE |
| 1 | 23 | 23 | sh 21 | 6-POINT STAR BADGE |
| 1 | 24 | 24 | sh 22 | DOME |
| 1 | 25 | 25 | sh 23 | SPINY NORMAN |
| 1 | 26 | 26 | sh 24 | STARBURST 1 |
| 1 | 27 | 27 | sh 25 | STARBURST 2 |
| 1 | 28 | 28 | sh 26 | STAR 1 |
| 1 | 29 | 29 | sh 27 | STAR 2 |
| 1 | 30 | 30 | sh 28 | APPLE |
| 33 | 31 | 63 | sh 30 | PAGE TURN LH |
| 33 | 64 | 96 | sh 31 | PAGE TURN RH |
| 33 | 97 | 129 | sh 32 | SARDINE ROLL UP (bottom L → top R) |
| 33 | 130 | 162 | sh 33 | SARDINE ROLL UP (bottom R → top L) |
| 33 | 163 | 195 | sh 34 | ROLL UP (bottom to top) |
| 33 | 196 | 228 | sh 35 | BURST OUT THROUGH CENTRE |
| 33 | 229 | 261 | sh 59 | VORTEX |
| 33 | 262 | 294 | sh 36 | WOOSH from the centre |
| 33 | 295 | 327 | sh 37 | WOOSH from top left hand corner |
| 33 | 328 | 360 | sh 38 | WOOSH from top left hand, arriving top to bottom |
| 33 | 361 | 393 | sh 39 | WOOSH side by side panel jump |
| 33 | 394 | 426 | sh 40 | blinds 1: horizontal blinds / 8 slats |
| 33 | 427 | 459 | sh 41 | blinds 2: vertical blinds / 8 slats |
| 33 | 460 | 492 | sh 42 | blinds 3: horizontal strips vertically revolve |
| 33 | 493 | 525 | sh 43 | blinds 4: vertical strips horizontally revolve |
| 33 | 526 | 558 | sh 44 | blinds 5: tiles rotating in columns 12 × 12 |
| 33 | 559 | 591 | sh 45 | blinds 6: tiles rotating in rows 12 × 12 |
| 33 | 592 | 624 | sh 46/47 | FOLDING CHOCOLATE BOX |
| 33 | 625 | 657 | sh 48/49 | FOLDING BOX 2 |
| 33 | 658 | 690 | sh 01/29 | TWISTUP |
| 33 | 691 | 723 | sh 50 | DUMMY SHAPE |
| 33 | 724 | 756 | sh 51 | FLAG main |
| 33 | 757 | 789 | sh 52 | TILE UNIVERSE true 3d |
| 33 | 790 | 822 | sh 53 | WOBBLY FLAT |
| 33 | 823 | 855 | sh 54/55 | EVOLVING MOBIUS STRIP twistup |
| 33 | 856 | 888 | sh 55 | EVOLVING MOBIUS STRIP turn inside-out |
| 33 | 889 | 921 | sh 56 | POOL SURFACE |
| 33 | 922 | 954 | sh 57 | CURTAINS horizontally opening in the middle |
| 33 | 955 | 987 | sh 58 | DUMMY EVOLVING SHAPE |
| 1 | 988 | 988 | sh 60 | COIN |
| 1 | 989–1007 | | sh 61–79 | DUMMY SHAPE ×19 |
| 759 | 1008 | 1766 | — | empty |
| 1767 | 1767 | 3533 | — | RESERVED FOR DISC2 (use only if fitted) |

Notes on this table:

- The `Move` column is a **shape-id, or an ordered pair of shape-ids**. Entries
  like `sh 46/47`, `sh 48/49`, `sh 54/55`, `sh 01/29` are 33-track runs bound to a
  *from→to* pair, i.e. genuine inter-shape transitions living in the same address
  space as self-transitions. [A]
- `sh 55` appears both as the second half of `54/55` and alone. Likely a draft
  slip; flag for S3 check. [B]
- `sh 59` (VORTEX) is interleaved out of numeric order between `sh 35` and `sh 36`,
  showing the library grew by append and the `sh nn` id is **not** the track order. [C]
- Nineteen DUMMY SHAPE single-track slots plus one DUMMY EVOLVING SHAPE reserve
  ids for site-authored shapes without disturbing the map. [C]
- Track 0–2 control tracks are almost certainly the shape table / library
  directory itself. Not stated. [C]

### 3.3 The transition table

A square table indexed **[this shape → next shape]**, each cell holding
`(start_track, finish_track)`. [A]

**S1:** 75 × 75, for 75 shape buttons. Populated via Mtest:

```
TRANSITION <button num> [TO <button num>] START <library track> [TO FINISH <library track>]
```
abbreviable to three characters, e.g. `TRA 3 TO 3 STA 50 FIN 82`. [A]

Diagonal cells are self-to-self transitions (e.g. a globe rotating back to its
original position). Any cell not explicitly programmed defaults to an **ooze**:
a linear transition between the two endpoint positions, synthesised by Mirage at
replay, not precomputed. [A]

Two rules from S1 that must be preserved:

1. **Coverage.** Declaring a shape button obliges you to fill its entire row *and*
   column; unfilled cells leave the machine unable to perform the transition. [A]
2. **Entry order is load-bearing.** The self-to-self transition must be entered
   *first* for a given shape, because entering it back-fills that shape's row and
   column with default oozes — which would otherwise overwrite hand-authored
   specials. S1's worked example is explicit: `TRA 4 TO 4` then `TRA 5 TO 5`, and
   only then the specials `TRA 4 TO 5` and `TRA 5 TO 4`, which must each be
   entered separately because the reverse is not implied. [A]

**S2:** the same record, promoted to a first-class UI object (VDU menu 7, "Shape
Set Up"). Four boxes along the top: **From Shape / To Shape / Start Track /
Finish Track**. Changing From/To updates Start/Finish to show the current cell
contents, or `none` if unpopulated. `New Shape` softkey commits the edit
permanently. [A] Table dimensions not restated in S2. [B]

`TRA a TO b STA n FIN m` and the four-box editor are the same record in two
notations. [C]

### 3.4 Effects and sweeps

| | S1 | S2 |
|---|---|---|
| Sweeps per effect | 50 | **42** |
| Effects addressable | 75-shape space; panels give 30 effects + 30 shapes | **250** on-line |
| Default sweep time | not stated | **15 frames** |
| Keyframe term | sweep stores a destination | **END POINT** (button legended SWEEP on early machines) |
| Reserved | effect −1 and effect 0 must be correctly defined, as supplied in `MASTER.ALL` | effect 0 shown when no effect selected |

The S2 terminology change is worth adopting wholesale: a **sweep** is the
*interval*, an **end point** is the *stored parameter set* at its boundary. S1
used "sweep" for both. [A]

Each end point stores: size, centre/position, shape, and — if the Border-Matte
chord was held at entry time (S1) — border and matte colours, border width and
crop value. State changes are separate records anchored at a fraction through a
sweep. [A]

### 3.5 State / Input / Key insertions, and the mask (S2 only)

The most important new storage concept in S2. State, Input and Key insertions are
each a **(value set, mask) pair**. The mask selects which functions Mirage
actually changes; unmasked functions pass through untouched, so the operator need
not fully define the menu each time. [A]

- **States:** BORDER, MATTE, CROP, CONT, TRANSP, TRAIL, ARM, LINEAR. [A]
- **Input:** FRONT A, BACK A, FRONT B, BACK B, FREEZE, FRONT FROZEN, BACK FROZEN. [A]
- **Key:** EXTERN KEY, EXTERN INV, EXTERN GAIN %, EXTERN OFFSET %. [A]

Up to **100 state menus may be inserted into a sweep**, at any point. [A]

`FIND STATE` / `FIND INPUT` / `FIND KEY` step the effect forward to the next
insertion of that class and display it — a *typed cursor* over the sweep. This
replaces S1's `set` + `enter state` chord, which found the next state change,
obeyed it and stopped. [A]

Insertions do not persist until the whole effect is re-recorded
(`INSERT DELETE EFFECT`); S2 warns about this at the end of every menu procedure. [A]

**Design implication [C]:** the historical machine stored sparse partial-state
deltas with an explicit applied-fields bitmask, not full state snapshots per
keyframe. Adopt this rather than snapshot-diffing.

### 3.6 State vocabulary drift, S1 → S2

| S1 State module | S2 States menu | Note |
|---|---|---|
| Border | BORDER | |
| Matte (turns on matte *and* crop) | MATTE, CROP | split into two |
| Manual Transp / Auto Transp | TRANSP | collapsed |
| Opaque | (collapsed into TRANSP) | see below |
| Trail | TRAIL | |
| Invert Key | (moved to Key menu: EXTERN INV) | |
| Curved | — | **gone** |
| Linear | LINEAR | |
| Cont | CONT | |
| Live | — | not applicable; no Joystick module |
| — | ARM | moved in from S1 Transition module |
| Source select A/B/Frozen ×2 | (moved to Input menu) | |

**S1 p.11, transcribed in WU-SM-02 0.2, changes the reading of this table.** The
S1 State module distinguishes `Manual Transp` ("controls the transparency of
front/back overlaps"), `Auto Transp` ("controls transparency from stored shape"),
and `Opaque` ("makes front/back overlap opaque. Overrides Auto and Manual. Not in
trail"). S2 collapses all three into one continuous `TRANSP`. `Opaque` is
therefore most likely not a *removed* facility but the **zero end of the
continuous control**, exactly as `Manual` and `Auto` collapse into it. [C]

The same page corroborates the §10 S1 errata entry on the Joystick pot table: the
four assignable sets are named here as **Ext Key, Manual (transparency), Border
and Matte**, confirming the row labelled "Size & position" is mislabelled.

`Opaque`'s declared incompatibility with `Trail` is load-bearing evidence for the
hidden-surface mechanism; see WU-SM-02 §3.4.

The loss of Curved is notable: S1 offered curved-in-space position moves as a
mode. In FVP the REF/OBJECT frame pair makes curved paths constructible directly,
so the mode may have been made redundant rather than removed. [C]

### 3.7 Engineering values

**S1:** `ENG <num> [<new value>]` from Mtest; querying by omitting the value.
Documented numbers: **ENG 48** front/back switch delay (typically 993, range
0–1023, larger reduces delay); **ENG 28** output key delay (typically 12, range
0–255, larger increases delay); **ENG 22** output key mode (0 = binary fixed
slice, 4 = binary slice from ENG 21, 1 = analogue gain from ENG 20); **ENG 20**
analogue key gain (0–16383, non-linear, typically 6000); **ENG 21** binary key
slice level (0–16383, non-linear, typically 64). Input key delay is a *hardware*
pair of rotary switches on VIU card 16, nearest pin 1 — not software-settable. [A]

**S2:** VDU ENG VALUES menu. Named adjustments: INPUT TYPE (RGB/CODED), BLACK
LEVEL, GAIN, HF GAIN, OUT KEY TYPE (HARD/SOFT), HUE (0–360°), H PHASE, S PHASE,
OUTKEY SLICE (0–100%), MATTE EDGE (NONE/WIDE/SLICE/FIXED), IN KEY DELAY, IP
PICTURE PHASE, FB DELAY, OUTKEY GAIN (0–100%, softkey only), OUTKEY EDGE
(NONE/WIDE/SLICE/FIXED), DPU ESU PHASE ("DO NOT TOUCH"), OP PIC PHASE. Requires
the local/remote switch on the VIU controller card set to REMOTE. [A]

MATTE/KEY edge semantics: NONE = key extends to blanking (picture surrounded by
white matte); WIDE = slightly wider than normal; SLICE = slice level adjustable
0–100%; FIXED = slice fixed at nominal. [A]

**250 engineering files**, of which **file 0 is a reference baseline**: loaded
automatically at power-on, always displayed as all-zero, and *"if file 0 is
altered the values of all other files change accordingly"* — i.e. files 1–249 are
**deltas against file 0**. File 0 is to be set once at installation. S2 advises
limiting real use to ~10 files to avoid filling disk memory. [A]

This is the direct successor to S1's `.ENG` split (engineering data saved
separately so different edit suites can be recombined with one effects file) and
is a materially better design. [C]

### 3.8 Serialisation grammar (S1)

The strongest precedent in either document. `SAVE` emits **the command language
itself** as a replayable text file:

```
SAVE [EFFECT] / [ENGINEERING] / [TRANSITION] / [ALL] "<filename>"
```

- `.EFF` — effects only
- `.ENG` — engineering only
- `.TRA` — transition table only (allows the same library shapes to be mapped
  under different buttons)
- `.ALL` — entire runtime data state

Reload via `OBEY "<filename>"`. `NEW` initialises the transition table to null
transitions and clears all effects — mandatory first command of a cold-start save
file. `COMMENT` is a null command permitting annotation, but comments are **not
preserved** when the file is re-saved. [A]

Panel action, Mtest command, and serialised state are three views of one grammar.
Mtest can be exited (`EXIT`) leaving Mirage running, and re-entered later; shapes
may be edited and compiled while Mirage is on air, the only exclusion being
writing shapes onto the library. [A]

**Recommendation [P]:** adopt this property. Make the scatter-dve project file a
replayable command log, not an opaque binary, and make the UI, the scripting
interface and the file format the same grammar.


### 3.9 Starlight's impact on storage (S3) — ADR-SM-003 resolved

**Finding: nothing in the shape library changes.** Three independent statements
in S3 §1.2 converge on this:

1. Starlight "may be fitted to any Mirage in the field", and the parts list is a
   crate, one replacement card and three floppies. There is no library rebuild,
   no library-disk change, and no re-bake of shapes. [A]
2. The crate "connects to the **ESU** for the source of its shape information and
   control commands, and to the **DPU** for the destination of its lighting
   calculations." Shape information is *read off the existing path*; Starlight is
   a consumer of it. [A]
3. The demonstration-effects floppy "requires that the default (as shipped)
   **shape table 0** and default (as shipped) **library shapes** are in use."
   Both are pre-Starlight artefacts, used unmodified. [A]

**ADR-SM-003 therefore resolves in favour of the minimal frame.** The library
track remains the FVP viewpoint-independent address map plus validity mask.
Per-sample normals and depth are **not** stored; surface orientation is derived
at run time inside the Starlight crate from the shape information the ESU already
supplies. [A for the premises; C for the conclusion, but the conclusion is hard
to avoid — a bolt-on crate cannot change a disk format it does not own.]

The §7.2 provision for optional per-sample depth and normal planes can be dropped
from day-one scope. Keep the container extensible; do not build them.

#### 3.9.1 Corroboration: the shading is computed on a coarse grid

Filtering and grid shift (S3 §4.5.1, §5.2.2–3) are expressed entirely in terms of
a **coarse grid**:

| Filtering | Behaviour |
|---:|---|
| `0` | full interpolation of shading across coarse grids |
| `+1` | flat shading per coarse grid — a "posterisation/mosaic" effect |
| `+2` | flat shading across 2 × 2 areas of coarse grid (quads) |
| `+3` | flat shading across 3 × 3 areas |
| `−1` | **default**; smoothing filter across the grids, limiting the allowable rate of change of light intensity |
| `−2` | as `−1`, more filtered |

Grid shift: `Zero` (default) gives each coarse grid its own calculated pattern;
`Shift 1` applies the pattern calculated for a coarse grid to the **previous**
coarse grid horizontally; `Shift 2` to the previous but one. Its documented use
is when filtering is set between `+1` and `+3` and "the wrong shading level is
creeping around a corner". [A]

Two readings follow. First, shading is **evaluated per coarse-grid cell and
interpolated to pixels**, not evaluated per pixel from stored normals. Second,
grid shift exists because the value attributed to a cell is systematically off by
one cell at a discontinuity — the signature of a finite-difference estimate of
surface orientation taken across an interval and attributed to the wrong end of
it. [C, strong]

The coarse grid is almost certainly the same coarse grid S1's shape diagnostics
already refer to — §6.3, "all x and y in the coarse grid are 0.0". Its dimensions
are not stated in any of S1–S3. [C]

**Confirmed by S5 (draft 0.3).** The coarse-grid inference above is no longer an
inference. S5 §"description of the preferred embodiment", describing FIG. 3,
states that the retiming buffer 31 holds coordinates in "shifting" batches, each
batch containing the spatial coordinates of **three image elements adjacent to
the image point P** for which the computation is being done; those three elements
"define a small facet of the skin of the object, including the point P", and
circuit 32 uses them to compute **the three components of the unit vector NP
normal to the facet at P**. [A]

So the normal is a **per-facet quantity derived from a three-sample
neighbourhood** — a cross product over a local triangle, computed in the
pipeline, from a shifting window. That is precisely the finite-difference
estimate §3.9.1 inferred from the grid-shift control, and it explains grid shift
directly: the facet's normal is attributed to one point P of a three-sample
neighbourhood, so at a discontinuity the shading lands one cell away from where
it belongs.

**Note how strong this corroboration is for ADR-SM-003.** In S5's machine, XYZ
*is* stored explicitly, 24 bits per component, for every surface element — and
Quantel *still* computes the normal on the fly rather than storing it. If normals
are not stored in a machine that has per-element 3D coordinates to spare, they
are certainly not stored in Mirage, which does not.

**Correction to a prior scatter-dve working assumption.** Starlight has been
treated in earlier sessions as *per-pixel* lighting on the strength of
EP0320166A1. Two separate problems with that, both now visible. The attribution
was wrong (§2.1). And the substance is a conflation: the intensity factor is
**applied** per pixel — the EP 0248626 abstract says each pixel is adjusted by an
intensity factor, and S5's control circuit 38 multiplies RGB by I — but it is
**evaluated** per facet from a three-sample neighbourhood and interpolated, with
the filtering ladder controlling how much interpolation happens. Both statements
are true of the same system. §11.2 item 3 closes.

#### 3.9.2 Where the lighting is applied

The replacement card "receives the lighting data from the Starlight crate and
applies the required lighting pattern digitally to the digitised input video
which passes through this card." [A]

So lighting is a **separate per-field modulation raster**, multiplied (or, under
spectral-add, added) onto the video path — never baked into a shape frame. This
is the storage-relevant point and it is unambiguous.

The phrase "digitised **input** video" was flagged **[B]** in draft 0.2, against
§4.5.4's xtube/ytube being defined in **screen** components. **S5 settles it in
favour of the literal reading.**

In S5 FIG. 1 the signal order is unambiguous:

```
skin store 1 → [chisel 12 / adder 56] → floating viewpoint 9 (XYZ view)
             → STARLIGHT 76  (RGB × I)
             → 3D→2D converter 77
             → masking 78
             → picture store 15 (raster)
             → display 17
```

The starlight circuit sits **before** the 3D→2D converter and before the raster
store. Lighting is applied to the RGB of each surface element **in 3D view space,
in element order, prior to projection**. [A]

That is write-side modulation, exactly as the Starlight manual says. **ADR-SM-014
resolves in favour of write-side.** The apparent tension with xtube/ytube
dissolves once you notice that S5's starlight circuit is fed *post-viewpoint*
coordinates ("new X Y Z (view)") and takes a line-of-sight vector LP as an input
— it is working in view space, so "screen components" of ray direction are
available to it without the picture having been rasterised. §11.2 item 4 closes.

Caveat on transfer: S5's machine and Mirage are different products. But they are
the same architecture family — S5 says its starlight and floating-viewpoint
circuits *are* the Mirage ones — and the Mirage manual's own wording agrees. [A/C]

#### 3.9.3 Shape table 0

S3 §1.2 refers to "the default (as shipped) **shape table 0**". S1 and S2
describe a single transition/shape table; the ordinal implies a **set of numbered
shape tables** by the Starlight era, selectable as a unit. This is consistent with
S1's `.TRA` files already being separable ("allows the same library shapes to be
mapped under different buttons", §3.8), now promoted to a numbered first-class
object. Nothing further is said. [B]

#### 3.9.4 The address map runs forwards — a correction to §3.1 and §7.2

S5's background section recites Quantel's own prior art in terms precise enough
to fix the direction of the mapping, which draft 0.1 got backwards.

From **UK 2,119,594 / US 4,709,393** as recited: picture point signals
constituting the input picture are received **in raster order**; the addresses at
which they are **stored** in the frame store are selected so as to rearrange them
relative to their input raster position; the store is then **read in sequence**.
Where a computed write address falls between storage locations, "the picture
point signal is distributed proportionally to the adjacent addresses". A
selection of addresses defining a particular skin is called an **address map**. [A]

From **UK 2,158,671** (floating viewpoint) as recited: "the address map comprises
three dimensional addresses, and the addresses are projected on a notional
viewing surface, before being used to identify the two dimensional addresses in
the frame storage means for the input picture point signals". [A]

Two corrections follow.

1. **Mirage is a forward/scatter DVE, not a reverse-mapped one.** For each input
   sample it computes a destination address and writes there, splatting
   proportionally across the four surrounding pixels. It does not, for each output
   pixel, fetch a source address. Draft 0.1 §7.2's "per-output-pixel
   source-address map in object space" has the direction inverted; read it as a
   **per-input-sample 3D address, projected live to a 2D destination address**.
   The project is, aptly, named for the right thing.
2. §3.1's `[C]` conclusion — that the FVP library track is a viewpoint-independent
   3D address map reprojected live — is **promoted to [A]**, stated in as many
   words in UK 2,158,671 as recited by S5.

S5's own picture store 15 works identically: if the projected X′Y′ of an element
does not coincide with a storage location but lies within the area defined by
four pixel addresses, the RGBS components are distributed proportionally to those
four addresses. [A] The splat weights are the anti-aliasing; get them wrong and
every shape in the library will alias differently from the machine.

S5 also carries a **hidden-surface stage**, and it reopens a question none of
S1–S4 addresses.

Masking circuit 78 compares the Z of each arriving element against the Z already
stored for the same X′Y′ and takes one of three actions: new Z substantially
smaller, replace the stored RGBS; new Z substantially larger, discard the
arrival; the two Z values equal or substantially so, **blend or average** the
stored and arriving RGBS "according to a predetermined function". [A] A 1-deep
soft Z with an explicit equal-depth blend band, not a hard depth test. Picture
store 15 gains "an additional plane for storing the corresponding unconverted
component" to hold the Z. [A]

#### 3.9.4.1 Mirage resolves occlusion — the mechanism is undocumented

**Empirical constraint, from the machine rather than the paperwork: v1 Mirage
sphere effects are opaque.** You do not see the far hemisphere through the near
one. Whatever Mirage does, it arbitrates between surface sheets. [A — observed
behaviour of the product]

That constraint does real work, because it rules out the obvious reading of
ADR-001 and it rules out the cheapest alternative.

**It is not "accumulate everything and normalise".** Accumulate-then-normalise is
a **resampling** rule: fractional destination addresses splat proportionally
across four pixels, and under minification many source pixels landing on one
destination pixel are area-averaged. That is what UK 2,119,594 describes and it
is correct. It is *not* a rule for compositing one sheet against another — two
hemispheres each arriving with weight ≈ 1 would normalise to a 50/50 blend, i.e.
a glass sphere. So accumulation must be scoped to **contributions from the same
sheet**, with a separate mechanism arbitrating between sheets. [C]

**It is not back-face culling either.** Starlight's `Front 1 Back 2` zoning
(§4.5.3) presupposes *visible* back-facing surfaces — the interior of an
open-ended cylinder, the reverse side of a page turn. Both are rendered and both
are lit, under different models. So back faces are drawn, and something decides
which sheet wins where they overlap. [A/C]

**In S1 the problem does not arise at run time.** `trans` applies the 3D move,
perspectives, projects to 2D and writes the library track (§3.1). The bake can
therefore depth-order its samples, or drop occluded ones outright, and the
runtime inherits a finished 2D warp field with occlusion already resolved for
that viewpoint. [C, strong]

**In S2 it does.** FVP moves projection into the real-time path, so hidden
surface moves with it. Draft 0.1's §3.1 called the S1 → S2 change "the single
most important structural delta"; this is a corollary it did not draw out. The
change does not merely make the track viewpoint-independent — it forces a
hidden-surface mechanism into hardware that previously did not need one. [C]

**Candidate mechanisms**, none documented in any held source, in rough order of
period plausibility:

1. **Traversal-order painter's algorithm.** The address map is walked over the
   (u,v) lattice in a known order and later writes win. Correct if the walk is
   back-to-front, which for the convex library shapes is a matter of flipping the
   u and/or v scan direction according to the sign of the viewing direction — a
   two-bit decision per field. Cheap enough for 1985 and requires no extra store.
2. **A depth plane and compare**, as S5. More capable, more expensive, and
   demonstrably within Quantel's repertoire by 1987.
3. Some combination — ordering for the library shapes, depth for the awkward
   ones.

**The discriminating test is a non-convex self-occluding shape.** A painter's
ordering is correct for SPHERE, CYLINDER, CONE, the cans and the toroids; it
breaks on SPINY NORMAN, the STARBURSTs and the PAGE TURNs, where the surface
folds over itself and no single scan order is back-to-front everywhere. If period
footage of those shapes shows clean occlusion, ordering alone is insufficient and
option 2 or 3 is in play.

#### 3.9.4.2 Consequences for the model

- **ADR-001 needs scoping, not replacing.** Accumulate-then-normalise governs
  contributions *within* a sheet. Between sheets, something arbitrates. Draft
  0.3's first pass treated the accumulation rule as global and concluded, wrongly,
  that any depth arbitration would change the look.
- **This makes WU-28 look like emulation rather than extension.** Separating
  "accumulate within a sheet" from "arbitrate between sheets" is exactly what a
  k-buffer does. The open question is not whether scatter-dve needs the
  separation but which historical mechanism it should reproduce.
- **The equal-depth blend band is worth keeping in view** whichever route is
  taken: it is what makes a crease or a fold-back blend rather than z-fight, and
  a strict depth-sorted resolve will not reproduce it.
- **Facing is still not a depth question.** `Front 1 Back 2` comes from the sign
  of the surface normal (§4.5.3, §4.6.2), available from the three-sample facet
  independently of any arbitration mechanism.

Whether S5's circuit 78 is inherited from Mirage or added for the skin store
remains genuinely open. The "additional plane" wording is suggestive of an
addition but proves little, since Mirage might arbitrate by ordering rather than
by depth. Logged as §11.3 item 17.

#### 3.9.4.3 Direct evidence from a CYLINDER effect (S7)

A single off-air frame settles two things the paperwork left ambiguous.

The frame shows a cylinder, open-topped, viewed slightly from above. The near
wall's **exterior** carries a face. Through the open top, the far wall's
**interior** is visible as a band of blue with regular white vertical bars.

**1. Sheet arbitration is confirmed, and it is not soft.** There is no trace of
the far wall through the near wall. The near wall's top edge cuts the visible
interior band cleanly. [A]

This is consistent with a traversal-order painter's algorithm and does not
discriminate against a depth compare. A cylinder is a single sheet wrapped round;
walking `u` from the far side to the near side writes the near wall last and it
wins, producing exactly this. Convex, single sheet, one scan-direction flip.
§3.9.4.1's discriminating cases — PAGE TURN, SPINY NORMAN — are untouched by this
frame.

**A diagnostic worth noting.** The left and right silhouettes are where near and
far walls sit at nearly equal depth. An equal-depth blend band of the S5 kind
would show there as a blended seam down each edge. S7 is too compressed at the
silhouette to call it; a clean capture of a rotating cylinder would settle
whether the arbitration is a hard decision or a soft one, and therefore whether
S5's circuit 78 is inherited or added.

**2. The back of the surface carries different video from the front.** The
interior is not the face reversed. It is different content. [A]

The document already held every piece needed to explain this and had not
connected them:

- §3.5, S2's Input menu: `FRONT A`, `BACK A`, `FRONT B`, `BACK B`, `FREEZE`,
  `FRONT FROZEN`, `BACK FROZEN`. [A]
- §5.5: Mirage emits **FRONT/BACK CONTROL** to the external input matrix and
  front/back switcher, which supplies encoded and RGB feeds. [A]
- §3.7: **ENG 48**, *front/back switch delay*, typically 993, range 0–1023,
  larger reduces delay — a fine timing trim on a switch in the video path. [A]

**Reading: FRONT and BACK are the two sides of the surface, not a transition A/B
pair.** Facing selects the video source per sample; A and B are the two
selectable feeds on each side; the FREEZE variants let each side be frozen
independently. ENG 48 exists because that switch has to be timed into the video
path. [C, strong — four independent threads plus rendered evidence]

Where the switch physically lives is not stated. An external matrix cannot
switch at sample rate, so the likely arrangement is that Mirage tells the
switcher which sources to route as the front and back feeds, digitises both, and
selects between them internally per sample. The FREEZE / FRONT FROZEN / BACK
FROZEN triple implies two independently freezable input paths, which fits. [C]

**Consequence for Starlight.** `Front 1 Back 2` zoning (§4.5.3) is therefore *not*
a new concept in 1988. The base machine already distinguished facing and already
acted on it, by switching source. Starlight extends an existing facing signal
from choosing the picture to choosing the lighting model. That makes the facing
test a long-standing part of the pipeline rather than something Starlight
introduced, and reinforces §3.9.4.2's point that facing is independent of
whatever resolves occlusion.

**Consequence for the model.** §7.4's `EndPoint` carries one input and §7.6 shades
one sheet. Both need a per-sample facing bit selecting between two sources.
Opened as ADR-SM-018.

On the blue interior itself: most plausibly the BACK input carrying a grid test
pattern, which is what a demonstration would feed. The alternative reading is the
S2 `MATTE` state, but matte is a flat colour and the white bars are structure, so
probably not. Left [B].

#### 3.9.5 Storage medium

S4 gives effects and data storage as **on-line Winchester disk memory with floppy
backup**. The S2 micro floppy is backup and software-distribution media only.
Program files may now exceed one floppy — V4.02 ships on two, which is precisely
why copymir V3.01 is part of the kit. [A]
---

## 4. Coordinate and parameter model

### 4.1 Breaking changes, S1 → S2

| Quantity | S1 (SHAPE / Mtest) | S2 (FVP) |
|---|---|---|
| Screen coords | screen heights; y from **−0.5 top to +0.5 bottom**; x ±0.5·k, k = horizontal points / vertical lines | **8 × 6 unit grid**; position referred to picture centre, 0 = centre; X = 4, Y = 3 places centre at **top right** |
| Y sign | increases **downward** | increases **upward** |
| Size | fraction of full-size picture (0.5 = half) | fraction; `.5 SET` = half size |
| Rotation | **degrees**, explicit start and end | **revolutions**, destination-only |
| Rotation sign | not stated | **positive = anticlockwise** |
| View distance | screen heights; **bake-time** SHAPE parameter | **live**; default **56 units = 7 screen widths**; 1 screen height = 6 units |
| Size vs distance | perspective size decrease = (z move distance)/(view distance) | rate of size change ∝ **1/d²** |
| Expansion limit | hard authoring rule: **max 2:1** | **"infinite expansion"** |
| z limit | must not pass below ≈ **−1.5 screen heights** or `trans` aborts | not stated; eyeball may pass *through* the picture, which reappears reversed |
| Colour | RGB, 0.0–1.0 per channel | **LUM / SAT / HUE**, each 0–100%, all default 0 |
| Crop | sizes in the same scale as size and centre | **0–100%**, 0 = no crop, 100 = invisible |
| Border | top/bottom/left/right widths, 1.0 = entire picture width or height | widths ≤ **4% of picture size** sit outside the picture; beyond that they intrude |
| Numeric precision | not stated | 7 digits either side of the point, 6.5 digits internal (as printed — self-inconsistent) |

**Two of these will bite the port.**

1. **Y sign inverts** between the two documents. Every S1 SHAPE example must be
   sign-flipped before being replayed against an FVP-model engine.
2. **View distance units are internally contradictory in S2** — see §10.

### 4.2 Rotation semantics (S2) — recommended for adoption

Spin is stored as a **destination**, not an angle:

> One complete 360° rotation equals one unit. The whole number before the point
> says how many complete revolutions will be made; the digits after the point
> define the **final angle regardless of the angle it started from**. [A]

So `3.5` = three and a half revolutions anticlockwise, ending exactly
upside-down. This is not an angle — it is a **(winding count, terminal phase)**
pair. It makes sweep interpolation well-defined without reference to the start
value, which is precisely what an end-point-list model requires. [C]

### 4.3 Notch quantisation (S2)

Constrains trackball action to steps. Grid-consistent throughout:

- SIZE: ⅛ steps (0.125)
- SPIN: ¹⁄₁₆ of a revolution (0.0625 = 22.5°)
- POS: ⅛ width, ⅙ height (= **1 unit** on the 8 × 6 grid)

[A]

### 4.4 The 3D transform vocabulary (S1)

Exactly six primitives: **{Turn, Move} × {X, Y, Z}**, each with a start and end
value, each qualified as operating about the **Screen** axis or the **Object**
axis. Concatenated in order into a "3D move". Turns in degrees, moves in screen
heights. Axes right-handed, cartesian, orthogonal. [A]

- The **screen axis** remains fixed. The **object axis** is coincident with the
  screen axis before any transform and thereafter remains fixed relative to the
  object. Hence for the *first* transform, screen and object are equivalent. [A]
- Rotation about an arbitrary axis is done by **manual conjugation**: move the
  object so the desired axis lies on a screen axis, turn, then apply the inverse
  move. S1 gives the general algorithm explicitly. [A]
- S1 §6(d) warns that incrementing two angles simultaneously does **not** compose
  as expected: two simultaneous 0–360 screen-axis rotations produce a rotation
  about an axis at 45° to the horizontal, in which two corners of a square stay
  fixed, not the expected single-corner-fixed rotation. [A]

Worked constraint from S1 §6(c), the rolling cone: for a cone to roll without
slipping, `sin(θ) = 1/n` for integer n, where θ is the cone half-angle; n = 2
gives θ = 30°, so with height 0.55 the radius must be 0.3175 and the cone must
turn twice on its own axis per circuit of the plane. Useful as a regression
fixture. [A]

**S2 promotes the reference frame to a live object.** REF and OBJECT are
independently lit toggles; with both lit the axes move together, with one
deselected they separate. On-screen 3D cross-wires show both, with **solid =
axis pointing out of the screen, dotted = into the screen**. The conjugation of
S1 §5 is now implicit in the REF/OBJECT pair. [A]

**Implication [P]:** model this as two live TRS frames, not as a baked matrix
product, because the operator can grab either frame at any time during editing.


### 4.5 The Starlight lighting model (S3)

#### 4.5.1 Reflectance belongs to the light, not the surface

The single most unusual thing in the whole document. In real life the reflective
properties belong to the surface; with Starlight they "are all attributed to the
light sources". There is no surface-material menu — S3 §4.4.2 says so in as many
words: you will not find a switch marked 'surface model' with options for wood,
glass, metal. You find, instead, a spectral/diffuse ratio and a reflection-law
model **per light**. [A]

The consequence is called out as a feature: two lights aimed at the same point on
the same surface can reflect as if that surface were matte for one light and
gloss for the other — "something that cannot be done in real life". [A]

For emulation this inverts the usual ownership of BRDF parameters: material lives
on the `(light, zone)` pair, not on the geometry. Reproduce it rather than
normalising it into a conventional material model; effects authored on the real
machine will depend on it.

#### 4.5.2 Diffuse and "spectral"

S3 spells **specular** as **"spectral"** throughout — in prose, in section
headings and in menu box legends. It is consistent, not a one-off typo. Keep the
historical spelling in any UI claiming fidelity; use `specular` internally.

Per light, per zone, a **ratio** in `0.0 … 1.0`: `0.0` = wholly diffuse, `1.0` =
wholly specular, any mix between. [A]

Diffuse is always **multiplicative**: the diffuse level reflected from a piece of
the picture is calculated and the hardware darkens or lightens the image by a
factor proportional to it, mirroring how much light a real surface absorbs. [A]

Specular is switchable globally, **MULTIPLY / ADD** (VDU 6). Multiply applies the
same mechanism to the specular term. **ADD** is justified by the case multiply
cannot express, and the manual gives it twice: *a totally black object reflecting
a white specular highlight*. In real life specular light is not discoloured by
absorption because it is wholly reflected, so ADD is the physically motivated
option and MULTIPLY the stylised one — the reverse of what the naming suggests. [A]

Eight specular models per zone, selected in VDU 7:

```
Model 1 · Model 2 · Model 3 · Model 4 · Ramp · Posterise · 2 ring · 4 ring
```

Models 1–4 are unnamed reflection laws; the remaining four are plainly
stylisations. **The mathematics is promised in Chapter 2 and is not present in
this draft.** [A for the list; semantics unresolved — see §11.2]

#### 4.5.3 Zones

Per light, the video surface may be split into two zones, with specular modelling
specified independently for each. The `Zones` box (VDU 7) selects how membership
is defined:

| Setting | Meaning |
|---|---|
| Zone 1 only | zone-1 modelling over the whole video surface |
| Zone 2 only | zone-2 modelling over the whole video surface |
| Front 1 Back 2 | zone 1 on all front video surfaces, zone 2 on all back video surfaces |
| Back 1 Front 2 | the converse |
| Varizone | membership supplied by the varizone generator |
| Inv. Varizone | the inverse of the varizone generator |

[A]

Rationale given: an object with two distinct surface finishes. The worked example
is a cylindrical wooden pencil — gloss painted body, becoming matte along the
sharpened cone. [A]

`Front 1 Back 2` implies Starlight knows **facing** at coarse-grid resolution —
further evidence that orientation is computed rather than stored (§3.9). [C]

The **varizone generator** (MORE 6) is a rectangle shrunk inwards from each of
the four picture edges independently — `top / bottom / left / right`, each
`0 … +100%` — described as working "in a similar fashion to the control of the
border and crop generators". [A] Edge profile (hard or soft) unspecified.

**Lights 3–6 are zone-locked.** In VDU 7, lights 3 and 5 have a Zone 1 control
only; lights 4 and 6 have a Zone 2 control only; in each case the manual adds
"which in turn is the entire video surface". For the four parallel lights the
zone is therefore degenerate as a *spatial* concept — it selects which of the two
model registers the light reads. [A]

It is not, however, cosmetic: **global gain has separate values for zone 1 and
zone 2** (§4.5.5), so a light hard-bound to zone 2 is scaled by global gain 2.
Lights 3/5 and lights 4/6 thus form two independently-trimmable groups. [C]

#### 4.5.4 Styles and shapes

| Style | Available on | Position matters | Spin matters |
|---|---|---|---|
| **Point** | lights 1, 2 only | yes | **no** — light emanates equally in all directions |
| **Beam** | lights 1, 2 only | yes | yes — directional along the light's z axis |
| **Parallel** | all six | lights 3–6: **no**, position has no meaning; lights 1–2: yes | lights 3–6: **yes**, direction = light z; lights 1–2: **no** |

[A]

Behaviours to reproduce exactly:

- A **beam emits symmetrically out of both +z and −z**. Explicitly stated, and
  easy to get wrong. [A]
- A **parallel light on light 1 or 2** takes its direction from the *light origin
  → object origin* vector, so it re-aims itself as either moves; spinning it does
  nothing. On lights 3–6 the direction is simply the light's own z axis. [A]
- Beam intensity falls off as a **cosinusoid** in the angle of the ray from the z
  axis — maximum along z, minimum at the boundary of the cone — "in the standard
  shipping form", implying the law is a fitted option. [A]
- A **point light spun about some other axis in the tree** does change the
  lighting, but only because its origin moved, not because of any orientation
  change. [A]
- As a point source recedes it **asymptotes to a parallel light**; as it
  approaches the surface the reflection narrows to a pool of light near the
  source. [A]

**Shape** modifier (VDU 7), orthogonal to style: `Full · Xtube · Ytube`. Xtube
holds the **x screen component** of the light-ray direction constant while y
behaves normally, turning a point source into a long horizontal strip light.
Ytube is the transpose, giving a vertical strip. Note these are defined in
**screen** components, not object components. [A]

#### 4.5.5 Intensity, gain and colour — the stacking order

Four stages, all keyframable:

| Stage | Menu | Range | Default |
|---|---|---|---|
| Per-light intensity | trackerball menu, light axis | −400% … +400% | light 1 non-zero; **lights 2–6 at 0.0** on power-up/clear |
| Light-source gain and colour (all six combined) | MORE 6 | LUM / SAT / HUE | luma **100%**, sat **0%** |
| Ambient light gain and colour | MORE 6 | LUM / SAT / HUE | luma **30%**, sat **0%** — "a gentle overall fill-in light which by experience has proven desirable" |
| Global gain, applied to light sources **and** ambient, separately per zone | MORE 6 | −200% … +200% | 100% |

[A throughout]

**Negative intensity is a first-class facility.** A light with negative intensity
"cancels out other light"; the patterning and modelling behave as if it were a
real source, so two identically configured lights differing only in the sign of
intensity cancel exactly. Documented use: knocking back a part of the scene that
has washed out from light overflow off properly lit areas. It is programmable
into effects "in just the same way as object z size is". [A]

Colour is in the **same LUM/SAT/HUE 0–100% space as S2** — §11 Q6 answered for
lighting. Light colour interacts multiplicatively with picture colour: with a
pure red light, no blue or green content in the picture reflects any diffuse
light at all. [A]

#### 4.5.6 Parameter packing on the light axis

A light axis reuses the object axis's **size triple** to carry lighting
parameters:

| Object axis field | Light axis meaning | Range |
|---|---|---|
| x size | spectral/diffuse **ratio 1** (zone 1) | 0.0 … 1.0 |
| y size | spectral/diffuse **ratio 2** (zone 2) | 0.0 … 1.0 |
| z size | **intensity** | −400% … +400% |

Stated across three places: §3.1.1 ("the part of the display normally used for
control of axis size is now used for light source intensity and the mixture of
spectral and diffuse light"), §4.3.1 (intensity is "what would be the z size were
this axis that of an object"), §4.4.2 (the ratios are "what would be object x and
y size"). [A for the field set; **C** for the specific x↔ratio1 / y↔ratio2
assignment, which is the join of the three statements rather than any single one.]

**The consequence matters more than the packing.** Because lighting rides on
ordinary axis records, it needs **no new keyframe machinery**. S3 §5.1 is
explicit: values put into this menu "are storable upon insertion of an endpoint,
and will ramp through the effect". Lights are animated by exactly the mechanism
that animates the object.

MORE 6 parameters are likewise "remembered upon insertion of a **way-point** or
end-point (a change is signified by the asterisk in the flags field of the vdu)".
Note **way-point** as a term used alongside end-point; S2 used end-point only,
and draft 0.1 §3.4 adopted the S2 pair *sweep / end point*. Whether a way-point
is a distinct object or just a non-terminal end point is not stated. [B]

### 4.6 The starlight illumination model (S5)

This section replaces the "list of model names" that §4.5.2 could offer in draft
0.2. S5 gives the formula the starlight circuit evaluates and the circuit that
evaluates it.

#### 4.6.1 The formula

For each image point P, the light directed to the viewer is

```
I  =  Ia·Ka  +  ( Ip / (r + k) ) · ( Kd·cos A  +  Ks·cosⁿ B )
```

| Term | S5's definition |
|---|---|
| `Ia` | ambient light intensity |
| `Ka` | reflection coefficient for the skin |
| `Ip` | point source intensity of the notional spot light |
| `r` | distance of the source S from the skin |
| `Kd` | diffuse reflectivity |
| `Ks` | specular reflectivity |
| `k` | "an empirical constant" |
| `n` | "a small integer, say 2" |
| `A` | angle between **NP** (unit normal at P) and **SP** (vector from source S to P) |
| `B` | angle between **LP** (unit line-of-sight vector from P to the viewing surface) and **RP** (the direction of ray SP reflected from the facet) |

[A]

This is **Phong**, in its original form: Lambertian diffuse plus a
cosine-power specular lobe measured between the *reflected ray* and the *view
vector*, over an ambient floor. Not Blinn — there is no half-vector. Two details
worth not smoothing over:

- **Distance attenuation is `1/(r + k)`, not inverse-square.** Linear falloff
  with an additive constant, the constant existing to keep the term finite as the
  source approaches the surface. This is what S3 §4.2 is describing in prose when
  it says the reflection narrows to a pool of light near the source and
  approximates a parallel light as the source recedes. [A/C]
- **The line of sight LP "may be assumed to be fixed."** [A] The specular term
  uses a constant view vector — an orthographic viewing assumption, taken for
  cost. Reproduce it: a correct per-pixel view vector will put highlights in
  visibly different places from the machine.

S5 notes these quantities "can be assumed to remain constant for a particular
object and illumination", i.e. Kd, Ks, Ka and n are per-setup constants rather
than per-sample data — consistent with Starlight attributing them to the light
rather than the surface (§4.5.1).

#### 4.6.2 The circuit (S5 FIG. 3)

```
XYZ (post-viewpoint) ─▶ 31 retiming buffer (shifting 3-sample batches)
                        └▶ 32 calc NP  (unit normal of facet 70/71/72 at P)
S (operator) ──────────▶ 33 calc SP, and outputs Ip and r
      NP, SP, LP ──────▶ 34 cos calc  →  cos A, cos B
   cos A, cos B, Kd, Ks▶ 35 calc      →  Kd·cos A + Ks·cosⁿ B
   that, Ip, r, Ia, Ka ▶ 36 I calc    →  I
                  RGB ─▶ 38 control   →  RGB × I
```

Input circuit 37 supplies Ia and Ka under operator control; Kd and Ks are
"predetermined" inputs to 35. Buffers are needed in the RGB path so that circuit
36's output lands on the right pixel. [A]

Note the split: **34 computes the geometry** (two cosines), **35 computes the
reflection law**, **36 applies intensity, distance and ambient**. Starlight's
per-light "model" box selects behaviour at stage 35, and its gains act at stage
36. That decomposition is worth keeping in the emulator, because it is the one
the historical controls are organised around.

#### 4.6.3 Mapping the Starlight controls onto the formula

The strongest available reading, all **[C]** unless marked, and to be checked
against S6:

| S3 control | Formula term |
|---|---|
| Light intensity, −400…+400% | `Ip`, signed |
| Spectral/diffuse ratio, 0.0…1.0, per zone | the `(Kd, Ks)` split — plausibly `Ks = ratio`, `Kd = 1 − ratio` |
| Zone 1 / Zone 2 spectral model | the function applied at stage 35 to `cos B` |
| Ambient light, LUM/SAT/HUE, default luma 30% | `Ia`, coloured; `Ka` is not separately exposed and is presumably folded in |
| Light source gain, LUM/SAT/HUE, default luma 100% | a common scale on `Ip` across all six lights, plus light colour |
| Global gain, −200…+200%, per zone | a scale on `I` itself, after ambient — S3 says it applies to light sources **and** ambient, which puts it outside the bracket [A] |
| Spectral MULTIPLY vs ADD | whether the `Ks·cosⁿB` term stays inside the `RGB × I` product (multiply) or is added after it (add) |
| Point / Beam / Parallel | how `SP` and `r` are derived; parallel makes `SP` constant and drops the `1/(r+k)` term |
| Filtering, grid shift | post-processing of the per-facet `I` lattice before interpolation (§3.9.1) |

**The eight spectral models are almost certainly stage-35 look-up tables on
`cos B`.** Two supports. S5's chisel circuit (FIG. 5) uses exactly this idiom —
"a function generator 55 which **may include a look-up table**" driven by `cos C`
— so a cosine-indexed LUT is the house pattern for a shaping function. And the
four named models are unnameable as physics but trivial as LUTs:

| S3 model | Likely stage-35 function of `cos B` |
|---|---|
| Model 1 … Model 4 | `cosⁿ B` for four values of `n` — S5's "small integer, say 2" says the exponent is a parameter, and four fixed exponents is the obvious way to expose it |
| Ramp | linear in `cos B`, i.e. `n = 1` with no power |
| Posterise | a quantised LUT — stepped highlight |
| 2 ring | a LUT with two lobes → two concentric highlight rings |
| 4 ring | four lobes → four rings |

All **[C]**. The ring models in particular are non-physical by construction and
have no analogue in the S5 formula; they are stylisations that only make sense as
table contents. Confirming or refuting this is the main thing S6 is wanted for.

#### 4.6.4 What S5 still does not answer

- **One light, white.** S5's starlight circuit handles a single notional
  spot-light "assumed to be white in this example". Mirage has six, coloured, and
  S3 gives no summation rule. Straight summation of `I` before the ambient floor
  is the obvious reading, but it is a reading.
- **Sign and clamping.** Nothing is said about negative `cos A` or `cos B`.
  Mirage explicitly allows negative intensity, and beams explicitly emit out of
  both `+z` and `−z` (§4.5.4), which hints at magnitude rather than clamped
  behaviour somewhere in the chain — but that is a guess.
- **Backlight** has no counterpart in S5. The one thin lead: if back-facing
  surfaces are normally excluded by a `cos A < 0` test, "backlight on" may simply
  disable that test. Marked as a hypothesis only, not to be implemented on.
- **Zones** have no counterpart in S5 either; they are a Mirage product feature
  layered on top.
---

## 5. Control / UI model

### 5.1 S1 — modular panel

Physical: modules 4¼″ wide × 5¼″ (3U) high; a standard frame mounts four into a
17″ × 5¼″ panel. Modules joined nose-to-tail by 40-way flat cable to a local
control box processor, which talks to Mirage over a **4-wire RS422** link,
permitting remote operation and panel assignment. Mirage can be driven by as few
as two modules; standard complement is four. [A]

Five module types:

| Module | Role |
|---|---|
| **Transition** | Lever arm with thermometer display, Take button with time display, four buttons: Clear (return to flat at full size), Reverse (play current sequence backwards), Set Time (a.k.a. Learn Time), Arm (enable external Take, e.g. from an editor). Lever arm and Take can take over from each other mid-transition. |
| **Joystick** | Two two-axis joysticks, three pots, five buttons: Lock (fix aspect ratio), 4:3 (momentary), Limit (constrain size to nominal…zero), Nom Size, Centre. Pots are assignable by the State module to one of four sets: **border, matte, manual transparency, external soft key**. |
| **Numeric** | Keypad + display, five function buttons (four used): Effect, Sweep, Shape, %. |
| **State** | Three groups: (a) source select and freeze; (b) mode control and analogue-parameter assignment; (c) effect entry. |
| **Selector** | 15 buttons, laid out to leave room for user labelling between rows; designated as shapes or effects. Four selectors typically configured as 30 effects + 30 shapes. |

Two interaction idioms worth carrying forward:

1. **The two-key chord.** Hold a State-module verb, press a target on the Numeric
   or Selector module. `Empty Effect` + `10`. `Enter Sweep` + `Delete Sweep` +
   `effect` = replace this sweep. Border-Matte held with Enter Sweep adds the
   colour/width/crop parameters to the stored sweep. [A]
2. **Numeric-prefix-as-virtual-button.** With `10` in the numeric display, the
   `effect` key *is* selector button 10 — S1: *"even if that selector panel does
   not, in fact, exist"*. Selector panels are therefore a **cache over a larger
   address space**, not the model itself: 75 shapes addressable, 30 buttons fitted. [A]

S2 preserves the idiom exactly: `7 VDU` jumps directly to Shape Set Up (menu 7). [A]

**Recommendation [P]:** implement numeric-prefix-plus-context as a universal
accelerator (a command palette that accepts `<n> <verb>`), rather than inventing
a new shortcut scheme.

### 5.2 S2 — trackball and VDU

Panel set: **NUMERIC**, **3D CONTROL**, **TRANSITION**, **DISK** (micro floppy:
effects memory + system software backup), plus a **data display (VDU)** which
mimics panel selection and exposes less-used functions. Joystick, State and
Selector modules are gone. [A]

Global mode: **PLAY / EDIT** toggle. EDIT is entered by the PLAY/EDIT button or
implicitly by pressing any button on the right of the 3D control module
(POS, SPIN, SIZE, COL, PICT, CROP/MATTE, BORD, MORE, VDU, REF, OBJECT).
Confirmed by the Transition module LED reading EDIT. [A]

Two columns of four buttons form a **mode matrix**:

| Major mode (left column) | Parameter class (right column) | Effect |
|---|---|---|
| PICT | POS / SPIN / SIZE / COL | position, size, spin, transparency |
| CROP/MATTE | POS / COL | picture crop; matte colour |
| BORD | POS / SIZE / COL | border offset, width, colour |
| MORE (1st press) → VIEW | SIZE | viewing distance |
| MORE (2nd press) → FOCUS | COL | defocus |

Modifiers: **SHIFT** (access third axis — trackball natively controls two),
**USE H** / **USE V** (soft keys locking the trackball to a single axis),
**NOTCH** (quantise), **CLEAR** (return to flat, both REF and OBJECT lit).

Worked chords from S2 §2.2, useful as a UI conformance suite:

```
PICT SIZE                → BALL X,Y (locked)
PICT POS                 → BALL X,Y
PICT SPIN                → BALL X,Y
PICT SPIN SHIFT          → BALL Z            (Z-axis rotation)
PICT POS SHIFT           → BALL Z            (eyeball toward/away; 1/d²)
PICT SIZE SHIFT USE V    → BALL Y only       (aspect ratio)
PICT SIZE SHIFT USE H    → BALL X only
PICT SPIN USE V          → BALL X rotate only
REF POS                  → separates the two axis sets
OBJECT SPIN SHIFT        → spin image about its own Z
REF OBJECT SPIN          → XY reference-axis spin
CROP/MATTE COL           → BALL Lum; SHIFT → BALL Hue, Sat
CROP/MATTE POS           → BALL top/left; SHIFT → bottom/right
BORD SIZE                → BALL horiz/vert width
BORD POS                 → BALL top/left; SHIFT → bottom/right   (offset border)
BORD COL                 → BALL Lum; SHIFT → Hue, Sat
MORE SIZE                → BALL viewing distance
MORE COL                 → BALL focus
```

Handwritten annotation on p.4: SIZE on the Z axis uses `USE H` + `SHIFT`. [B]

All trackball operations have a numeric equivalent: select the function, type the
value, press **SET**. `PIC SIZE .5 SET` = half-size picture. [A]

### 5.3 S2 VDU menu tree

Top Menu (menu 1), eight softkey boxes:

```
STATES   INPUT   KEY   MORE        SHAPE   SET UP   DISK
```

Softkey row is `INSERT DELETE SET NEXT | SET H SET V SET H SET V`. [A]

Known menu numbers: **1** Top, **3** States, **4** Input, **5** Keys, **7** Shape
Set Up, **12** Set Clock. Top Menu 2 reached via MORE, containing at least SET
CLOCK and ENG VALUES. [A] Menus 2, 6, 8–11 unaccounted for — likely SET UP, DISK,
and the Top Menu 2 leaves. [C]

Value entry inside menus is uniform: move the cursor with the trackball onto a
field, then either `T-BALL SET` (toggles trackball between cursor and value
control, field highlights) or type digits and press `NUMBER SET`. Pressing
`NUMBER SET` with the Numeric display **blank** restores the field to its
**default value** — a small but worth-copying affordance. [A]

Real-time clock: battery-backed, set via menu 12, fields Day (1–31), Month
(1–12), Year, Day-of-week (1–7), Hour, Minute, Second. [A]

### 5.4 Editing grammar (S2)

A uniform three-token grammar, `[XXX] <verb> <noun>`:

```
verbs:  INSERT | DELETE | SET
nouns:  EFFECT | END POINT | STATE | INPUT | KEY
```

with `INSERT`+`DELETE` together meaning **replace**.

```
DELETE EFFECT                    clear current effect from panel
INSERT END POINT                 memorise current parameters as a keyframe
XXX INSERT DELETE EFFECT         record effect XXX (LED shows "USED" if occupied)
XXX SET ENDPOINT                 set this sweep's time to XXX frames
XXX SET EFFECT                   set the whole effect's time to XXX frames
INSERT DELETE EFFECT             commit timing/state edits back to memory
XXX DELETE EFFECT                erase effect XXX
XXX DELETE INSERT EFFECT         replace old effect with edited version
```

The pencil annotation on S2 p.14 inserting DELETE into `XXX INSERT EFFECT` is an
operator correcting the draft against §2.4.2. Recording over an existing effect
requires INSERT DELETE EFFECT. [B — manuscript, but internally corroborated]

### 5.5 Video interfacing (S2 §1.2, Figure 1)

Input matrix and front/back switcher (Quantel-suppliable) offering 16 encoded
loop-through inputs and 8 RGB terminating inputs, fed by REF (sync). Mirage
receives RGB input, encoded input, a 25-way ENCODED/RGB CONTROL bus, and
INPUT REF (B/burst); it emits FRONT/BACK CONTROL back to the switcher, plus RGB
output, encoded video output, and KEY OUT. KEY IN may be fed from a bus. Mirage
output may be H-phased ±½ line so timing it into the mixer is not a problem. The
key out signal fits the area of the output picture. [A]

The front/back switch signal driving an external matrix is inherited directly
from S1, which notes the same design gives maximum flexibility by allowing a
mix/effects bank or external keyer to generate Mirage's single video input. [A]


### 5.6 S3 — the control tree and the new VDU pages

#### 5.6.1 The control tree

Starlight generalises the S2 REF/OBJECT chain into a full parent/child **axis
tree**, described as "more closely resembl[ing] the structure adopted for the
control of multi-channel machines". [A]

Default topology at power-up and after clear:

```
                              WORLD
             ┌──────────────────┼──────────────────────┐
           REF 3            GANTRY 8               GANTRY 9
             │            ┌─────┴─────┐        ┌────┬───┴────┬────┐
           REF 2      * LS REF 1  * LS REF 2  *L3  *L4      *L5  *L6
             │            │           │
          * REF 1      LIGHT 1     LIGHT 2
             │
         * OBJECT
```

- **WORLD** is new: an immovable axis centred on the centre of 3D space, with no
  controls of its own, from which everything hangs. "All axes must be suspended
  from some other axis." [A]
- **Gantries 8 and 9** are bare reference axes with special names — a scaffold to
  which any number of lights may be attached, so the whole combination can be
  positioned and spun as a rigid group. The manual's worked example: a vertical
  cylinder rotating slowly about y, with two beam lights aimed at its centre from
  outside, the light pair orbiting y in the **opposite** sense to the cylinder —
  trivial with a gantry, fiddly without. [A]
- Lights 1 and 2 have their own **light-source reference axes**, needed because
  they may be point or beam lights and so require more degrees of freedom. Lights
  3–6 have none, being parallel-only. Hence there is no axis `3.1`. [A]
- `*` marks a reconnectable attachment point; "there is no limit to how many axes
  may be hung from another". [A]

Axis identifiers are `a.b`:

| Axis | id | Axis | id |
|---|---|---|---|
| reference 3 | `0.3` | light source 3 | `3.0` |
| reference 2 | `0.2` | light source 4 | `4.0` |
| reference 1 | `0.1` | light source 5 | `5.0` |
| object | `0.0` | light source 6 | `6.0` |
| light source reference 1 | `1.1` | gantry 8 | `8.0` |
| light source 1 | `1.0` | gantry 9 | `9.0` |
| light source reference 2 | `2.1` | | |
| light source 2 | `2.0` | | |

The rule: `a` names the principal axis, `b = 0` is that principal axis itself,
and `b` increases by 1 for each related reference axis above it. Gantries have no
reference axis, so `b` is always 0. [A]

Navigation: dial the identifier on the numerics module, then press **REF** or
**OBJECT** depending on which axis slot on the VDU it should occupy; the other
slot auto-updates to the parent or child as appropriate. S3 states the
reinterpretation plainly — **"the REF and OBJECT buttons are now really UPPER
AXIS and LOWER AXIS buttons"**. [A] This is a real semantic change to the S2
panel and should be carried into any faithful surface mapping.

The S2 four soft-button walk (up / down / previous / next, borrowed from
multi-channel operation) still works in parallel; the identifier mechanism exists
because after reconnection the tree no longer matches the operator's mental
model. [A]

Typing only `a` selects a default `b`: `0`→object, `1`→light 1, `2`→light 2,
`3`–`6`→lights 3–6, `8`/`9`→gantries. So the fast path to any light is
*number → OBJECT*, and to a gantry *8 or 9 → REF*. [A]

Reconnection is edited in **VDU 8**. Fifteen possible attachment points: World,
Gantry 8, Gantry 9, Reference 1, Reference 2, Reference 3, the Object, Light 1
reference, Light 1, Light 2 reference, Light 2, Light 3, Light 4, Light 5, Light
6. Constraints: an element may not be connected to itself, nor make a connection
that would create a separate looped branch. [A]

**Design implication [P]:** ADR-SM-006 (REF and OBJECT as two live TRS frames) is
superseded. Implement a general scene graph with a fixed WORLD root, seed it with
the historical default topology, and keep the `a.b` identifier as the stable
external name for an axis. The reconnection legality rules are exactly
"no self-edge, no cycle" — a standard DAG-with-single-parent check.

#### 5.6.2 New VDU pages

| Page | Contents |
|---|---|
| **VDU 2** | `LIGHTING` on/off square — the global Starlight enable |
| **VDU 6 — Lighting** | Light 1…6 ON/OFF; Filtering `−2…+3`; Grid Shift `Zero / Shift 1 / Shift 2`; Spectral `MULTIPLY / ADD`; Backlight `ON / OFF` |
| **VDU 7 — Lighting control** | Per light: Zone 1 model, Zone 2 model, Zones, Styles, Shape. Lights 1 and 2 have all five; lights 3 and 5 have Zone 1 only; lights 4 and 6 have Zone 2 only |
| **VDU 8 — Tree** | Eight reconnection boxes over fifteen attachment points |
| **MORE 6 — Lights** | Light Source (lum/hue/sat), Ambient Light (lum/hue/sat), Variable Zone (top/bottom/left/right, `0…+100%`), Global Gain (zones 1 and 2, `−200…+200%`) |

Menu numbers 6, 7 and 8 were unaccounted for in draft 0.1 §5.3's reconstruction
of the S2 tree; Starlight fills them. MORE 6 confirms that the MORE prefix
reaches a second bank of numbered pages rather than a single overflow page. [A]

`LIGHTING` off makes the Mirage behave as a Mirage without Starlight: effects
containing lighting still play, Starlight controls still respond, but no lighting
appears. Note that **Starlight continues to send data to the main Mirage even
when LIGHTING is off** — the switch gates application, not calculation. [A]

Per-light ON/OFF is presented primarily as an **edit affordance**: switching a
light off eliminates its contribution without altering any of its parameters, so
the operator can audit what each light in a multi-light rig is actually
contributing. The manual's analogy is switching off the power to an electric
light rather than changing the lamp. But the switch state **is** captured if an
end point is inserted while it is off, and S3 warns about this explicitly — same
semantics as border on/off in S1. [A]
---

## 6. Shape generation and authoring toolchain (S1)

Only S1 documents this. S2 says nothing about how shapes are made, only how they
are addressed — a gap S3 may or may not fill.

### 6.1 The plugin surface

A shape is a **Pascal/1000 subprogram**, not a whole program, linked against the
Quantel-supplied main program `obgen`. Files per shape:

| Extension | Purpose |
|---|---|
| `.PAS` | user-written Pascal source |
| `.CMD` | transfer file that compiles and links |
| `.LOD` | linker command file |
| `.REL` | compiler output, purged after link |
| `.QUE` | question file — the parameter schema |
| `.ANS` | answer file — persisted parameter values |
| `.RUN` | runnable shape program |

Supplied programs: `/shapes/obgen.rel` (main program, compiled form), `/shapes/trans.run`
(reads the EMA array, performs 3D moves, perspectives, writes to the Mirage
library), `/programs/shape.run` (overall controller/interpreter, reads softkeys;
no other shape software may be run except through it), `/shapes/shape.cmd`
(transfer file that runs Shape — must be run as a transfer file), `/shape/object.txt`
(registry of available shapes). [A]

Typical `.CMD`:
```
pascal,spher.pas,,-
link,spher.lod
pu,spher.rel
```
Typical `.LOD`:
```
scom
shema, object
re, obgen. rel
re, spher. rel
en, spher. run
```
`scom` links into the system common; `shema, object` sets up the shared EMA area
named `object` used by the object generator. New shapes are built by copying an
existing pair and changing every reference. [A]

Running Shape: `tr shape` on single-terminal systems with 0.5 MB — **warning: must
not be used on multi-terminal systems, high risk of destroying any open file**.
On multi-terminal systems with ≥1.0 MB use `shape`. [A]

### 6.2 The `.QUE` grammar — recommended as the shape manifest format

```
QUESTION
PROMPT "<prompt string>"
<answer type>            ; real | integer (16-bit) | string (≤80 chars)
MIN <min value>          ; not for string variables
MAX <max value>
[DEFAULT <default value>]  ; must come after MIN and MAX
[REMARK "<remark string>"] ; units, shown after the answer gap
[HELP "<help string>"]     ; shown on the help key
END
...
EXIT                     ; after last question
```

Example:
```
QUESTION
PROMPT "Radius of the cylinder?"
REAL MIN 0.0 MAX 1.0 DEFAULT 0.25
REMARK "screen heights."
HELP "Gives the radius of the cylinder, which is constant through a move."
END
```

Guidance embedded in S1: when choosing max and min, remember the maximum expansion
for a Mirage picture is 2:1, and radii less than 0.0 are not sensible. A shape
with no variables needs no `.QUE` — but the `.ANS` file must still be created by
hand (edit, create, exit without content). [A]

`object.txt` reuses the **same parser** with an empty prompt, the DEFAULT string
carrying the runnable program name:

```
QUESTION
PROMPT " "
STRING
DEFAULT "<shape prog name>"
REMARK "<type of shape>"
HELP "<help text>"
END
```

[A]

Parser conventions, shared by Mtest and the question-file interpreter: commands
case-insensitive, abbreviable to three characters or any intermediate length; any
number of spaces as separator; string parameters fully quoted; limited-range
integers and reals fully checked; `&` at end of line continues onto the next.
Strings up to 80 characters. [A]

**Recommendation [P]:** adopt this grammar essentially verbatim as the scatter-dve
shape manifest. It is a declarative parameter schema *and* a UI-generation
language in eight lines, and the registry reusing the same parser is a genuinely
elegant touch.

### 6.3 Shape diagnostics (S1)

Because Shape owns the terminal, error messages are suppressed and diagnostics
must be written to another terminal, a printer, or a text file. Documented failure
signatures:

| Symptom | Cause |
|---|---|
| Rectangle (Mirage screen boundary) drawn, shape not | shape lies off the graphics screen |
| Rectangle drawn, single white dot at centre | all x and y in the coarse grid are 0.0 |
| "move in progress" flashes straight to "move is finished", nothing visible | Trans or the shape program has aborted |

If Trans crashed, check the move section transforms: bringing the object too far
out of screen (z too negative) causes the abort. Fix by keeping z above about
−1.5 screen heights — the consolation being that the expansion would have exceeded
2:1 anyway. If the shape program aborted, the two main causes are array overflow
and answer-file text handling; the latter is usually the program reading past
end-of-file, because `obgen` executes the input-file reset **once** per run, so a
shape program that makes multiple positions must reset the input file itself. [A]

### 6.4 Host and boot (S1)

HP A700 running RTE-A. Boot: `VCP> %bdc` (see errata), then RTE loads,
`/SYSTEM/-WELCOME9.CMD` executes, CI prompt `CI>`. Mirage started with `startup`,
which prompts for an effect file, default `[master.all]`, and takes 70–80 seconds
of dots. `startup` schedules `/mirage/mtest.run`, which reads the effect file,
sets up data areas for `/mirage/mirage.run`, and runs it. Success message:
"starting up Mirage", control panel display changes from `off` to a number.
`exit` leaves Mirage running; `stop` then `exit` stops it. Shape development runs
with or without Mirage running. [A]

Failure mode worth noting: if the PARALLEL card is disconnected from the ESU's
COMPUTER INTERFACE SOCKET, or the ESU is off or not connected to the VIU, the
PARALLEL card receives continuous interrupts, MTEST returns `.` and the Mirage
driver stops trying to drive the machine. Recovery is `stop`, reconnect, `run`. [A]

Editor softkeys, if lost: `li,/system/edkey.lst` at the CI prompt. [A]


### 6.5 Authoring at the Starlight era — Contour, and the rest of the package (S4)

S3 says nothing about how shapes are made. S4 does.

**Contour** is a separately-priced option of the Starlight era: a **mouse- and
tablet-driven shape generator**. The user specifies a new shape by drawing a
profile and a few cross-sections; Contour transforms those drawings into "a full
3D description that Mirage understands", ready for use. Explicitly *no computer
skills required*, and it runs **off-line while Mirage continues with effects
generation**. Its stated target market is packaging and container shapes for
television advertising. [A]

This is the supported replacement for the S1 Pascal / `obgen` / `trans` pipeline
as the *operator-facing* authoring route. It does **not** follow that the Pascal
route was withdrawn: Contour is an option, `shape.run` is part of the base
system, and nothing in S3 or S4 says otherwise. Whether `.QUE` / `.ANS` /
`object.txt` survive is still unanswered. [B]

For scatter-dve the two are complementary rather than competing. Keep the
`.QUE`-derived declarative manifest (§6.2) as the machine-facing format, and
treat a profile-plus-cross-sections editor as one *producer* of manifests and
geometry — which is roughly what Contour was. [P]

**Other S4 content bearing on the model:**

- **Corner Pinning ships as part of the Starlight package** (not as an option).
  Each individual corner of the picture is pinned into position and Mirage
  derives true 3D position, size, spin and perspective. **"Four levels of
  motion"** are available, **"as well as true throughpoint curving"**. [A]
- Throughpoint curving is worth setting against §3.6, where S1's `Curved` state
  was noted as having vanished in S2 and speculatively explained away as made
  redundant by the REF/OBJECT pair. Curved paths reappear here as a named
  capability. Whether it is the same mechanism, and whether corner-pin data forms
  part of the end-point record, is unknown. [B]
- Internal processing is **4:4:4:4**. Standard signal I/O: coded video, RGB, key.
  Optional signal I/O: Quantel digital, 601 digital — via the **Combiner** option,
  which makes Mirage fully digitally compatible with Harry, Encore and the
  Digital Production Center. [A]
- Power supplies: 115 V 60 Hz ±10% (NTSC) / 240 V 50 Hz ±10% (PAL). [A]
- The brochure names the system feature as **Floating Viewpoint Control** — the
  S2 machine, confirmed by its marketing name. [A]
---

## 7. Proposed model for scatter-dve **[P]**

Everything in this section is proposal, not evidence.

### 7.1 Layering

```
┌─ Command grammar (text) ────────────────────────────┐  ← project file == command log
│  NEW / SAVE / OBEY / COMMENT                        │
│  TRA <a> TO <b> STA <n> FIN <m>                     │
│  INSERT|DELETE|SET  EFFECT|ENDPOINT|STATE|INPUT|KEY │
├─ Runtime data model ────────────────────────────────┤
│  ShapeTable · EffectStore · EngineeringStore        │
├─ Shape library (content-addressed) ─────────────────┤
│  ShapeRun[] → Frame[] (1 or N)                      │
└─ Shape authoring (offline) ─────────────────────────┘
   manifest (.que-derived) + generator → ShapeRun
```

### 7.2 Shape library

```
ShapeRun {
  id:            u16          # historical "sh nn"
  from_shape:    u16          # == id for self/static
  to_shape:      u16          # differs for sh 46/47-style pairs
  frames:        Frame[]      # len 1 (static) or N (evolving); historical N = 33
  title:         string
}
```

`Frame` content is **settled as of draft 0.2** — see §3.9. It is a
per-output-pixel source-address map in object space plus a validity mask, and
nothing else. Starlight does not add channels: it derives surface orientation
from this map at run time. Keep the container extensible (a plane list rather
than two fixed planes) so a later finding can be accommodated without a format
break, but do **not** build depth or normal planes now.

**Direction, corrected in 0.3 (§3.9.4).** The map runs *forwards*: for each input
sample it holds a 3D address, projected live to a 2D destination, and the sample
is splatted proportionally across the four surrounding output pixels. Earlier
drafts described it as a per-output-pixel source-address map, which is the wrong
way round and would produce a different — and differently aliased — image.

Recommended container: one directory per shape, a TOML/JSON manifest plus binary
frame planes; **not** an emulated flat track image. Keep a
`track_number → (shape_id, frame_index)` mapping table as a separate artefact so
historical `.TRA` files and the FVP library map can be replayed verbatim against
the modern store.

### 7.3 Shape table

Sparse map, not a dense 75×75 array:

```
ShapeTable: Map<(from: u16, to: u16), TrackRange>
```

with lookup falling back to a synthesised **ooze** (linear interpolation between
endpoint fields) when the cell is absent. Preserve S1's coverage warning as a
lint, not a hard error, and preserve the ordering hazard by making self-transition
insertion an explicit "fill row/column with oozes" operation the user can see and
undo — rather than a silent side effect.

### 7.4 Effect model

```
Effect {
  id:        u16                # 1..250
  endpoints: EndPoint[]         # <= 42
}

EndPoint {
  duration_frames: u16          # default 15
  shape:           u16
  size:            Vec3
  position:        Vec3         # 8 x 6 unit grid, y up
  spin:            Vec3         # revolutions; int = winding, frac = terminal phase
  view_distance:   f32          # units; default 56
  focus:           f32
  border:          BorderParams?    # present only if authored (S1 Border-Matte chord)
  matte:           MatteParams?
  crop:            CropParams?
  ref_frame:       Transform     # REF      -- superseded, see `axes` below
  object_frame:    Transform     # OBJECT   -- superseded, see `axes` below
  insertions:      Insertion[]

  # --- Starlight (S3) ---------------------------------------------------
  axes:            AxisTree      # WORLD-rooted; supersedes ref_frame/object_frame
  lights:          Light[6]
  lighting_on:     bool          # VDU 2 LIGHTING
  light_gain:      ColourGain    # lum/hue/sat, default 100 / 0 / 0
  ambient:         ColourGain    # lum/hue/sat, default  30 / 0 / 0
  global_gain:     [f32; 2]      # per zone, -200..+200 %, default 100
  varizone:        Insets        # top/bottom/left/right, 0..100 %
  spectral_mode:   Multiply | Add
  filtering:       i8            # -2..+3, default -1
  grid_shift:      u8            # 0..2, default 0
}

Light {
  on:          bool              # VDU 6; default true for all six
  intensity:   f32               # -400..+400 %   (historical: axis z size)
  ratio1:      f32               # 0.0..1.0       (historical: axis x size)
  ratio2:      f32               # 0.0..1.0       (historical: axis y size)
  style:       Point | Beam | Parallel      # Point/Beam legal only on lights 1,2
  shape:       Full | XTube | YTube
  zone1_model: SpecularModel     # Model1..4 | Ramp | Posterise | Ring2 | Ring4
  zone2_model: SpecularModel
  zones:       Zone1Only | Zone2Only | Front1Back2 | Back1Front2
             | Varizone | InvVarizone
}

AxisTree {
  nodes: Map<AxisId, Axis>       # AxisId is the historical "a.b"
}                                # WORLD is implicit: immovable, uncontrollable

Axis {
  parent:  AxisId | WORLD
  position: Vec3
  spin:     Vec3                 # revolutions; winding + terminal phase
  size:     Vec3                 # object axes: size. light axes: (ratio1, ratio2, intensity)
}

Insertion {
  at:    f32                    # fraction through the sweep, 0.0..1.0
  class: State | Input | Key
  mask:  bitset                 # which fields are applied
  values: ...                   # only masked fields meaningful
}
```

Cap insertions at 100 per sweep to match. Interpolation defaults sine-profiled,
with LINEAR as a state flag, per both sources.

### 7.5 Engineering store

250 slots. Slot 0 is the calibration datum; slots 1–249 store **deltas** against
it, so editing slot 0 shifts everything — reproduce this behaviour, including the
"always displays as zero" presentation, and gate slot 0 behind an
installation-mode confirmation.

### 7.6 Lighting pipeline

Keep the lighting raster a **distinct output** of the pipeline; do not fold it
into the frame. Per field:

```
for each coarse-grid cell, over a shifting 3-sample window:
    NP  = unit normal of the facet through the three neighbouring samples
    zone = facing test | varizone | fixed, per light
    for each enabled light L:
        SP, r = geometry of L to this point   (parallel: SP constant, drop 1/(r+k))
        cosA  = NP . SP
        cosB  = LP . reflect(SP, NP)          (LP fixed - orthographic view vector)
        term  = Kd(L,zone)*cosA + model(L,zone)[cosB]      # stage 35
        I    += signed_intensity(L) / (r + k) * term       # stage 36
    I = I * light_source_gain + ambient
    I = I * global_gain[zone]
apply filtering (-2..+3) and grid shift (0..2) across the cell lattice
interpolate cell values to pixel resolution         -> lighting raster
composite in 3D view space, BEFORE projection:
    diffuse  always multiplicative:  RGB * I
    specular multiplicative or additive per spectral_mode
then project (3D->2D), splat proportionally into the four surrounding
pixel addresses, accumulating contributions rather than depth-testing them
```

Notes:

- Implement the filtering ladder literally rather than as a generic blur. It is
  cheap, and it is the only place the historical shading resolution is directly
  observable — an emulator that gets `+1` and `+2` right is demonstrably running
  on the same grid as the machine.
- Grid shift is a lattice-index offset applied *before* interpolation, not a
  sub-pixel shift of the finished raster.
- Materials are indexed by `(light, zone)`, not by surface. Do not refactor this
  into a per-surface material system, however tempting.
- **`model(L,zone)` is a look-up table on `cos B`**, not a `pow()` call. Build it
  as a table from the start (§4.6.3); the four "Model n" entries are then just
  four tabulated exponents alongside Ramp, Posterise and the two ring models, and
  no special-casing is needed when S6 tells us what the tables actually contain.
- **`LP` is constant.** Do not helpfully compute a per-pixel view vector.
- Compositing is **write-side, pre-projection** — ADR-SM-014 resolved, §3.9.2.

### 7.7 UI

- Retain the **mode matrix** (major mode × parameter class) with SHIFT / USE H /
  USE V / NOTCH modifiers, mapped onto the Stream Deck XL + trackball surface
  already specified in the control-surface work units.
- Retain **REF/OBJECT as independently selectable live frames** with cross-wire
  overlay, solid/dotted for toward/away — but reinterpreted per S3 as **UPPER
  AXIS / LOWER AXIS** selectors over a general tree, not as two fixed frames.
- Provide the **`a.b` axis identifier** as a first-class addressing scheme, and
  the *number → OBJECT* fast path to any light. Preserve the historical default
  `b` values so `3 OBJECT` reaches light 3.
- Add the Starlight pages: `VDU 6` (lighting switches and shading resolution),
  `VDU 7` (per-light models, zones, style, shape), `VDU 8` (tree reconnection),
  `MORE 6` (gains, ambient, varizone). Reproduce the **asterisk in the flags
  field** as the indicator that a MORE-6 parameter has been altered and will be
  captured on the next insertion.
- Retain **numeric-prefix-plus-context** as the universal accelerator.
- Retain **typed find** (FIND STATE / FIND INPUT / FIND KEY) as a class-filtered
  cursor over the timeline rather than a generic keyframe list.
- Retain **`NUMBER SET` with empty entry restores the default**.
- Adopt **END POINT** for keyframe and **SWEEP** for interval, project-wide.

---

## 8. Regression fixtures available from the sources

Directly usable as tests:

1. **S1 §6(c) rolling cone.** `sin θ = 1/n`; n = 2 → θ = 30°; h = 0.55 →
   r = 0.55·tan30° = 0.3175; two axis-rotations per circuit. Transform list given
   in full in the source.
2. **S1 §6(a) corner-to-corner flat rotate.** Conjugation angle
   arctan(vertical lines / horizontal lines) = 36.079°; transform list
   `Screen Z turn 36.079/36.079`, `Screen X turn 0/360`, `Screen Z turn −36.079`.
3. **S1 §6(d) two-angle non-composition.** Two simultaneous 0–360 screen-axis
   rotations must produce a rotation about a 45° axis with **two** square corners
   fixed. A good check that we have not silently "fixed" the historical behaviour.
4. **S1 §16–17 four-sweep example effect.** Zoom flat to small top left → move to
   top right becoming globe → globe spins to centre → globe explodes (shapes
   1, 4, 4, 14 in effect 10). Exercises insert, edit and delete sweep.
5. **S2 spin destination.** `3.5` from any start angle must end exactly inverted
   having made three and a half anticlockwise revolutions.
6. **S2 notch grid.** POS notch step must equal exactly 1 grid unit in both axes.
7. **S2 perspective.** Default 56 units; size change rate ∝ 1/d²; passing d
   through zero reverses the image.

From S3:

8. **Default control tree.** Power-up and clear must reproduce the topology and
   identifier table of §5.6.1 exactly: lights 1 and 2 under gantry 8 via their
   reference axes, lights 3–6 under gantry 9, `REF 3 → REF 2 → REF 1 → OBJECT`
   under WORLD. Assert `3.1` does not resolve.
9. **Default lighting state.** All six lights ON in VDU 6, but intensities of
   lights 2–6 at `0.0`, so exactly one light is visibly present. Filtering `−1`,
   grid shift `Zero`, light-source gain luma 100% sat 0%, ambient luma 30% sat
   0%, global gain 100% on both zones.
10. **Filtering ladder.** `0` → smooth interpolation; `+1` → one flat value per
    coarse grid; `+2` → flat over 2 × 2 grids; `+3` → flat over 3 × 3. Count
    distinct shading values across a known gradient to verify the cell size.
11. **Grid shift.** With filtering `+1`, `Shift 1` must move the whole shading
    pattern exactly one coarse grid horizontally relative to `Zero`.
12. **Negative-intensity cancellation.** Two lights identical in every parameter
    except intensity sign must produce a net-zero contribution.
13. **Beam bidirectionality.** A beam light must illuminate symmetrically along
    both `+z` and `−z`. Any implementation that clamps to the forward hemisphere
    fails this.
14. **Parallel-light direction on lights 1/2 vs 3–6.** Move the object with a
    parallel light 1: the illumination direction must track the light-origin →
    object-origin vector. Do the same with parallel light 3: the direction must
    not change. Spin each: light 1 must not respond, light 3 must.
15. **Point-source limit.** As a point light's distance increases without bound,
    its lighting pattern must converge on that of a parallel light of the same
    direction.
16. **Zone locking.** Lights 3 and 5 must expose only a zone-1 model and be scaled
    by global gain 1; lights 4 and 6 only zone-2 and global gain 2.

From S5:

17. **The illumination formula.** Evaluate `I = Ia·Ka + Ip/(r+k)·(Kd·cos A +
    Ks·cosⁿ B)` against a hand-worked case: unit normal along +z, source at 45°,
    fixed view vector, `n = 2`. Any implementation that silently substitutes a
    half-vector, inverse-square falloff, or a per-pixel view vector fails.
18. **Distance falloff is linear.** Doubling `r` must multiply the direct term by
    `(r+k)/(2r+k)`, not by ¼. With `k` chosen, the term must stay finite as
    `r → 0`.
19. **Normal from a three-sample facet.** Given three adjacent coarse-grid
    samples, the computed normal must equal the normalised cross product of the
    two edge vectors, and must be attributed to the point P of the batch — not to
    the centroid. Getting the attribution wrong reproduces grid shift by accident
    and masks a real bug.
20. **Splat weights.** An element projecting to a non-integer X′Y′ must distribute
    proportionally across exactly the four surrounding pixel addresses, summing
    to unity.
21. **An opaque sphere.** Wrap a picture onto SPHERE and rotate. The far
    hemisphere must not be visible through the near one, and the terminator must
    not show a 50/50 blend. The single most direct test that sheet arbitration
    exists at all, and the one that falsified draft 0.3's first pass.
22. **Minification averages, occlusion does not.** Under 8:1 minification many
    source pixels landing on one destination pixel must area-average
    (accumulate-then-normalise, within the sheet). Two *sheets* landing on one
    pixel must not. Both behaviours in one fixture, because conflating them is
    the failure mode.
23. **Visible back faces, from S7.** An open-topped cylinder viewed from slightly
    above must show: the near wall's exterior carrying the FRONT source; the far
    wall's interior, through the opening, carrying the BACK source; the near
    wall's top edge cutting the visible interior band cleanly; and no trace of
    the far wall through the near one. Back-face culling fails this, as does any
    single-source model. Reproduces S7 directly.
24. **Non-convex self-occlusion.** PAGE TURN and SPINY NORMAN. A traversal-order
    painter's algorithm is expected to fail here; if the machine did not, its
    arbitration is not ordering alone. Discriminating fixture for §11.3 item 17.
25. **Front/back source switching.** Feed distinguishable pictures to the front
    and back inputs. Every sample must take its source from the sign of the
    surface normal, and freezing one side must not freeze the other.
26. **Orthographic specular.** Move the object laterally with a fixed light and
    fixed viewpoint: the specular highlight must not track as it would under a
    per-pixel view vector.

---

## 9. Open architectural decisions

| ID | Decision | Status |
|----|----------|--------|
| ADR-SM-001 | Project file is a replayable text command log (S1 SAVE/OBEY model) | Proposed |
| ADR-SM-002 | Shape manifest grammar derived from `.QUE` | Proposed |
| ADR-SM-003 | Frame content: address map + validity mask only; no stored depth or normals | **Resolved by S3 — accepted** (§3.9) |
| ADR-SM-004 | Sparse masked state deltas rather than full snapshots | Proposed |
| ADR-SM-005 | Spin stored as (winding, terminal phase) destination | Proposed |
| ADR-SM-006 | REF/OBJECT as two live TRS frames, not a baked product | **Superseded by ADR-SM-009** |
| ADR-SM-007 | Engineering slots as deltas against slot 0 | Proposed |
| ADR-SM-008 | Coordinate convention: adopt FVP 8×6 units, y up; sign-flip all S1 material on import | Proposed |
| ADR-SM-009 | WORLD-rooted general axis tree (S3 control tree) replaces the fixed REF/OBJECT chain; `a.b` identifiers kept as stable external axis names | Proposed |
| ADR-SM-010 | Lighting is a separate per-field modulation raster composited onto the video path, never baked into a shape frame | Proposed |
| ADR-SM-011 | Shading evaluated per coarse-grid cell and interpolated, with the historical filtering (−2…+3) and grid-shift (0…2) controls exposed literally | Proposed |
| ADR-SM-012 | Reflectance parameters owned by the `(light, zone)` pair, not by the surface | Proposed |
| ADR-SM-013 | Light parameters modelled as named fields; the historical x/y/z-size packing retained only as a compatibility view for import/export | Proposed |
| ADR-SM-014 | Lighting composited **write-side, in 3D view space, before projection** | **Resolved by S5 — accepted** (§3.9.2) |
| ADR-SM-015 | Illumination evaluated as the S5 Phong-family formula, with `1/(r+k)` falloff and a fixed line-of-sight vector; per-light-per-zone specular via a `cos B` look-up table rather than a `pow()` | Proposed |
| ADR-SM-016 | Forward/scatter resampling: per-input-sample 3D address projected live to a 2D destination, proportional four-pixel splat, accumulate-then-normalise **within a surface sheet** | Proposed |
| ADR-SM-017 | Occlusion **between** sheets arbitrated by a mechanism yet to be identified — traversal ordering, depth compare, or both (§3.9.4.1) | **Blocked on §11.3 item 17** |
| ADR-SM-018 | Two video sources per effect, front and back, selected per sample by surface facing; each independently freezable; A/B feed selection on each side (§3.9.4.3) | Proposed |

---

## 10. Errata and corrections log

### S1

| Ref | Issue |
|-----|-------|
| Help file 1 vs 2 | VCP boot command given as `%bdi` in "Starting to use the Hewlett Packard computer", `%bdc` in "Hints on using the Mirage programs and RTE-A". `%bdc` is corroborated by the transcript `VCP> %BDC` and is presumed correct. |
| Joystick pot table | First row labelled "Size & position" but its content (luminance, saturation, hue; joysticks set width and symmetry) is plainly the **Border** mode. The four assignable sets are named in the prose as border, matte, manual transparency, external soft key. Row label is wrong. |
| Transition module | Button legend "Set Time" vs prose "Learn Time" — same function. |
| Mtest STATE | `KEY = <gain> = <offset>` is malformed as printed; presumably `KEY = <gain>, <offset>` or two separate assignments. |
| Mtest STATE | Boolean list uses *manual key / autokey / external key* where the panel says Manual Transp / Auto Transp / Ext Key. Explicit mapping table needed. |
| p.16 | Manuscript insertion `ENTER EFFECT 10 : 10 - ENTER EFFECT + EFFECT` — operator clarifying the chord. |

### S2

| Ref | Issue |
|-----|-------|
| §2.2.4 vs §2.3.2 | Viewing distance "7 screen **heights**" vs "7 screen **widths**". §2.3.2 gives 1 screen height = 6 units and PERSPECTIVE default = 56; 56 / 8 units-per-width = 7 widths. **Widths is correct; §2.2.4 is the error.** |
| §2.2.5 / §2.3 | Two examples both numbered **17** (FOCUS, and numeric entry half-size). |
| §3.3 heading | Section headed "State" but every cross-reference is to **3.4**; §3.5 refers to "the States menu (Sec 3.4)". No §3.4 exists — the State section is misnumbered. |
| §3.4/§3.5 | §3.5 numbered as if §3.4 preceded it; sections 3.4 and (probably) a Disk/Set Up section are missing or renumbered in this draft. |
| §2.3.2 | "Mirage recognizes 7 numbers before or after the decimal point with 6.5 digits internal accuracy" — self-inconsistent as printed. |
| §2.4.1 step 5 | `XXX INSERT EFFECT` corrected in manuscript to `XXX INSERT DELETE EFFECT`, consistent with §2.4.2. |
| Library table | "RESERVED **FIR** DISC2"; "6 SIDED PYRAMID with **opeb** bottom"; `sh 55` listed twice (as `54/55` and alone); `sh 59` VORTEX interleaved out of order between `sh 35` and `sh 36`. |
| p.4 | Manuscript: "SIZE Z AXIS USE H, SHIFT". |
| §2.3.1 | Library table header `Size Start End Move Title` — "Move" column actually carries the shape id or id-pair, not a move. |

### S3

| Ref | Issue |
|-----|-------|
| Whole document | Both cover sheets are hand-annotated **OUTDATED**. Treat every quantitative claim below as provisional pending a later revision. |
| Throughout | **"spectral"** is used consistently where **specular** is meant — in prose, headings and menu box legends. Not a typo; a house malapropism. |
| §2.1.1, §2.1.2 | Headings "Modelling Diffuse Light" and "Modelling Spectral Light" with one-word bodies ("Diffuse light.", "Spectral light."). The mathematical theory promised in §1.2.1 is entirely absent — the single largest gap in the document. |
| §5.2.5 | "Backlight — ON / OFF" followed by the body "Backlight." Function undocumented anywhere. |
| §5.5 | States there are "**eight** control boxes" on VDU 8, then names **ten**: Light 1 reference, Light 1, Light 2 reference, Light 2, Light 3, Light 4, Light 5, Light 6, Reference 3 and the Object. The tree diagram in §3.1.1 shows eight `*` markers — at LS REF 1, LS REF 2, Lights 3–6, REF 1 and OBJECT. Neither list matches the other. |
| §3.1.1 vs §3.1.1 diagram | Prose says "even the object and **reference axis 3** may be reconnected"; the diagram places its reconnection marker on **REF 1**, not REF 3. Diagram and prose disagree on which object-limb reference axis is mobile. |
| §3.1.4 vs §5.2.3 | Grid shift values given as `(zero, +1, +2)` in one place and `Zero / Shift 1 / Shift 2` in the other. Naming only. |
| §3.1.4 | Lists the varizone generator under MORE 6 as `(top, bottom, left, right)` without ranges; §5.4.3 supplies `0` to `+100%`. |
| §4.4.3 / §5.2.4 | The spectral add/multiply passage is duplicated near-verbatim between chapter 4 and chapter 5, including the black-object-white-highlight example. Chapter 5 was evidently written by copy-paste from chapter 4. |
| §4.5.1 | Forward-references the filtering and grid shift description to "the section 'VDU 6 — Lighting'", i.e. §5.2 — the "described elsewhere" chain runs forward, not back, throughout chapters 4 and 5. |
| §3.1.1 | "lighting gantry" is spelled `gantry` in the prose and `Gantry 8`/`Gantry 9` in the diagram; OCR of the scan renders one instance as "ga(n)try". Cosmetic. |
| §4.4.1 | Says light intensity "falls off from a maximum along the z axis to a minimum on the boundary of the cone" for beams "in the standard shipping form" — implying the cosinusoid is a build option, but no alternative is documented. |

### S4

| Ref | Issue |
|-----|-------|
| Bullet list | Advertises "4:4:4:4 component digital quality"; the body text gives internal processing as 4:4:4:4 and standard I/O as coded video / RGB / key, i.e. the 4:4:4:4 path is only realised with the optional Combiner. Marketing compression, not an error, but worth not taking at face value. |
| Body | Corner Pinning is described in the Starlight section as shipped with the package, and separately under its own heading as something Mirage "is now equipped with" — no statement of whether it is available without Starlight. |
| Body | "Four levels of motion" is used without definition. |
### S5

| Ref | Issue |
|-----|-------|
| Face page, references cited | **US 4,899,295 (Nonweiler) is dated 2/1989**; the EPO record gives the grant as 6 Feb **1990**. Face-page typo. This is the Starlight patent (S6), so the date matters when searching. |
| Face page vs body | The abstract begins "Electronic image processing for manipulating data representing a three dimensional object. **The three dimensional object.** The three dimensional position and colour…" — a dangling fragment, present in the granted text. |
| Brief description of drawings | "FIG. 2 is a diagram explanatory of **FIG. 2**" — should be FIG. 1. |
| Body, formula | The illumination formula is mangled in OCR as `I = laKa + Ip/(r+k) [Kd cos A + Ks cos" B]`. Read as `Ia·Ka` and `cosⁿ B`; the variable list immediately below disambiguates both, and `n` is defined there as "a small integer, say 2". |
| Body | "identical with the circuit **8**" where the masking circuit under discussion is **78**. |
| Body | Circuit 53's output is described as applied to function generator 55, but the sentence reads "This output signal from circuit **53**" where the preceding sentence has the cos C signal coming from circuit **54**. Chisel path only; does not affect the starlight chain. |
| Throughout | The scanned OCR renders many digits and words poorly (`21 1,345`, `2,158,67 1`, `emperical`, `spectral`/`spatial` confusions). Quote from the granted text, not the OCR layer. |
| Note, not an erratum | S5 spells **specular** correctly throughout, unlike S3. The "spectral" malapropism is the Mirage manual's, not Quantel engineering's. |

### Corrections to this document

| Draft | Correction |
|-------|------------|
| 0.1 → 0.3 | §7.2's frame described as a "per-output-pixel source-address map". Direction inverted; it is a per-input-sample destination map. §3.9.4. |
| 0.1 → 0.2 | §11 Q3 recorded a "strong prior" that the mask concept extended to a Light insertion class. It did not. §4.5.6. |
| 0.2 → 0.3 | EP 0320166 A1 treated as the Starlight patent. It is not; S6 is. §2.1. |
| 0.3, first pass | §3.9.4 presented S5's masking circuit 78 as the home of Starlight's front/back zone distinction. Wrong; facing comes from the sign of the normal. |
| 0.3, second pass, addendum | S7 shows the back of the surface carrying a different picture from the front. §3.5, §5.5 and §3.7 each held part of the explanation — the FRONT/BACK input pair, the FRONT/BACK CONTROL line to the switcher, and ENG 48's front/back switch delay — and drafts 0.1–0.3 transcribed all three without connecting them. Front/back is a facing-driven source switch, not a transition A/B pair. §3.9.4.3, ADR-SM-018. |
| 0.3, second pass | §3.9.4 then argued circuit 78 could not be Mirage behaviour because depth arbitration "would change the look", Mirage being a machine "where the ghosting is the point". **Flatly wrong.** v1 Mirage sphere effects are opaque; the machine arbitrates between surface sheets. The error was conflating accumulate-then-normalise as a *resampling* rule (correct, and what the patents describe) with accumulation as a *compositing* rule between sheets (never the behaviour). Corrected in §3.9.4.1–.2; ADR-SM-016 rescoped, ADR-SM-017 opened, fixtures 21–24 added, §11.3 item 17 rewritten and promoted. |


---

## 11. Question ledger

### 11.1 Answered in draft 0.2, by S3 and S4

**Q1 — did the library track gain normals or a depth channel? NO.** Resolved in
§3.9. Starlight is a field-fit upgrade that consumes shape information from the
ESU and works with the as-shipped library shapes and shape table 0; orientation
is derived at run time at coarse-grid resolution. **ADR-SM-003 closes.** The
frame format is settled and §7.2's optional depth and normal planes drop out of
day-one scope. This was the item gating the whole storage design, and it has
gone the cheap way.

**Q3 — did the mask/insertion concept extend to a Light class? NO.** Lighting
parameters ride on ordinary axis records and the MORE 6 page, and are captured by
ordinary end-point (and way-point) insertion, ramping through the effect exactly
as object parameters do. There is no `INSERT LIGHT` / `FIND LIGHT`. Draft 0.1's
"strong prior that it did" was wrong. §4.5.6.

**Q4 — lighting parameter representation.** Colour is the same LUM/SAT/HUE
0–100% space as S2. Gains follow the established percentage style (−400/+400 for
per-light intensity, −200/+200 for global gain, 0–100 for varizone). Lights are
attached to **neither** the REF nor the OBJECT frame by default: they hang from
two new gantry axes directly under a new WORLD root, and may be reconnected to
almost any other axis. §4.5.5, §5.6.1.

**Q5 — is the ooze still a pure linear interpolation?** Defused rather than
answered, and defused favourably. Because normals are *derived* from the address
map rather than stored, linear interpolation of the address map yields
self-consistent orientation throughout an ooze; there is no separate normal
channel to blend badly. Whatever shading behaviour an ooze produces is the honest
consequence of the interpolated surface, and reproducing the interpolation
reproduces it.

**Q6 — did the coordinate and parameter conventions survive? YES.** Light
position and spin are controlled "in an identical manner to that of the object",
and every new quantity uses FVP percentage conventions. §4.5.5.

**Q7 — control system name and panel complement.** The panel is unchanged in
kind: numerics module, trackerball menu, VDU pages, and the multi-channel
four-button axis walk. What changes is semantics — REF and OBJECT become UPPER
AXIS and LOWER AXIS — plus four new pages. S4 names the system feature
**Floating Viewpoint Control** and lists Encore as a separate product; see §2.

**Q8 — shape authoring toolchain.** Partially. **Contour** (mouse and tablet,
profile plus cross-sections, runs off-line while Mirage keeps generating effects)
is the era's supported authoring route. The fate of `.QUE` / `.ANS` /
`object.txt` remains unknown. §6.5.

### 11.2 Answered in draft 0.3, by S5

**Item 1 (old) — the lighting mathematics.** Largely recovered. §4.6 gives the
closed-form illumination model and the evaluating circuit. What remains is the
contents of the eight spectral-model tables and the six-light summation rule; the
*form* of the model is no longer in doubt.

**Item 3 (old) — per-pixel vs per-coarse-grid.** Closed. Both, at different
stages: evaluated per facet from a three-sample neighbourhood, applied per pixel.
The earlier "per-pixel lighting" claim also rested on a misattributed patent
(§2.1). See §3.9.1.

**Item 4 (old) — compositing position.** Closed in favour of **write-side,
pre-projection**. ADR-SM-014 accepted. See §3.9.2.

Draft 0.3 also closes two things that were not on the list because draft 0.2 did
not know they were open: the **direction of the address map** (forward/scatter,
§3.9.4) and the **identity of the Starlight patent** (§2.1).

### 11.3 Still open, in priority order

1. **Obtain S6 — US 4,899,295 / EP 0248626 B1** (Nonweiler, Quantel, priority
   3 Jun 1986). This is now the top action and it subsumes several items below.
   It should give the eight spectral models, the multi-light summation, the
   coarse-grid dimensions, and possibly backlight. It is a granted patent in a
   readily searchable family, so this is a tractable ask, unlike a superseding
   revision of S3.
2. **The eight spectral-model tables.** Models 1–4, Ramp, Posterise, 2 ring,
   4 ring. §4.6.3 gives a well-supported hypothesis (stage-35 LUTs on `cos B`);
   it is still a hypothesis.
3. **Multi-light summation.** S5 has one white light; Mirage has six coloured
   ones. Where in the chain does summation happen relative to the ambient floor
   and the distance term?
4. **Sign and clamping.** Negative `cos A` / `cos B` handling, and how it squares
   with negative light intensity and with beams emitting out of both `±z`.
5. **Backlight.** Still one word in S3 and absent from S5. §4.6.4 offers a
   hypothesis (disabling a back-face rejection test) that is explicitly not
   licence to implement.
6. **Coarse grid dimensions.** Still unstated in S1–S5.
7. **Varizone edge profile** — hard boundary or soft ramp?
8. **A non-outdated revision of S3.** Chapter 2 would confirm or refute §4.6
   against the product as shipped rather than against a sibling machine.
9. **Shape tables.** How many, how selected; does the transition table dimension
   change from S1's 75 × 75?
10. **Sweeps per effect and effects count at V4.02.**
11. **Corner Pinning** — are "four levels of motion" and "true throughpoint
    curving" the return of S1's `Curved`? Is corner-pin data part of the end-point
    record?
12. **Multi-channel Mirage.** Referenced twice by S3 as the source of the control
    tree structure; undocumented.
13. **Way-point versus end-point** — distinct record type, or a non-terminal end
    point?
14. **Engineering values at V4.02**, including any Starlight-crate ↔ DPU phasing.
15. **FOCUS** ranges and defaults.
16. **Interlace.** Lighting is computed per field; does a field-rate light move
    produce field-rate shading judder on a static object?
16a. **Where does the front/back source switch live**, and at what granularity?
    §3.9.4.3 argues for an internal per-sample select between two digitised
    feeds, with the external matrix merely routing. Confirm against the DVM8000/1
    hardware documentation. ENG 48's 1024-step delay is the clue to chase.
17. **How does Mirage arbitrate between surface sheets?** *(Substantially
    advanced by WU-SM-02 0.2: ordering is effectively excluded and a depth plane
    favoured, on the `Opaque`/`Trail` mutual exclusion. Retained here until A1
    confirms.)* Promoted to the top of
    this list below item 1. The machine demonstrably resolves occlusion — v1
    sphere effects are opaque — but no held source says how, and FVP forces the
    mechanism into the real-time path (§3.9.4.1). Candidates: traversal-order
    painter's algorithm, a depth plane and compare as in S5, or both. Gates
    ADR-SM-017 and determines whether WU-28 is emulation or extension. Check S6,
    the DVM8000/1 documentation, US 4,563,703 and UK 2,158,671 in full rather
    than as recited, and period footage of PAGE TURN and SPINY NORMAN.
18. **Verify whether EP 0320166 A1 is in fact the family member of US 5,103,217.**
    Low stakes now that the misattribution is understood, but worth closing so the
    project's citation record is clean.

---

## 12. Handoff protocol for the next session

1. **Get S6: US 4,899,295 / EP 0248626 B1.** One granted patent, identified by
   number, in a family the EPO record already lays out. It is a far cheaper
   target than a superseding Starlight manual and it answers more. Everything
   marked `[C]` in §4.6.3 stands or falls on it.
2. Do not re-derive S1–S5 material. Diff only.
3. Closed: ADR-SM-003, ADR-SM-014. Live: ADR-SM-009 … 013, 015, 016. §7 can now
   be promoted from proposal to specification for everything except the
   spectral-model table contents.
4. Append to §10; keep the corrections log cumulative across all five held sources.
5. **WU-SM-02 — Surface Arbitration and the Front/Back Source Pair** is now
   written and takes 02 in the chain. It gates ADR-SM-017 and informs WU-26/27/28,
   and it opens ADR-SM-019/020/021.
   The Starlight lighting pipeline unit, previously proposed as WU-SM-02, becomes
   **WU-SM-03**. It can be split out of §7.6 and written as a real specification
   with the spectral models left as a pluggable table, in parallel with WU-SM-02:
   it depends on ∂z in `Lattice::jacobian()` but not on the outcome of the
   arbitration question. Do not wait for S6 to start it.
6. Revisit the scatter-dve GPU route assessment against §3.9.1, §3.9.4 and §7.6
   together. The workload is now fully characterised: per-facet normal from a
   three-sample window, one Phong evaluation per light per cell, a fixed
   filtering ladder, interpolation to pixels, then a forward splat. That is a
   materially different shape from the per-pixel model previously assumed —
   cheaper in shading, but with no early-out, doubled destination contention where
   sheets overlap, and a wider read-modify-write per splat (WU-SM-02 §5.2). The
   1080p50 conclusion should be re-derived rather than inherited.
7. **Run a cross-reference pass over this document before ingesting anything
   else.** The front/back source finding (§3.9.4.3) sat unnoticed across three
   drafts in §3.5, §5.5 and §3.7 — all three correctly transcribed, never read
   against each other. Rule 2 above ("diff only") is right for throughput and
   wrong for synthesis; one pass reading the held material against itself, rather
   than against the next source, is now part of the protocol.
8. Note for the control-surface work units: nothing in draft 0.3 changes the
   surface mapping, but ADR-SM-009 (UPPER/LOWER AXIS over a general tree) has not
   yet been reflected there.

---

*End of WU-SM-01 draft 0.3. 0.1 covered S1 and S2; 0.2 added S3 (Starlight
operator's manual, OUTDATED draft) and S4 (Starlight brochure); 0.3 adds S5
(US 5,103,217) and identifies S6 (US 4,899,295 / EP 0248626) as the outstanding
primary source.*
