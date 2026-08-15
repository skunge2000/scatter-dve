# Decisions
Append only. Numbered. A decision is settled unless explicitly superseded by a
later numbered entry. Do not reopen; propose a superseding ADR instead.

---

**ADR-001 — Forward scatter, not inverse gather.**
Every contemporary DVE and every GPU does inverse mapping: for each output
pixel, sample the source. Mirage does the opposite, per US 4,563,703. Forward
scatter is chosen because it is the only formulation that handles surfaces which
fold, tear, self-intersect or shatter, and because accumulation transparency and
the signature granulation and explosion effects fall out of it naturally.
Costs accepted: random writes, magnification holes, a normalisation pass.

**ADR-002 — Tile binning for bandwidth; four-bank split retained for
serialisation.**
These are distinct problems and only the first is solved by caching. Tile
binning converts scattered writes into streaming and keeps the accumulator in
cache. Quantel's four-bank decomposition is retained because a single
accumulator still requires four sequential read-modify-writes per fragment,
serialising on store-to-load latency; four banks give four independent chains
that pipeline. Fixed address offsets on write, common addressing and summation
on read, exactly as the patent.

**ADR-003 — Integer accumulators, int64 for colour.**
Chosen for determinism first and headroom second. See I4, I6.

**ADR-004 — No legalisation. Clamp only to v210 protocol limits.**
See I2.

**ADR-005 — 4:2:2 v210 only for I/O; 4:4:4 internally.**
4:4:4 SDI is not used in practice, so it is out of scope as a transport.
Internally, chroma is upsampled to 4:4:4 once at the front and downsampled once
at the back. Warping 4:2:2 directly is rejected: scatter computes an
independent destination per sample, so chroma lands at non-integer positions
relative to luma and reconstruction produces colour fringing that follows the
geometry. Output chroma must be low-passed before decimation because the warp
synthesises high-frequency chroma the source never had.

**ADR-006 — Host is the M1 Max MacBook Pro with UltraStudio 4K Mini. CPU
first; Metal deferred.**
The 4K Mini is Thunderbolt 3, full duplex, one clock, with composite and
component analogue inputs and 12G headroom. The M1 Max offers 8 performance
cores and roughly 400 GB/s memory bandwidth, which removes the binning
bandwidth concern entirely. Metal compute with imageblocks and integer atomics
maps onto this design almost unchanged and is the fallback if the splat
overruns budget at 1080p50 — not before.

**ADR-007 — Development starts at 576i25 / 576p25.**
10.4 Mpx/s versus 103.7 for 1080p50, a factor of ten of headroom, and it is the
standard Mirage actually worked in, enabling direct comparison with archive
footage.

**ADR-008 — `src/core/` and `src/video/` carry no platform dependencies.**
*Superseded by ADR-013.* Original rationale was an independent Linux
correctness oracle on a second machine. The module split is retained; the
rationale is not.

**ADR-009 — Phase 1 transparency is pure weighted accumulation. k-buffer
deferred.**
Accumulation is authentic — it is the patent's default behaviour — and requires
no depth buffer, no sorting and no per-pixel layer storage. The k-buffer is a
quality refinement for correct layered compositing and comes after the pipeline
works end to end.

**ADR-010 — Genlock is out of scope for the proof of concept.**
Free-running. Input timing derives from the source, output from the device
clock, so an unlocked source drifts against the output; the drift is logged and
tolerated. The 4K Mini has a sync input, so one BNC of black burst resolves it
whenever it matters. Playing from file there is nothing to drift against.

**ADR-011 — Diagnostic coverage view goes to the Mac display or the spare
UltraStudio Monitor 3G, not a second SDI output.**
The 4K Mini's "2 × program out" are near-certainly mirrored copies of one frame
buffer rather than independent channels. Supersedes an earlier assumption that
they could carry independent signals. See C-003.

**ADR-012 — Session-bounded development with repository-held state.**
See `SESSION-PROTOCOL.md`. The repository is the only authoritative record; the
assistant's recall is not.

**ADR-013 — Single machine. Supersedes ADR-008.**
Development, testing and hardware runs all happen on the M1 Max MacBook Pro
with the UltraStudio 4K Mini attached. No second build host, no cross-machine
verification, no remote git host required. The `src/core/` and `src/video/`
split is retained, but for a simpler reason than ADR-008 gave: those modules
link no Blackmagic SDK, so the bulk of the test suite runs with no hardware
connected and no driver installed.

**ADR-014 — Trunk-only git. No per-work-unit branches.**
Solo developer, one machine. Work commits directly to `main`; a green work unit
is marked with an annotated tag `wu-NN-green`. Abandoning a bad session is
`git reset --hard <last green tag>`, which is simpler than branch bookkeeping
and achieves the same thing.

**ADR-015 — Determinism oracle is the single-threaded build, not a second
machine.**
I6 survives ADR-013 intact and matters more without a second platform to diff
against. The reference is `--threads 1`: any multi-threaded run must produce
byte-identical output to it on the same input. This is checked in-suite rather
than by hand.

**ADR-016 — Hand-rolled test harness, not `assert()` and not a framework.**
Release builds define `NDEBUG`, which compiles `assert()` to nothing, so an
assert-based suite passes silently in exactly the configuration that ships.
`tests/harness.hpp` provides `CHECK` and `CHECK_ONCE`, always evaluated,
counted, and reported with a non-zero exit code. No external dependency:
nothing needs one yet and a dependency is a thing that breaks between sessions.

**ADR-017 — Warnings are errors, including `-Wconversion` and
`-Wsign-conversion`.**
The colour path is full of narrowing between 10-bit codes, 16-bit samples,
32-bit weights and 64-bit accumulators. An implicit conversion there is
usually an invariant being bent, so the compiler is configured to refuse it.
Verified achievable at WU-01: the full suite builds clean under this set.

**ADR-018 — v210 short-group padding is black, and deterministic.**
Widths that are not a multiple of 6 leave unused components in the final
16-byte group. These are packed as `kCode10Black` for luma and
`kCode10ChromaZero` for chroma, and ignored on unpack. Determinism is the
requirement: without it `unpack -> pack` is not byte-identical at such widths
and I7 could not be stated for them. Black rather than replication of the edge
pixel because a decoder that renders the full group should show nothing rather
than a smear. Neither 720 nor 1920 exercises this path.

**ADR-019 — Test pattern generation logic is header-only
(tools/testpat.hpp), shared by the tool and its test.**
`tools/make_testpat.cpp` is its own `add_executable`, not part of
`scatter-core`, and `tests/test_testpat.cpp` links only `scatter-core` (via
`scatter_test()`). Neither links the other's translation unit, so
`makeRamp`, `makeZonePlate`, `makeExcursion` and `writeV210` live in a
header both include, rather than being duplicated or exposed only through
the executable. Same interface/implementation split the project already
uses for v210 (WU-02); header-only here specifically because there is no
compiled library boundary between "tool" and "test" for this unit.

**ADR-020 — Chroma resampling filter coefficients, fixed at WU-04.**
`docs/architecture.md` section 5 specified filter *shape* only — "4- or
6-tap" for the 422->444 upsampler, "9- or 11-tap" half-band for the 444->422
downsampler — and left the concrete taps open. WU-18's NEON path needs
something byte-exact to diff against (the same relationship v210.hpp/.cpp
already has to WU-17), so this session picked and froze them:

- Upsample (co-sited-exact, one filtered tap per pair): `out[2i] = in[i]`
  exactly; `out[2i+1] = (-in[i-1] + 9in[i] + 9in[i+1] - in[i+2] + 8) >> 4`.
  Coefficients (-1, 9, 9, -1)/16 — the standard 4-tap half-sample
  interpolator, symmetric, unity DC gain, negative outer lobes.
- Downsample: `out[i] = (3in[2i-5] - 25in[2i-3] + 150in[2i-1] + 256in[2i] +
  150in[2i+1] - 25in[2i+3] + 3in[2i+5] + 256) >> 9`. Coefficients (3, -25,
  150, 256, 150, -25, 3)/512 — a standard truncated half-band low-pass; the
  half-band property means every tap at a nonzero even offset from centre
  is exactly zero, so nominally "11-tap" is 7 nonzero multiplies.
- Edge handling: index clamped to the plane (replicate the boundary
  sample), same choice both directions.
- Rounding: round-half-up via "add half the divisor, then arithmetic
  shift" — the same convention `core/types.hpp`'s `toCode10` uses. C++20's
  guaranteed-arithmetic signed right shift makes this round correctly for
  the negative partial sums the outer lobes produce, not just positive
  ones.
- No clamp on the numeric result beyond what a 16-bit unsigned `Sample` can
  hold. I2 forbids clamping to any legal-range value, and a
  representational-range wrap (well-defined modulo-65536 narrowing, not
  UB) is not that — it is not pulling anything toward a legal range, the
  container is just finite. Worst-case overshoot on a step is 1/16 of the
  step (upsample) or 22/512 (downsample), both worked by direct
  computation in `tests/test_chroma.cpp`; this does not reach the
  representable boundary for chroma content with any reasonable headroom
  around `kChromaZero`, and has not been exercised at the v210 protocol
  limits themselves.

Both filters sum to an exact power of two, so a flat field survives with
zero rounding error in either direction — the property `tests/test_chroma.cpp`
checks first, before the coefficients are checked individually.

**ADR-021 — `file_source.cpp`/`file_sink.cpp` compile into `scatter-core`,
not `scatter`, despite living under `src/io/`.**
`docs/architecture.md` section 8's module-layout sketch places all of
`src/io/` under the `scatter` application target, which links the Blackmagic
DeckLink SDK. Read literally, that would put this unit's raw `.v210`
reader/writer — needed now, at WU-05, five units before DeckLink arrives at
WU-14 — behind an SDK dependency it does not have: `file_source.cpp` and
`file_sink.cpp` use only `<fstream>` and `v210::pack/unpackImage`. Compiled
into `scatter-core` instead, alongside `src/video/`, so
`tests/test_ramp_roundtrip.cpp` — and everything after it that reads or
writes a file — keeps running with no hardware connected and no driver
installed, the same property ADR-013 states for that target. When WU-14
adds `com_ptr.hpp` and the `decklink_*.cpp` files, those alone define the
`scatter` target's actual DeckLink dependency; `file_source.cpp` and
`file_sink.cpp` do not move. Does not reopen ADR-013: the `src/core`/
`src/video` split ADR-013 describes is untouched, this only clarifies which
CMake target a `src/io/` file lands in, which ADR-013 did not itself
address.

**ADR-022 — Lattice Catmull-Rom basis, edge handling and Jacobian scope,
frozen at WU-06.**
`docs/architecture.md` sections 4.1/4.2 specify the shape — "129×129 control
lattice... Catmull-Rom (cubic) interpolant... `J = [∂x/∂u ∂x/∂v; ∂y/∂u
∂y/∂v]`" — without fixing three implementation choices, the same kind of gap
ADR-020 filled for chroma's filter taps:

- Basis: uniform (not centripetal or chordal) Catmull-Rom, tangent at each
  control vertex `(next - prev) / 2` — the standard 4-point Hermite matrix
  form, `q(0) = p1`, `q(1) = p2`. Chosen because the lattice's own
  parametrisation is uniform (`u`, `v` are lattice-vertex indices, not
  arc-length), exactly where the uniform variant is well-behaved and the
  other two exist to fix problems — cusps, overshoot on non-uniform
  spacing — that do not arise here.
- Edge handling: control-vertex lookups outside `[0, kLatticeMax]` replicate
  the nearest edge vertex — the same choice ADR-020 makes for chroma's
  filter taps, applied here to the lattice's 4-point stencil instead of a
  sample row.
- Jacobian scope: `eval()` carries `(x, y, z)` per section 4.1's "producing
  (x, y, z)", but `jacobian()` is deliberately 2×2 (`dx/du`, `dx/dv`,
  `dy/du`, `dy/dv`) per section 4.2's stated definition — it drives
  `K = 1/|det J|` and the destination-raster filter footprint, both
  properties of the 2D output raster alone. `dz/du`/`dz/dv` are not
  computed; WU-26's surface normals (cross product of the two tangent
  vectors, section 4.2) will need them and can add them to this interface
  without changing `eval()` or the existing `Jacobian` struct.

Does not reopen `docs/architecture.md` — a reference document, not an ADR —
it fixes what that document left open, the same relationship ADR-020 has to
architecture.md section 5.

**ADR-023 — EWA footprint representation and K's compression clamp, frozen
at WU-07.**
architecture.md 4.2 gives K's formula (`K = 1/|det J|`) and says it is
"clamped to a configured maximum compression", and separately states "J is
the elliptical filter kernel" for anisotropic minification, without fixing:

- The clamp's value or where it lives. Chosen as a caller-supplied
  parameter to `densityCompensation()`, `maxK`, not a project-wide constant
  like `kLatticeSize` or the tile size: unlike those, it is not a property
  of a data structure this project fixes once — it belongs to whichever
  accumulation-stage configuration (WU-09 onward) actually chooses an
  operating point, and nothing in WU-07 has grounds to pick a number
  nobody has decided yet. `densityCompensation()` clamps via
  `std::min(rawK, maxK)`; `|det J| == 0` (a fold collapsing area to a
  point or line, permitted by I1) produces IEEE 754 `+inf` for `rawK`,
  well-defined and not UB, and clamps identically to any other extreme
  compression.
- The concrete representation of "the elliptical filter kernel". Chosen as
  `EwaFootprint{majorAxis, minorAxis, majorAngle}`: the image of the unit
  circle in source-parameter space `(du, dv)` under `J` is an ellipse
  whose semi-axes are `J`'s singular values and whose major-axis direction
  (in destination `(x, y)` space) is the corresponding left singular
  vector, both obtained in closed form from the eigen-decomposition of the
  symmetric 2x2 matrix `J*J^T` — not `J^T*J`, whose eigenvalues are
  identical but whose eigenvectors describe directions in source `(u, v)`
  space instead, not what a destination-raster filter footprint needs.
  Exact for 2x2, so no iterative SVD is needed anywhere in this path. When
  the footprint is a circle (pure rotation, or any `J` with equal singular
  values) `majorAngle` is left at 0 rather than computed from a degenerate
  eigenvector problem, since every direction is then a major axis.

Does not reopen `docs/architecture.md` — same relationship ADR-020 and
ADR-022 have to it, filling gaps the document leaves open on purpose.

**ADR-024 — Fragment generation and tile binning: pixel/lattice-parameter
mapping, pixel-space Jacobian, tile-local coordinate encoding, and
supersampling thresholds, frozen at WU-08.**
architecture.md 4.1, 4.4 and 4.6 fix the shape of pass 1 without fixing
several concrete numbers or representations, the same kind of gap
ADR-020/022/023 filled for earlier units:

- **Source pixel -> lattice parameter.** 4.1 says only that the lattice
  "covers the source raster"; `core/lattice.hpp`'s `(u, v)` range over the
  fixed `[0, kLatticeMax]` (128) control-vertex index space regardless of
  the source raster's actual resolution. Chosen: linear across the whole
  range, `u = px * kLatticeMax / (width - 1)` (`v` likewise), so the
  source raster's corners land exactly on the lattice's corners for any
  resolution — the only mapping that does not privilege one raster size
  over another.
- **K and supersampling use the pixel-space Jacobian, not the raw
  lattice-parameter one.** 4.2 names `J` directly as `core/lattice.hpp`'s
  `jacobian()` output, in units of "per unit of `(u, v)`". "The local
  compression ratio" (4.2) is a statement about source pixels per
  destination pixel, not about the 129-vertex control mesh, and the two
  differ by a constant per-axis scale factor that grows with resolution
  (`kLatticeMax / (dim - 1)` — at 1920 pixels wide, `128 / 1919`, roughly
  0.0667 per axis and 0.00445 on the determinant). Using the raw
  lattice-parameter Jacobian directly would make K and the supersampling
  decision depend on source resolution rather than on the warp itself.
  `core/binner.cpp`'s `pixelJacobian()` applies the chain rule — scaling
  `jacobian()`'s output by that same constant, the inverse of the first
  bullet's mapping — with no extra lattice evaluation.
- **Tile-local coordinate encoding for a replicated fragment.** 4.4's
  fragments "whose footprint straddles a tile boundary are replicated
  into the neighbour". `Frag::x`/`y` (`core/types.hpp`'s `SubPos`, 12.4
  fixed) are documented as tile-local, but unsigned — and the four-bank
  splat's footprint (4.5's base, base+1, base+stride, base+stride+1) can
  reach up to one whole pixel outside a fragment's home tile, which is
  negative relative to the neighbour tile it's replicated into. Chosen:
  every stored coordinate is biased by exactly one pixel
  (`kSubPixelOne`, already defined in `core/types.hpp`) — `stored =
  (position_relative_to_this_tile + 1px)` in 12.4 fixed — applied
  uniformly to every fragment, home or replica, so one decode rule
  applies to a tile's whole bin. A home-tile fragment's stored value
  therefore ranges over `[1px, (kTileSize+1)px)`; a replica's over
  `[0px, 1px)`. WU-09's splat un-biases by subtracting `kSubPixelOne`
  before taking the integer base cell. `core/types.hpp`'s existing
  `kTileSize <= 4096` static assert already gives more headroom than
  this needs at either compile-time tile size (16 or 32).
