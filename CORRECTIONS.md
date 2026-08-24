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

**C-011 — a curved shape's front-facing point (WU-11's cylinder/sphere) is
not necessarily where coverage is densest; it is often where magnification
peaks instead.**
*Claimed (implicitly, this session's own first draft of
`tests/test_shapes.cpp`'s pipeline checks):* a shape's front-facing point
(cylinder's `theta == 0`, sphere's `phi == psi == 0`), being the most
face-on part of the surface to the camera, is a safe destination pixel to
assert solid, near-exact source-colour coverage at.
*Correct:* the front-facing point is exactly where `d(x)/d(theta)` (and
the sphere's equivalent gimbal-angle derivatives) is largest — `cos(0) ==
1`, the maximum of cosine over the shape's own angular range — so it is
where the local pixel-space Jacobian determinant is largest, i.e. where
*magnification*, not compression, peaks, for any shape whose angular span
is wide relative to the source raster's own pixel density. Under
magnification the four-bank splat's fixed 2x2 footprint (architecture.md
4.5) cannot widen to compensate — 4.6's own adaptive supersampling raises
fragment *count*, not footprint *size* — so the front-facing point can be
the sparsest-covered part of the frame, not the densest. Measured directly
this session: with a 64x64 source and a cylinder sized for illustration
(radius 100, `angleSpan` 0.8·pi, both centred in a 256x256 destination),
the front-facing destination pixel resolved within a few hundred codes of
pure background, not the source colour, while pixels nearer the shape's
angular extremes — where the surface turns away from the camera and
compression dominates instead — resolved almost exactly to source. Fixed
within this unit's own test file: `tests/test_shapes.cpp`'s pipeline
checks use a larger source raster (256) so compression dominates almost
everywhere in the shape's footprint instead, and search for whichever
destination pixel is empirically farthest from the background colour to
check for genuine full coverage, rather than assuming its location in
advance. Not a defect in any production code — `core/binner.cpp`'s
magnification/supersampling behaviour is architecture.md's own
already-frozen design (4.6), working exactly as specified; this only
corrects a mistaken assumption in this session's own test-writing, caught
before WU-11 shipped.

**C-012 — bit-exact (`==`) comparison of two independently-computed
floating-point expressions is not portable across compilers/platforms,
even when the algebra says they must be equal.**
*Claimed (`tests/test_shapes.cpp`'s first draft of
`test_sphere_reduces_to_cylinder_cross_section_at_zero_vertical_span()`,
same session as C-011):* since `cos(0) == 1.0` exactly in IEEE 754, and
multiplying any finite value by exactly `1.0` cannot introduce rounding,
`buildSphereLattice()`'s `x`/`z` with `angleSpanV == 0` should equal
`buildCylinderLattice()`'s `x`/`z` bit-for-bit, checked with `==` across
all 16641 control vertices — reasoning correct about that one
multiplication step in isolation, wrongly generalised to the *whole*
expression.
*Correct:* `./tools/close.sh 11`, run on the M1 Max with AppleClang, failed
at exactly one of those checks (one grid point out of 16641) —
`tests/test_shapes.cpp:230`, `c.x == s.x`. `core/shapes/cylinder.cpp`
computes `x` as `centerX + radius * sin(theta)` (one multiply, one add);
`core/shapes/sphere.cpp` computes the same value as `centerX + radius *
sin(phi) * cosPsi` (two multiplies, one add) — two different expressions
that are algebraically identical when `cosPsi == 1.0` but are not
guaranteed to be *computed* identically by every compiler on every
platform: fused-multiply-add contraction, operation reassociation, and
transcendental-function rounding can all legally differ between a
single-multiply and a double-multiply expression, or between architectures
with and without native FMA hardware, without violating IEEE 754 (a
compiler is free to contract `a + b*c` into one rounding via FMA, and not
required to make the same choice for an expression with an extra factor).
This session attempted to reproduce the exact mechanism in the Linux cloud
sandbox — Clang 18 on x86_64, forcing `-mfma -ffp-contract=fast` to
simulate ARM64's native FMA availability — and could not: all 129 sampled
points matched bit-for-bit there. The precise trigger on AppleClang/ARM64
is therefore not conclusively identified (candidates include FMA
contraction, operation reassociation under `-O3`, or a 1-ULP difference in
Apple's `libm` `sin`/`cos` versus glibc's for the specific angles this
test's constants happen to produce), and pinning it down further is not
necessary: the lesson generalises regardless of mechanism. Fixed within
this unit's own test file: the `x`/`z` comparison uses a tight relative
tolerance (`1e-12`, roughly 4000x a double's ~1-ULP relative noise floor of
~2e-16) instead of `==`. The `y` comparison in the same test stays exact —
`radius * sin(psi)` with `psi` identically `0.0` for every row involves no
rounding to differ on, on any platform, since multiplying by an exact zero
is exact regardless of contraction or reassociation; only the
non-trivial-angle terms needed loosening. Not a defect in
`core/shapes/cylinder.cpp` or `sphere.cpp` — both compute the geometrically
correct surface; this only corrects an unsafe assumption in this session's
own test assertion. **General lesson for future units:** when a test
compares two *differently-shaped* floating-point expressions for equality
(not literally the same expression evaluated twice), even where the
algebra guarantees they compute the same real number, use a tight relative
tolerance, not `==` — bit-exactness across compilers/platforms is only
safe to assert for values that provably involve no rounding at all (exact
zeros, exact powers of two, values read back without arithmetic), the same
distinction this correction's own fix draws between the `x`/`z` checks
(loosened) and the `y` check (left exact).

**C-013 — `bmdModePALp` ("576p25") failing `DoesSupportVideoMode` on the
real UltraStudio 4K Mini was attributed to this one device's driver, not to
576p25 not being a real broadcast signal in the first place.**
*Claimed (`DECISIONS.md` ADR-032/ADR-033, this session):* `bmdModePALp`
being unsupported was framed as "the UltraStudio 4K Mini's driver simply
does not offer `bmdModePALp`... in combination with `bmdFormat10BitYUV`" —
implying a different device, or a future driver, might offer it. *Correct:*
Steve's own domain knowledge, given directly in this session: 576p25 "is
not a valid HDMI, SDI or analogue output format" — full stop, not a
device-specific gap. Checked against a secondary source rather than taken
on faith alone: ITU-R BT.1358 does define a 576p25 raster on paper, and it
sees real use on DVD-Video and file-based delivery, but the progressive SD
format actual broadcast infrastructure and hardware (EDTV) deployed was
**576p50** (Australia's SBS/Seven Network, historically), not 25p; standard
SD transmission over SDI/HDMI/analogue is interlaced —
576i25 (50 fields/second) — full stop, which is exactly what
`bmdModePAL` provides and what this session's own second real-terminal run
confirmed working (`ctest`: `test_decklink_output` green, 5.34s). No code
change follows from this correction beyond what ADR-033 had already done —
`bmdModePAL` was already the fix — but the *reasoning* for why
`bmdModePALp` failed needed correcting so a future session does not retry
it against different hardware or a driver update expecting a different
result. **Also worth separating explicitly:** ADR-007's own "576i25 /
576p25" development-standard naming is about this project's *internal*
processing raster during Phase 1/2 (file-to-file, no hardware, no signal
format at all — just 25 progressive frames per second of pixel data) and
is unaffected by this correction; the confusion this session made was
carrying that internal, signal-format-free convention over into Phase 3's
literal choice of `BMDDisplayMode` as if "576p25" named an SDI mode the way
"576i25" genuinely does. Does not reopen ADR-032, ADR-033 or ADR-007 — the
decisions those entries freeze (looped single-frame playback's mechanism;
`bmdModePAL` as the working display mode; the internal-raster development
target) are all unchanged; this corrects the *stated reason* one of them
gave, per `CORRECTIONS.md`'s own purpose.

**C-014 — `DECISIONS.md` ADR-038's own runbook, for WU-15b's real
one-hour run, did not account for `tests/test_decklink_output.cpp`'s own
completion log line hardcoding its bounded-run duration as separate
literal text, independent of the actual `sleep_for(...)` value.**
*Claimed (implicitly, ADR-038, prior session):* hand-editing only the
`std::this_thread::sleep_for(...)` literal at line 168 is sufficient for a
WU-15b run's own reported evidence to honestly describe itself — nothing
else in the file needed to change for one temporary, reverted run.
*Correct:* `test_looped_playback_runs_with_no_dropped_or_late_frames()`'s
own closing `std::fprintf(stderr, ...)` call separately hardcodes the
phrase "over a 5-second bounded run" as literal text, not derived from the
actual sleep duration in any way. Steve's real WU-15b run, with the sleep
literal correctly edited to `seconds(3600)` per ADR-038, produced
`completed=89998 displayedLate=0 dropped=0 flushed=0` — arithmetically
consistent with a genuine ~3600-second run at `bmdModePAL`'s own 25fps
(3600 x 25 = 90000, within a couple of frames of ordinary preroll/
stop-boundary slop) and confirmed directly by Steve as the real hour-long
run, not assumed from the arithmetic alone — but the log line itself still
printed "over a 5-second bounded run," verbatim, since that text was never
part of ADR-038's own edit instructions. Not a defect in
`LoopedFramePlayback` or in the run's own real behaviour — the `stats()`
counts themselves are accurate and unaffected; only this one descriptive
string is wrong once the duration it describes is hand-edited elsewhere.
Worth recording so a future reader of this exact log line, or a future
session reusing ADR-038's own runbook for some other duration, does not
take the printed duration at face value. No code change follows: `tests/
test_decklink_output.cpp` is back to exactly its own `wu-15a-green`
content (ADR-038's own revert step already restores it), and the string is
only ever wrong during the one temporary, uncommitted state ADR-038's own
edit produces, never in anything committed. Does not reopen ADR-032,
ADR-033 or ADR-038 — this corrects a gap in ADR-038's own runbook
completeness, not a design decision any of those entries freeze.

**C-016 — this session told Steve `decklink_input.cpp`'s
`-Wsign-conversion` fix was applied and ready to rebuild while the fix
existed only in the assistant's own sandbox copy, never written to
`~/src/scatter-dve` on the Mac.**
*Claimed:* editing the file at the path the assistant had read it from
constituted fixing it — "the fix is in, rebuild" — with no separate step
to confirm the change had actually reached the real repository.
*Correct:* the assistant's own sandbox and `~/src/scatter-dve` are two
independent filesystems; a device-bridge write-back is a distinct,
fallible action, not an automatic consequence of editing a file the
assistant happened to read over that bridge earlier in the session.
Steve reran `cmake --build build` and hit the identical error at the
identical line, which is what surfaced this — the sandbox edit had simply
never left the sandbox. Only caught because Steve pasted the second build
log rather than taking "fixed" on trust. No claim about the SDK, the
project's own code, or any ADR is wrong here — this is purely a session
mechanics failure, logged because `SESSION-PROTOCOL.md`'s own discipline
("this is how a fresh session avoids repeating them") applies just as much
to how the assistant delivers a fix as to what the fix contains.
`SESSION-PROTOCOL.md` now states this explicitly (Session close, and
anti-drift rule 8): no file is "delivered" until written back via the
device bridge and re-read from the real repository to confirm, and no
session may tell Steve to rebuild before that confirmation happens. Does
not reopen any ADR — this is a process gap, not a design decision.

**C-015 — a decode()-by-colour-signature fragment-reassembly check (WU-16b's
own `tests/test_row_band.cpp`, first draft) assumed a source pixel's own
(px, py) signature identifies at most one `Frag` per tile, without
checking that assumption against 4.6's own supersampling.**
*Claimed (this session's own first draft of
`test_row_range_reassembles_with_more_bands_than_rows()`):* a magnifying
pixel-affine map (scale 2.0, chosen only so `numBands` (11) could exceed
`H` (5) at a source small enough to be a cheap test) was a safe choice for
a check built on `tests/test_binner.cpp`'s own `decode()` technique
(`Frag::Y`/`Cb` re-encoding a fragment's own source `(px, py)`).
*Correct:* under magnification, `chooseSupersample()` (`core/binner.cpp`,
4.6) subdivides a source pixel into up to 16 sub-samples, several of which
can land on the same integer destination cell — measured directly (this
session, the exact construction first drafted): multiple source pixels'
own signature keys appeared 4 or 16 times in a single tile. `decode()`'s
own signature identifies which *source pixel* a fragment came from, not
which *sub-sample*, so a magnifying map breaks the "at most one `Frag` per
signature per tile" assumption this test's own reassembly check depends
on — not a defect in `generateFragmentsRowRange()`, `generateFragments()`,
or the row-range reassembly property genuinely under test, and caught by
this session's own diagnostic run (a standalone reproduction outside the
test harness) before the test was relied on as evidence of anything.
`tests/test_binner.cpp`'s own decode()-based checks all use compressive
maps (det J < 1, so `chooseSupersample()` always returns 1) for exactly
this reason, a constraint this session's first draft of the second
row-band test did not carry over from it. Fixed within this unit's own
test file: `test_row_range_reassembles_with_more_bands_than_rows()` now
uses a compressive map (scale 0.5) instead of a magnifying one, matching
`tests/test_binner.cpp`'s own convention and this same file's other
row-range test; `generateFragmentsRowRange()` itself was never in
question, and the fix touches no production code. **General lesson for
future units:** a `decode()`-by-colour-signature technique for checking
`Frag` identity or uniqueness is only valid where `chooseSupersample()`
returns 1 everywhere (det J < 1, no subdivision) — a future unit reusing
this pattern under magnification needs a genuinely sub-sample-unique key
(e.g. also encoding `sx`/`sy`) or a different verification approach
entirely.

**C-017 — `DECISIONS.md` ADR-052 (WU-21f) claimed a rigid rotation of
`buildSphereLattice()`'s own control points could produce negative depth
(`z < 0`), and used that claim to justify a conservative rotation
amplitude, without checking the claim against the sphere invariant
`shapes.hpp` itself already documents.**
*Claimed:* "a large enough rotation can produce negative depth ... this
project's own binner/splat/resolve path has never been exercised against
negative depth before" — presented as a real, unresolved risk, and used to
justify bounding `kYawAmplitude`/`kPitchAmplitude` well short of a full
rotation.
*Correct:* every point `buildSphereLattice()` produces, for any
`angleSpanH`/`angleSpanV` including values far beyond a single small demo
patch, satisfies `(x-centerX)^2 + (y-centerY)^2 + (z-radius)^2 ==
radius^2` exactly — `shapes.hpp`'s own documented invariant (ADR-027),
unconditional on the angular range used to build it. `rotateLattice()`
(WU-21f's own test-file helper) rotates every point rigidly about
`(centerX, centerY, radius)` — the sphere's own true centre, the exact
same point the invariant above is centred on — and a rigid rotation about
a sphere's own centre preserves distance from that centre by definition.
So every rotated point is *still* exactly `radius` from `(centerX,
centerY, radius)`, for any rotation angle whatsoever, which alone forces
`(z - radius)^2 <= radius^2`, i.e. `z` in `[0, 2*radius]` — always, with
no dependence on rotation amplitude at all. Negative depth from this
specific rotation (about the sphere's own true centre) is not possible;
the earlier claim was an informal, unverified worry ("the patch sits
mostly in front of the pivot, rotating it might push part of it past
zero") that did not get checked against the actual invariant already
sitting in the codebase. Caught this session, while scoping WU-21g's own
request for a continuous (unbounded) rotation axis, when re-deriving the
bound properly turned out to remove the concern entirely rather than
requiring a larger bound. Does not affect WU-21f's own delivered code
(`CaptureConsumer::setLattice()`, the mutex design) — only the amplitude
reasoning in its own `DECISIONS.md`/`WORK-UNITS.md` prose was wrong, not
anything built. `DECISIONS.md` ADR-053 (WU-21g) carries the corrected
reasoning forward and removes the amplitude conservatism this mistake had
caused. **General lesson for future units:** when a geometric worry can be
checked against an invariant the codebase already states and proves
(`shapes.hpp`'s own documented sphere equation, `test_shapes.cpp`'s own
check of it), check it before writing the worry into a design record as
an open risk — an unverified "this might be unsafe" is not free to state
even when phrased honestly as unresolved, if five minutes of algebra
against already-available documentation would have settled it.

**C-018 — `DECISIONS.md` ADR-054 (WU-21h) claimed shift+cursor keys would
be recognised via the `ESC [ 1 ; 2 <letter>` xterm sequence "on both
[Terminal.app and iTerm2] by default," and shipped that as the only way to
reposition the sphere.**
*Claimed:* "A shift-modified cursor key arrives as `ESC [ 1 ; 2 <letter>`
on both of those specific terminals by default (the common xterm
'modifyOtherKeys' convention)" — stated as this session's best
understanding, explicitly flagged elsewhere in the same entry as
"genuinely terminal-dependent, not verified in this session," but still
shipped as the unit's own sole position-control mechanism rather than
alongside a non-terminal-dependent fallback.
*Correct:* Steve tried it at his own real terminal — shift+arrow did not
work. Which of Terminal.app's own settings, TERM value, or some other
cause is responsible was not diagnosed (Steve moved directly to proposing
a fix rather than debugging the sequence), and this session has no way to
attach a real terminal emulator to investigate further either. Fixed by
removing the terminal-dependent mechanism entirely rather than chasing the
exact sequence Steve's own setup actually sends: WU-21i replaces
shift+cursor (and, for one consistent scheme, `I`/`O` too) with six plain
letter keys, `X`/`x`/`Y`/`y`/`Z`/`z`, uppercase increments and lowercase
decrements — ordinary single-byte characters no terminal emulator
disagrees about, avoiding this whole class of problem rather than solving
it for one specific terminal. Plain (unshifted) cursor-key rotation, which
Steve confirmed did work, is untouched. **General lesson for future
units:** a flagged-as-unverified terminal behaviour is still a real claim
shipped as a unit's only mechanism for something — flagging the
uncertainty honestly (as ADR-054 did) is necessary but not sufficient;
where a simpler, universally-supported alternative exists (plain
characters here, instead of a modifier-key escape sequence), preferring it
from the start avoids needing this entry at all.

**C-019 — WU-28a's first draft of `splatTileKBuffer()` assumed that
`splatCorners()` only invokes its `sink` callback for a corner that
actually contributes weight, carried over unexamined from how the plain
path (`accumulateCorner()`) happens to tolerate the opposite.**
*Claimed (implicitly, this session's own first draft of
`core/splat.cpp`'s `splatTileKBuffer()`):* routing every `sink` call
straight into `routeIntoKBuffer()` — claim a free slot for a new tag, or
evict the farthest-so-far occupied one if none are free — was a correct,
direct translation of the tag-routing/eviction policy ADR-059 already
fixed, for any corner `splatCorners()` visits inside tile bounds.
*Correct:* `splatCorners()` (unchanged, shared with `splatTile()`) calls
`sink(bank, cellX, cellY, rawWeight)` for all four of a fragment's corners
that fall inside the tile, including any whose bilinear weight is exactly
zero — routine for a fragment sitting at an exact grid position (three of
its four corners get `rawWeight == 0`) and possible whenever a fragment's
fractional position is exactly 0 on one axis. `accumulateCorner()`
tolerates this silently in the plain path: multiplying by a `rawWeight` of
0 adds nothing to an `AccumCell` that may never be inspected again if
nothing else ever touches that cell. `routeIntoKBuffer()` does not have
that same safety margin — a zero-weight visit would still claim a free
slot (marking a cell "occupied" by a tag that made no real contribution to
it) or, worse, evict a genuinely-contributing tag's slot to make room for
one that contributes nothing. Caught by this session's own first `ctest`
run in the cloud sandbox, not by design review: `tests/
test_kbuffer_storage.cpp`'s `test_single_tag_matches_plain_sumBanks()`
failed two of its "no other cell touched" checks (the fragment's own
three zero-weight corners had each phantom-claimed a slot in a
neighbouring cell). Fixed within this unit's own file:
`splatTileKBuffer()`'s sink lambda now skips any corner with
`rawWeight == 0` before calling `routeIntoKBuffer()`. Not a defect in
`splatCorners()` or in `accumulateCorner()`/the plain path — both behave
exactly as WU-09/ADR-025 already established, correctly, for what the
plain path's own arithmetic needs. **General lesson for future units:** a
new consumer of an existing shared helper (`splatCorners()` here) needs
its own review of that helper's full contract, not just the part its
existing caller happens to rely on — a zero-weight callback invocation was
already part of `splatCorners()`'s documented behaviour (every in-bounds
corner, not just contributing ones), simply never load-bearing for any
caller before this one.

**C-020 — ADR-059's own scoping named "a sphere's own front and back" as
the k=2 minimal case WU-28 exists to fix, but neither WU-28a nor WU-28b
built the piece that gives front and back different `Frag::tag` values —
without it, the k-buffer they built cannot ever separate a self-fold at
all.**
*Claimed (implicitly, by omission — ADR-059 through ADR-061, and this
project's own `WORK-UNITS.md` `WU-28a`/`WU-28b` status lines before this
session):* that building WU-28a's storage and WU-28b's resolve was
sufficient to fix the visible symptom WU-21g/h's own full-360-degree
sphere wrap first put on screen (ADR-053) — Steve's own question, "should
WU-28b have shown occlusion," surfaced that this was never actually true.
*Correct:* the k-buffer keys its up to `kBufferK` slots *by `Frag::tag`*
(ADR-059's own deliberate choice, made to keep same-tag accumulation
order-independent for I6 — see `src/core/splat.cpp`'s `routeIntoKBuffer()`,
line ~182: same tag reuses the same slot). But `Frag::tag` is a single
scalar per `generateFragments()`/`runFrame()` call — `core/binner.cpp`'s
own fragment-generation loop does `frag.tag = tag;` for every fragment it
emits in that call, from one `std::uint8_t tag` function parameter sourced
from `PipelineParams::tag`. A single self-folding lattice, generated in one
call (exactly `tests/test_decklink_live_sphere.cpp`'s own
`makeSphereLattice()`, `angleSpanH == 2*pi`), therefore stamps the *same*
tag onto every fragment regardless of whether it comes from the surface's
near side or its folded-away far side. Front and back always land in the
same k-buffer slot and get summed together by the same
order-independent `accumulateCorner()` arithmetic the plain path already
uses — Opaque and Blend resolve modes both degenerate to numerically the
same output `composite()` already produced before WU-28 existed, for this
exact content. Separately (compounding, not the root cause):
`tests/test_decklink_live_sphere.cpp` never sets `PipelineParams::kBufferMode`
away from its default `Off` either, so today the k-buffer code path is not
even reached by that file — but fixing that alone would not produce
visible occlusion, per the tag-collision issue above. Would have manifested
(had the wiring gap alone been fixed without this one) as `Opaque`/`Blend`
modes silently producing pixel-identical output to the plain path on real
self-folding content, with no test in `tests/test_kbuffer_resolve.cpp`
positioned to catch it — that unit's own accept criterion for real content
was I6 (thread-count independence) only, deliberately scoped away from
multi-tag correctness against real geometry (WORK-UNITS.md's own WU-28b
entry, "Parts A/B already cover the multi-slot resolve arithmetic
directly" against synthetic data) — a reasonable call for *that* unit's own
job, but one that left this gap with nothing positioned to surface it until
Steve looked at the actual screen. Now WU-28c/WU-28d
(`WORK-UNITS.md`), gated on WU-26 (Normals from lattice) — see
`DECISIONS.md` ADR-062 for the full scoping and why the fix depends on
WU-26 specifically rather than a same-unit shortcut. **General lesson for
future units:** an ADR naming a concrete real-world symptom as a unit's own
motivating case (here, "a sphere's own front and back" made visible by
WU-21g/h) is a claim that the finished feature will fix *that exact,
observable thing* — worth a session checking against the real symptom
before calling the motivating case closed, not just against the synthetic
test data the unit's own `Accept:` line happened to specify.

**C-021 — WU-26 close-out: `wu-26-green` was created before the delivered
files were committed, so the tag initially pointed at a commit lacking
WU-26's own changes entirely.** Session 37 delivered WU-26's seven changed
files to the real repository via the device bridge, leaving them as
uncommitted working-tree changes (writing a file to disk is not committing
it), then walked Steve through the manual `git tag -a wu-26-green ...`
fallback (triggered by the already-accepted `test_decklink_device`/ADR-035
duplex exception) without an intervening `git add`/`git commit` step. A `git
tag` command tags whatever commit `HEAD` currently is, oblivious to
working-tree state — it does not check for or refuse uncommitted changes,
unlike `close.sh`, which explicitly does (`git status --porcelain` non-empty
→ "ERROR: uncommitted changes. Commit them first", refuses to proceed).
WU-28b's own close-out earlier in this same session was unaffected by this
same mistake only because its own code had already been committed in a
prior session — Steve's working tree was genuinely clean when that tag was
created, confirmed directly beforehand. **General lesson: any close-out
instructions using the manual-tag fallback (not `close.sh` itself) must
include an explicit `git add`/`git commit` step before the `git tag`
step, spelled out in the same command block** — `close.sh`'s own built-in
uncommitted-changes check is not present on that path, and nothing else
catches its absence; `git push origin main` reporting "Everything
up-to-date" immediately after such a tag is a symptom worth recognising on
sight, not routine output. Fixed the same session: `wu-26-green` deleted
locally and on `origin`, the seven files committed for real, `wu-26-green`
recreated on the commit that actually contains them, both pushed — see
`HANDOFF.md`.

**C-022 — `docs/architecture.md`'s Phase 7/lighting text named the wrong
model and the wrong evaluation granularity; no wrong patent citation was
actually present to fix.**
*Claimed:* `docs/architecture.md` line 20 ("Per-pixel lighting and shading
(the 'Starlight' equivalent) in a later phase") and line 464 ("Phase 7 —
Starlight. Normals from the lattice, Blinn-Phong with 4–8 lights..."). The
continuation prompt that opened WU-32 additionally asserted this text
"scopes Starlight from the wrong patent" (EP0320166A1).
*Correct:* checked directly against the real repository (`docs/
gpu-route-assessment.md` does not exist; the relevant text lives in
`docs/architecture.md` §4.7/§10/§13) before writing anything. Two real
errors: the model is **Phong**, not Blinn-Phong — S5 (US 5,103,217) gives
`cos B` between the line of sight and the reflected ray, the original Phong
formulation, not Blinn's half-vector one (ADR-069). And shading is
*evaluated* per coarse-grid facet and *interpolated*, not evaluated
per pixel (ADR-070) — line 20's "per-pixel lighting and shading" overstates
what the intensity factor's per-pixel *application* actually means. But the
"wrong patent" half of the claimed correction does not hold: `docs/
architecture.md` §13's own provenance section already cites **US 5,103,217
(Cawley)** as "the closest published document to the Starlight lighting
option" and never cites EP0320166A1 anywhere in this repository — grepped
directly, no match. The EP0320166A1 misattribution documented in
`docs/sources/WU-SM-01.md` §2.1 describes an *earlier* scatter-dve working
assumption that this repository's current `docs/architecture.md` text does
not actually carry. What *is* missing from §13 is the real Starlight patent
identified by this research — **EP 0248626 / US 4,899,295 (Nonweiler,
priority 3 June 1986)** — not yet obtained, added as a citation rather than
a fix. `docs/architecture.md` lines 20 and 464–465 corrected this session;
§13 gained the S6 citation. See ADR-069/ADR-070.

**C-023 — the 1080p50 performance headroom conclusion (ADR-007) predates
findings that push in both directions and cannot be inherited unexamined.**
*Claimed:* ADR-007's "10.4 Mpx/s versus 103.7 for 1080p50, a factor of ten
of headroom" and `docs/architecture.md` §11's per-stage cycle budget (splat
60 cycles/px, normalise+composite 30 cycles/px) as grounds for confidence
that 1080p50 is reachable.
*Correct:* not wrong as stated at the time — this is not a computational
error the way C-005 was — but its scope has since changed under it. I8
(nothing culled, back faces always splat) means destination contention
roughly doubles wherever two sheets overlap, which for a closed shape is
most of its area; ADR-072/ADR-074's future arbitration stage adds a wider
read-modify-write per splat (colour, weight, and now depth/tag-slot state)
where WU-28's k-buffer is in use; against that, ADR-070's coarse-grid
shading is *cheaper* than the per-pixel evaluation the original budget
implicitly assumed, since a facet's `I` is computed once and interpolated
rather than recomputed per pixel. Net effect on the 1080p50 conclusion is
genuinely unknown — the pushes do not obviously cancel, and re-deriving the
budget was flagged as its own task in
`docs/sources/WU-SM-02.md` §5.2 (Task D6). `ADR-007` is not reopened and
`docs/architecture.md` §11's own budget table is not rewritten here, since
this correction has no computed replacement figure to offer yet — only the
recorded fact that the old one should not be cited as settled until D6 is
done. Whoever picks up WU-34 (or a dedicated performance-re-derivation unit)
should re-run the budget against I8/ADR-070/ADR-072 before quoting a
1080p50 conclusion either way.

**C-024 — this session's own WU-23a close-out instructions told Steve to
run `tools/close.sh`, expecting the ADR-035 duplex-check exception
"should not come up" because the unit itself has no DeckLink-linked test.**
*Claimed:* `HANDOFF.md`'s original Session-43 "Steve's own next steps"
said to run `./tools/close.sh 23a` and that the already-accepted
`test_decklink_device`/ADR-035 duplex exception "should not come up" for
this unit, since `test_interlace` itself has no DeckLink dependency.
*Correct:* `ctest --test-dir build` runs every registered test whenever
the DeckLink SDK is configured, not only the unit's own new test —
`test_decklink_device`'s `test_at_least_one_device_is_full_duplex` check
runs on every close-out regardless of which unit is being closed, and per
ADR-034/ADR-035 it now fails structurally, not just temporarily: the going-
forward hardware is a Monitor 3G and a Recorder 3G, two separate devices,
neither full duplex, and ADR-034 already names this check as failing "not
even once the 4K Mini's PSU is sorted." `close.sh` treats any test failure
as blocking and refuses to tag, so it does not succeed for essentially any
close-out while this check exists and fails — not only for units that
themselves touch DeckLink code, which is what this session's own
(wrong) reasoning assumed. Corrected in `HANDOFF.md`'s own Session-43
close-out instructions: skip `close.sh`, build and test manually, tag by
hand once nothing *other* than this named exception fails, and push
explicitly (`git push origin main` / `git push origin --tags`) — the
manual-tag path does not push on its own, `SESSION-PROTOCOL.md`'s own
anti-drift rule 9.

**C-025 — ADR-078's own description of `video::Deinterlacer`'s input shape
("fed one field-native Raster444 at a time... one field parity's own
temporal sequence per instance") does not match what `libavfilter/vf_w3fdif.c`
actually reads, and could not implement the real algorithm as stated.**
*Claimed (`DECISIONS.md` ADR-078, `WORK-UNITS.md`'s WU-23b1 entry):*
`video::Deinterlacer` is fed one already-field-split, half-height Raster444
per call — the caller's own choice of a single parity's temporal sequence,
"the direct analogue of FFmpeg's own single input stream" — and reconstructs
the missing rows from that one stream's own history plus "whichever of
prev/next is the correct adjacent field for that parity."
*Correct:* re-fetched and re-read `libavfilter/vf_w3fdif.c` directly this
session (`deinterlace_plane_slice()`, lines ~361-467) rather than trusting
ADR-078's paraphrase, per `SESSION-PROTOCOL.md` rule 6. Two things ADR-078
got wrong:

1. **`cur` and `adj` are full-height, both-parities-present ("weave")
   frames, not field-native ones.** The low-pass taps read `cur_data` at
   *anchor*-parity row offsets only (`in_lines_cur[j] = cur_data + y_in *
   stride`, `y_in` always same parity as the *other* field per the ±odd
   offset arithmetic) — that much is a real single-field read. But the
   high-pass taps read **both** `cur_data` and `adj_data` at *missing*-parity
   row offsets (`in_lines_cur[j] = cur_data + y_in*stride; in_lines_adj[j] =
   adj_data + y_in*stride`, then `filter_simple_high`/`filter_complex_high`
   sum both into `work_line`) — i.e. `cur`'s own frame is read a *second*
   time, at the *other* parity's rows, supplying one of the filter's three
   real field inputs itself. A field-native, single-parity `cur` (as
   ADR-078 described) has no missing-parity rows to read at all; the real
   algorithm is not expressible against one. Confirmed independently by the
   filter's own name: three distinct field acquisitions combine per output
   — the anchor field and the missing field, both from `cur`'s own weave
   frame, plus the missing field's own temporal neighbour from `adj` — not
   three frame-level history slots each contributing one field. `cur`/`adj`
   must therefore be full-height frames, video/interlace.hpp's own
   "full-height interlaced frame" shape (`extractField()`'s own precondition
   vocabulary), the same shape WU-23a's `extractField()` already consumes —
   not its half-height output.
2. **In frame-rate mode, `adj` is unconditionally `prev`, never `next`.**
   `adj = s->field ? s->next : s->prev`, and `s->field` only toggles inside
   `filter()` when `s->mode` (field-rate mode) is set; frame mode
   (`s->mode == 0`, the mode WU-23b needs — ADR-078 correctly picked this
   part) calls `filter()` once per input frame with `s->field` fixed at its
   initial `0`, so `adj` is always `prev`. Traced the actual `prev`/`cur`/
   `next` shift in `filter_frame()` against three pushed frames directly
   (not inferred): pushing frame 0 produces no output (`prev` still null);
   pushing frame 1 produces the *first* output, `cur` = frame 0, `adj` =
   `prev` = a *clone of frame 0 itself* (FFmpeg's own stream-start
   convention: `s->cur = av_frame_clone(s->next)` when `cur` was null,
   giving frame 0 its own duplicate as a startup `prev`); pushing frame 2
   produces `cur` = frame 1, `adj` = `prev` = frame 0 (now a real, distinct
   previous frame). `next` is set on every push (needed so the shift has
   something to promote into `cur` on the *following* call) but is never
   read by any single call's own reconstruction math in frame-rate mode —
   confirmed by `filter_simple_high`/`filter_complex_high`'s own parameter
   list (`in_lines_cur`, `in_lines_adj` only).

Not a coefficient or edge-handling error — those parts of ADR-078 (values,
scale, reflect-by-±2, uniform plane treatment, frame-rate-mode choice) all
checked out exactly against the real source, re-confirmed this session.
This corrects the *data-flow* half of ADR-078's design only. `DECISIONS.md`
ADR-079 (this session) carries the corrected design forward: `push()` takes
a full-height weave `Raster444` (matching `extractField()`'s own input
shape); internally holds `prev`/`cur`/`next` (three `std::optional<Raster444>`
slots, shifted the same way `filter_frame()` shifts, including the
duplicate-first-frame startup convention above — this reproduces
`WORK-UNITS.md`'s own already-frozen push-count/output-count Accept line,
"first field pushed produces no output; the second produces one
reconstructed frame," exactly, and is not itself in question); the
reconstruction reads `cur`+`prev` only, `next` existing solely to be shifted
into `cur` on the following call. `WORK-UNITS.md`'s WU-23b1 entry is edited
this session to describe the corrected shape directly (that file is
"edited as scope firms up," not append-only, per `SESSION-PROTOCOL.md`'s own
table) — its `Accept:` line's own "prev/cur/next... hold the correct three
frames at each step" wording is unaffected by this correction and is
honoured by the corrected design exactly as written, verified indirectly
through output content (`next`'s own identity is checked by confirming it
becomes `cur` — and therefore the next call's own anchor-row output — on the
following push, since it is never read directly into any single call's own
result). Does not reopen `INVARIANTS.md` or any earlier ADR; ADR-078's own
source-confirmation and multi-frame-history findings (nothing persists
outside this new component) are unaffected. **General lesson for future
units:** ADR-078's own error came from reading FFmpeg's coefficient tables
and edge-handling loop closely but reasoning about the surrounding
`prev`/`cur`/`next` plumbing from the algorithm's name and a paraphrase
rather than tracing the actual pointer/array arguments each filter function
receives — the same class of gap rule 6 exists to catch, but this time in
a *design* session's own findings, not a *build* session's recall of them;
confirming "which buffer does this parameter actually point into" line by
line is the only way to catch a wiring error like this, prose summaries
(including this project's own ADRs) are not a substitute for it.

**C-026 — this session's own close-out `git add` block named
`video/deinterlace.hpp`/`.cpp`, missing the `src/` prefix every other
reference to these two new files in the same session (including this
session's own real `git status --short` output, captured directly)
already had right.**
*Claimed (`HANDOFF.md`'s original "Steve's own next steps" block, and
this session's own chat message restating it):* `git add
video/deinterlace.hpp video/deinterlace.cpp ...`.
*Correct:* the files live at `src/video/deinterlace.hpp` and
`src/video/deinterlace.cpp` — confirmed by this same session's own `git
status --short` (`?? src/video/deinterlace.cpp`, `??
src/video/deinterlace.hpp`, captured and reported earlier in the same
session's own close-out) and by every other path reference in
`CMakeLists.txt`, `DECISIONS.md` ADR-079 and this file's own C-025 entry,
all of which say `src/video/...` correctly. The close-out block's own
`git add` line alone dropped the prefix — an inconsistency within the
same session's own output, not a stale assumption carried from an
earlier one. Steve ran the command as given and hit `fatal: pathspec
'video/deinterlace.hpp' did not match any files` immediately, which is
what surfaced it — no build or code claim was wrong, only this one
command line. Fixed within the same turn: corrected `git add` line given
directly in chat and written into `HANDOFF.md`'s own close-out block on
the real repository via the device bridge. **General lesson for future
sessions, extending `SESSION-PROTOCOL.md`'s own anti-drift rule 9 and
C-016's lesson about sandbox-vs-real-repository claims:** "double-check
every file path against a real `git status --short`" means the exact
string that goes into the close-out command block, character for
character, checked once more immediately before sending it — not merely
having looked at the right `git status --short` output earlier in the
same session and trusting memory or a paraphrase of it when typing the
final command out. A session can hold the correct information and still
hand over the wrong command if the two are never diffed against each
other directly.

**C-027 — WU-23b2's own prior scoping stub (`WORK-UNITS.md`, written
under ADR-078) described `CaptureConsumer::processOne()` as a plausible
place to call `Deinterlacer::push()` directly, and framed the question
of whether a third file was needed as depending on the output-side
decimate's own complexity -- both wrong once the real call sites were
read.**
*Claimed (`WORK-UNITS.md`'s WU-23b2 entry, `DECISIONS.md` ADR-078):*
"Wires a `video::Deinterlacer` instance into `io/decklink_capture_consumer.cpp`
(new owned member, `processOne()` calls it ahead of the existing warp)
... likely also touching `core/resolve.hpp`/`core/pipeline.cpp` for a new
orchestration entry point if the decimate is more than a couple of
inline calls inside `processOne()` itself" -- presenting the decimate's
own complexity as the thing that would decide whether a third file was
needed.
*Correct:* read `io/decklink_capture_consumer.cpp`'s `processOne()` and
`core/pipeline.cpp`'s `runFrameBytes()` directly this session.
`processOne()` does not call into the warp pipeline in stages at all --
it calls `scatter::runFrameBytes()` exactly once, a single monolithic
function performing v210 unpack, chroma upsample into a local
`Raster444`, `runFrame()`, chroma downsample and v210 pack entirely
inside its own body (`core/pipeline.cpp`, lines ~630-680). The
intermediate chroma-upsampled weave `Raster444` -- exactly the shape
`video::Deinterlacer::push()` requires -- is a local variable of
`runFrameBytes()` itself, never returned to or reachable from any
caller. So `processOne()` has no "ahead of the existing warp" point to
insert a `push()` call at, regardless of how simple or complex the
output-side decimate turns out to be -- a new `core/resolve.hpp`/
`core/pipeline.cpp` orchestration entry point (`DECISIONS.md` ADR-080,
`runFrameBytesDeinterlaced()`) is required for a more basic reason than
the one the stub anticipated: not because the decimate is complex (it
turns out to be a provable no-op for this project's own frame-rate-only
mode, ADR-080), but because no caller of `runFrameBytes()` has ever had
access to its own internal unpacked frame. Not a defect in
`runFrameBytes()` -- WU-21a/ADR-048 wrote it before any caller needed
that access, and folding all five stages into one call was the right
choice for every caller before this one. This corrects a planning-time
assumption in `WORK-UNITS.md` about *where* the wiring's own complexity
would come from, not a design decision any earlier ADR froze. See
`DECISIONS.md` ADR-080 for the corrected design and the file split it
produces (WU-23b2a/WU-23b2b).

**C-028 — WU-23b2b's own `Files:` line (`WORK-UNITS.md`, ADR-080) named
only `tests/test_decklink_capture_consumer.cpp` as needing an edit for
`CaptureConsumer`'s new required `DeinterlaceCoefficients` constructor
parameter; two more real call sites existed and were missed until
Steve's own real-terminal build caught them.**

*Claimed (`WORK-UNITS.md`'s WU-23b2b entry, both before and after this
session's own edit):* `io/decklink_capture_consumer.hpp`/`.cpp` edited,
plus `tests/test_decklink_capture_consumer.cpp`, "not counted against
the cap." No other file named as needing a change for the new
constructor parameter.

*Correct:* `CaptureConsumer` is also constructed directly in
`tests/test_decklink_live_output.cpp` (WU-21c) and
`tests/test_decklink_live_sphere.cpp` (WU-21i/WU-22c) — real,
independent demo/smoke-test entry points, not part of
`test_decklink_capture_consumer.cpp`'s own translation unit. Both broke
at Steve's own real-terminal `cmake --build`:
`test_decklink_live_output.cpp:136` (old 3-argument call, now missing
the required `coeffs` parameter) and `test_decklink_live_sphere.cpp:565`
(old 4-argument call with `coverageCallback` positioned where `coeffs`
now belongs, so the compiler saw a `std::function` where a
`DeinterlaceCoefficients` was expected). This session's own search for
"other callers" before writing `HANDOFF.md` never happened at all — the
close-out reasoned only from `WORK-UNITS.md`'s own `Files:` line, which
this project's convention (`SESSION-PROTOCOL.md`) treats as authoritative
for what a unit touches, without separately grepping the repository for
every real construction site of a class whose constructor signature was
about to change. Fixed the same session, once Steve reported the build
error: both files updated to pass
`scatter::video::DeinterlaceCoefficients::Complex` explicitly (matching
ADR-081's own choice for every caller in this project), each verified
written back to the real repository and checksum-confirmed byte for
byte, same as every other file this session touched.

**General lesson for future sessions, extending C-027's own "read the
real call sites, don't trust a scoping stub's account of them" lesson
one level further:** when a work unit changes a *public* function or
constructor signature (not just its body), `grep`-ing the whole
repository for every real call site is required before writing the
close-out — a `Files:` line written during an earlier *scoping* session
(here, WU-23b2's ADR-080, written before `io/decklink_capture_consumer.hpp`'s
full set of callers was ever enumerated) is a plan, not a fact already
checked against the real tree, and this project already has several
independent demo/smoke-test files that construct the same DeckLink
classes directly outside their own "official" test file
(`test_decklink_live_output.cpp`, `test_decklink_live_sphere.cpp`) — a
pattern worth checking for by name on any future signature change to a
class either of them touches.

**C-029 — Fixing C-028's build break (adding the required `coeffs`
argument to `test_decklink_live_output.cpp`/`test_decklink_live_sphere.cpp`)
was incomplete: both files also carry their own copy of the
`framesProcessed + framesFailed == framesPopped` accounting invariant,
now stale, and Steve's own real-terminal `ctest` run caught it failing
on both.**

*Claimed (this session's own C-028 fix, and the header comment it added
to both files):* "Otherwise unaffected: this test never reads
`CaptureConsumerStats::framesStreamStart`" — i.e. fixing the constructor
call was the whole fix.

*Correct:* both files independently re-implement the same
`framesProcessed + framesFailed == framesPopped` `CHECK` that
`tests/test_decklink_capture_consumer.cpp` already had (and that this
session's own WU-23b2b work correctly widened to a third term there) --
`test_decklink_live_output.cpp:179` and
`test_decklink_live_sphere.cpp:710`. Both are genuinely exercised: `run()`
now counts a `ProcessResult::StreamStart` result against the new
`framesStreamStart` counter instead of `framesProcessed`, for the very
first successfully-decoded frame of *any* `CaptureConsumer` instance,
including the ones these two files construct -- not something specific
to `test_decklink_capture_consumer.cpp`'s own `CaptureConsumer`. Steve's
own real ctest run showed exactly this: `test_decklink_live_output`
(`framesPopped=82`, `framesProcessed=81`, `framesFailed=0` -- the missing
1 is the stream-start frame) and `test_decklink_live_sphere`
(`framesPopped=420`, `framesProcessed=419`, `framesFailed=0`, same
missing 1). Fixed the same session: both `CHECK`s widened to the same
three-term form `test_decklink_capture_consumer.cpp` already uses, both
diagnostic `fprintf` lines extended to print `framesStreamStart`, and
both files' own C-028-era header comment (which asserted the false "this
test never reads `framesStreamStart`" claim) corrected in place rather
than left standing next to code that now contradicts it.

**General lesson, sharpening C-028's own lesson rather than repeating
it:** C-028 already established "grep the whole repository for every
real call site before closing out a session that changes a public
signature" -- true, but insufficient on its own, because it only finds
sites that fail to *compile*. A behavioural change to what a class's
public state means (here: a state transition that used to be impossible
now legitimately happens, moving population from one counter to a new
one) can silently invalidate a caller's own *runtime* invariant even
when that caller's call site still compiles cleanly against the new
signature once the argument list is fixed. The correct check after a
class's behavioural contract changes is not "does every call site still
compile" but "does every reader of this class's own observable state
(here, `CaptureConsumerStats`) still hold correct assumptions about it"
-- broader than a grep for the constructor name alone, and this session
still did not do it for the first fix. Worth a repository-wide grep for
every touched struct/counter name, not just the changed function's own
name, on any future session that adds a new outcome to an existing
state machine.
