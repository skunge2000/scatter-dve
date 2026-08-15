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

### WU-14 — DeckLink device enumeration and ComPtr `green`
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
*Done:* `wu-14-green` tagged. Implemented in a Linux cloud sandbox with no
Blackmagic SDK and no AppleClang/Xcode toolchain at all — the first unit
since WU-05 whose own code was never run through the Linux Clang 18/GCC 13/
ASan/UBSan matrix before being written to disk, since that matrix cannot see
anything gated behind `BLACKMAGIC_SDK_DIR` (see `DECISIONS.md` ADR-031 for
why, and for the full design: the real SDK's `IDeckLink`/`IDeckLinkIterator`
shape, the owned- vs. borrowed-reference distinction `ComPtr::adopt()`
exists for, and the capability-check design). Built and verified for the
first time at the real terminal, on the M1 Max with AppleClang: configured
clean (`DeckLink SDK found at .../Blackmagic DeckLink SDK 16.0`), built
clean under `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` (the `-w` exemption on the SDK's own vendored
`DeckLinkAPIDispatch.cpp` worked as intended — zero warnings anywhere,
including this project's own new files), and `test_decklink_device` passed
all 8 checks against the real UltraStudio 4K Mini — one device enumerated,
non-empty model/display names, full-duplex confirmed both by the
`BMDDeckLinkVideoIOSupport` attribute bits and by live `QueryInterface` for
both `IID_IDeckLinkInput` and `IID_IDeckLinkOutput`, `QueryInterface`'s COM
identity guarantee held, repeated enumeration stable. `./tools/close.sh 14`
then ran clean on the first attempt — all fifteen tests passed (fourteen
carried over unchanged from WU-01 through WU-13, plus
`test_decklink_device`), and `close.sh` needed no changes: it reuses the
existing `build/` directory's CMake cache, which already had
`BLACKMAGIC_SDK_DIR` set from this session's own first configure, so the
variable did not need to be re-passed or added to `close.sh` itself — one
of the two things `HANDOFF.md` flagged as unverified going into this
session's own close, now resolved. Phase 3 (SDI output) is under way:
device enumeration and `ComPtr` are done; WU-15 (scheduled playback, file
source to SDI out) is next.
### WU-15a — Scheduled playback: one looped warped frame, file source to SDI out `green` (`wu-15a-green`)
See `DECISIONS.md` ADR-032 for the full design and for why this splits from
the single WU-15 line above (`HANDOFF.md`'s own flag going into this
session, resolved after reading the real SDK — the same order ADR-028 used
for WU-12a/WU-12b).
**Files:** `src/io/decklink_output.hpp`, `src/io/decklink_output.cpp` (both
new), `tests/test_decklink_output.cpp` (new); plus `CMakeLists.txt`
(`decklink_output.cpp` added to the existing `scatter-decklink` target,
`test_decklink_output` added alongside `test_decklink_device` — CMakeLists.txt
edits have never counted against the "3 source files" cap in any earlier
unit either).
**Accept:** a genuinely warped frame (a cylinder over a zone plate, built
via WU-11's `buildCylinderLattice()` and WU-10's `runFrameFile()`) is
written to a real `.v210` file, then read back and played out via
`LoopedFramePlayback` on the real UltraStudio 4K Mini for a bounded
few-second run: `IDeckLinkOutput::RowBytesForPixelFormat(bmdFormat10BitYUV,
...)` matches this project's own `v210::rowBytesMin()` at the test width
(ADR-032's own consistency check); every `ScheduledFrameCompleted()` result
over that run is `bmdOutputFrameCompleted`, never
`bmdOutputFrameDisplayedLate` or `bmdOutputFrameDropped`; `stop()` returns
cleanly. Steve confirms once, by eye, that the warped frame is visible on a
broadcast monitor while the test runs — architecture.md 10 Phase 3's own
"warped frames appear on a broadcast monitor" half, not its "stable for an
hour" half (that is WU-15b, below). Does not claim endurance.
*Status:* implemented in a Linux cloud sandbox with no Blackmagic SDK and no
AppleClang/Xcode toolchain at all, the same shape ADR-031 already used for
WU-14 — reasoned through against the real SDK headers and the SDK's own
`FilePlayback`/`SignalGenerator` samples (ADR-032's own citations), written
straight to the real machine via the device bridge. Built and run at the
real terminal this session: `test_decklink_output` passed both checks
(`ctest -R test_decklink_output`, 5.34s) against the real UltraStudio 4K
Mini, after switching the display mode from `bmdModePALp` to `bmdModePAL`
(ADR-033; `bmdModePALp` — "576p25" — is not a real transmittable SDI/HDMI/
analogue signal at all, not merely unsupported by this one device — see
`CORRECTIONS.md` C-013). **Still needed before this line goes `green`:**
the full suite / `./tools/close.sh 15a` at the real terminal (only the one
new test has been run in isolation so far), and Steve's own by-eye
confirmation that the warped cylinder frame actually appeared on a
broadcast monitor during the run — the automated checks confirm the
DeckLink-side mechanics, not what is actually on the wire.

**Hardware incident, this session, after the above:** running the full
suite immediately after the passing `test_decklink_output` run left the
UltraStudio 4K Mini completely unresponsive (gone from Desktop Video, Media
Express, and Apple's own Thunderbolt System Report; no Thunderbolt power
passthrough). Isolated by hand to the unit itself, not the Mac/port/cable/
driver — see `DECISIONS.md` ADR-034 for the full incident record and
causation assessment. **Verification target changes to the UltraStudio
Monitor 3G** until the 4K Mini's status is resolved; no code change follows
(device selection was already generic, not 4K-Mini-specific — ADR-034). The
`bmdModePAL` mode-support finding above (ADR-033) was confirmed on the 4K
Mini specifically and is not yet re-confirmed on the Monitor 3G — first job
of the next real-terminal run.

**Monitor 3G run, this session, after the pivot:** full suite run against
the Monitor 3G — 15 of 16 passing. `test_decklink_output` passed both
checks again (`ctest -R test_decklink_output`, 5.13s), confirming
`bmdModePAL` + `bmdFormat10BitYUV` also works on the Monitor 3G, not just
the 4K Mini. The one failure, `test_decklink_device`'s
`test_at_least_one_device_is_full_duplex`, is expected and does not block
this line — the Monitor 3G is playback-only by design, so that check
correctly reports no duplex device found; see `DECISIONS.md` ADR-035, which
also records that this does not reopen WU-14 (`wu-14-green` stands). Steve
reported seeing "a circular zone plate" on the monitor — **not yet
confirmed whether the cylinder curvature itself was visible**, as distinct
from the zone plate pattern's own inherent concentric-ring shape, which is
what `Accept:` above actually requires; asked directly, Steve confirmed it
looked like a plain, undistorted zone plate, not a bent one.

**Investigation opened, not yet closed.** That answer is a real problem if
true of the actual SDI output, so before touching any code the file
`LoopedFramePlayback` actually reads was checked independently of hardware:
`writeWarpedTestFrame()`'s own steps (`makeZonePlate` -> `buildCylinderLattice`
with the test's exact `CylinderParams` -> `runFrameFile`) were reproduced
standalone (g++, no CMake, in the device bridge's own Linux VM, linking
only `scatter-core`'s files — no Blackmagic SDK involved at all) and both
the pre-warp and post-warp `.v210` files were dumped to 8-bit PGM/PNG for
direct visual inspection. Result: the post-warp file is **clearly, strongly
warped** — a vertically-elongated oval with black letterboxing on both
sides, not remotely a plain circular zone plate; the cylinder mechanism
(`buildCylinderLattice`/`runFrameFile`) is doing exactly what ADR-027
specifies. This rules out `runFrameFile`/`buildCylinderLattice` themselves
as the cause of what Steve saw — the file being handed to
`LoopedFramePlayback::create()` is unambiguously the warped one, not the
source. What is still open: whether that same warped content is what
actually left the Monitor 3G's SDI output, or whether Steve's own
description was a first-glance impression that didn't specifically register
the letterboxing/ellipse shape versus a genuinely wrong signal on the wire.
Steve was shown the actual before/after image and asked to look again,
specifically for black bars down both sides of the picture and an oval
(not round) ring shape, before this line's own by-eye `Accept:` clause is
either marked satisfied or treated as a real `decklink_output.cpp` bug.

**Investigation closed: false alarm, not a code defect.** Steve still
reported the plain zone plate after that, so the investigation went one
level deeper: temporary checksum instrumentation was added to
`decklink_output.cpp` (`startWith()`, `fillFrameBuffer()`, and
`ScheduledFrameCompleted()`, reading its own `completedFrame` argument back
through a fresh `StartAccess`/`GetBytes`) and run for real. Result: the
checksum was identical at every single stage — the file on disk, what
`fillFrameBuffer()` wrote into the DeckLink buffer, what read back
immediately after, and what the SDK itself reported back on the first three
actual `ScheduledFrameCompleted()` calls. Cross-checked further: the same
checksum, computed independently on a Linux-built, no-SDK reproduction of
the identical pipeline, matched byte-for-byte. That rules out this
project's own code as far as anything a user-space API call can observe.
Steve then asked to try a different shape/amount before accepting that
conclusion (fair, cheap to check) — `writeWarpedTestFrame()` was swapped to
a sphere warp (both axes, stronger than the cylinder) as a diagnostic,
confirmed correctly warped in the same off-hardware reproduction, and run
for real. Steve's own conclusion, once he looked again: both the cylinder
and sphere runs are actually fine — the picture he was seeing already had
the warp, but a 720x576 4:3-ish frame on his 16:9 monitor gets stretched
back out horizontally by the display's own aspect handling, which had made
the cylinder's own horizontal compression (baked into the pixel content)
look deceptively close to un-warped at a glance. See `DECISIONS.md`
ADR-036 for the full record. The diagnostic instrumentation and the sphere
swap were both reverted — `decklink_output.cpp`/`.hpp` and
`tests/test_decklink_output.cpp` are back to ADR-032's own cylinder design,
unchanged by this investigation, with a caution comment added against this
exact false alarm recurring. **`Accept:`'s by-eye clause is now
satisfied** — Steve confirmed the warped frame is visible (accounting for
his monitor's own 4:3-on-16:9 handling). Still needed before this line goes
`green`: the full suite and `./tools/close.sh 15a` at the real terminal, on
the now-reverted (clean, cylinder-only) build.

**Clean rebuild + full suite, on the reverted build:** run and confirmed
good by Steve. `Accept:`'s every clause is now satisfied: the mechanics
(zero dropped/late frames, clean stop — the original real-hardware run,
unaffected by this investigation, still stands as that evidence) and the
by-eye warp confirmation (this investigation, closed via ADR-036) are both
done. `./tools/close.sh 15a` and tagging `wu-15a-green` are Steve's own
next action, not run this session — per this project's own "the assistant
does not run `close.sh`" rule, this line stays `wip` until he reports that
back.

**`./tools/close.sh 15a`, run by Steve:** 15/16 — the same, single, already-
understood `test_decklink_device` duplex failure ADR-035 predicted, nothing
new. `close.sh` itself has no way to know that failure is an accepted
exception (its own gate is "any failure blocks tagging," which is the
right default behaviour for a script that shouldn't be hardcoding hardware-
availability judgment calls) — so it correctly refused to tag. **Steve
tagged `wu-15a-green` by hand**, accepting the ADR-035 exception himself
rather than waiting or extending `close.sh` — his own call to make, made.
This line is `green`.

### WU-15b — Scheduled playback endurance: one hour, no dropped frames `green`
The literal, still-unmet half of architecture.md 10 Phase 3's own accept
criterion ("stable for an hour"). Not new implementation — WU-15a's own
`LoopedFramePlayback` mechanism, unchanged, run for longer than one session
can itself assert green (`SESSION-PROTOCOL.md`'s own "one session, one work
unit" sizing). No `Files:` line: this is Steve's own hands-on verification
step (start WU-15a's own test, or a longer-duration invocation of the same
mechanism, leave it running unattended for an hour, report the logged
`stats().dropped`/`stats().displayedLate`/`stats().completed` counts back),
the same category of thing `HANDOFF.md`'s own "Environment check" section
already asks for by hand, not a session's own job to assert from a
terminal. See `DECISIONS.md` ADR-032.
**Accept:** one hour on a broadcast monitor, zero
`stats().dropped`/`stats().displayedLate` across the whole run.
*Done:* confirmed by Steve, real terminal, real hardware (UltraStudio
Monitor 3G), per `DECISIONS.md` ADR-038's own runbook (`tests/
test_decklink_output.cpp` line 168 hand-edited to `seconds(3600)`,
`caffeinate -s` + `nohup`/`disown`, AC power, lid open, per `HANDOFF.md`'s
own "What to run at your terminal"). One continuous hour of scheduled
playback: `completed=89998 displayedLate=0 dropped=0 flushed=0` — 89998
is consistent with 3600s x `bmdModePAL`'s own 25fps (3600 x 25 = 90000),
the couple-frame shortfall unremarkable preroll/stop-boundary slop, not a
dropped or late frame (both those counters are independently zero). Steve
confirmed by eye that the cylinder warp stayed visible for the whole run —
not the ADR-036 false-alarm plain-zone-plate look. See `CORRECTIONS.md`
C-014: the test's own completion log line printed "over a 5-second bounded
run" regardless — a separate hardcoded string ADR-038's own edit
instructions never touched, not evidence the run was actually short; the
arithmetic above and Steve's own direct confirmation are what settle it,
not that string. `tests/test_decklink_output.cpp`'s own temporary edit was
reverted (`git checkout --`) immediately after, per ADR-038 — the file is
back to exactly `wu-15a-green`'s own committed content; no code change
results from WU-15b. No new tag: WU-15b was never scoped with
`Files:`/`Accept:` source lines for `close.sh` to gate on (ADR-032), and
nothing here changes the buildable tree from `wu-15a-green`'s own state,
so there is nothing for `close.sh` to build/test/tag against. **Phase 3
(SDI output) is now done in full** — architecture.md 10's own "done when"
line ("warped frames appear on a broadcast monitor... stable for an hour")
is satisfied across WU-14, WU-15a and WU-15b together.

---

## Phase 4 — Threading and NEON

### WU-16a — Thread pool, QoS, per-worker bin arenas: PASS 2 (tile-parallel) `wip`
See `DECISIONS.md` ADR-040 for the full design and for why this splits
from the single bare WU-16 line above (this session's own first job, per
Steve's own brief: real scoping before any code — architecture.md
section 6 describes a fuller two-pass design than one line names, and the
full scope does not fit `SESSION-PROTOCOL.md`'s "3 source files" cap
without reopening `core/binner.hpp`/`.cpp`, WU-08's own frozen interface).
**Files:** `src/core/pipeline.hpp` (new), `src/core/pipeline.cpp`,
`src/core/resolve.hpp` (one new `PipelineParams::threads` field, default
1), `tests/test_threading.cpp` (new); plus `CMakeLists.txt`
(`find_package(Threads REQUIRED)`, `Threads::Threads` linked into
`scatter-core`, `test_threading` added — CMakeLists.txt edits have never
counted against the "3 source files" cap in any earlier unit either).
**Accept:** `runFrame()` at `PipelineParams::threads == 8` (and several
other thread counts, including values <= 1 and one larger than the
frame's own total tile count) produces output bit-identical, every
destination pixel's Y/Cb/Cr, to `threads == 1`, for a genuinely warped,
multi-tile frame whose destination extent is not an exact multiple of the
tile size — `tests/test_threading.cpp`'s
`test_threaded_pipeline_matches_single_threaded()` and its own second-
geometry sibling, checking WORK-UNITS.md's own literal "8-thread output
bit-identical to single-threaded (I6)" line directly. `ThreadPool`
(`core/pipeline.hpp`) itself is also checked directly, independent of
`runFrame()` — every worker index reached exactly once per `runOnAll()`
call, across repeated calls on the same pool, plus clean teardown.
PASS 1 (fragment generation, `core/binner.cpp`) is unchanged and always
single-threaded this unit — not in scope; see ADR-040 for the reasoning
and for WU-16b, below, which is.
*Status:* implemented and verified in a Linux cloud sandbox — Clang 18
and GCC 13, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all fifteen tests green — the fourteen carried over
unchanged plus `test_threading`, ~1.5 million checks, zero warnings under
the project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set), plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (clean) and, new for this unit, GCC 13 with `-fsanitize=thread`
(clean, no data race — the first genuinely concurrent code inside
`scatter-core`, so this was checked empirically rather than trusted on
reasoning alone, per this project's own C-011/C-012 lesson). **Still
needed before this line goes `green`:** `setWorkerQoS()`'s own
`#ifdef __APPLE__` branch (`pthread_set_qos_class_self_np`,
`<pthread/qos.h>`) is unverified — no AppleClang/Xcode toolchain exists in
the Linux cloud sandbox this unit was implemented in, the same gap
ADR-031/032 already named for WU-14/WU-15a's own Apple-only surfaces — so
the full suite and `./tools/close.sh 16a` still need a real-terminal run
before this can be tagged `wu-16a-green`.

### WU-16b — Thread pool: PASS 1 row-band parallelism, per-worker
generation-time bin arenas `todo`
Not scoped with `Files:`/`Accept:` lines yet — named here, per
`DECISIONS.md` ADR-040's own "not decided here, deliberately" note, for
whichever future session picks it up. architecture.md section 6's fuller
two-pass design: partition `core/binner.cpp`'s `generateFragments()` by
source row bands across `ThreadPool`'s own workers, each with a private,
preallocated per-tile fragment-bin arena (not the single shared
`TileBins` PASS 1 currently populates), then a barrier (`ThreadPool::
runOnAll()`'s own two-consecutive-calls shape already supports this, per
ADR-040), then PASS 2 (WU-16a, unchanged) reading every worker's own
generation-time arena for each tile it resolves. Needs a row-range-aware
addition to `core/binner.hpp`/`.cpp` that keeps the `v`-parameter
denominator keyed to the source raster's whole height, not a band's (see
ADR-040 for why the current `generateFragments()` cannot simply be called
once per band with a shorter `SourceRaster::height`) — whichever session
starts this should read `core/binner.cpp`'s own `pixelToLattice()` first,
the same "read the real shape before scoping" discipline ADR-031/032 used
for the DeckLink SDK, applied here to this project's own frozen code
instead of a third-party one.
### WU-17 — NEON v210 unpack and pack `todo`
**Accept:** bit-identical to scalar reference.
### WU-18 — NEON chroma resampling `todo`
### WU-19 — Real time at 576i25 `todo`

---

## Phase 5 — Live capture

### WU-20 — DeckLink input, format detection, ring buffer `todo`
Targets the **UltraStudio Recorder 3G** by name — see `DECISIONS.md`
ADR-039, which completes ADR-037's own third follow-up. Not otherwise
scoped: no `Files:`/`Accept:` lines yet — whichever session starts this
should read the real SDK's own `IDeckLinkInput`/capture-callback shape
first, per this project's own established practice for new hardware
surfaces (ADR-031/032's own reading-before-scoping discipline), rather
than assume from `docs/architecture.md`'s own Input subsection, which
still describes the original single-full-duplex-device (4K Mini) design
unrevised.
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
