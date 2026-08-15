# Work units
One session, one unit. Each lists the files to pass to `context.sh` and the
acceptance criterion. Units are ordered; do not skip.

Status: `todo` / `wip` / `green` / `red`

---

## Phase 1 — Portable core, file to file, 576p25, single-threaded

### WU-01 — Repo skeleton `green`
CMake, test harness, `types.hpp` with fixed-point aliases and `Frag`. No
algorithm.
**Files:** `CMakeLists.txt`, `src/core/types.hpp`, `tests/harness.hpp`,
`tests/test_smoke.cpp`
**Accept:** `cmake --build build && ctest` green. *Done:* 2076 checks pass in
Release and Debug at both candidate tile sizes, under `-Werror -Wconversion
-Wsign-conversion`.

### WU-02 — v210 unpack and pack, scalar reference `green`
**Files:** `CMakeLists.txt`, `src/video/v210.hpp`, `src/video/v210.cpp`,
`tests/test_v210.cpp`
**Accept:** random-buffer round trip byte-identical; codes 4 and 1019 survive;
row-stride handling matches the 6-pixels-per-16-bytes packing for 720 and 1920.
*Done:* 635 checks in Release and Debug at both tile sizes, AppleClang 17,
clean under ASan and UBSan. Packed output verified byte-identical to FFmpeg's
v210 encoder at 12x2, 48x3, 720x4 and 1920x2.

### WU-03 — Test pattern generator `green`
Full-range ramp (code 4 to 1019 on Y, Cb, Cr), zone plate, and a pattern with
deliberate sub-black and super-white excursions.
**Files:** `tools/make_testpat.cpp`, `tests/test_testpat.cpp`
**Accept:** patterns written as raw `.v210`; ramp contains codes 4 and 1019.
*Done:* `wu-03-green` tagged (this status line was stale — corrected at
WU-04). 10995 checks; see session 3's history in git log for detail.

