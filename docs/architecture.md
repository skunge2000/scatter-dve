# Scatter-mapping DVE — architecture and implementation plan

A real-time forward-mapping digital video effects processor reproducing the
semantics of the Quantel Mirage (DVM8000/1, 1982), implemented in software on
macOS with Blackmagic UltraStudio 4K Mini I/O.

Target host: MacBook Pro M1 Max, 32 GB.

---

## 1. Scope

### In scope

- Forward-mapping (scatter) warp of live or file-based video onto an arbitrary
  parametric surface defined by a control lattice.
- Per-pixel density compensation and anisotropic anti-aliasing derived
  analytically from the mapping Jacobian.
- Accumulation-based transparency, as the original.
- Coarse-grid-evaluated, per-pixel-applied lighting and shading (the
  "Starlight" equivalent) in a later phase — corrected from "per-pixel
  lighting" per `DECISIONS.md` ADR-070/CORRECTIONS.md C-022: the intensity
  factor is applied per pixel but evaluated once per coarse-grid facet and
  interpolated, not recomputed per pixel.
- 4:2:2 10-bit SDI in and out via v210, at 625i50/576p25, 1080p25 and 1080p50.

### Out of scope

- Genlock. Proof-of-concept runs free.
- 4:4:4 or RGB I/O. Not used in practice.
- Legalisation, clipping or range limiting of any kind.
- GPU acceleration. Deferred until measurement proves it necessary.

### Success criterion

Recognisable reproduction of at least three signature Mirage effects — cylinder
map, sphere map, page turn — running at 1080p50 through SDI with no dropped
frames, with output that stands comparison against the 1982 MPC showreel.

---

## 2. Design invariants

These are contractual. Every module must uphold them and every test exists to
verify them.

| # | Invariant | Rationale |
|---|-----------|-----------|
| 1 | Forward scatter, never inverse gather | Non-invertible maps, folds, tears and shattering are only expressible this way. This is what makes it a Mirage rather than an ADO. |
| 2 | No clipping except v210 protocol limits (codes 4–1019) | Codes 0–3 and 1020–1023 are reserved for TRS. Everything else, including sub-black and super-white, passes through untouched. A DVE does not legalise. |
| 3 | Offset-binary 16-bit internal colour | 10-bit code shifted left 6. Black stays at its BT.601/709 code (64, i.e. 4096 after the shift) and chroma at its achromatic code (512, i.e. 32768); no signed arithmetic needed anywhere. |
| 4 | 64-bit integer accumulators | Worst case per fragment is 65535 × 65535 ≈ 4.29 × 10⁹, already at the uint32 boundary before multi-fragment coverage. |
| 5 | Normalise before compositing | Offset-binary premultiplied values composited against zero produce a green fringe on partial coverage. Divide by accumulated weight first, then composite using coverage as alpha. |
| 6 | Integer arithmetic throughout the accumulation path | Integer addition is associative, so output is bit-identical regardless of thread count or scheduling. This is the single most valuable debugging property in the whole design. |
| 7 | Identity map round-trips bit-exactly | Input v210 equals output v210, byte for byte, illegal excursions included. |

---

## 3. Signal path

```
SDI in (v210 10-bit 4:2:2)
  │
  ├─ v210 unpack ────────────────► Y/Cb/Cr planar, 16-bit offset-binary
  │
  ├─ chroma upsample 4:2:2→4:4:4 ► three full-rate 16-bit planes
  │
  ├─ [de-interlace to frame]      (interlaced modes only, switchable)
  │
  ├─ PASS 1: fragment generation ► fragment records, binned by output tile
  │     lattice eval → Jacobian → K → EWA footprint → [shading]
  │
  ├─ PASS 2: tile resolve ───────► 16-bit planar output
  │     four-bank splat → accumulate → normalise
  │
  ├─ [re-interlace]
  │
  ├─ chroma low-pass + decimate ─► 4:2:2 16-bit
  │
  └─ v210 pack ──────────────────► SDI out
```

Buffer sizes:

