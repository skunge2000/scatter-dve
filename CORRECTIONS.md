# Corrections

Append only. Errors already made during design, with the correction. Reading
this file is how a fresh session avoids repeating them. Discovering a new error
is a normal outcome and belongs here in the same session it is found.

---

**C-001 — int32 accumulators are insufficient.**
*Claimed:* int32 colour accumulators are "comfortable".
*Correct:* with 16-bit colour and weights normalised so unity is 32768, a single
fragment reaches 65535 × 65535 ≈ 4.29 × 10⁹, which is at the uint32 boundary
before any multi-fragment coverage is considered. int64 required. Would have
manifested as wrap-around sparkles on peak-white areas — precisely where
super-white content lives. Now I4.

**C-002 — the four-framestore rationale was mischaracterised.**
*Claimed:* Quantel needed four parallel framestores "because it couldn't cache".
*Correct:* US 4,563,703 states the reason explicitly — a single store cannot
perform four sequential read-modify-writes within one input pixel period at
reasonable speed. Four stores divide the required per-device access rate by
four, from roughly 9 ns to 37 ns per access at 13.5 MHz sampling. Aggregate
bandwidth is unchanged. The distinction matters because the serialisation
problem does not disappear when the accumulator becomes cache-resident, which
is why the four-bank split is retained. Now ADR-002.

**C-003 — the 4K Mini's two program outputs are not independent.**
*Claimed:* the second SDI output could carry a diagnostic coverage view while
the first carried the effect.
*Correct:* the spec lists "2 × program out, 1 × loop out", which almost
certainly means mirrored copies of one frame buffer. Unverified but not to be
designed around. Diagnostics go to the Mac display or the spare Monitor 3G.
Now ADR-011.

**C-004 — "pedestal" misapplied to digital black level.**
*Claimed:* the 64 offset in 10-bit component video described as a pedestal.
*Correct:* pedestal is analogue NTSC setup, 7.5 IRE above blanking, absent from
PAL and from all digital component representations. In BT.601/709 black and
blanking are the same level, black is simply the code assigned to black, and
the space below is footroom. Terminology note now in I3.

**C-005 — bin traffic figure stated twice as much as it is.**
*Claimed:* 3.3 GB/s each way at 1080p50.
*Correct:* 33.2 MB per frame at 50 fps is 1.66 GB/s write plus 1.66 GB/s read,
3.3 GB/s total. The conclusion — negligible against M1 Max bandwidth — is
unaffected, but the figure is now correct in `docs/architecture.md`.
