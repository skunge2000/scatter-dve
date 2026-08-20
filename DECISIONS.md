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

**ADR-041 — Thread pool: PASS 1 row-band parallelism, per-worker
generation-time bin arenas, completing architecture.md section 6's full
two-pass design ADR-040 deliberately scoped down to PASS 2 alone. Frozen
at WU-16b, Phase 4's second unit.**

ADR-040 (WU-16a) named this unit and its own reason for existing in full:
"a row-range parameter on `generateFragments()` that leaves `src.height`
itself untouched for the `v` calculation — straightforward in itself, but
it touches `core/binner.hpp` *and* `core/binner.cpp`, which together with
`core/pipeline.hpp` (new) and `core/pipeline.cpp` (touched) would be four
source files before this unit's own test, one over
`SESSION-PROTOCOL.md`'s... cap." With `core/pipeline.hpp` already built
and unchanged by this unit, the four-file problem does not recur:
WU-16b's own scope is exactly the row-range entry point ADR-040 already
specified, plus the `core/pipeline.cpp` dispatch/merge logic that uses it
— three source files (`core/binner.hpp`, `core/binner.cpp`,
`core/pipeline.cpp`) plus one new test, within the cap. This session's own
first job, per Steve's own brief (the same discipline WU-16a's own
session used, and ADR-031/032 used before that for the DeckLink SDK):
read `core/binner.cpp`'s `generateFragments()` and its anonymous-namespace
`pixelToLattice()` closely before writing anything, and verify ADR-040's
own diagnosis against the real code rather than assume it. It checks out
exactly as ADR-040 described: `generateFragments()`'s row loop
(`for (int py = 0; py < src.height; ++py)`) and `pixelToLattice(double(py),
src.height)`'s own denominator both read the same `SourceRaster::height`
field, so a row-band parallel PASS 1 cannot be built by calling the
existing function once per band against a `SourceRaster` whose own
`height` was shortened to the band's extent — that would move the
`v`-parameter's whole domain, not just which rows are visited, corrupting
the lattice mapping for every row in the band.

