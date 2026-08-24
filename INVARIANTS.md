# Invariants

Frozen. Changing one requires a superseding ADR in `DECISIONS.md` and a review
of every test that depends on it. No session may relax an invariant to make
code work.

**I1 — Forward scatter, never inverse gather.**
Source samples are thrown at the destination. Non-invertible maps, folds, tears
and shattering are only expressible this way. This is what makes it a Mirage
rather than an ADO.

**I2 — No clipping, no legalisation, ever.**
The only permitted clamp is to v210 protocol limits, 10-bit codes 4 to 1019,
applied at the pack stage alone. Codes 0–3 and 1020–1023 are reserved for TRS
and writing them would synthesise spurious SAV/EAV. Everything else —
sub-black, super-white, filter ringing, normalisation overshoot at fold edges —
passes through untouched. Legalisation is a separate function belonging at
transmission, not in a DVE.

**I3 — Offset-binary 16-bit internal RGB colour (ADR-085; supersedes this
invariant's original YCbCr-internal text).**
10-bit code shifted left 6. Black sits at code 4096 (`kBlack`) on every
channel — R, G and B are all full-range, unlike the superseded design's Y
(full-range) plus Cb/Cr (offset around the achromatic code, 512/32768); no
channel needs a mid-point offset any more. No signed arithmetic anywhere in
the colour path.

*Terminology:* do not call the 64 offset a "pedestal". Pedestal is analogue
NTSC setup, 7.5 IRE above blanking, absent from PAL and from all digital
component representations. In BT.601/709 black and blanking are the same level
and the space below 64 is footroom.

**I4 — 64-bit integer colour accumulators.**
Worst case for a single fragment, on any one of the three colour channels
independently, is 65535 × 65535 ≈ 4.29 × 10⁹, already at the uint32 boundary
before any multi-fragment coverage. Re-derived directly against
`core/types.hpp` under ADR-085's RGB migration rather than assumed to carry
over either way: the bound comes only from each channel's own `Sample`/
`Weight` storage-type range (I2 already lets any channel reach it in full),
never from where that channel's nominal resting point sits, so it was
already identical for Y, Cb and Cr and stays identical for R, G and B —
unaffected by moving from one full-range channel plus two nominally-offset
ones to three full-range ones. Weight accumulators may be int32.

**I5 — Normalise before compositing.**
Divide accumulated colour by accumulated weight first, then composite using
coverage as alpha. Compositing premultiplied offset-binary values against zero
gives a green fringe on every partially covered edge, because Y=0, Cb=Cr=0 is
not black — it is below-black luma with fully saturated chroma.

**I6 — Integer arithmetic throughout the accumulation path.**
Integer addition is associative, so output is bit-identical regardless of
thread count, tile size or scheduling order. This is the project's most
valuable debugging property: the single-threaded build is a permanent oracle,
and any multi-threaded run must match it byte for byte. Never introduce
floating point into accumulation.

**I7 — Identity map round-trips bit-exactly.**
Input v210 equals output v210, byte for byte, illegal excursions included. This
is the foundation test. If it passes, the transport, the offsets, the
accumulators and the normalisation are all honest, and every artefact seen
afterwards is genuinely the warp.

**I8 — Nothing is culled; back faces always splat.**
Back-facing surfaces are rendered, not dropped, and carry the back source
(`docs/sources/WU-SM-01.md` §3.9.4.1/.3, ADR-073). Splat count therefore
stays fixed at source-raster size regardless of shape or viewpoint — do not
add an early-out on facing anywhere in the fragment-generation or splat
path. Destination contention roughly doubles wherever two sheets overlap,
which for a closed shape is most of its area; this is expected, not a
regression to fix.

**I9 — No per-sample normal or depth channel in the lattice/library
format; both are derived at run time.**
The library frame is an address map plus a validity mask and nothing else
(ADR-066). Surface normals (`core/jacobian.hpp`'s `surfaceNormal()`, WU-26)
and any future depth-gradient sheet-membership tolerance (ADR-072) are
computed from the lattice's own Jacobian each time they are needed, never
read back from a stored per-sample channel. A change that adds a stored
normal or depth plane to the lattice/library container requires a
superseding ADR, not a quiet extension of the container.

**I10 — Shading is applied pre-projection; resolve-time shading is
unavailable.**
The source sample is shaded in 3D view space, before the 3D→2D projection
step, and the *shaded* value is what gets splatted (ADR-068). No pipeline
stage after the splat may re-derive or adjust lighting from an accumulated
cell, because I9 already forbids carrying the normal that would require.

**I11 — Within-sheet contributions accumulate; between-sheet blending is a
separate resolve-time stage.**
Two source samples from the same surface sheet landing on overlapping
footprints accumulate exactly as ADR-001 already governs — this is
resampling, not compositing, and stays untouched. Two samples from
*different* sheets never simply accumulate-and-normalise; they are resolved
by whatever between-sheet mechanism ADR-072/ADR-074 eventually settles,
behind the swappable interface ADR-074 requires. A resolve path that cannot
tell "same sheet" from "different sheet" is not a valid implementation of
either half of this invariant.