- **Supersampling thresholds and cap.** 4.6 fixes the shape ("2x2 or
  4x4... when det J exceeds a threshold... cap the subdivision factor")
  without fixing the numbers. Chosen: `threshold2x2 = 1.0`, anchored to
  4.6's own literal "det J > 1" rather than an invented number;
  `threshold4x4 = 4.0`, the point where 2x2's linear-per-axis rate
  (`sqrt` of the area factor) would itself reach 2 pixels of gap; both
  configurable per caller (`SupersampleConfig`), with `maxSupersample`
  the hard cap 4.6 requires. All strict `>`, so a boundary value (`det J
  == 1` or `== 4` exactly) does not subdivide.
- **Off-raster samples are dropped, not clamped.** Neither 4.1 nor 4.4
  says what happens to a source sample whose destination falls outside
  the raster being resolved. Chosen: no fragment is emitted (counted in
  `BinStats::droppedOffRaster`) rather than clamping its position into
  range, which would fabricate a destination the warp never produced.

Not yet fixed, and deliberately not an ADR: `Frag::z`'s quantisation from
`Vec3::z`'s double. Nothing downstream reads it before WU-28's k-buffer,
and WU-08's accept criteria do not exercise it; `core/binner.cpp` rounds
and saturates to the `uint16_t` range as the simplest placeholder, to be
revisited when something actually depends on it.

Does not reopen `docs/architecture.md`, ADR-022 or ADR-023 — same
relationship ADR-020 has to the document itself; the pixel-space-Jacobian
distinction uses ADR-023's `densityCompensation()` unchanged, just with a
correctly-scaled input.

**ADR-025 — Four-bank splat: bilinear corner weights, tile-edge corner
handling, and the SubPos decode, frozen at WU-09.**
architecture.md 4.5 fixes the bank/corner addressing scheme itself — bank A
the base cell, B base+1, C base+stride, D base+stride+1; "each fragment
performs exactly one read-modify-write per bank"; on resolve, all four
banks addressed identically and summed — without fixing:

- **How a fragment's weight is distributed across its four corners.** 4.5
  alone is silent on this; section 13's provenance note fixes it instead —
  "forward scatter with fractional addresses supplying splat weights".
  Chosen: bilinear, using the fragment's own sub-pixel fractional position
  (`SubPos`'s low 4 bits per axis, already defined by ADR-024) as the two
  axis weights, integer fixed-point throughout (I6), never floating point.
  A corner's raw weight is the product of its two axis weights (0..16
  each), so 0..256 (`kSubPixelOne^2`); a corner's contribution to
  `AccumCell::Y`/`Cb`/`Cr`/`w` is `(colour or w) * frag.w * rawWeight`,
  right-shifted by 8 (`2 * kSubPixelBits`). This is the standard bilinear-
  splat formula for a fractional destination position, not an invented
  number the way ADR-023's `maxK` or ADR-024's supersampling thresholds
  are — there was only one geometrically sensible choice once "fractional
  addresses supplying splat weights" is taken literally. Truncation from
  the right shift is per-corner and deterministic, so a fragment's four
  corner contributions do not always sum back to exactly its own weight
  (up to 3 parts in 256 can be lost) — acceptable, since I6 requires
  determinism and order-independence, not exact weight conservation across
  one fragment's own footprint.
- **What happens when a corner falls outside the tile being splatted.**
  Not an independent choice: the necessary consequence of WU-08's already-
  frozen replication scheme (ADR-024). A fragment's footprint reaches at
  most one pixel past its base cell per axis, and `core/binner.cpp`
  already replicates any fragment whose footprint crosses a tile boundary
  into the neighbouring tile's own bin, with tile-local coordinates
  recomputed relative to it. So of the up to four physical copies one
  source-level fragment can become, spread across up to four tiles' bins,
  each copy's own out-of-range corners either belong to a *different*
  copy in a *different* tile's bin, or — at the destination raster's own
  edge, where there was no neighbour to replicate into — correspond to no
  real destination pixel at all. `splat.cpp`'s internal `splatCorners()`
  simply skips a corner whose computed cell address falls outside
  `[0, kTileSize)` on either axis: not a clamp, not a wraparound, not a
  fabricated destination (the same "do not fabricate a destination the
  warp never produced" reasoning ADR-024's off-raster-drop choice uses).
- **Decoding `SubPos` back to a signed (base, fraction) pair.** ADR-024
  biases every stored coordinate by `+kSubPixelOne` so an unsigned field
  can hold a replica's up-to-one-pixel-negative position; WU-09 needs the
  inverse. Chosen: widen to `int32_t`, subtract `kSubPixelOne`, then use
  C++20's guaranteed two's-complement, floor-rounding signed right shift
  (`>> kSubPixelBits`) together with the matching bitwise-and mask to
  split into a (possibly negative) integer base cell and a non-negative
  fraction in one step — the standard fixed-point decode technique,
  applied to a signed range for the first time in this codebase because
  ADR-024's bias is exactly what makes that range signed.

Does not reopen `docs/architecture.md` or ADR-024 — same relationship
ADR-020/022/023/024 have to the document and to each other: this fills a
gap architecture.md leaves open on purpose, and treats ADR-024's biased
encoding as a fixed input, not something to revisit.

**ADR-026 — Composite background is a caller-supplied flat colour; WU-10's
own orchestration entry points live in `core/resolve.hpp`, not a new
`pipeline.hpp`.**
architecture.md 4.8 fixes normalise-then-composite's arithmetic ("out =
Σ(w·colour)/Σw... composite against the background using Σw as alpha")
without saying what "the background" is — nothing upstream of WU-10
produces a second image to composite against; that is Phase 2's k-buffer
(WU-28, 4.7). Two things this session had to decide that architecture.md
leaves open:

- **What the background is.** Chosen: a caller-supplied constant colour
  (`Background{Y, Cb, Cr}`, `core/resolve.hpp`), default legal black (I3's
  `kBlack`/`kChromaZero`), not a second raster. A second raster is what
  4.7's phase-1 transparency (pure weighted accumulation, in scope this
  session per WORK-UNITS.md) would composite *the whole output* onto in a
  multi-layer pipeline, but nothing between WU-01 and WU-10 defines what
  that second raster would even be for a single-frame, single-surface
  pipeline — inventing one now would be answering a question WU-28's own
  k-buffer design (4.7's phase 2) has not been reached yet to ask, let
  alone answer. A flat colour is the smallest thing that satisfies 4.8's
  literal requirement (something to composite zero-weight and
  partially-covered cells against) without prejudging that later design.
  `runFrame()`/`runFrameFile()`'s `PipelineParams::background` defaults to
  black so an identity-map, fully-covered frame (this unit's own I7 check)
  never has occasion to composite against anything else.
- **`alpha`'s range.** 4.8 says "using Σw as alpha" without saying whether
  Σw is already a `[0, 1]`-scale fraction. It is not: `core/types.hpp`'s
  own `Weight` comment already notes a single, non-overlapping surface's
  weight legitimately exceeds `kWeightUnity` wherever many source samples
  land in one destination cell, and this unit's own zone-plate accept
  criterion routinely accumulates Σw far above `kWeightUnity` in a single
  cell under compression (order 1000x at 32:1, C-007). Chosen: alpha is
  Σw clamped to `[0, kWeightUnity]` — coverage saturates at "fully
  covered" rather than growing without bound (which would give the
  background a *negative* contribution once alpha exceeds 1, not what
  "coverage" means) or inverting the blend. A cell with Σw == 0
  (`normaliseCell()`'s flagged case, 4.8's "zero-weight case flagged
  rather than silently producing black") composites as pure background
  without ever reading its meaningless colour fields.

A third thing this session had to decide, unrelated to 4.8's own gap but
forced by the same file-layout question ADR-021 already answered once:
**where `core/pipeline.cpp`'s orchestration functions (`runFrame()`,
`runFrameFile()`) are declared.** `docs/architecture.md` section 8's
module-layout sketch names `pipeline.hpp/.cpp # orchestration, thread
pool, barriers` as a pair, the same way it names every other `core/`
module. Chosen: declare them in `core/resolve.hpp` instead, with no
`pipeline.hpp` at all yet. The thread pool and barriers section 8's own
comment describes are WU-16's (Phase 4), not built yet; this unit's own
orchestration is one function with no state of its own to expose beyond
it; and SESSION-PROTOCOL.md's work-unit cap ("touch at most 3 source files
plus its test") is already spent by `resolve.hpp`, `resolve.cpp` and
`pipeline.cpp` — a fourth header has nowhere to go without exceeding it.
ADR-021 already established the precedent for exactly this situation —
declaring a new `.cpp`'s public entry points in an existing, related
header instead of a header of its own, for `file_source.cpp`/
`file_sink.cpp` in `video/raster.hpp` — applied here the same way. When
WU-16 actually adds thread-pool state, `pipeline.hpp` arrives with it and
these declarations move there; nothing about `runFrame()`/`runFrameFile()`'s
own signatures is expected to change when that happens.

Does not reopen `docs/architecture.md` or ADR-021 — extends ADR-021's
precedent to a second, later case rather than revisiting it.

**ADR-027 — Cylinder and sphere: orthographic projection, front-at-zero
depth convention, gimbal-angle sphere parametrisation, and a shared
`shapes.hpp`, frozen at WU-11.**
`docs/architecture.md` 4.1 fixes the *mechanism* every shape must use — "a
shape is a function of `(u, v, t)` producing `(x, y, z)` in output raster
space, sampled onto a coarse lattice" — without fixing cylinder or sphere's
actual parametrisation, or how a 3D surface point becomes a 2D destination
position. Section 13's provenance note points at GB 2,158,671 ("3D address
map projected to a viewing surface") and US 5,150,213 ("projection onto
non-planar surfaces") for the historical grounding, but neither document is
quoted for a formula — this is this session's own design work, the same
kind of gap ADR-020/022/023/024/025/026 filled for earlier units.

- **Projection: orthographic, not perspective.** Every affine lattice
  WU-01 through WU-10 built (test-local `makeAffineLattice`/
  `makePixelAffineLattice`, since a shipped `plane.cpp` does not exist yet)
  already writes `Vec3::x`/`y` as literal destination-raster pixel
  coordinates with `z` uniformly `0`, and `core/binner.cpp` already reads
  `dest.x`/`dest.y` directly as pixel coordinates (its off-raster check is
  `dest.x < 0 || dest.x >= destWidth`, no division by depth anywhere) — no
  projection step exists between `Lattice::eval()` and its consumers today.
  `core/jacobian.hpp`'s `K`/EWA formulas (ADR-023) operate on `∂x/∂u`, etc.
  directly, not on a post-perspective-divide quantity. Introducing a real
  camera/lens model (a perspective divide by `z` or `z + focalLength`)
  would need to change `core/binner.cpp` and `core/jacobian.hpp` too, which
  this unit's file scope does not include and architecture.md does not ask
  for. Orthographic projection needs none of that: a shape's `(x, y)` *is*
  its destination position, offset by a caller-supplied centre; `z` carries
  straight through as a depth scalar, exactly like every earlier lattice's
  `z = 0`. This is also historically apt — contemporary hardware DVEs of
  this class did not carry a perspective camera either.
- **Depth convention: `z = 0` at each shape's front-most (nearest-camera)
  point, increasing into the screen.** Every earlier lattice (the affine/
  plane case) sits at `z = 0` uniformly, being the flat degenerate case;
  choosing the same zero point for a curved shape's nearest point means a
  cylinder or sphere smoothly reduces toward the existing flat case as
  `angleSpan`/`angleSpanH`/`angleSpanV` shrink toward zero, rather than
  introducing a discontinuity at the shape boundary. It also matches
  `core/binner.cpp`'s `toDepth()`, whose saturate-at-zero clamp already
  assumes depth does not go negative — true by construction here, since
  `1 - cos(...)` and `1 - cos(...)*cos(...)` are both always `>= 0`.
- **Cylinder: axis parallel to the destination raster's `y` axis (a
  vertical "roll"), the standard cylinder-DVE look.** `s = col /
  kLatticeMax`, `t = row / kLatticeMax` (control-vertex indices normalised
  to `[0, 1]`, not a per-source-pixel scale — see below); `θ = (s - 0.5) *
  angleSpan`; `x = centerX + radius·sin θ`, `z = radius·(1 - cos θ)` — the
  standard parametrisation of a circle for the cross-section that actually
  curves; `y = centerY + (t - 0.5)·heightSpan`, linear, because a
  cylinder's axis direction does not curve at all, only its cross-section
  does. `θ = 0` (`s = 0.5`, horizontal centre) is exactly the front-facing
  point, `x = centerX`, `z = 0`.
- **Sphere: independent yaw/pitch ("gimbal") angles, not textbook
  longitude/colatitude spherical coordinates.** `φ = (s - 0.5) *
  angleSpanH`, `ψ = (t - 0.5) * angleSpanV`; `x = centerX + radius·sin
  φ·cos ψ`, `y = centerY + radius·sin ψ`, `z = radius·(1 - cos φ·cos ψ)`.
  Chosen over the textbook `(θ, φ)` parametrisation (which ties a single
  polar angle to both horizontal and vertical extent) because a Mirage-
  style sphere effect exposes independent horizontal- and vertical-wrap
  controls, the same split the cylinder already has between its curved
  axis (`angleSpan`) and its straight one (`heightSpan`) — the natural
  generalisation keeps that same independence for both of the sphere's
  angles instead of collapsing to one. This is not the standard formula,
  so it is verified, not asserted: substituting into `(x - centerX)^2 + (y
  - centerY)^2 + (z - radius)^2` gives `radius^2·[cos^2ψ·(sin^2φ + cos^2φ)
  + sin^2ψ] = radius^2·[cos^2ψ + sin^2ψ] = radius^2` identically — every
  control vertex lies exactly on the sphere of the configured radius for
  any `angleSpanH`/`angleSpanV`, which `tests/test_shapes.cpp` checks
  directly rather than trusting the algebra alone. Setting `angleSpanV` to
  0 collapses `x` and `z`'s formulas exactly to the cylinder's own (`ψ ≡
  0`, `cos ψ = 1`, `sin ψ = 0`, leaving `x = centerX + radius·sin φ`, `z =
  radius·(1 - cos φ)`, identical to the cylinder's `theta`/`x`/`z` with
  `phi` in place of `theta`) — `y` does not similarly collapse to the
  cylinder's own vertical mapping, since the sphere has no `heightSpan`
  parameter and this degenerate case instead pins `y` at `centerY`
  uniformly; the shared structure is the two shapes' curved cross-section,
  not the whole lattice. The two shapes' parametrisations are siblings, not
  independent inventions.
- **No `srcWidth`/`srcHeight` parameter on `buildCylinderLattice()`/
  `buildSphereLattice()`, unlike the test-local affine helpers.** ADR-024
  already makes `core/binner.cpp`'s `pixelToLattice()` map a source
  raster's first/last pixel to lattice parameter `0`/`kLatticeMax` for any
  resolution — the lattice's own index space is already resolution-
  normalised. The test-local affine helpers needed `srcWidth`/`srcHeight`
  only because they chose to parametrise scale *per source pixel*
  (`scaleX` = output pixels per source pixel); `radius`, `angleSpan` and
  `heightSpan` are not naturally expressible that way for a curved surface,
  and there is no other reason a shape function needs to know the source
  raster's resolution — it only ever reads the lattice's own normalised
  `[0, 1]` fraction (`col`/`kLatticeMax`, `row`/`kLatticeMax`).
- **A shared `core/shapes/shapes.hpp`, not one header per shape.**
  architecture.md 8's module-layout sketch names `shapes/plane.cpp
  cylinder.cpp sphere.cpp pageturn.cpp explode.cpp` with no header at all.
  `CylinderParams`/`SphereParams` and the two build functions need
  declaring somewhere a test (and eventually `src/app/config.cpp`) can
  reach; one header per shape would make this unit four source files
  (`cylinder.hpp`, `cylinder.cpp`, `sphere.hpp`, `sphere.cpp`) against
  its test, over SESSION-PROTOCOL.md's three-source-file cap. A single
  `shapes.hpp` for both keeps the unit at exactly three source files.
  Same "does this need its own header" judgement call ADR-021 and ADR-026
  already made for `file_source.cpp`/`file_sink.cpp` and `pipeline.cpp`
  respectively — applied here from the start, rather than discovered
  mid-unit, since two shapes sharing one header was foreseeable going in.

Not decided here, deliberately: self-occlusion/back-face handling (I1 and
architecture.md 4.7 phase 1 already specify accumulate-everything, no
sorting, no culling — a cylinder or sphere wide enough to fold back on
itself is exactly the "non-invertible maps, folds, tears" I1 exists for,
and nothing about that needs a decision from this unit); `plane.cpp` itself
(still implicit via the affine path, per HANDOFF.md, and not this unit's
job to formalise); and horizontal-axis cylinders or any other orientation
(the vertical-axis convention above is the only one built; rotating the
roles of `u`/`v` to build a horizontal one is a straightforward future
variant, not exercised or required by WU-11's own accept criteria).

Does not reopen `docs/architecture.md` — same relationship every ADR since
ADR-020 has to it, filling a gap the document leaves open on purpose.

**ADR-028 — Page turn (WU-12a): the shape-function `t` scoping question,
the flat/curl parametrisation, and the scope split with WU-12b's
priority-tag opacity, frozen at WU-12a.**
`docs/architecture.md` 4.1 names a page turn only implicitly (`shapes/
plane.cpp cylinder.cpp sphere.cpp pageturn.cpp explode.cpp`, section 8) and
4.7/section 9 fix what a page turn must ultimately *do* ("Transparent flap
by default; opaque with priority tag set") without fixing its geometry or
how opacity can work without WU-28's k-buffer — the same kind of gap
ADR-020 through ADR-027 filled for earlier units, but this session's own
`HANDOFF.md` (end of WU-11) flagged two things to decide deliberately
before scoping the unit at all, not just the usual "work out the
parametrisation" gap. Both are below, along with why this splits into
WU-12a (this session, transparent mode — shape only) and WU-12b (next
session, priority-tag opacity — `core/resolve.hpp`/`.cpp` only).

- **The shape-function `t` question.** `docs/architecture.md` 4.1: "a
  shape is a function of `(u, v, t)`... Optionally keyframed, with
  temporal interpolation between shape lattices. This is Mirage's morph."
  WU-13 is "Keyframed lattices, temporal interpolation (morph)"
  (`WORK-UNITS.md`). A page turn's own "how far turned" fraction looks, at
  first glance, like it could be that `t` — it is the first shape whose
  look genuinely changes over the course of an effect, unlike WU-11's
  static cylinder/sphere. **Chosen: it is not.** `docs/architecture.md`
  4.1's `t` and WU-13's own accept criterion both describe one specific
  mechanism — temporal interpolation *between two whole, independently
  authored lattices* (Mirage's morph: dissolve/interpolate control-vertex
  positions from lattice A to lattice B over N frames), a capability that
  has nothing to do with any one shape's own parametrisation and applies
  equally to any pair of lattices, curved or affine. A page turn's "how
  far turned" fraction is instead exactly the same kind of thing
  `CylinderParams::angleSpan` or `SphereParams::angleSpanH` already are
  (ADR-027): one scalar field on that shape's own params struct, read once
  per `buildXLattice()` call, with no dependency on any interpolation
  mechanism existing. Naming it `turnProgress`, not `t`, is deliberate —
  avoiding the collision with 4.1's own name for a different, not-yet-built
  thing. This unit adds no keyframing, no lattice-to-lattice interpolation
  and no change to `Lattice`, `Lattice::eval()` or `Lattice::jacobian()`'s
  signatures; a page turn animated over many frames is simply many calls to
  `buildPageTurnLattice()` with different `turnProgress` values, one per
  frame, exactly as a cylinder animated over many frames would be many
  calls with different `angleSpan` — WU-13's own keyframe/morph mechanism,
  whenever built, is orthogonal to this and could in principle interpolate
  *between* two page-turn lattices (or a page-turn and a sphere) the same
  way it interpolates between any other pair, without either shape
  function needing to change.

- **The flat/curl parametrisation.** Modelled as a rolling sheet, per
  `HANDOFF.md`'s own "the obvious starting point": a flat portion nearest
  a fixed spine (hinge), and a curled portion — the part that has turned —
  wrapping a partial cylinder of a configured radius, arc-length
  parametrised so the sheet does not stretch as more of it feeds into the
  curl. `PageTurnParams::turnProgress` (`[0, 1]`) sets `flatLen = (1 -
  turnProgress) * width`, the distance from the spine, measured along the
  flat sheet, at which the split falls; a control vertex at distance `sx`
  from the spine is flat (`x = spineX + sx`, `z = 0`) if `sx <= flatLen`,
  or curled (`a = sx - flatLen`, `theta = a / radius`, `x = spineX +
  flatLen + radius*sin(theta)`, `z = radius*(1 - cos(theta))`) otherwise —
  the curled branch reusing ADR-027's own cylinder cross-section formula
  verbatim, parametrised by arc length instead of a fixed angular span.
  Three properties this derivation gives, each checked directly in
  `tests/test_pageturn.cpp` rather than only asserted:
  - **Position- and tangent-continuous at the flat/curl boundary, for any
    `turnProgress`.** At `theta == 0` (the boundary itself), the curled
    branch gives `x = spineX + flatLen`, `z = 0` — exactly the flat
    branch's own value there. Differentiating both branches with respect
    to `sx`: the flat branch has `dx/dsx = 1`, `dz/dsx = 0` everywhere;
    the curled branch's `dx/da = cos(theta)`, `dz/da = sin(theta)`, and
    `da/dsx = 1`, so at `theta == 0` (`a == 0`) it gives `dx/dsx =
    cos(0) = 1`, `dz/dsx = sin(0) = 0` — identical to the flat branch's
    own derivative. No kink at the seam, for any `radius`/`width`/
    `turnProgress` combination, not just ones this session happened to
    test.
  - **The spine never moves.** `sx == 0` (`s == 0`, control-vertex column
    0) is always `<= flatLen` (`flatLen >= 0` for `turnProgress` in its
    documented `[0, 1]` range), so the spine column is always on the flat
    branch, `x == spineX` and `z == 0` exactly, regardless of
    `turnProgress` — `radius` never enters the calculation there at all.
    `tests/test_pageturn.cpp`'s `test_pageturn_spine_never_moves()`
    checks this at several `turnProgress` values directly.
  - **`turnProgress == 0` reduces exactly to the flat (affine) case.**
    `flatLen == width` then, so `sx <= flatLen` holds for every `sx` in
    `[0, width]` (`s` never exceeds 1) — the curled branch is never
    reached, and every control vertex sits at `z == 0`, `x` linear in `s`,
    the same "reduces to the existing flat case" property ADR-027 already
    established for cylinder/sphere as their own angular spans shrink to
    zero.

  `radius` must be positive whenever `turnProgress > 0` (`theta` divides
  by it wherever the curled branch is reached) — not checked here, the
  same unchecked-precondition convention `Lattice::at()`'s row/col bounds
  and `core/binner.cpp`'s tile-local indexing already use throughout this
  codebase. Large `radius`/`width`/`turnProgress` combinations (`theta`
  exceeding `2*pi`, wrapping the curl around on itself more than once) are
  not guarded against — same "extreme parameters produce an extreme but
  well-defined fold, not sanitized away" choice WU-11's own cylinder
  leaves `angleSpan > pi` to do (I1: "non-invertible maps, folds, tears
  and shattering are only expressible this way").

- **Why no `core/binner.cpp`, `core/splat.cpp` or `core/resolve.cpp`
  change is needed for transparent mode.** WU-11's own `HANDOFF.md` note
  ("WU-06 through WU-11 are all shape-agnostic — verify this holds, don't
  assume it") is checked directly here, not assumed: `docs/
  architecture.md` 4.7 phase 1's own transparency ("overlapping surfaces
  sum... no sorting, no depth buffer") requires nothing more than two
  independently generated fragment sets landing in the same tile bins,
  which `core/binner.hpp`'s `generateFragments()` already supports simply
  by being called twice against the same `TileBins` — it appends, it does
  not clear or otherwise depend on prior bin contents. `tests/
  test_pageturn.cpp`'s
  `test_pipeline_pageturn_transparent_accumulates_over_page_behind()`
  checks the consequence directly rather than by inspection of the code:
  splatting a page-turn flap and a full-canvas "page behind" together into
  one set of bins produces, at every destination cell, an `AccumCell`
  exactly equal (bit-for-bit — I6, integer addition is associative) to
  splatting each layer separately and adding their `AccumCell`s
  component-wise afterward. This is the direct, checkable form of "phase 1
  is pure weighted accumulation" (ADR-009, unchanged) for the *first* time
  two independently authored surfaces have actually been run through this
  pipeline together — WU-11 only ever populated one lattice per
  `runFrame()` call. `core/pipeline.cpp`'s `runFrame()` itself is
  unchanged and still only takes one lattice/source pair per call;
  producing a composited two-layer frame is the caller's own job (this
  unit's test does it directly against `TileBins`/`TileAccum`/
  `splatTile()`/`sumBanks()`, bypassing `runFrame()`'s single-shape
  convenience wrapper), the same way `tests/test_zoneplate.cpp`'s own
  reference checks reach past `runFrame()` when a check needs access
  `runFrame()` itself does not expose.

- **Why priority-tag opacity is a separate unit, WU-12b, not built this
  session, and its own design direction decided now.**
  `SESSION-PROTOCOL.md`'s sizing cap ("touch at most 3 source files plus
  its test... if a unit cannot meet this, split it before starting")
  cannot be met by one unit covering both modes: transparent mode's own
  shape needs `core/shapes/shapes.hpp` (touched) and `core/shapes/
  pageturn.cpp` (new) — two source files, this unit's own scope. A
  narrower-than-k-buffer opacity mechanism (below) needs `core/
  resolve.hpp` and `core/resolve.cpp` — a different two source files, in a
  different module, from a different unit's own reasoning. Together that
  is four source files before either unit's own test, one over the cap;
  `core/shapes/*` and `core/resolve.*` are also different enough concerns
  (a shape's own geometry versus how two already-accumulated layers
  combine at resolve time) that combining them into one work unit would
  violate the spirit of the cap even if the raw file count somehow fit.
  Decided instead: WU-12a (this session) is the shape and transparent mode
  alone; WU-12b (next session) is priority-tag opacity alone, `core/
  resolve.hpp`/`.cpp` only, no shape file changes needed (WU-12b can be
  exercised against WU-12a's own `buildPageTurnLattice()` and any earlier
  shape without modification to either).

  The mechanism itself, decided now as WU-12b's own scope rather than
  re-litigated when that session starts: `docs/architecture.md` 4.7 phase
  2 describes "the priority tag forcing opacity for the read-replace-write
  case" as part of the *general* k-buffer (WU-28, ADR-009 unchanged,
  "nearest 8 depth-sorted layers per pixel") — a mechanism this project
  explicitly does not have yet and is not this unit's job to build early.
  But "opaque" does not have to wait for that in order to mean something
  honest for exactly *two* layers with a caller-fixed order (which is all
  a page turn's own accept criterion — one flap, one page behind — ever
  needs): given two already-splatted `AccumCell`s for a lower layer and an
  upper layer, and the upper layer's own tag, WU-12b's own
  `compositeLayered()` (sketch; the actual name and signature are WU-12b's
  own to fix) reads the lower layer's `AccumCell`, resolves it against the
  caller's background as `composite()` already does today (the "read"),
  then composites the upper layer's own resolved colour over *that*
  result using the upper layer's own coverage as alpha (the "write",
  replacing what was read) — literally the patent's "read-replace-write"
  phrase 4.7 already quotes, for the case where the upper layer's tag
  equals a caller-configured opaque tag. Where it does not, the two
  `AccumCell`s are summed first and normalised/composited once — 4.7
  phase 1's own default, and the identity this unit's own
  `test_pipeline_pageturn_transparent_accumulates_over_page_behind()`
  already establishes. Both branches live in one new function operating
  on two already-resolved layers' worth of `AccumCell`, not per-fragment
  or per-`Frag::tag` (accumulation already discards which fragment
  contributed to a summed cell by the time `splatTile()` finishes, and
  reaching back to fix that would touch `core/splat.cpp`, outside WU-12b's
  own two-file scope) — an honest, narrower "opaque" than the general
  k-buffer: exactly two layers, ordered by the caller rather than sorted
  by depth, no per-pixel arbitration among more than two surfaces. This is
  a design sketch, not yet implemented or tested; `DECISIONS.md` records
  this scope decision now (matching WU-11's own `HANDOFF.md`, which
  flagged this same question) but the mechanism's actual signature and
  behaviour are WU-12b's own to freeze once written and tested, the same
  way every other ADR in this file follows, not precedes, its own unit's
  test-writing.

- **A shared `core/shapes/shapes.hpp`, extended rather than given its own
  header.** Same "does this need its own header" judgement ADR-027 already
  made of itself for cylinder/sphere, applied again: `PageTurnParams`/
  `buildPageTurnLattice()` are declared in the existing `shapes.hpp`
  rather than a new `pageturn.hpp`, keeping this unit at exactly two
  source files (`shapes.hpp`, touched; `pageturn.cpp`, new) against its
  test.

Not decided here, deliberately: corner-lift or diagonal-curl page-turn
variants (this unit's own roll, curling uniformly along the whole spine,
is the classic Mirage/patent look `HANDOFF.md`'s own "rolling... sheet"
framing points at; a corner peel is a plausible future variant, not built
or required by WU-12a's own accept criteria); the flat/curl seam's second
derivative (position and tangent match exactly at the seam, per the
derivation above, but curvature does not — the flat branch's second
derivative is identically zero and the curled branch's is not, at any
`radius`, meeting only C1 not C2 — between the 129 control vertices this
project's own Catmull-Rom expansion smooths across it with no visible
defect at any radius/width ratio this session exercised, and `Lattice::
jacobian()`'s own agreement with central differences, checked directly at
and near the seam by `tests/test_pageturn.cpp`, is unaffected by this,
the same way it was unaffected by C-008(a)'s unrelated lattice-edge issue
— but an extremely tight curl radius relative to `width / kLatticeMax`
could in principle show as a slight visual crease between control
vertices; not measured, not fixed, flagged here rather than fabricated as
either a problem or a non-problem); and WU-12b's own opacity mechanism
past the scope decision recorded above.

Does not reopen `docs/architecture.md`, ADR-009 or ADR-027 — same
relationship every ADR since ADR-020 has to the document; ADR-009's
k-buffer deferral is unchanged (WU-12b's own two-layer mechanism is not a
k-buffer and does not claim to be one), and ADR-027's cylinder cross-
section formula is reused, not altered, by this unit's own curled branch.

**ADR-029 — `compositeLayered()`: name, signature and behaviour frozen at
WU-12b; implemented as two calls to `composite()` rather than a hand-rolled
blend; the opaque tag is a function parameter, confirming rather than
reopening ADR-028's own sketch.**
ADR-028 (WU-12a) sketched priority-tag opacity's mechanism in full but
explicitly left "the actual name and signature" for this unit to freeze
"once written and tested". This session wrote and tested it; this entry
freezes what came out.

- **Signature, declared in `core/resolve.hpp`, implemented in
  `core/resolve.cpp`:**
  ```cpp
  CompositedCell compositeLayered(const AccumCell& lower, const AccumCell& upper,
                                   std::uint8_t upperTag, std::uint8_t opaqueTag,
                                   const Background& bg = kDefaultBackground) noexcept;
  ```
  Name kept as `compositeLayered` — ADR-028's own placeholder — since
  nothing surfaced while implementing it that made a different name clearer;
  no reason found to depart from the sketch's own choice. Parameter order
  (`lower`, `upper`, then the two tags, then `bg` last with the same
  default-to-`kDefaultBackground` convention `composite()` above it already
  uses) mirrors `composite()`'s own shape as closely as a two-layer function
  can, so a reader who already knows `composite()` needs to learn only what
  is different: two cells instead of one, and the two tags that decide how
  they combine.

- **Only `upper`'s tag is read.** `lower`'s own tag, if a caller happens to
  be tracking one, plays no part in the decision — not an oversight, the
  literal reading of ADR-028's own sketch ("the upper layer's own tag, a
  caller-configured opaque tag") and of architecture.md 4.7 phase 2's
  "opaque with priority tag set" for a page-turn flap: it is the flap (the
  upper layer here) whose own tag can force opacity, not whatever is
  beneath it. A caller with a lower layer that also needs to force its own
  opacity against something still further down is a three-or-more-layer
  case this unit's own two-layer scope does not claim to handle — ADR-009's
  k-buffer (WU-28), unchanged.

- **Implemented as two calls to `composite()` (declared just above it in
  `core/resolve.hpp`), not a hand-rolled blend.** The opaque branch's own
  "read" step — composite `lower` against `bg` — is literally a call to
  `composite(lower, bg)`; its result, `CompositedCell{Y, Cb, Cr}`, is then
  reinterpreted as a `Background{Y, Cb, Cr}` (`asBackground()`,
  `resolve.cpp`, an anonymous-namespace helper — the two structs are the
  same three `Sample` fields under different names for different pipeline
  stages, so this is a relabelling, not a conversion) and handed to a
  second `composite(upper, ...)` call for the "write" step. This was not
  the only way to implement ADR-028's own read-replace-write description —
  a version written directly against `AccumCell`/`Sample` fields, calling
  `normaliseCell()` and a per-channel blend once each rather than
  `composite()` twice, would also satisfy it — but composing two calls to
  an already-tested public function is simpler, touches strictly fewer
  lines of new arithmetic in `core/resolve.cpp`, and gets partial upper
  coverage correct for free rather than by extra casework: `composite()`'s
  own alpha-clamp means `upper.w == 0` makes the second call return
  `afterRead` unchanged (lower alone — the "unaffected" half of WU-12b's
  own accept criterion) and `upper.w >= kWeightUnity` makes it return
  upper's own resolved colour outright, independent of `afterRead` (the
  "resolves close to the flap's own colour" half) — both fall out of
  `composite()`'s existing, already-verified behaviour rather than needing
  their own new tests of a duplicated blend. `tests/
  test_layered_composite.cpp`'s own Part A checks this composition
  directly and exactly (not just by pipeline-level inference) at both of
  those edges and at a genuine partial-alpha point in between.
- **The transparent (tag-mismatch) branch sums via a small local
  `sumCells()` helper** (`core/resolve.cpp`, anonymous namespace) —
  component-wise `Y`/`Cb`/`Cr`/`w`, `reserved` left at its value-initialised
  0 since nothing reads it — then calls `composite()` once. Exact, per I6
  (integer addition is associative), the same identity `tests/
  test_pageturn.cpp`'s own
  `test_pipeline_pageturn_transparent_accumulates_over_page_behind()`
  already established for two real splatted layers; this unit's own
  contribution is wiring that identity into `compositeLayered()`'s own
  fallback branch, not re-deriving it.

- **The opaque tag is a function parameter, not a `PipelineParams` field.**
  ADR-028's own sketch already described the mechanism this way ("a
  caller-configured opaque tag" listed alongside the two `AccumCell`s and
  `Background` as one of the function's own inputs), so implementing it
  as a parameter confirms that sketch rather than deciding something new —
  recorded here because the session's own brief asked whether this
  surfaced as a real open question once actually implementing, and it is
  worth being explicit that it did not: no alternative was seriously
  in play. A `PipelineParams`-style field would only make sense once some
  orchestration entry point (`runFrame()` or a future two-layer sibling)
  calls `compositeLayered()` on a caller's behalf and needs somewhere
  caller-wide to park the value; nothing in WU-12b's own scope adds such an
  entry point — `core/resolve.hpp`/`.cpp` only, no `core/pipeline.cpp`
  change, matching WU-12a's own precedent of exercising its two-layer
  identity directly against `TileBins`/`TileAccum` rather than through
  `runFrame()`. `PipelineParams::tag` (ADR-026) already exists but serves a
  single-layer `runFrame()` call passing one tag through to
  `generateFragments()`; it has no natural generalisation to "the upper
  layer's tag" and "the opaque tag" as a pair without inventing an
  orchestration shape this unit was not asked to build. Left for whichever
  future unit actually wires two-layer opacity into `runFrame()` (not
  scheduled) to decide, the same way ADR-026 itself left the k-buffer's own
  background question to WU-28.

- **Test file: `tests/test_layered_composite.cpp`, new** — WORK-UNITS.md's
  own WU-12b line names no file (left "TBD when this unit starts"); chosen
  to name the function it tests' own subject (a layered composite) rather
  than the shape that motivates it (a page turn, already `tests/
  test_pageturn.cpp`'s own name, WU-12a's), since `compositeLayered()`
  itself is shape-agnostic — it operates on two `AccumCell`s and two tags,
  nothing about a page turn specifically. Two parts: direct, hand-built
  `AccumCell` unit tests of the function alone (exact equality throughout —
  integer arithmetic, I6, not the cross-platform floating-point
  bit-exactness CORRECTIONS.md C-012 warns against for differently-shaped
  expressions), and the pipeline-level scenario HANDOFF.md's own "Next work
  unit" section suggested (a page-turn flap over a full-canvas page behind,
  duplicated locally from `tests/test_pageturn.cpp` per SESSION-PROTOCOL.md
  rule 2), checked exactly against an independent local re-derivation of
  the read-replace-write formula rather than a fuzzy tolerance, since
  nothing about this path involves the kind of cross-compiler
  floating-point noise C-012 found — normalise, blend and sum are all
  fixed-point.

Does not reopen `docs/architecture.md`, ADR-009 or ADR-028 — completes
ADR-028's own sketch, the relationship ADR-028's closing paragraph already
describes for whichever unit finished it; ADR-009's k-buffer deferral is
unchanged, and ADR-026's `composite()`/`Background` are reused unaltered,
not modified, by this unit's own two calls to them.

**ADR-030 — Keyframed lattices, temporal interpolation ("morph"):
`morphLattice()`'s name, signature and blend formula; exactly two
keyframes, not an ordered sequence; linear, not Catmull-Rom, interpolation
in time; and `core/lattice.hpp`/`.cpp`, not a new header, as its home —
frozen at WU-13.**
`docs/architecture.md` 4.1 names the mechanism ("a shape is a function of
`(u, v, t)`... Optionally keyframed, with temporal interpolation between
shape lattices. This is Mirage's morph") without fixing what a keyframe is
operationally, how many a single morph spans, the interpolation method, or
where the function lives — the same kind of gap ADR-020 through ADR-029
filled for earlier units. ADR-028 (WU-12a) already ruled out one candidate
reading — a page turn's own `turnProgress` is *not* an instance of this
`t` — and committed this unit's own scope in doing so: "WU-13's own morph
is temporal interpolation between two whole, independently authored
lattices (dissolve/interpolate control-vertex positions from lattice A to
lattice B over N frames), a capability that has nothing to do with any one
shape's own parametrisation and applies equally to any pair of lattices,
curved or affine." This entry freezes the mechanism that sentence
sketches.

- **What a keyframe is, operationally, and how many a morph spans.** A
  keyframe is nothing more than an already-built `Lattice` — the output of
  any shape function (`buildCylinderLattice()`, `buildPageTurnLattice()`,
  a plain affine one, or another `morphLattice()` call) or a hand-built
  one, exactly as any of those already are on their own. No new struct
  pairs a `Lattice` with a frame number or timestamp, and no ordered
  keyframe *sequence* type is introduced. ADR-028's own sentence above
  already commits to exactly two lattices per morph ("lattice A to lattice
  B"), not a bracketing search over an ordered sequence — so that reading
  is not this session's own choice to make, only to record. Deciding which
  two keyframes bracket a given frame, and reducing that to a single blend
  fraction, is orchestration: the same kind of caller-side responsibility
  ADR-026 already left the k-buffer's background question to (WU-28, not
  scheduled) and ADR-029 already left "the upper layer's tag as a
  `PipelineParams` field" to (whichever future unit wires two-layer
  opacity into `runFrame()`, not scheduled). Nothing between WU-01 and
  WU-13 defines a shot's own timeline or keyframe list, so inventing a
  type for one now would be answering a question no orchestration layer
  has been reached yet to ask — the same reasoning ADR-026 gives for not
  inventing a second raster before WU-28's k-buffer exists to need one.
  `morphLattice()` itself therefore takes only the two already-selected
  `Lattice`s and an already-computed blend fraction, exactly parallel to
  how `buildPageTurnLattice()` takes an already-computed `turnProgress`,
  not a turn start/end time.
- **Interpolation method: linear, not Catmull-Rom.** Checked against the
  module's existing cubic machinery before assuming the obvious answer,
  per `SESSION-PROTOCOL.md` rule 3's spirit (do not silently re-decide a
  frozen thing, but do check a new one against it): ADR-022's Catmull-Rom
  basis governs *spatial* interpolation within one lattice's own
  129×129 control mesh, where each cell's tangent is defined from its two
  neighbouring control vertices — a 4-point stencil that requires values
  on both sides to define a tangent, which is exactly why adjacent cells
  agree at their shared knot (ADR-022's own C1 argument). A morph has no
  such neighbourhood: exactly two keyframe lattices are defined, with no
  third or fourth lattice to supply an incoming or outgoing tangent, and
  nothing in architecture.md 4.1 asks for tangent continuity across a
  morph in the first place — "dissolve" (ADR-028's own word, echoing
  architecture.md's use elsewhere) names a cross-fade, not a spline.
  Spatial (`u`, `v`) and temporal (`t`) interpolation are orthogonal axes
  here, and only the spatial one has the 4-point neighbourhood a cubic
  basis needs; the temporal one is exactly two endpoints, for which linear
  is not a simplification of some fancier answer but the only interpolant
  that does not invent data outside its own two inputs.
- **Blend formula: `out = from*(1 - t) + to*t`, not the algebraically
  equivalent `out = from + t*(to - from)`.** Chosen specifically for
  `CORRECTIONS.md` C-012's own lesson — multiplying a finite value by an
  exact `0.0` or exact `1.0` introduces no rounding, regardless of FMA
  contraction, reassociation or platform `libm` differences, the same
  reasoning C-012's own fix already relies on for its untouched `y`
  check. At `t == 0.0`: `1.0 - 0.0 == 1.0` exactly, so `from*1.0 == from`
  exactly and `to*0.0` contributes an exact (possibly negative) zero,
  leaving the sum exactly `from`. At `t == 1.0`: `1.0 - 1.0 == 0.0`
  exactly, so the sum is exactly `to`. The `from + t*(to - from)` form
  does not have this property — `from + 1.0*(to - from)` is not
  guaranteed bit-identical to `to` in IEEE 754 for arbitrary `from`/`to`,
  since `to - from` followed by `from + (...)` is a different rounding
  path than reading `to` back directly. This gives `morphLattice()` the
  same "reduces exactly to the boundary case" property ADR-027 already
  established for `angleSpan`/`angleSpanH`/`angleSpanV` shrinking to zero
  and ADR-028 established for `turnProgress == 0` — here, at *both* of
  the morph's own endpoints, checked with `==` in `tests/test_morph.cpp`
  rather than a tolerance, since both operations involved (`* 1.0`, `*
  0.0`, and the following exact-zero-preserving add) are provably
  rounding-free, the same standard C-012 draws between its own loosened
  `x`/`z` checks and untouched `y` check. Interior `t` values are not
  claimed bit-exact against any independently-computed reference and are
  checked with a tight relative tolerance instead, per C-012's general
  lesson.
- **`t` is not clamped or validated.** Same unchecked-precondition
  convention `Lattice::at()`'s row/col bounds and
  `PageTurnParams::turnProgress` already use (ADR-028): a `t` outside
  `[0, 1]` linearly extrapolates past whichever keyframe it overshoots —
  well-defined arithmetic, not undefined behaviour, and an extreme but
  legitimate case rather than one sanitised away (I1-adjacent: this
  project does not clamp its way out of extreme parameters elsewhere
  either).
- **Name and signature, declared in `core/lattice.hpp`, defined in
  `core/lattice.cpp`:**
  ```cpp
  Lattice morphLattice(const Lattice& from, const Lattice& to, double t);
  ```
  Named for architecture.md 4.1's own term ("This is Mirage's morph"),
  the same way `tests/test_layered_composite.cpp` was named for the
  function it exercises rather than the shape that motivated it (ADR-029).
  Not `noexcept`: unlike `Lattice::eval()`/`jacobian()`/`at()`, which touch
  no new storage, `morphLattice()` default-constructs a fresh `Lattice` —
  a `std::vector<Vec3>` allocation that can throw — the same reason
  `buildCylinderLattice()`/`buildSphereLattice()`/`buildPageTurnLattice()`
  are not `noexcept` either, despite also being pure functions of their
  inputs otherwise.
- **Home: an addition to `core/lattice.hpp`/`.cpp`, not a new
  `core/keyframe.hpp`/`.cpp`, and not `core/shapes/shapes.hpp`.** Weighed
  against both of this project's own precedents for "does this need its
  own header": ADR-021 (`file_source.cpp`/`file_sink.cpp` declared in the
  existing `video/raster.hpp`) and ADR-026 (`runFrame()`/`runFrameFile()`
  declared in `core/resolve.hpp`, with no `pipeline.hpp` yet) both chose
  an existing, closely related header over a new one when the new
  function's own file-scope budget did not need a header of its own.
  `morphLattice()` needs no new type — no params struct, unlike every
  shape builder, since it has only two `Lattice` inputs and a scalar — so
  a new header/`.cpp` pair would be pure overhead against
  `SESSION-PROTOCOL.md`'s file cap for no structural benefit.
  `core/shapes/shapes.hpp` was considered and rejected: every function
  there *populates* a fresh `Lattice`'s control vertices from a parametric
  surface definition (`CylinderParams`, `SphereParams`, `PageTurnParams`);
  `morphLattice()` instead *consumes* two already-populated `Lattice`s,
  however either was built, and produces a third — a lattice-to-lattice
  operation, not a shape. `SESSION-PROTOCOL.md` rule 2 ("never rename or
  refactor across module boundaries... names in headers are fixed") is
  read here as forbidding a change to `Lattice`'s *existing* interface
  (`at()`, `eval()`, `jacobian()`, all untouched), not as forbidding a new
  addition to the same header — the same reading ADR-026 already relies
  on when it adds `runFrame()`/`runFrameFile()` to `core/resolve.hpp`
  alongside `composite()` without touching `composite()` itself. A free
  function at namespace scope, not a `Lattice` member or static factory:
  it needs no private access, and every existing shape builder is
  likewise a free function returning `Lattice` by value, not a member —
  `morphLattice()` matches that convention rather than the class's own
  `at()`/`eval()`/`jacobian()` member style.
- **No `core/shapes/*.cpp`, `core/binner.cpp`, `core/splat.cpp`,
  `core/resolve.*` or `core/pipeline.cpp` change.** `morphLattice()`
  returns a plain `Lattice` — the same type every shape builder already
  returns — so `Lattice::eval()`/`jacobian()` and everything downstream of
  a populated lattice (fragment generation, the four-bank splat,
  normalise/composite) are already shape-agnostic (ADR-027's own framing,
  reused verbatim: "everything downstream of a populated `Lattice`... is
  shape-agnostic already"). `runFrame()` still takes exactly one
  already-built `Lattice` per call, unchanged; producing a morphed
  lattice to pass to it is the caller's own job, the same "caller
  assembles, `runFrame()` stays a single-lattice convenience wrapper"
  precedent ADR-028's own transparent-accumulation note and ADR-029's
  scope note both already establish. This unit is proven entirely against
  `Lattice`'s own public API in `tests/test_morph.cpp`, the same way
  WU-06 proved `jacobian()` without any pipeline-level test at all —
  `morphLattice()` sits at exactly WU-06's own layer (pure lattice
  mathematics), not at the shape layer WU-11/WU-12a sit at, so it needs no
  `runFrame()`-level check to be a complete, independently useful unit
  (`SESSION-PROTOCOL.md`'s own "independently useful" sizing requirement).
- **Test file: `tests/test_morph.cpp`, new.** Named for the mechanism
  (architecture.md 4.1's own "morph"), the same convention
  `tests/test_jacobian.cpp` and `tests/test_layered_composite.cpp` already
  use — the function/mechanism under test, not the shape that happens to
  populate the keyframes in any one test case. Checks, in order: the two
  boundary reductions (`t == 0` exactly reproduces `from`, `t == 1`
  exactly reproduces `to`, every control vertex, `==`, per the blend
  formula's own bit-exactness argument above); a representative interior
  `t` against an independently-computed reference blend, tight relative
  tolerance (C-012); and `Lattice::jacobian()`'s analytic derivatives
  agreeing with central differences (WU-06's own method, reused) on a
  genuinely morphed, non-affine lattice (a blend of two distinct,
  independently built curved keyframes — reusing
  `shapes::buildCylinderLattice()`/`buildSphereLattice()`/
  `buildPageTurnLattice()` from the test file only, exactly as
  `tests/test_shapes.cpp` and `tests/test_pageturn.cpp` already do for
  their own Jacobian checks — proving the interpolant differentiates
  correctly on real blended surface data, not just synthetic single-shape
  data). Duplicates no fixture from `tests/test_jacobian.cpp`,
  `tests/test_shapes.cpp` or `tests/test_pageturn.cpp` — builds its own
  keyframe lattices locally, per `SESSION-PROTOCOL.md` rule "one unit, one
  test".

Does not reopen `docs/architecture.md`, ADR-021, ADR-022, ADR-026, ADR-027
or ADR-028 — same relationship every ADR since ADR-020 has to the
document, and this unit's own choices extend ADR-021/ADR-026's
"existing header when the new scope doesn't need one" precedent and
ADR-022's Catmull-Rom basis rather than revisiting either; ADR-028's own
`t`-scoping note is completed, not amended, by the mechanism this entry
freezes.

**ADR-031 — DeckLink device enumeration and `ComPtr`: the real SDK's
interface shape, `ComPtr`'s own design (modeled on the SDK's own sample,
plus one deliberate addition), the enumeration API's home and capability-
check design, and `BLACKMAGIC_SDK_DIR` as a CMake cache variable — frozen at
WU-14, Phase 3's first unit.**
`docs/architecture.md` 7 sketches the DeckLink SDK's shape (COM-style
`AddRef`/`Release`, `CreateDeckLinkIteratorInstance()`, "write a small
intrusive `ComPtr` and use it everywhere") without fixing any interface's
actual member list, `ComPtr`'s own concrete shape, or where the enumeration
code lives — `HANDOFF.md`'s own "Next work unit" note (going into this
session) flagged this as genuinely new ground, not a "fill in a
parametrisation architecture.md already named" gap the way WU-11 through
WU-13 all were, and asked this session to read the real SDK headers under
`~/src/Blackmagic DeckLink SDK 16.0` before scoping `WORK-UNITS.md`'s WU-14
lines, rather than working from architecture.md's summary alone. This entry
records what that reading found and the concrete choices it forces.

- **What `IDeckLink` actually is, and where it actually lives.** Not in
  `DeckLinkAPI.h` itself, despite that header defining nearly every other
  DeckLink interface (`IDeckLinkIterator`, `IDeckLinkInput`,
  `IDeckLinkOutput`, `IDeckLinkProfileAttributes`, and dozens more) — `class
  IDeckLink` is defined in `DeckLinkAPIDiscovery.h` (included transitively
  by `DeckLinkAPI.h`, so no separate `#include` is needed in practice, but
  worth recording since architecture.md 7 does not say which header, and
  guessing wrong would have looked plausible). Its own member list is
  minimal — exactly two methods, `GetModelName(CFStringRef*)` and
  `GetDisplayName(CFStringRef*)`, both macOS-specific (`CFStringRef`, not a
  portable string type) — confirming architecture.md 7's "one `IDeckLink`
  exposing both `IDeckLinkInput` and `IDeckLinkOutput`" is a claim about
  what `QueryInterface` can produce *from* an `IDeckLink`, not about
  `IDeckLink`'s own interface: `IDeckLinkInput`, `IDeckLinkOutput` and
  `IDeckLinkProfileAttributes` are all obtained via `QueryInterface`
  (`IID_IDeckLinkInput`, `IID_IDeckLinkOutput`,
  `IID_IDeckLinkProfileAttributes`), never exposed directly on `IDeckLink`.
  Verified against the SDK's own `Samples/DeviceList/main.cpp`, which does
  exactly this (`deckLink->QueryInterface(IID_IDeckLinkProfileAttributes,
  ...)`) rather than any direct accessor.
- **The enumeration entry point's real ownership convention.**
  `CreateDeckLinkIteratorInstance()` (declared `extern "C"` in
  `DeckLinkAPI.h`, implemented in the SDK's own `DeckLinkAPIDispatch.cpp` by
  CFBundle-loading `/Library/Frameworks/DeckLinkAPI.framework` at runtime
  and forwarding to it — architecture.md 7's own "loads DeckLinkAPI.bundle
  via CFPlugIn at runtime" description, confirmed directly rather than
  taken on faith) returns `nullptr` if the driver/bundle is not present
  (`Samples/DeviceList/platform.cpp`'s own `GetDeckLinkIterator()` wrapper
  checks exactly this), or otherwise an already-owned `IDeckLinkIterator*` —
  ordinary COM factory convention, the same as this SDK's every other
  `Create*Instance()` function. `IDeckLinkIterator::Next(IDeckLink**)`
  follows the same convention for each device it hands back, checked with
  `== S_OK` (`Samples/DeviceList/main.cpp`'s own `while
  (deckLinkIterator->Next(&deckLink) == S_OK)`), and returns something other
  than `S_OK` once enumeration is exhausted. Both are therefore *owned*
  results, not *borrowed* ones — a real distinction this project's own
  `ComPtr` has to get right (below), unlike a callback parameter such as a
  future `VideoInputFrameArrived(IDeckLinkVideoInputFrame*)` (architecture.md
  7's Input section: "valid only for the duration of the call unless you
  `AddRef` it"), which is borrowed and must be retained by the callee.
- **`ComPtr`'s own shape: modeled on the SDK's own `Samples/*/com_ptr.h`,
  not invented from scratch, plus one deliberate addition.** The SDK ships
  a `com_ptr<T>` sample header, present near-verbatim in `CapturePreview`,
  `DeviceStatus`, `FileCapture`, `FilePlayback`, `InputLoopThrough`,
  `KeyerOutput`, `MetalOutput`, `MultiPreview`, `SignalGenerator` and
  `SignalGenHDR` — copy/move constructors and assignment, a templated
  `QueryInterface`-based converting constructor
  (`com_ptr<T>(REFIID, com_ptr<U>)`), `get()`, `operator->`/`operator*`,
  `explicit operator bool`, `releaseAndGetAddressOf()`. Read in full this
  session (`Samples/CapturePreview/com_ptr.h`) rather than assumed from its
  name. Chosen: this project's own `src/io/com_ptr.hpp` (architecture.md 8
  already names this exact file) matches that shape closely, renamed to this
  codebase's own `PascalCase` type convention (`Lattice`, `AccumCell`,
  `EwaFootprint`, ... — `com_ptr` would be the only lowercase-leading type
  name in the whole tree) — deliberately *not* a differently-shaped
  hand-rolled version, since "write a small intrusive `ComPtr` and use it
  everywhere" (architecture.md 7) already has an obvious, working,
  battle-tested answer sitting in the same SDK, one every Blackmagic sample
  and every piece of Blackmagic-adjacent documentation already speaks.
  **One deliberate addition:** `adopt(T*)`. The sample's own raw-pointer
  constructor and raw-pointer `operator=` always call `AddRef()` — correct
  for a *borrowed* pointer, wrong for an *owned* one. Checked directly
  against the SDK's own sample code rather than assumed: `Samples/FileCapture
  /DeckLinkDeviceDiscovery.cpp` constructs `m_discovery` (a `com_ptr
  <IDeckLinkDiscovery>`) directly from `CreateDeckLinkDiscoveryInstance()`'s
  raw-pointer return via the AddRef'ing constructor, which — if
  `CreateDeckLinkDiscoveryInstance()` follows the same already-owned
  convention `CreateDeckLinkIteratorInstance()` does, which every other
  `Create*Instance()` function in this SDK does — over-retains by exactly
  one reference, never released, for the lifetime of that `com_ptr`. Harmless
  in that sample (one long-lived singleton, in a short-lived process), but
  not a pattern to copy uncritically into a real-time app that enumerates
  and reopens devices repeatedly across a long-running process, against
  architecture.md 12's own named risk ("Reference-count leaks lock the
  device... never hold a raw interface pointer"). `ComPtr::adopt(T*)`
  releases whatever the `ComPtr` currently holds and takes ownership of the
  new pointer with no `AddRef` — used for `CreateDeckLinkIteratorInstance()`'s
  direct return. `releaseAndGetAddressOf()` (already present in the SDK's
  own sample, unchanged here) already has the right semantics for the
  *out-parameter* case (`IDeckLinkIterator::Next()`) for free — release,
  return `&m_ptr`, let the callee write an owned pointer straight in, no
  `AddRef` on either side — so no second new method was needed for that
  half of the enumeration loop, only for the direct-return factory call.
- **The enumeration API: `DeviceInfo` plus `enumerateDeckLinkDevices()`, in
  new `src/io/decklink_device.hpp`/`.cpp`.** This project's first `src/io/`
  header — `file_source.cpp`/`file_sink.cpp` (WU-05) reused an existing
  `video/raster.hpp` rather than creating one of their own (ADR-021), but
  nothing existing in `src/io/` declares device-enumeration types, so a new
  header is not a judgement call here the way it was for ADR-021/ADR-026/
  ADR-030 — there is no existing `io/` header to extend. Declares `DeviceInfo
  {modelName, displayName, supportsCapture, supportsPlayback,
  ComPtr<IDeckLink> device}` and `std::vector<DeviceInfo>
  enumerateDeckLinkDevices()`. `#include "DeckLinkAPI.h"` directly in the
  header rather than forward-declaring `IDeckLink` — consistent with every
  other header in this project, none of which use a forward-declare-only
  pattern for their own dependencies; this makes `decklink_device.hpp`
  unambiguously macOS-only (fine — it belongs to the new `scatter-decklink`
  CMake target below, never `scatter-core`, the same boundary ADR-013
  already draws). String fields converted to `std::string` immediately
  inside `decklink_device.cpp` (`CFStringRef` released right after
  conversion, matching the SDK's own `DlToStdString`/`DeleteString` sample
  helpers folded into one step) rather than exposed as `CFStringRef` in the
  public struct, keeping `DeviceInfo` itself CoreFoundation-string-free
  beyond the `ComPtr<IDeckLink>` handle it necessarily still carries.
- **Capability check design: what a minimal WU-14 test can assert without
  opening a stream.** `IDeckLinkProfileAttributes::GetInt(
  BMDDeckLinkVideoIOSupport, &value)` returns a bitfield
  (`bmdDeviceSupportsCapture`, `bmdDeviceSupportsPlayback`) queryable with no
  stream open at all (`Samples/DeviceList/main.cpp`'s own
  `print_attributes()` does exactly this, never calling
  `EnableVideoInput`/`EnableVideoOutput`). `enumerateDeckLinkDevices()`
  reads both bits into `DeviceInfo`; `tests/test_decklink_device.cpp`
  additionally confirms a live `QueryInterface` for both `IID_IDeckLinkInput`
  and `IID_IDeckLinkOutput` succeeds on whichever device reports both bits —
  checking architecture.md 7's "one `IDeckLink` exposing both" claim two
  independently-failing ways rather than trusting the attribute alone. What
  this unit's own test does *not* and cannot assert without contradicting
  its own scope: anything downstream of `EnableVideoInput()`/
  `EnableVideoOutput()`/`StartStreams()` (mode support, actual frame
  delivery, dropped-frame behaviour) — that is WU-15's own job
  (`WORK-UNITS.md`: "Scheduled playback, file source to SDI out"), not
  this one's, and folding a stream-opening check into WU-14 would be
  exactly the kind of unit `SESSION-PROTOCOL.md`'s own sizing note asks to
  be split rather than crammed in — considered directly this session and
  not needed, since WU-14's own scope (enumeration and `ComPtr` alone,
  per `WORK-UNITS.md`'s own heading) never required a stream-opening check
  to be a complete, independently useful unit in the first place.
- **`BLACKMAGIC_SDK_DIR`: a CMake cache variable, not a hardcoded path.**
  Considered directly rather than assumed, per this session's own brief.
  Chosen: a `CACHE PATH` variable, default empty, following the exact
  pattern `SCATTER_TILE_LOG2` (ADR, implicitly, via `CMakeLists.txt`'s own
  comment) already establishes elsewhere in this same file — a documented
  `-D` flag with the invocation shown inline. Reasons a hardcoded path was
  rejected: the real path
  (`~/src/Blackmagic DeckLink SDK 16.0`) lives under the developer's own
  home directory, not this repository, with a version number baked into its
  own folder name that will not survive the next SDK update unchanged; the
  SDK ships its own `End User License Agreement.pdf` alongside its headers,
  a further reason not to vendor or hardcode a path implying those headers
  belong to this repository; and ADR-013/ADR-021's own "no unit leaves the
  tree unbuildable" property requires the Linux cloud sandbox's existing
  `scatter-core`/test matrix to keep configuring with zero SDK present,
  which a hardcoded, unconditionally-`#include`d path would break outright
  on any machine (including that sandbox) where it does not resolve. The new
  `scatter-decklink` target and `test_decklink_device` are therefore built
  only when `APPLE` is true *and* `BLACKMAGIC_SDK_DIR` resolves to a
  directory actually containing `Mac/include/DeckLinkAPI.h`; otherwise
  `CMakeLists.txt` emits a `STATUS` message and skips the block entirely,
  the same fail-soft shape ADR-021 already established for keeping
  `file_source.cpp`/`file_sink.cpp` buildable with no SDK present, applied
  here to an entire new target rather than a file-target placement.
- **Warnings on the SDK's own vendored `DeckLinkAPIDispatch.cpp`.** This
  project's `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
  -Werror` set (ADR-017) is unverified against generated, third-party SDK
  code — `set_source_files_properties(... COMPILE_OPTIONS "-w")` exempts
  only that one vendored file, leaving every file this project actually
  owns (`com_ptr.hpp`, `decklink_device.hpp`/`.cpp`,
  `test_decklink_device.cpp`) under the full set unchanged. Flagged as
  **unverified**, not confirmed working — this session has no AppleClang or
  Xcode toolchain to check it against; if the trailing `-w` does not in fact
  override the target-level `-Werror` the way this entry assumes, that is
  this unit's own bug to fix at the real terminal, not architecture.md's or
  ADR-017's to relax.
- **This entire unit is unverified by this session — deliberately, not by
  oversight.** Every ADR from ADR-020 through ADR-030 was written after its
  own unit's code had already been implemented and run through the full
  Linux-cloud-sandbox matrix (Clang 18, GCC 13, ASan/UBSan, both tile
  sizes) and, in most cases, `close.sh` on the real M1 Max besides. WU-14
  cannot follow that shape: no Blackmagic SDK and no AppleClang/Xcode
  toolchain exist in the Linux cloud sandbox this session's own drafting
  happened in, and the device bridge's own shell tool is a sandboxed Linux
  VM with neither either — `HANDOFF.md`, going into this session, already
  flagged this as expected, not a gap to route around. `com_ptr.hpp`,
  `decklink_device.hpp`/`.cpp` and `tests/test_decklink_device.cpp` are
  therefore reasoned through against the real SDK headers (this entry's own
  citations above) rather than compiled and run by this session at all.
  `WORK-UNITS.md`'s WU-14 line stays `wip`, not `green`, until built and run
  at the real terminal.

Not decided here, deliberately: `decklink_input.cpp`/`decklink_output.cpp`
and anything past enumeration (architecture.md 8's own module-layout sketch
names both; WU-20 and WU-15 respectively are where they arrive, not this
unit); and whether the Desktop Video / UltraStudio 4K Mini's own input and
output are separately confirmed working in Media Express — `HANDOFF.md`'s
own "Environment check still outstanding" only ever asked this session to
confirm the device *enumerates*, which this unit's own accept criteria
match exactly (`GetModelName`/`GetDisplayName`/`QueryInterface`/attribute
queries, no stream), not to confirm a capture/playback round trip, which
remains outstanding for whichever unit first needs it (WU-15, most likely).

Does not reopen `docs/architecture.md`, ADR-013, ADR-017 or ADR-021 — same
relationship every ADR since ADR-020 has to the document; ADR-013's
single-machine, SDK-free `scatter-core` boundary is extended to a second
target (`scatter-decklink`) rather than crossed, and ADR-017's warning set
is applied unchanged to every file this project owns, with only the one
vendored SDK file exempted, not the set itself relaxed.

**ADR-032 — Scheduled playback: the real SDK's output-buffer access shape,
the preroll/refill idiom taken from the SDK's own samples rather than
architecture.md's illustrative figure, the WU-15a/WU-15b split, and this
unit's own frame-source scope — frozen at WU-15a, Phase 3's second unit.**
`docs/architecture.md` 7's Output subsection sketches the mechanism
("`CreateVideoFrame()` and write into `GetBytes()`... `ScheduleVideoFrame()`
with a 3-frame preroll, then `StartScheduledPlayback()`... Refill from
`SetScheduledFrameCompletionCallback`") without fixing several things a
literal reading gets wrong or leaves open — the same kind of gap ADR-031
found for enumeration, found this time by reading `IDeckLinkOutput`/
`IDeckLinkVideoFrame`/`IDeckLinkMutableVideoFrame` themselves in
`DeckLinkAPI.h` and the SDK's own `FilePlayback`/`SignalGenerator` samples
(`DeckLinkPlaybackDevice.cpp`, `SyncController.mm`) rather than
architecture.md's summary alone, per this session's own brief. This entry
also records the WU-15 split `HANDOFF.md` flagged going into this session
as likely needed, decided *after* this reading, not before it — the same
order ADR-028 followed for WU-12a/WU-12b.

- **`GetBytes()` is not a method on `IDeckLinkVideoFrame`/
  `IDeckLinkMutableVideoFrame` at all.** architecture.md 7's "write into
  `GetBytes()`" reads as if it were direct; it is not. `DeckLinkAPI.h`
  declares `GetBytes()` on a separate interface, `IDeckLinkVideoBuffer`,
  obtained via `QueryInterface(IID_IDeckLinkVideoBuffer, ...)` from the
  frame `CreateVideoFrame()` returns, and bracketed by
  `StartAccess()`/`EndAccess()`. Confirmed directly against the real SDK's
  own `SignalGenerator` sample (`SyncController.mm`'s `ScopedBufferBytes`
  helper: `com_ptr<IDeckLinkVideoBuffer> buffer(IID_IDeckLinkVideoBuffer,
  frame); buffer->StartAccess(flags); buffer->GetBytes(&lockedMem); ...
  buffer->EndAccess(flags)`), not assumed from architecture.md's summary —
  exactly the discipline this session's own brief asked for. `src/io/
  decklink_output.cpp`'s `fillFrameBuffer()` follows this shape.
- **Row bytes: ask the SDK, do not assume this project's own
  `v210::rowBytesMin()` agrees with it.** architecture.md 7 already states
  this rule for the *input* side ("Always use `GetBytesPerRow()`; never
  compute row stride yourself") but says nothing about output; the same
  reasoning applies symmetrically, so `decklink_output.cpp` calls
  `IDeckLinkOutput::RowBytesForPixelFormat(bmdFormat10BitYUV, width,
  &rowBytes)` and uses that value for both `CreateVideoFrame()`'s
  `rowBytes` argument and how many bytes it reads from the source file —
  never `video::v210::rowBytesMin()`, which this file does not even
  include. `bmdFormat10BitYUV` is literally FourCC `'v210'`
  (`DeckLinkAPIModes.h`'s own comment), so the two values are expected to
  agree in practice; `tests/test_decklink_output.cpp`'s
  `test_v210_rowbytes_matches_project_own_computation()` checks this
  directly, once, against the real hardware, rather than leaving it an
  unstated assumption every other check in that file would otherwise rely
  on silently.
- **Frame source for this unit: raw packed `.v210` bytes read directly from
  disk via `<fstream>`, not `video::readV210File()`'s unpack path
  (WU-05).** `IDeckLinkOutput::CreateVideoFrame()`'s buffer for
  `bmdFormat10BitYUV` wants exactly the packed byte layout the file already
  contains — unpacking to `Sample` planes and repacking would be pure waste
  for this unit's own job of moving bytes from a file to a DeckLink buffer
  unchanged, and would pull in a `video/v210.hpp` dependency
  `decklink_output.cpp` does not otherwise need. `src/io/decklink_output.cpp`'s
  own `readRawFile()` mirrors `file_source.cpp`'s existing "read exactly N
  bytes, fail if short" idiom (WU-05), applied to a raw byte buffer instead
  of an unpacked image.
- **Preroll depth: half a second of frames, computed from the negotiated
  display mode's own frame rate — not architecture.md 7's illustrative
  "3-frame" figure.** Neither real sample uses 3 frames: `FilePlayback`'s
  `DeckLinkPlaybackDevice::play()` prerolls `round(m_frameRate / 2.0)`
  frames ("Preroll 1/2 second of frames," its own comment);
  `SignalGenerator`'s `SyncController.mm` `startRunning()` sets
  `selectedDevice.videoPrerollSize = framesPerSecond / 2` ("Set the preroll
  to 1/2 second of video frames"). architecture.md 7's "3-frame" reads as
  the *minimum* the mechanism needs to have anything queued at all, not a
  number either real sample actually ships. Chosen: follow the real
  samples' own idiom — `std::lround(framesPerSecond / 2.0)`, computed from
  `IDeckLinkDisplayMode::GetFrameRate()`'s own `frameDuration`/`timeScale`
  for whichever display mode was actually negotiated, not hardcoded to any
  one standard's frame rate.
- **Refill idiom: exactly one replacement frame scheduled per
  `ScheduledFrameCompleted()` call, not a batch refill.** Matches
  `FilePlayback`'s own shape exactly:
  `DeckLinkPlaybackDevice::ScheduledFrameCompleted()` calls
  `scheduleVideo()` once per completion. `LoopedFramePlayback::
  ScheduledFrameCompleted()` calls its own `scheduleOne()` once, the
  simplest mechanism that keeps the DeckLink-internal queue at a roughly
  constant depth with no extra bookkeeping (a target queue depth, a
  buffered-frame-count poll, etc. — none of it needed once refill is
  exactly "one out, one back in").
- **Stop sequencing: `StopScheduledPlayback()` then
  `SetScheduledFrameCompletionCallback(nullptr)` then
  `DisableVideoOutput()`, no condition-variable wait.** `SignalGenerator`'s
  own `stopRunning()` does exactly this, relying on `DisableVideoOutput()`'s
  own documented behaviour ("`DisableVideoOutput` will block until all
  scheduled frames are completed or flushed," that sample's own comment) —
  a simpler shape than `FilePlayback`'s own `stopScheduledPlayback()`,
  which additionally takes a mutex and waits on a condition variable for
  `ScheduledPlaybackHasStopped()`. Chosen: `SignalGenerator`'s simpler
  shape — this unit has no UI thread to keep responsive during the wait
  (`FilePlayback`'s own reason for the extra machinery, dispatching back to
  a main queue) and no audio stream to coordinate against, so the extra
  synchronization buys nothing here.
- **A real (non-trivial) `IUnknown` reference count on
  `LoopedFramePlayback` itself, not a shortcut.** It registers itself
  directly as the `IDeckLinkVideoOutputCallback` (`SetScheduledFrameCompletionCallback(this)`)
  rather than using a separate delegate object the way `SignalGenerator`'s
  `DeckLinkOutputDevice` does (`DeckLinkOutputCallback`, a distinct heap
  object) — one class playing both roles, since this unit has no UI object
  of its own for a delegate pattern to make sense against. `create()`
  heap-allocates it and adopts the constructor's own initial `m_refCount{1}`
  via `ComPtr::adopt()` (ADR-031's own "already one reference, take
  ownership, no `AddRef`" case, applied here to this project's own object
  rather than an SDK factory return) rather than the SDK's; `AddRef()`/
  `Release()` are real, atomic, delete-on-zero, matching the SDK's own
  `DeckLinkOutputCallback`/`DeckLinkPlaybackDevice` samples exactly —
  `SetScheduledFrameCompletionCallback()` takes ownership the same way any
  COM interface pointer handed to a callee for storage does, so this object
  must survive exactly as long as something (the SDK, or this project's own
  caller) holds a reference to it, not merely as long as `create()`'s own
  caller happens to keep its `ComPtr` alive.

**The WU-15a/WU-15b split.** `WORK-UNITS.md`'s WU-15 line, going into this
session, stated one accept criterion — "one hour on a broadcast monitor, no
dropped frames" — directly copied from architecture.md 10's own Phase 3
"done when" line. `HANDOFF.md`'s own note flagged this, going into this
session, as "exactly the kind of real capture/playback smoke test... worth
scoping, and splitting if needed... *before* writing any implementation
code for it," the same instruction this session's own brief repeated. Read
*after* the SDK research above, not before it (same order ADR-028 used for
WU-12a/WU-12b): an hour-long unattended run is not something this session
can itself assert green — no hardware access from the cloud sandbox, and
even at the real terminal an hour exceeds what "one session, one work unit"
(`SESSION-PROTOCOL.md`) sensibly means by a single sitting. Split into:

- **WU-15a (this session).** `src/io/decklink_output.hpp`/`.cpp` (new) —
  `LoopedFramePlayback`, the mechanism above. `tests/test_decklink_output.cpp`
  (new) — builds one genuinely warped frame (a cylinder over a zone plate,
  reusing WU-11's `buildCylinderLattice()` and WU-10's `runFrameFile()`,
  exactly as any earlier unit's own tests already do) and writes it to a
  real `.v210` file, so "file source" is genuine, not simulated; enumerates
  the real UltraStudio 4K Mini (WU-14's `enumerateDeckLinkDevices()`,
  unchanged) and obtains its `IDeckLinkOutput` via `QueryInterface`; runs
  `LoopedFramePlayback` for a bounded few-second window; asserts zero
  `bmdOutputFrameDisplayedLate`/`bmdOutputFrameDropped` results and at least
  one `bmdOutputFrameCompleted`, and a clean `stop()`. This is "get one
  static warped frame scheduled and playing," the first half of
  architecture.md 10's own Phase 3 "done when" line ("warped frames appear
  on a broadcast monitor") — confirmed once, by eye, at the real terminal
  while the bounded test runs; the automated checks confirm the DeckLink
  side's own mechanics (no dropped/late frames over that window, clean
  start/stop), not what is actually on the wire, the same division of
  labour ADR-031's own `HANDOFF.md` note already draws between what a test
  can assert and what a human confirms by hand.
- **WU-15b (not this session, not yet scheduled in `WORK-UNITS.md`'s own
  numbering beyond this note).** The literal, still-unmet half of
  architecture.md 10's own Phase 3 criterion: run the same
  `LoopedFramePlayback` mechanism unattended for one hour on the real
  hardware, confirm `stats().dropped`/`stats().displayedLate` are still
  zero and `stats().completed` is consistent with an hour's worth of
  frames at the negotiated frame rate, "stable for an hour." This is not
  implementation work — WU-15a's own mechanism, unchanged, run for longer
  — so it is not scoped with new `Files:`/`Accept:` source-file lines the
  way every other work unit in this file is; it is Steve's own hands-on
  verification step (start it, leave it running, report the logged
  dropped-frame count back), the same category of thing HANDOFF.md's own
  "Environment check" section already asks for by hand (Desktop Video
  Setup, Media Express), not a session's own job to assert green from a
  terminal.

**Display mode: `bmdModePALp`, not `bmdModePAL`.** ADR-007 names both
"576i25 / 576p25" as this project's development standard without choosing
between them for output. This project has no de-interlace/field-split
machinery yet (`WORK-UNITS.md`'s WU-23, Phase 6, not built) — every frame
`runFrame()`/`runFrameFile()` produces today is one whole progressive
raster — so playing it out as one whole progressive raster
(`bmdModePALp`) is the honest match; `bmdModePAL` would transmit the same
progressive content as an interlaced signal with no field-splitting step
ever run on it, which is not what "576i25" is supposed to mean even though
a *static* test frame would not visibly show the difference. Whether the
UltraStudio 4K Mini actually supports `bmdModePALp` with `bmdFormat10BitYUV`
is **unverified this session** — `LoopedFramePlayback::create()` checks
`DoesSupportVideoMode()` itself and fails cleanly (a null `ComPtr` result,
not a crash) rather than assuming; if it turns out unsupported at the real
terminal, `bmdModePAL` is the documented fallback (this project's own
progressive-only limitation would then mean picking a supported progressive
HD mode instead, or accepting interlaced transmission of progressive
content for this one smoke test only — a decision for whichever session
next touches this, not resolved here).

**This entire unit is unverified by this session — deliberately, same
reason ADR-031 gives for WU-14.** No Blackmagic SDK and no AppleClang/Xcode
toolchain exist in the Linux cloud sandbox this session drafted in;
`decklink_output.hpp`/`.cpp` and `tests/test_decklink_output.cpp` are
reasoned through against the real SDK headers and samples (this entry's own
citations above) rather than compiled and run by this session at all.
`WORK-UNITS.md`'s WU-15a line stays `wip`, not `green`, until built and run
at the real terminal.

Not decided here, deliberately: WU-15b's own eventual `WORK-UNITS.md` entry
number and exact reporting format (a log line, per this unit's own
`PlaybackStats`, is enough for now); decoding a real multi-frame file
sequence, as opposed to looping one static frame (a later, not-yet-named
unit's job — WU-15a's own frame source is deliberately the minimal thing
that is still honestly "file source to SDI out," per the user's own framing
of the split going into this session); and anything about live capture
input (WU-20, Phase 5, untouched).

Does not reopen `docs/architecture.md`, ADR-007, ADR-013, ADR-021 or
ADR-031 — same relationship every ADR since ADR-020 has to the document;
ADR-007's "576i25 / 576p25" naming is read literally (both named, output
picks progressive for the stated reason) rather than amended; ADR-021's
`scatter-core`/`scatter` file-placement precedent and ADR-031's `ComPtr::
adopt()`/enumeration are reused unaltered, not modified, by this unit's own
code.

**ADR-033 — Display mode confirmed: `bmdModePAL`, not `bmdModePALp`.
Completes, not reopens, ADR-032's own deferred fallback note.**
ADR-032 named `bmdModePALp` as WU-15a's first choice, explicitly flagged it
"unverified this session," and named `bmdModePAL` as "the documented
fallback... a decision for whichever session next touches this." That
session is this one, continuing the same sitting rather than a later one:
the real terminal's first run of `test_decklink_output` failed at
`LoopedFramePlayback::create()` with no signal beyond a null result, so
`decklink_output.cpp`'s `startWith()` gained per-step `stderr` diagnostics
(temporary — see its own header comment; to be folded into `PlaybackStats`
or dropped once this unit is fully green, not left as permanent scaffolding)
before asking for a second run. That run's own output is the evidence this
entry freezes:

```
LoopedFramePlayback::startWith: DoesSupportVideoMode failed, hr=0x00000000 supported=false (displayMode=0x70616c70)
```

`hr == S_OK` (`0x00000000`) with `supported == false` — the call itself
succeeded; the UltraStudio 4K Mini's driver simply does not offer
`bmdModePALp` (`'palp'`, confirmed by the printed `displayMode` value) in
combination with `bmdFormat10BitYUV` via `bmdVideoConnectionUnspecified`/
`bmdNoVideoOutputConversion`. Not a code defect — `DoesSupportVideoMode()`
did exactly its job, and `LoopedFramePlayback::create()` failed cleanly (a
null `ComPtr`, not a crash or hang) exactly as ADR-032 already required of
it. `tests/test_decklink_output.cpp`'s `kDisplayMode` changes to
`bmdModePAL`; ADR-032's own reasoning for why this is an honest substitution
for a *static* test frame (both fields of an interlaced-transmitted frame
still come from the one unchanging buffer WU-15a plays back, so there is no
combing artifact to reveal the substitution) is unchanged and not repeated
here.

Not logged in `CORRECTIONS.md`: nothing was claimed and found wrong.
ADR-032 explicitly declined to assert `bmdModePALp` would work — it named
the risk and pre-selected a fallback precisely because it could not verify
either from the cloud sandbox — so this session hitting that named risk and
using the named fallback is the mechanism working as designed, not a
correction to an earlier claim. Same distinction `HANDOFF.md`'s own
"Corrections this session" section drew for WU-14's session: an item
flagged unverified resolving (here, via its own documented fallback rather
than the primary choice working outright) is not the same thing as an
earlier claim turning out wrong.

Does not reopen ADR-032 — completes its own explicitly-deferred fallback
decision, the same relationship ADR-029 has to ADR-028's own deferred
opacity-mechanism sketch.

**ADR-034 — UltraStudio 4K Mini hardware fault: pause work against it;
UltraStudio Monitor 3G becomes the active hardware target for continued
WU-15a/WU-15b verification. Does not supersede ADR-006 or ADR-013.**

Immediately after the passing run recorded in ADR-033 (`test_decklink_output`
green, 5.34s), Steve ran the full suite (`ctest --test-dir build
--output-on-failure`). Two tests failed — `test_decklink_device` and
`test_decklink_output`, both on `CHECK(!devices.empty())` — and the
UltraStudio 4K Mini was found completely unresponsive: absent from Desktop
Video Setup, absent from Media Express, absent from Apple's own Thunderbolt
System Report (not merely "no signal" — not enumerated as a Thunderbolt
device at all), and no longer providing USB-PD/Thunderbolt power passthrough
to the MacBook Pro. Steve diagnosed this by hand: restarting the Blackmagic
driver helper did not recover it; swapping the spare UltraStudio Monitor 3G
onto the same Thunderbolt port and cable worked immediately and normally.
That swap isolates the fault to the 4K Mini unit itself — not the Mac, not
the port, not the cable, not the driver install.

**Causation assessment, stated with its actual limits, not overclaimed
either way.** This project's code — `decklink_device.cpp`/`.hpp` (WU-14) and
`decklink_output.cpp`/`.hpp` (WU-15a) — calls only the DeckLink SDK's public,
documented API surface: `IDeckLinkIterator` enumeration,
`IDeckLinkProfileAttributes` capability queries, `DoesSupportVideoMode()`,
`EnableVideoOutput()`/`DisableVideoOutput()`, `CreateVideoFrame()` plus
`IDeckLinkVideoBuffer::GetBytes()`/`StartAccess()`/`EndAccess()`,
`ScheduleVideoFrame()`, `StartScheduledPlayback()`/`StopScheduledPlayback()`,
and `SetScheduledFrameCompletionCallback()`. None of that surface reaches
firmware, PCIe/Thunderbolt bus (re-)enumeration, or USB-PD/Thunderbolt power
negotiation — those are the driver's and the device's own firmware's job,
below anything a user-space SDK client can touch. This is a materially
different failure from the one named risk architecture.md 12 already
carries ("a reference-count leak locks the device until reboot"): that risk
describes a *driver-state* lockout where the device stays hardware-visible
but refuses new sessions until the process (or host) restarts — recoverable
by a reboot, and within reach of this project's own `ComPtr` refcounting
being wrong. What was actually observed — gone from the Thunderbolt bus
itself, no power passthrough — is a hardware/firmware/power-level fault, not
a refcount leak, and reboot-recoverability was never established either way
before this session's own hardware access ended. On the evidence available
(clean, fast, fully-passing single-test run; unit dead only after a
later, broader suite run that itself passed its non-hardware-dependent
tests normally; the fault isolated to this one unit via direct swap) it is
judged **unlikely** this project's code caused it, but this is not proven
with certainty — a genuinely rare coincidence (the unit's first-ever real
signal-output use surfacing a pre-existing hardware fault) cannot be ruled
out from software evidence alone. No corresponding `CORRECTIONS.md` entry:
nothing this project earlier claimed about the SDK or the hardware has been
shown wrong by this incident.

**Decision:** pause further work against the 4K Mini. For the remainder of
WU-15a's own verification (full-suite green, `close.sh 15a`, Steve's by-eye
confirmation) and for WU-15b when it starts, the **UltraStudio Monitor 3G**
becomes the active hardware playback target, until the 4K Mini's status is
resolved (Blackmagic support and/or a hardware check, both Steve's own next
steps, not this project's). **No code change follows from this pivot.**
`decklink_device.cpp`'s enumeration (WU-14) and
`firstPlaybackCapableOutput()` in `tests/test_decklink_output.cpp` (WU-15a)
already select by `IDeckLinkProfileAttributes`/`supportsPlayback` and
`QueryInterface(IID_IDeckLinkOutput)`, not by model name or a 4K-Mini-
specific assumption — the Monitor 3G, once it is the (or the first)
playback-capable device the SDK's own iterator returns, is picked up by the
existing code unchanged. What genuinely is unverified and becomes this
pivot's own open question: whether `DoesSupportVideoMode(bmdModePAL,
bmdFormat10BitYUV)` succeeds on the Monitor 3G the way it did on the 4K
Mini (ADR-033) — the Monitor 3G is a simpler, output-only device (no
capture inputs at all, per architecture.md's own description of it as a
diagnostic-coverage-view target, ADR-011) and its supported mode list is not
assumed to be a superset or subset of the 4K Mini's without checking.

**Relationship to earlier ADRs, stated precisely:** does not supersede
ADR-006 ("Host is the M1 Max MacBook Pro with UltraStudio 4K Mini") or
ADR-013 ("Single machine... with the UltraStudio 4K Mini attached") — those
describe this project's primary target hardware for the reasons given there
(full duplex, 12G headroom, composite/component inputs the Monitor 3G does
not have), and nothing about this incident changes that the 4K Mini remains
the intended hardware once it is working again. Nor does it amend ADR-011
(diagnostic coverage view to the Monitor 3G or the Mac's own display, not a
second SDI output) — this pivot uses the Monitor 3G in a role beyond that
one (as the primary WU-15a/WU-15b verification target, not a secondary
coverage view) for as long as the 4K Mini is unavailable, which is a
hardware-availability-driven substitution, not a redesign of what either
device is for. If the 4K Mini does not recover, a future session would need
its own ADR to actually change the project's primary-hardware decision;
this entry does not make that call.

**ADR-035 — `test_decklink_device`'s full-duplex check is expected to fail
while the UltraStudio Monitor 3G is the only attached device. Does not
block WU-15a's own close; does not reopen WU-14.**

The first full-suite run against the Monitor 3G (ADR-034's pivot) reported
15 of 16 passing: `test_decklink_output` passed both its checks (5.13s),
and every one of `test_smoke` through `test_testpat` (unaffected by any
hardware) passed as always. `test_decklink_device` failed one check —
`test_at_least_one_device_is_full_duplex`, `foundDuplexDevice` staying
false (`test_decklink_device.cpp:53`).

Not a defect. That check exists to confirm architecture.md 7's own claim,
"The UltraStudio 4K Mini is full duplex: one `IDeckLink` exposing both
`IDeckLinkInput` and `IDeckLinkOutput`" — a fact about the 4K Mini, never
claimed to hold for every device this project might ever enumerate. The
Monitor 3G is playback-only by design (no capture input at all — already
noted in ADR-034 and, before that, in architecture.md's own description of
it as a diagnostic-coverage-view target, ADR-011); `QueryInterface` for
`IDeckLinkInput` on it correctly fails, so `foundDuplexDevice` correctly
stays false. The test is doing its job — this is the real, current state
of the attached hardware, accurately reported, not a bug in
`decklink_device.cpp`, `ComPtr`, or the test itself.

**WU-14 is not reopened.** `wu-14-green` recorded a real, passing run
against the 4K Mini at the time; nothing about swapping which device is
physically attached now makes that historical result untrue, the same
principle already applied to ADR-032/033 by ADR-034 (a later hardware
change doesn't retroactively unmake an earlier verified result).

**Decision:** WU-15a's own close does not require
`test_decklink_device`'s full-duplex check to pass while the Monitor 3G is
the only device attached — that check was never part of WU-15a's own
`Accept:` criteria, which is playback-only. `./tools/close.sh 15a`'s
full-suite run is expected to show this one specific, understood exception
(15/16, not 16/16) for as long as the 4K Mini remains unavailable; this is
not a blanket "ignore failures" policy — any *other* unexplained failure in
that run still blocks the close exactly as it always would. If the 4K Mini
recovers, the full suite should return to 16/16 and this exception no
longer applies — worth re-checking then, not assumed permanent. No code
change follows: weakening `test_decklink_device.cpp`'s own duplex check to
tolerate a non-duplex device would erase real regression coverage for the
4K Mini (this project's actual target hardware) for no benefit.

**ADR-036 — WU-15a warp-visibility false alarm, closed: root cause was
4:3-on-16:9 display scaling, not a code defect. No code change results.**

Steve's first by-eye check of the Monitor 3G run (WORK-UNITS.md's own
WU-15a entry) reported a plain, undistorted zone plate — not the cylinder
warp `tests/test_decklink_output.cpp` is supposed to produce. Investigated
in stages, each one real evidence rather than a guess:

1. `writeWarpedTestFrame()`'s own steps (`makeZonePlate` ->
   `buildCylinderLattice` -> `runFrameFile`) were reproduced standalone —
   g++, no CMake, no Blackmagic SDK, in the device bridge's own Linux VM —
   and the resulting file dumped to a PNG. Clearly, strongly warped: a
   vertically-elongated oval with black letterboxing on both sides. Ruled
   out `runFrameFile`/`buildCylinderLattice` as the cause.
2. The *actual* file on Steve's own Mac (`/tmp/scatter_wu15a_frame.v210`,
   written by the real test run) was converted the same way, via a
   throwaway tool built with his own AppleClang. Also clearly warped,
   confirming the pipeline is correct on the real machine too, not just in
   the Linux reproduction.
3. Steve still reported the plain zone plate after both of the above.
   Temporary checksum instrumentation was added to
   `decklink_output.cpp` (`startWith()`, `fillFrameBuffer()`, and
   `ScheduledFrameCompleted()` reading its own `completedFrame` argument
   back through a fresh `StartAccess`/`GetBytes`) and run for real. The
   checksum was identical at every stage this project's own code and the
   DeckLink SDK's own API can observe: disk read, buffer write, buffer
   read-back, and the SDK's own completion-callback readback on the first
   three actual completions. Cross-checked further: that exact checksum,
   computed independently on the Linux reproduction from step 1, matched
   byte-for-byte (`0x5fac3fb42c5f7d48`, 1 105 920 bytes both places). This
   is as deep as any user-space API call can verify, and it came back
   clean at every point.
4. Steve asked to try a different shape/warp amount before accepting that
   conclusion — reasonable and cheap, so `writeWarpedTestFrame()` was
   swapped to a sphere (both axes, pi/2, stronger than the cylinder's
   pi/3) as a diagnostic, confirmed warped the same way as step 1, and run
   for real with the same checksum instrumentation still in place.
5. Steve's own conclusion, on looking again at both the cylinder and
   sphere results: both are actually fine. He was watching a 720x576
   4:3-ish SD frame on a 16:9 monitor; the display's own handling stretches
   the active picture back out horizontally, and that stretch had been
   making the cylinder warp's own horizontal compression (real, baked into
   the pixel content — architecture.md's whole point) look deceptively
   close to un-warped at a glance. Recognising the sphere result for what
   it actually was ("stretched to 16:9") is what surfaced this.

**Root cause: display-side aspect handling on Steve's own monitor,
misread at a glance — not a defect anywhere in this project's code, and
not a `CORRECTIONS.md` entry** (nothing this project claimed was shown
wrong; the pipeline, `decklink_output.cpp`, and the SDK's own API all
checked out at every stage this session could verify). **No code change
results.** The diagnostic checksum instrumentation in `decklink_output.cpp`/
`.hpp` and the sphere swap in `tests/test_decklink_output.cpp` are both
reverted; the unit is back to exactly ADR-032's own cylinder design. One
durable change: `tests/test_decklink_output.cpp`'s own header comment now
warns about this exact false-alarm mode (4:3 content on a 16:9 monitor)
so a future by-eye confirmation doesn't repeat the same investigation.

WU-15a's own `Accept:` by-eye clause is satisfied by this — Steve has now
confirmed the warped frame is visible, on both shapes tried, accounting
for his monitor's own scaling. Full suite + `./tools/close.sh 15a` on the
reverted build is still Steve's own next step, not this entry's to claim.

**ADR-037 — Going-forward target hardware is a two-device split: UltraStudio
Monitor 3G for output, UltraStudio Recorder 3G for input. Supersedes
ADR-006's specific device choice and ADR-011's "spare" framing of the
Monitor 3G. Does not reopen ADR-013.**

Steve's own decision, stated directly at the end of this session: going
forward, output uses the UltraStudio Monitor 3G and input uses the
UltraStudio Recorder 3G — two separate devices, not one full-duplex unit.
Both are already in hand, not a future purchase — this is the project's
real target hardware now, not a provisional workaround. The UltraStudio 4K
Mini (ADR-006's original choice, unresponsive since the incident this
session — ADR-034) is **on hold pending a PSU replacement**: not retired
outright, but not part of the active plan either, and its return (if it
comes) does not revert this decision.

That PSU diagnosis is also worth folding back into ADR-034's own causation
assessment, without reopening it: a failed power supply is exactly the
kind of mundane, physical, hardware-level fault ADR-034 already judged
most likely and explicitly distinguished from anything this project's code
could reach (no code path touches power delivery). This doesn't change
ADR-034's conclusion, just gives it a concrete, plausible real-world shape.

**Relationship to earlier ADRs, stated precisely, the same discipline
ADR-034 itself used:**

- **Supersedes ADR-006's device choice** ("Host is the M1 Max MacBook Pro
  with UltraStudio 4K Mini... full duplex... 12G headroom... composite and
  component analogue inputs") — the going-forward hardware is neither full
  duplex on one device nor does it carry the 4K Mini's analogue inputs;
  ADR-006's other reasoning (CPU first, Metal deferred based on the M1
  Max's own compute headroom) is about the *host*, not this device choice,
  and is unaffected.
- **Does not reopen ADR-013** ("Single machine... no second build host, no
  cross-machine verification") — still one Mac, still one development
  environment; what changes is how many DeckLink devices are attached to
  it (two, not one), not how many machines are involved.
- **Supersedes ADR-011's framing of the Monitor 3G** as "the spare...not a
  second SDI output," scoped to a secondary diagnostic-coverage-view role
  alongside a primary full-duplex 4K Mini. The Monitor 3G is now the
  primary output device outright, not a spare. Where the diagnostic
  coverage view itself goes with this hardware in place — the Mac's own
  display alone, now that the Monitor 3G is spoken for, or something else
  — is not decided by this entry; a future session picks that up if it
  becomes relevant again.

**Concrete follow-ups this entry surfaces but does not resolve** (no
unscoped code in this entry, per this project's own discipline — these are
named for whichever session picks them up next, not solved here):

1. `tests/test_decklink_device.cpp`'s `test_at_least_one_device_is_full_duplex`
   (WU-14) checks a fact — architecture.md 7's "The UltraStudio 4K Mini is
   full duplex: one `IDeckLink` exposing both `IDeckLinkInput` and
   `IDeckLinkOutput`" — that will never be true of this project's actual
   going-forward hardware (Monitor 3G and Recorder 3G are two separate
   devices, neither full duplex). ADR-035 treated this check's failure as
   a temporary, hardware-availability exception while the 4K Mini was
   simply absent; this entry means that framing no longer fits — the
   check's own premise doesn't describe the intended hardware at all
   anymore, not even once the 4K Mini's PSU is sorted. WU-14 itself stays
   `green` (`wu-14-green` recorded a true result against hardware attached
   at the time, same principle ADR-034/035 already applied) — but a future
   session should decide whether to retire this specific check, rescope
   what "full duplex" means for a two-device architecture, or something
   else. Not decided here.
2. Genlock (ADR-010, "free-running... an unlocked source drifts against
   the output... the 4K Mini has a sync input, so one BNC of black burst
   resolves it whenever it matters") was reasoned about for one device
   sharing one internal clock domain between its own input and output.
   Two independent devices have no such shared clock by construction —
   drift between capture and playback may matter more, sooner, than
   ADR-010 anticipated. Not resolved here; worth revisiting once input
   (the Recorder 3G) actually gets exercised by this project's own code,
   not before.
3. `WORK-UNITS.md`'s WU-15b (`todo`) and any future capture-side work
   should target the Recorder 3G/Monitor 3G split directly rather than
   describing a single device, once either is actually touched by new
   `Files:`/`Accept:` scoping.

**ADR-038 — WU-15b's own duration mechanism: hand-edit the existing
bounded-run literal for one temporary, uncommitted run, not a CLI arg or
environment variable. Completes an implicit gap in ADR-032's "not
implementation work" framing; does not reopen it.**

ADR-032 already fixed WU-15b's own shape precisely: "WU-15a's own
`LoopedFramePlayback` mechanism, unchanged, run for longer... not scoped
with new `Files:`/`Accept:` source-file lines the way every other work unit
in this file is." What it left unstated is *how* "run for longer" is
actually invoked — `tests/test_decklink_output.cpp`'s
`test_looped_playback_runs_with_no_dropped_or_late_frames()` hardcodes the
bounded run's own length as a single literal,
`std::this_thread::sleep_for(std::chrono::seconds(5))` (line 168), with no
parameter anywhere that already lets a caller ask for an hour instead. Two
candidate mechanisms were weighed before touching anything, per this
session's own brief:

- **A configurable duration — CLI arg or environment variable.** Rejected.
  This is new implementation by ADR-032's own already-frozen standard, not
  an exception to it: a new parameter needs a name, a type, a default,
  validation (what happens to a negative or absurd value), and a decision
  about whether it also changes WU-15a's own `close.sh`-driven ~5-second
  smoke-test invocation or only a separate manual one — none of which
  `architecture.md` or any existing ADR speaks to, all of which WU-15b's
  own `WORK-UNITS.md` line already declined to scope with a
  `Files:`/`Accept:` pair. It also introduces a real foot-gun the
  hardcoded literal does not have: if the default value were ever set to
  anything but the committed 5 seconds, or an environment variable meant
  for one manual run leaked into a later `close.sh` invocation's own
  environment, every future work unit's own `close.sh` run would silently
  hang for up to an hour waiting on one test, with nothing in `close.sh`
  itself (which only gates on git-dirty, an existing tag, build failure or
  test failure, never on suspiciously long runtimes) to catch it.
- **Hand-edit the existing literal, once, for exactly one manual
  invocation — not committed to git, reverted immediately after.** Chosen.
  WU-15a's own mechanism is genuinely unchanged; the only thing different
  is how long an already-frozen, already-verified bounded-run test sleeps
  before calling `stop()`, for one run Steve invokes directly and
  discards. This needs no new parameter, no default-value decision and no
  interaction question with any other caller, because there is no other
  caller — `close.sh`'s own `ctest` run always executes whatever the
  committed literal says, so its cost stays fixed at ADR-032's own
  ~5-second figure forever, regardless of what any one manual run
  temporarily does to a dirty working tree. This is the same "temporary,
  purpose-specific edit, run, then discard" shape ADR-036's own checksum
  instrumentation and sphere-warp diagnostic already used in this project
  — the only difference is those were committed and later reverted across
  several commits, appropriate for a multi-step investigation spanning a
  session's own narrative; this is a single, self-contained
  edit-build-run-revert cycle within one sitting, so it does not need its
  own commit at all.

**Mechanics, frozen:**

- **Edit:** `tests/test_decklink_output.cpp` line 168,
  `std::this_thread::sleep_for(std::chrono::seconds(5));` →
  `std::this_thread::sleep_for(std::chrono::seconds(3600));`. Nothing else
  in the file changes — this is the file's only bounded-run-length literal
  (the two comments mentioning "five seconds"/"a few seconds", lines 18
  and 164, are descriptive prose, not code, and are irrelevant to the
  run's actual behaviour either way).
- **Build:** `cmake --build build` (incremental; picks up the one changed
  translation unit).
- **Run:** the `test_decklink_output` binary directly
  (`./build/test_decklink_output`) or `ctest --test-dir build -R
  test_decklink_output --output-on-failure` — either invokes the same
  binary. **Not `./tools/close.sh`**, for two independent reasons: its own
  git-dirty gate would refuse to run at all while the edit is uncommitted
  (correct behaviour, not a bug to route around), and even if the tree
  were clean, `close.sh`'s own job is tagging a work unit `green` against
  `Files:`/`Accept:` criteria WU-15b was explicitly never scoped with
  (ADR-032) — there is nothing for it to tag.
- **Revert:** `git checkout -- tests/test_decklink_output.cpp` (or `git
  restore tests/test_decklink_output.cpp`) immediately after the run,
  before doing anything else in the repository. `git status` should be
  clean again, matching `wu-15a-green`'s own committed content exactly,
  before any future `close.sh` run for a later work unit — otherwise that
  gate would incorrectly refuse on what looks like an unrelated dirty
  tree.
- **No commit for the edit/revert pair itself.** Unlike ADR-036's own
  investigation, this is not a multi-commit narrative worth checkpointing
  — the edit exists only to make one binary sleep longer before its own
  already-frozen `stats()` checks run, and reverting it before the next
  session (or the next `close.sh` invocation) leaves no trace to record.

**Not decided here, deliberately:** whether a real configurable duration is
ever worth building as its own future unit, if repeated multi-hour runs
turn out to be needed often enough that hand-editing becomes tedious —
flagged as a possible future convenience, the same way ADR-037 names
follow-ups without resolving them, not decided or scoped now.

Does not reopen `DECISIONS.md` ADR-032 — completes a gap that entry left
implicit (it fixed WU-15b's *scope*, not its *invocation mechanism*), the
same relationship ADR-033's `bmdModePAL` confirmation and ADR-029's
`compositeLayered()` freeze both already have to their own respective
"not decided here" notes.

**ADR-039 — Names the UltraStudio Recorder 3G, not a generic "input
device," as the target for all future capture-side work. Completes
ADR-037's own third named follow-up; does not scope WU-20.**

ADR-037 already decided the going-forward hardware split — UltraStudio
Monitor 3G for output, UltraStudio Recorder 3G for input — and named three
follow-ups it explicitly declined to resolve itself, the third being:
"`WORK-UNITS.md`'s WU-15b (`todo`) and any future capture-side work should
target the Recorder 3G/Monitor 3G split directly rather than describing a
single device, once either is actually touched by new `Files:`/`Accept:`
scoping." WU-15b (session 18) already exercised the Monitor 3G half of
that by name, with no ambiguity — its own `Accept:` line and every result
reported named the Monitor 3G throughout, never a generic "the output
device." The input half was still open: nothing in this project's own
tracked state (`WORK-UNITS.md`'s bare WU-20 heading, `docs/architecture.md`'s
own Input subsection) named the Recorder 3G anywhere; both still describe
input purely in terms of the (now paused, ADR-034/037) UltraStudio 4K
Mini.

**Decided now, ahead of WU-20's own scoping, at Steve's own request: the
UltraStudio Recorder 3G is the named input target for all future
capture-side work in this project** — WU-20 (DeckLink input, format
detection, ring buffer), WU-21 (full loop through), WU-22 (diagnostic
coverage view), and anything else Phase 5 touches. Nothing else about
Phase 5's own design changes here — no `Files:`/`Accept:` lines are
written by this entry, and WU-20 itself remains `todo`, unscoped, for
whichever future session actually starts writing `src/io/decklink_input.cpp`
to pick up per `SESSION-PROTOCOL.md`'s own discipline (read the real SDK's
`IDeckLinkInput`/capture-callback shape first, the same order ADR-031 and
ADR-032 already used for enumeration and output, rather than assume from
architecture.md's own summary). This entry only fixes *which physical
device* that future scoping targets, closing the specific ambiguity
ADR-037's own follow-up #3 named.

**`docs/architecture.md` is not edited by this entry.** Section 7's Input
subsection (and its scattered "4K Mini" references — the full-duplex claim
at lines 303-304, "confirm the 4K Mini enumerates with both input and
output" at line 436, among others) still describes the original
single-full-duplex-device design ADR-006 named, unrevised. Consistent with
this project's own established convention: no ADR since ADR-020 has ever
edited `architecture.md` itself, including ADR-037, which superseded
ADR-006's device choice without touching the document's own text —
`DECISIONS.md`'s own ADRs are what a session actually trusts for current
hardware truth, `architecture.md` remains reference material for the
original design intent, and a future session reading its Input subsection
should already know (from `SESSION-PROTOCOL.md`'s own reading order) to
check `DECISIONS.md` for whether any later ADR superseded what it says,
the same way this entry and ADR-037 both do for its now-stale
4K-Mini-specific hardware claims.

**Genlock (ADR-037's own second follow-up) is not addressed here** — still
deferred to whichever session actually touches the Recorder 3G's own code,
per ADR-037's own reasoning; naming the device does not by itself require
reasoning about its clock domain.

Does not reopen ADR-006, ADR-013, ADR-034 or ADR-037 — extends ADR-037's
own device-naming decision to the one place it explicitly left open (the
input device's own name in this project's *forward-looking* documentation,
`WORK-UNITS.md`), the same "completes, does not reopen" relationship
ADR-033/035/038 already have to the entries they complete.

**ADR-040 — Thread pool, QoS, per-worker bin arenas: scoped to PASS 2
alone (tile-parallel splat/resolve/composite); `core/pipeline.hpp` arrives
per ADR-026's own anticipation; PASS 1's row-band parallelism is deferred
to WU-16b. Frozen at WU-16a, Phase 4's first unit.**

`WORK-UNITS.md`'s own WU-16 line, going into this session, was bare: a
title ("Thread pool, QoS, per-worker bin arenas") and one accept
criterion ("8-thread output bit-identical to single-threaded (I6)"), no
`Files:` line — this session's own first job, per Steve's own brief, was
real scoping before any code. `docs/architecture.md` section 6 already
describes a fuller design than that one line names — two parallel passes,
not one:

```
8 worker threads, QoS USER_INTERACTIVE.
  Pass 1 partitions the source by row bands. Each worker owns a private
  per-tile bin arena, preallocated, bump-allocated.
  Barrier.
  Pass 2 partitions by tile. Each worker reads all 8 workers' lists for
  its tiles, in fixed worker order.
```

plus a capture-callback thread and an output-scheduler thread that belong
to the real-time device pipeline (Phase 5, not this unit — WU-16 is pure
`src/core/`, no DeckLink, per `HANDOFF.md` going into this session).

**Why this session builds PASS 2's tile-parallelism only, not both
passes.** `core/binner.cpp`'s `generateFragments()` (WU-08, frozen green)
loops `for (py = 0; py < src.height; ++py)` and uses that same
`src.height` as the *denominator* in `pixelToLattice(py, src.height)`
(ADR-024) — the row-loop bound and the lattice-parameter scale are the
same field, not two independently variable things. Partitioning PASS 1 by
row bands the way architecture.md section 6 sketches therefore cannot be
done by calling the existing `generateFragments()` once per band with a
shorter `SourceRaster::height`: that would change `v`'s own denominator
along with the loop bound, corrupting the lattice mapping for every row
in the band. The honest fix is a row-range parameter on
`generateFragments()` that leaves `src.height` itself untouched for the
`v` calculation — straightforward in itself, but it touches
`core/binner.hpp` *and* `core/binner.cpp`, which together with
`core/pipeline.hpp` (new) and `core/pipeline.cpp` (touched) would be four
source files before this unit's own test, one over
`SESSION-PROTOCOL.md`'s "touch at most 3 source files plus its test" cap
— and `core/binner.hpp`/`.cpp` is WU-08's own frozen interface, not this
unit's to casually reopen just to fit a parallelism scheme in. Chosen,
the same "split when the full scope doesn't fit" discipline ADR-028
(WU-12a/WU-12b) and ADR-032 (WU-15a/WU-15b) already used: this session
(WU-16a) builds PASS 2's tile-parallelism only, which needs no
`core/binner.*` change at all; a future WU-16b builds PASS 1's row-band
parallelism, and is named, not built, here (see below).

**Why PASS 2 alone still honestly satisfies WORK-UNITS.md's own accept
line.** "8-thread output bit-identical to single-threaded (I6)" is a
statement about `runFrame()`'s own output for a given `PipelineParams`,
not a requirement that every internal stage be threaded — WU-16a's own
`tests/test_threading.cpp` checks the literal line directly, running
`runFrame()` at `threads == 1` and `threads == 8` (among others) over a
genuinely warped, multi-tile frame and checking every destination pixel's
`Y`/`Cb`/`Cr` bit-for-bit. PASS 2 (bank-resolve, normalise, composite) is
also where `core/binner.cpp`'s tile binning (ADR-002, ADR-024) already
partitions work by tile — `TileBins::tile(tx, ty)` is already one
independent `std::vector<Frag>` per tile, populated in full before PASS 2
starts — so "the natural unit of per-thread work," per this session's own
brief, was already sitting there without needing a new partitioning
scheme invented for it. PASS 1 stays exactly WU-08's own single-threaded
loop, called once, synchronously, before any worker thread is started —
untouched, unparallelised, and therefore carrying zero risk to WU-08's
own frozen correctness.

**`core/pipeline.hpp`, new — arrives exactly where ADR-026 said it
would.** ADR-026 (WU-10) declared `runFrame()`/`runFrameFile()` in
`core/resolve.hpp` instead of a `pipeline.hpp` of their own, precisely
because "the thread pool and barriers [section 8's module-layout sketch]
describes are WU-16's (Phase 4), not built yet... When WU-16 actually
adds thread-pool state, `pipeline.hpp` arrives with it and these
declarations move there." They do not move: ADR-026 also said "nothing
about `runFrame()`/`runFrameFile()`'s own signatures is expected to
change when that happens," and nothing does — both keep their exact
WU-10 signatures, still declared in `core/resolve.hpp`. `pipeline.hpp`
arrives holding only the new thread-pool machinery
(`ThreadPool`, `setWorkerQoS()`) that `core/pipeline.cpp`'s own
`runFrame()` now uses internally.

**`ThreadPool`: persistent workers, a generation-counter dispatch/barrier,
not a task queue.** A fixed-size pool of `numThreads` `std::thread`
workers, spawned once by the constructor and joined once by the
destructor — "8 worker threads" as a standing resource, matching
architecture.md section 6's own framing, not something WU-16a respawns
every frame. Its only operation, `runOnAll(fn)`, calls `fn(workerIndex)`
once per worker (`workerIndex` in `[0, size())`) and blocks the calling
thread until every call has returned, using a mutex, a generation counter
and two condition variables (one for dispatch, one for completion) —
simpler than a general work-stealing task queue, and sufficient for this
unit's own single call per `runFrame()` invocation. Two consecutive
`runOnAll()` calls from the same caller are already architecture.md
section 6's own "barrier" for free — the second call's work is never
dispatched to any worker until the first has fully drained — a property
this unit does not itself need (PASS 1 stays unparallelised, so
`runFrame()` only ever calls `runOnAll()` once) but is there, unchanged,
for WU-16b's own future use once PASS 1 needs the same two-call shape.
Verified directly for exactly the dispatch/barrier property claimed:
`tests/test_threading.cpp`'s `test_threadpool_runs_every_worker_every_round()`
confirms every worker index is called exactly once per `runOnAll()` call,
across several calls on the same pool, and `test_threadpool_of_one()`
checks the degenerate one-worker case; both also exercise clean pool
teardown, since a stuck `join()` in the destructor would hang the test
process rather than fail a `CHECK`.

**The `threads <= 1` path is a separate, untouched loop — never routed
through `ThreadPool`, even with `numThreads == 1`.** `core/pipeline.cpp`'s
own `runFrame()` branches before constructing anything: `threads <= 1`
runs the same per-tile loop WU-10 originally wrote (now sharing a small
`resolveOneTile()` helper with the threaded path below it, so the two
cannot silently diverge — see below), with no `ThreadPool`, no mutex, no
condition variable anywhere on that path. This is deliberate, not an
optimisation: ADR-015 names the single-threaded build "a permanent
oracle," and if that oracle path shared `ThreadPool`'s own dispatch
machinery (even degenerately, at `numThreads == 1`), a bug in the new
synchronisation code could corrupt the one reference every future
threading unit diffs against, silently. Keeping it a plain, inspectable
loop with zero new moving parts preserves exactly the property ADR-015
needs it to have.

**`resolveOneTile()`, factored out, shared by both paths.** WU-10's
original per-tile body (bank-resolve via `sumBanks()`, then
normalise/composite every covered cell into `dest`) is now a small
function both the `threads <= 1` loop and the threaded path call
identically, rather than two hand-written copies that could quietly drift
apart — the same "reuse a tested function rather than duplicate its
logic" preference ADR-029 already used for `compositeLayered()`. Verified
empirically, not just by this reasoning, since it is exactly the kind of
claim C-012 warns against trusting on algebra alone: `tests/
test_zoneplate.cpp` and every other pre-existing pipeline-level test
(`test_shapes.cpp`, `test_pageturn.cpp`, `test_layered_composite.cpp`,
`test_morph.cpp`) all still pass unchanged against this refactored
`threads <= 1` path, across the full Clang 18 / GCC 13, Release/Debug,
tile 4/5 matrix plus ASan+UBSan (see "Verification," below) — the
oracle's own output did not move.

One byte-level change within that shared body, confirmed behaviour-
preserving before relying on it: the original loop constructed a fresh
`TileAccum accum;` inside the `ty`/`tx` loop (relying on `TileAccum`'s
own constructor to zero it); `resolveOneTile()` instead calls
`accum.clear()` on a `TileAccum` constructed once outside the loop and
reused across every tile. `core/splat.cpp`'s own `TileAccum::TileAccum()`
and `TileAccum::clear()` both zero every bank identically (the
constructor `fill()`s each bank with a freshly-value-initialised
`std::vector<AccumCell>`; `clear()` `fill()`s each existing bank's
elements with `AccumCell{}`, the same all-zero state) — read directly in
`core/splat.cpp` before relying on it, not assumed. This is also exactly
what `TileAccum::clear()`'s own doc comment already anticipated: "e.g.
for reuse across tiles (WU-16's per-worker bin arenas will want this)."

**"Per-worker bin arena," in this unit's own scope: one `TileAccum` plus
one `AccumCell` scratch buffer per worker, for the pool's lifetime.**
`core/pipeline.cpp`'s threaded path allocates exactly `pool.size()`
`TileAccum`s and `pool.size()` `AccumCell` scratch vectors once, before
`runOnAll()`, indexed by `workerIndex` — each worker's own
`resolveOneTile()` calls, for every tile it is assigned, read and write
only its own arena entry, calling `.clear()` between tiles the same way
the single-threaded path now does. No worker ever touches another
worker's arena, and no locking is needed inside the hot per-tile loop at
all: `TileBins::tile()` is read-only during PASS 2 (PASS 1 has already
fully returned, synchronously, on the calling thread, before
`pool.runOnAll()` is ever invoked — a plain happens-before edge, no
"generation" needed for that half), and each tile writes a disjoint block
of `dest`'s pixels (its own `originX`/`originY`, `localWidth`/
`localHeight`), so two workers can never write the same `dest` index.
Confirmed empirically as well as reasoned through: the full suite was run
under ThreadSanitizer (`-fsanitize=thread`, GCC 13) as well as ASan+UBSan
— see "Verification" — specifically because this is the first unit in
the project with more than one thread of execution inside `scatter-core`
at all, and reasoning about the absence of a data race is exactly the
kind of claim this project's own C-012/C-011 lessons say to check
empirically rather than trust on inspection alone.

**Tile partitioning: static, interleaved (`tileIndex % pool.size()`), not
work-stealing.** Simplest scheme that gives every worker a share of tiles
differing by at most one, computed with no shared mutable state and no
per-tile synchronisation cost. I6 (integer addition is associative) means
correctness does not depend on *which* worker processes *which* tile, or
in what order any of them finish — only that every tile is processed by
exactly one worker and every worker's own writes land in the disjoint
block that tile owns — so there is no correctness reason to prefer a
fancier scheme, and no throughput measurement from this session's own
Linux cloud sandbox (a different core count and memory hierarchy from the
M1 Max) would be a meaningful basis for choosing one. Load-balancing
tuning (uneven per-tile fragment cost, e.g. a heavily-magnified tile
under C-011's own lesson costing much more to splat than a sparse one) is
explicitly not decided here — flagged as a possible future refinement,
not scoped or built.

**QoS: `setWorkerQoS()`, Apple-only, unverified this session — same
shape and same caveat ADR-031/032 already used for WU-14/WU-15a's own
Apple-only surfaces.** architecture.md section 6's own "Apple Silicon
gotcha" is quoted directly in `core/pipeline.hpp`'s own comment:
`pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)`, called
once by each worker thread at startup, guarded by `#ifdef __APPLE__` (via
`<pthread/qos.h>`) and a no-op everywhere else — the same "fail soft on a
platform that does not have this surface at all" shape ADR-031's
`BLACKMAGIC_SDK_DIR` guard already uses for a different Apple-only
dependency, for the same underlying reason (ADR-013: `scatter-core` must
keep building and testing in the Linux cloud sandbox, no Apple toolchain,
unchanged). **This one function is unverified by this session**: no
AppleClang/Xcode toolchain exists in the Linux cloud sandbox this unit
was implemented in, so neither the exact header (`<pthread/qos.h>`) nor
the exact call signature has been compiled against Apple's own headers —
only reasoned through from architecture.md's own quoted call and Apple's
documented API shape. It is also, by its own nature, not something any
test in this Linux sandbox could meaningfully assert about even if it did
compile there (no QoS classes to introspect on a platform that does not
have them) — `tests/test_threading.cpp`'s own
`test_set_worker_qos_does_not_crash()` only confirms the (here, no-op)
call links and returns. Everything else this unit builds — `ThreadPool`
itself, the tile partition, `resolveOneTile()`, the `threads<=1`/
`threads>1` branch in `runFrame()` — is ordinary portable C++20 with no
platform guard at all, and is fully verified by this session (see
below). `WORK-UNITS.md`'s own WU-16a line stays `wip`, not `green`, until
built and run at the real terminal, matching WU-14/WU-15a's own
precedent for exactly this "one small Apple-only piece unverified, the
rest is not" situation.