**`generateFragmentsRowRange()`, new, `core/binner.hpp`/`.cpp` — the
row-range-aware entry point ADR-040 already specified.** Same parameter
list as `generateFragments()` plus `rowStart`/`rowEnd` (unchecked
preconditions, `0 <= rowStart <= rowEnd <= src.height` — the same
unchecked-index convention every other tile/row index in this codebase
already uses, e.g. `Lattice::at()`); the row loop's bound becomes
`rowStart`/`rowEnd`, while every `pixelToLattice()`/`pixelJacobian()` call
inside the loop body is untouched and still reads `src.width`/
`src.height` in full — the "honest fix" ADR-040 named. `generateFragments()`
itself is now a thin wrapper, `generateFragmentsRowRange(..., 0,
src.height, outBins)` — its own signature and behaviour are exactly WU-08's
frozen ones, unchanged; `SESSION-PROTOCOL.md` rule 2 ("never rename or
refactor... names in headers are fixed") is read here the same way
ADR-026/030 already read it for `core/resolve.hpp`/`core/lattice.hpp`: as
forbidding a change to an existing function's own contract, not as
forbidding a new sibling entry point beside it. Verified directly, not
just reasoned through: `tests/test_row_band.cpp`'s
`checkRowRangeReassembly()` partitions a source raster into several row
bands (including more bands than source rows, so several bands are empty)
using the exact `(worker * height) / numWorkers` formula
`core/pipeline.cpp` itself uses, generates each band into its own
`TileBins`, and checks the union against a single whole-raster
`generateFragments()` call tile by tile — not just the same set of source
pixels present, but every matching fragment's own encoded position,
colour and weight fields bit-identical. See `CORRECTIONS.md` C-015 for an
error this session's own first draft of that test file made and caught
before relying on it.

**Row-band partition: contiguous, `(worker * src.height) / numWorkers`,
not interleaved.** architecture.md 6's own words are "Pass 1 partitions
the source by row *bands*" — contiguous ranges — in contrast to its own
"Pass 2 partitions by tile" (no "bands" language there), which WU-16a
already implemented as an interleaved `tileIndex % threads` split. Taken
literally: PASS 1 gets contiguous bands, PASS 2 keeps its own interleaved
tile split, unchanged. `rowStart(w) = (w * src.height) / numWorkers`,
`rowEnd(w) = ((w + 1) * src.height) / numWorkers` covers `[0, src.height)`
exactly once, with no gap or overlap, for any `numWorkers`, and degrades
harmlessly to an empty band (`rowStart == rowEnd`) whenever `numWorkers`
exceeds `src.height` — verified directly, not just reasoned: `tests/
test_row_band.cpp`'s `test_row_range_reassembles_with_more_bands_than_rows()`
(row-range level) and `test_threaded_pipeline_more_workers_than_source_rows()`
(pipeline level, `runFrame()` at `threads` in `{2, 3, 8, 16}` against a
4-row source) both exercise this directly, the same "harmless, not a
crash" standard WU-16a's own `test_threaded_pipeline_matches_single_threaded()`
already established for PASS 2's own more-workers-than-tiles case
(`threads == 16` there). Per-row-band load-balancing (a warp whose
magnification varies sharply across the frame gives some bands far more
supersampled fragments to generate than others, the same class of concern
C-011 already raised for a shape's own front-facing point) is explicitly
not addressed here — flagged as a possible future refinement, the same
"not decided here" treatment ADR-040 already gave PASS 2's own uneven
per-tile fragment cost, for the same reason: no throughput measurement
from this session's own Linux cloud sandbox would be a meaningful basis
for choosing a scheme, and correctness (I6) does not depend on the
partition being balanced, only on it being a true partition.

**Per-worker "generation-time bin arena": one whole-frame `TileBins` per
row-band-generating worker, not a partial one.** A row band's own
fragments can land in *any* tile, not just tiles "near" that band's own
source rows — the warp is exactly what determines destination position,
and nothing about a contiguous row range constrains it (a cylinder's or
sphere's own curvature already routes source rows nonlinearly across the
destination, and I1's own folds/tears are the extreme case). So each
worker's own arena must be sized for the whole destination raster, the
same size `TileBins` PASS 1 built as a single shared instance in WU-16a.
All `numWorkers` arenas are fully constructed, serially, on the calling
thread before `pool.runOnAll()` ever dispatches PASS 1 — no reallocation
happens once the parallel section starts, the same "preallocate before
the hot parallel section" shape WU-16a's own per-worker `TileAccum`/
`tileCells` arenas already used one stage later, applied here one stage
earlier. Memory cost is `numWorkers` times a single `TileBins`' own size
where WU-16a needed one — an inherent consequence of "private per-tile
bin arena... per worker" (architecture.md 6's own words), not a
regression this unit introduces or a concern architecture.md itself
raises; not measured or tuned, same as every other throughput question
this unit defers.

**The barrier: `ThreadPool::runOnAll()`'s own two-consecutive-calls
shape, unused until now, exactly as `core/pipeline.hpp`'s own doc comment
already anticipated** ("the two-call barrier shape is here... for
whichever future unit (ADR-040's own named WU-16b) parallelises pass 1
the same way and actually needs it"). No change to `core/pipeline.hpp`
itself — `ThreadPool` needed nothing new; `runFrame()` in
`core/pipeline.cpp` simply calls `pool.runOnAll()` twice on the same pool,
once for PASS 1 (row-band generation into the per-worker arenas above)
and once for PASS 2 (tile-parallel resolve, below), and the second call's
own work is never dispatched to any worker until the first has fully
drained on the calling thread — architecture.md 6's own "Barrier," free.

**`resolveOneTile()` generalised to take a list of PASS-1 sources
(`std::span<const TileBins* const>`), not a single `TileBins`, extending
WU-16a's own "both paths call the same function, so they cannot silently
diverge" property from two paths to three.** PASS 2 now needs to splat a
tile's own contribution from *every* worker's arena, in fixed order
(architecture.md 6's own "each worker reads all 8 workers' lists for its
tiles, in fixed worker order") — not from one shared `TileBins` the way
WU-16a's own threaded PASS 2 read. Two designs were weighed: a second,
hand-written function duplicating `resolveOneTile()`'s own bank-resolve/
normalise/composite body for the multi-source case, or generalising the
existing function's first parameter to a small ordered list of sources,
with the `threads <= 1` path wrapping its own single `TileBins` in a
one-element `std::array`. Chosen: the latter. `splatTile()` does not
clear `accum` itself (`core/splat.hpp`'s own documented contract) and
integer addition is associative (I6), so looping over however many
sources a caller supplies, splatting each into one `accum` cleared once
beforehand, is exact regardless of count or order — one source (the
`threads <= 1` case) is not a special case needing its own code path,
just the `N == 1` instance of the same loop, and duplicating
`resolveOneTile()`'s own body into a second function would have
reintroduced exactly the "two hand-written copies that could quietly
drift apart" risk ADR-040 itself named and eliminated at WU-16a. The
`threads <= 1` path's own call site changes from `resolveOneTile(bins,
...)` to `resolveOneTile(soloSource, ...)`, wrapping `bins` in a
one-element `std::array<const TileBins*, 1>` — arithmetically identical
to WU-16a's own call (`splatTile()` is still called exactly once per
tile, against the same tile bin, in the same order), verified rather than
merely reasoned: the full test suite, including every pre-existing
pipeline-level test (`test_zoneplate.cpp`, `test_shapes.cpp`,
`test_pageturn.cpp`, `test_layered_composite.cpp`, `test_morph.cpp`,
`test_threading.cpp`'s own `threads <= 1` checks), still passes unchanged
against this refactored `threads <= 1` path across the full verification
matrix below — the oracle's own output did not move, the same kind of
differential check WU-16a's own `resolveOneTile()` extraction already
relied on for its own byte-level `TileAccum` reuse change.

**`tilesX`/`tilesY`/`totalTiles` computed via `tileCount()`
(`core/binner.hpp`) directly, not via a shared `TileBins`' own
`tilesX()`/`tilesY()`.** WU-16a's own code built one shared `TileBins`
before either branch and read its `tilesX()`/`tilesY()`; WU-16b's own
`threads > 1` branch no longer builds one shared `TileBins` at all (PASS
1 now writes into `numWorkers` separate per-worker arenas instead), so
there is nothing to query. `tileCount()` is the exact free function
`TileBins`'s own constructor already calls internally to derive
`tilesX_`/`tilesY_` (`core/binner.cpp`, unchanged) — calling it directly,
once, before either branch, is behaviour-preserving by construction (same
function, same arguments, just invoked one line earlier) rather than a
new computation, and lets the `threads <= 1` branch's own `TileBins`
construction happen after this hoist with no change to what value it
ends up holding.

**Verification.** Built and tested in a Linux cloud sandbox (Ubuntu
24.04, Clang 18.1.3, GCC 13.3.0, cmake 3.28, ninja — the same environment
WU-16a's own session used) across the same matrix: Clang 18 and GCC 13,
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all
sixteen tests green — the fourteen carried over from before WU-16a plus
`test_threading` and the new `test_row_band`, zero warnings under this
project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes (clean) and GCC 13 with
`-fsanitize=thread` (clean, no data race, both across the full suite and
standalone against `test_threading`/`test_row_band` under
`TSAN_OPTIONS=halt_on_error=0`) — the latter checked with particular
care since this unit adds the project's first concurrent code inside PASS
1 (WU-16a's own concurrency was PASS 2 only), the same "check empirically,
not just by inspection" standard C-011/C-012 already established.

Unlike WU-16a, this unit adds **no new Apple-only surface** —
`setWorkerQoS()` (`core/pipeline.cpp`) is untouched, and every symbol this
unit adds or changes (`generateFragmentsRowRange()`, `resolveOneTile()`'s
new `std::span` parameter, `runFrame()`'s own row-band dispatch) is
ordinary portable C++20 already exercised in full by the matrix above. But
`WORK-UNITS.md`'s own WU-16b line stays `wip`, not `green`, for the same
procedural reason every other unit's line has: per this project's own
established convention (`SESSION-PROTOCOL.md`, "the assistant does not run
`close.sh`"), Steve's own real-terminal `cmake --build` + `ctest` +
`./tools/close.sh 16b` run is still outstanding, even though nothing about
this unit's own content is expected to behave differently there than in
the Linux sandbox above.

**Not decided here, deliberately:** per-row-band load balancing (see
above); bump-allocated/preallocated bin arenas — architecture.md 6's own
"preallocated, bump-allocated" phrase describes an allocation strategy
`TileBins`' own `std::vector<Frag>` growth (`push_back()`, unchanged since
WU-08) does not implement, for either the single arena WU-16a left
untouched or the `numWorkers` arenas this unit adds; not this unit's job
either, the same "not decided here" ADR-040 already left this exact gap
at. A persistent, caller-owned `ThreadPool` reused across many `runFrame()`
calls instead of one constructed and joined per call (WU-19's own job,
unchanged from ADR-040's own deferral — this unit still constructs one
`ThreadPool` per `runFrame()` call when `threads > 1`, now used for two
`runOnAll()` rounds instead of one). NEON (WU-17/18), untouched.

Does not reopen `docs/architecture.md`, ADR-002, ADR-008, ADR-013,
ADR-015, ADR-017, ADR-024, ADR-026, ADR-029, ADR-031 or ADR-040 — same
relationship every ADR since ADR-020 has to the document; extends
ADR-040's own deliberately-deferred WU-16b item rather than revisiting
any decision ADR-040 itself made; ADR-024's `pixelToLattice()`/
`pixelJacobian()` denominator convention is reused unaltered — still
keyed to `src.width`/`src.height` in full, exactly as WU-08 fixed it, now
simply invoked over a caller-chosen row subset rather than always the
whole raster; ADR-015's single-threaded oracle is preserved deliberately
(see "`resolveOneTile()` generalised," above: the `threads <= 1` path's
own arithmetic is bit-for-bit unchanged, verified via the unmoved output
of every pre-existing pipeline-level test, not merely reasoned to be
unchanged).

**ADR-042 — NEON v210 unpack/pack: the sandbox's own real cross-compile-and-
run verification capability (materially stronger than ADR-031/032's
DeckLink precedent), the new-sibling-function design (scalar entry points
untouched forever), the group-level vectorisation shape, and the
`__ARM_NEON`/`CMAKE_SYSTEM_PROCESSOR` guards — frozen at WU-17, Phase 4's
first NEON unit.**

`WORK-UNITS.md`'s own WU-17 line, going into this session, was bare: a title
and one accept criterion ("bit-identical to scalar reference"), no `Files:`
line — the same shape WU-16's own line was in before WU-16a/16b split it
(ADR-040) and WU-14's DeckLink-enumeration line was in before ADR-031. This
session's own first job, per Steve's own brief: real scoping — reading
`src/video/v210.hpp`/`.cpp` (the WU-02 scalar reference this unit must match
bit-for-bit, a frozen interface) and `tests/test_v210.cpp` (the existing
scalar coverage this unit must not regress) — before writing code, plus
working out, from the real toolchain rather than assuming, whether this
unit's own Linux cloud sandbox (x86_64) can do anything better than
ADR-031/032's "reasoned through against headers, unverified until the real
terminal" shape for a genuinely different kind of platform gap: NEON
intrinsics are ARM64 machine instructions the sandbox's CPU cannot execute
natively, not an absent SDK.

**The sandbox question, answered with evidence, not assumed.** Checked
directly: `apt-cache policy g++-aarch64-linux-gnu qemu-user-static` in this
session's own Linux cloud sandbox (Ubuntu 24.04) showed both installable
from the standard Ubuntu repositories already in this sandbox's package
manager configuration; installed via `apt-get install g++-aarch64-linux-gnu
qemu-user-static` (exit 0, no network/permission issue — this sandbox runs
as root with allowlisted access to the standard package registries).
Confirmed with a standalone smoke test before trusting it for anything real
(a NEON `vaddq_u16` on a small buffer): `aarch64-linux-gnu-g++-13 -static`
compiles genuine `<arm_neon.h>` intrinsic code for AArch64, and
`qemu-aarch64-static ./binary` runs it and produces the correct result —
this sandbox can **actually execute** ARM64 NEON code, not merely
cross-compile it blind. This is a materially different, stronger situation
than ADR-031/032's DeckLink units, which could not even compile against the
real SDK in this sandbox at all. Also checked, not assumed: Clang cross-
compiles the same way (`clang++ --target=aarch64-linux-gnu
--gcc-toolchain=/usr`, dynamically linked, run via `qemu-aarch64-static -L
/usr/aarch64-linux-gnu ./binary`) — both compilers this project's own
matrix already uses for x86_64 are therefore available for aarch64 too.
GCC's ASan+UBSan combination (`-fsanitize=address,undefined`) was tried and
reproducibly segfaults *qemu itself* (`qemu: uncaught target signal 11`,
confirmed twice, on both a standalone prototype and the real project files)
— a known class of limitation in QEMU's user-mode emulation of ASan's
shadow-memory mmap layout, not a defect in this unit's own code; UBSan
alone (`-fsanitize=undefined`, no ASan) runs clean under the same emulator.
**Decision:** this sandbox is used for genuine execution-verified
cross-compilation — GCC 13 and Clang 18, Release and Debug, both tile
sizes, plus UBSan alone — with ASan specifically deferred to the real M1
Max terminal alongside this unit's own final `close.sh` confirmation, the
same "one thing genuinely unverified here, everything else is not" shape
ADR-031's own `setWorkerQoS()` note already used for a different Apple-only
surface, but reached by actually trying the sandbox's own tools rather than
assuming a NEON unit must default to ADR-031's fully-unverified shape.

**Design: new NEON-suffixed sibling functions; the scalar entry points are
never modified, extended, or made to dispatch internally.** `unpackRow`,
`packRow`, `unpackImage` and `packImage` (WU-02, frozen) are untouched —
still exactly what `v210.hpp`'s own top comment already called them,
"Scalar only." New siblings — `unpackRowNeon`, `packRowNeon`,
`unpackImageNeon`, `packImageNeon` — declared in the same `v210.hpp`,
defined in the same `v210.cpp`, guarded by `#if defined(__ARM_NEON)` in
both files. Two designs were weighed, per this session's own brief: making
the *existing* names platform-dispatch internally (same signature, NEON
body on ARM, scalar body elsewhere) — matching `v210.hpp`'s own now-stale
WU-02-era comment, "Row operations — the primitives WU-17 replaces" — or
adding distinctly-named siblings and leaving the scalar ones alone forever.
Chosen: siblings. Internal dispatch would remove the very thing this unit's
own accept criterion ("bit-identical to scalar reference") needs to exist
at all — on an ARM build there would be no way to call "the scalar
implementation, specifically" to diff against, only whichever one platform
dispatch picked. This also reads `SESSION-PROTOCOL.md` rule 2 ("never
rename or refactor... names in existing headers are fixed") the same way
ADR-026/030/038 already do — as forbidding a change to an existing
function's own contract, not as forbidding a new sibling beside it — the
same precedent, applied here to a platform variant rather than a new
capability. The stale "primitives WU-17 replaces" comment is corrected in
place (comment-only, no signature or behaviour change) to say what this
entry actually decides.

**No new header/`.cpp` pair.** `docs/architecture.md` section 8's own
module-layout sketch already names this file pair `v210.hpp/.cpp # unpack/
pack, scalar reference + NEON` — one file pair holding both, not a separate
`v210_neon.hpp/.cpp` — so unlike ADR-021/026/027/030's own "does this need
its own header" judgement calls (each filling a gap architecture.md left
open), this one is answered directly by the document's own already-written
words. `Files:` for this unit is therefore `src/video/v210.hpp`,
`src/video/v210.cpp` (both extended, not new) plus `tests/test_v210_neon.cpp`
(new) — two source files plus its test, within `SESSION-PROTOCOL.md`'s cap
with room to spare.

**Vectorisation shape: one v210 group (16 bytes, 4 words) per NEON register,
mask/shift vectorised across all 4 words at once, field-to-Y/Cb/Cr placement
left scalar.** v210's own layout (`v210.hpp`'s own diagram) puts each 10-bit
field at bit offset 0, 10 or 20 within one of 4 words per group, but *which*
of Y/Cb/Cr owns which offset differs per word — an irregular interleave, not
a uniform stride. One `vld1q_u32` loads a whole group's 4 words into one
128-bit register; three vector ops (`vandq_u32`/`vshrq_n_u32` for bits 0/10/
20) extract all 12 fields for all 4 words at once, replacing the scalar
`readGroup()`'s 12 individual `component()` calls (one word, one shift, one
mask each) with 3 vector op-pairs. The field ->  Y/Cb/Cr placement itself
(which of the 12 extracted values is `Y[0]` vs `Cb[0]` vs...) stays 12
scalar lane reads (`readGroupNeon`) or writes (`writeGroupNeon`, gathering
into the same per-word layout before the symmetric vectorised
shift-and-`vorrq_u32` reassembly) — the same "no clean vector shuffle across
an irregular interleave" reasoning `docs/architecture.md`'s own bin-traffic
section already gives for the splat's own "NEON has no scatter instruction"
limitation, applied here to a different irregular-interleave problem. This
is a deliberately modest first step — vectorising the part that is
genuinely uniform across all 4 words (the bitfield mask/shift), leaving the
part that is not (the interleave) as scalar data movement, the same ratio
of vector-to-scalar work in both directions by construction. A denser
scheme (`vld4q_u32` across 4 groups at once, deinterleaving by word-position
into a fully SoA layout before any scalar step) was considered and set
aside as a future refinement, not decided here — this unit's own accept
criterion is bit-identical correctness, not throughput (WU-19's job,
architecture.md's own Phase 4 "done when" line), and the simpler one-group
shape is easier to verify correct by direct field-by-field inspection
against `readGroup`/`writeGroup`'s own known per-word field assignments,
which this entry's own comments in `v210.cpp` state explicitly rather than
leaving to be reverse-engineered from the code.

**Short-final-group handling (ADR-018) needs no separate NEON-vs-scalar
split at all.** Checked directly against the real scalar code before
assuming a "NEON fast path, scalar tail" structure would be needed: scalar
`readGroup()`/`writeGroup()` have no dependency on `width` whatsoever — they
unconditionally decode/encode a full 16-byte group's twelve 10-bit fields;
the short-final-group branching lives entirely in `unpackRow`/`packRow`,
which decide how much of an *already-decoded* group to write out (unpack)
or which entries to replace with `kPadLuma`/`kPadChroma` before encoding
(pack), never in the group codec itself. `readGroupNeon`/`writeGroupNeon`
are therefore width-independent, drop-in replacements for exactly that one
step, and `unpackRowNeon`/`packRowNeon` are `unpackRow`/`packRow`'s own
loop bodies with only `readGroup` -> `readGroupNeon` / `writeGroup` ->
`writeGroupNeon` substituted — identical short-group branching,
identical `fromCode10`/`toCode10` calls, copied rather than shared (this
file's own `#if defined(__ARM_NEON)` block is meant to be self-contained)
to keep the two loops independently readable. `toCode10`'s own round-and-
clamp (I2) and `fromCode10`'s own offset-binary shift are not
reimplemented or vectorised here — called identically from both paths, so
I2's "pack is the one and only clamp site" property is exactly as true of
the NEON path as the scalar one, by construction rather than by a second
proof.

**CMake: `test_v210_neon` gated on `CMAKE_SYSTEM_PROCESSOR`, deliberately
not folded into the neighbouring `-mcpu=apple-m1` block; no new
cross-compilation build mode added to the project.** Two separate
`CMakeLists.txt` decisions:

- The executable itself must be gated at the CMake level, not left to a
  runtime skip inside the test — the functions it calls are not *declared*
  at all without `__ARM_NEON`, so it would fail to *compile*, the same
  "gate the whole executable, not sprinkle `#ifdef` through test code"
  shape `BLACKMAGIC_SDK_DIR` already uses for `test_decklink_device`/
  `test_decklink_output` (ADR-031), applied here for a processor-target
  reason instead of a missing vendor SDK. Chosen guard:
  `CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"` (matching both
  Apple's own `arm64` spelling and Linux's `aarch64`) — deliberately
  **not** added to the existing `if(APPLE AND CMAKE_SYSTEM_PROCESSOR
  MATCHES "arm64|aarch64")` `-mcpu=apple-m1` block a few lines above it in
  the same file, because that block is Apple-specific *tuning*, orthogonal
  to whether NEON intrinsics are available at all, and this guard must also
  match a non-Apple AArch64 Linux build (this session's own cross-compiled
  verification build, above) that the `APPLE AND` block never would.
- **No new project-level cross-compilation infrastructure was added.** This
  session's own aarch64-cross-plus-`qemu-aarch64-static` verification (a
  CMake toolchain file setting `CMAKE_SYSTEM_NAME Linux`,
  `CMAKE_CROSSCOMPILING_EMULATOR "qemu-aarch64-static;-L;/usr/aarch64-linux-gnu"`,
  etc.) was used ad hoc, outside the repository, the same "standalone
  reproduction, not new project machinery" shape ADR-036's own
  no-CMake/no-SDK verification already used. Considered and rejected:
  committing that toolchain file into `tools/` so a future session could
  reuse it. Rejected for the same reason ADR-038 declined to build a
  configurable-duration mechanism for WU-15b's own one-off need — Steve's
  own real verification of this unit happens natively on the M1 Max (an
  AArch64 host already; no cross-compilation of any kind needed there), so
  a committed cross-toolchain file would be permanent repository machinery
  serving only this session's own sandbox limitation, not this project's
  actual development target. The exact commands are recorded in this
  entry's own "Verification" section below instead, reproducible by a
  future session if the same sandbox question ever comes up again.

**Verification.** Built and run in this session's own Linux cloud sandbox
(Ubuntu 24.04) via genuine AArch64 cross-compilation and execution under
`qemu-aarch64-static`, not merely reasoned through:

- Default (x86_64, no toolchain file) matrix, confirming this unit leaves
  the existing sandbox matrix completely unaffected: Clang 18 and GCC 13,
  Release and Debug, tile 4 and 5 (eight configurations) plus GCC 13 with
  `-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
  sizes — all sixteen tests green (`test_v210_neon` itself absent from
  this matrix, correctly, per its own `CMAKE_SYSTEM_PROCESSOR` gate;
  configure log confirms the "skipping test_v210_neon" `STATUS` message),
  zero warnings under this project's full `-Wall -Wextra -Wpedantic
  -Wconversion -Wsign-conversion -Werror` set.
- AArch64 cross-compile + `qemu-aarch64-static` execution (GCC 13.3.0, via
  a CMake toolchain file, not committed — see above): Release and Debug,
  tile 4 and 5 (four configurations), plus GCC 13 with `-fsanitize=undefined
  -fno-sanitize-recover=all` — all seventeen tests green in every
  configuration (the sixteen carried over, `bit-identical to threads==1`
  etc. all still holding on AArch64, plus the new `test_v210_neon`, 53
  checks, zero warnings under the same full warning set). `-fsanitize=
  address` was also tried at Debug/tile5 and reproducibly crashes
  `qemu-aarch64-static` itself (`uncaught target signal 11`) before any
  test code runs — the sandbox limitation named above, not a result to
  report as a test failure.
- Clang 18.1.3 cross-compile (`--target=aarch64-linux-gnu
  --gcc-toolchain=/usr`, standalone — `v210.cpp` + `test_v210_neon.cpp`
  compiled directly, no CMake, the same ADR-036-style standalone shape),
  run under `qemu-aarch64-static -L /usr/aarch64-linux-gnu`: `PASS
  test_v210_neon (53 checks)`, zero warnings under the same full warning
  set — confirming this unit compiles clean under both compilers this
  project's own matrix already uses for x86_64, not only GCC.

`WORK-UNITS.md`'s own WU-17 line stays `wip`, not `green`, for the same
procedural reason every other unit's line has (`SESSION-PROTOCOL.md`, "the
assistant does not run `close.sh`") — Steve's own real-terminal `cmake
--build` + `ctest` + `./tools/close.sh 17` run, natively on the M1 Max
(where this unit's NEON path needs no cross-compilation or emulation at
all, being already AArch64 hardware), is still outstanding. Unlike
ADR-031/032's DeckLink units, this is not "reasoned through against
headers, entirely unverified until the real terminal" — every line this
unit adds has already executed correctly, genuinely, in this session's own
sandbox; what remains for the real terminal is confirming AppleClang agrees
(a real, open question — this session's own aarch64 verification used GCC
and Clang's mainline cross target, not AppleClang, which is a different
compiler with its own `-mcpu=apple-m1` tuning already applied via the
existing CMake block) and the one specifically-untried case, ASan on
AArch64, which the real M1 Max can attempt without the qemu limitation this
session's own sandbox hit.

Does not reopen `docs/architecture.md`, ADR-013, ADR-017, ADR-018 or
ADR-031 — same relationship every ADR since ADR-020 has to the document;
ADR-013's Linux-cloud-sandbox-buildable `scatter-core` property is extended
by a second, gated build mode (the AArch64 cross toolchain, used ad hoc,
not committed) rather than crossed — the default x86_64 matrix is provably
unaffected (see "Verification," above); ADR-017's warning set is applied
unchanged to every line this unit adds, on every compiler and every target
tried; ADR-018's short-final-group padding behaviour is reused unaltered,
proven identical on the NEON path by construction (see above) rather than
re-decided; ADR-031's "gate an entire executable at the CMake level, not
via scattered `#ifdef`" shape is reused for a second, unrelated
platform-availability question.

**ADR-043 — NEON chroma resampling: the module-layout question checked
directly rather than assumed, the interior/edge vectorisation shape (a
sliding-window boundary clamp, not v210's fixed bit-interleave), the
associativity argument for why NEON's own multiply-accumulate order is
still bit-exact, and reuse of ADR-042's sandbox verification capability and
CMake guard unchanged — frozen at WU-18, Phase 4's second NEON unit.**

`WORK-UNITS.md`'s own WU-18 line, going into this session, was barer than
WU-17's own line was before ADR-042 (WU-17 at least already had "bit-
identical to scalar reference" written down; WU-18 had neither that nor a
`Files:` line). This session's own first job, per Steve's own brief and
`SESSION-PROTOCOL.md`'s own discipline: real scoping before any code —
reading `src/video/chroma.hpp`/`.cpp` (ADR-020's own frozen filter
coefficients, rounding convention and edge handling, and a frozen
interface) and `tests/test_chroma.cpp` closely, the same order ADR-042
already used for `v210.hpp`/`.cpp`/`test_v210.cpp`.

**The module-layout question, checked directly rather than carried over
from ADR-042's own precedent unexamined.** `docs/architecture.md` section
8's own module-layout sketch names `v210.hpp/.cpp` as "unpack/pack, scalar
reference + NEON" explicitly, but its `chroma.hpp/.cpp` line reads only
"422↔444 polyphase," with no "+ NEON" — a real difference in how much
detail the two one-line comments carry, worth checking rather than
assuming the same "extend the existing file pair" answer automatically
carries over. Two independent pieces of evidence resolve it the same way
ADR-042 already went, without reopening `docs/architecture.md` itself:

- `chroma.hpp`'s own top comment, written at WU-04 — before this session,
  before WU-17 even existed — already says: "the scalar reference here is
  the oracle WU-18's NEON path is diffed against — the same relationship
  v210.cpp already has to WU-17." The file's own header already commits to
  this exact relationship; the module-layout sketch's terser comment is an
  omission of detail, not a contrary design signal.
- `docs/architecture.md`'s own Phase 4 "done when" line, section 10: "8-
  thread output is bit-identical to single-threaded, at frame rate," under
  a phase titled "Threading and NEON... Thread pool, QoS, NEON v210 and
  chroma paths" — chroma is one of exactly two modules this document ever
  names for a NEON path, on equal standing with v210, in the one place
  that actually states which modules get one. The module-layout table's
  per-line comments are simply not uniformly detailed (`splat.hpp/.cpp`'s
  own line likewise says nothing about NEON, yet a *different* section
  entirely — the bin-traffic prose — is what actually states splat stays
  scalar, "NEON has no scatter instruction"); the table itself is not
  where this document records which modules do or do not get a NEON path.

**Decided: no new header/`.cpp` pair — `src/video/chroma.hpp`/`.cpp`,
extended, exactly ADR-042's own precedent for `v210.hpp`/`.cpp`.** `Files:`
for this unit is `src/video/chroma.hpp`, `src/video/chroma.cpp` (both
extended, not new) plus `tests/test_chroma_neon.cpp` (new) — two source
files plus its test, the same shape WU-17 used.

**Design: new NEON-suffixed sibling functions — `upsampleRowNeon`,
`downsampleRowNeon`, `upsampleImageNeon`, `downsampleImageNeon` — guarded
by `#if defined(__ARM_NEON)`, the scalar `upsampleRow`/`downsampleRow`/
`upsampleImage`/`downsampleImage` (WU-04) untouched forever.** Same
reasoning ADR-042 already gave for v210: this unit's own accept criterion
("bit-identical to scalar reference," `WORK-UNITS.md`) needs "the scalar
implementation, specifically" to remain callable to diff against, which
internal platform dispatch would remove.

**Vectorisation shape: genuinely different from v210's own, because the
irregularity is a different kind.** v210's own NEON path (ADR-042)
vectorises a *fixed per-group bit interleave* — the same shape for every
group, entirely independent of row width, with the irregular part (which
of 12 fixed lane positions holds Y/Cb/Cr) confined to 12 scalar reads/
writes per group, never growing with the image. Chroma's own filters have
no such fixed-shape irregularity: their irregularity is `clampIndex()`'s
boundary replication (ADR-020), which is a no-op at every interior index
and only actually replicates a sample at a handful of indices near either
end of a row — a *sliding-window* problem, not a *fixed-layout* one. The
two units therefore cannot share a vectorisation shape even though both
are "the second/first NEON unit in this project" — chosen for chroma,
checked against the real edge-index arithmetic before writing any
intrinsic, not assumed from v210's own precedent:

- **Interior indices — where every tap's `clampIndex()` call is
  provably a no-op — are vectorised, four lanes of `int32x4_t` per batch.**
  For `upsampleRowNeon`'s halfway samples: interior is `1 <= i <= cw-3`
  (derived from the two clamped taps, `i-1 >= 0` and `i+2 <= cw-1`); for
  `downsampleRowNeon`: interior is `3 <= i <= cw-3` (from the widest tap,
  `2i-5 >= 0` and `2i+5 <= width-1`). Both bounds are worked from the exact
  same `clampIndex(idx, n)` the scalar functions call, not approximated.
  The co-sited half of `upsampleRowNeon` (`out[2i] = in[i]`) never needs
  clamping at all — `in[i]` is always in range for `i` in `[0, cw)` — and
  is left a plain scalar copy: there is no arithmetic there to diverge on,
  and vectorising a copy buys nothing a decent compiler does not already
  do for a linear loop.
- **Edge indices — where a tap's `clampIndex()` call actually replicates
  a boundary sample — are computed scalar, calling `clampIndex()` and
  `roundShift()` directly, the identical helpers the scalar row functions
  already use.** Upsample: index 0, plus whatever tail near `cw`'s own
  edge does not fill a 4-lane batch. Downsample: the first three indices
  (its wider 7-tap footprint needs more interior margin than upsample's
  4-tap one), plus the equivalent tail. Same "vectorise the genuinely
  uniform part, leave the irregular part scalar" discipline ADR-042
  already used for v210's own field placement — applied here to a
  boundary-clamp irregularity instead of a bit-interleave one, which is
  why the *split itself* (interior batch vs. scalar remainder) differs
  from v210's own (fixed vector extraction vs. fixed scalar placement)
  even though the underlying principle is the same.
- **`downsampleRowNeon`'s own load pattern: `vld2` deinterleaving loads
  at six fixed offsets around `2i`, not a single contiguous load.** The
  filter decimates by two (output index `i` centres on input `2i`), so
  consecutive output lanes need input positions two apart — a plain
  `vld1` at consecutive addresses does not line up with consecutive
  lanes. `vld2_u16(ptr)` deinterleaves 8 elements into two 4-lane halves,
  `val[0][k] = ptr[2k]` and `val[1][k] = ptr[2k+1]`; choosing `ptr = in +
  2*i + offset` for each of the filter's odd tap offsets (and `in + 2*i`
  itself for the even centre tap) makes `val[1]` (or `val[0]`, for the
  centre) exactly the four lanes' worth of that tap, worked out
  arithmetically before writing the intrinsics, not by trial and error.
  Six loads cover the filter's seven nonzero-coefficient tap positions
  (offset 0 and offset +1 share one load) — some redundancy, twelve total
  sub-loads for seven distinct tap positions, accepted in exchange for a
  design simple enough to verify by direct per-lane arithmetic, matching
  this unit's own "correctness over throughput" scope (`WU-19`'s job, not
  this one's, the same deferral ADR-042 already made for v210's own denser
  `vld4q_u32` alternative). Output here is contiguous (`out[i]`, no
  interleave, unlike the halfway samples' strided `out[2i+1]`), so the
  result vector is stored with a single `vst1_u16` — no gather/scatter
  needed on the store side at all.
- **Arithmetic is 4-lane `int32x4_t`, matching the scalar functions' own
  `std::int32_t` accumulator width exactly, not 8-lane `int16x4_t`.**
  Worst-case magnitude is ~1.18M for upsample's four terms and ~40M for
  downsample's seven (both terms bounded by `chroma.hpp`'s own stated
  worst-case overshoot, 1/16 and 22/512 of a step respectively) — both far
  under `int16`'s ~32K range but comfortably inside `int32`'s, exactly the
  reason the scalar reference itself accumulates in `std::int32_t` rather
  than `Sample`. Rounding uses `vshrq_n_s32`, C++20's guaranteed-
  arithmetic right shift on a negative-capable `int32x4_t` lane, matching
  `roundShift()`'s own `(sum + half) >> shift` exactly, tap for tap, lane
  for lane; narrowing uses `vmovn_s32` (truncating, taking the low 16
  bits), matching a `Sample(...)` conversion's own modulo-65536 narrowing
  exactly — the same "no clamp beyond what the container can hold"
  property `chroma.hpp`'s own comment already documents for the scalar
  path, preserved bit-for-bit on the NEON one.
- **Why reordering NEON's own multiply-accumulate terms relative to the
  scalar functions' left-to-right sum is still guaranteed bit-exact — an
  argument this unit needed and v210's own NEON path (bitfield mask/shift,
  no multi-term sum at all) never had occasion to make.** Integer addition
  and multiplication by a compile-time constant are associative and
  commutative whenever no term overflows the accumulator's own width —
  true here throughout `Sample`'s entire 16-bit domain, checked directly
  against `chroma.hpp`'s own worst-case bounds above, not assumed. This is
  explicitly *not* `CORRECTIONS.md` C-012's hazard: C-012's own lesson is
  specific to floating point, where two differently-*shaped* expressions
  for the same real number are not guaranteed to round identically (FMA
  contraction, reassociation, transcendental-function noise); there is no
  equivalent hazard in exact, non-overflowing integer arithmetic, where
  any grouping of the same terms produces the same result. Worth stating
  explicitly, once, in this entry — not because either filter's own
  design changed as a result, but because a future session reusing this
  unit's own reasoning for some other integer NEON path should not need
  to re-derive why C-012 does not apply.

**CMake: `test_chroma_neon` added to the exact same
`CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"` guard block
`test_v210_neon` already uses, not a second guard.** Both executables'
whole bodies call functions that do not exist to declare at all without
`__ARM_NEON`, on the same processor-target reasoning ADR-042 already
established; no reason to duplicate the condition. No new project-level
cross-compilation infrastructure was added, same as ADR-042 — the aarch64
toolchain file used for this session's own sandbox verification was ad
hoc, outside the repository, not committed.

**A genuine bug this unit's own aarch64 cross-compile caught, and why the
x86_64 matrix alone would not have.** This session's own first draft of
`tests/test_chroma_neon.cpp` used `std::vector<Sample> in(std::size_t(cw));`
and the equivalent with `width` — the identical most-vexing-parse mistake
`HANDOFF.md` already recorded for WU-17's own first draft of
`test_v210_neon.cpp` ("`std::vector<Sample> Y(std::size_t(width)), ...`,
which C++ grammar reads as a function declaration"). It compiled clean in
every one of this session's own default x86_64 configurations, because
`test_chroma_neon` is gated behind the same `CMAKE_SYSTEM_PROCESSOR`
condition as `test_v210_neon` and is therefore never *compiled* there at
all — only the aarch64 cross-compile actually built this file and
surfaced the error. Fixed the same way WU-17's own session fixed it:
naming a `const std::size_t` local first so the vector constructor's sole
argument is a plain identifier, not a parenthesized cast expression. Not a
`CORRECTIONS.md` entry — caught and fixed within the same edit, before any
claim was ever made based on the broken draft, the identical "routine
iteration, not a design/reasoning error" distinction `HANDOFF.md` already
drew for WU-17's own instance of this exact mistake. Recorded here anyway,
briefly, because it is direct evidence for why this session actually ran
the aarch64 cross-compile rather than treating ADR-042's own established
capability as a formality: an x86_64-only verification would have shipped
this bug silently, to be caught only at the real M1 Max terminal.

**Verification.** Built and run in this session's own Linux cloud sandbox
(Ubuntu 24.04), reusing ADR-042's own established capability directly
rather than re-deriving it — `g++-aarch64-linux-gnu`/`qemu-user-static`
installed the same way, confirmed still available in this fresh sandbox
rather than assumed persistent from a prior session:

- Default x86_64 matrix (Clang 18.1.3/GCC 13.3.0, Release/Debug, tile 4/5,
  eight configurations) plus GCC 13 ASan+UBSan at both tile sizes: all
  green, sixteen tests each, `test_chroma_neon`/`test_v210_neon` both
  correctly absent per the shared CMake gate — confirms this unit leaves
  the existing sandbox matrix completely unaffected.
- AArch64 cross-compile (GCC 13.3.0, ad hoc toolchain file, not committed)
  + genuine `qemu-aarch64-static` execution: Release/Debug, tile 4/5, all
  eighteen tests green (the sixteen carried over plus `test_v210_neon` and
  the new `test_chroma_neon`) — this run is what caught the most-vexing-
  parse bug above, on its first attempt, before any fix. GCC 13 with
  `-fsanitize=undefined -fno-sanitize-recover=all` on AArch64: eighteen
  tests green, clean, no UBSan report. `-fsanitize=address` on AArch64
  reproducibly crashes `qemu-aarch64-static` itself
  (`uncaught target signal 11`), confirmed again this session — the exact
  sandbox/emulator limitation ADR-042 already named, not a code defect,
  and not required by this unit's own accept criterion.
- Clang 18.1.3 cross-compile (`--target=aarch64-linux-gnu
  --gcc-toolchain=/usr`, standalone, no CMake — same shape ADR-042's own
  Clang check used), run under `qemu-aarch64-static -L
  /usr/aarch64-linux-gnu`: `PASS test_chroma_neon (34 checks)`, zero
  warnings under this project's full `-Wall -Wextra -Wpedantic
  -Wconversion -Wsign-conversion -Werror` set — confirming both compilers
  this project's matrix already uses for x86_64 compile this unit clean,
  not only GCC.

`WORK-UNITS.md`'s own WU-18 line stays `wip`, not `green`, for the same
procedural reason every other unit's line has (`SESSION-PROTOCOL.md`, "the
assistant does not run `close.sh`") — Steve's own real-terminal `cmake
--build` + `ctest` + `./tools/close.sh 18` run, natively on the M1 Max, is
still outstanding. Every line this unit adds has already executed
correctly, genuinely, in this session's own sandbox on real (emulated)
AArch64 machine code, the same "not merely reasoned through against
headers" standard ADR-042 already established for WU-17; what remains for
the real terminal is confirming AppleClang agrees (a real, open question —
this session's own verification used GCC and Clang's mainline cross
target, not AppleClang) and ASan on native AArch64 hardware, which the M1
Max can attempt without the qemu limitation this session's own sandbox
hit.

Does not reopen `docs/architecture.md`, ADR-004, ADR-020 or ADR-042 — same
relationship every ADR since ADR-020 has to the document; ADR-020's frozen
filter coefficients, rounding convention and edge handling are read as a
fixed input and reproduced exactly, not altered — the NEON path computes
the identical filter, not an approximation of it; ADR-042's sandbox
verification capability, sibling-function design and CMake guard are
reused directly, not re-derived, for exactly the parts genuinely common to
both units, while the vectorisation shape itself is worked out fresh for
chroma's own different kind of irregularity, per the "module-layout
question" section above.

**ADR-044 — WU-19 split into WU-19a (persistent, caller-owned ThreadPool —
this session) and WU-19b (real-time measurement at 576i25 on the M1 Max —
Steve's own job, not scoped with Files:/Accept:); `PipelineParams::pool`'s
design and precedence rule over `threads`; no benchmarking tool committed
this session. Frozen at WU-19a, Phase 4's last unit.**

`WORK-UNITS.md`'s own WU-19 line, going into this session, was barer than
any prior unit's own line has been going in — not even WU-16's or WU-17's
one-line accept criterion, just a title ("Real time at 576i25"). This
session's own first job, per Steve's own brief and `SESSION-PROTOCOL.md`'s
own discipline: real scoping before any code — reading `docs/architecture.md`
section 10's own Phase 4 "done when" line ("8-thread output is bit-identical
to single-threaded, at frame rate"), section 6 (the threading model) and
section 11's own budget/splat discussion ("the splat is the item most likely
to overrun... 576i25 has a 10x larger budget"), plus `src/core/pipeline.cpp`/
`.hpp` (ADR-040/041's own current threading implementation) and
`src/video/v210.cpp`/`chroma.cpp` (ADR-042/043's own two NEON paths) — all
read in full before deciding what this unit actually covers, per this
session's own brief.

**The central fact this unit is scoped around: "at frame rate" is a
real-hardware timing claim, and this project's own Linux cloud sandbox CPU
has no relationship to the M1 Max's actual throughput.** Every prior
Phase-4 unit's own accept criterion was a correctness statement (bit-
identical output, ADR-040/041; bit-identical to scalar reference, ADR-042/
043) — checkable in full inside this sandbox, the same way every unit since
WU-01 has been. WU-19's own accept criterion, read literally off
architecture.md's own Phase 4 "done when" line, is not: "at frame rate" is a
wall-clock measurement against 576i25's own 40ms/frame budget (25 fps), and
nothing about running code faster or slower in an x86_64 cloud container
says anything true or false about whether the same code meets that budget
on 8 M1 Max performance cores. This is the same category of limit ADR-031/
032 already named for hardware this sandbox cannot reach at all (no
Blackmagic SDK, no AppleClang) — except here the gap is not "cannot compile
it here," which this sandbox has repeatedly turned out to handle better than
expected (ADR-042's own aarch64-cross-plus-qemu discovery), but "can compile
and run it here, and the result would not mean what it needs to mean." A
faster or slower number from this sandbox's own CPU is not evidence either
way about the real target hardware, so this unit does not produce one, or
claim one — the same discipline ADR-031/032 already established for
"reasoned through, not asserted, until the real terminal," applied here to a
measurement problem rather than a missing-SDK problem.

**What *is* checkable here, and is this session's own actual scope: the one
concrete piece of implementation work every prior Phase-4 ADR already named
by number as WU-19's own job and left undone.** ADR-040's own "Per-frame
`ThreadPool` construction, not a caller-owned persistent pool — deliberately
left for WU-19": `runFrame()` currently spawns and joins `params.threads` OS
threads on every single call when `params.pool` — this unit's own new field
— is absent, real overhead a persistent, reused-across-frames pool avoids,
and real overhead this sandbox *can* observe changing (fewer OS thread
create/join syscalls per call is not a hardware-dependent claim, it is
countable) even though it cannot say what that overhead is worth in
milliseconds on the M1 Max. This is squarely a correctness-preserving
refactor: whether `runFrame()` constructs its own `ThreadPool` per call or
is handed one that already exists, I6 (integer addition is associative)
still guarantees bit-identical output to the `threads <= 1` oracle — the
exact property `--threads 1` vs `--threads N` (ADR-015) already checks, and
still the right and sufficient oracle for this unit's own change, per this
session's own brief. WU-17's own deferred denser `vld4q_u32` v210 scheme and
WU-18's own deferred `downsampleRowNeon` load-count reduction are **not**
addressed here — both were explicitly deferred pending evidence from
profiling that they are actual bottlenecks (ADR-042/043's own "not decided
here" notes), and no profiling has happened yet; picking either up now would
be optimising a guess, not a measurement, the opposite of what those two
ADRs asked WU-19 to do with them.

**Decision: split into WU-19a (this session, the persistent-pool lifecycle
change) and WU-19b (not this session — Steve's own real-time measurement at
his own terminal), the same "split when the full scope doesn't fit one
sitting the same way" discipline ADR-028/032/040 already used for WU-12a/b,
WU-15a/b and WU-16a/b — except the reason for the split here is not
`SESSION-PROTOCOL.md`'s own 3-file cap (WU-19a alone comes nowhere near it,
below) but the same "this session cannot itself assert the real thing green"
reason ADR-032 already gave for WU-15b (an hour-long unattended hardware run
"exceeds what 'one session, one work unit' sensibly means by a single
sitting," and — the closer parallel here — is not a thing a session with no
hardware access can produce evidence about at all, only reason towards).**

- **WU-19a (this session).** The persistent `ThreadPool` mechanism, plus the
  bit-identity verification this sandbox can actually perform. Scope
  decided *before* writing code, per this session's own brief: does this
  fit `SESSION-PROTOCOL.md`'s own 3-source-files-plus-test cap? Yes, with
  room to spare — two source files, not three: `core/resolve.hpp` (one new
  `PipelineParams::pool` field) and `core/pipeline.cpp` (the threaded
  PASS-1/PASS-2 body factored out into a `runThreaded(..., ThreadPool&,
  ...)` helper, called against either a fresh per-call pool — WU-16a/16b's
  own unchanged behaviour — or the caller's own persistent one). No change
  to `core/pipeline.hpp` at all: `ThreadPool`'s own public interface
  (`size()`, `runOnAll()`) already has everything a caller needs to
  construct one and pass its address in; the class itself needed no new
  member, method or constructor for this unit. `Files:` below.
- **WU-19b (not this session, not yet scheduled in `WORK-UNITS.md`'s own
  numbering beyond this note).** The literal thing architecture.md's own
  Phase 4 "done when" line asks for and WU-19a's own mechanism enables but
  cannot itself verify: run `runFrame()`/`runFrameFile()` at 576i25 (720x576)
  with a real, persistent `ThreadPool` (this unit's own `PipelineParams::pool`)
  at a real worker count on the real M1 Max, and confirm per-frame wall-clock
  time stays under 40ms (25 fps) — "at frame rate," architecture.md 10's own
  words — the same "Steve's own hands-on verification, not a session's own
  job to assert from a terminal" category WU-15b (ADR-032) already used for
  an hour-long endurance run this project's own sandbox could not produce
  either. Not implementation work in the WU-15b sense — no new
  `Files:`/`Accept:` source-file lines — a simple `std::chrono` wrap around
  a `runFrame()`/`runFrameFile()` call, timed at his own terminal, is enough
  to get a real number; no new project machinery is committed for this,
  the same "no committed mechanism for a hardware-only, largely one-off
  need" choice ADR-038 already made for WU-15b's own duration edit and
  ADR-042 already made for not committing its own aarch64 cross-toolchain
  file. If that measurement shows the splat or either NEON path is actually
  the bottleneck, WU-17/18's own already-named deferred refinements
  (`vld4q_u32`, `downsampleRowNeon`'s load-count reduction) — or something
  this session has not anticipated — become a future unit's own job to pick
  up with real evidence behind them; not decided or scoped here.

**A benchmarking tool was considered for this session's own scope and
rejected, deliberately, not merely not thought of.** Two designs were
weighed: (a) build a small `tools/bench_pipeline.cpp` this session, gated
into the build, that Steve runs at his own terminal for WU-19b's own
measurement; (b) build nothing, and let Steve time an existing entry point
(`runFrame()`/`runFrameFile()`, both already public, unchanged) with a
`std::chrono` wrapper of his own at the terminal, the same shape ADR-038's
own WU-15b runbook already used (a temporary, uncommitted, hand-written
measurement, not a permanent committed mechanism). Chosen: (b). A committed
benchmarking tool's own numbers, produced anywhere this session can run it,
would be exactly the kind of sandbox-CPU timing this entry's own opening
section already explains means nothing for the real question — building one
here would not shorten WU-19b's own real work by a single M1 Max
millisecond, and would spend part of this unit's own file budget on tooling
whose only genuine use is at a terminal this session cannot reach anyway.
This is not a claim that a benchmarking tool would never be useful — if
WU-19b's own real measurement, or a later Phase 6 tuning unit (WU-25's own
"profile; tune tile size"), turns out to need one repeatedly rather than
once, that is a future unit's own call to make with real motivation behind
it, the same "not decided here" treatment this project gives every deferred
convenience it names but does not build (ADR-037's own follow-ups, ADR-038's
own "not decided here" on a configurable duration mechanism).

**`PipelineParams::pool`: a non-owning `ThreadPool*`, default `nullptr`,
forward-declared in `core/resolve.hpp` rather than pulled in via a new
`#include "core/pipeline.hpp"`.** `ThreadPool` (`core/pipeline.hpp`,
ADR-040) is used here only as a pointer type — nothing in `core/resolve.hpp`
calls a member on it or needs its size — so a forward declaration
(`class ThreadPool;`) is sufficient and keeps this header's own dependency
graph exactly as it was, the same "does this need a new #include" judgement
ADR-021/026/030 already applied to "does this need a new header." A caller
that actually constructs a `ThreadPool` to pass its address in must already
`#include "core/pipeline.hpp"` itself to do that, so nothing is lost by not
pulling the dependency in here too.

**Precedence when both `PipelineParams::threads` and `PipelineParams::pool`
are set: `pool->size()` alone decides the worker count used; `threads` is
not consulted at all in that branch.** Two designs were weighed: requiring
the two fields to agree (and doing what — asserting, in a codebase with no
runtime-checked preconditions anywhere else, ADR-024's own `Lattice::at()`
convention included? silently preferring one?), or making `pool`'s own
presence alone the complete signal, with `threads` simply unread whenever
`pool != nullptr`. Chosen: the latter — a caller supplying a pool has
already made the sizing decision by constructing it with a particular
`numThreads`; asking the same caller to also keep a second field in sync
with that decision is a foot-gun with no benefit, the same reasoning
ADR-038 already used to reject a new parameter needing "a name, a type, a
default, validation" for a need better met by not inventing the extra
moving part at all. This also does not, and cannot, silently produce wrong
output if a caller does supply a mismatched `threads` value alongside a
real `pool` — I6 already guarantees any true partition of rows into bands
and tiles across workers is bit-identical to the oracle regardless of
worker count, so the only way a mismatch could actually corrupt anything is
if the *implementation* used one field to decide the partition shape and
the other to decide how many worker callbacks actually run — which
`runThreaded()`'s own single `numWorkers = pool.size()` (used for both,
consistently, inside that one function) already rules out by construction,
and which `tests/test_persistent_pool.cpp`'s own
`test_persistent_pool_size_governs_partition_not_threads_field()` checks
directly rather than only by reading the code — a pool of size 3 against a
`threads` field of 99, and separately of 1, both still producing output
bit-identical to the `threads == 1`/`pool == nullptr` oracle.

**`runThreaded()`: the WU-16a/16b threaded PASS-1/PASS-2 body extracted
unchanged into its own function, taking `ThreadPool&` rather than owning
one.** Not a rewrite — every statement inside it (the per-worker generation-
time bin arenas, the row-band `runOnAll()` call, the barrier, the tile-
parallel `runOnAll()` call) is byte-for-byte what WU-16b's own
`runFrame()` already did after constructing its local `ThreadPool pool(
params.threads)`, only now reading `pool` as a parameter instead of a local
variable. `runFrame()` itself becomes a three-way branch: `pool == nullptr
&& threads <= 1` takes the unchanged single-threaded oracle loop (WU-10's
own body, never touched by WU-16a, WU-16b or this unit); `pool != nullptr`
calls `runThreaded()` against the caller's own pool; otherwise (`pool ==
nullptr && threads > 1`) constructs a local `ThreadPool` for exactly this
call and calls `runThreaded()` against it, exactly reproducing WU-16b's own
prior behaviour. Verified, not just reasoned through: the full pre-existing
test suite (`tests/test_threading.cpp`, `tests/test_row_band.cpp`, and
every earlier pipeline-level test) passes completely unchanged against this
refactored `runFrame()` — the same "the oracle's own output did not move"
verification method WU-16a/16b's own sessions already used for their own
byte-level `TileAccum`-reuse and `resolveOneTile()`-generalisation changes.

**Verification.** Built and tested in this session's own Linux cloud
sandbox (Ubuntu 24.04, Clang 18.1.3, GCC 13.3.0, cmake 3.28.3, ninja) across
the same matrix every Phase 4 unit has used: Clang 18 and GCC 13, Release
and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, all
seventeen tests green — the sixteen carried over unchanged plus
`tests/test_persistent_pool.cpp`, ~2.56 million checks in that one binary
alone, zero warnings under this project's full `-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Werror` set), plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes
(clean, no ASan/UBSan report) and GCC 13 with `-fsanitize=thread` at both
tile sizes (clean, no data race reported, both across the full suite and
standalone against `test_threading`/`test_row_band`/`test_persistent_pool`
under `TSAN_OPTIONS=halt_on_error=0`) — checked with particular care since
this unit is the first to let a `ThreadPool` genuinely outlive a single
`runFrame()` call and be driven by more than one such call in sequence, the
same "check empirically, not just by inspection" standard this project
applied to its own first concurrent code at all (WU-16a). `WORK-UNITS.md`'s
own WU-19a line stays `wip`, not `green`, pending Steve's own real-terminal
`cmake --build` + `ctest` + `./tools/close.sh 19a` run, the same procedural
reason every other unit's line has had (`SESSION-PROTOCOL.md`, "the
assistant does not run `close.sh`") — nothing about this unit's own content
is expected to behave differently there than in the sandbox above; unlike
WU-14/WU-15a/WU-17/WU-18, this unit touches no Apple-only surface at all, so
there is no piece of it this sandbox could not already fully verify.

**Files:** `src/core/resolve.hpp`, `src/core/pipeline.cpp` (both extended,
not new), `tests/test_persistent_pool.cpp` (new); plus `CMakeLists.txt`
(`test_persistent_pool` added, same `scatter_test()` pattern as
`test_threading`/`test_row_band` — CMakeLists.txt edits have never counted
against the "3 source files" cap in any earlier unit either).

**Accept:** a `ThreadPool` constructed once, outside `runFrame()`, and
reused across many calls (including calls against different frame
geometries in sequence, and calls whose `PipelineParams::threads` field
deliberately disagrees with the pool's own `size()`) produces output
bit-identical to the `PipelineParams::threads <= 1`, `pool == nullptr`
oracle on every call, not only the first; the existing per-call-construction
threaded path (`pool == nullptr`, `threads > 1`) is unchanged, verified by
the full pre-existing suite passing unmoved; pool reuse across many calls is
itself clean — no hang, no leak — which a stuck `ThreadPool` destructor or a
second-round dispatch bug would show up as a test-process timeout, not a
silently wrong answer. Does **not** include, and does not claim: any
statement about whether `runFrame()`/`runFrameFile()` actually completes
within 576i25's own frame budget on real hardware — that is WU-19b,
unscoped and unbuilt by this entry, deliberately.

Not decided here, deliberately, and named for whoever picks up WU-19b or
whatever comes after it: the actual real-time measurement itself; whether
WU-17's own denser `vld4q_u32` v210 scheme or WU-18's own
`downsampleRowNeon` load-count reduction are worth building, which depends
entirely on evidence WU-19b's own measurement (or later profiling) has not
yet produced; per-tile and per-row-band load balancing beyond the static
interleaved/contiguous partitioning ADR-040/041 already chose (both
entries' own "not decided here," unchanged, still not this unit's job);
and a committed benchmarking tool, considered and rejected above for this
session specifically, not for all time.

Does not reopen `docs/architecture.md`, ADR-015, ADR-021, ADR-024, ADR-026,
ADR-030, ADR-031, ADR-032, ADR-038, ADR-040, ADR-041, ADR-042 or ADR-043 —
same relationship every ADR since ADR-020 has to the document; ADR-015's
single-threaded oracle is preserved deliberately (the `pool == nullptr &&
threads <= 1` branch is untouched, byte for byte, across every unit through
this one); ADR-040's own explicit deferral ("a persistent, caller-owned
`ThreadPool`... WU-19's own job") is completed, not reopened, by
`PipelineParams::pool`; ADR-041's `runThreaded()`-shaped body (the row-band/
tile dispatch this entry extracts unchanged) is reused, not altered;
ADR-042/043's own "not decided here" deferrals of denser NEON schemes are
left exactly as deferred, pending evidence this unit does not produce;
ADR-021/026/030's "does this need a new header/#include" judgement is
applied again, the same way, to a new pointer field; and ADR-031/032/038's
own "reasoned through here, verified for real only at the terminal, no
committed machinery for a hardware-only one-off" discipline is extended to
a measurement problem, the first time this project has hit that particular
shape of gap rather than a missing-SDK or missing-toolchain one.

**ADR-045 — Tile size (Q1) settled: `SCATTER_TILE_LOG2=5` (32x32),
already this project's own default since WU-01, confirmed as the project's
tile size going forward. Frozen at WU-19b, on real M1 Max whole-pipeline
throughput evidence.**

`CMakeLists.txt`'s own tile-size comment has read "Q1, open... WU-09
benchmarks both. Do not resolve Q1 before then" since WU-01, and every
session's own `HANDOFF.md` "Open questions" section has carried Q1
forward, unresolved, straight through WU-09 and every unit since — WU-09's
own accept criterion asked only that both tile sizes *build and pass the
same tests* (`SCATTER_TILE_LOG2` 4 and 5, part of this project's own
standing verification matrix since that unit), not that either be timed
against the other; nothing before this project had a working threaded
pipeline to measure a whole-frame throughput question against honestly in
the first place — a single-threaded splat microbenchmark could only ever
speak to cache behaviour in isolation, not to the question architecture.md
4.5 itself poses ("32x32 may still win on reduced edge replication despite
spilling. This is an empirical question, not an architectural one").

**WU-19b's own real-time measurement is the first evidence this project has
had that can actually answer it, and it was run at both tile sizes for
exactly that reason** — not originally scoped as part of WU-19b's own
accept criterion (`WORK-UNITS.md`'s own WU-19b line names no particular
tile size), but a natural, cheap extension once the measurement mechanism
already existed: rebuild `scatter-core` at `SCATTER_TILE_LOG2=4`, relink
the same scratch benchmark (`DECISIONS.md` ADR-044's own "no committed
tool" scratch-file shape, reused unchanged for a second build), rerun the
identical warped-frame/thread-count sweep. Result, both tile sizes, same
warped 720x576 cylinder-over-zone-plate frame, same 20-iteration warmup and
200-iteration average, same M1 Max, same session:

| threads | 2^5 (32x32) ms/frame | 2^4 (16x16) ms/frame | 2^5 advantage |
|---|---|---|---|
| 1 | 48.766 | 50.269 | 3.1% |
| 2 | 24.665 | 25.710 | 4.2% |
| 4 | 13.101 | 14.349 | 9.5% |
| 8 | 6.868 | 7.528 | 9.6% |

32x32 is faster at every thread count measured — not a one-off at a single
point, and the margin widens rather than narrows as worker count grows
(3-4% at 1-2 threads, up to ~9.5% at 4-8), consistent with architecture.md
4.5's own stated tradeoff: 32x32's accumulator (128 KB across four banks)
exactly saturates the M1 P-core's own 128 KB L1D, where 16x16's (32 KB)
comfortably fits with margin to spare — the plausible mechanism being that
16x16's own extra edge-replication overhead (architecture.md 4.4: "roughly
6% at 32x32, 12% at 16x16") costs more, at real per-tile fragment volumes
and worker counts, than any cache-fit advantage its smaller footprint buys,
the same tradeoff 4.5's own table already flagged as needing a real
measurement rather than an assumption either way.

**Decision: `SCATTER_TILE_LOG2=5` is confirmed and settled as this
project's own tile size going forward** — not a new choice (it has been
`CMakeLists.txt`'s own default since WU-01), but no longer merely an
unexamined default sitting on an explicitly "open" question. `Q1` is
closed. Both values remain fully configurable and fully exercised by this
project's own standing verification matrix (`SCATTER_TILE_LOG2` 4 and 5,
every unit's own sandbox verification since WU-09) — nothing about this
decision removes tile 4 from the build or the test matrix, the same way
confirming `bmdModePAL` (ADR-033) did not remove `bmdModePALp` from the
codebase's own vocabulary, only from being the thing actually used. A
future unit revisiting resolution (Phase 6's own WU-25, "1080p25, then
1080p50; tile-size tuning") is where this question would need re-asking,
not before — this entry's own evidence is 720x576-scale only, and
architecture.md 4.5's own cache-fit reasoning (128 KB tile exactly matching
L1D size) is itself resolution-independent in principle but untested at
any resolution larger than this session's own measurement.

**Scope, stated precisely: this settles which tile size, not whether the
splat generally, or either NEON path, needs further work.** That question
— already asked and answered "no evidence either is a bottleneck" by this
same session's own WU-19b entry in `WORK-UNITS.md`, at both tile sizes now
— is unaffected by which of the two turned out faster; both stayed
comfortably under the 40ms/8-thread budget architecture.md 10 names.

Does not reopen `docs/architecture.md`, ADR-002 or ADR-044 — same
relationship every ADR since ADR-020 has to the document; ADR-002's
four-bank split (the reason a "tile" has an accumulator at all) is
unaffected — this entry settles the tile's own edge length, not whether to
tile or how the four banks are addressed; ADR-044's own WU-19b scope
("time `runFrame()`/`runFrameFile()`... at a real worker count on the real
M1 Max... no committed benchmarking tool") is extended to a second build
configuration using the exact same uncommitted scratch mechanism, not
altered or reopened.

**ADR-046 — WU-20 split into WU-20a (a portable, allocation-free SPSC ring
buffer — this session, genuinely built and run in the Linux cloud sandbox,
including under ThreadSanitizer) and WU-20b (the DeckLink-specific capture
object — not this session, reasoned through only, same shape as WU-14/
WU-15a); the real `IDeckLinkInput`/`IDeckLinkInputCallback` shape confirmed
against the real SDK; two architecture.md 7 inaccuracies found by that
reading; and what three real capture samples actually do for frame
retention and format-change restart, which differs sample to sample. Frozen
at WU-20a, Phase 5's first unit.**

`WORK-UNITS.md`'s own WU-20 line, going into this session, named its
hardware target (UltraStudio Recorder 3G, ADR-039) but had no `Files:`/
`Accept:` scoping at all — this session's own first job, per Steve's own
brief and this project's established practice for a new hardware surface
(ADR-031/032's own reading-before-scoping discipline, cited by WU-20's own
line), was reading the real SDK's `IDeckLinkInput`/capture-callback shape
before writing anything, not assuming from architecture.md 7's own Input
subsection — which ADR-039 already flags as describing the original,
now-superseded single-full-duplex-device design, unrevised.

**What `IDeckLinkInput`, `IDeckLinkInputCallback` and
`IDeckLinkVideoInputFrame` actually are, read directly from
`Mac/include/DeckLinkAPI.h` rather than assumed from architecture.md's own
snippet.** `IDeckLinkInput` (obtained via `QueryInterface` from `IDeckLink`,
same convention ADR-031 already established for `IDeckLinkOutput`) declares
`DoesSupportVideoMode()`, `GetDisplayMode()`/`GetDisplayModeIterator()`,
`EnableVideoInput()`/`EnableVideoInputWithAllocatorProvider()`/
`DisableVideoInput()`, `GetAvailableVideoFrameCount()`, the audio-input
trio, `StartStreams()`/`StopStreams()`/`PauseStreams()`/`FlushStreams()`,
`SetCallback(IDeckLinkInputCallback*)` and `GetHardwareReferenceClock()` —
confirming architecture.md 7's own sketch (`EnableVideoInput`, `SetCallback`,
`StartStreams`) is accurate as far as it goes, just incomplete.
`IDeckLinkInputCallback` has exactly two methods,
`VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode*,
BMDDetectedVideoInputFormatFlags)` and
`VideoInputFrameArrived(IDeckLinkVideoInputFrame*, IDeckLinkAudioInputPacket*)`
— both named exactly as architecture.md 7 already has them.
`IDeckLinkVideoInputFrame` extends `IDeckLinkVideoFrame` (adding only
`GetStreamTime()`/`GetHardwareReferenceTimestamp()`), and
`IDeckLinkVideoFrame` itself declares `GetWidth()`/`GetHeight()`/
`GetRowBytes()`/`GetPixelFormat()`/`GetFlags()`/`GetTimecode()`/
`GetAncillaryData()` — no `GetBytes()` anywhere on either interface.
`bmdVideoInputEnableFormatDetection` (`BMDVideoInputFlags`, value `1 << 0`)
is exactly the flag architecture.md 7 names; `BMDVideoInputFormatChangedEvents`
carries three independent bits, `bmdVideoInputDisplayModeChanged`,
`bmdVideoInputFieldDominanceChanged` and `bmdVideoInputColorspaceChanged`.

**Two architecture.md 7 inaccuracies this reading found, the same kind
ADR-032 already found on the output side — recorded here, not corrected in
the document itself, per this project's own established convention (no ADR
since ADR-020 has ever edited architecture.md; ADR-039's own closing
paragraph states the reasoning most recently).**

- **"Always use `GetBytesPerRow()`"" names a method that does not exist.**
  The real interface declares `GetRowBytes()` (`IDeckLinkVideoFrame`, above)
  — not a behavioural difference, just the wrong name, but exactly the kind
  of thing that would have looked plausible if guessed rather than read,
  the same caution ADR-031's own `IDeckLink`-location finding names.
- **`GetBytes()` is not directly on `IDeckLinkVideoFrame`/
  `IDeckLinkVideoInputFrame` on the input side either.** ADR-032 already
  found this true for output (`IDeckLinkVideoBuffer`, obtained via
  `QueryInterface(IID_IDeckLinkVideoBuffer, ...)`, bracketed by
  `StartAccess()`/`EndAccess()`); the same interface serves both directions
  (`IDeckLinkVideoBuffer` is generic, not output-specific), and neither
  `IDeckLinkVideoFrame` nor `IDeckLinkVideoInputFrame` gained a `GetBytes()`
  of their own anywhere in this reading. Neither half of WU-20 actually
  needs this: WU-20a (below) never touches a frame's pixel contents at all,
  and WU-20b's own scope (also below) stops at retaining and queueing the
  frame handle — reading pixel bytes into this project's own pipeline is
  WU-21's job ("Full loop through at 576i25"), not named or built here. This
  is intentionally the same "stop at the boundary this unit's own name
  implies" discipline WU-14 already used (enumeration, no stream opened) and
  WU-15a already used (loop one frame, no sequence decoding).

**Three real C++ capture samples surveyed
(`Samples/CaptureStills/DeckLinkInputDevice.cpp`, `Samples/InputLoopThrough/
DeckLinkInputDevice.cpp`, `Samples/CapturePreview/DeckLinkInputDevice.mm`) —
read in full, not assumed from one of them generalizing to all three, because
they turn out to disagree with each other on exactly the two questions
WU-20's own line names.**

- **Frame retention: all three do "copy or retain, then return"
  (architecture.md 7's own phrase), but no two do it quite the same way.**
  `CaptureStills` calls `videoFrame->AddRef()` directly and pushes the raw
  pointer into a `std::queue<IDeckLinkVideoFrame*>` guarded by a
  `std::mutex`/`std::condition_variable`, with the consumer (a separate
  thread, `WaitForVideoFrameArrived()`) eventually calling `Release()` once
  done. `InputLoopThrough` and `CapturePreview` both instead construct their
  own `com_ptr<IDeckLinkVideoInputFrame>` directly from the raw callback
  parameter — the SDK's own sample `com_ptr`'s raw-pointer constructor
  AddRefs, exactly the borrowing case this project's own `ComPtr`'s
  raw-pointer constructor already handles identically (ADR-031) — and hand
  that smart pointer onward via `std::function`/`std::shared_ptr`. All three
  are the same underlying operation (`AddRef`, hand off, `Release` once the
  consumer is done); this project's own `ComPtr`'s existing borrowing
  constructor (`explicit ComPtr(T* borrowedPtr)`, already built at WU-14,
  unchanged) is exactly the right tool for whichever WU-20b's own capture
  object ends up doing with an arriving `IDeckLinkVideoInputFrame*` — no new
  `ComPtr` capability is needed for this, only its already-existing borrowing
  case, applied here for the first time to a genuinely borrowed callback
  parameter rather than a factory/enumerator result (ADR-031's own
  anticipated-but-not-yet-exercised distinction, "unlike a callback parameter
  such as a future `VideoInputFrameArrived(IDeckLinkVideoInputFrame*)`...
  which is borrowed and must be retained by the callee").
- **Format-change restart: two samples do it, one deliberately does not.**
  `CaptureStills` and `CapturePreview` both restart the stream
  (`StopStreams()`, then `EnableVideoInput()` with the newly detected mode
  and pixel format, then `StartStreams()`) inside `VideoInputFormatChanged()`,
  gated on `notificationEvents & (bmdVideoInputDisplayModeChanged |
  bmdVideoInputColorspaceChanged)` — confirming architecture.md 7's own claim
  ("restart the stream on mode change... without it, switching source
  standards silently produces garbage") against real, working code, not
  merely trusting the prose. `InputLoopThrough` does not: its own
  `VideoInputFormatChanged()` only invokes a notification callback
  (`m_videoFormatChangedCallback`) and never calls `StopStreams()`/
  `EnableVideoInput()`/`StartStreams()` itself — a deliberately different
  design for a sample built to loop through one continuously-configured mode,
  not to adapt to a changing one. Since WU-20's own line names "format
  detection" as one of its three pieces, `CaptureStills`/`CapturePreview`'s
  restart-on-change behaviour is the one WU-20b (below) follows, not
  `InputLoopThrough`'s non-restarting variant — the disagreement between
  samples is exactly why this needed reading three, not one, before deciding.
  `CaptureStills` additionally restarts a second way, inside
  `VideoInputFrameArrived()` itself: when a frame's own
  `bmdFrameHasNoInputSource` flag transitions from set to clear (signal
  recovery, not a format change), it also calls `StopStreams()`/
  `FlushStreams()`/`StartStreams()` once before accepting frames as good.
  This is a different concern from format detection (signal *presence*, not
  signal *format*) and interacts with WU-20a's own ring in a way this session
  has not worked out (should a `bmdFrameHasNoInputSource` frame be pushed to
  the ring at all, or filtered before `tryPush()`?) — named here, not decided
  or built, for WU-20b or WU-21 to pick up.

**Why none of the three samples' own frame-handoff mechanism satisfies
architecture.md 6's own requirement for the capture callback thread, and why
this project's ring buffer is therefore a new design, not adapted from the
SDK — unlike `ComPtr` (ADR-031) and the preroll/refill idiom (ADR-032), both
of which were.** Architecture.md 6: "Capture callback thread (driver-owned).
Retains the frame, pushes to a lock-free ring, returns immediately. Never
blocks, never allocates." `CaptureStills`' own `std::queue` push allocates
(a queue node) and a full, bounded queue would block a producer — moot in
that sample only because it never actually bounds its own queue's size, not
because the mechanism itself is non-blocking by design.
`InputLoopThrough`/`CapturePreview` instead invoke a `std::function`
synchronously, on the callback thread itself, before returning — not "push
and return immediately" at all; whatever the invoked callback body does runs
with the driver's own callback thread held for its entire duration.
architecture.md's own requirement is stricter than any of the three real
samples this project has read provide, so WU-20a's own `RingBuffer<T,
Capacity>` (`src/core/ring_buffer.hpp`, new, header-only) is built directly
against that requirement rather than adapted from an existing idiom.

**The WU-20a/WU-20b split, decided after this reading, not before it — the
same order ADR-028/032/040/044 already used for WU-12a/b, WU-15a/b, WU-16a/b
and WU-19a/b.** WU-20's own three named pieces — format-detection-aware
`EnableVideoInput`, a capture callback implementing `IDeckLinkInputCallback`
with correct frame retention, and a ring buffer — do not all carry the same
kind of dependency. The first two need `DeckLinkAPI.h` and cannot be built or
run in this session's own Linux cloud sandbox at all (no Blackmagic SDK, no
AppleClang/Xcode toolchain — the same gap ADR-031/032 already named for
WU-14/WU-15a, unchanged). The ring buffer needs neither: as designed above,
it is ordinary, portable C++20 with no DeckLink or platform dependency
whatsoever, the same "core/ — portable, zero platform dependencies" charter
architecture.md 8 already states for every other file in that directory.
Combining all three into one unit would force the ring buffer's own
correctness — the one genuinely new (not-adapted-from-a-sample) design this
whole unit introduces, and concurrency-critical besides — into WU-14/WU-15a's
own "reasoned through against headers, entirely unverified until the real
terminal" shape, for no reason: nothing about a fixed-capacity SPSC ring
templated on an arbitrary `T` requires the SDK to compile, run, or stress
under ThreadSanitizer, the same "check concurrency empirically, not just by
inspection" standard WU-16a (ADR-040) established for this project's first
concurrent code. Splitting lets WU-20a get that verification for real, this
session; WU-20b inherits WU-14/WU-15a's own unavoidable shape for the parts
that actually need the SDK.

- **WU-20a (this session).** `src/core/ring_buffer.hpp` (new, header-only) —
  `RingBuffer<T, Capacity>`, single-producer/single-consumer, fixed capacity,
  allocation-free after construction. `tests/test_ring_buffer.cpp` (new) —
  one source file plus its test, comfortably within
  `SESSION-PROTOCOL.md`'s cap (the same header-only-plus-test shape WU-07's
  `core/jacobian.hpp` already used). Design, frozen here:
  - **Single producer, single consumer only** — concurrent callers on the
    same side are not supported and not guarded against, the same
    "caller's own bug, not guarded against here" convention this codebase
    already uses for unchecked preconditions elsewhere (`Lattice::at()`'s
    row/col bounds, `ThreadPool::runOnAll()`'s "`fn` must not throw"). This
    matches WU-20b's own future shape exactly: one driver-owned callback
    thread producing, one consumer thread draining (WU-21's own job).
  - **Fixed capacity, a compile-time template parameter, with no default
    chosen by this unit.** The same "do not invent an operating-point
    number nobody has decided yet" discipline ADR-023 already used for
    `maxK` and ADR-024 for the supersample thresholds — sizing a real
    capture ring against actual buffering needs (architecture.md 7's own
    "Expect 3-4 frames end-to-end... 60-80ms at 50p") is WU-20b's own
    concern, once it exists to have one; `tests/test_ring_buffer.cpp`
    exercises several arbitrary capacities (3, 4, 5, 16) to prove the class
    itself is correct at any of them, not to pick one.
  - **One slot always left empty** (the classic circular-buffer technique),
    so `head_ == tail_` is an unambiguous empty test with no second,
    separately-synchronized size counter needed. Usable capacity is
    `Capacity`, not `Capacity + 1` — checked directly, not just documented,
    by `test_capacity_reports_usable_slots_not_backing_storage()`.
  - **A full ring drops the new item and increments a counter; it never
    blocks, waits, or overwrites the oldest entry.** `tryPush()` returns
    `bool`; on failure the caller's own `T&&` argument is left untouched —
    still fully owned by the caller, exactly as if `tryPush()` had never
    been called — so a caller passing a `ComPtr<IDeckLinkVideoInputFrame>`
    that fails to push still correctly releases its own `AddRef()` when that
    local `ComPtr` goes out of scope at the end of the callback, the same
    "no reference silently retained past its owner's intent" property
    architecture.md 12's own risk table names ("reference-count leaks lock
    the device") for a different failure route. `droppedCount()` — a
    relaxed `std::atomic<std::size_t>`, the same convention
    `io/decklink_output.hpp`'s `PlaybackStats` (WU-15a) already uses for its
    own atomics — is this class's own equivalent surfaced statistic, for
    whichever future caller wants to log it periodically the way
    `PlaybackStats` already is.
  - **Acquire/release, not a mutex — the textbook single-producer/
    single-consumer construction.** The producer's relaxed load of its own
    `head_`, acquire load of `tail_` (to check fullness against the
    consumer's latest progress), slot write, then release store of the new
    `head_`; the consumer's mirror image against `tail_`/`head_`. This
    publishes a slot's contents together with the index update that makes it
    visible, in both directions, with no lock anywhere on the hot path —
    checked empirically, not just reasoned through (below), since a subtly
    wrong memory-ordering argument is exactly the kind of claim this
    project's own C-011/C-012 lessons say not to trust on inspection alone.
  - **A popped slot is explicitly reset to a default-constructed `T`,
    rather than relying on whatever state `T`'s own move constructor leaves
    a moved-from object in.** Every retained-handle type this project has
    (`ComPtr<T>`) already nulls its own moved-from pointer, making this
    redundant for that one case — but a ring buffer whose whole reason to
    exist is not silently retaining a reference past its owner's intent is
    worth making that property true by construction, for any `T`, not only
    the one this project happens to use today.

  **A genuine compile error this unit's own real-compiler verification
  caught, fixed before any claim was made based on it — the same "routine
  iteration, not a design/reasoning error" distinction `HANDOFF.md`/ADR-043
  already drew for WU-17/WU-18's own most-vexing-parse mistakes, not a
  `CORRECTIONS.md` entry for the identical reason.** `tests/
  test_ring_buffer.cpp`'s first draft wrote
  `CHECK(RingBuffer<Tracked, 5>::capacity() == std::size_t(5));` directly —
  `CHECK` (`tests/harness.hpp`) is a single-argument function-like macro, and
  the template argument list's own comma split this into two macro
  arguments, a hard compile error under every compiler tried. Fixed by
  binding the specialization to a local `using Ring5 = RingBuffer<Tracked,
  5>;` alias first, the same "name it before using it inside a
  single-argument macro" fix this file's own comment now documents inline.

  **Verification.** Built and run in this session's own Linux cloud sandbox
  (Ubuntu 24.04, GCC 13.3.0, Clang 18.1.3, CMake 3.28.3, Ninja) — genuinely,
  not reasoned through, the whole existing project (all eighteen tests, not
  `test_ring_buffer` in isolation) rebuilt and re-run at every point below,
  confirming this addition leaves every earlier unit's own result unmoved:
  GCC and Clang, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (`ring_buffer.hpp`
  itself has no tile-size dependency at all — confirmed directly, not
  assumed, by grep before writing this sentence — so this axis exists only
  to confirm the addition does not somehow interact with it, which it does
  not), GCC with `-fsanitize=address,undefined -fno-sanitize-recover=all`,
  and GCC with `-fsanitize=thread` against the real concurrent
  producer/consumer test (`TSAN_OPTIONS=halt_on_error=0`, both standalone and
  as part of the full suite) — all eighteen tests green throughout, zero
  warnings under this project's full `-Wall -Wextra -Wpedantic -Wconversion
  -Wsign-conversion -Werror` set, no ASan/UBSan/TSan report. Additionally,
  and not part of this project's own established per-unit matrix until now:
  Clang's own `-fsanitize=address,undefined` and `-fsanitize=thread` builds
  were also run standalone against `test_ring_buffer.cpp` directly (this
  session's own sandbox initially lacked the `libclang-rt-18-dev` package
  those need; installed via `apt-get`/`dpkg` before retrying) — both clean,
  the first time this project has exercised Clang's own sanitizers rather
  than GCC's alone. This is a materially stronger verification than any
  prior DeckLink-adjacent unit has had going into its own real-terminal
  confirmation: WU-14 and WU-15a were "reasoned through against headers,
  entirely unverified until the real terminal" by necessity (no SDK, no
  AppleClang, in that session's own sandbox); WU-20a needed neither, and got
  genuine execution, including under two different sanitizer implementations
  checking its concurrency specifically. `WORK-UNITS.md`'s own WU-20a line
  stays `wip`, not `green`, for the same procedural reason every other unit's
  line has (`SESSION-PROTOCOL.md`, "the assistant does not run `close.sh`")
  — nothing about this unit's own content is expected to behave differently
  at Steve's own terminal than in the sandbox above.

- **WU-20b (not this session, not yet scheduled in `WORK-UNITS.md`'s own
  numbering beyond this note).** `src/io/decklink_input.hpp`/`.cpp` (new) —
  a capture object mirroring `io/decklink_output.hpp`'s own
  `LoopedFramePlayback` shape (ADR-032): implements `IDeckLinkInputCallback`
  directly (real `IUnknown` refcounting, `ComPtr::adopt()` on `create()`, the
  same pattern this project already has one working instance of), calls
  `EnableVideoInput()` with `bmdVideoInputEnableFormatDetection` after
  confirming `IDeckLinkProfileAttributes::GetFlag(
  BMDDeckLinkSupportsInputFormatDetection, ...)` reports the device actually
  supports it (`CapturePreview`'s own pattern, checked rather than assumed —
  this project already has the identical `IDeckLinkProfileAttributes`-based
  capability-check idiom for `BMDDeckLinkVideoIOSupport`, WU-14/ADR-031),
  retains each arriving frame via `ComPtr`'s existing borrowing constructor
  and pushes it into a caller-owned `RingBuffer<ComPtr<IDeckLinkVideoInputFrame>,
  N>` (WU-20a, above; `N` — the actual capacity — is this unit's own number to
  pick, not decided here), and restarts the stream in `VideoInputFormatChanged()`
  on `bmdVideoInputDisplayModeChanged | bmdVideoInputColorspaceChanged`
  (`CaptureStills`/`CapturePreview`'s own pattern, above — not
  `InputLoopThrough`'s non-restarting one). Device selection generic, by
  `supportsCapture` and a live `QueryInterface(IID_IDeckLinkInput)` succeeding
  — the same "not device-specific" convention ADR-034 already established for
  output, applied here even though ADR-039 names the intended physical device
  (UltraStudio Recorder 3G) — the code does not need to know that name to
  find the right device, only the right capability bits. `tests/
  test_decklink_input.cpp` (new) — hardware-only, gated on
  `BLACKMAGIC_SDK_DIR` exactly as `test_decklink_device.cpp`/
  `test_decklink_output.cpp` already are. This remains reasoned-through-only
  until built and run at Steve's own real terminal, the identical shape
  ADR-031/032 already used for WU-14/WU-15a and for the identical reason:
  no Blackmagic SDK, no AppleClang/Xcode toolchain, in this session's own
  Linux cloud sandbox.

  **Not decided here, deliberately, and named for whoever picks up WU-20b:**
  the ring's own capacity `N`; whether a frame carrying
  `bmdFrameHasNoInputSource` should be pushed to the ring at all or filtered
  before `tryPush()`; `CaptureStills`' own second restart trigger (signal
  recovery inside `VideoInputFrameArrived()`, distinct from format-mode-change
  restart, above); and the consumer side of the ring — draining it into
  `runFrame()`/`runFrameFile()` is WU-21's own job ("Full loop through at
  576i25"), not this one's.

Does not reopen `docs/architecture.md`, ADR-009, ADR-013, ADR-021, ADR-030,
ADR-031, ADR-032, ADR-034, ADR-039 or ADR-040 — same relationship every ADR
since ADR-020 has to the document; ADR-031's `ComPtr` (including its existing
borrowing constructor, exercised here for a genuinely borrowed callback
parameter for the first time, not changed) and `BLACKMAGIC_SDK_DIR`-gated
"fail soft with no SDK present" shape are reused unaltered; ADR-032's own
`GetBytes()`-is-not-on-the-frame finding is extended to the input side, not
revisited; ADR-034's "select by capability, not by model name" device
convention is extended to input; ADR-039's own naming of the UltraStudio
Recorder 3G as this project's input target is unaffected — this entry decides
how the *code* finds a device, not which physical device Steve attaches;
ADR-040's "check concurrency empirically" standard is applied to a second,
differently-shaped concurrent primitive (lock-free/atomic rather than
mutex-based) for the first time, not weakened; and ADR-021/030's own "does
this need its own header" judgement is applied again, the same way, to
`ring_buffer.hpp`'s placement in `core/` rather than `io/` — motivated by
capture, but with zero DeckLink dependency of its own, the same reasoning
that already placed `file_source.cpp`/`file_sink.cpp` in `scatter-core`
despite living under `src/io/`.

**ADR-047 — WU-20b (DeckLink capture object) built this session as one
unit, not split further; the ring capacity, pixel-format-on-restart and
no-input-source-frame questions ADR-046 left open, all decided; the real
SDK's own `CaptureStills` signal-recovery restart reused directly; and why
this sandbox cannot compile or run any of it. Frozen at WU-20b, Phase 5's
second unit.**

`WORK-UNITS.md`'s own WU-20b entry, going into this session, was a sketch
only — ADR-046's own closing paragraph named `src/io/decklink_input.hpp`/
`.cpp` and `tests/test_decklink_input.cpp` but explicitly declined to freeze
`Files:`/`Accept:`, per this project's own "the sketch's own session does the
real scoping" discipline. This session's own first job, per that discipline
and per `SESSION-PROTOCOL.md`'s own reading-before-scoping rule: re-read the
real SDK's `IDeckLinkInput`/`IDeckLinkInputCallback`/`IDeckLinkVideoInputFrame`
shape directly (not just ADR-046's own summary of it), re-read the three real
capture samples' own frame-retention and format-change-restart code (not just
ADR-046's own account of what they do), and re-read this project's own
`decklink_device.hpp`/`.cpp`, `com_ptr.hpp`, `decklink_output.hpp`/`.cpp` and
`ring_buffer.hpp` for established idiom — all done this session before any
line of `decklink_input.hpp`/`.cpp` was written. Confirmed against the real
headers/samples: every shape ADR-046 recorded (the `IDeckLinkInput`/
`IDeckLinkInputCallback` member lists, `bmdVideoInputEnableFormatDetection`,
the three samples' own disagreeing frame-handoff and restart behaviour) reads
exactly as that entry describes — nothing corrected, nothing re-litigated.

**The split question: does WU-20b itself fit one session, or does it need
its own further split (`SESSION-PROTOCOL.md`'s "touch at most 3 source files
plus its test... if a unit cannot meet this, split it before starting")?**
Decided, after the reading above, not before it — the same order every prior
split in this project used (WU-12a/b, WU-15a/b, WU-16a/b, WU-19a/b, WU-20a/b
itself). **No further split.** ADR-046's own sketch already named exactly two
new source files (`decklink_input.hpp`, `decklink_input.cpp`) plus one test —
comfortably within the cap, with a full file to spare, the same shape WU-14
(`com_ptr.hpp` plus `decklink_device.hpp`/`.cpp`) and WU-15a
(`decklink_output.hpp`/`.cpp`) both already used for their own single-session
DeckLink units. The candidate splits the session's own brief named going in —
"`EnableVideoInput` + format detection" versus "the callback/retention/
ring-push wiring" — do not correspond to two independently useful pieces the
way WU-12a/WU-12b (shape versus compositing) or WU-16a/WU-16b (PASS 2 versus
PASS 1) did: `EnableVideoInput` with format detection *only* matters because
something is listening for the frames and format-change notifications it
produces, and the callback/retention/ring-push wiring *only* matters once
video input is actually enabled — splitting them would produce two units
where neither is independently useful on its own, violating
`SESSION-PROTOCOL.md`'s own "independently useful" requirement rather than
satisfying its file-count cap. This mirrors `LoopedFramePlayback` (WU-15a)
exactly: one class that both enables the stream and implements the
completion callback, not two. A candidate real-hardware-endurance split (the
WU-15a/WU-15b shape — "this session cannot itself assert the real thing
green") was also weighed and rejected for this unit specifically: WU-15b's
own reason for existing was a genuinely unattended *one-hour* run, longer
than one sitting can sensibly mean; nothing about WU-20b's own `Accept:`
(below) asks for anything beyond a bounded, few-second smoke run of the same
order WU-15a's own test already uses — if a future session's own real-hardware
experience surfaces a genuine need for an unattended capture endurance run,
that is its own unit to scope then, with real evidence behind it, not
invented here on spec (the same "not decided here, no evidence yet"
discipline ADR-044 already used for WU-19b's own throughput question).

**Real `Files:`/`Accept:`, frozen — see `WORK-UNITS.md`'s own updated WU-20b
entry for the literal text; recorded here for the reasoning.** `Files:`
`src/io/decklink_input.hpp`, `src/io/decklink_input.cpp` (both new),
`tests/test_decklink_input.cpp` (new); plus `CMakeLists.txt`
(`decklink_input.cpp` added to the existing `scatter-decklink` target,
`test_decklink_input` added alongside `test_decklink_device`/
`test_decklink_output` — CMakeLists.txt edits have never counted against the
"3 source files" cap in any earlier unit either).

**`CaptureSource`, mirroring `LoopedFramePlayback`'s own shape (ADR-032)
exactly, applied to input for the first time.** Implements
`IDeckLinkInputCallback` directly — this class *is* the callback registered
with `SetCallback` — with a real, non-trivial `IUnknown` reference count,
`ComPtr::adopt()` on `create()` for the constructor's own initial reference,
the identical pattern this project already has one working instance of on
the output side. `create()` confirms `IDeckLinkProfileAttributes::GetFlag(
BMDDeckLinkSupportsInputFormatDetection, ...)` reports true (`CapturePreview`'s
own checked-not-assumed idiom, already cited in ADR-046) — a hard requirement
of this unit, not a soft fallback, since WU-20's own line names "format
detection" as one of its three original pieces (`WORK-UNITS.md`) — confirms
`DoesSupportVideoMode()` for the caller's requested display mode (the same
precautionary check `LoopedFramePlayback::startWith()` already makes on the
output side), then follows `CaptureStills`'/`CapturePreview`'s own call order
(`SetCallback`, `EnableVideoInput`, `StartStreams`) rather than inventing a
different one.

**Three design questions ADR-046 explicitly left open, decided now:**

- **Ring capacity: 8, a fixed compile-time constant
  (`kCaptureRingCapacity`), not a caller-supplied template parameter.**
  architecture.md 7 names "3-4 frames end-to-end... 60-80ms at 50p" as the
  expected buffering depth; 8 is double the high end of that range —
  headroom over the documented expectation, not the bare minimum, the same
  margin WU-15a's own preroll (half a second of frames, not the SDK's
  illustrative "3-frame" figure) already chose for a comparable buffering
  decision. Fixed rather than caller-configurable for the same reason
  ADR-023's `maxK` and ADR-024's supersample thresholds are configurable
  *parameters* while `kLatticeSize`/`kTileSize` are fixed *constants* — this
  is a property of a data structure this project fixes once, not an
  operating point a caller chooses per use, and nothing about WU-20b's own
  scope gives a caller a reason to want a different one. Not tuned against a
  real measured capture-to-consume latency — WU-21's job, once a consumer
  actually drains this ring; flagged, not resolved, the same "not decided
  here, no evidence yet" deferral ADR-044 already used for WU-19b's own
  throughput question.
- **Pixel format never changes on a format-change restart — every
  `EnableVideoInput()` call, initial or restarted, requests
  `bmdFormat10BitYUV` regardless of `detectedSignalFlags`.** `CaptureStills`/
  `CapturePreview` both pick a pixel format from the detected signal's own
  RGB-vs-YUV and bit-depth flags, reasonable for a general-purpose capture
  utility built to accept whatever a user points an HDMI source at. This
  project is not that: ADR-005 already fixes 4:2:2 v210 as the only
  supported I/O format anywhere in this codebase (no RGB unpack exists, no
  8-bit or 12-bit path exists), and ADR-039 already names the real target
  hardware as a fixed-format SDI device, the UltraStudio Recorder 3G — there
  is no legitimate signal this project's own pipeline could receive that
  would make adapting pixel format on the fly anything other than a silent
  no-op followed by a pipeline that cannot read the result. A restart whose
  detected signal genuinely is not 10-bit YUV (a misconfigured or
  wrong-standard source) is therefore an honest failure, not a case to
  paper over: `EnableVideoInput()` at a mismatched pixel format returns
  something other than `S_OK`, and `VideoInputFormatChanged()`'s own handling
  of that (below) stops the object cleanly rather than silently continuing
  in some indeterminate state.
- **A `bmdFrameHasNoInputSource` frame is filtered before ever attempting a
  push, not queued for a future consumer to filter.** Consistent with this
  project's own established "never fabricate a destination the warp never
  produced" reasoning (ADR-024's off-raster-sample drop is the closest
  precedent), a frame carrying no real signal is not real capture content;
  handing one to a future consumer indistinguishably from a genuine frame
  would be exactly that kind of fabrication, one step earlier in the
  pipeline than any ADR before this one has drawn that line. `CaptureStills`'
  own signal-recovery restart — on the first valid frame after an invalid
  one, `StopStreams()`/`FlushStreams()`/`StartStreams()` before accepting
  frames as good, and that first recovery frame itself is not queued either
  — is reused directly, not reinvented: a real, shipped Blackmagic sample's
  answer to a real problem (transient garbage immediately after signal
  reacquisition), the same "reuse a working SDK idiom rather than
  hand-roll a differently-shaped one" preference ADR-031 already applied to
  `ComPtr` and ADR-032 already applied to the preroll/refill mechanism —
  in contrast to WU-20a's own ring buffer, which *was* a new design, because
  architecture.md's own requirement for it was stricter than anything any
  sample provided. No such gap exists here: nothing in architecture.md
  contradicts `CaptureStills`' own recovery-restart behaviour, so there is no
  reason to invent a different one.

**Two failure paths — a failed format-change restart, a failed
signal-recovery restart — both call a shared `stopFromCallback()` rather than
`stop()` itself, and both are unverified in a way this project has not
previously flagged.** Every prior "stop cleanly on failure" path in this
project (`LoopedFramePlayback::create()`'s own failure branches, ADR-032) is
called from a normal call stack, not from inside a callback the SDK itself is
in the middle of invoking. `stopFromCallback()` — `StopStreams()`,
`SetCallback(nullptr)`, `DisableVideoInput()`, guarded by the same
`m_stopping` compare-exchange `stop()` itself uses, so a later external
`stop()` call or a second callback-driven failure is a no-op — is reasoned
through against the real SDK's own documented behaviour (nothing in
`DeckLinkAPI.h` forbids calling `SetCallback`/`DisableVideoInput` from within
a callback the callback interface itself is currently executing, and
`CaptureStills`' own `VideoInputFrameArrived()` already calls
`StopStreams()`/`FlushStreams()`/`StartStreams()` — a comparable
call-from-within-the-callback pattern — on its own signal-recovery path) but
**not confirmed safe by execution**, the same category of gap ADR-031's own
`setWorkerQoS()` note and ADR-042's own AppleClang-vs-mainline-Clang note
already used for a piece of a unit reasoned through but not itself run. If
this turns out unsafe at the real terminal, it is this unit's own bug to fix
there, not a case for weakening the reasoning above without evidence.

**Real-hardware test design: a genuine loopback, not a synthetic
stand-in.** Unlike WU-15a's own test (which could manufacture its own file
source — a real signal on the *output* side is just data this project's own
pipeline already knows how to produce), a capture unit's own smoke test has
no equivalent: this sandbox cannot synthesize an SDI signal for the Recorder
3G to receive. `tests/test_decklink_input.cpp`'s own header comment
documents the concrete real-hardware setup that makes its own
`stats().framesArrived` check meaningful rather than moot: the UltraStudio
Monitor 3G's own SDI output, physically patched into the Recorder 3G's own
SDI input. Both devices are already this project's real target hardware
(ADR-037) sitting on the same desk, so this needs no third piece of
equipment, only a cable between two units already in hand — a real-hardware
precondition this test's own `Accept:` states explicitly, the same way
WU-15a's own `Accept:` states "a broadcast monitor" as a precondition of its
own by-eye clause rather than leaving it implicit. Without that loopback
connected, `CaptureSource::create()`/`stop()` still need to run cleanly (the
mechanics this test's automated `CHECK`s gate on: create-succeeds,
zero-frames-arrived does not by itself indicate a defect, and the accounting
invariant `framesPushed + ring.droppedCount() <= framesArrived` holds
unconditionally by construction, real signal or not) — the same division of
labour ADR-032/WU-15a already drew between what an automated check can
assert and what a human, or in this case a physical cable, has to supply.

**This entire unit is unverified by this session — deliberately, the same
discipline ADR-031/032/046 already established for every DeckLink-touching
unit before it, extended here to a third consecutive one.** No Blackmagic
SDK and no AppleClang/Xcode toolchain exist in the Linux cloud sandbox this
session drafted in — the same gap ADR-031 named for WU-14, ADR-032 named for
WU-15a, and ADR-046 already named for this unit's own sketch going in.
`decklink_input.hpp`/`.cpp` and `tests/test_decklink_input.cpp` are reasoned
through against the real SDK headers and the three real capture samples this
session re-read (this entry's own citations above, extending ADR-046's own)
rather than compiled or run by this session at all. `WORK-UNITS.md`'s own
WU-20b line stays `wip`, not `green`, until built and run at the real
terminal, including the loopback setup this entry's own "Real-hardware test
design" section names as a precondition of its own `Accept:` — not merely
`cmake --build` succeeding.

Not decided here, deliberately, and named for whoever picks up WU-21 ("Full
loop through at 576i25"): the consumer side of the ring — draining
`CaptureFrameRing` into `runFrame()`/`runFrameFile()`, and reading pixel bytes
out of a retained `IDeckLinkVideoInputFrame` for the first time anywhere in
this project (the `IDeckLinkVideoBuffer`/`StartAccess`/`EndAccess` pattern
ADR-032 already established for output, extended to input the same way
ADR-046 already extended the `GetBytes()`-is-not-on-the-frame finding) — is
that unit's own job, not this one's; genlock/clock-domain drift between the
Recorder 3G's own capture clock and the Monitor 3G's own output clock
(ADR-037's own second follow-up, still open); and whether
`kCaptureRingCapacity`'s chosen value of 8 actually matches real observed
buffering depth once WU-21 gives this project its first real consumer to
measure against.

Does not reopen `docs/architecture.md`, ADR-005, ADR-024, ADR-031, ADR-032,
ADR-037, ADR-039 or ADR-046 — same relationship every ADR since ADR-020 has
to the document; ADR-005's v210-only I/O scope and ADR-039's own device
naming are read as fixed inputs, not revisited; ADR-024's "do not fabricate a
destination the warp never produced" reasoning is extended to a second,
earlier pipeline stage (a whole frame, not a single fragment), not altered;
ADR-031's `ComPtr` (its existing borrowing constructor, exercised here for a
second genuinely-borrowed callback parameter) and ADR-032's `LoopedFramePlayback`
shape (real `IUnknown` refcounting, `ComPtr::adopt()` on `create()`, fail-clean
on any startup error) are both reused directly, not modified; and ADR-046's own
WU-20b sketch is completed, not amended — every question that entry explicitly
left open for this unit is decided above, and nothing this entry decides
contradicts what that entry already froze (the real `IDeckLinkInput`/
`IDeckLinkInputCallback` shape, the WU-20a/WU-20b split itself).

**ADR-048 — WU-21 split into WU-21a (`runFrameBytes()`, the in-memory
sibling of `runFrame()`/`runFrameFile()` — this session, genuinely built and
run in the Linux cloud sandbox across the full portable-unit matrix) and
WU-21b (the DeckLink-specific capture consumer that reads real pixel bytes
out of a retained `IDeckLinkVideoInputFrame` and calls WU-21a's own new
entry point — not this session, reasoned-through scope sketch only, same
shape as WU-14/WU-15a/WU-20b); confirms `wu-20a-green`/`wu-20b-green` both
actually exist, correcting `HANDOFF.md`'s own going-in uncertainty about
them. Frozen at WU-21a, Phase 5's third unit.**

**Tag state, confirmed before anything else, per this session's own brief.**
`HANDOFF.md` going into this session flagged both `wu-20a-green` and
`wu-20b-green` as session 26's own account of them, not itself re-confirmed
by anything run that session. `git tag -l` and `git describe` against the
real repository (via the device bridge, not this project's own Linux cloud
sandbox, which has no access to it), run directly rather than trusted from
that account, show both tags genuinely exist, `HEAD` sits at `wu-20b-green`
exactly, and the working tree is clean — `524e48a`, the `WU-20b`
verified-at-the-real-terminal commit `HANDOFF.md` itself names. Both units
are therefore already `green`, not `wip` as `WORK-UNITS.md` still read going
into this session; `WORK-UNITS.md`'s own WU-20a/WU-20b lines are corrected
to say so (see `HANDOFF.md` for this session's own account, and the "What to
run at your terminal" note this session leaves for the actually-outstanding
items — none, for WU-20a/WU-20b specifically).

**WU-21's own first job, per its own line in `WORK-UNITS.md` and per this
project's established discipline for a new hardware surface (ADR-031's own
reading-before-scoping precedent): re-read the real SDK's
`IDeckLinkVideoInputFrame`/`IDeckLinkVideoBuffer`/`StartAccess`/`EndAccess`
shape directly, not just ADR-046/047's own summary of it, before scoping
anything.** Confirmed against `Mac/include/DeckLinkAPI.h` directly, this
session: `IDeckLinkVideoBuffer` (`GetBytes`, `GetSize`, `StartAccess`,
`EndAccess`) is unchanged from ADR-032's own account of the output side —
the same interface, `IID_IDeckLinkVideoBuffer`, serves both directions, as
ADR-046 already found and this session's own re-read confirms rather than
merely repeats. `IDeckLinkVideoFrame` (`GetWidth`, `GetHeight`,
`GetRowBytes`, `GetPixelFormat`, `GetFlags`, ...) and `IDeckLinkVideoInputFrame`
(adding only `GetStreamTime`/`GetHardwareReferenceTimestamp`) both read
exactly as ADR-046 already recorded — `GetRowBytes()`, not
architecture.md 7's own misnamed `GetBytesPerRow()`; no `GetBytes()` of
their own. `bmdBufferAccessRead`/`bmdBufferAccessWrite` are two independent
bits of the same `BMDBufferAccessFlags` bitmask (`DeckLinkAPI.h` line ~140),
confirming `io/decklink_output.cpp`'s own `fillFrameBuffer()`
(`bmdBufferAccessWrite`) and this unit's own read side
(`bmdBufferAccessRead`) are the same bracket, opposite direction, not two
different mechanisms. Also checked directly, not assumed: none of the three
real capture samples this project has already surveyed (`CaptureStills`,
`InputLoopThrough`, `CapturePreview`) reads pixel bytes out of a retained
input frame via `IDeckLinkVideoBuffer` at all —
`grep -rl IID_IDeckLinkVideoBuffer` under `Mac/Samples` finds `SignalGenerator`
(output, already ADR-032's own citation), `KeyerOutput`, `ExportToTape`,
`ClosedCaptions`, `SignalGenHDR` and `PlaybackStills`, none of them a capture
sample; `CaptureStills` itself reads pixel bytes on the *input* side via a
different, macOS-specific interface (`IDeckLinkMacVideoBuffer`, a
`CVPixelBufferRef` bridge used only to write a TIFF still), not
`IDeckLinkVideoBuffer`. This confirms, by direct search rather than
inference from absence, `HANDOFF.md`'s own framing of this as "the first
time this project reads pixel bytes out of a retained
`IDeckLinkVideoInputFrame`" — genuinely new ground, not adapted from an
existing sample the way `ComPtr` (ADR-031) and the preroll/refill idiom
(ADR-032) were, the same kind of finding ADR-046 already made for the ring
buffer itself.

**The split decision, made after that reading, not before it — the same
order every prior split in this project used (WU-12a/b, WU-15a/b, WU-16a/b,
WU-19a/b, WU-20a/b).** `WORK-UNITS.md`'s own WU-21 line, going into this
session, named only its title and Phase-5 position, no `Files:`/`Accept:` at
all. Architecture.md 10's own Phase 5 "done when" line — "live SDI in,
warped, SDI out, one hour clean" — is the *whole* remaining phase, not one
work unit's honest scope: it requires, at minimum, (a) draining
`CaptureFrameRing` and reading a retained frame's own pixel bytes (SDK-
dependent, unverified in this sandbox by construction, ADR-031/032/046/047's
own established shape); (b) converting those bytes into something this
project's own warp pipeline can consume, warping them, and packing the
result back to v210 bytes (portable, with no DeckLink dependency of its
own, once (a) has handed over a raw byte pointer and stride — the same
"zero DeckLink dependency, genuinely testable here" distinction ADR-046
already drew between the ring buffer and the capture object it feeds); and
(c) scheduling the result continuously back out over SDI on the Monitor 3G
— a materially different output mechanism from `LoopedFramePlayback`'s own
WU-15a design, which loops one static, precomputed frame forever and has no
facility to accept a new one each cycle. Combining (a) and (b) alone would
already exceed `SESSION-PROTOCOL.md`'s own three-source-file cap (two new
`src/io/` files for (a), two touched `src/core/` files for (b)); combining
all three is not "one session, one work unit" by any reading. Chosen, the
same "portable piece now, genuinely verified; SDK-dependent piece next,
reasoned through only" shape ADR-046 already used for WU-20a/WU-20b:

- **WU-21a (this session).** Piece (b) alone, built as `runFrameBytes()` —
  see below.
- **WU-21b (not this session, not yet scheduled in `WORK-UNITS.md`'s own
  numbering beyond this note).** Piece (a): a new `CaptureConsumer` (working
  name; `src/io/`, two new files plus a test, the same shape WU-20b used)
  that drains a `CaptureFrameRing` on its own consumer thread, obtains each
  retained frame's own raw packed-v210 bytes and row-byte stride via
  `IDeckLinkVideoBuffer::StartAccess(bmdBufferAccessRead)`/`GetBytes`/
  `EndAccess` (this entry's own SDK re-read above; mirrors
  `io/decklink_output.cpp`'s own `fillFrameBuffer()`, read direction), and
  calls WU-21a's own `runFrameBytes()` on the bytes while still inside that
  `StartAccess`/`EndAccess` bracket — a captured frame's buffer contents are
  only guaranteed valid between those two calls, so the unpack must happen
  before `EndAccess`, not after, the same discipline `fillFrameBuffer()`
  already established for the write direction. Produces warped v210 bytes
  in memory; does **not** reschedule them onto `IDeckLinkOutput` — piece (c)
  above is a further, not-yet-named unit's own job, since it needs a new
  output-scheduling mechanism `LoopedFramePlayback` does not provide.
  Reasoned-through-only when it is built, the same shape ADR-031/032/046/047
  already used for every DeckLink-touching unit before it — this sandbox
  has no Blackmagic SDK and no AppleClang/Xcode toolchain, unchanged.
- **Piece (c)** (real, continuous SDI-out scheduling of a live-produced
  frame stream) and the eventual one-hour endurance run architecture.md 10's
  own "done when" line names (the same category of Steve's-own-hands-on-job
  WU-15b/ADR-032 and WU-19b/ADR-044 already used for a comparable
  unattended-hardware criterion) are named here but not scoped — a future
  session's own job, once WU-21b exists to build against.

**`runFrameBytes()`: the in-memory sibling of `runFrame()`/`runFrameFile()`,
`core/resolve.hpp`/`core/pipeline.cpp` (both extended, not new).** Same
signal path as `runFrameFile()` (v210 unpack, chroma upsample, `runFrame()`,
chroma downsample, v210 pack) with the file-I/O bookends replaced by a
caller-supplied source byte pointer/stride and a caller-supplied,
already-sized destination byte pointer/stride — exactly the shape WU-21b's
own `CaptureConsumer` needs, since a captured frame's own pixel bytes live
in a DeckLink-owned buffer for the duration of one `StartAccess`/`EndAccess`
bracket, never a file. No new header: `core/resolve.hpp` already declares
`runFrame()` and `runFrameFile()` side by side (ADR-026); a third sibling
declared next to them is the same "extend an existing, closely related
header" judgement ADR-021/026/030/038/044 already made repeatedly, not a new
one. No `bool` return the way `runFrameFile()` has: unlike that function,
there is no file open/read/write step that can fail here — `runFrame()`
itself has no failure return, by construction, and neither do
`v210::unpackImage`/`packImage` or `chroma::upsample`/`downsampleImage`.

**Deliberately not implemented by making `runFrameFile()` call it.** The two
functions run the identical middle sequence (unpack, upsample, `runFrame()`,
downsample, pack); `runFrameFile()`'s own body is left completely untouched
rather than refactored to share it. Weighed directly, per this session's own
brief: the risk of a refactor's own regression against a five-unit-old,
already-frozen (WU-10), zero-margin-for-error function this session's own
accept criterion does not need to touch, against the benefit of not
duplicating roughly fifteen lines of straight-line calls to five already
independently-tested functions (`v210::unpackImage`/`packImage`,
`chroma::upsample`/`downsampleImage`, `runFrame()` itself) — genuinely
different from ADR-040/041's own `resolveOneTile()` extraction, where two
hand-written copies of *evolving, multi-worker orchestration logic* really
could quietly drift apart under a later change to either path. Nothing here
is hand-rolled twice; the same five already-tested functions are called in
the same order twice, which is a much smaller and more inspectable kind of
duplication. Flagged, not fixed speculatively — this project's own
"not decided here" convention (e.g. ADR-042/043's own deferred denser NEON
schemes) applied to a much smaller question — as a candidate for
consolidation if a third caller ever needs the identical sequence.

**Test: `tests/test_pipeline_bytes.cpp`, new.** Two checks, both genuinely
run in this session's own Linux cloud sandbox (no DeckLink or platform
dependency anywhere in `runFrameBytes()` itself, so — unlike WU-14/15a/20b —
nothing here needs the real terminal to verify at all):

- `runFrameBytes()` and `runFrameFile()` produce byte-identical output for
  the same lattice/source/params, checked exactly against a genuine,
  off-centre 0.7x-compression affine warp over a zone plate (not an
  identity map — real fragment generation, splat and resolve, matching this
  project's own "exercise the real thing, not the degenerate case"
  preference wherever a differential check is cheap enough to do so).
- `runFrameBytes()` itself satisfies I7 (identity map round-trips
  bit-exactly) directly, against a *flat-chroma* pattern (zone plate, not a
  ramp). A first draft of this check used `tools/testpat.hpp`'s own
  `makeRamp()` and failed immediately — caught and fixed within the same
  editing pass, before any claim was made based on the broken draft, the
  same "routine iteration, not a design/reasoning error" distinction
  `HANDOFF.md`/ADR-043 already drew for WU-17/18's own most-vexing-parse
  mistakes and C-015 drew for a supersampling-signature assumption, not a
  `CORRECTIONS.md` entry for the identical reason: CORRECTIONS.md C-006
  already establishes, from WU-05's own session, that a *non-flat* chroma
  signal (a ramp's own Cb/Cr planes are ramps too, per `tools/testpat.hpp`)
  does not survive `chroma::upsampleImage()` followed by
  `downsampleImage()` unchanged, regardless of the lattice, including the
  identity map exercised here — this session's own first draft simply
  picked the wrong one of `tests/test_zoneplate.cpp`'s own two already-
  documented pattern categories (`chromaExpectedExact` true only for flat
  chroma) rather than discovering anything new about the chroma path
  itself. Fixed by switching to `makeZonePlate()`, whose chroma planes are
  already held flat (`kChromaZero`) for exactly this reason, per that
  function's own file header.

**Verification.** Built and tested in this session's own Linux cloud
sandbox (Ubuntu 24.04, GCC 13.3.0, Clang 18.1.3, CMake 3.28.3, Ninja) —
genuinely, not reasoned through, the whole existing project (all nineteen
tests, not `test_pipeline_bytes` in isolation) rebuilt and re-run at every
point below: GCC and Clang, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5
(eight configurations, all nineteen tests green — the eighteen carried over
unchanged plus `test_pipeline_bytes`, zero warnings under this project's
full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror` set),
plus GCC 13 with `-fsanitize=address,undefined -fno-sanitize-recover=all` at
both tile sizes (clean, no ASan/UBSan report). No ThreadSanitizer run: unlike
WU-20a, this unit adds no concurrency of any kind — `runFrameBytes()` is a
plain, single-threaded sequence of calls (threading, when `PipelineParams`
asks for it, is `runFrame()`'s own concern, unchanged and untouched by this
unit). `WORK-UNITS.md`'s own WU-21a line stays `wip`, not `green`, for the
same procedural reason every other unit's line has
(`SESSION-PROTOCOL.md`, "the assistant does not run `close.sh`") — Steve's
own real-terminal `cmake --build` + `ctest` + `./tools/close.sh 21a` run is
still outstanding, even though nothing about this unit's own content is
expected to behave differently there than in the sandbox above (no
Apple-only or DeckLink-only surface anywhere in it).

