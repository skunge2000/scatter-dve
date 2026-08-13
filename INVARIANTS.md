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

**I3 — Offset-binary 16-bit internal colour.**
10-bit code shifted left 6. Black sits at its BT.601/709 code, 64 (4096 after
the shift); chroma at its achromatic code, 512 (32768). No signed arithmetic
anywhere in the colour path.

*Terminology:* do not call the 64 offset a "pedestal". Pedestal is analogue
NTSC setup, 7.5 IRE above blanking, absent from PAL and from all digital
component representations. In BT.601/709 black and blanking are the same level
and the space below 64 is footroom.

**I4 — 64-bit integer colour accumulators.**
Worst case for a single fragment is 65535 × 65535 ≈ 4.29 × 10⁹, already at the
uint32 boundary before any multi-fragment coverage. Weight accumulators may be
int32.

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
