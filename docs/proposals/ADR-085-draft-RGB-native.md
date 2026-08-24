# ADR-085 (ACCEPTED — see DECISIONS.md ADR-085)

**Internal colour representation becomes native RGB throughout the
pipeline, superseding I3's YCbCr-internal design. The 4:2:2 v210 I/O
boundary is unchanged (ADR-005 stands as written); the RGB conversion
moves to that boundary instead of happening transiently inside shading
alone. Hard-cutover migration, scoped as its own dedicated phase, decided
directly with Steve.**

Status: **ACCEPTED**, WU-38, this session. Steve reviewed this draft and
approved it as written, except I4 (re-derived directly against the real
accumulator code rather than assumed either way — see `DECISIONS.md`
ADR-085). `DECISIONS.md` (ADR-085), `INVARIANTS.md` (I3, I4) and
`WORK-UNITS.md` (Phase 9, `WU-38`–`WU-44`) are all updated accordingly;
`docs/architecture.md` is not yet — that rewrite is `WU-43`'s own job,
once the production code it describes has actually landed. This document
is kept as the historical record of the proposal; treat `DECISIONS.md`
ADR-085 as the settled entry from here on, per `SESSION-PROTOCOL.md` rule
3 ("never reopen an ADR").

---

## 1. Motivation

Raised directly by Steve, immediately after WU-34b (ADR-084) shipped a
transient per-sample YCbCr→RGB→multiply→YCbCr round trip for the
Starlight shading multiply: rather than optimise where that round trip
happens, make RGB the pipeline's actual internal representation, matching
the real machine.

