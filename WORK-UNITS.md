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

### WU-06 — Lattice and Jacobian `wip`
129×129 control lattice, Catmull-Rom expansion, analytic first derivatives.
**Files:** `src/core/lattice.hpp`, `src/core/lattice.cpp`, `tests/test_jacobian.cpp`
**Accept:** analytic derivatives agree with central differences to 1e-6
relative across the lattice interior and at edges.
*So far:* implemented and committed. 411 checks, clean under
`-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror` on Clang 18
and GCC 13 (Release+Debug, tile 4+5) and GCC 13 ASan+UBSan, all in a Linux
sandbox. **Not yet run on the M1 Max / AppleClang, and not yet tagged** —
this session had no access to a real terminal on that machine. Run
`./tools/close.sh 06` there to finish: on a clean green build it tags
`wu-06-green` and this line moves to `green`. See HANDOFF.md.

### WU-07 — K and EWA footprint from J `todo`
**Files:** `src/core/jacobian.hpp`, `tests/test_ewa.cpp`
**Accept:** `K = 1/|det J|` correct for known affine cases; ellipse axes match
analytic values for pure scale, pure rotation and shear; clamping at the
configured maximum compression behaves.

### WU-08 — Fragment generation and tile binning `todo`
**Files:** `src/core/binner.hpp`, `src/core/binner.cpp`, `tests/test_binner.cpp`
**Accept:** fragment count equals source samples under compression; boundary
straddling replicates into exactly the right neighbours; no fragment lost or
duplicated within a tile.

### WU-09 — Four-bank splat `todo`
**Files:** `src/core/splat.hpp`, `src/core/splat.cpp`, `tests/test_splat.cpp`
**Accept:** four-bank result identical to a single-accumulator reference
implementation; int64 headroom verified at synthetic worst case.

### WU-10 — Normalise, composite, first affine warp `todo`
**Files:** `src/core/resolve.hpp`, `src/core/resolve.cpp`,
`src/core/pipeline.cpp`, `tests/test_zoneplate.cpp`
**Accept:** identity map still bit-exact (I7 holds through the full path); zone
plate through 4:1 and 32:1 compression shows no aliasing; no green fringing on
partial coverage (I5).

---

## Phase 2 — Shapes

### WU-11 — Cylinder and sphere `todo`
### WU-12 — Page turn, transparent and priority-tag opaque `todo`
**Accept:** reproduces US 4,563,703 FIG. 5 in both modes.
### WU-13 — Keyframed lattices, temporal interpolation (morph) `todo`

---

## Phase 3 — SDI output

### WU-14 — DeckLink device enumeration and ComPtr `todo`
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