**`PipelineParams::threads`, a new field on an existing struct, not a new
parameter to `runFrame()`.** `core/resolve.hpp`'s `PipelineParams`
(ADR-026) gains one field, `threads = 1`, rather than `runFrame()` itself
gaining a new parameter — every existing caller (WU-10 through WU-15b's
own tests, none of which this session touches) keeps compiling unchanged
and keeps taking the exact single-threaded path it always has, since a
default-constructed or explicitly-listed `PipelineParams` that never
mentions `threads` gets `1`. This is an additive change to a struct's own
field list, not a rename or a signature change to anything
`SESSION-PROTOCOL.md` rule 2 calls fixed — the same kind of addition
ADR-026 itself made when it gave `Background` a defaulted field, and the
same "extend, don't touch existing behaviour" shape WU-12b's own
`compositeLayered()` addition to `core/resolve.hpp` already used next to
the untouched `composite()`.

**Per-frame `ThreadPool` construction, not a caller-owned persistent
pool — deliberately left for WU-19.** `runFrame()`'s own signature is
unchanged (`lattice`, `src`, `params`, `dest` — no pool parameter), so
when `params.threads > 1` it constructs a local `ThreadPool` for the
duration of that one call and lets it go out of scope (join) before
returning. This spawns and joins `params.threads` OS threads on every
such call, real overhead a genuinely persistent, reused-across-frames
pool would avoid — but WU-16's own accept criterion is about
determinism, not throughput, and `WORK-UNITS.md`'s own next Phase-4 unit,
WU-19 ("Real time at 576i25"), is explicitly where per-frame overhead and
real-time scheduling become this project's actual job to solve, not
before it exists to solve them — the same "do not answer a question the
unit whose job it is has not been reached yet to ask" reasoning ADR-026
already used for the k-buffer's own background question (WU-28) and
ADR-030 used for a keyframe-sequence type (no orchestration layer for one
exists yet). Not decided here, deliberately.