This is well-founded, not a stylistic preference. ADR-003 ("integer
accumulators, int64 for colour") and ADR-005 ("4:2:2 v210 only for I/O;
4:4:4 internally") were never fidelity claims about Mirage's own real
architecture — ADR-003 chose integer accumulators for cross-thread
determinism, and ADR-005 solved a chroma-fringing problem specific to
warping 4:2:2 directly. Neither considered RGB at all. I3 froze
"offset-binary 16-bit internal colour" as this project's own early
engineering convenience, at a point before anyone here knew what Mirage's
real internal representation was. Steve has since supplied that fact
directly (the same domain knowledge ADR-084 already used): Mirage was
RGB-native throughout its whole signal path, with both the analogue/
composite and the 4:2:2 digital I/O converted to/from RGB at the boundary
alone. This ADR replaces a convenience decision with the real one, now
that the real one is known.

## 2. What does NOT change

**The 4:2:2 v210 wire protocol is unaffected.** Input and output stay
v210 10-bit 4:2:2 exactly as ADR-005 and `docs/architecture.md`'s own
"out of scope: 4:4:4 or RGB I/O" line already state — that line is about
the transport, not the internal representation, and this ADR does not
touch it.

**ADR-005's own chroma-fringing argument still holds unchanged.** Chroma
is upsampled 4:2:2→4:4:4 once at the front and downsampled once at the
back, for exactly the reason ADR-005 gives (independent per-sample
destinations in a forward scatter mean chroma lands at non-integer
positions relative to luma; warping 4:2:2 directly produces fringing that
follows the geometry). What changes is what happens to that 4:4:4 result
immediately afterward: today it *is* the internal representation for the
whole pipeline; under this ADR it becomes an intermediate that gets
converted to RGB once, right there at the front boundary, before PASS 1
ever runs — and the inverse conversion happens once, at the back boundary,
immediately before chroma downsample.

I1 (forward scatter), I5 (normalise before compositing), I7 (identity
round-trip bit-exact), I9 (no stored normal/depth) and I10 (shading
pre-projection) are all colour-space-agnostic and unaffected in substance
— though I7 in particular becomes the critical regression test for this
whole migration, and needs re-proving against the new representation, not
assumed to still hold.

## 3. What this supersedes

**I3 (`INVARIANTS.md`), directly.** Proposed replacement text:

> Offset-binary 16-bit internal RGB colour. 10-bit code shifted left 6.
> Black at code 4096 on every channel (`kBlack`) — no achromatic
> mid-point offset any channel needs; R, G and B are all full-range,
> unlike today's Y (full-range) plus Cb/Cr (offset around `kChromaZero`).
> No signed arithmetic anywhere in the colour path.

**I4 (`INVARIANTS.md`), needs re-derivation, not just a reword.** Today's
worst-case bound (65535 × 65535 ≈ 4.29 × 10⁹ per fragment) was derived
against one full-range channel (Y) and two channels offset around a
mid-point (Cb/Cr), where the *offset* component's own multiplication
against a weight behaves differently from a full-range component's. Under
RGB, all three channels are full-range like Y is today. Whether the
worst-case per-fragment product bound, and therefore the 64-bit
accumulator headroom, changes under that shape is a real open question
for whoever starts this work, not something this draft resolves.

**ADR-005's "4:4:4 internally" framing, in spirit, not in its actual
content.** The chroma-upsample-once/downsample-once behaviour it
specifies is preserved verbatim (see §2); only the description of what
the pipeline *does* with that 4:4:4 result changes. Whether this needs a
formal supersession note on ADR-005 itself, or just this ADR's own
existence as the record of the reframing, is a small procedural question
for whoever writes the accepted version.

## 4. Scope — every file this touches

Confirmed by direct inspection this session, not estimated:

- **`src/core/types.hpp`** — `Frag` and `AccumCell` both carry `Y, Cb, Cr`
  fields directly; this is I3's literal implementation. Becomes `R, G, B`.
- **`src/core/binner.cpp`/`.hpp`** — `sampleBilinear()` reads RGB from the
  source directly. WU-34b's `applyShading()` (ADR-084) *simplifies*: it
  currently does a full YCbCr→RGB→multiply→YCbCr round trip; under this
  ADR the colour arriving at that call site is already RGB, so shading
  becomes a bare per-channel multiply by intensity. `ColourStandard`/
  `coeffsFor()` (BT601/BT709) no longer belong solely to shading — they're
  needed at the I/O boundary too. Where they should live is an open
  question below.
- **`src/core/resolve.hpp`/`.cpp`** — PASS 2's splat/accumulate/normalise/
  composite math is written in terms of `Y`/`Cb`/`Cr` fields; reshapes to
  `R`/`G`/`B`. The arithmetic *shape* (weighted accumulate, divide by
  weight, offset-binary-safe) does not change; only what the three
  channels mean does.
- **`src/video/v210.cpp`, `src/video/chroma.cpp`/`.hpp`** — this is where
  the new boundary conversion is added: v210 unpack → chroma upsample →
  **new: YCbCr→RGB** on the input side; **new: RGB→YCbCr** → chroma
  downsample → v210 pack on the output side. I2's clip-to-protocol-limits
  behaviour almost certainly stays exactly where it is today — at the
  YCbCr boundary, immediately around pack/unpack — since v210's protocol
  limits (codes 4–1019) are inherently YCbCr code values with no literal
  RGB equivalent; this draft assumes I2 is unaffected but flags it for
  confirmation.
- **`docs/architecture.md`** — the Design Invariants table (I3, and I4's
  rationale column) and the §3 signal-path diagram both need rewriting to
  describe the new boundary conversions.
- **Tests — 21 of the 35 files in `tests/`** touch `Y`/`Cb`/`Cr` fields or
  comparisons directly (confirmed by grep this session): `test_binner`,
  `test_chroma`, `test_chroma_neon`, `test_coarse_shading`,
  `test_coverage_capture`, `test_deinterlace`, `test_ewa`,
  `test_field_pipeline`, `test_interlace`, `test_jacobian`,
  `test_kbuffer_resolve`, `test_kbuffer_storage`, `test_layered_composite`,
  `test_lighting`, `test_morph`, `test_pageturn`, `test_pipeline_bytes`,
  `test_ramp_roundtrip`, `test_row_band`, `test_scan_order_invariance`,
  `test_splat`. This is the real cost of the migration — the production
  code changes are comparatively mechanical; every one of these fixtures
  encodes expected values in the colour space under test, and every one
  needs its expected values re-derived, not just recompiled.

## 5. Migration decision

**Hard cutover — Steve's explicit choice, this session, over the
incremental/shadow-path alternative** (add RGB alongside YCbCr, prove
parity, cut over module by module, retire YCbCr last). Trade-off, stated
plainly: no shadow path means no bit-for-bit parity proof step and no
"suite stays green throughout" property — the suite goes red across most
of its 21 affected files for the duration of this work and comes back
green only once the whole cutover is complete. That is a deliberate,
larger-than-normal single undertaking. It does not fit `SESSION-PROTOCOL.md`'s
normal work-unit shape (3 source files, ~400 lines, green at the end of
every unit) — it needs its own phase, with the normal "green after every
unit" expectation explicitly suspended for the internal steps of that
phase and restored at the phase's end, the same way `WORK-UNITS.md`
already groups work into named phases (e.g. "Phase 1 — Portable core,
file to file, 576p25, single-threaded").

## 6. Suggested work breakdown (sketch only — not committed to `WORK-UNITS.md`)

For whoever starts this, once accepted:

1. `types.hpp` R/G/B rename + `INVARIANTS.md` I3 rewrite + I4 magnitude
   re-derivation.
2. `v210.cpp`/`chroma.cpp` gain the boundary conversion stages, both
   directions.
3. `binner.cpp`/`.hpp` — `sampleBilinear` reads RGB; `applyShading`
   simplifies to a bare multiply; resolve where `ColourStandard`/
   `coeffsFor` live now that both shading and the I/O boundary need them.
4. `resolve.hpp`/`.cpp` PASS 2 reshaped to R/G/B.
5. `docs/architecture.md` rewritten (invariants table, signal-path
   diagram).
6. The ~21 dependent test files, worked through in natural clusters
   rather than one at a time, since fixture values need re-deriving
   together per cluster: v210/chroma; binner/EWA/jacobian; resolve/
   kbuffer/layered-composite; pipeline/threading/row-band/field; lighting/
   coarse-shading.

Each of these steps is individually larger than a normal work unit's cap,
knowingly, per the hard-cutover decision in §5.

## 7. Open sub-questions for whoever starts this work

- I4's magnitude bound under all-full-range RGB (§3) — does the 64-bit
  accumulator headroom change, and if so does anything else depend on the
  current bound?
- Where `ColourStandard`/`coeffsFor` should live once both shading and the
  I/O boundary need them — likely promoted out of `binner.cpp` into a
  shared colour-conversion module near `types.hpp`, but not decided here.
- Whether the boundary conversion is per-frame or needs to be
  parameterised the same way WU-34c's deferred per-frame scene ownership
  already needs to be, since both land in `core/pipeline.cpp` around the
  same call site.
- Fixture-value re-derivation strategy for the 21 affected tests — hand-
  derive each independently, matching WU-34b's own "mirror the math
  independently, never call the production function" test-design
  precedent (ADR-084 §"Built this session"), versus a mechanical
  transform of existing fixtures. The former is safer and is what this
  project has done every time so far; it is also the larger share of the
  actual work in this whole ADR.

## 8. Status and next step

This session already closed out and delivered WU-34b (ADR-084), built,
tested across the full 10-configuration matrix, and written back to the
real repository — see `HANDOFF.md` for Steve's own close-out commands.
This ADR is a separate, fresh initiative and is not layered onto today's
build. Steve reviews and edits this draft; once he accepts it, it gets
logged as ADR-085 in `DECISIONS.md`, `INVARIANTS.md`'s I3 (and I4, once
re-derived) get updated, and `WORK-UNITS.md` gains a new phase heading
with the breakdown in §6 turned into real, ordered work-unit entries —
all as a dedicated future session's own first job, starting fresh against
the real repository state the same way every session in this project
does.