| Standard | Active pixels | v210 row bytes | v210 frame bytes | Pixel rate |
|---|---|---|---|---|
| 625i50 | 720 × 576 | 1920 | 1 105 920 | 10.4 Mpx/s |
| 1080p25 | 1920 × 1080 | 5120 | 5 529 600 | 51.8 Mpx/s |
| 1080p50 | 1920 × 1080 | 5120 | 5 529 600 | 103.7 Mpx/s |

v210 packs exactly 6 pixels per 16 bytes, and both 720 and 1920 divide by 6,
so row lengths are exact and already 128-byte aligned. Do not assume this for
arbitrary widths; always take row bytes from the SDK.

---

## 4. Core algorithm

### 4.1 Shape as a control lattice

A shape is a function of `(u, v, t)` producing `(x, y, z)` in output raster
space, sampled onto a coarse lattice once per frame.

This is the direct descendant of the Mirage address map, which held coarse
addresses on disc at every 8th pixel and every 8th line and expanded them in
hardware. We keep the structure and increase the resolution:

- Lattice: 129 × 129 control vertices covering the source raster.
- Evaluated once per frame on one thread — negligible cost at 16 641 vertices.
- Optionally keyframed, with temporal interpolation between shape lattices.
  This is Mirage's morph.

### 4.2 Per-pixel expansion and the Jacobian

Expand the lattice to per-pixel destinations with a Catmull-Rom (cubic)
interpolant. Differentiate the same interpolant analytically to obtain

```
J = [ ∂x/∂u  ∂x/∂v ]
    [ ∂y/∂u  ∂y/∂v ]
```

The Jacobian yields three things at once, which is why it is the centre of the
design:

- **Density compensation.** `K = 1 / |det J|`, clamped to a configured maximum
  compression. This is exactly Mirage's K parameter, but computed per pixel and
  exactly, rather than per region by area integration on an HP minicomputer.
- **Filter footprint.** J *is* the elliptical filter kernel. Anisotropic
  minification comes out correct with no mip pyramid.
- **Surface normal.** Cross product of the two tangent vectors, for lighting.

### 4.3 Fragment records

```cpp
struct Frag {                 // 16 bytes
    uint16_t x, y;            // tile-local destination, 12.4 fixed
    uint16_t Y, Cb, Cr;       // 16-bit offset-binary
    uint16_t w;               // coverage weight, 1.15 fixed (32768 = unity)
    uint16_t z;               // depth
    uint8_t  tag;             // priority / surface id
    uint8_t  reserved;
};
```

One fragment per source sample under compression; 4 or 16 under adaptive
supersampling when `det J > 1` (see 4.6).

### 4.4 Tile binning

Random scatter writes are converted to streaming by sorting fragments into
screen-space tiles, then processing each tile with its accumulator resident in
cache. This is what tile-based GPUs do, and on CPU it is straightforward cache
blocking.

- Tile size is a compile-time constant. Default 32 × 32; 16 × 16 is the
  alternative. **Benchmark both.**
- Fragments whose footprint straddles a tile boundary are replicated into the
  neighbour. Overhead is roughly 6% at 32 × 32, 12% at 16 × 16.

Bin traffic:

| Standard | Fragments/frame | MB/frame | GB/s (write + read) |
|---|---|---|---|
| 625i50 | 414 720 | 6.6 | 0.33 |
| 1080p50 | 2 073 600 | 33.2 | 3.3 |

Against roughly 400 GB/s of unified memory bandwidth on M1 Max, this is
irrelevant even with 4× supersampling.

### 4.5 The four-bank splat

Quantel used four parallel framestores, each with its own multiplier and adder,
because a single store could not sustain four sequential read-modify-writes per
input pixel at 13.5 MHz. Keep the decomposition, for the modern equivalent
reason: it converts one serialised dependency chain into four independent ones
that pipeline.

- Bank A addressed with the base cell, B with base+1, C with base+stride,
  D with base+stride+1.
- Each fragment performs exactly one read-modify-write per bank.
- On resolve, all four banks are addressed identically and summed. This
  reconstitutes the full 2×2 footprint for every output cell.

Accumulator cell: three int64 colour accumulators plus one int32 weight
accumulator, padded to 32 bytes.

