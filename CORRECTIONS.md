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

**C-006 — WU-05's stated accept criterion for chroma was unachievable as
written.**
*Claimed (`WORK-UNITS.md`):* "ramp and excursion patterns round-trip
bit-exactly through unpack → upsample → downsample → pack."
*Correct:* worked through with the actual coefficients ADR-020 froze at
WU-04 — not yet decided when this WU-05 line was drafted — a non-flat
chroma signal does not survive `chroma::upsampleImage` followed by
`chroma::downsampleImage` unchanged. Verified by direct computation (see
`tests/test_ramp_roundtrip.cpp`'s file header) for both the ramp and the
excursion pattern: deviations range from single-code rounding differences up
to several hundred codes at the excursion pattern's sharp transitions,
including intermediate sums that go negative before `Sample`'s
modulo-65536 narrowing. This is intended, not a bug: ADR-020's downsample
filter is a half-band low-pass, chosen as an anti-aliasing stage, not
designed as a perfect-reconstruction pair with the upsample filter. What
*is* exact, and is what this unit now actually tests: v210 file I/O in
isolation, for any pattern; the full chain for a flat chroma field, per
ADR-020; and luma through the full chain, always, since chroma resampling
never touches Y. `WORK-UNITS.md`'s WU-05 accept line is corrected to state
these three properties. No ADR is reopened — ADR-020's filters are
unchanged; this corrects a planning-time assumption in `WORK-UNITS.md`, not
a design decision in `DECISIONS.md`.

**C-007 — `AccumCell::w` (int32, I4) does not share `AccumCell::Y`/`Cb`/`Cr`
(int64)'s headroom at the same synthetic scale.**
*Claimed (implicitly, going into WU-09's worst-case test):* the "int64
headroom... at synthetic worst case" WORK-UNITS.md's WU-09 accept line asks
for could be checked by routing a million full-weight fragments (`Y = Cb =
Cr = 65535`, `w = kWeightMax = 65535`), all landing on one cell, through
the real splat path (`Frag` -> `accumulateCorner()` -> `AccumCell`).
*Correct:* doing so overflows `AccumCell::w` — signed integer overflow,
undefined behaviour — long before `AccumCell::Y`/`Cb`/`Cr` are anywhere
near their own limit. A full-weight fragment's colour contribution is
`kMaxFragContribution` (~4.29e9) but its weight contribution is only `w`
itself (up to 65535), so `AccumCell::w`'s int32 capacity (~2.147e9) is
exhausted at roughly 32768 such fragments landing in one cell, versus
`AccumCell::Y`'s int64 capacity (~9.22e18) only being exhausted at roughly
2.15 billion — a difference of exactly `kWeightMax + 1`, dimensionally,
since colour is weight times a 16-bit sample. `tests/test_splat.cpp`
splits the worst-case coverage in two instead:
`test_int64_headroom_full_pipeline()` drives 25000 full-weight fragments
through the real `splatTile()`/`splatTileReference()` path — safely under
`AccumCell::w`'s ~32768-fragment ceiling, while already far beyond what a
hypothetical 32-bit colour accumulator could hold — and
`test_int64_headroom_million_fragment_arithmetic()` checks the literal "a
million" claim `core/types.hpp`'s own `kMaxFragContribution` comment makes,
directly against `ColourAccum`'s arithmetic in isolation, not routed
through the shared `Frag`/weight code path. Not a defect in `splat.cpp` or
in I4 itself — I4 only ever claims int32 "may be" sufficient for weight,
never that it shares colour's synthetic million-fragment ceiling — but
worth recording so a future session does not assume `AccumCell::w` has
headroom it does not have. Under architecture.md 4.4's own bin-traffic
table, realistic per-cell fragment counts (order 1000 under 32:1
compression) stay nowhere near either ceiling; this only matters for
deliberately synthetic stress tests, or if some future unit changes that.

**C-008 — two distinct floating-point issues in `core/lattice.cpp`'s
Catmull-Rom evaluation, both surfaced for the first time by WU-10's full
pipeline (nothing earlier ran an affine map end to end through it).**

*(a) Edge-derivative damping.* *Claimed (implicitly, by every earlier unit
that used `Lattice::jacobian()`):* the analytic derivative `jacobian()`
returns is the true local slope of the surface `eval()` evaluates,
everywhere in `[0, kLatticeMax]`. *Correct:* true in the interior, but not
within lattice-parameter distance 1 of either edge. ADR-022's
edge-replication clamp (`core/lattice.cpp`'s `blend()`, `std::clamp(row,
0, kLatticeMax)` / `std::clamp(col, 0, kLatticeMax)`) substitutes a
duplicate of the edge control vertex for any stencil position that would
fall outside `[0, kLatticeMax]`. That is the right choice for `eval()`
itself (a duplicated point contributes the same *value* a true
extrapolation would approximate, at the edge). It is not neutral for
`jacobian()`: `basisDeriv(0) = {-0.5, 0, 0.5, 0}` and `basisDeriv(1) = {0,
-0.5, 0, 0.5}` both assign nonzero weight to the neighbour one cell beyond
the edge — exactly the position ADR-022 replaces with a duplicate — so the
derivative loses a term a true (unclamped) Catmull-Rom tangent would have
included, and picks up an extra copy of the edge point's own weight
instead. Measured directly (this session): up to 50% of the true affine
slope at the very edge parameter (`t = 0` or `t = 1`), easing off across
that one lattice cell, i.e. affecting only source pixels whose lattice
parameter falls within `[0, 1)` or `(kLatticeMax - 1, kLatticeMax]` —
which, since `core/binner.cpp`'s `pixelToLattice()` maps a source raster's
own first and last pixel to `u/v = 0` and `u/v = kLatticeMax` exactly,
always includes that raster's own first and last row/column, regardless of
raster size. A genuine, already-frozen (ADR-022) property of the lattice's
edge handling — not a defect WU-10 introduces, and not fixable from
`core/resolve.hpp`/`.cpp`/`core/pipeline.cpp` alone, since `core/lattice.cpp`
is outside this unit's file scope. Worked around within WU-10's own files:
`tests/test_zoneplate.cpp`'s I7 check uses source rasters no wider or
taller than `kLatticeSize` (129), so consecutive pixels always advance by
at least one full lattice cell and only the true edge pixel itself lands
in the damaged region, and its full-pipeline I5 check (which structurally
cannot avoid using a source raster's own first/last column — see that
test's own header comment) uses a coverage/hull check robust to the
weight distortion rather than an exact hand-derived value. Flagged in
`HANDOFF.md`'s open questions as a real limitation worth a future
`core/lattice.cpp` fix (a true extrapolated tangent at the edge, instead
of a duplicated point, for `jacobian()` specifically) rather than resolved
here.

*(b) Supersampling-threshold floating-point noise at exactly 1.0.*
*Claimed (implicitly, by WU-08's `chooseSupersample()`, architecture.md
4.6):* an affine map's pixel-space Jacobian determinant, for a map whose
true (real-number) determinant is exactly 1.0 — an identity map, WU-10's
own I7 check — evaluates to exactly `1.0` in IEEE 754 double precision
too, so `chooseSupersample()`'s bare `> 1.0` comparison never triggers.
*Correct:* the affine-reproduction identity a Catmull-Rom basis satisfies
(`Σ basis(t) * p_i` reproduces an affine function of the `p_i` exactly) is
an algebraic identity, not a bit-for-bit one; ordinary IEEE 754 rounding in
`core/lattice.cpp`'s bicubic evaluation lands the *computed* determinant on
either side of `1.0` — pixel by pixel, essentially at random — for an
identity map's interior (non-edge) pixels. Measured directly (this
session, `W = 64, H = 48`): 1448 of 3072 interior pixels' computed
determinant differed from exactly `1.0` at or below the 10th decimal
place, i.e. genuine floating-point noise below `1e-10`, not a real
determinant difference. `chooseSupersample()` is correct on its own terms
— a bare `>` comparison is exactly what 4.6 asks for — but that makes it
exactly as sensitive to this noise as any bare `>` against a value that is
not bit-exact always is: roughly half of an identity map's interior pixels
spuriously triggered 2x2 supersampling, which does not itself corrupt a
result (`generateFragments()`'s adaptive supersampling is still correct at
`n = 2`) but meant WU-10's own I7 full-pipeline check saw chroma values
computed via a different code path than the luma-exact identity case
assumes, intermittently, machine- and build-dependent. Fixed within WU-10's
own file scope: `core/resolve.hpp`'s `PipelineParams::supersample` now
defaults to WU-08's own ADR-024 threshold values (`threshold2x2 = 1.0`,
`threshold4x4 = 4.0`) plus `kSupersampleThresholdMargin = 1e-6`
(`defaultPipelineSupersample()`) — several orders of magnitude larger than
the measured noise (below `1e-10`) and far smaller than any determinant
difference a real magnifying warp would need treated differently for.
`core/binner.cpp`'s `chooseSupersample()` itself is unchanged; this is a
caller-supplied operating point, not a fix to 4.6's own comparison. Does
not reopen ADR-024.

**C-009 — the zone-plate anti-aliasing test's reference resample used a box
average, which is the wrong "high-quality offline reference" for what this
algorithm actually computes.**
*Claimed (`tests/test_zoneplate.cpp`, as first written this session):*
under `scale`:1 uniform compression, an independent box average over each
destination cell's back-projected `scale` x `scale` source rectangle is a
valid reference to check `runFrame()`'s own zone-plate output against, to
within a per-pixel tolerance.
*Correct:* architecture.md 4.5 fixes the splat's mechanism — every source
sample always spreads across exactly a 2x2 destination neighbourhood via a
bilinear (`fracX`/`fracY`) split, regardless of the local compression
ratio; anti-aliasing under heavy compression comes entirely from many
overlapping source samples' bilinear spreads summing together, not from
widening the splat footprint itself. That is, by construction, a
triangular ("tent") reconstruction filter of full width two destination
cells per axis, not a box filter of width `scale`. The two agree on total
energy (both are valid, energy-conserving low-pass filters) but not on
shape, and for a chirp signal built specifically to sweep through and past
the destination's Nyquist rate (the zone plate, by design — WU-03), the
two references diverge sharply: measured directly (this session, 4:1
compression, `srcSize = 128`), a box-average reference disagreed with the
actual (correct, non-aliased) pipeline output by up to 236 code values at
a single pixel and by 96.8 on average across the whole frame, while an
independently-implemented triangular-kernel reference (`Lattice::eval()`
for each candidate source sample's true warped position, weighted by a
hand-written hat function, entirely independent of `core/binner.cpp` and
`core/splat.cpp`) agreed with the same pipeline output to within 0.54 code
at 4:1 and 6.71 at 32:1 — both consistent with ordinary fixed-point
rounding, not a defect. `tests/test_zoneplate.cpp`'s `referenceCode()` now
computes the triangular-kernel reference instead of a box average, and its
two per-pixel tolerances are tightened accordingly (20.0/32.0 to 8.0/20.0)
now that the reference matches the algorithm's own intended filter shape
rather than a different, merely energy-equivalent one. No production code
changed — `core/binner.cpp`'s splat mechanism is architecture.md 4.5's own
design, frozen since WU-08/WU-09, and was never in question.

**C-010 — WU-10's full-pipeline I5 (no green fringe) test assumed a
uniform-colour source resolves to *exactly* its own colour wherever
coverage is complete, regardless of how per-fragment weight is
distributed.**
*Claimed (`tests/test_zoneplate.cpp`'s
`test_pipeline_partial_coverage_no_fringe()`, as first written this
session):* a flat, single-colour source, splatted and resolved through the
real pipeline anywhere its aggregate coverage reaches or exceeds full,
composites to exactly that source colour, checkable with `==`.
*Correct:* true of the real-number arithmetic architecture.md 4.8
specifies, but not of `core/splat.cpp`'s actual fixed-point one. ADR-025
(WU-09) already documents that the four-bank splat's `accumulateCorner()`
truncates each of a fragment's (up to four) corner contributions
separately — one right-shift per corner, not one divide after summing all
four — and does not conserve weight exactly across corners, a deliberate
trade-off for I6's determinism ("up to 3 parts in 256 can be lost" per
fragment, per corner). Summed across the many overlapping fragments a
translated flat source produces, this can leave a systematic bias of a
code value or two even at genuinely full coverage: measured directly (this
session, the same 64x64-into-128x64 offset-placement construction this
test already used), a consistent +1 on the resolved Y and Cr channels, +0
on Cb, for that construction's own colours — small, but enough to fail an
exact `==` check. Not a defect in `core/splat.cpp` (ADR-025 already calls
this out as intentional) or in `core/resolve.cpp` (`composite()`'s own
divide is separately, correctly rounded — see `tests/test_zoneplate.cpp`'s
`test_composite_partial_coverage_no_green_fringe()`, which exercises
`composite()` directly against a hand-built `AccumCell` and is unaffected,
since it never goes through `core/splat.cpp`'s per-corner path at all).
Fixed within WU-10's own test file: `test_pipeline_partial_coverage_no_fringe()`
now checks composited columns against a small tolerance
(`kRoundingMargin`, 4 codes — comfortably above the measured drift,
comfortably below the hundreds-of-codes separation between the test's own
source and background colours) instead of exact equality, and its
partial-coverage columns' hull check (already needed for C-008(a)'s
reasons, since those are the raster's own first/last source columns) is
widened by the same margin at each endpoint. Pure-background columns,
which involve no accumulation arithmetic at all, remain exact.