### WU-04 — Chroma resampling `green`
4:2:2 → 4:4:4 polyphase interpolator (co-sited), 4:4:4 → 4:2:2 with half-band
low-pass before decimation.
**Files:** `src/video/chroma.hpp`, `src/video/chroma.cpp`, `tests/test_chroma.cpp`
**Accept:** flat fields preserved exactly; impulse response matches designed
coefficients; ringing on step edges is present and unclipped.
*Done:* `wu-04-green` tagged. 21342 checks, AppleClang on the M1 Max
(Release, tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13
(Release+Debug, tile 4+5) and GCC 13 ASan+UBSan. (This status line was stale
— corrected at WU-05, same doc-sync slip WU-03's status had at WU-04.)

### WU-05 — File I/O and identity passthrough `green`
Raw `.v210` source and sink, plane descriptors, and an end-to-end identity path.
**This is the I7 milestone.**
**Files:** `src/video/raster.hpp`, `src/io/file_source.cpp`,
`src/io/file_sink.cpp`, `tests/test_ramp_roundtrip.cpp`
**Accept (corrected this session — see CORRECTIONS.md C-006):** v210 file
I/O (`writeV210File`/`readV210File`) round-trips bit-exactly for any
pattern, isolated from chroma resampling; the full chain (unpack → upsample
→ downsample → pack), run file-to-file, round-trips bit-exactly for a flat
chroma field (ADR-020) and for luma always (chroma resampling never touches
Y); chroma on a ramp or excursion pattern is *not* bit-exact through that
chain — the downsample filter is a deliberately lossy anti-aliasing stage,
not a perfect-reconstruction pair with the upsample filter — and is checked
instead for staying within the v210 protocol range (I2).
*Done:* `wu-05-green` tagged. 4459 checks, AppleClang on the M1 Max
(Release, tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13
(Release+Debug, tile 4+5) and GCC 13 ASan+UBSan.

### WU-06 — Lattice and Jacobian `green`
129×129 control lattice, Catmull-Rom expansion, analytic first derivatives.
**Files:** `src/core/lattice.hpp`, `src/core/lattice.cpp`, `tests/test_jacobian.cpp`
**Accept:** analytic derivatives agree with central differences to 1e-6
relative across the lattice interior and at edges.
*Done:* `wu-06-green` tagged. 411 checks, AppleClang on the M1 Max
(Release, tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13
(Release+Debug, tile 4+5) and GCC 13 ASan+UBSan in a Linux sandbox.

### WU-07 — K and EWA footprint from J `green`
**Files:** `src/core/jacobian.hpp`, `tests/test_ewa.cpp`
**Accept:** `K = 1/|det J|` correct for known affine cases; ellipse axes match
analytic values for pure scale, pure rotation and shear; clamping at the
configured maximum compression behaves.
*Done:* `wu-07-green` tagged. 69 checks, AppleClang on the M1 Max (Release,
tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13 (Release+Debug, tile
4+5) and GCC 13 ASan+UBSan in a Linux cloud sandbox.

### WU-08 — Fragment generation and tile binning `green`
**Files:** `src/core/binner.hpp`, `src/core/binner.cpp`, `tests/test_binner.cpp`
**Accept:** fragment count equals source samples under compression; boundary
straddling replicates into exactly the right neighbours; no fragment lost or
duplicated within a tile.
*Done:* `wu-08-green` tagged. 38348 checks at tile 2^5, 10124 at tile 2^4
(test_binner's boundary-replication check is O(width × height × tilesX ×
tilesY), hence the difference — not a bug), AppleClang on the M1 Max
(Release, tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13
(Release+Debug, tile 4+5) and GCC 13 ASan+UBSan in a Linux cloud sandbox.

### WU-09 — Four-bank splat `green`
**Files:** `src/core/splat.hpp`, `src/core/splat.cpp`, `tests/test_splat.cpp`
**Accept:** four-bank result identical to a single-accumulator reference
implementation; int64 headroom verified at synthetic worst case.
*Done:* `wu-09-green` tagged. Both accept criteria checked directly —
`test_splatTile_matches_reference_random` (50000 randomised fragments,
positions spanning interior/tile-edge/replica base cells, every cell of
`Y`/`Cb`/`Cr`/`w` checked exactly against `splatTileReference()`) and the
two int64-headroom tests (see C-007 for why they're split at two scales
rather than one). 6179 checks at tile 2^5, 1571 at tile 2^4 (the difference
is `test_clear_zeroes_all_banks`' and the random-equivalence test's
per-cell loops, both `O(kTilePixels)`, same reason WU-08's check count
differed by tile size), AppleClang on the M1 Max (Release, tile 2^5,
`close.sh`'s config) plus Clang 18 / GCC 13 (Release+Debug, tile 4+5) and
GCC 13 ASan+UBSan in a Linux cloud sandbox.

### WU-10 — Normalise, composite, first affine warp `green`
**Files:** `src/core/resolve.hpp`, `src/core/resolve.cpp`,
`src/core/pipeline.cpp`, `tests/test_zoneplate.cpp`
**Accept:** identity map still bit-exact (I7 holds through the full path); zone
plate through 4:1 and 32:1 compression shows no aliasing; no green fringing on
partial coverage (I5).
*Done:* `wu-10-green` tagged. All three accept criteria checked directly —
`test_i7_identity_full_pipeline()`, `test_zoneplate_4to1_matches_reference()`
+ `test_zoneplate_32to1_matches_reference()`,
`test_composite_partial_coverage_no_green_fringe()` +
`test_pipeline_partial_coverage_no_fringe()` — AppleClang on the M1 Max
(Release, tile 2^5, `close.sh`'s config) plus Clang 18 / GCC 13
(Release+Debug, tile 4+5) and GCC 13 ASan+UBSan in a Linux cloud sandbox,
all clean, zero warnings. See `CORRECTIONS.md` C-008 through C-010 for the
lattice-edge floating-point issues and reference-methodology errors this
unit's own full-pipeline tests were the first to expose, all worked around
within this unit's own files without touching `core/lattice.cpp` or
`core/splat.cpp`.

---

## Phase 2 — Shapes

### WU-11 — Cylinder and sphere `green`
**Files:** `src/core/shapes/shapes.hpp`, `src/core/shapes/cylinder.cpp`,
`src/core/shapes/sphere.cpp`, `tests/test_shapes.cpp`
**Accept:** every control vertex `buildCylinderLattice()`/
`buildSphereLattice()` writes lies exactly (double-precision tolerance) on
the surface of the configured radius — `(x-cx)^2 + (z-R)^2 == R^2` for the
cylinder, `(x-cx)^2 + (y-cy)^2 + (z-R)^2 == R^2` for the sphere — checked at
every one of the 129x129 control vertices, not spot values; `Lattice::
jacobian()`'s analytic derivatives agree with central differences (WU-06's
own method, reused) across a populated cylinder and a populated sphere
lattice, proving the interpolant differentiates correctly on genuinely
curved, non-affine control data for the first time; `runFrame()` with a
flat source through each shape produces no crash and every covered
destination pixel resolves to the source's own colour within ADR-025's
already-established rounding margin. Explicitly not testing self-occlusion
or back-face correctness — that is Phase 2's k-buffer (WU-28) to resolve
(architecture.md 4.7; see HANDOFF.md), and overlapping surface points are
expected to simply accumulate at this phase, not be sorted or culled.
*Done:* `wu-11-green` tagged. All three accept criteria checked directly
— the on-the-surface algebraic identity for every control vertex, the
Jacobian-vs-central-difference check reused from WU-06, and the three
`runFrame()` pipeline checks (cylinder, a self-overlapping/folded
cylinder, sphere) — AppleClang on the M1 Max (Release, tile 2^5,
`close.sh`'s config) plus Clang 18/GCC 13 (Release+Debug, tile 4+5) and
GCC 13 ASan+UBSan in a Linux cloud sandbox, all clean, zero warnings.
First `close.sh 11` attempt was red on AppleClang — one bit-exact
floating-point comparison (`CORRECTIONS.md` C-012) that the cloud sandbox
hadn't caught; fixed within `tests/test_shapes.cpp` alone (tight tolerance
in place of `==`, no production code touched), re-verified across the
full matrix, and confirmed green on the second `close.sh 11` run.
### WU-12a — Page turn, transparent mode `green`
**Files:** `src/core/shapes/shapes.hpp`, `src/core/shapes/pageturn.cpp`,
`tests/test_pageturn.cpp`
**Accept:** reproduces US 4,563,703 FIG. 5's transparent default
(architecture.md 4.7 phase 1; section 9's own test-plan entry, "Transparent
flap by default") — every control vertex `buildPageTurnLattice()` writes is
either exactly flat or lies exactly (double-precision tolerance) on the
surface of the configured curl cylinder, whichever side of the
`turnProgress`-determined flat/curl split it falls on; the spine (hinge)
never moves, for any `turnProgress`; `turnProgress == 0` reduces exactly to
the flat/affine case; `Lattice::jacobian()`'s analytic derivatives agree
with central differences (WU-06's own method) on a populated, genuinely
curling page-turn lattice, including at and near the flat/curl seam; and
two independently generated fragment sets — a page-turn flap and a
full-canvas "page behind" — splatted into the *same* tile bins produce an
`AccumCell` at every destination cell exactly equal (bit-for-bit, I6) to
splatting each layer separately and summing the two results component-wise,
with the flap's own most solidly covered pixel showing strictly higher
accumulated weight and a composited colour that differs from the
page-behind-alone colour by more than ordinary rounding — proving
accumulation transparency actually happens for two real surfaces sharing
one frame. See `DECISIONS.md` ADR-028. Priority-tag opaque mode is WU-12b,
below — does not fit this unit's own file scope; see ADR-028's own scoping
note.
*Done:* `wu-12a-green` tagged. Implemented and verified in a Linux cloud
sandbox first — Clang 18 and GCC 13, Release and Debug, `SCATTER_TILE_LOG2`
4 and 5 (eight configurations, all green, zero warnings, checked explicitly
in the build logs), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean, no ASan or UBSan
report. `tests/test_pageturn.cpp`: 126512 checks (Clang 18, Release, tile
2^5). `./tools/close.sh 12a` then ran clean on the M1 Max with AppleClang
(Release, tile 2^5, the config `close.sh` builds) on the first attempt —
all twelve tests passed, no cloud/AppleClang divergence this time (unlike
WU-11's own C-012).

### WU-12b — Page turn, priority-tag opaque `green`
**Files:** `src/core/resolve.hpp`, `src/core/resolve.cpp`, plus
`tests/test_layered_composite.cpp` (new; name frozen this session — see
`DECISIONS.md` ADR-029).
**Accept:** reproduces US 4,563,703 FIG. 5's "opaque with priority tag set"
mode (architecture.md 4.7 phase 2's own phrase; section 9), via
`DECISIONS.md` ADR-028's own scope decision — a narrower, two-layer
"read-replace-write" mechanism, not WU-28's general k-buffer (ADR-009
unchanged): given two already-splatted `AccumCell` layers (lower, upper)
and the upper layer's own tag, wherever that tag equals a caller-configured
opaque tag the upper layer's own coverage forces opacity (composite the
lower layer against the background first, then composite the upper layer's
own resolved colour over *that* result using the upper layer's own alpha,
replacing rather than summing); any other tag falls back to WU-12a's own
accumulation-sums default (the two `AccumCell`s summed first, then
normalised/composited once). No `core/shapes/*`, `core/binner.cpp` or
`core/splat.cpp` change — exercised against WU-12a's own
`buildPageTurnLattice()` (or any earlier shape) unmodified.
*Done:* `wu-12b-green` tagged. Implemented and verified in a Linux cloud
sandbox first — `compositeLayered()` added to `core/resolve.hpp`/`.cpp` per
`DECISIONS.md` ADR-029 (name, signature and behaviour frozen there),
`tests/test_layered_composite.cpp` new (28 checks: four direct `AccumCell`
unit-test cases covering the tag-mismatch sum path and the opaque path's
full/zero/partial-alpha edges exactly, plus two pipeline-level checks
reusing WU-12a's own page-turn flap/page-behind construction, checked
exactly against an independent local re-derivation of the read-replace-
write formula, not a tolerance). Full matrix green: Clang 18 and GCC 13,
Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations, zero
warnings, checked explicitly in the build logs — not just exit codes —
under the project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean, no ASan or UBSan
report. `./tools/close.sh 12b` then ran clean on the M1 Max with
AppleClang (Release, tile 2^5, the config `close.sh` builds) on the first
attempt — all thirteen tests passed, no cloud/AppleClang divergence this
time (unlike WU-11's own C-012). WU-12 as a whole is done: both of US
4,563,703 FIG. 5's modes reproduced (WU-12a's transparent default,
WU-12b's opaque-with-priority-tag). The WU-12a/WU-12b split above is not
reconciled back into a single WU-12 entry — permanent record of how the
work actually split, the same precedent WU-04's session set correcting
WU-03's stale status line in place rather than erasing it.
### WU-13 — Keyframed lattices, temporal interpolation (morph) `green`
**Files:** `src/core/lattice.hpp`, `src/core/lattice.cpp`, `tests/test_morph.cpp`
(new).
**Accept:** see `DECISIONS.md` ADR-030 for the full design (keyframe count,
blend formula and its rationale, and why `core/shapes/*`, `core/binner.cpp`,
`core/splat.cpp`, `core/resolve.*` and `core/pipeline.cpp` need no change).
`morphLattice(from, to, t)` at `t == 0` reproduces `from` exactly (every
control vertex, `==`) and at `t == 1` reproduces `to` exactly, both by
construction of the `from*(1-t) + to*t` blend formula (C-012: multiplying by
an exact `0.0`/`1.0` is rounding-free); an interior `t` matches an
independently-computed reference blend to a tight relative tolerance (C-012,
not `==`); and `Lattice::jacobian()`'s analytic derivatives on a morphed
lattice built from two distinct, genuinely curved keyframes (not two affine
ones) agree with central differences of `eval()` (WU-06's own method,
reused) across the lattice interior, at its edges, and at the flat/curl or
angular-span seams either keyframe shape may itself contain — proving the
interpolant differentiates correctly on real blended surface data. No
`runFrame()`-level check — this unit sits at WU-06's own layer (pure lattice
mathematics, proven against `Lattice`'s own public API), not the shape
layer WU-11/WU-12a sit at; see ADR-030's own reasoning.
*Done:* `wu-13-green` tagged. Implemented and verified in a Linux cloud
sandbox first — Clang 18 and GCC 13, Release and Debug, `SCATTER_TILE_LOG2`
4 and 5 (eight configurations, all fourteen tests green, zero warnings,
checked explicitly in the build logs), plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes: clean, no ASan or UBSan report. `tests/test_morph.cpp`: 150189
checks (Clang 18, Release, tile 2^5). `./tools/close.sh 13` then ran clean
on the M1 Max with AppleClang (Release, tile 2^5, the config `close.sh`
builds) on the first attempt — all fourteen tests passed, no
cloud/AppleClang divergence this time (unlike WU-11's own C-012). Phase 2
(Shapes) is now done in full: cylinder, sphere, both page-turn compositing
modes, and keyframed/morphed lattices are all reproduced.

---

## Phase 3 — SDI output

### WU-14 — DeckLink device enumeration and ComPtr `wip`
**Files:** `src/io/com_ptr.hpp`, `src/io/decklink_device.hpp`,
`src/io/decklink_device.cpp` (all new), `tests/test_decklink_device.cpp`
(new); plus `CMakeLists.txt` (new `scatter-decklink` target and
`BLACKMAGIC_SDK_DIR` cache variable — CMakeLists.txt edits have never
counted against the "3 source files" cap in any earlier unit either).
**Accept:** `enumerateDeckLinkDevices()` returns at least one device on this
machine (the UltraStudio 4K Mini, confirmed enumerating this session, per
`HANDOFF.md`); every returned device has a non-empty model and display name;
at least one enumerated device reports both `bmdDeviceSupportsCapture` and
`bmdDeviceSupportsPlayback` via `IDeckLinkProfileAttributes::GetInt`
(`BMDDeckLinkVideoIOSupport`) *and* a live `QueryInterface` for both
`IID_IDeckLinkInput` and `IID_IDeckLinkOutput` succeeds against it —
checking architecture.md 7's "one `IDeckLink` exposing both" claim two
independent ways; `QueryInterface`'s COM identity guarantee (same interface,
same object, same pointer) holds through `ComPtr`'s own converting
constructor; repeated enumeration is stable (same device count both times).
No stream is opened anywhere in this unit — no
`EnableVideoInput()`/`EnableVideoOutput()`/`StartStreams()`/scheduled
playback call in `decklink_device.cpp` or its test; that is WU-15 onward's
own job. See `DECISIONS.md` ADR-031 for the full design.
*Status this session:* implemented and written to disk via the device
bridge; **not yet built or run**. This unit needs the real Blackmagic SDK
and an AppleClang/Xcode toolchain, neither present in the Linux cloud
sandbox this session's own implementation and verification work ran in —
the same reason ADR-021 kept `file_source.cpp`/`file_sink.cpp` out of any
SDK dependency, now the reason this unit's own code has never actually been
compiled by this session at all, only reasoned through against the real SDK
headers. Stays `wip` until built and `test_decklink_device` run at the real
terminal; see `HANDOFF.md` for exactly what to run and what is and is not
yet confirmed.
### WU-15 — Scheduled playback, file source to SDI out `todo`
**Accept:** one hour on a broadcast monitor, no dropped frames.

---

## Phase 4 — Threading and NEON

### WU-16 — Thread pool, QoS, per-worker bin arenas `todo`
**Accept:** 8-thread output bit-identical to single-threaded (I6).
### WU-17 — NEON v210 unpack and pack `todo`
**Accept:** bit-identical to scalar reference.
### WU-18 — NEON chroma resampling `todo`
### WU-19 — Real time at 576i25 `todo`

---

## Phase 5 — Live capture

### WU-20 — DeckLink input, format detection, ring buffer `todo`
### WU-21 — Full loop through at 576i25 `todo`
### WU-22 — Diagnostic coverage view `todo`

---

## Phase 6 — Scale up

### WU-23 — Interlace and field mode `todo`
### WU-24 — Adaptive supersampling `todo`
### WU-25 — 1080p25, then 1080p50; tile-size tuning `todo`

---

## Phase 7 — Starlight

### WU-26 — Normals from lattice `todo`
### WU-27 — Blinn-Phong, linear light, two-sided `todo`
### WU-28 — k-buffer `todo`
### WU-29 — Environment map `todo`

---

## Phase 8 — Authoring

### WU-30 — Embedded Lua shape programs `todo`
### WU-31 — OSC or WebSocket control `todo`