| Tile | Bytes/bank | Total, 4 banks | Fits |
|---|---|---|---|
| 16 × 16 | 8 KB | 32 KB | L1D comfortably |
| 32 × 32 | 32 KB | 128 KB | Exactly M1 P-core L1D — measure |

M1 performance cores have a 128 KB L1 data cache and a large, fast shared L2,
so 32 × 32 may still win on reduced edge replication despite spilling. This is
an empirical question, not an architectural one.

### 4.6 Holes and supersampling

Under magnification (`det J > 1`) scatter leaves gaps between landing points.
The correct fix is to emit more fragments, not to patch afterwards:

- Subdivide each source sample into 2×2 or 4×4 sub-samples when `det J`
  exceeds a threshold, interpolating source colour bilinearly.
- Cap the subdivision factor. Uncapped, extreme magnification generates
  unbounded fragment counts.
- Retain a post-hoc dilation of zero-weight cells as a safety net only, and
  make it switchable — the granulation artefact it suppresses is a signature
  Mirage look you may want to keep.

### 4.7 Transparency

**Phase 1: pure weighted accumulation.** Overlapping surfaces sum, exactly as
the original. Order-independent, no sorting, no depth buffer. This is
authentic and it is what the patent describes as the default.

**Phase 2: k-buffer.** Nearest 8 depth-sorted layers per pixel, composited
back-to-front at resolve, with the priority tag forcing opacity for the
read-replace-write case the patent describes for an opaque page-turn flap.

Do not build phase 2 until phase 1 is working. It is a quality refinement, not
a prerequisite.

### 4.8 Normalisation

Per output cell: `out = Σ(w · colour) / Σw`, computed as an int64 divide, with
the zero-weight case flagged rather than silently producing black. Use a
reciprocal LUT plus one Newton-Raphson step if the divide shows up in profiles;
it probably will not on this hardware.

Then, and only then, composite against the background using `Σw` as alpha.

---

## 5. Chroma handling

Scatter mapping is unusually unforgiving here, because every sample gets an
independently computed destination. Warping 4:2:2 directly leaves chroma
samples at non-integer positions relative to their luma neighbours, and
reconstruction produces colour fringing that follows the geometry.

- **Input.** 4- or 6-tap polyphase interpolator, respecting co-sited chroma
  placement (Rec. 601 and Rec. 709 both co-site Cb/Cr with even luma samples
  horizontally). Never pixel-double; the aliasing gets smeared into visible
  artefacts by the warp.
- **Output.** 9- or 11-tap symmetric half-band low-pass on Cb/Cr *before*
  decimating. The warp synthesises high-frequency chroma the source never had,
  wherever the map compresses. Skip this filter and the aliasing will look
  like a bug in the splat.
- Both filters have negative lobes and will ring on sharp transitions, pushing
  legal source content outside 64–940. That is correct and it passes through.

**Write your own converters.** Most library YUV paths clamp to TV range
silently — swscale, anything routed via 8-bit, most RGB helpers. The v210
unpack and chroma resampling are being written by hand for NEON anyway; keep
the whole chain in-house and inherit nobody's clamp.

### Interlace

For 625i50 and 1080i: de-interlace to frames for warping, re-interlace on
output. Vertical chroma is already full-rate in 4:2:2, so only field pairing is
fiddly. Provide a **field mode** that warps each field independently — that is
what Mirage actually did, and it produces the authentic vertical softness and
motion judder.

---

## 6. Threading model

M1 Max: 8 performance cores, 2 efficiency cores.

- **Capture callback thread** (driver-owned). Retains the frame, pushes to a
  lock-free ring, returns immediately. Never blocks, never allocates.
- **8 worker threads**, QoS `USER_INTERACTIVE`.
  - Pass 1 partitions the source by row bands. Each worker owns a private
    per-tile bin arena, preallocated, bump-allocated.
  - Barrier.
  - Pass 2 partitions by tile. Each worker reads all 8 workers' lists for its
    tiles, in fixed worker order.
- **Output scheduler thread.** Driven by the frame completion callback.

Because accumulation is integer, results are bit-identical regardless of thread
count or interleaving. This is a hard requirement, not a nicety: it is what
lets a single-threaded reference build be diffed against the production build
byte for byte.