**Not decided here, deliberately, and named for whoever builds WU-16b:**
PASS 1's own row-band partitioning and private per-tile bin arena *during
generation* (architecture.md section 6's fuller sketch) — needs a
row-range-aware entry point on `core/binner.cpp`'s `generateFragments()`
that leaves the `v`-parameter denominator keyed to the source raster's
*whole* height, not a band's, plus a merge step so PASS 2 can read every
worker's own generation-time arena for a given tile rather than the
single `TileBins` PASS 1 currently produces; per-tile load-balancing
beyond static interleaving; and a persistent, caller-owned `ThreadPool`
that `runFrame()` can reuse across many calls instead of constructing one
per call (WU-19's own job, immediately above). None of these are begun
here.

**Verification.** Built and tested in a Linux cloud sandbox (not the
device bridge's own more limited Linux VM this session also had
available — a separate, full Ubuntu 24.04 sandbox with Clang 18.1.3 and
GCC 13.3.0, cmake 3.28, ninja) across the same matrix prior core-only
units used: Clang 18 and GCC 13, Release and Debug, `SCATTER_TILE_LOG2` 4
and 5 (eight configurations, all fifteen tests green — the fourteen
carried over unchanged plus `tests/test_threading.cpp`, ~1.5 million
checks in that one binary alone, zero warnings under this project's full
`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror` set,
ADR-017), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes (clean, no ASan/UBSan
report) and, new for this unit specifically because it is the first with
genuine concurrency inside `scatter-core`, GCC 13 with `-fsanitize=thread`
(clean, no data race reported, both across the full suite and standalone
against `test_threading` under `TSAN_OPTIONS=halt_on_error=0` to surface
every report rather than stop at the first). `WORK-UNITS.md`'s own
WU-16a line stays `wip`, not `green`, pending only `setWorkerQoS()`'s own
Apple-only branch being built and run at the real terminal — everything
else above is fully verified by this session.

Does not reopen `docs/architecture.md`, ADR-002, ADR-008, ADR-013,
ADR-015, ADR-017, ADR-024, ADR-026, ADR-029 or ADR-031 — same relationship
every ADR since ADR-020 has to the document; ADR-015's single-threaded
oracle is preserved deliberately (see "The `threads <= 1` path," above),
not weakened; ADR-024's tile binning and ADR-002's four-bank split are
read as fixed inputs, unaltered, the same way ADR-024 itself already
reused ADR-023; ADR-026's own anticipation of `pipeline.hpp` and its
`PipelineParams` are both extended, not amended; and ADR-031's
`BLACKMAGIC_SDK_DIR`-style "fail soft on a platform without this surface"
shape is reused for a second, unrelated Apple-only dependency, not
altered.
