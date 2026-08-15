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