### Apple Silicon gotcha

Threads inherit QoS, and default QoS may schedule work onto efficiency cores,
producing mysterious 3–4× slowdowns. Call
`pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0)` in every worker
at startup. Consider joining an `os_workgroup` for the real-time media path if
jitter proves troublesome.

---

## 7. macOS and DeckLink specifics

### SDK mechanics

- Blackmagic Desktop Video SDK. Compile `DeckLinkAPIDispatch.cpp` into the
  project; it loads `DeckLinkAPI.bundle` via CFPlugIn at runtime.
- Entry point `CreateDeckLinkIteratorInstance()`. Link
  `CoreFoundation.framework`. No Objective-C required.
- Interfaces are COM-style with `AddRef`/`Release`. Write a small intrusive
  `ComPtr` and use it everywhere; leaked references manifest as devices that
  cannot be reopened until reboot.
- The UltraStudio 4K Mini is full duplex: one `IDeckLink` exposing both
  `IDeckLinkInput` and `IDeckLinkOutput`, sharing one clock.

### Input

```
EnableVideoInput(bmdModePAL, bmdFormat10BitYUV,
                 bmdVideoInputEnableFormatDetection)
SetCallback(...)   // IDeckLinkInputCallback
StartStreams()
```

- `VideoInputFrameArrived` gives a frame valid only for the duration of the
  call unless you `AddRef` it. Copy or retain, then return.
- Implement `VideoInputFormatChanged` and restart the stream on mode change.
  Without it, switching source standards silently produces garbage.
- Always use `GetBytesPerRow()`; never compute row stride yourself.

### Output

- `CreateVideoFrame()` and write into `GetBytes()`. The driver allocates
  DMA-friendly, correctly aligned memory. Implementing your own
  `IDeckLinkVideoFrame` over your own buffer removes one copy and is the later
  optimisation, not the starting point.
- `ScheduleVideoFrame()` with a 3-frame preroll, then
  `StartScheduledPlayback(0, timescale, 1.0)`.
- Refill from `SetScheduledFrameCompletionCallback`.

### Correction on the two SDI outputs

The 4K Mini's spec lists "2 × program out, 1 × loop out". The two program
outputs are almost certainly mirrored copies of one frame buffer, not
independent channels — verify before designing around it. **Send the diagnostic
coverage view to a Metal window on the Mac's own display, or to the spare
UltraStudio Monitor 3G.** The loop out gives a reclocked copy of the source for
side-by-side comparison, which is genuinely useful.

### Timing

Free-running. Expect 3–4 frames end-to-end: one to capture, one to process,
one to two in the output queue. 60–80 ms at 50p. Do not chase it.

Input timing derives from the source, output from the device clock, so an
unlocked source will still drift against the output. When that becomes
annoying, one BNC of black burst into the sync input fixes it. Playing from
file, there is nothing to drift against.

### Environment

- Desktop Video installs a system extension requiring approval in
  System Settings → Privacy & Security. Do this before writing any code and
  confirm the device enumerates in Desktop Video Setup.
- Current Desktop Video lists macOS Sequoia 15 or Tahoe 26 as supported. Match
  the driver release to the installed OS before pinning an SDK version.
- Build: CMake, clang, `-std=c++20 -O3 -mcpu=apple-m1`, `arm_neon.h`.
- Allocate with `posix_memalign` to 128 bytes, or `mmap`/`MAP_ANON` for the
  bin arenas. No huge pages on macOS.

---

## 8. Module layout