**Not decided here, deliberately, and named for whoever picks up WU-21b:**
`CaptureConsumer`'s own exact name, signature and file layout (sketched
above, not frozen — the same "sketch now, real scoping when that session
starts" discipline ADR-046 already used for WU-20b); the ring's own drain
policy while empty (polling with a short sleep, versus something WU-20a's
own `RingBuffer` does not currently offer to block on — not decided here);
whether a captured frame's own reported `GetWidth()`/`GetHeight()`/
`GetRowBytes()` should be trusted directly per-frame or checked once against
the display mode `CaptureSource::create()` was given (a mismatch after a
format-change restart is exactly the kind of thing `VideoInputFormatChanged()`
already exists to signal, per ADR-046/047, so there may be nothing new to
check here — flagged, not resolved); and piece (c), continuous SDI-out
scheduling of a live-produced frame stream, and the eventual one-hour
endurance run, both above.

Does not reopen `docs/architecture.md`, ADR-010, ADR-021, ADR-026, ADR-031,
ADR-032, ADR-038, ADR-040, ADR-041, ADR-046 or ADR-047 — same relationship
every ADR since ADR-020 has to the document; ADR-026's own `core/resolve.hpp`
layout (declaring `runFrame()`/`runFrameFile()` side by side rather than in a
new `pipeline.hpp`) is extended to a third sibling, not amended; ADR-032's
`IDeckLinkVideoBuffer`/`StartAccess`/`EndAccess` finding is confirmed by a
fresh, direct re-read (not merely cited) and named as the interface WU-21b
will extend to input, the same way ADR-046 already extended it in words
without yet building the code that uses it; ADR-046/047's own WU-20a/WU-20b
split precedent is reused for a second, structurally similar split, not
revisited; and ADR-010's free-running genlock scope, ADR-037's own still-open
second follow-up (capture/output clock-domain drift) are both untouched --
`runFrameBytes()` processes one already-arrived frame's worth of bytes and
has no timing or clock-domain behaviour of its own to reason about, the same
"naming a device does not require reasoning about its clock domain" logic
ADR-039 already used for a different piece of this same phase.