```
scatter-dve/
  CMakeLists.txt
  src/
    core/                 # portable, zero platform dependencies
      types.hpp           # fixed-point aliases, Frag, plane descriptors
      lattice.hpp/.cpp    # lattice storage, Catmull-Rom eval, derivatives
      jacobian.hpp        # det, inverse, K, EWA ellipse from J
      binner.hpp/.cpp     # pass 1: fragment generation + tile binning
      splat.hpp/.cpp      # four-bank accumulation
      resolve.hpp/.cpp    # normalise, composite
      pipeline.hpp/.cpp   # orchestration, thread pool, barriers
      shapes/
        plane.cpp cylinder.cpp sphere.cpp pageturn.cpp explode.cpp
    video/
      v210.hpp/.cpp       # unpack/pack, scalar reference + NEON
      chroma.hpp/.cpp     # 422↔444 polyphase
      interlace.hpp/.cpp
    io/
      com_ptr.hpp
      file_source.cpp     # raw .v210, y4m
      file_sink.cpp
      decklink_device.cpp
      decklink_input.cpp
      decklink_output.cpp
    diag/
      coverage_view.cpp   # weight and det J visualisation
    app/
      main.cpp
      config.cpp          # effect and mode configuration
  tests/
    test_v210.cpp
    test_chroma.cpp
    test_jacobian.cpp
    test_ramp_roundtrip.cpp
    test_determinism.cpp
    test_zoneplate.cpp
  tools/
    make_testpat.cpp
```

`core/` and `video/` link no Blackmagic SDK, so most of the test suite runs
with no hardware connected and no driver installed. Two CMake targets:
`scatter-core` (core + video) and `scatter` (the application, adding io, diag
and app).

---

## 9. Test and validation plan

Written before or alongside the code, not after.

| Test | Method | Pass criterion |
|---|---|---|
| v210 round trip | Random buffers through unpack→pack | Byte-identical; NEON matches scalar reference |
| Full-range ramp | Ramp from code 4 to 1019 on Y, Cb, Cr, plus deliberate sub-black and super-white, identity map | Output v210 bit-identical to input, excursions preserved |
| Jacobian | Analytic derivatives vs central differences over the lattice | Agreement to 1e-6 relative |
| Determinism | Same frame, 1 thread vs 8 threads, both tile sizes | Bit-identical output |
| Anti-aliasing | Zone plate through 4:1 and 32:1 compression | No aliasing; compare against a high-quality offline reference resample |
| Page turn | Reproduce patent US 4,563,703 FIG. 5 | Transparent flap by default; opaque with priority tag set |
| Endurance | 1 hour live loop at target standard | Zero dropped or repeated frames beyond documented clock drift |

The ramp test is the foundation. If it passes, the transport, the offsets, the
accumulators and the normalisation are all honest, and every artefact seen
afterwards is genuinely the warp.

---

## 10. Phased implementation

**Phase 0 — Environment.** Install Desktop Video, approve the system
extension, confirm the 4K Mini enumerates with both input and output. Capture
and play back a clip in Media Express. No code.
*Done when:* the device works in someone else's software.

**Phase 1 — Portable core, file to file, 576p25, single-threaded.**
v210 unpack/pack, chroma resample, lattice, Jacobian, K, four-bank splat,
normalise. Affine maps only.
*Done when:* the ramp round-trip and zone plate tests pass.

**Phase 2 — Shapes.** Cylinder, sphere, page turn. Keyframed lattices with
temporal interpolation.
*Done when:* the patent FIG. 5 page turn reproduces in both transparency modes.

**Phase 3 — Output to SDI.** Monitor path only, still reading from file.
*Done when:* warped frames appear on a broadcast monitor, stable for an hour.

**Phase 4 — Threading and NEON.** Thread pool, QoS, NEON v210 and chroma
paths. Real-time at 576i25.
*Done when:* 8-thread output is bit-identical to single-threaded, at frame rate.

**Phase 5 — Live capture.** Full loop through, 576i25. Format detection.
Diagnostic coverage view on the Mac display.
*Done when:* live SDI in, warped, SDI out, one hour clean.

**Phase 6 — Scale up.** 1080p25, then 1080p50. Profile; tune tile size;
adaptive supersampling.
*Done when:* 1080p50 sustained, or the bottleneck is identified and documented.

**Phase 7 — Starlight.** Normals from the lattice, Phong with 4–8 lights in
linear light, two-sided lighting, optional environment map — corrected from
"Blinn-Phong" per `DECISIONS.md` ADR-069/CORRECTIONS.md C-022: the real
Starlight illumination formula (US 5,103,217) is the original Phong
formulation (`cos B` between the line of sight and the reflected ray, no
half-vector), evaluated per coarse-grid facet (ADR-070) and material owned
by `(light, zone)` rather than surface (ADR-071). Front/back source pair
selected by facing (ADR-073) and the sheet-arbitration mechanism (ADR-072/
ADR-074, still open on M1/M2/M3) are also part of this phase's real scope —
see `WORK-UNITS.md` WU-27/WU-33/WU-34/WU-35. k-buffer for correct layered
transparency (already partly built, `WU-28a`/`WU-28b`/`WU-28c`/`WU-28d` —
see `WORK-UNITS.md`; not yet reconciled with ADR-072's transparency-
coefficient rule, tracked as WU-35).

**Phase 8 — Authoring.** Embedded Lua for shape programs, honouring the
original's model in which shapes are programs rather than menu entries. OSC or
WebSocket control surface.

---

## 11. Performance budget

At 1080p50, 103.7 Mpx/s across 8 performance cores at roughly 3.2 GHz gives
about 247 core-cycles per source pixel. Rough allocation:

| Stage | Budget (cycles/px) |
|---|---|
| v210 unpack + chroma upsample | 15 |
| Lattice eval, Jacobian, K | 40 |
| Fragment emit + bin write | 25 |
| Splat (4 banks, RMW) | 60 |
| Normalise + composite | 30 |
| Chroma low-pass + v210 pack | 20 |
| Headroom | 57 |

The splat is the item most likely to overrun, and it is the one least amenable
to vectorisation — NEON has no scatter instruction, so it stays scalar.
576i25 has a 10× larger budget, which is the reason to start there.

---

## 12. Risks

| Risk | Mitigation |
|---|---|
| Desktop Video / macOS version mismatch | Phase 0 exists precisely to find this before any code |
| Threads scheduled to efficiency cores | Explicit QoS in every worker; verify with a core-occupancy trace |
| Splat overruns budget at 1080p50 | Tile-size tuning first; Metal compute with imageblocks and integer atomics as the fallback, which maps onto this design almost unchanged |
| Supersampling explodes fragment count | Hard cap on subdivision factor |
| Interlaced field pairing wrong | Field mode is both a feature and a simpler fallback |
| Reference-count leaks lock the device | ComPtr everywhere; never hold a raw interface pointer |

---

## 13. Notes on provenance

The architecture follows Quantel's own, as documented in **US 4,563,703**
(Taylor, Kellar, Hinson; GB priority 19 March 1982) and its sibling
**US 4,709,393**. Specifically retained: forward scatter with fractional
addresses supplying splat weights; the density compensation term K equal to the
local compression ratio; the four-bank framestore decomposition with fixed
address offsets on write and common addressing on read; the coarse address
lattice with spatial and temporal interpolation; and the priority tag for
forcing opacity. Related reading: **GB 2,158,671** (3D address map projected to
a viewing surface), **US 5,150,213** (projection onto non-planar surfaces) and
**US 5,103,217** (Cawley — 3D position with per-surface-element colour; a
skin-store/chisel machine, not Mirage itself, but one that names and reuses
Mirage's own starlight, floating-viewpoint and colour-processing circuits as
commercial building blocks, and the source of the Phong illumination formula
in `DECISIONS.md` ADR-069). **EP 0248626 / US 4,899,295** (Nonweiler,
priority 3 June 1986) is the Starlight patent proper, cited by name within
US 5,103,217 — identified but not yet obtained; see `docs/sources/
WU-SM-01.md` §2.1 and `DECISIONS.md` ADR-069/070/071. (This corrects no
citation previously in this file: this repository never cited EP 0320166A1,
the misattribution some earlier scatter-dve *working assumptions*, outside
this file, had carried — see CORRECTIONS.md C-022.)

What has been changed, and why:

- K and the filter footprint come from an analytic Jacobian rather than area
  integration on a host minicomputer. Exact and per-pixel.
- Tile binning replaces brute-force parallel framestores for the *bandwidth*
  problem, while the four-bank split is retained for the *serialisation*
  problem. These are distinct issues and only the first is solved by caching.
- Adaptive supersampling eliminates magnification holes, which the original
  could not afford to do.
- Shading in linear light, which the original almost certainly did not do.