**ADR-049 — WU-21b: DeckLink capture-side pixel read (`CaptureConsumer`,
`src/io/decklink_capture_consumer.hpp`/`.cpp`). Reasoned-through scope only,
same shape as WU-14/WU-15a/WU-20b/WU-21a's own DeckLink-touching
predecessors before it -- drafted and written via the device bridge with no
Blackmagic SDK and no AppleClang/Xcode toolchain available in this session's
own Linux cloud sandbox to check it against. UNVERIFIED as of this session's
own close; needs building and running at the real terminal, with the Monitor
3G → Recorder 3G SDI loopback connected (the same physical setup
`test_decklink_input.cpp`'s own header comment already documents), before
this unit can go `green`.**

**Tag state and delivery-pair state, confirmed before anything else, per
this session's own brief.** `git tag`/`git log`/`git status` run directly
against the real repository via the device bridge (not trusted from
`HANDOFF.md`'s own account) confirm `wu-21a-green` genuinely exists, `HEAD`
sits at `0d2b004` ("WU-21a: mark green, tagged wu-21a-green (session 27
close)"), and that commit already includes `WORK-UNITS.md` and `HANDOFF.md`
-- the pair session 27's own `HANDOFF.md` flagged as still-uncommitted going
into this session was in fact committed before this session started,
resolving that uncertainty rather than confirming it. The working tree was
otherwise clean before this session's own writes. One stale
`.git/index.lock` (0 bytes) was found sitting in the working tree,
left behind by an earlier full-form `git status` run via the device
bridge in this same session -- `device_bash` cannot unlink files on a
mounted folder, so git's own attempt to write-then-remove an
index-refresh lock left the empty lock file in place. Read-only git
commands (`status`, `log`, `tag`, `show`) are confirmed unaffected by its
presence, but it should be removed (`rm ~/src/scatter-dve/.git/index.lock`,
after confirming no real git process is running) before Steve's own next
`git commit` or `close.sh` run, in case those are less tolerant of it. See
`HANDOFF.md` for the same note.

**Reading, before scoping, per this project's own established discipline for
a new hardware surface (ADR-031's own precedent, reused by
ADR-046/047/048).** Re-read the real SDK's own `DeckLinkAPI.h` directly this
session (not merely ADR-048's own summary of it): `IDeckLinkVideoBuffer`
(`GetBytes`, `GetSize`, `StartAccess`, `EndAccess`) is confirmed unchanged
from ADR-032/048's own account of it and is the same interface, obtained via
`QueryInterface`, this unit now reads through for the first time rather than
merely names; `IDeckLinkVideoFrame` (`GetWidth`, `GetHeight`, `GetRowBytes`,
`GetPixelFormat`, `GetFlags`, `GetTimecode`, `GetAncillaryData` -- the first
three all returning `long`) and `IDeckLinkVideoInputFrame : public
IDeckLinkVideoFrame` (adding only `GetStreamTime`/`GetHardwareReferenceTimestamp`)
both read exactly as ADR-046/048 already recorded. Also reread
`src/io/decklink_output.hpp`/`.cpp` (`LoopedFramePlayback::fillFrameBuffer()`,
the write-direction pattern this unit mirrors), `src/io/decklink_input.hpp`/
`.cpp` (WU-20b, the `CaptureSource`/`CaptureFrameRing` this unit drains),
`src/core/ring_buffer.hpp` (WU-20a, confirming `tryPop()` has no blocking
form), `src/core/resolve.hpp`'s `runFrameBytes()` declaration and
`PipelineParams` (WU-21a), and `docs/architecture.md` sections 3, 6, 7, 9, 12
plus ADR-037/039 -- real target hardware unchanged (UltraStudio Recorder 3G
for input, UltraStudio Monitor 3G for output, UltraStudio 4K Mini still on
hold).

**The four questions ADR-048 explicitly left open for this unit, decided
now** (the same reasoning is duplicated, word for word in substance, in
`src/io/decklink_capture_consumer.hpp`'s own header comment). Consumer-thread
ownership and lifetime are fully independent of `CaptureSource`'s own: the
two objects share only the `CaptureFrameRing` between them, the same "caller
owns the ring, both sides just reference it" shape WU-20a already
established for the producer side, so starting or stopping a
`CaptureConsumer` neither starts nor stops the `CaptureSource` feeding it,
and a caller wanting a clean shutdown stops both itself, in whichever order
it prefers -- a frame still sitting in the ring when this consumer stops is
simply abandoned, its `ComPtr` releasing the retained reference normally,
the same "no leak either way" property WU-20b's own
`VideoInputFrameArrived()` comment already relies on for a failed
`tryPush()`. Extracted bytes are handed to `runFrameBytes()` synchronously,
while still inside the `StartAccess`/`EndAccess` bracket, never copied into
a separate pool buffer first -- a captured frame's own buffer contents are
only guaranteed valid between those two calls (the same requirement
`fillFrameBuffer()` already honours on the write side), `runFrameBytes()`
itself is synchronous and retains nothing past its own call, so calling it
directly against the mapped pointer is both correct and needs no pool
buffer this unit would otherwise have to invent and size. The ring's own
drain policy while empty is a short (1ms) poll sleep: `RingBuffer` (WU-20a,
frozen) offers no blocking pop, `tryPop()`'s own "never blocks" contract
exists for the producer/callback thread's sake (architecture.md 6), and this
consumer thread is not that thread and is free to wait, so a short sleep
avoids spinning a full core without needing a new blocking-pop capability on
an already-frozen class. A captured frame's own reported
`GetWidth()`/`GetHeight()`/`GetRowBytes()` are trusted directly, per frame,
never checked against the display mode `CaptureSource::create()` was given
-- the same "ask the SDK, do not assume this project's own computation
agrees with it" rule architecture.md 7 already states for row bytes,
extended here to width/height for the same reason, and a genuinely
different geometry after a format-change restart (`VideoInputFormatChanged()`)
is exactly the kind of thing a new frame could legitimately report, so
trusting it directly needs no separate consistency check to stay correct;
destination geometry is fixed once, at this consumer's own construction
(`PipelineParams::destWidth`/`destHeight`), the same "geometry fixed for the
object's own lifetime" shape `LoopedFramePlayback` already uses for its own
`m_width`/`m_height` on the output side.

**Design.** `src/io/decklink_capture_consumer.hpp`/`.cpp` (new):
`CaptureConsumerStats` (three `std::atomic<int>` counters --
`framesPopped`, `framesProcessed`, `framesFailed` -- same relaxed-read
convention every other stats struct in this project already uses) and
`CaptureConsumer`, constructed from a caller-owned `CaptureFrameRing&`, a
`Lattice` (copied in, not referenced -- the consumer thread reads it on
every popped frame for as long as it runs), and a `PipelineParams` (copied
in, fixing destination geometry for the object's own lifetime). `start()`/
`stop()` use the same `compare_exchange_strong` idiom
`CaptureSource::stop()`/`LoopedFramePlayback::stop()` already use, safe to
call at most once each; the destructor calls `stop()`, safe even if
`start()` was never called. `run()` is the consumer thread body: while not
stopping, `tryPop()` the ring, sleep 1ms and continue if empty, otherwise
count the pop and call `processOne()`, counting the result as processed or
failed, never both, never neither. `processOne()` checks
`GetPixelFormat() == bmdFormat10BitYUV` defensively (`CaptureSource` only
ever requests that format, per `decklink_input.hpp`'s own class comment, so
this is "ask rather than assume" caution, not an expectation of ever seeing
anything else), reads width/height/row-bytes directly off the frame with
explicit `int`/`std::ptrdiff_t` casts (the SDK returns `long` for all three;
this project's own `-Wconversion -Wsign-conversion -Werror` build requires
the explicit cast, the same convention used throughout this codebase),
obtains `IDeckLinkVideoBuffer` via `ComPtr`'s `QueryInterface`-converting
constructor, brackets `GetBytes()` with `StartAccess(bmdBufferAccessRead)`/
`EndAccess(bmdBufferAccessRead)`, calls `scatter::runFrameBytes()` against
the mapped pointer while still inside that bracket, and -- only on full
success -- moves the resulting destination bytes into `m_latestFrame` under
`m_mutex`. `copyLatestFrame()` copies the most recently successfully
produced frame's own bytes out under the same lock, returning `false` and
leaving `out` unchanged if nothing has been produced yet -- the mechanism
this unit's own test and any future WU-21c caller alike use to retrieve
output; this unit does not itself reschedule those bytes onto
`IDeckLinkOutput` (WU-21c's own job, a materially different mechanism from
`LoopedFramePlayback`'s own "loop one static frame forever" design, not
built here).

`tests/test_decklink_capture_consumer.cpp` (new): mirrors
`test_decklink_input.cpp`'s own style exactly -- device enumeration, a
format-detection-capable input, `CaptureSource::create()` at `bmdModePAL`
(720x576, this project's own confirmed-working development standard), a
`CaptureConsumer` built against a locally-duplicated identity lattice
(`SESSION-PROTOCOL.md` rule 2: one unit, one test, no shared fixture across
test translation units) matching that geometry, a bounded 5-second run (the
same order of magnitude `test_decklink_input.cpp`'s own bounded smoke test
already uses), then `stop()` on both objects and two accounting-invariant
checks that hold unconditionally regardless of whether a real signal is
present (`framesProcessed + framesFailed == framesPopped`; `framesPopped <=`
capture's own `framesPushed`). If `framesProcessed` is zero -- the loopback
not connected -- the test prints a NOTE and returns rather than failing, the
same "nothing plugged in right now is a real, honestly reportable state, not
a defect" convention `test_decklink_input.cpp` already uses; only when
frames were actually processed does it additionally check
`copyLatestFrame()` returns a buffer of the expected size
(`v210::rowBytesMin(kWidth) * kHeight`). Deliberately reuses the identity
map, not a genuine warp -- this test's own job is proving the
`StartAccess`/`GetBytes`/`runFrameBytes`/`EndAccess` mechanics work against
real captured bytes, not re-proving `runFrameBytes()`'s own warp
correctness, already genuinely verified in the cloud sandbox at WU-21a.

`CMakeLists.txt`: `src/io/decklink_capture_consumer.cpp` added to the
existing `scatter-decklink` library's source list; a new
`target_link_libraries(scatter-decklink PRIVATE scatter-core)` added, since
this is the first `scatter-decklink` file ever to call into `scatter-core`
(`runFrameBytes()`, `core/resolve.hpp`) rather than only the DeckLink SDK --
`PRIVATE`, because `scatter-core` is an implementation detail of this one
new file, not part of `scatter-decklink`'s own existing public surface
(`decklink_device.hpp`/`decklink_output.hpp`/`decklink_input.hpp` include no
`core/` header). A new `test_decklink_capture_consumer` executable added
alongside `test_decklink_device`/`test_decklink_output`/`test_decklink_input`,
linking both `scatter-decklink` and `scatter-core` explicitly (the same
dual-link pattern `test_decklink_output` already uses, needed here for the
same reason: the test itself calls `core/lattice.hpp` and `core/resolve.hpp`
symbols directly, not merely through `scatter-decklink`), registered via
`add_test`. Caught and fixed within this same session, before any claim was
made: the first draft of this `CMakeLists.txt` edit added the library source
and the new `target_link_libraries` line but omitted the new test's own
`add_executable`/`target_link_libraries`/`target_include_directories`/
`add_test` block entirely -- found by rereading the committed file straight
back (`SESSION-PROTOCOL.md` anti-drift rule 8's own re-read-to-confirm step)
and comparing it against the neighbouring `test_decklink_input` block, not
by any build (this sandbox cannot build DeckLink code); fixed and
re-delivered before this session's own close.

**Not decided here, deliberately, and named for whoever picks up WU-21c or
otherwise touches this next.** `kCaptureRingCapacity`'s own chosen value of
8 (WU-20a/20b, ADR-046) is still not measured against real observed
buffering depth -- this unit's own bounded 5-second test reports
`framesPopped`/`framesPushed`/`framesFailed` precisely so that comparison
becomes possible at Steve's own real terminal, but this session does not run
that test and so does not make the comparison itself. Genlock/clock-domain
drift between the Recorder 3G's own capture clock and the Monitor 3G's own
output clock (ADR-037's own second follow-up, named again at ADR-047/048)
remains open -- `CaptureConsumer` processes one already-arrived frame's
worth of bytes at a time and has no timing or clock-domain behaviour of its
own to reason about, the same "naming a device does not require reasoning
about its clock domain" logic ADR-039 already used for a different piece of
this phase; WU-21c's own continuous SDI re-output is where that question
actually has to be answered, not this unit's. `test_decklink_capture_consumer`'s
own accounting checks and, if the loopback is connected, the frame-size
check, are UNVERIFIED against real hardware as of this session's own close
-- the whole point of this being a DeckLink-only unit reasoned through in a
Linux cloud sandbox with neither the SDK nor an AppleClang/Xcode toolchain
available.

Does not reopen `docs/architecture.md`, ADR-026, ADR-031, ADR-032, ADR-037,
ADR-039, ADR-046, ADR-047 or ADR-048 -- same relationship every ADR since
ADR-020 has to the document; ADR-032's `IDeckLinkVideoBuffer`/`StartAccess`/
`EndAccess` finding is extended to a second direction, not amended;
ADR-046's `RingBuffer` (no blocking pop) is used exactly as frozen, not
modified; ADR-048's own WU-21a/b/c split and its four named open questions
are completed for the `b` piece, not revisited -- every question that entry
left open for this unit is decided above, and nothing here contradicts what
ADR-048 already froze (the `runFrameBytes()` signature and `PipelineParams`
fields WU-21a built, in particular, are used exactly as WU-21a left them).
ADR-037's own genlock follow-up and WU-20b's own `stopFromCallback()` safety
question (ADR-047) are both still open, named again above, not answered
here.

**Real-hardware verification, same session, run at Steve's own real
terminal after this entry was first written.** `cmake --build build` clean
-- the SDK found at `/Users/stephenneal/src/Blackmagic DeckLink SDK 16.0`,
`scatter-decklink` and every DeckLink-gated target (including the new
`test_decklink_capture_consumer`) configured and built. `ctest --test-dir
build --output-on-failure`: 24 of 25 tests passing, the sole failure the
already-accepted `test_decklink_device`/`foundDuplexDevice` exception
(ADR-035), unrelated to this unit. `./build/test_decklink_capture_consumer`
run directly, with the Monitor 3G -> Recorder 3G SDI loopback connected,
over its own bounded 5-second window: `framesArrived=123 framesPushed=89 |
framesPopped=81 framesProcessed=81 framesFailed=0`, all 7 automated checks
passing, including the `copyLatestFrame()` size check (only reached when
`framesProcessed > 0`) -- the first genuine, real-signal confirmation
anywhere in this project that a captured frame's own pixel bytes can be read
through `IDeckLinkVideoBuffer`'s `StartAccess`/`GetBytes`/`EndAccess`
bracket and fed into `runFrameBytes()` end to end, not merely reasoned
through. `framesProcessed(81) + framesFailed(0) == framesPopped(81)` and
`framesPopped(81) <= framesPushed(89)` both hold exactly, as this unit's own
`Accept:` requires.

The `framesArrived`/`framesPushed` gap (123 vs. 89, roughly 28% of arriving
callbacks never reaching the ring) is the first real data this project has
had for the `kCaptureRingCapacity=8` open question (ADR-046, named again at
ADR-047/048/049): a nontrivial share of arriving frames were not queued at
all during this run. `framesPushed(89) - framesPopped(81) == 8`, exactly the
ring's own capacity, is consistent with the ordinary "up to one ring's worth
left unconsumed when the bounded run stops" behaviour rather than a growing
backlog, so the gap reads more like ring-full drops concentrated somewhere
during the run (most plausibly at start, before the consumer thread has
spun up and begun draining) than the consumer falling permanently behind.
Not conclusively diagnosed here -- this session has no per-frame timing
instrumentation to separate a startup burst from sustained backpressure --
named as a real, now-measured data point for whoever next revisits
`kCaptureRingCapacity`, not resolved.

Confirmed `green`, tagged `wu-21b-green` (confirmed present on `git tag` at
Steve's own real terminal, alongside every earlier tag through
`wu-21a-green`). This addendum does not reopen anything named in this
entry's own "Does not reopen" paragraph above -- it records a real-terminal
result for a unit that paragraph already described as reasoned-through-only
at the time it was written, nothing more.

**ADR-050 -- WU-21c: continuous SDI re-output (`LiveFramePlayback`,
`src/io/decklink_live_output.hpp`/`.cpp`), scheduling a live-produced frame
stream (WU-21b's own `CaptureConsumer::copyLatestFrame()`) onto
`IDeckLinkOutput`. Closes architecture.md 10's own Phase 5 "done when" line
("live SDI in, warped, SDI out") end to end for the first time. A fixed pool
of frame buffers, round-robin refilled and rescheduled exactly once per
completion, replaces `LoopedFramePlayback`'s own single-static-buffer design;
the pool-sizing and no-explicit-sync genlock/clock-domain decisions are both
frozen here. Reasoned-through scope only, same shape as every DeckLink-
touching unit before it -- drafted and written via the device bridge with no
Blackmagic SDK and no AppleClang/Xcode toolchain available in this session's
own Linux cloud sandbox to check it against. UNVERIFIED as of this session's
own close; needs building and running at the real terminal, with the same
Monitor 3G <-> Recorder 3G SDI loopback WU-20b/WU-21b's own tests already
document, before this unit can go `green`.**

**Tag state, confirmed before anything else, per this session's own brief.**
`git tag`/`git log --oneline -8`/`git status --short`, run directly against
the real repository via the device bridge rather than trusted from
`HANDOFF.md`'s own account: `wu-21b-green` confirmed present, alongside every
earlier tag through it; `HEAD` at `bb97fa1` ("WU-21b: record real-hardware
verification and wu-21b-green tag; note Weston 3-field preference for future
WU-23"), which already carries the WU-21b real-hardware-verification update
to `WORK-UNITS.md` -- the commit `HANDOFF.md` going into this session flagged
as outstanding for Steve to run was in fact already made before this session
began, resolving that uncertainty rather than repeating it. Working tree
otherwise clean before this session's own writes.

**Reading, before scoping, per this project's own established discipline for
a new hardware surface (ADR-031's own precedent, reused by
ADR-046/047/048/049).** Re-read the real SDK's own `DeckLinkAPI.h` directly
this session: `IDeckLinkOutput` (`DoesSupportVideoMode`, `EnableVideoOutput`,
`CreateVideoFrame`, `RowBytesForPixelFormat`, `ScheduleVideoFrame`,
`SetScheduledFrameCompletionCallback`, `GetBufferedVideoFrameCount`,
`StartScheduledPlayback`/`StopScheduledPlayback`) is confirmed unchanged from
ADR-032/033's own account; `IDeckLinkVideoOutputCallback` has exactly the two
methods `io/decklink_output.hpp`'s own `LoopedFramePlayback` already
implements, `ScheduledFrameCompleted`/`ScheduledPlaybackHasStopped`, nothing
new. Also reread the real SDK's own `FilePlayback` sample
(`DeckLinkPlaybackDevice.cpp`'s `ScheduledFrameCompleted()`/`scheduleVideo()`)
specifically for how it handles genuinely *changing* content across
completions, not just cited from ADR-032's own earlier account of its preroll
figure: confirmed directly that `scheduleVideo()` obtains a fresh
`com_ptr<IDeckLinkVideoFrame>` from its own media reader on every call and
schedules that new object, never rescheduling a previously-completed frame
pointer unchanged -- the real SDK's own precedent for exactly this unit's own
"content changes every tick" problem, distinct from `LoopedFramePlayback`'s
own "content never changes" case. Also reread
`src/io/decklink_output.hpp`/`.cpp` (`LoopedFramePlayback`, the preroll idiom
and `fillFrameBuffer()` pattern this unit extends from one buffer to a pool),
`src/io/decklink_capture_consumer.hpp`/`.cpp` (WU-21b, `copyLatestFrame()`,
this unit's own upstream source of frames), `src/io/decklink_input.hpp`/`.cpp`
(WU-20b, `CaptureSource` -- confirmed this unit touches it only indirectly,
through `CaptureConsumer`, never `IDeckLinkInput` itself), and
`docs/architecture.md` sections 3 (the signal path diagram, whose final "v210
pack -> SDI out" leg this unit closes for a live source for the first time),
6 (the threading model's own "Output scheduler thread. Driven by the frame
completion callback" -- this unit's own class realises that thread role
exactly as already named, now driving a pool instead of one buffer), 7
(Output subsection's own preroll/refill idiom, ADR-032's own citations,
extended here rather than re-read as new ground), 9 (the endurance test's own
pass criterion, "zero dropped or repeated frames beyond documented clock
drift" -- explicitly naming clock drift as an accepted, already-anticipated
phenomenon rather than a defect to engineer away, direct support for this
entry's own no-explicit-sync design below), and 12 (the reference-count-leak
risk, `ComPtr` used throughout unchanged) plus ADR-010 (free-running scope,
unchanged), ADR-032 (the mechanism this unit extends), ADR-037 (the
Monitor 3G/Recorder 3G device split, and its own second follow-up, genlock/
clock-domain drift, named again at ADR-047/048/049 and squarely this unit's
own concern for the first time -- see below) and ADR-039 (input device
naming, unaffected).

**Why this is a materially different mechanism from `LoopedFramePlayback`,
not an extension of it.** `LoopedFramePlayback` (WU-15a) reschedules the
exact same `IDeckLinkMutableVideoFrame` object every completion because its
own content is fixed at `create()` time and never changes -- one buffer is
correct and sufficient precisely because nothing ever needs to write into it
again after the first fill. This unit's whole job is the opposite: content
changes, once per output tick, from whatever `CaptureConsumer` most recently
produced. A buffer currently queued for scheduled playback cannot safely be
overwritten -- the driver may still be reading it -- so a single reused
buffer would either corrupt in-flight output (writing while it plays) or
force writes to wait for a completion that has not yet happened (serialising
refill against playback for no reason). The real fix, confirmed against the
SDK's own `FilePlayback` precedent above: more than one buffer, cycled so
that only a buffer whose own completion has *just* fired is ever written to.

**Pool sizing: exactly `round(frameRate / 2)` buffers -- ADR-032's own
half-second preroll convention, reused for pool size rather than invented
fresh.** Not a coincidence: the pool must hold exactly as many buffers as are
concurrently in flight (scheduled, not yet completed) at any instant in
steady state, and that number *is* the preroll depth -- this unit prerolls
by scheduling every pool buffer once at `create()` time (mirroring
`LoopedFramePlayback::startWith()`'s own preroll loop, generalised from N
schedules of one buffer to one schedule each of N buffers), so exactly
`poolSize` buffers are always in flight from that point on. `CreateVideoFrame()`
is called exactly `poolSize` times, once each, entirely within `startWith()`
-- setup-time allocation, never on the real-time completion-callback thread,
the same "allocate once, never in the hot path" discipline architecture.md 6
already states for the capture callback thread, applied here to the output
side's own setup step rather than its own steady-state refill (which touches
no new memory, only `memcpy`s into already-allocated buffers).

**Refill/reschedule policy: round-robin in lockstep with completion order,
relying on the SDK's own FIFO completion guarantee -- no separate
in-flight/free bookkeeping needed.** `ScheduledFrameCompleted()` is confirmed
(architecture.md 7, and by construction of `StartScheduledPlayback`'s own
documented behaviour) to fire in exactly the order frames were scheduled;
`LoopedFramePlayback`'s own single-buffer refill already relies on the same
property implicitly (it only ever has one buffer, so order does not matter
there). This unit cycles `m_nextPoolIndex` once per completion, in that same
order, so the buffer index refilled and rescheduled after any given
completion is always exactly the one whose own completion just fired -- safe
by construction, with no separate "is this buffer still queued" flag or
tracking structure needed. `refillAndSchedule()` is one shared function used
both for the initial preroll loop and every subsequent completion, the same
"one function, not two hand-written copies that could quietly drift apart"
preference ADR-029/040/041 already established.

**Genlock / clock-domain interoperation, decided for this unit specifically
-- ADR-037's own second follow-up, open since it was first named at ADR-037
and repeated unresolved at ADR-047/048/049, and this unit's own concern for
the first time: a real capture clock (the Recorder 3G's own arrival cadence,
however irregular) and a real output clock (the Monitor 3G's own fixed
`ScheduleVideoFrame`/`StartScheduledPlayback` cadence) now genuinely have to
interoperate, not just be named separately in a document.** Decided: no
explicit synchronisation of any kind. Every refill calls
`consumer.copyLatestFrame()` and uses whatever it returns, with no timestamp
comparison against the output's own schedule, no queue of not-yet-shown
produced frames, and no attempt to detect or correct drift. Two consequences,
both accepted rather than engineered around: a capture/process rate slower
than the output's own fixed cadence shows as the same bytes scheduled more
than once in a row (`framesRepeated()`, this unit's own new counter,
incremented whenever a refill finds nothing fresher than what is already
scheduled); a rate faster than the output's own cadence shows as some
produced frames never being sampled at all before a newer one silently
supersedes them in `CaptureConsumer`'s own single-slot "latest" buffer (WU-21b,
unchanged) -- no backlog ever accumulates either way, which is the point:
architecture.md 9's own endurance pass criterion already names "documented
clock drift" as an accepted, expected phenomenon for exactly this
free-running (ADR-010) scope, not a defect this unit is asked to eliminate,
and a policy that buffered a growing backlog to avoid ever repeating a frame
would trade that away for unbounded, worsening latency over a long run --
the opposite of what a live monitoring/re-output path wants. This is a
narrower, concrete decision about how two independently clocked producers
and consumers meet at one shared single-slot buffer, not a genlock
implementation -- ADR-010's free-running scope is unchanged, and true
clock-domain measurement (is the repeat/skip rate actually a problem in
practice, at what magnitude) is deliberately left to whoever next revisits
this, the same "named, not resolved" treatment `kCaptureRingCapacity`'s own
`framesArrived`/`framesPushed` gap got at WU-21b (ADR-049) -- `framesRepeated()`
exists specifically so a future session has a real number to look at, the way
WU-21b's own accounting gave one for the ring-capacity question.

**Row-bytes consistency, enforced at `create()` itself, not left to a
caller's own separate test.** WU-15a's own
`test_v210_rowbytes_matches_project_own_computation()` checks
`IDeckLinkOutput::RowBytesForPixelFormat()` against
`video::v210::rowBytesMin()` once, as a standalone test, because a mismatch
there would only ever affect that one static file's own single buffer.
Here, every pool buffer is refilled from `CaptureConsumer`'s own
`video::v210::rowBytesMin()`-sized bytes on an ongoing basis, so the same
mismatch would silently misalign every live frame for as long as this object
runs, not just fail one check once. `startWith()` therefore checks it
directly and fails `create()` outright on a mismatch, upgrading WU-15a's own
test-only check to a hard runtime precondition for this unit specifically --
does not change `decklink_output.cpp`'s own behaviour, which is unaltered.

**`fillFrameBuffer()` duplicated, not shared, between
`decklink_output.cpp` and `decklink_live_output.cpp`.** Same reasoning
ADR-048 already gave for not routing `runFrameFile()` through
`runFrameBytes()` internally: the function is a few lines of straight-line
SDK calls (`QueryInterface` for `IDeckLinkVideoBuffer`, `StartAccess`,
`GetBytes`, `memcpy`, `EndAccess`), calling five already-tested SDK entry
points in the same order in both files -- the risk of a shared-header
refactor's own regression against `decklink_output.cpp`'s own frozen,
already-hardware-verified (`wu-15a-green`) body outweighs the benefit of not
duplicating roughly a dozen lines. Flagged, not fixed speculatively, as a
candidate for consolidation into a shared `io/` helper if a third caller ever
needs the identical bracket -- the same "not decided here" treatment ADR-048
already used for its own analogous duplication.

**`PlaybackStats` reused directly from `io/decklink_output.hpp`, unmodified
-- ADR-029's own "reuse a tested type" precedent, not a near-duplicate
struct.** `completed`/`displayedLate`/`dropped`/`flushed` mean exactly the
same thing for this unit's own pool-cycling scheduler as they already do for
`LoopedFramePlayback`'s own single-buffer one -- both are counts of
`ScheduledFrameCompleted()`'s own `result` argument, nothing about the
refill policy changes what those four outcomes mean. `framesRepeated()` is
kept as this unit's own separate counter, not added as a fifth field to
`PlaybackStats` itself: it would be meaningless for `LoopedFramePlayback`
(definitionally true of every single completion there, since it always
reschedules the same buffer), and `SESSION-PROTOCOL.md` rule 2's "never
rename or refactor... names in headers are fixed" is read here, consistent
with ADR-026/030/040's own reading of the same rule, as permitting an
*addition* that fits the existing type's own meaning -- `framesRepeated`
does not.

**Files:** `src/io/decklink_live_output.hpp`, `src/io/decklink_live_output.cpp`
(both new: `LiveFramePlayback`), `tests/test_decklink_live_output.cpp` (new);
plus `CMakeLists.txt` (`decklink_live_output.cpp` added to the existing
`scatter-decklink` library's source list -- no new `target_link_libraries`
needed, since `scatter-decklink` already privately links `scatter-core` as of
WU-21b and `video::v210::rowBytesMin()` is this unit's only `scatter-core`
symbol; a new `test_decklink_live_output` executable registered alongside
the other three DeckLink tests, linking both `scatter-decklink` and
`scatter-core` for the same reason `test_decklink_capture_consumer` already
does -- CMakeLists.txt edits have never counted against the "3 source files"
cap in any earlier unit either).

**Not decided here, deliberately, and named for whoever picks up WU-21d or
otherwise touches this next.** The literal one-hour endurance run
architecture.md 10's own Phase 5 "done when" line names, and a by-eye
confirmation that the Monitor 3G's own SDI output genuinely shows live,
changing content rather than a frozen frame -- both Steve's own hands-on job
at the real terminal, the same category WU-15b (ADR-032) and WU-19b (ADR-044)
already used for a comparable unattended-hardware criterion this sandbox
cannot produce evidence about; `framesRepeated()`'s own real value against
real hardware, and whether the "always latest, no backlog" policy's
frame-repeat/frame-skip behaviour actually looks acceptable in practice, both
UNVERIFIED as of this session's own close; a possible future
timestamp-based alignment between capture arrival time and output schedule
time, if the "always latest" policy's own behaviour turns out to look bad in
practice -- not attempted here, not decided, named only as a candidate
refinement; and pool sizing tuned against measured real capture-to-output
latency rather than ADR-032's own half-second convention borrowed for it --
same "not decided here, no evidence yet" deferral `kCaptureRingCapacity` and
WU-19b's own throughput question already received.

Does not reopen `docs/architecture.md`, ADR-010, ADR-024, ADR-026, ADR-029,
ADR-031, ADR-032, ADR-037, ADR-039, ADR-046, ADR-047, ADR-048 or ADR-049 --
same relationship every ADR since ADR-020 has to the document; ADR-010's
free-running scope is unchanged, extended by a concrete no-explicit-sync
decision at one shared buffer, not superseded; ADR-032's `LoopedFramePlayback`
shape (real `IUnknown` refcounting, `ComPtr::adopt()` on `create()`, the
preroll idiom, fail-clean on any startup error) is reused directly for a
second class, not modified -- `LoopedFramePlayback` itself is untouched by
this entry; ADR-037's own genlock follow-up is addressed for this one unit's
own narrow case, not resolved in general, and its device-naming/two-device
split is read as a fixed input; ADR-048/049's own `CaptureConsumer` design
(`copyLatestFrame()`'s exact contract) is consumed exactly as those entries
left it, unaltered.

**Real-hardware verification, same session, run at Steve's own real
terminal after this entry was first written.** `cmake --build build` clean.
`ctest --test-dir build --output-on-failure`: 25 of 26 tests passing, the
sole failure the already-accepted `test_decklink_device`/`foundDuplexDevice`
exception (ADR-035), unrelated to this unit; the new `test_decklink_live_output`
itself passing, all 10 automated checks green. `./build/test_decklink_live_output`
run directly, with a live source patched into the Recorder 3G's own SDI
input (not the Monitor 3G → Recorder 3G self-loop WU-20b's/WU-21b's own
tests use — this unit's own body above reasoned through that self-loop
before Steve confirmed a live source was available instead) and the Monitor
3G's own HDMI-mirrored output watched live, over its own bounded 5-second
window: `completed=124 displayedLate=0 dropped=0 flushed=0
framesRepeated=18`; `framesArrived=124 framesPushed=97 | framesPopped=89
framesProcessed=89 framesFailed=0`. `framesProcessed(89) + framesFailed(0)
== framesPopped(89)` and `framesPopped(89) <= framesPushed(97)` both hold
exactly, as this unit's own `Accept:` requires; `completed(124) > 0`,
`displayedLate == 0`, `dropped == 0`, all as required.

`framesPushed(97) - framesPopped(89) == 8`, exactly `kCaptureRingCapacity`,
the same exact relationship WU-21b's own real run showed (`89 - 81 == 8`) —
a second real data point consistent with WU-21b's own reading (ADR-049's
addendum): the gap reads as "up to one ring's worth left unconsumed when the
bounded run stops," not a growing backlog. The `framesArrived`/`framesPushed`
gap itself (124 vs. 97, ~22%) is close in magnitude to WU-21b's own ~28%,
reinforcing rather than newly resolving `kCaptureRingCapacity=8`'s own open
question — still not conclusively diagnosed, still named, not resolved, now
with two consistent real-hardware data points instead of one.

`framesRepeated=18` of `completed=124` (~15%) is this unit's own first real
data point for the no-explicit-sync genlock/clock-domain decision above:
roughly one output tick in seven found nothing fresher than what was already
scheduled and repeated it rather than stall, visible by eye as a mild
jerk/stutter in the re-output picture on the Monitor 3G's own HDMI-mirrored
output during this run — consistent with `CaptureConsumer`'s own measured
throughput (89 processed frames over ~5 seconds, roughly 18fps) running
below the output's own fixed 25fps schedule. Exactly the accepted
consequence this entry's own no-explicit-sync paragraph above named in
advance, not a surprise; the magnitude is now measured rather than merely
anticipated.

A second, previously unanticipated finding, caught by eye on this same run
and confirmed by re-reading this unit's own `refillAndSchedule()`: the
re-output picture on the Monitor 3G's own output showed a few seconds of
solid green before genuine captured content appeared, at the very start of
playback. Not a defect in the loop itself — `startWith()`'s own preroll loop
schedules every pool buffer once, via `refillAndSchedule()`, before
`StartScheduledPlayback` runs; at that instant `CaptureConsumer` has not yet
produced its own first output, so `copyLatestFrame()` returns false for each
of those initial calls, and per this file's own "nothing fresher — leave the
buffer's existing content unchanged" policy (`refillAndSchedule()`'s own
body, this entry's genlock paragraph above), each pool buffer's content is
left exactly as `CreateVideoFrame()` first allocated it — effectively
zero-filled `v210`, which decodes as a strongly saturated green once
converted for display, a well-known artifact of displaying unwritten YUV.
Green clears once enough completions have cycled through to refill every
pool slot with real content, which is gated on `CaptureSource`/
`CaptureConsumer` producing their own first output, not on anything in the
output side — matching the "few seconds" observed. This entry's own genlock
paragraph above reasoned about the *steady-state* "nothing fresher since
last refill" case; it did not anticipate the *cold-start* "never filled at
all" case, which is the one actually hit here. Named, not fixed in this
session — a candidate fix (fill each pool buffer with black immediately
after `CreateVideoFrame()`, before the preroll loop schedules any of them,
so a cold start shows black rather than green) is left for whoever next
touches this file.

Confirmed `green`, tagged `wu-21c-green` (git tag command given to Steve for
his own real terminal; not yet confirmed present as of this addendum being
written). This addendum does not reopen anything named in this entry's own
"Does not reopen" paragraph above — it records a real-terminal result for a
unit that paragraph already described as reasoned-through-only at the time
it was written, plus the cold-start green finding as new evidence, not a
reopening of any prior entry.

**ADR-051 — WU-21e: first live-warped-video demo, `tests/
test_decklink_live_sphere.cpp`, a real sphere lattice through the live
pipeline WU-21c already verified.** Steve asked, directly after WU-21c's own
real-hardware verification, when it would be possible to see genuinely
warped (not identity-mapped) live video for the first time, ideally a
sphere. Answer, checked against the real delivered code rather than assumed:
today, with no new algorithmic work, because the two pieces that combine to
answer it were already independently complete and verified before the
question was asked. `core/shapes/sphere.cpp`'s own `buildSphereLattice()`
(WU-11, ADR-027) has its own algebraic proof and passing test
(`test_shapes.cpp`) that every returned control vertex lands exactly on the
configured sphere, for any `angleSpanH`/`angleSpanV` — this project's
sphere-warp math has been correct since WU-11, long before any DeckLink
work began. `CaptureConsumer::CaptureConsumer(CaptureFrameRing&, Lattice,
PipelineParams)` (WU-21b) takes its lattice as a genuine constructor
parameter, copied in, not a hardcoded identity map — WU-21c's own test
choosing `makeIdentityLattice()` was that test's own local scoping choice
(ADR-050: "this unit's own job is the pool-refill/reschedule mechanics, not
re-proving `runFrameBytes()`'s own warp correctness"), not a constraint
`CaptureConsumer` itself imposes. `LiveFramePlayback` (WU-21c) never
inspects what produced the bytes `copyLatestFrame()` returns — it is
warp-agnostic by construction, since its whole job is scheduling whatever
the consumer last produced. So the live pipeline WU-21c's own real-hardware
run just verified (real capture, real processing, real re-output, zero
dropped/late frames) already exercises `runFrameBytes()` with whatever
lattice it is given; only the identity choice in that one test kept the
result looking flat.

This unit's own scope is therefore narrow by construction: one new test
file, no `src/` changes at all, `tests/test_decklink_live_sphere.cpp`
duplicated from WU-21c's own `test_decklink_live_output.cpp`
(`SESSION-PROTOCOL.md` rule 2 — one unit, one test, no shared fixture) with
exactly one line materially different — `CaptureConsumer` built from
`scatter::shapes::buildSphereLattice(SphereParams{radius=220,
angleSpanH=1.2, angleSpanV=1.0, centerX=360, centerY=288})` (a 720x576
destination raster's own centre; radius/angleSpan chosen in the same range
`test_shapes.cpp`'s own sphere checks already use — 150-220 output pixels,
0.8-1.2 radians — for a demo shape with clear curvature that stays on
raster, not re-deriving anything about the sphere formula itself) — instead
of an identity lattice. The bounded run length is 10 seconds, not WU-21c's
own 5, purely to give a real by-eye look more time; every other mechanical
`Accept:` check is identical to WU-21c's own, because swapping the lattice
changes nothing about what pool-refill/reschedule mechanics those checks
measure. `CMakeLists.txt` gets a new `test_decklink_live_sphere` executable,
registered identically to `test_decklink_live_output`.

The one criterion this unit adds cannot be automated: whether the Monitor
3G's own re-output, watched by eye (SDI, mirrored to HDMI — ADR-050's own
addendum already confirmed this project's UltraStudio Monitor 3G design),
genuinely shows the live source mapped onto a sphere rather than a flat
picture. Same division of labour every unattended-hardware by-eye criterion
in this project has used since WU-15b/WU-19b — this entry does not claim
that confirmation has happened; it is Steve's own next terminal action.

Reasoned-through-only as of this entry — not built or run in this session's
own Linux cloud sandbox (no Blackmagic SDK, no AppleClang/Xcode toolchain,
same gap every DeckLink-touching unit has had) and not yet run at Steve's
own real terminal either. Does not reopen ADR-011 (WU-11's own cylinder/
sphere design), ADR-027 (the sphere parametrisation itself), ADR-048/049/050
(the live pipeline's own design) or anything named in ADR-050's own "Does
not reopen" paragraph — this entry recombines already-frozen pieces, it
does not alter any of them.

**ADR-052 — WU-21f: rotating live sphere demo, superseding WU-21e's own
first cut before it was ever built, run, committed or tagged.** Steve ran
neither of WU-21e's own real-hardware checks himself before reporting back
by eye (the same immediate, direct feedback loop WU-21c's own real-hardware
verification used): the sphere looked cropped vertically, and did not look
fully wrapped, and he asked for two further changes (keypress-driven run
length, two-axis rotation) in the same message — enough combined new scope
to become its own unit per this project's own a/b/c/.../f splitting
discipline (WU-12a/b, WU-15a/b, WU-19a/b, WU-20a/b, WU-21a/b/c/d/e/f), not a
quiet in-place edit of an already-delivered unit.

**Diagnosing the crop/wrap report against the actual formula, not
guessing.** `buildSphereLattice()`'s own formula (ADR-027, unchanged,
untouched by this entry): `x = centerX + radius*sin(phi)*cos(psi)`,
`y = centerY + radius*sin(psi)`, where `phi = (s - 0.5)*angleSpanH`,
`psi = (t - 0.5)*angleSpanV`. At `psi == 0`, `max|x - centerX| ==
radius*sin(angleSpanH/2)`; at `phi == 0`, `max|y - centerY] ==
radius*sin(angleSpanV/2)`. WU-21e's own chosen values (`angleSpanH = 1.2`,
`angleSpanV = 1.0`) are unequal, so these two extents were unequal by
construction — not a pipeline defect, a parameter choice this entry's own
predecessor made too conservatively and asymmetrically for a first demo.
Fix: `angleSpanH == angleSpanV == 2.0` radians (half-angle 1.0 rad),
equal by construction so the on-screen extents are equal too, and
comfortably inside the `+/-pi/2` half-angle fold threshold
`tests/test_shapes.cpp`'s own comment documents (folding starts once a
half-angle exceeds `pi/2` — 1.0 rad is roughly two thirds of that,
still squarely in the same non-degenerate regime WU-11's own tests already
cover). Radius increased slightly, 220 to 260, for a somewhat larger demo
disk; centre unchanged at the 720x576 destination raster's own middle.

**Rotation: a rigid 3D rotation of the already-built lattice's own control
points, implemented in the test file, not in `core/shapes/sphere.cpp`.**
`SphereParams` has no rotation parameter, and this entry does not add one —
adding one would touch a fourth file (`shapes.hpp` and `sphere.cpp` both)
for no benefit `buildSphereLattice()` itself needs, since the desired
effect (the video-mapped patch tumbling as a rigid body, not the *texture
mapping* changing) is exactly what a post-hoc rotation of already-computed
`(x, y, z)` points gives directly, around the sphere's own true centre
`(centerX, centerY, radius)` — shapes.hpp's own documented centre, not the
front-facing point at `z == 0` a naive rotation-around-the-lattice's-own-
origin would use instead. Yaw (x/z-plane rotation) applied first, then
pitch (y/z-plane rotation) applied to the yawed result — an arbitrary but
fixed composition order, not commutative in general, not claimed to matter
for this entry's own visual goal (a convincing simultaneous two-axis tumble,
not a physically exact camera/object model — no camera model exists
anywhere in this project, ADR-027's own orthographic-projection paragraph).
Two different, non-integer-ratio periods (4.0s yaw, 6.2s pitch) rather than
one shared period, so the combined motion does not simply repeat on a short
cycle. `buildSphereLattice()` itself is called exactly once, at start; every
subsequent frame's own lattice is `rotateLattice(baseSphere, yaw(t),
pitch(t))`, a fresh `Lattice` each time (the same "populate a fresh Lattice,
return by value" convention every shape-builder in this project already
uses), fed to the new `CaptureConsumer::setLattice()` below.

**The one real risk this approach carries, deliberately not resolved
here.** Every shape this project has ever built — cylinder, sphere, page
turn — is constructed so `z >= 0` always (shapes.hpp's own documented
invariant, ADR-027/028). A rigid rotation around a pivot can move a point
to `z < 0` (nearer than the shape's own original front-most point) if the
rotation is large enough relative to the patch's own extent from the pivot;
this project's own binner/splat/resolve path has never been exercised
against negative depth before, and this session cannot build or run
anything to check what it does there. Rather than find out on real
hardware mid-demo, rotation amplitude here (`kYawAmplitude = 0.5` rad,
`kPitchAmplitude = 0.35` rad) is chosen conservatively, well short of where
this patch's own geometry would plausibly reach `z < 0` — sidestepping the
question, not answering it. Named here for whoever next wants a larger,
more dramatic rotation (a full 180-degree-plus tumble, or a real spin
revealing the sphere's own far side) — that is new, unverified territory,
not this unit's own job.

**`CaptureConsumer::setLattice()` — the one `src/` change this unit makes,
and the reason this needed a new work unit rather than staying entirely in
the test file.** `CaptureConsumer`'s own lattice was `const`, fixed for the
object's whole lifetime, since WU-21b (ADR-049) — correct for every use
case before this one, where nothing ever needed the lattice to change after
construction. Animating rotation needs exactly that. Changed: `m_lattice`
is no longer `const`; a new `m_latticeMutex`, dedicated and separate from
the existing `m_mutex` that guards `m_latestFrame`, guards it instead — the
same "each piece of cross-thread state gets its own lock" shape this class
already used for `m_latestFrame`, extended rather than reused, so a caller
polling `copyLatestFrame()` and a caller animating `setLattice()` never
contend with each other. `processOne()` takes a snapshot copy of the
current lattice under that lock, before touching the capture frame's own
`StartAccess`/`GetBytes`/`EndAccess` bracket at all — keeping that bracket
exactly as tight as ADR-048/049 already established, not held open any
longer while a few hundred KB of control vertices get copied. A full-
lattice copy under lock, once per captured frame (not once per output
tick — capture and output run at different, independently clocked rates,
ADR-050's own genlock paragraph), is the same order of cost
`docs/architecture.md`'s own "16,641 control vertices, once per frame, not
in the fixed-point accumulation path" reasoning already accepts elsewhere
(`core/shapes/shapes.hpp`'s own header comment). `setLattice()` takes effect
for frames processed after the call returns, never retroactively, and is
safe to call from any thread, including before `start()`.

**Keypress-until-stop.** A dedicated thread blocks on a single raw
keypress via `termios` (`ICANON`/`ECHO` cleared, restored before returning),
so the main loop never blocks on stdin itself and can keep animating the
rotation on its own timer (`kRotationUpdateInterval`, 80ms/12.5Hz) while
waiting. Falls back to a bounded 60-second run if `tcgetattr` fails (stdin
not a real TTY) — the same "a caller running this unattended is a real,
honestly reportable state, not a defect" convention this project's own
DeckLink tests already use for "nothing physically connected"
(`test_decklink_input.cpp` and others). The keypress-wait thread is
`detach()`ed, not joined, once the main loop exits by either path — a
thread permanently blocked in one blocking `read()`, with nothing left
referencing it at process exit, is the same "abandoned cleanly" shape this
project already accepts for a ring slot still holding a retained frame at
shutdown (`io/decklink_capture_consumer.hpp`'s own header comment, WU-21b).

Reasoned-through-only as of this entry — not built or run in this session's
own Linux cloud sandbox (no Blackmagic SDK, no AppleClang/Xcode toolchain)
and not yet run at Steve's own real terminal either. Supersedes WU-21e's own
`tests/test_decklink_live_sphere.cpp` content entirely (that unit's own
WORK-UNITS.md entry is marked superseded, not deleted, per this project's
own practice of leaving a record rather than erasing one). Does not reopen
ADR-011, ADR-027, ADR-028, ADR-048, ADR-049, ADR-050 or ADR-051, or anything
named in ADR-050's own "Does not reopen" paragraph — `buildSphereLattice()`,
`LiveFramePlayback`, and every other already-frozen piece this entry
combines are all unaltered; only `CaptureConsumer`'s own lattice mutability
is genuinely new.

**Real-hardware feedback, same session, from Steve at his own real
terminal.** The tumble was visible and, in Steve's own words, "interesting."
Two things asked for next: one rotation axis continuous rather than
oscillating (the other can stay back-and-forth), and materially more
angular wrap — this entry's own `angleSpanH`/`angleSpanV` (2.0 rad each,
~114.6 degrees) read on real hardware as "between 120 and 180" degrees of
the sphere, with the video visibly stopping short of the poles. Both are
real, correctly diagnosed reports, not defects in anything built here —
`kAngleSpanH`/`kAngleSpanV` were this entry's own demo parameter choice,
not a pipeline limit. See ADR-053 (WU-21g) for the fix. Also see
`CORRECTIONS.md` C-017: this entry's own "a large enough rotation can
produce negative depth" paragraph, used above to justify a conservative
rotation amplitude, does not hold up against `shapes.hpp`'s own documented
sphere invariant — caught while scoping WU-21g's own request for
unbounded continuous rotation, not on this entry's own real-hardware run.
Confirmed neither `wu-21e-green` nor `wu-21f-green` has been tagged --
WU-21f's own test file content is superseded by WU-21g below, the same
"superseded before being tagged" treatment WU-21e itself received from
this entry, one unit later.

**ADR-053 — WU-21g: full sphere wrap (pole to pole, seamless 360 degrees
azimuthally), one continuous rotation axis and one oscillating axis, and
the corrected rotation-safety reasoning C-017 above establishes.** Direct
response to the real-hardware feedback immediately above.

**The wrap fix, diagnosed against the actual formula, the same discipline
ADR-052 already used for WU-21e's own report.** `buildSphereLattice()`'s
own formula never limits `angleSpanH`/`angleSpanV` — WU-21e's 1.2/1.0 and
WU-21f's 2.0/2.0 were both this project's own demo choices, not anything
the pipeline enforces. Two changes, each independently justified: (1)
`angleSpanV = pi` exactly — at `psi = +-pi/2` (the half-angle this value
produces), `cos(psi) == 0`, so `x = centerX` and `z = radius` for every
column at that row regardless of `phi` -- every point at that row collapses
to the single 3D point `(centerX, centerY +- radius, radius)`, the sphere's
own north/south pole exactly. This is the standard, expected singularity
of an equirectangular sphere parametrisation (every line of longitude
meets at a pole) -- not a defect, and `test_ewa.cpp`'s own existing
coverage of degenerate/near-degenerate Jacobians (4.6) is the same
machinery this pinch exercises, not new territory this entry adds. `pi` is
exactly the boundary `tests/test_shapes.cpp`'s own comment documents
(folding begins once a half-angle *exceeds* `pi/2`; `sin` is monotonic on
the closed interval `[-pi/2, pi/2]`, attaining its extremes exactly at the
endpoints) -- reaching the poles and staying on the non-folding side are
the same value, not a trade-off. (2) `angleSpanH = 2*pi` exactly -- at
`phi = +-pi`, `sin(phi) == 0` and `cos(phi) == -1` for both signs alike, so
column `0` (`s == 0`) and column `kLatticeMax` (`s == 1`) map to the
identical 3D point for any given `psi` -- the source image's own left and
right edges meet exactly at the sphere's own back seam, a mathematically
seamless full wrap, not an arbitrary large number. This does place `phi`
well outside `[-pi/2, pi/2]` for most of the lattice, which is `tests/
test_shapes.cpp`'s own documented folding regime (I1's "non-invertible
maps, folds, tears" case, `architecture.md` 4.7 phase 1: overlapping
surface points accumulate, are not sorted or culled, `WU-28`'s k-buffer not
built yet) -- expected and accepted here, not avoided, because a full
horizontal wrap is definitionally a case where the front and back
hemispheres both exist in the same lattice and, under this project's own
orthographic projection with no occlusion sorting, can visibly overlap on
screen where their projected positions coincide. Named plainly, not solved:
this is genuinely untested-for-visual-quality territory (the geometry
itself is proven correct and non-crashing by `test_shapes.cpp`'s own
explicit `angleSpan > pi` case; how a full wrap's own back-hemisphere
content actually reads on screen, blended against the front with no
depth-based hiding, is Steve's own next real-hardware observation, not
something this entry can predict).

**Rotation, corrected per `CORRECTIONS.md` C-017.** The sphere invariant
`shapes.hpp` already documents -- every control vertex sits exactly
`radius` from `(centerX, centerY, radius)`, for any `angleSpanH`/
`angleSpanV` -- means a rigid rotation about that same point can never
produce `z < 0`: rotation preserves distance from the pivot, so every
rotated point stays exactly `radius` from `(centerX, centerY, radius)`,
which alone bounds `z` to `[0, 2*radius]` regardless of rotation angle.
ADR-052's own conservative amplitude bound was based on an unverified
worry, not a real constraint (C-017) -- removed here, not replaced with a
different bound. Yaw (the horizontal spin axis) is now continuous and
unbounded: `yaw(t) = kYawAngularVelocity * t`, one full revolution every 8
seconds, wrapped via `std::fmod` for a bounded, readable value (not
required for correctness -- `sin`/`cos` wrap on their own -- purely so
logged/debugged angle values stay small). Pitch stays oscillating,
back-and-forth, as Steve asked, amplitude increased from 0.35 to 1.0
radian now that C-017 removes the reason it was kept small, period
unchanged (6.2s, still a different, non-integer-ratio period from yaw's
own 8s, so the combined motion keeps tumbling rather than repeating on a
short cycle). `rotateLattice()` itself, and `CaptureConsumer::setLattice()`
(WU-21f, `src/io/decklink_capture_consumer.hpp`/`.cpp`), are both
unchanged -- this entry's own scope is entirely new `SphereParams`/
rotation-schedule constants inside the one test file, no `src/` edit
needed this time.

**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
-- WU-21f's own content superseded, same "superseded before tagged"
treatment WU-21e received one unit earlier). No `src/` or `CMakeLists.txt`
change -- `setLattice()` already exists (WU-21f) and needs nothing new to
support a continuously-updating angle instead of an oscillating one; the
executable target already exists and points at this same filename.

**Accept:** the same mechanical criteria WU-21c/WU-21e/WU-21f's own tests
already use, unchanged for the same reason each of those entries already
gave (what changes here is the lattice's own content and update schedule,
never what the pool-refill/reschedule mechanics measure). Unautomatable,
Steve's own by-eye job, same as every DeckLink visual criterion since
WU-15b: confirm the video now wraps fully around, reaching the poles top
and bottom with the expected pinch there, seamlessly at the back seam;
confirm yaw spins continuously in one direction while pitch rocks back and
forth; and report, honestly, what the un-occluded front/back overlap
during a full wrap actually looks like -- this entry does not predict that
answer.

Reasoned-through-only as of this entry -- not built or run in this
session's own Linux cloud sandbox, not yet run at Steve's own real
terminal. Does not reopen ADR-011, ADR-027, ADR-028, ADR-048, ADR-049,
ADR-050, ADR-051 or ADR-052's own live-pipeline/`setLattice()` design --
only ADR-052's own rotation-amplitude justification is corrected (via
C-017), and only this entry's own new `SphereParams`/rotation-schedule
constants are new.

**Real-hardware feedback, same session, from Steve at his own real
terminal: "hugely better."** The pole-to-pole/360-degree wrap geometry
above is confirmed good and is not touched again below. Two follow-ups:
first, a backlog item, not for this session — how this project eventually
handles transparency and front/back switching, given this unit's own full
wrap makes the sphere's front and back hemispheres visibly overlap on
screen with no occlusion sorting, exactly as this entry's own `Accept:`
text predicted it might without predicting the result. Recorded on
`WORK-UNITS.md`'s own WU-28 entry (the k-buffer unit already on the
roadmap for exactly this class of problem) rather than inventing a new
backlog entry for the same open question. Second, immediate: replace this
entry's own automatic yaw/pitch schedule with manual keyboard control —
see ADR-054 (WU-21h) directly below.

**ADR-054 — WU-21h: rudimentary interactive UI, replacing WU-21g's own
automatic rotation schedule with keyboard control.** Cursor keys rotate
(left/right = yaw, up/down = pitch), shift+cursor keys reposition
(shift+left/right = `centerX`, shift+up/down = `centerY`), `I`/`O`
shrink/grow the sphere (`I` = "in" to the screen, smaller radius; `O` =
"out" of the screen, larger radius), `Q` quits. The full pole-to-pole/
360-degree wrap (`angleSpanH == 2*pi`, `angleSpanV == pi`, ADR-053) is
unchanged — Steve's own feedback was about the wrap working, not about
changing it further.

**Input handling redesigned from timer-driven to event-driven, because
there is no more idle animation to advance between keypresses.** WU-21g's
own design ran a separate keypress-wait thread (one blocking read, sets an
atomic flag) alongside a main-thread loop polling that flag every 80ms to
keep advancing an automatic rotation schedule regardless of input. This
unit's own whole point is that nothing moves except in direct response to
a keypress, so that split design (and its own 80ms polling interval,
which existed only to keep automatic rotation smooth between keypress
checks) has no job left to do. Replaced with a single loop on the main
thread: block on `readKey()`, act on whatever it returns, repeat until
`Quit`. Simpler than WU-21g's own design, not just different — one thread
instead of two, no polling interval to choose, no atomic flag.

**`readKey()` and the xterm escape-sequence convention it parses --
genuinely terminal-dependent, not verified in this session.** A plain
cursor key arrives as `ESC [ <letter>` (`A`/`B`/`C`/`D` for up/down/right/
left) on essentially every terminal emulator, including macOS Terminal.app
and iTerm2. A shift-modified cursor key arrives as `ESC [ 1 ; 2 <letter>`
on both of those specific terminals by default (the common xterm
"modifyOtherKeys" convention) -- this is the sequence `readKey()` parses,
but is not a universal standard the way the plain 3-byte sequence is, and
this session has no way to run a real terminal emulator to confirm it
against Steve's own setup. If shift+cursor is not recognised there, the
fallback is harmless by construction: an unmatched sequence at any point
in `readKey()`'s own parsing returns `Key::Unknown`, which the main loop
simply ignores (no `setLattice()` call, no state change) -- never
misread as a different, unintended action. Worth Steve reporting back
either way, since the fix (if needed) is narrow -- adjusting which byte
sequence `readKey()` matches, not the rotation/position/resize logic
itself. One known, accepted rough edge, not engineered around: a bare
`ESC` keypress, not followed by the bytes of a real arrow sequence, leaves
`readKey()` blocked waiting for bytes that will never come until the next
real keypress arrives and gets consumed as if it were the rest of that
sequence -- acceptable for what Steve himself called a "rudimentary UI".

**`makeSphereLattice()`/`rotateLattice()` both take radius/centre as
parameters now, not file-level constants, rebuilt fresh on every
state-changing keypress.** WU-21g's own version hard-coded
`kRadius`/`kCenterX`/`kCenterY` as `constexpr`, since nothing changed them
at runtime; `I`/`O`/shift+cursor now do, so both functions take them as
arguments instead. A full rebuild (`buildSphereLattice()`, 16,641 control
vertices) plus a full rotation (`rotateLattice()`, another 16,641-vertex
pass) on every single keypress, including whatever a terminal's own
key-repeat produces while a key is held, is the same order of per-frame
cost `shapes.hpp`'s own header comment already accepts for this project's
control-lattice work generally ("this runs once per frame ... not in the
fixed-point I4/I6 accumulation path") -- cheap enough on any real hardware
this project targets not to need a cheaper incremental update path.

**The non-interactive fallback, keeping `ctest` from hanging.** If
`tcgetattr` fails (stdin is not a real TTY -- an unattended `ctest` run is
exactly this case), this unit does not attempt to read a key at all: it
runs a short, static, 10-second bounded window with the sphere at its own
initial position, then proceeds to the same final accounting checks every
other DeckLink live test already uses. The alternative -- blocking on
`readKey()` regardless -- would hang `ctest --test-dir build
--output-on-failure` forever the first time this test's own turn came up
in an unattended run, since no keypress can ever arrive there. Same
"unattended is a real, honestly reportable state, not a defect" convention
this project's own DeckLink tests already use for "nothing physically
connected".

**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
-- WU-21g's own content superseded, same "superseded before tagged"
treatment WU-21e and WU-21f each received one unit earlier). No `src/` or
`CMakeLists.txt` change -- `CaptureConsumer::setLattice()` (WU-21f)
already supports an arbitrarily-changing lattice on an arbitrary update
schedule, keypress-driven or timer-driven alike; the executable target
already exists and points at this same filename.

Reasoned-through-only as of this entry -- not built or run in this
session's own Linux cloud sandbox, not yet run at Steve's own real
terminal. Does not reopen ADR-011, ADR-027, ADR-028, ADR-048, ADR-049,
ADR-050, ADR-051, ADR-052 or ADR-053's own wrap geometry -- only this
entry's own input-handling mechanism and the two now-runtime-mutable
`SphereParams` fields (`radius`, `centerX`/`centerY`) are new.

**Real-hardware feedback, same session, from Steve at his own real
terminal: plain cursor-key rotation worked; shift+cursor repositioning did
not.** See `CORRECTIONS.md` C-018 -- this entry's own `ESC [ 1 ; 2
<letter>` shift-sequence claim, already flagged above as unverified when
written, turned out wrong. Steve proposed the fix directly rather than
asking for a terminal-sequence investigation: letter keys instead of a
modifier-key sequence. See ADR-055 immediately below.

**ADR-055 — WU-21i: letter-key manual controls, replacing WU-21h's own
broken shift+cursor scheme.** `X`/`x` increment/decrement `centerX`,
`Y`/`y` increment/decrement `centerY`, `Z`/`z` increment/decrement
`radius` (`Z` bigger/"out", `z` smaller/"in" -- the same sense WU-21h's own
`O`/`I` had) -- one consistent uppercase-increments/lowercase-decrements
scheme across all three axes, replacing shift+cursor and `I`/`O` alike.
Plain cursor-key rotation (left/right = yaw, up/down = pitch) is
unchanged -- it worked on Steve's own terminal, per the feedback
immediately above, so this entry does not touch it.

**Why letter keys, not a different escape-sequence guess.** WU-21h's own
`ESC [ 1 ; 2 <letter>` sequence was one specific convention among several
real ones different terminal emulators use for shift+arrow (others include
`ESC [ <letter>` prefixed differently, or no distinguishable sequence at
all depending on a terminal's own settings) -- debugging which one Steve's
own actual setup sends would need either a real terminal emulator this
session cannot attach to, or several more untested guesses shipped one at
a time. Six ordinary printable ASCII letters sidestep the entire class of
problem: every terminal emulator, without exception or configuration,
delivers a plain character keypress as that exact single byte -- nothing
to detect, nothing that can fail to match a convention. `readKey()`
(unchanged in shape from WU-21h, smaller in scope) now needs only the
already-working plain-arrow escape sequence plus six single-character
comparisons, dropping the shift lookahead entirely rather than replacing
it with a different guess.

**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
-- WU-21h's own content superseded, same "superseded before tagged"
treatment WU-21e/f/g each received one unit earlier). No `src/` or
`CMakeLists.txt` change -- `CaptureConsumer::setLattice()` (WU-21f)
already supports this; the executable target already exists and points at
this same filename.

Reasoned-through-only as of this entry -- not built or run in this
session's own Linux cloud sandbox, not yet run at Steve's own real
terminal. Does not reopen ADR-011, ADR-027, ADR-028, ADR-048, ADR-049,
ADR-050, ADR-051, ADR-052, ADR-053 or ADR-054's own plain-cursor-rotation
design (confirmed working, untouched) -- only ADR-054's own shift-detection
mechanism is replaced, corrected via C-018, not extended.

**ADR-056 — WU-22a: weight-capture plumbing, `PipelineParams::weightOut`
(architecture.md section 8's `src/diag/coverage_view.cpp`, Phase 5's own
"done when" line's "diagnostic coverage view on the Mac display"; session
30's own scoping work).** Session opened by asking Steve directly which
work unit was next per `HANDOFF.md`'s own list of named candidates; he
picked WU-22, which `WORK-UNITS.md` had only ever carried as a bare
`### WU-22 — Diagnostic coverage view` line with no `Files:`/`Accept:` of
its own — this session's own first job, per this project's established
practice for any unit reaching real work with no real scope yet (the same
discipline ADR-040/044/046/048 already used for WU-16/19/20/21), was
therefore real scoping before any code: reading `docs/architecture.md` in
full for what "coverage view" and "det J" actually name in this codebase
(section 4.2's Jacobian, section 8's module layout, section 7's own
"Send the diagnostic coverage view to a Metal window on the Mac's own
display, or to the spare UltraStudio Monitor 3G" correction), then two
direct questions to Steve: where should it display (Metal window on the
Mac vs. the spare Monitor 3G vs. deciding after scoping the data half),
and whether a first cut should capture weight alone or weight and det J
together. Steve picked a Metal window on the Mac's own display, and
weight alone.

**Why weight alone is close to free and det J is not, and why that alone
justifies a first cut without det J at all.** `AccumCell::w`
(`core/types.hpp`, architecture.md 4.5/4.8) is a per-*destination-pixel*
quantity already computed by every `runFrame()` call, in
`core/pipeline.cpp`'s `resolveOneTile()`, immediately before `composite()`
reads it to derive alpha and then the function discards it — capturing it
needs no new arithmetic, only a second write alongside the existing three
(`dest.Y`/`Cb`/`Cr`). `det J` (`core/jacobian.hpp`) is a per-*source*-pixel
quantity evaluated once during PASS 1 (`core/binner.cpp`'s
`generateFragmentsRowRange()`), consumed immediately by
`densityCompensation()`/`chooseSupersample()`, and never retained past that
one use — there is no existing destination-pixel (or even
source-pixel-indexed) slot anywhere in this codebase for a caller to read
it back out of; capturing it would mean either a new PASS-1 accumulation
path or a wholly separate, coarser lattice-resolution evaluation, real
design work this session did not do. Explained to Steve in these terms
before he chose; the choice narrows this unit's own scope to "expose an
already-computed value" rather than "compute and expose a new one".

**Split, per this project's own portable-piece-now/platform-piece-next
discipline (ADR-046/048's own precedent for WU-20/WU-21).** WU-22a below:
the weight-capture plumbing itself, `core/resolve.hpp`/`core/pipeline.cpp`,
genuinely portable (no DeckLink, no Cocoa/Metal, nothing this session's own
Linux cloud sandbox cannot build and run for real) and built, tested and
verified for real this session, the same "reasoned-through-only" gap this
project's every Apple-only or DeckLink-only unit has had closed *before*
being handed to Steve. WU-22b, `src/diag/coverage_view.cpp`, is not built
this session — a real, first-of-its-kind new dependency for this project
(Metal, windowing; `src/diag/` and `src/app/` do not exist anywhere in the
tree yet, confirmed directly this session via the device bridge before any
of this ADR was written) that deserves its own real scoping session rather
than being rushed in alongside WU-22a's own already-substantial design and
verification work.

**Design: `PipelineParams::weightOut`, a non-owning pointer, default
`nullptr` — the same shape `PipelineParams::pool` (WU-19a, ADR-044) already
established for an optional extra, applied here to an optional extra
*output* rather than an optional extra input.** Considered and rejected: a
second output raster threaded through `runFrame()`'s/`runFrameBytes()`'s/
`runFrameFile()`'s own signatures directly — this project's own precedent
(`PipelineParams::pool`, `PipelineParams::threads`) already establishes
that an optional per-call extra belongs on the params struct, not
propagated through three call signatures at once, and every existing
caller (WU-10 through WU-21i) keeps compiling and behaving exactly as
before, byte for byte, with `weightOut` left at its default. Tight-packed,
row-major (`dy * destWidth + dx`), `video::Raster444`'s own convention —
simpler than `video/raster.hpp`'s `Plane`'s arbitrary-stride support, since
nothing about this buffer needs to match a DeckLink-supplied row stride the
way v210 output genuinely does; it never leaves the process as a video
signal. Captures `AccumCell::w` itself, *before* `composite()`'s own
`[0, kWeightUnity]` clamp — a caller can see how far above unity a heavily
overlapped cell's real coverage reached (architecture.md 4.4's own "order
1000 fragments under 32:1 compression"), not merely whether it saturated;
WU-22b's own colour-mapping choice for what to do with a value well above
unity is exactly the kind of visualisation decision this unit should not
make on WU-22b's behalf.

**One write site: `core/pipeline.cpp`'s `resolveOneTile()`, guarded by a
single null check, alongside its existing `dest.Y`/`Cb`/`Cr` writes.**
Every threading branch `runFrame()` has (the `threads<=1` oracle loop,
`params.pool != nullptr`, the per-call `ThreadPool`) already funnels
through this one function, so nothing else in `core/pipeline.cpp` needed
to change — the same "one shared function, cannot silently diverge across
paths" property WU-16a's own file comment already relied on for `dest`
itself, extended here to a second output for free rather than by design
effort. Indexed by `params.destWidth`, not `dest.width` — the two are
always equal by `runFrame()`'s own precondition, but `weightOut`'s own
buffer is documented and sized in terms of `params.destWidth` specifically
(`core/resolve.hpp`'s own doc comment), so the write reads that field
rather than relying on the equality holding silently.

**Files:** `src/core/resolve.hpp` (new `PipelineParams::weightOut` field),
`src/core/pipeline.cpp` (`resolveOneTile()`'s one new write, guarded by a
null check), `tests/test_coverage_capture.cpp` (new); plus `CMakeLists.txt`
(`test_coverage_capture` registered via the existing `scatter_test()`
function — CMakeLists.txt edits have never counted against the "3 source
files" cap in any earlier unit either).

**Accept, and what this unit deliberately does not claim.** Four
properties, each checked directly rather than reasoned through: (1)
capture has zero effect on the pipeline's own existing, already-verified
composited output — `weightOut` is a pure side channel, never a second
input, checked by running the identical construction twice (`weightOut`
null vs. supplied) and diffing `dest` byte for byte; (2) the captured
buffer is exactly zero at destination cells no fragment's footprint can
reach and exactly `kWeightUnity` at cells built from two adjacent,
non-edge source samples under an unscaled placement — see this entry's
own "genuine finding" paragraph below for why "exactly", not
"approximately"; (3) the captured value at every destination cell is
bit-for-bit the same `AccumCell::w` `resolveOneTile()` itself accumulated
— proven by an independent recomputation through the same public PASS-1/
PASS-2 primitives (`generateFragments()`, `splatTile()`, `sumBanks()`)
this test's own file, never touching `core/pipeline.cpp`'s
`resolveOneTile()` or `PipelineParams::weightOut` at all — a plumbing
check (did the right value reach the right index), not a fresh proof that
`splatTile()`/`sumBanks()`'s own arithmetic is correct, which WU-08/09/10
already established; (4) capture is unaffected by
`PipelineParams::threads` — I6's own guarantee (`tests/test_threading.cpp`)
extended to this unit's second output, checked directly at threads 1, 2
and 8 against a magnifying, non-tile-aligned construction. Does not
include, and does not claim, anything about `det J` capture (see above),
`runFrameBytes()`/`runFrameFile()` also exposing `weightOut` (both already
route through `runFrame()` internally and therefore already support it via
`PipelineParams`, but neither is exercised by this unit's own test — a
caller reaching `runFrame()` through either wrapper gets exactly this
unit's own guarantees, untested by name), or anything about `src/diag/` or
a Metal window — WU-22b's own job, not built this session.

**A genuine finding this session's own test-writing surfaced, caught
before being written into this ADR as a claim rather than after.** The
first draft of accept criterion (2) above asserted "near `kWeightUnity`"
(a tolerance of `kWeightUnity/128`) across the *entire* interior of the
same offset-placement construction `tests/test_zoneplate.cpp`'s own
`test_pipeline_partial_coverage_no_fringe()` already uses (a 64x64 flat
source placed at a 0.5-pixel offset within a 128x64 destination) — and
failed, at 14221 of 14471 checks passing, one specific check. Diagnosed
directly against `core/jacobian.hpp`'s own `densityCompensation()` (not
guessed at): for this exact construction, the *destination row* that
corresponds to source row `py = 0` (this placement's `offsetY = 0`, so
source row and destination row coincide exactly) has `dydv` computed at
exactly `0.5` instead of the true `1.0` — `CORRECTIONS.md` C-008(a)'s own
already-documented edge-derivative damping, measured directly for this
construction rather than assumed: `K = 1/|detJ|` doubles to `2.0` at that
one row, and every destination cell in it inherits double the intended
coverage weight. `test_zoneplate.cpp`'s own analogous check routes around
the identical effect on *colour* with a rounding margin, because colour
normalises the weight distortion away (`out = Σ(w·colour)/Σw`); weight
itself has nothing to normalise against, so a tolerance band cannot paper
over a genuine 2x deviation the way it papers over a few codes of
fixed-point rounding. Fixed within this unit's own test file, not by
loosening the tolerance: the check now uses only destination rows and
columns one full step inside the raster's own edge (rows `[1, destH-1)`,
columns `[34, 95)` instead of `[0, destH)`/`[33, 96)`), where the true,
undamped Jacobian applies and the captured weight is `kWeightUnity`
*exactly*, checked with `==`, not a tolerance — a tighter, more honest
check than the one first attempted, not a weaker one. Not a defect in
`core/pipeline.cpp`'s new write site or in anything WU-22a delivers — the
weight this unit captures is genuinely, faithfully, whatever
`AccumCell::w` already was; C-008(a) is a already-frozen, already-corrected
property of `core/lattice.cpp`'s edge handling this unit's own capture is
right to reproduce faithfully rather than paper over. Not logged as a new
`CORRECTIONS.md` entry — C-008(a) already documents this exact mechanism
and its own "up to 50%" figure in full; this is this session's own
application of an already-known correction to a new test's construction,
the same "routine iteration, not a new lesson" category WU-21a's own
`makeRamp`-instead-of-`makeZonePlate` slip (C-006, already documented) was
left unlogged for, per ADR-048.

**Verified for real, this session, in this project's own Linux cloud
sandbox — genuinely built and run, not reasoned through.** Clang 18.1.3
and GCC 13.3.0, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all twenty tests green — the nineteen carried over from
before this unit plus `test_coverage_capture`, 14221 checks, zero warnings
under the project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean, no ASan or UBSan
report. This unit touches no Apple-only or DeckLink-only surface at all —
unlike every WU-14/15a/17/18/20b/21b/21c/... unit before it, there is no
piece of it this sandbox could not already fully verify; UNVERIFIED only
in the sense every unit is until Steve's own real terminal confirms
`cmake --build`/`ctest` clean there too and runs `./tools/close.sh 22a`.
Drafted and written via the device bridge to the real repository, then
re-confirmed landed correctly via `sha256sum` on both sides (identical
hashes, all four files) rather than a full re-stage — this session's own
device-bridge file-staging path (`device_stage_files`) began failing with
`untrusted_device` (a stale/expired desktop sign-in, per the error's own
message) partway through this unit's own close-out, after the writes
themselves had already succeeded via `device_commit_files`, which kept
working; `device_bash` also kept working throughout, which is what the
`sha256sum` cross-check used instead. See `HANDOFF.md` for what this means
for Steve's own next session.

**A stale `.git/index.lock` was also found, this session, via
`device_bash`, and could not be cleared from here — the same
"`device_bash` cannot delete files" limitation this environment already
documents.** Flagged for Steve's own real terminal, not fixed here; see
`HANDOFF.md`'s own "Before doing anything else" section.

Does not reopen `docs/architecture.md`, ADR-010, ADR-013, ADR-015, ADR-017,
ADR-021, ADR-024, ADR-025, ADR-026, ADR-040, ADR-044 or C-008 — the
Jacobian edge-derivative damping C-008(a) already documents is applied
here exactly as already frozen, not revisited; `PipelineParams::pool`'s
own shape (ADR-044) is reused for a second optional field, not modified;
`core/pipeline.cpp`'s own threading branches (ADR-040/041/044) are
unchanged, only `resolveOneTile()`'s own body gains one new guarded write.

**ADR-057 — WU-22b: diagnostic coverage view, the Metal/Cocoa window itself
(`src/diag/coverage_view.hpp`/`.mm`, `tools/coverage_view_demo.cpp`;
architecture.md section 8's `src/diag/coverage_view.cpp`, Phase 5's own
"done when" line; ADR-056's own WU-22a/WU-22b split).** Session opened,
per Steve's own explicit instruction, by first requesting device-bridge
access to `~/src/scatter-dve` and the Blackmagic DeckLink SDK folder, then
reading `SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`, this file,
`CORRECTIONS.md` and `INVARIANTS.md` in full, then verifying `HANDOFF.md`'s
own account of the real repository's tag/commit state directly against the
real repository via the device bridge (`git tag`, `git log --oneline -10`,
`git status --short`, `git show wu-22a-green --stat`) rather than trusting
the file's own prose — `wu-22a-green` confirmed present, and its own
`--stat` confirmed the exact seven files `HANDOFF.md` claimed:
`CMakeLists.txt`, `DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`src/core/pipeline.cpp`, `src/core/resolve.hpp`,
`tests/test_coverage_capture.cpp`. Only then, per Steve's own explicit
instruction not to assume a design, was WU-22b actually scoped, over two
rounds of direct questions (the first round re-asked in clearer,
self-contained form at Steve's own request) covering: launch mechanism (a
new standalone target vs. a flag on the existing live-sphere demo
executable), whether it needs to run concurrently with live capture/output
or can start offline/file-driven, target refresh rate, the weight-to-colour
mapping (and where 1.0x coverage sits in it), window size/resizability, and
`PipelineParams::weightOut`'s own buffer lifetime/threading needs across a
display-window thread and a live-pipeline thread, should one ever exist.

**Steve's own explicit selection criterion, this session — "lowest
overhead in processing terms" — is what decided the launch mechanism and
the offline/online split, not a default this session picked on its own.**
In-process (a flag on the existing live-sphere demo executable, sharing one
process and address space with the capture/output pipeline) beats any
IPC-based design (a second process reading frames over a socket, shared
memory segment, or similar) on that exact criterion — no serialisation, no
inter-process copy, no socket/pipe syscall overhead per frame, only a
pointer hand-off within one process — so in-process is this unit's own
frozen design intent for how a future live-wired coverage window reaches
the pipeline's data. That, in turn, is what makes an *offline*, file/
synthetic-data-driven first cut the right split for *this* session rather
than a contradiction of the in-process choice: this project's own
established portable-piece-now/platform-piece-next discipline
(ADR-046/048/056) already establishes that a new platform dependency gets
built and reasoned through on its own before being wired to a second,
independently complex subsystem (here, the live DeckLink capture/output
pipeline) in the same unit — and Steve's own selection, when asked
directly whether WU-22b should start offline or wired live, confirmed
exactly that ordering ("Yes, offline first"). WU-22b below is therefore
`CoverageWindow` (the reusable class) plus `coverage_view_demo` (a
standalone, hand-run tool that feeds it one static, synthetically warped
frame) — proving the Metal/Cocoa window itself works, in isolation, before
a second unit (WU-22c, below, not built this session) pays the real cost of
wiring it to a second thread's worth of live, concurrently-produced data.

**Colour mapping: grayscale, black at weight 0, white at exactly
`kWeightUnity` (1.0x coverage), clipped to white above — Steve's own
choice, offered against a heatmap alternative and a "leave it open" option,
and picked for being the simplest thing that answers the one question this
diagnostic view exists to answer (architecture.md 7's own correction
naming it "diagnostic"): is a given destination cell under-, at-, or
over-covered, at a glance, with no colour-scale legend required to read
it.** Clipping above unity (rather than continuing to brighten, or wrapping,
or saturating some other channel) was chosen because this unit has no
principled second axis to spend on "how far above unity" without turning a
diagnostic view into a second design problem WU-22a's own `weightOut`
capture (captured pre-clamp specifically so a future view *could* show
this) deliberately left open rather than answering itself (ADR-056); a
future unit that wants to distinguish "1.0x" from "40x" can add a second
mapping mode without changing `CoverageWindow`'s own public shape, since
`updateWeights()` takes the raw, unclamped values, not a pre-mapped image —
the black/white clip is applied inside `coverage_view.mm`'s own
`weightToGray()`, not upstream of it.

**Window: fixed size, matching `PipelineParams::destWidth`/`destHeight`
exactly, not resizable — Steve's own choice, for the same "simplest thing
that answers the one question" reason, and because a fixed size needs no
scaling pass between the captured buffer and the window's own pixels,
which is both simpler to get right on this project's first Metal unit and
strictly cheaper per frame than a resizable window's scaling would be.**
Redraw-on-new-data, not timer-driven: `MTKView`'s own
`enableSetNeedsDisplay = YES` / `paused = YES` combination (Apple's own
documented mechanism for "draw only when told to"), so
`CoverageWindow::updateWeights()` is the only thing that ever schedules a
frame — for this session's own static, single-frame demo this makes no
visible difference, but it is the right choice for WU-22c's own eventual
live-refresh case too: a genuinely free-running display timer would
redraw stale data between live frames for no benefit, where
redraw-on-new-data instead tracks the pipeline's own actual frame rate
exactly, whatever it turns out to be, with no separate rate to keep in
sync.

**Threading: `updateWeights()`/`run()` must be called from the main thread
only — a hard Cocoa/AppKit requirement this unit's own design cannot
relax, not a choice among alternatives — documented in `coverage_view.hpp`
itself, in the same place a caller would look to use the class, rather
than only here.** WU-22b's own `coverage_view_demo` satisfies this
trivially (`main()` itself is the main thread, and there is no second
thread in this tool at all). The double-buffer/atomic-swap design Steve
confirmed as the right shape for a *future* live-wired caller — a
background thread producing frames fills a fresh weight buffer and hands
ownership of it across in one `dispatch_async(dispatch_get_main_queue(),
...)` block, so `CoverageWindow` itself only ever observes one buffer at a
time, on the main thread, with no locking of its own — is recorded as
design intent in `coverage_view.hpp`'s own doc comment on `updateWeights()`
for WU-22c to pick up, not implemented here: there is no second thread
anywhere in this unit's own deliverable to exercise it against, and
building unexercised threading code into a unit this sandbox cannot run at
all would be exactly the kind of unverified-and-unverifiable risk
`SESSION-PROTOCOL.md`'s own discipline exists to keep out of a single work
unit.

**WU-22c, added to `WORK-UNITS.md` this session as `todo`, unscoped beyond
what is recorded here: wiring `CoverageWindow` to
`tests/test_decklink_live_sphere.cpp`'s own live capture/output pipeline,
behind a new flag (working name `--show-coverage`), using exactly the
double-buffer/`dispatch_async` mechanism above.** Not scoped further than
that in this ADR — per this project's own practice (ADR-040/044/046/048/
056), a future unit gets real `Files:`/`Accept:` scoping in its own
session, against the real, by-then-verified `CoverageWindow` rather than
against this session's own untested design intent for it.

**Files:** `src/diag/coverage_view.hpp` (new — the platform-independent
public interface: `CoverageWindowConfig`, `CoverageWindow`, pimpl'd, no
Objective-C type or Apple framework `#include` anywhere in this file, the
same "keep the platform dependency behind the .cpp/.mm boundary" shape
`com_ptr.hpp`/`decklink_device.hpp` already use for the Blackmagic SDK,
ADR-031, applied here to a different platform surface); `src/diag/
coverage_view.mm` (new — this project's first Objective-C++ translation
unit: `NSWindow`, `MTKView`/`MTKViewDelegate`, `id<MTLDevice>`/
`id<MTLTexture>`/`id<MTLRenderPipelineState>`, an inline MSL shader
compiled at runtime via `newLibraryWithSource:options:error:`, and the
keypress-quit/window-close handling ADR-054/055 already established for a
terminal, applied here to a Cocoa window instead); `tools/
coverage_view_demo.cpp` (new — a hand-run tool, not a test, the same
`add_executable`-only shape `tools/make_testpat.cpp` established at WU-03
for exactly the same reason: no automatable pass/fail criterion exists for
"does a GUI window look right", and registering this as a `ctest` would
hang any headless/unattended `ctest` run on a keypress that could never
arrive; builds one sphere-warped frame with `PipelineParams::weightOut`
capture enabled, choosing a sphere specifically because CORRECTIONS.md
C-011 already establishes that its front-facing point is usually the
*sparsest*-covered point rather than the densest, making the displayed
image a livelier check that this is reading real, non-uniform capture data
rather than a hand-rolled gradient); plus `CMakeLists.txt` (a new
`if(APPLE) ... endif()` block: `enable_language(OBJCXX)`, a
`scatter-diag` static library linking `Cocoa`/`Metal`/`MetalKit`/
`QuartzCore`, and the `coverage_view_demo` executable — CMakeLists.txt
edits have never counted against the "3 source files" cap in any earlier
unit either).

**Known risk points, flagged here for whoever builds this first, rather
than discovered silently.** (1) The fragment shader's UV convention —
`coverage_view.mm`'s own `kShaderSource` derives `uv.y` as `(1 - ndc.y)/2`
on the stated assumption that Metal's texture origin is top-left and NDC y
grows upward, intended to land row 0 of the weight buffer (this project's
own `dy * destWidth + dx` row-major convention, top row first) at the top
of the window; if the displayed image is vertically flipped, that sign is
the one place to fix it, flagged in three places in the source itself (the
file header, the shader comment, and nowhere else, deliberately, so there
is exactly one place to look). (2) ARC correctness and the `-fobjc-arc`
compile flag actually taking effect — this project's first Objective-C++
translation unit at all, so there is no earlier unit's own build log to
compare against. (3) Whether `newLibraryWithSource:options:error:`
actually compiles `kShaderSource` cleanly at runtime on Steve's own
Metal/OS version — inline runtime shader compilation has never been
exercised anywhere in this codebase before. (4) The `[NSApp stop:nil]`-
plus-dummy-`NSEventTypeApplicationDefined`-event quit mechanism
(`requestQuit`, `coverage_view.mm`) — a documented but easy-to-get-subtly-
wrong Cocoa idiom; if `Q`/window-close does not actually return control
from `-[NSApp run]`, this is the first place to look. (5) `kWeightUnityLocal`
in `coverage_view.mm` is a hand-mirrored copy of `scatter::kWeightUnity`
(`core/types.hpp`), kept out of this translation unit deliberately (see
above) — nothing enforces the two stay in sync; a future change to
`core/types.hpp`'s own constant must be mirrored here by hand, and nothing
in this codebase will fail to compile or fail a test if it drifts, since
`core/types.hpp` is never included here at all.

**Accept — and what this unit, honestly, does not and cannot claim.** No
programmatic accept criterion exists for this unit at all, by design (see
`coverage_view_demo`'s own reason for being a tool, not a test, above):
acceptance is Steve, at his own real terminal, confirming `cmake --build`
succeeds, `./build/coverage_view_demo` opens a window showing a visibly
non-uniform grayscale sphere-coverage image (not a blank, solid, or
crashed window), and that `Q` or closing the window cleanly exits it. This
unit does NOT claim: that it compiles (never attempted anywhere this
session could run a compiler against Cocoa/Metal/AppleClang at all); that
any of the five known risk points above are actually correct rather than
merely reasoned-through against Apple's own documentation; that
`WU-22c`'s own live-wiring will work once attempted, only that its
threading shape is recorded as intent; or anything about a det-J second
channel, a heatmap colour mode, or the spare UltraStudio Monitor 3G output
architecture.md 7 also names — all explicitly out of this unit's own scope,
per Steve's own scoping answers above.

**UNVERIFIED IN FULL — reasoned through only, never built or run anywhere,
by explicit, stronger-than-usual constraint this session.** Steve's own
instruction this session was stronger than this project's ordinary
Apple-only-unit caveat: this sandbox cannot compile or run anything
touching Metal or Cocoa at all, not merely DeckLink — no Xcode/AppleClang
toolchain exists here of any kind, so unlike WU-14/15a/17/18/20b/21b/21c/
.../22a, where this sandbox could at least build and test the portable
majority of the unit and leave only a DeckLink-specific residue
unverified, WU-22b's entire deliverable is unverified, end to end. All
four files (`coverage_view.hpp`, `coverage_view.mm`,
`coverage_view_demo.cpp`, the `CMakeLists.txt` block) were written locally,
pushed to the real repository via the device bridge's `device_bash`
(heredocs run directly against the mounted real repository at
`~/src/scatter-dve`, this session's own device-bridge folder-access grant —
`device_stage_files` returned `HTTP 403 untrusted_device` for every call
attempted this session, so the write-then-re-stage-and-`Read`-back
confirmation this project's own `SESSION-PROTOCOL.md` rule 8 asks for used
`device_bash`'s own `grep`/`wc -l`/`sha256sum` against the mounted files
directly instead — the same fallback ADR-056 already used once before),
then confirmed present with the expected content and byte counts via
`device_bash`, not compiled or run. `git status --short`, run this session
via `device_bash`, again found the same stale `.git/index.lock`
ADR-056 already flagged, still unremovable from here for the same reason;
still flagged for Steve's own real terminal, not fixed here.

Does not reopen `docs/architecture.md`, ADR-031, ADR-040, ADR-044,
ADR-054, ADR-055 or ADR-056 — `com_ptr.hpp`/`decklink_device.hpp`'s own
platform-boundary shape (ADR-031) is reused, not modified; `ThreadPool`'s
own non-copyable/non-movable resource-class shape (ADR-040) is reused for
`CoverageWindow`, not modified; `PipelineParams::pool`/`weightOut`'s own
non-owning-pointer convention (ADR-044/056) is reused, not modified; the
keypress-quit convention (ADR-054/055) is reused for a window instead of a
terminal, not modified; `PipelineParams::weightOut`'s own capture-side
plumbing and guarantees (ADR-056) are consumed exactly as already frozen,
not revisited.

**Verified at Steve's own real terminal, same session, before this entry's
own first commit — the "UNVERIFIED IN FULL" paragraph above describes
this ADR's own state at the moment it was drafted, not its final state.**
`cmake --build build` succeeded cleanly, zero warnings (Objective-C++
`-fobjc-arc`/Metal/Cocoa framework linking all worked on the first attempt
— none of the five known risk points above turned out to be build-time
failures). `./build/coverage_view_demo` opened a 512x512 window showing a
visibly non-uniform grayscale image — a partial dome/cap shape (this
demo's own `SphereParams::angleSpanH`/`angleSpanV` of 1.2 radians each,
not a full wrap), edges visibly brighter than the centre, matching
`CORRECTIONS.md` C-011's own prediction (the sphere's front-facing point
is the sparsest-covered, not the densest) rather than merely "some
non-uniform pattern" — a specific, falsifiable check, not a vague one, and
it passed. Known risk point (1), the shader's UV/vertical-flip
convention, resolved cleanly with no fix needed: the image displayed
right-side up on the first attempt. Both quit paths confirmed clean:
pressing `Q` with the window focused, and closing the window via its own
close control, each close the window AND return control to the calling
shell promptly, with no hang — known risk point (4), the `[NSApp
stop:nil]`-plus-dummy-event mechanism, works as designed. Known risk
points (2) and (3) (ARC correctness, inline runtime shader compilation)
are subsumed by "it built and ran with zero warnings and rendered
correctly" — nothing further to say about either individually. Known risk
point (5) (`kWeightUnityLocal` hand-mirrored from `core::kWeightUnity`)
remains a standing, not-yet-triggered risk — nothing this session's own
testing could exercise a drift between the two constants, since
`core/types.hpp` has not changed. WU-22b is genuinely `green` in
substance; the formal tag (`wu-22b-green`) and its own commit are Steve's
own next action, not recorded by this session.

**ADR-058 — WU-22c: wiring `CoverageWindow` into the live capture/output
pipeline (`tests/test_decklink_live_sphere.cpp`, `src/diag/coverage_view.hpp`/
`.mm`, `src/io/decklink_capture_consumer.hpp`/`.cpp`, `CMakeLists.txt`;
architecture.md section 8's own "done when" line; ADR-057's own WU-22c
paragraph, picked up here).** Session opened, per Steve's own explicit
instruction, by first requesting device-bridge access to `~/src/scatter-dve`
and the Blackmagic DeckLink SDK folder, then reading `SESSION-PROTOCOL.md`
(including its own "Session close" section, updated Session 31 to require an
explicit `git push` in every future close-out — noted here since it changes
this session's own final command block to Steve, not because it changed
anything about this unit's own design), `HANDOFF.md`, `WORK-UNITS.md`, this
file, `CORRECTIONS.md` and `INVARIANTS.md` in full, then verifying
`HANDOFF.md`'s own account of the real repository's tag/commit state
directly against the real repository via the device bridge (`git tag`,
`git log --oneline -10`, `git status --short`) rather than trusting the
file's own prose — `wu-22b-green` confirmed present, and `git show
wu-22b-green --stat` confirmed the exact seven files `HANDOFF.md` claimed:
`CMakeLists.txt`, `DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`src/diag/coverage_view.hpp`, `src/diag/coverage_view.mm`,
`tools/coverage_view_demo.cpp`. `origin` confirmed configured
(`https://github.com/skunge2000/scatter-dve.git`) and `main` confirmed in
sync with `origin/main` (`git status -sb`). Only then, per Steve's own
explicit instruction not to assume a design beyond ADR-057's own WU-22c
paragraph, was WU-22c actually scoped, via direct questions covering: the
flag's own name and behavior, how the terminal keypress loop and Cocoa's
own required main-thread run loop coexist, which thread produces frames and
whether ADR-057's own double-buffer/`dispatch_async` sketch still fits once
that thread structure was actually read (`decklink_capture_consumer.hpp`/
`.cpp`, `decklink_live_output.hpp`/`.cpp`, `decklink_input.hpp` all read in
full before answering this one), what pressing Q in the coverage window
should quit, and redraw cadence.

**Flag: `--show-coverage`, Steve's own choice to keep ADR-057's own working
name rather than change it.** Parsed from `argv` in `main()`; any other
argument is silently ignored, matching every other test executable in this
project (none of which parse `argv` today), since `ctest` may invoke this
binary with its own arguments this unit has no reason to reject. With the
flag absent, `showCoverage` is `false` throughout, `coverageActive` (see
below) is therefore always `false`, no `CoverageWindow` is constructed, and
`CaptureConsumer`'s own new `coverageCallback` parameter stays at its
default `nullptr` — `decklink_capture_consumer.cpp`'s own `processOne()`
allocates and fills nothing extra when it is null, so this file's own
pre-WU-22c behavior is unchanged in both source (the flag-off branch's own
code is untouched, not refactored, per SESSION-PROTOCOL.md's own anti-drift
rule) and cost.

**The threading question Steve named "a real, unresolved architecture
question, not a detail to assume" — how `test_decklink_live_sphere.cpp`'s
own blocking terminal keypress loop coexists with Cocoa's own
main-thread-only run loop `CoverageWindow` needs — is answered by unifying
onto one main-thread loop via GCD dispatch sources, Steve's own selection
among the options this session offered.** Concretely: a
`DISPATCH_SOURCE_TYPE_READ` dispatch source on `STDIN_FILENO`, queued onto
`dispatch_get_main_queue()`, runs alongside `CoverageWindow::run()`'s own
`[NSApp run]` call on that same (and only, at that point in the process) main
thread — GCD's own main queue is drained as part of `NSApp run`'s own event
processing, the documented mechanism every Cocoa app already uses to
interleave dispatch-queued work with UI events, not something this project
invented. This is why the flag-on path cannot reuse the existing blocking
`readKey()`: a blocking `std::getchar()` call inside a dispatch source's own
event handler would freeze the Cocoa run loop itself on a stray ESC
keypress (an already-accepted, narrower rough edge in the flag-off path,
per WU-21h/i and `CORRECTIONS.md` C-018 — freezing only terminal input),
not just terminal input — so `IncrementalKeyParser`
(`tests/test_decklink_live_sphere.cpp`) is a new, non-blocking, per-byte
state machine (`kIdle`/`kSawEsc`/`kSawEscBracket`) that reifies the same
ESC/`[`/letter parsing `readKey()` already does inline, one byte at a time
as `handleCoverageStdinReadable()` reads them non-blockingly (`STDIN_FILENO`
set `O_NONBLOCK` for the duration of the flag-on interactive loop only,
restored after). Both GCD entry points used
(`dispatch_source_set_event_handler_f`, `dispatch_async_f`, see below) are
the function-pointer variants, not the Objective-C Blocks-based ones —
`tests/test_decklink_live_sphere.cpp` stays a plain `.cpp` file, not a
`.mm`, the same platform boundary `com_ptr.hpp`/`decklink_device.hpp`
already keep for the Blackmagic SDK and `coverage_view.hpp`/`.mm` now keep
for Cocoa/Metal (ADR-031/057).

**Which thread produces frames, and whether ADR-057's own double-buffer/
`dispatch_async` sketch survived contact with the real threading structure:
yes, in spirit, but the actual implementation hands off a fresh
heap-allocated buffer per frame rather than a literal reusable pair of
buffers.** `CaptureConsumer` (WU-21b) already runs its own dedicated
consumer thread (`run()`/`processOne()`, spawned in `start()`) — neither the
DeckLink driver's own capture-completion thread (`CaptureSource`, WU-20b)
nor `LiveFramePlayback`'s own scheduled-completion thread
(`ScheduledFrameCompleted()`, driver-owned, WU-21c) touch pixel bytes or
weights directly; `CaptureConsumer`'s own consumer thread is the one and
only place a coverage buffer could originate. `decklink_capture_consumer.hpp`
gains an opt-in `CoverageCallback` (`std::function<void(std::vector<
WeightAccum>)>`, default `nullptr`) — the same "non-owning/default-absent,
zero cost when not opted into" shape `PipelineParams::pool`/`weightOut`
already established (ADR-044/056), extended here from a raw pointer to a
`std::function` because what this hook hands across is ownership of a
freshly filled buffer, not a pointer into memory the consumer thread is
about to reuse next frame. `processOne()` allocates a local
`std::vector<WeightAccum>` sized to `destWidth * destHeight` only when the
callback is set, points a per-call copy of `PipelineParams` at it
(`callParams.weightOut`, `m_params` itself never mutated), passes that copy
to the same `runFrameBytes()` call that already produces `m_latestFrame`
(so the coverage buffer and the frame it describes are always the same
frame — no separate second render pass), and invokes the callback,
`std::move`-ing the buffer out, only after `m_latestFrame` is published.
The callback given to `CaptureConsumer` in `test_decklink_live_sphere.cpp`
allocates one `CoverageDispatchContext` (window pointer, the moved-in
`std::vector<WeightAccum>`, width, height) on the heap and hands it to
`dispatch_async_f(dispatch_get_main_queue(), ctx, &applyCoverageOnMainThread)`;
`applyCoverageOnMainThread()`, running on the main thread, reclaims the
context via `std::unique_ptr` and calls `CoverageWindow::updateWeights()`.
This achieves the same property ADR-057's own literal double-buffer sketch
was after — `CoverageWindow` only ever observes one buffer at a time, on
the main thread, with no locking of its own — at the cost of one small
heap allocation/free per displayed frame instead of a fixed pair of buffers
reused forever; picked over a literal double-buffer because `std::vector`'s
own move semantics already give a full, safe ownership transfer per call
with no additional bookkeeping (an atomic pointer swap, two named buffers,
whose-turn-is-it logic) that a real double-buffer would need to add on top,
and because this pipeline's own frame rate (PAL, WU-22's own target) makes
one small allocation per frame a cost this project's own conventions
elsewhere already accept without comment (e.g. `Lattice` itself, copied by
value throughout).

**Q, from either channel, quits the whole session — Steve's own choice
against "coverage window only."** `CoverageWindow` already turns a `Q`
keypress or a window-close into a call to its own internal `-requestQuit`
ObjC method, which makes `run()` return (WU-22b, `coverage_view.mm`). This
unit adds `CoverageWindow::requestQuit()`, a new public method
(`coverage_view.hpp`) that is a one-line forward to that same existing
`-requestQuit`, callable from any thread (both `[NSApp stop:]` and `[NSApp
postEvent:atStart:]`, which `-requestQuit` already calls, are documented
by Apple as thread-safe) — additive only, does not touch WU-22b's own
frozen internals, and is exactly the size SESSION-PROTOCOL.md's own work-unit
discipline allows without reopening ADR-057. `handleCoverageStdinReadable()`
calls this same method on recognizing `Key::Quit` from the terminal (or on
EOF/a read error, the same "treat as quit, not a spin" convention
`readKey()` already uses for EOF). Either channel firing makes `run()`
return; this function's own existing post-loop cleanup
(`playback->stop(); consumer.stop(); capture->stop();` plus stats) then
runs exactly as it already did before this unit, unchanged — "quits
everything" falls out of that existing structure, not new shutdown code.

**The coverage window never opens in a non-interactive run — a decision
this session made and is recording here rather than one Steve was asked
about directly, since WU-21i's own non-interactive fallback (stdin not a
real terminal, e.g. an unattended `ctest` run: no interactive control, a
static ten-second bounded run) already establishes the invariant this
follows from.** A Cocoa window's `run()` has no quit signal available to it
in that fallback — nothing will ever type Q, and nothing in the fallback's
own bounded-sleep design currently calls `requestQuit()` on a timer — so
opening one there would hang `ctest` forever instead of completing after
ten seconds. `coverageActive` (`showCoverage && interactive`) is therefore
computed once, up front, and downgrades `--show-coverage` to a no-op
whenever `tcgetattr(STDIN_FILENO, ...)` fails, with a `NOTE` printed to
`stderr` so this is visible, not silently silent. Determining `interactive`
this early — moved from its own original position (inside the `if
(playback)` block, just before the interactive/non-interactive branch) to
before `CaptureConsumer` is even constructed — is a pure reordering, not a
behavior change: `tcgetattr()` is a read-only query with no effect on the
terminal, so moving the call earlier changes nothing observable about the
flag-off path, only how soon in this function's own source order the answer
is known, which `coverageActive`'s own value needs before deciding whether
to construct `CoverageWindow` ahead of `CaptureConsumer` (see next
paragraph).

**Redraw cadence: every processed live frame, no throttling — Steve's own
choice against a throttled alternative.** Matches `CoverageWindow`'s own
`MTKView` `enableSetNeedsDisplay = YES` / `paused = YES` mode (ADR-057): the
window only redraws when told to, and `CaptureConsumer`'s own
`coverageCallback` fires once per successfully processed frame, with no
frame-skipping logic added by this unit — the same "every frame, not
sampled" cadence the rest of this live pipeline (capture, warp, output)
already uses end to end.

**Construction order: `CoverageWindow` is constructed before
`CaptureConsumer`, when `coverageActive` is true.** Not a Cocoa requirement
by itself (nothing about `CoverageWindow`'s own constructor cares what
already exists) but a plumbing requirement of this unit's own design: the
`coverageCallback` lambda passed into `CaptureConsumer`'s constructor
captures the `CoverageWindow*` it will later hand to
`dispatch_async_f`/`applyCoverageOnMainThread()`, so that pointer must
already exist. Both constructions happen before any thread this test itself
spawns starts (`CaptureConsumer::start()` is the first), so `CoverageWindow`'s
own construction — which does real Cocoa/Metal setup, `NSApplication
sharedApplication`, `MTLCreateSystemDefaultDevice()`, window/view creation —
still runs on the process's actual main thread, honouring the same hard
requirement `coverage_view.hpp`'s own doc comment on `updateWeights()`
already states.

**Does not reopen `docs/architecture.md`, ADR-031, ADR-040, ADR-044,
ADR-046, ADR-047, ADR-048, ADR-049, ADR-050, ADR-053, ADR-054, ADR-055,
ADR-056 or ADR-057** — `com_ptr.hpp`/`decklink_device.hpp`'s own
platform-boundary shape (ADR-031) is extended to a second platform surface,
not modified; `CaptureConsumer`/`LiveFramePlayback`'s own independent-
lifecycle, thread-per-object shape (ADR-040/046/047/048/049/050) is
observed and built on, not restructured — no new thread is spawned by this
unit, the existing consumer thread and the process's own main thread are
the only two involved; `PipelineParams::pool`/`weightOut`'s own
non-owning-pointer, opt-in convention (ADR-044/056) is the direct precedent
`CoverageCallback` extends, not modified; WU-21g/h/i's own sphere geometry,
rotation mathematics and letter-key control scheme (ADR-053/054/055) are
unchanged — `applyKey()` duplicates, rather than shares, the flag-off
loop's own inline switch specifically so that loop's own code stays
untouched; ADR-057's own frozen `CoverageWindow` design (colour mapping,
fixed window size, redraw-on-demand `MTKView` mode) is consumed exactly as
built, with exactly one additive public method
(`requestQuit()`) that ADR-057's own text already named as this unit's own
job to add.

**UNVERIFIED IN FULL: this entire unit was authored, reasoned through and
delivered via the device bridge with no Blackmagic SDK, no Cocoa, no Metal,
and no AppleClang/Xcode toolchain available to check any of it against —
the Linux cloud sandbox this session ran in has none of the four. Every
claim above about GCD's own main-queue/run-loop interleaving, dispatch
source non-blocking-read behavior, `dispatch_async_f`/
`dispatch_source_set_event_handler_f`'s own C-linkage-compatible plain
function-pointer usage from a `.cpp` translation unit, and the specific
sequencing this design depends on (main-queue draining actually happening
during `[NSApp run]`, a `DISPATCH_SOURCE_TYPE_READ` source firing exactly
when `read()` can return without blocking) is reasoned from Apple's own
published documentation, not confirmed by building or running any of it.**
Needs `cmake --build build` and a real interactive run — both with and
without `--show-coverage` — at Steve's own real terminal before this unit
can be called `green`. The single most likely first problem, if one exists:
GCD's own main-queue draining during `[NSApp run]` failing to interleave
promptly with Cocoa's own event processing in some way this design did not
anticipate (visible as sluggish or dropped keypresses in the coverage-window
build specifically, not the flag-off build) — nothing in this session's own
reading of Apple's documentation suggested this, but it is the one
mechanism in this design without a directly analogous, already-verified
precedent elsewhere in this codebase.

**Addendum, same session, before this ADR's own first commit — real-terminal feedback, and the fix it prompted.** Steve built and ran
`--show-coverage` at his own real terminal and reported back precisely
(not a vague "it doesn't work"): the coverage window opened and updated,
but none of the sphere controls worked while it had keyboard focus — only
the terminal drove them. Correct behavior, and the underlying cause is
simple and was not an open question this ADR's own scoping conversation
asked about: **keyboard focus in macOS is per-window.** The stdin dispatch
source this ADR describes only ever sees a keystroke typed while the
*terminal* itself has focus; `ScatterCoverageMTKView`'s own `-keyDown:`
(WU-22b, unchanged by this unit until now) only ever recognized `Q`, never
any of the six sphere-control letters or the four arrow keys. Two working
input channels existed, but only one of them understood the sphere's own
vocabulary. Asked directly whether the coverage window should also drive
the full control scheme, or stay a display-plus-quit surface with the
terminal as the sole control channel, Steve chose the former.

**Fix: `CoverageWindow` gains a second, generic opt-in hook —
`SpecialKey` and `setKeyHandler()` (`coverage_view.hpp`) — parallel to,
and following the same non-owning/default-absent shape as, `requestQuit()`
above.** `ScatterCoverageMTKView`'s own `-keyDown:` (`coverage_view.mm`)
still checks for `Q` first, unchanged; for every other key, if a handler is
set, it now classifies the four arrow keys by name (Cocoa's own documented
`NSUpArrowFunctionKey`-style private-use-area Unicode constants, the
standard mechanism for detecting them in `-keyDown:` — no state machine
needed, unlike the terminal's own ESC-sequence encoding, since Cocoa has
already fully decoded the key by the time `-keyDown:` runs) and forwards
either the arrow identity or the raw character to the handler. `CoverageWindow`
itself still carries no sphere-specific vocabulary — `SpecialKey` names
only the four arrows, generically; deciding what `'X'`/`'x'`/`'Y'`/`'y'`/
`'Z'`/`'z'` (or any other key) *means* stays entirely the caller's own job,
the same "this class has no application-level vocabulary of its own"
property `updateWeights()`'s own opaque `weightOut` buffer already has.
`tests/test_decklink_live_sphere.cpp` adds `mapCoverageWindowKey()` (a
third small duplicate of the same letter/arrow mapping, alongside `readKey()`
and `IncrementalKeyParser::mapLetter` — three different-shaped call sites,
the same reasoning already given above for why the first two stay separate
applies to a third) and wires `coverageWindow->setKeyHandler()` to run the
same `applyKey()`/`consumer.setLattice()` logic the stdin channel already
uses, before `run()` starts. Both the stdin dispatch source's own handler
and this new Cocoa `-keyDown:` handler execute on the main thread only
(GCD's main queue and Cocoa's own event dispatch are the same thread), so
both writing to the same `yaw`/`pitch`/`centerX`/`centerY`/`radius`
variables needs no new synchronisation — sequential access on one thread,
exactly as the stdin-only design already had.

Does not reopen anything from this ADR's own "does not reopen" paragraph
above, or `docs/architecture.md`; `requestQuit()`'s own already-drafted
design (this same ADR, above) is unchanged, not modified, by adding a
second, parallel hook alongside it. **Still `UNVERIFIED IN FULL` for this
specific fix** — delivered via the device bridge the same way the rest of
this unit was, reasoned through against Apple's own documented
`NSUpArrowFunctionKey`/`-charactersIgnoringModifiers` behavior, not built
or run by the session that wrote it. Needs the same real-terminal
`cmake --build build` plus an interactive `--show-coverage` run, this time
specifically clicking into the coverage window and confirming all ten
controls (six letters, four arrows) move the sphere from there too, before
this unit can be called `green`.

**ADR-059 — WU-28 scoping: tag-keyed bounded k-buffer (k=4), depth-sort-at-
resolve-only, split into WU-28a/WU-28b (`src/core/types.hpp`,
`src/core/splat.hpp`/`.cpp`, `src/core/resolve.hpp`/`.cpp`,
`src/core/pipeline.cpp`; `WORK-UNITS.md`'s own `WU-28` entry, replaced by
this ADR's own split below).** Session opened per Steve's own explicit
instruction: this session's job was to scope WU-28 (the front/back
occlusion/transparency problem WU-21g/h's own full-360-degree sphere wrap
first made concretely visible on screen, ADR-053, and `WU-28`'s own backlog
entry deferred), not to build it. Repository state verified directly
against the real repository via the device bridge before anything else
(`git tag`, `git log --oneline -5`, `git status --short`): clean working
tree, `HEAD` at `a40e403` (WU-22c), matching `HANDOFF.md`'s own account
exactly, no drift to report. (Separately, this same check found `wu-22c-green`
now exists and `origin/main` is in sync — Steve's own WU-22c close-out
completed between sessions; `WORK-UNITS.md`'s own WU-22c status line, still
reading `wip`, was fixed directly — see that entry.)

Then read `core/types.hpp`, `core/splat.hpp`/`.cpp`, `core/resolve.hpp`/`.cpp`
and `core/pipeline.cpp` in full before asking Steve anything, per his own
explicit instruction — grounding every scoping question in what the code
actually does today (`Frag`'s already-present, currently-unused `z` field;
`AccumCell`'s 32-byte layout with no depth tracking; `accumulateCorner()`
as the one place fragment arithmetic is written; `compositeLayered()`'s
existing two-caller-ordered-layer opacity mechanism, ADR-028/029;
`resolveOneTile()` as the one shared per-tile function every threading path
funnels through, which any new design must keep true of itself too) rather
than a guess.

Steve's first answer, in response to being asked how deep the fix should
go (narrow nearest-only / small bounded-k / general unbounded k-buffer),
was a request to first check how the real Quantel Mirage patent
(US 4,563,703) handles this, before choosing — treated the same way this
project already treats "read the real SDK before scoping" (ADR-031/032/046/
047/048/049/050), extended here to the patent text itself. Findings, from
the patent's own text (Google Patents, US4563703A): its "Z" parameter is
the fractional/sub-pixel coverage-weight fraction allocated to a storage
cell during the splat — the same role this project's own `rawWeight`
(`splat.cpp`'s `accumulateCorner()`) already plays, not a depth value at
all, and *not* what this project's own `Frag::z` field is for. The
patent's only overlap-handling mechanism is the two-layer page-turn-flap
case — transparent-by-default accumulation, or read-replace-write for a
flap marked opaque via an "identification tag" — already fully built in
this project as `compositeLayered()` (WU-12b, ADR-028/029). **The patent
discloses no general mechanism for more than two overlapping surfaces, for
a single surface folding over itself (a sphere's own front and back), or
for arbitrary depth-priority ordering.** `docs/architecture.md` section
4.7 phase 2's "nearest 8 depth-sorted layers" language is this project's
own extrapolation of a conventional computer-graphics k-buffer, not
anything drawn from the patent — this scoping session is genuinely
inventing new design territory here, not porting one.

Remaining scoping answers, each grounded in the code read above: occlusion
and transparency are one unit's worth of underlying k-buffer structure but
two separate, related resolve-time outcomes (Steve's original framing on
`WU-28`'s own backlog entry, reconfirmed, not reopened); memory is
explicitly not a constraint this design needs to economise around
("correctness first"); this design stays entirely inside `scatter-core`
(`core/types.hpp`, `core/splat.hpp`/`.cpp`, `core/resolve.hpp`/`.cpp`,
`core/pipeline.cpp` — no Blackmagic SDK, no Metal/Cocoa anywhere in the
touched set, confirmed against `CMakeLists.txt`'s own target split), so per
Steve's own explicit answer, WU-28a and WU-28b below are buildable,
runnable and testable directly in this project's cloud sandbox once someone
actually writes them — unlike every DeckLink/Cocoa-touching unit before
them, which could only ever be reasoned-through and handed off via the
device bridge.

The concrete design fork: Steve asked for headroom beyond the minimal case
(one folding sphere's own front and back, which alone would only ever need
k=2 — a convex surface's own self-fold crosses any line of sight at most
twice), i.e. genuine support for more than two independently overlapping
surfaces at a shared destination cell, without committing to a fully
unbounded structure — landing on a **fixed per-cell ceiling of k=4**
(matching `docs/architecture.md`'s own old "nearest 8" phase-2 language
loosely, chosen smaller here as the more contained first design; nothing
prevents a later unit from raising the constant).

Working through k=4 concretely surfaced a real risk against I6 (the
determinism oracle, `--threads 1` must be byte-identical to any
multi-threaded run — this project's own documented single most valuable
debugging property, never to be weakened): `Frag::z` is quantised to 16
bits, so adjacent source samples of the *same* continuous surface landing
in the same destination cell routinely produce exactly tied `z` values —
not a rare case. A naive "insert fragment, evict the farthest of k slots
when full" scheme, run incrementally during accumulation, is order-sensitive
on exactly this kind of tie, which would silently weaken I6. Resolution,
agreed with Steve: key each cell's (up to k) slots by `Frag::tag` (surface
id) instead of raw `z`. Same-tag contributions accumulate into their one
shared slot with exactly today's `accumulateCorner()` arithmetic — an
unchanged, already order-independent mechanism (ordinary
commutative/associative integer sum, I4/I6) — so ties within one surface
never need breaking at all. `z` is used only once, at resolve time, to sort
the small number of occupied slots front-to-back for the opaque/blend
composite step below, after all accumulation for that cell is already
finished — a sort over a fully-accumulated, order-independent input is
itself trivially order-independent.

The remaining fork was what happens when *more than k* distinct tags land
in one cell — genuinely more than one folding surface's own front/back,
Steve's own "headroom" case. Two options were put to Steve: (a) no hard cap
during accumulation at all (every distinct tag occupying a cell gets its
own transient slot, however many that is; truncate to the nearest k only
at resolve, after accumulation is complete — fully order-independent in
every case, no caveat needed, at the cost of `TileAccum`'s per-cell storage
becoming a small dynamic/tag-keyed structure instead of a fixed-size
array); or (b) a hard cap of k slots enforced during accumulation itself,
evicting the farthest-so-far tag when a (k+1)th distinct tag arrives,
keeping storage closer to today's fixed-array shape at the cost of that
specific eviction's outcome not being provably order-independent across
threading paths in this one edge case. **Steve chose (b)**, explicitly
accepting this as a documented, honest limitation (in the same spirit
`CORRECTIONS.md` already keeps this project honest about known caveats)
rather than paying the bigger structural change for a case — more than four
independently overlapping surfaces at one destination cell — expected to be
rare relative to the two-surface fold case this unit exists to fix.
WU-28a's own accept criteria test this eviction path for run-to-run
self-consistency (same thread count -> same output, every time), not for
correctness against an independent oracle, which is exactly what (b)'s
caveat concedes it cannot promise for that one case.

Resolve-side outcomes stay two, per Steve's own original `WU-28` framing:
opaque occlusion (nearest tag's slot wins outright, the rest discarded —
the same read-replace-write shape `compositeLayered()` already has,
ADR-028/029, but now driven by depth order among up to k tag-slots instead
of by caller-supplied layer order) and transparency (a user-controlled
blend across the occupied slots sorted front-to-back by `z`). This ADR does
not fix the exact blend formula or the exact new `PipelineParams` field
names — those are WU-28b's own job to design against the real,
already-accumulated slot data it will actually have, the same way `pool`
(WU-19a) and `weightOut` (WU-22a) were each designed as opt-in, default-off,
zero-cost-when-absent `PipelineParams` additions once their own unit
actually built them, not invented wholesale at scoping time.

Sizing: the full design — a new per-cell record type (`core/types.hpp`),
tag-routed accumulation with the eviction policy above
(`core/splat.hpp`/`.cpp`), a new depth-sort-and-composite resolve step plus
`PipelineParams` additions (`core/resolve.hpp`/`.cpp`), and the
`resolveOneTile()` wiring (`core/pipeline.cpp`) — does not fit
`SESSION-PROTOCOL.md`'s own "3 source files plus test, ~400 lines" cap in
one unit. Split into **WU-28a** (storage/accumulation: `core/types.hpp`,
`core/splat.hpp`, `core/splat.cpp`, new test) and **WU-28b**
(resolve/composite: `core/resolve.hpp`, `core/resolve.cpp`,
`core/pipeline.cpp`, new test), the same seam `core/splat.cpp` and
`core/resolve.cpp` already have between them from WU-09/WU-10 onward — see
`WORK-UNITS.md`'s own replaced `WU-28` entry for each sub-unit's own
`Files:`/`Accept:` lines. Both new entry points are additive, alongside
today's `splatTile()`/`sumBanks()`/`composite()`/`compositeLayered()`, not
a change to any of their existing contracts (`SESSION-PROTOCOL.md` rule
2) — every existing shape, test and unit that has never needed
multi-surface handling stays exactly as it is, untouched.

**No code written this session** — scoping only, as explicitly instructed.
Does not reopen `compositeLayered()`'s own existing two-layer design
(ADR-028/029, ADR-009's "not a k-buffer" note stays true of that specific
mechanism), `docs/architecture.md`, or any already-`green` unit. WU-28a is
the natural next session's own first job; per Steve's own answer during
this scoping conversation, that session can build, run and test it directly
in this sandbox, since nothing it touches leaves `scatter-core`.

**ADR-060 — WU-28a build: k-buffer storage/accumulation, unbanked, and the
zero-weight-corner routing fix (`src/core/types.hpp`, `src/core/splat.hpp`/
`.cpp`, `tests/test_kbuffer_storage.cpp`; `CMakeLists.txt`).** Session
opened by verifying real repository state directly (`git tag`, `git log
--oneline -10`, `git status --short`) against `HANDOFF.md`'s own account of
Session 33's close: `HEAD` at `a18a419` ("WU-28 scoping..."), clean tree,
`origin/main` in sync, no drift. Then `core/types.hpp`, `core/splat.hpp`/
`.cpp` re-read in full against ADR-059's own design before writing
anything, to confirm nothing drifted between the scoping session and this
one — nothing had.

Two concrete implementation choices this session made, within ADR-059's
already-fixed design, worth recording since neither was fully decided by
that scoping session:

**(1) The k-buffer storage is NOT split across `kBanks` (4) independent
banks the way `TileAccum` is**, despite ADR-059's own illustrative naming
("e.g. `splatTileKBuffer()`/`sumBanksKBuffer()`") echoing the plain path's
bank-oriented shape. `TileAccum`'s four-bank split exists to pipeline
store-to-load latency for a fragment's four read-modify-writes (ADR-002) —
a performance concern ADR-059's own "correctness first" scope does not ask
this unit to solve. Banking a tag-keyed, eviction-order-sensitive structure
would also require deciding a cross-bank merge order (which bank's own
tag-slot occupancy wins when two banks' partial views of one destination
cell disagree) that ADR-059 never specifies — a genuinely separate design
question, not a free extension of the plain path's scheme, and one this
unit's own scope should not silently invent an answer to. `TileKBufferAccum`
is therefore a single flat per-cell array of up to `kBufferK` `KSlot`,
written directly during `splatTileKBuffer()`'s own pass over `frags`, with
eviction driven by that pass's own arrival order — the same order-dependence
ADR-059 already documents and accepts for the >`kBufferK`-distinct-tags
case, not a new one this choice introduces. `splatTileKBuffer()`/
`sumBanksKBuffer()`'s own two-function shape is kept regardless (accumulate,
then extract to a flat output array), so a later unit could introduce
banking without changing either function's contract; `sumBanksKBuffer()`'s
own name is kept from ADR-059's suggestion even though, without banks, it
now performs a straight per-cell copy rather than a literal bank-sum —
documented in `splat.hpp`'s own comment on `TileKBufferAccum`, not silently
inconsistent with its name.

**(2) A real bug, caught by the cloud sandbox's own compiler and test run,
not reasoned through in advance: `splatCorners()` (unchanged, shared with
the plain path) calls its `sink` callback for every corner inside tile
bounds regardless of that corner's own bilinear weight, including the three
"dead" corners of a fragment sitting at an exact grid position, each with
`rawWeight == 0`.** In the plain path this is inert — `accumulateCorner()`
adds a zero contribution to an `AccumCell` that may never be inspected
again. It is not inert for tag-keyed k-buffer routing: a naive
`splatTileKBuffer()` would still let a `rawWeight == 0` visit claim a free
slot, or evict an occupied one, for a tag making zero real contribution to
that cell — this session's own first build of
`test_kbuffer_storage.cpp`'s `test_single_tag_matches_plain_sumBanks()`
failed exactly this way (two of its "no other cell touched" checks) on the
very first `ctest` run, not caught by design review beforehand. Fixed
within this unit's own file: `splatTileKBuffer()`'s sink lambda now skips
any corner with `rawWeight == 0` before calling `routeIntoKBuffer()`, so a
k-buffer touch always means a genuine one. Not a defect in `splatCorners()`
or in the plain path (`accumulateCorner()`'s own zero-add is genuinely
harmless there, by design) — this only corrects an unchecked assumption
that visiting a corner and contributing to it were the same event, true for
the plain path's arithmetic but not for the k-buffer's own occupancy
tracking. See `CORRECTIONS.md`'s new entry this session.

Built and tested directly in this project's cloud sandbox this session, a
first for any WU-28-adjacent unit and, per `HANDOFF.md`'s own Session-33
account, a first for this project generally: cloned the real
`skunge2000/scatter-dve` origin, applied the same files delivered to the
real repository via the device bridge, configured and built the
`scatter-core`/`test_kbuffer_storage` targets with the sandbox's own CMake
and compiler (GCC 13.3, Linux x86_64 — not Steve's own AppleClang/ARM64
toolchain), and ran the resulting binary directly. `ctest` across the full
portable suite (21 targets, everything `scatter-core` builds without the
Blackmagic SDK or Metal/Cocoa) passed clean, including every pre-existing
test unrelated to this unit — no regression. `test_kbuffer_storage` itself
passed 1082 checks after the zero-weight-corner fix above. This sandbox run
is real compiler/test evidence, not a substitute for Steve's own real
terminal: the sandbox's toolchain is not guaranteed to match his (see
CORRECTIONS.md C-012's own cross-compiler floating-point lesson, though
nothing in this unit's own integer-only accumulation path is exposed to
that specific risk) and the sandbox has no git identity of its own to
commit/tag/push with. Steve's own real-terminal build/run/commit/tag/push
remains this unit's own path to `green`, per `SESSION-PROTOCOL.md`.

Sizing: the three source files plus test came to 430 added lines total (175
across `core/types.hpp`, `core/splat.hpp`, `core/splat.cpp` and
`CMakeLists.txt`; 255 in `tests/test_kbuffer_storage.cpp`) — modestly over
`SESSION-PROTOCOL.md`'s own "~400 lines" figure, judged close enough to the
stated approximation not to warrant a further split, after trimming
comment density once from an initial draft that ran closer to 520.

Does not reopen ADR-059 (the tag-keyed, bounded-k, depth-sort-at-resolve
design; the hard-cap-with-eviction-caveat choice) or any already-`green`
unit. WU-28b (resolve/composite) remains untouched, unstarted, and out of
this unit's own file scope, per ADR-059's own split.

**ADR-061 — WU-28b build: k-buffer resolve, `KBufferResolveMode`
(`Off`/`Opaque`/`Blend`), and the GCC 13 `-Warray-bounds` false positive
(`src/core/resolve.hpp`/`.cpp`, `src/core/pipeline.cpp`,
`tests/test_kbuffer_resolve.cpp`; `CMakeLists.txt`).** Session opened by
requesting device-bridge access to `~/src/scatter-dve` only (not the
Blackmagic SDK folder — this unit touches only `src/core/`, per ADR-059's
own scoping). Read `SESSION-PROTOCOL.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`DECISIONS.md`, `CORRECTIONS.md` and `INVARIANTS.md` in full, then verified
real repository state directly (`git tag`, `git log --oneline -10`, `git
status --short`, `git status -sb`) rather than trusting `HANDOFF.md`'s own
Session-34 account, which recorded WU-28a as `wip`, not `green`, at that
session's own close: `wu-28a-green` is present in `git tag`; `HEAD` is at
`5ba1086` ("WU-28a: k-buffer storage..."), the same commit; `git status
-sb` reads `## main...origin/main` with no ahead/behind marker — Steve's
own real-terminal close-out had genuinely landed since Session 34, this
file's own text just hadn't caught up (`WORK-UNITS.md`'s WU-28a status
line corrected from `wip` to `green` this session, doc-only — see that
file). One untracked `Testing/` directory (`Testing/Temporary/
CTestCostData.txt`, `Testing/Temporary/LastTest.log`) was present at the
repository root, not part of the tracked tree and not blocking the clean-
tree check — almost certainly `ctest` run from the repository root instead
of `build/` at some point; flagged for Steve, not fixed here (`device_bash`
cannot delete files). `core/types.hpp`, `core/splat.hpp`/`.cpp`,
`core/resolve.hpp`/`.cpp`, `core/pipeline.cpp` then re-read in full against
ADR-059/060's own design before writing anything — nothing had drifted.

Two design questions ADR-059 left to this unit, decided:

**(1) A single `KBufferResolveMode` enum field (`Off`/`Opaque`/`Blend`) on
`PipelineParams`, not two separate fields, covers both "opacity mode and
blend control" (`WORK-UNITS.md`'s own phrase).** `Off` is the default
(zero-cost-when-absent, the same shape `pool`/`weightOut` already
established, ADR-044/056) — `resolveOneTile()` falls through to the
unchanged plain `splatTile()`/`sumBanks()`/`composite()` path entirely,
never allocating or touching WU-28a's own k-buffer storage. `Opaque` and
`Blend` are mutually exclusive resolve-time behaviours over the same
underlying occupied-slot set, not independent toggles, so one field
naturally expresses "which of these three things happens," rather than two
fields that could disagree (e.g. blend-control set with opacity mode off).

**(2) Blend mode's exact formula generalizes `compositeLayered()`'s own
existing two-layer read-replace-write mechanism (ADR-028/029) to up to
`kBufferK` occupied slots**, sorted nearest-first by `firstSeenZ`
(`KSlot`'s own "near = 0" convention), ties broken by smallest `tag` for a
deterministic total order, then composited farthest-to-nearest: `bg`
against the farthest slot's own `AccumCell`, the result read back as the
next slot's own background, repeated inward. Chosen specifically because
it reuses `composite()` as its only arithmetic primitive (no new blend math
to prove correct) and is exactly, directly cross-checkable against
`compositeLayered()` itself for the two-slot case — `tests/
test_kbuffer_resolve.cpp`'s own `test_blend_two_slots_matches_
compositeLayered()` does this. `Opaque` mode is simpler: the nearest
occupied slot (by the same `firstSeenZ`/`tag` order) wins outright,
`composite()`'d directly against `bg`, the rest discarded — the same
read-replace-write shape as `Blend`'s own single-slot case, not a
separately-reasoned mechanism.

A real, but minor, build issue — not a design or reasoning error, so no
`CORRECTIONS.md` entry: GCC 13's `-Werror=array-bounds` misfired on
`std::sort(occupied.begin(), occupied.begin() + n, ...)` over a
`std::array<const KSlot*, kBufferK>` (`kBufferK` = 4) with runtime bound
`n` (`error: array subscript 16 is outside array bounds of ... [1]`) — GCC's
own `__final_insertion_sort` internal threshold (16) confusing its
array-bounds analysis at this small, fixed capacity, confirmed a false
positive by building (Clang 18 raised nothing on the same code; the sorted
values and their consumers were already exhaustively covered by Part A/B
of this unit's own test before and after the change). Fixed by replacing
the `std::sort()` call with a hand-written insertion sort loop over the
same `occupied` pointer array — documented in `resolve.cpp`'s own comment
as a known compiler quirk at this array size, not suppressed via pragma.

Unlike WU-28a's own test, this unit's accept criterion (`WORK-UNITS.md`)
genuinely requires exercising real multi-threading, not fragment-order
permutation: `test_kbuffer_pipeline_threads_1_matches_threads_8()` runs the
full pipeline (`runFrame()` end to end, `Blend` mode, single tag — see
below) for a real WU-21g/h folding-sphere frame (`angleSpanH == 2*pi`,
`angleSpanV == pi`, the pole-to-pole seamless-wrap geometry ADR-053
establishes) at `--threads 1` and compares byte-for-byte, per-pixel
per-channel, against `--threads {2, 3, 8}` — I6 for the completed feature,
not just WU-28a's own storage step in isolation. `PipelineParams::tag` is
single-valued per `runFrame()` call, so a single call cannot itself route
fragments into more than one k-buffer slot per cell; this test's own job is
solely to prove threading determinism through the new code paths, since
multi-slot resolve arithmetic is already exhaustively covered directly by
Part A (`Opaque`) and Part B (`Blend`) against synthetic, hand-constructed
slot sets, including known-ratio and tie-break cases.

Built and tested directly in this project's cloud sandbox this session, a
fresh clone of the real `skunge2000/scatter-dve` origin (not a reused
sandbox from any prior session), confirmed at `wu-28a-green`/`5ba1086`
before any file was touched. Verification went beyond WU-28a's own
single-configuration precedent, given the new arithmetic (`compositeKBuffer`)
and the new concurrent code path (`resolveOneTile()`'s k-buffer branch) at
stake: GCC 13.3 and Clang 18.1, Release and Debug, `SCATTER_TILE_LOG2` 4
and 5, plus a Debug AddressSanitizer+UndefinedBehaviorSanitizer build, plus
a Release ThreadSanitizer build — eight configurations in all. All 22
portable `ctest` targets passed clean in every one, including every
pre-existing test this unit did not touch (no regression), with no
sanitizer report of any kind (checked directly against each build's own
`Testing/Temporary/*.log` for the ThreadSanitizer run, not just `ctest`'s
own pass/fail).

Sizing: 243 insertions across `src/core/resolve.hpp` (+70),
`src/core/resolve.cpp` (+65), `src/core/pipeline.cpp` (+109/-9) and
`CMakeLists.txt` (+8), plus 333 lines in the new
`tests/test_kbuffer_resolve.cpp` — 576 total, over `SESSION-PROTOCOL.md`'s
own "~400 lines" figure by a wider margin than WU-28a's own "modestly
over" 430, after one trimming pass from an initial 638 (comment density
only — no test case, blend/opaque coverage, or design-rationale content
cut to make the number smaller). Flagged plainly rather than force a
further split: the four touched files are exactly `WORK-UNITS.md`'s own
`Files:` list for this unit, already the minimum ADR-059's own PASS-2 vs.
resolve/composite split leaves for "resolve/composite" as one coherent
piece — `resolve.hpp`/`.cpp` cannot be split further without separating a
declaration from its own definition, and `pipeline.cpp`'s changes are
confined to `resolveOneTile()` and its two call sites, not spread wider.

Does not reopen ADR-059 or ADR-060, or any already-`green` unit. Both
WU-28 sub-units are now built and sandbox-tested; `wu-28b-green` (Steve's
own real-terminal build/run/commit/tag/push) is this unit's own remaining
step, per `SESSION-PROTOCOL.md`.
