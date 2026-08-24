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

### WU-16a — Thread pool, QoS, per-worker bin arenas: PASS 2 (tile-parallel) `green`
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
reasoning alone, per this project's own C-011/C-012 lesson).
*Done:* `wu-16a-green` tagged. Steve's own real-terminal run (M1 Max,
AppleClang) confirmed `setWorkerQoS()`'s `#ifdef __APPLE__` branch
compiles clean and the full suite passes 16/17 — the one failure,
`test_decklink_device`'s full-duplex check, is ADR-035's own already-
accepted exception (the UltraStudio Monitor 3G is playback-only),
unrelated to this unit. `./tools/close.sh 16a` correctly refused to tag
automatically (its own gate cannot distinguish an accepted exception from
a real failure, by design, ADR-035); Steve tagged `wu-16a-green` by hand,
the same way he did for `wu-15a-green`.

### WU-16b — Thread pool: PASS 1 row-band parallelism, per-worker
generation-time bin arenas `green` (`wu-16b-green`)
See `DECISIONS.md` ADR-041 for the full design. Completes architecture.md
section 6's full two-pass design that ADR-040 (WU-16a) deliberately
scoped down to PASS 2 alone.
**Files:** `src/core/binner.hpp` (one new `generateFragmentsRowRange()`
entry point; `generateFragments()` itself becomes a thin wrapper around
it, signature and behaviour unchanged), `src/core/binner.cpp`,
`src/core/pipeline.cpp` (PASS 1 now row-band-parallel when
`PipelineParams::threads > 1`; `resolveOneTile()` generalised to read a
list of per-worker PASS-1 arenas instead of one shared `TileBins`),
`tests/test_row_band.cpp` (new); plus `CMakeLists.txt`
(`test_row_band` added — CMakeLists.txt edits have never counted against
the "3 source files" cap in any earlier unit either). No
`src/core/pipeline.hpp` change — `ThreadPool`'s own two-consecutive-
`runOnAll()`-calls barrier, unused since WU-16a, is exactly what this
unit needed and already had.
**Accept:** `generateFragmentsRowRange()`, called once per row band
across several bands (including more bands than a source raster has
rows, so some bands are empty) and reassembled tile by tile, reproduces
a single whole-raster `generateFragments()` call exactly — same
fragments present, every field (position, colour, weight) bit-identical
— `tests/test_row_band.cpp`'s `checkRowRangeReassembly()` and its two
callers. `runFrame()` stays bit-identical to `threads == 1` (I6) now that
PASS 1 itself runs row-band-parallel too, including when `threads`
exceeds the source raster's own row count so some workers get an empty
band — `test_threaded_pipeline_more_workers_than_source_rows()`, plus
every pre-existing `tests/test_threading.cpp` check (unchanged, still
passing) now exercising the combined PASS-1+PASS-2-parallel path instead
of WU-16a's own PASS-2-only one.
*Status:* implemented and verified in a Linux cloud sandbox — Clang 18
and GCC 13, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all sixteen tests green — the fourteen carried over from
before WU-16a plus `test_threading` and the new `test_row_band`, zero
warnings under the project's full `-Wall -Wextra -Wpedantic -Wconversion
-Wsign-conversion -Werror` set), plus GCC 13 with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes (clean) and GCC 13 with `-fsanitize=thread` (clean, no data race,
both across the full suite and standalone against
`test_threading`/`test_row_band`) — checked with particular care since
this unit adds the project's first concurrent code inside PASS 1.
Unlike WU-16a, this unit adds no new Apple-only surface — `setWorkerQoS()`
is untouched.

**Status-line correction, WU-36 (this sweep):** this header was still
`wip`, saying a real-terminal run was still needed. `git tag` and `git log`
against the real repository confirm `wu-16b-green` exists, on commit
`5562cb9` ("WU-16b: record real-terminal confirmation (`test_decklink_
device` failure is ADR-035's known exception, not a regression)") — the
real-terminal confirmation this entry was waiting on already happened and
was simply never reflected in this line's own status word. Not touching
any of this unit's own source files.

### WU-17 — NEON v210 unpack and pack `green` (`wu-17-green`)
See `DECISIONS.md` ADR-042 for the full design and for this session's own
scoping work (this line was as bare going in as WU-16's own line was
before ADR-040 split it): the sandbox's own real cross-compile-and-run
verification capability, the new-sibling-function design, the group-level
vectorisation shape, and the CMake guards.
**Files:** `src/video/v210.hpp`, `src/video/v210.cpp` (both extended, not
new — `unpackRowNeon`/`packRowNeon`/`unpackImageNeon`/`packImageNeon`,
guarded by `#if defined(__ARM_NEON)`; the scalar `unpackRow`/`packRow`/
`unpackImage`/`packImage` are untouched), `tests/test_v210_neon.cpp` (new);
plus `CMakeLists.txt` (`test_v210_neon` added, gated on
`CMAKE_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$"` — CMakeLists.txt edits
have never counted against the "3 source files" cap in any earlier unit
either).
**Accept:** bit-identical to scalar reference. `tests/test_v210_neon.cpp`
diffs `unpackRowNeon`/`packRowNeon` against `unpackRow`/`packRow` directly
over random buffers at widths covering every residue mod 6 an even width
can take (both the full-group fast path and the short-final-group path,
ADR-018), including the full 10-bit domain (codes 0-3 and 1020-1023, not
just the legal range, so `packRowNeon`'s I2 clamp is checked at and around
its bounds); and `unpackImageNeon`/`packImageNeon` against
`unpackImage`/`packImage` over a whole 720x576 frame, byte-identical packed
output and sample-identical planar output, plus NEON's own round trip.
*Status:* implemented and verified in this session's own Linux cloud
sandbox — not only compiled but genuinely **executed** as real AArch64
machine code, via cross-compilation (GCC 13.3.0 and Clang 18.1.3) and
`qemu-aarch64-static`, a materially stronger verification than ADR-031/032's
DeckLink units could get (no SDK there at all; here, a real, if emulated,
ARM64 CPU). Default x86_64 matrix (Clang 18/GCC 13, Release/Debug, tile
4/5, plus GCC 13 ASan+UBSan) confirms this unit leaves the existing sandbox
build completely unaffected — sixteen tests green, `test_v210_neon`
correctly absent per its own CMake gate. AArch64 cross-compile +
`qemu-aarch64-static` execution: Release/Debug, tile 4/5, plus GCC 13
UBSan — seventeen tests green in every configuration, `test_v210_neon`
itself 53 checks, zero warnings under this project's full `-Wall -Wextra
-Wpedantic -Wconversion -Wsign-conversion -Werror` set on both GCC and
Clang. ASan specifically crashes `qemu-aarch64-static` itself (a known
sandbox/emulator limitation, not a code defect — see ADR-042) and remains
untried on any platform — not required by this unit's own accept criterion.
*Done:* confirmed at the real terminal, M1 Max, AppleClang: full suite 18
of 19 passing, `test_v210_neon` itself green (0.01s) — AppleClang compiled
this unit clean on the first attempt, no cloud/AppleClang divergence
(unlike WU-11's own C-012). The one failure, `test_decklink_device`'s
full-duplex check, is ADR-035's own already-accepted exception (the
UltraStudio Monitor 3G is playback-only), unrelated to this unit.
`./tools/close.sh 17` correctly refused to tag automatically (its own gate
cannot distinguish an accepted exception from a real failure, by design);
Steve was given the `git tag -a wu-17-green ...` command to run by hand, the
same way he did for `wu-15a-green`/`wu-16a-green`/`wu-16b-green`. **Doc-only
correction, this session, while re-verifying repository state before
starting WU-21d:** no `wu-17-green` tag exists in the real repository as of
this writing, confirmed directly via `git tag` — this line's own prior text
("Steve tagged `wu-17-green` by hand") overstated what had actually
happened, the same kind of doc/reality gap `HANDOFF.md`'s own Session-37
account (C-021) and this session's own WU-21i correction above both name.
The header line above (`pending Steve's manual tag`) already had this right;
only this paragraph's own body text was wrong. The code itself is genuinely
done and verified per this entry's own text above (full suite green at
Steve's own real terminal, `test_v210_neon` passing) — only the tag itself is
missing. Not touching any source file; see `HANDOFF.md` for the exact
command to close this out for real.

**Further correction, WU-36 (this sweep):** `wu-17-green` now exists —
confirmed via `git tag`/`git log` against the real repository, on commit
`bba3634` ("WU-21d: cold-start black fill for `LiveFramePlayback`'s own
pool") — landed later than this unit's own work, the same "tag lands on
whatever `HEAD` is at close time" pattern WU-28b's own entry already
documents. Header updated above from "pending Steve's manual tag" to plain
`green`. Not touching any source file.
### WU-18 — NEON chroma resampling `green` (`wu-18-green`)
See `DECISIONS.md` ADR-043 for the full design and for this session's own
scoping work (this line was barer going in than WU-17's own line was
before ADR-042 — WU-17 at least already had "bit-identical to scalar
reference" written down): the module-layout question checked directly
(`chroma.hpp/.cpp`'s own architecture.md line lacks v210's "+ NEON" suffix,
resolved via `chroma.hpp`'s own WU-04-era comment plus architecture.md's
Phase 4 "done when" line, both already naming chroma alongside v210 for a
NEON path), the interior/edge vectorisation shape (a sliding-window
boundary clamp, not v210's fixed bit-interleave), and reuse of ADR-042's
sandbox verification capability and CMake guard unchanged.
**Files:** `src/video/chroma.hpp`, `src/video/chroma.cpp` (both extended,
not new — `upsampleRowNeon`/`downsampleRowNeon`/`upsampleImageNeon`/
`downsampleImageNeon`, guarded by `#if defined(__ARM_NEON)`; the scalar
`upsampleRow`/`downsampleRow`/`upsampleImage`/`downsampleImage` are
untouched), `tests/test_chroma_neon.cpp` (new); plus `CMakeLists.txt`
(`test_chroma_neon` added to the same `CMAKE_SYSTEM_PROCESSOR` guard block
`test_v210_neon` already uses).
**Accept:** bit-identical to scalar reference. `tests/test_chroma_neon.cpp`
diffs `upsampleRowNeon`/`downsampleRowNeon` against `upsampleRow`/
`downsampleRow` directly over random full-16-bit-domain buffers at widths
spanning `chromaWidth(width)` from 1 through 14 (covering zero interior
batches, exactly one, and the transition into two, for both filters' own
differently-sized interior margins) plus 720/1920, the two real widths —
never a round trip through both filters, which C-006 already established
is not bit-exact for non-flat content; and `upsampleImageNeon`/
`downsampleImageNeon` against `upsampleImage`/`downsampleImage` over a
whole 720x576 frame.
*Status:* implemented and verified in this session's own Linux cloud
sandbox — not only compiled but genuinely **executed** as real AArch64
machine code, via cross-compilation (GCC 13.3.0 and Clang 18.1.3) and
`qemu-aarch64-static`, reusing ADR-042's own established capability
directly. Default x86_64 matrix (Clang 18/GCC 13, Release/Debug, tile 4/5,
plus GCC 13 ASan+UBSan) confirms this unit leaves the existing sandbox
build completely unaffected — sixteen tests green, `test_chroma_neon`
correctly absent per its own CMake gate (shared with `test_v210_neon`).
AArch64 cross-compile + `qemu-aarch64-static` execution: Release/Debug,
tile 4/5, plus GCC 13 UBSan — eighteen tests green in every configuration
(the sixteen carried over, `test_v210_neon` and the new `test_chroma_neon`
— 34 checks, zero warnings under this project's full `-Wall -Wextra
-Wpedantic -Wconversion -Wsign-conversion -Werror` set on both GCC and
Clang). This session's own first draft hit the identical most-vexing-parse
mistake `HANDOFF.md` recorded for WU-17's own first draft of
`test_v210_neon.cpp` — caught by the aarch64 cross-compile specifically
(the x86_64 matrix never compiles this file at all, per its own CMake
gate), fixed the same way, before any claim was made based on the broken
draft; see ADR-043's own "genuine bug" section. ASan on AArch64
reproducibly crashes `qemu-aarch64-static` itself, the same sandbox/
emulator limitation ADR-042 already named for WU-17, not a code defect,
and remains untried on any platform — not required by this unit's own
accept criterion.
*Done:* confirmed at the real terminal, M1 Max, AppleClang: full suite 19
of 20 passing, `test_chroma_neon` itself green (0.14s, then 0.00s on the
`close.sh` rebuild) — AppleClang compiled this unit clean on the first
attempt, no cloud/AppleClang divergence (unlike WU-11's own C-012, and
matching WU-17's own precedent). The one failure, `test_decklink_device`'s
full-duplex check, is ADR-035's own already-accepted exception (the
UltraStudio Monitor 3G is playback-only), unrelated to this unit —
`test_decklink_output` itself passed (5.28s/5.16s), confirming the
DeckLink-side mechanics this unit doesn't touch are still fine.
`./tools/close.sh 18` correctly refused to tag automatically (its own gate
cannot distinguish an accepted exception from a real failure, by design);
Steve was given the `git tag -a wu-18-green ...` command to run by hand,
the same way he did for
`wu-15a-green`/`wu-16a-green`/`wu-16b-green`/`wu-17-green`.

**Status-line correction, WU-36 (this sweep):** `git tag` against the real
repository confirms `wu-18-green` exists (commit `4ea23b8`, "WU-18: NEON
chroma resampling, verified via aarch64 cross-compile + qemu in the cloud
sandbox"). Header updated above from "pending Steve's manual tag" to plain
`green`. Not touching any source file.

Once tagged, **Phase 4 (Threading and NEON) is done in full** — thread pool/QoS/
per-worker bin arenas (WU-16a/16b) and both NEON units (WU-17 v210, WU-18
chroma) all green; WU-19 ("Real time at 576i25") is next and is the
phase's only remaining unit, the first whose own job is throughput rather
than correctness. (This session's own "once tagged, Phase 4 is done in
full" was premature, not wrong — WU-19 had not yet been scoped, and real
scoping the next session found it split into WU-19a/19b, below; Phase 4
did not actually finish until both closed out. Same doc-sync slip WU-03's
own status had at WU-04, corrected in place rather than erased.)
### WU-19a — Persistent, caller-owned ThreadPool `green` (`wu-19a-green`)
See `DECISIONS.md` ADR-044 for the full design and for why this splits from
the single bare WU-19 line above (this session's own first job, per Steve's
own brief: real scoping before any code — architecture.md 10's own Phase 4
"done when" line, "8-thread output is bit-identical to single-threaded, at
frame rate," is partly a correctness statement this sandbox can check in
full and partly a real-hardware timing claim it cannot produce evidence
about at all — see ADR-044's own opening section). Completes ADR-040's own
explicit deferral: "a persistent, caller-owned `ThreadPool` that `runFrame()`
can reuse across many calls instead of constructing one per call... WU-19's
own job."
**Files:** `src/core/resolve.hpp` (one new `PipelineParams::pool` field, a
non-owning `ThreadPool*`, default `nullptr`), `src/core/pipeline.cpp` (the
threaded PASS-1/PASS-2 body factored into a `runThreaded(..., ThreadPool&,
...)` helper, called against either a fresh per-call pool — WU-16a/16b's own
unchanged behaviour — or the caller's own persistent one), `tests/
test_persistent_pool.cpp` (new); plus `CMakeLists.txt` (`test_persistent_pool`
added, same `scatter_test()` pattern as `test_threading`/`test_row_band` —
CMakeLists.txt edits have never counted against the "3 source files" cap in
any earlier unit either). No `src/core/pipeline.hpp` change — `ThreadPool`'s
own existing public interface (`size()`, `runOnAll()`) already has
everything a caller needs.
**Accept:** a `ThreadPool` constructed once, outside `runFrame()`, and
reused across many calls — including calls against different frame
geometries in sequence, and calls whose `PipelineParams::threads` field
deliberately disagrees with the pool's own `size()` — produces output
bit-identical to the `PipelineParams::threads <= 1`, `pool == nullptr`
oracle on every call, not only the first; the existing per-call-construction
threaded path (`pool == nullptr`, `threads > 1`) is unchanged, verified by
the full pre-existing suite passing unmoved; pool reuse across many calls is
itself clean, no hang, no leak. Does **not** include, and does not claim,
any statement about whether `runFrame()`/`runFrameFile()` actually completes
within 576i25's own frame budget on real hardware — that is WU-19b, below,
unscoped and unbuilt this session, deliberately.
*Status:* implemented and verified in a Linux cloud sandbox — Clang 18 and
GCC 13, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight configurations,
all seventeen tests green — the sixteen carried over unchanged plus
`test_persistent_pool`, ~2.56 million checks, zero warnings under the
project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set), plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes (clean) and GCC 13 with
`-fsanitize=thread` at both tile sizes (clean, no data race — checked with
particular care, since this is the first unit to let a `ThreadPool` outlive
a single `runFrame()` call). This unit touches no Apple-only surface at all
— unlike WU-14/WU-15a/WU-17/WU-18, there is no piece of it this sandbox
could not already fully verify.

**Status-line correction, WU-36 (this sweep):** this line still said
real-terminal confirmation was pending. `git tag` against the real
repository confirms `wu-19a-green` exists (commit `4886f81`, "WU-19a:
persistent, caller-owned ThreadPool (ADR-044), verified in the cloud
sandbox") — that confirmation already happened and was never reflected in
this line's own status word. Header updated above from `wip` to `green`.
Not touching any source file.

### WU-19b — Real-time measurement at 576i25 on the M1 Max `green`
The literal, still-unmet half of architecture.md 10's own Phase 4 accept
criterion ("at frame rate") — not implementation work, no new
`Files:`/`Accept:` source-file lines, the same category WU-15b (ADR-032) was
for its own hour-long endurance run this project's own sandbox could not
produce evidence about either. Using WU-19a's own `PipelineParams::pool`
mechanism, time `runFrame()`/`runFrameFile()` at 576i25 (720x576) with a
real, persistent `ThreadPool` at a real worker count on the real M1 Max —
a simple `std::chrono` wrap at Steve's own terminal is enough, no committed
benchmarking tool (see `DECISIONS.md` ADR-044 for why one was considered and
rejected this session) — and confirm per-frame wall-clock time stays under
576i25's own 40ms budget (25 fps).
**Accept:** per-frame wall-clock time, averaged over 200 iterations after a
20-iteration warmup, against a genuinely warped (cylinder-over-zone-plate,
the same construction `tests/test_threading.cpp`/`test_persistent_pool.cpp`
use) 720x576 frame, using WU-19a's own `PipelineParams::pool`, stays under
40.000ms at some worker count `docs/architecture.md` section 6 actually
names ("8 worker threads").
*Done:* confirmed by Steve, real terminal, real hardware (M1 Max,
AppleClang, `-O3 -mcpu=apple-m1`, linked directly against
`libscatter-core.a` built at both candidate tile sizes):

| threads | tile 2^5 (32x32) ms/frame | tile 2^4 (16x16) ms/frame |
|---|---|---|
| 1 | 48.766 (over budget) | 50.269 (over budget) |
| 2 | 24.665 (OK) | 25.710 (OK) |
| 4 | 13.101 (OK) | 14.349 (OK) |
| 8 | 6.868 (OK, 5.8x headroom) | 7.528 (OK, 5.3x headroom) |

architecture.md 10's own Phase 4 "done when" line — "8-thread output is
bit-identical to single-threaded, at frame rate" — is now satisfied in
full: the bit-identical half by WU-16a/16b/19a (I6, `--threads 1` vs `--threads
N`, checked exhaustively in-suite), the frame-rate half by this measurement,
at architecture.md's own named 8-worker configuration, with a wide margin
rather than a narrow squeak. **Phase 4 (Threading and NEON) is now done in
full.**

**Q1 (tile size) settled.** Both candidate tile sizes were measured this
round, not just the default — tile 2^5 (32x32) is faster than tile 2^4
(16x16) at every one of the four thread counts, consistently by roughly
8-10%, not a one-off at a single point. This is the real, whole-pipeline,
real-M1-Max evidence architecture.md 4.4's own "Benchmark both" (and this
project's own `CMakeLists.txt` comment, "WU-09 benchmarks both. Do not
resolve Q1 before then") has been waiting on since WU-09, five sessions
before this project even had a working threaded pipeline to measure the
question honestly against. See `DECISIONS.md` ADR-045: `SCATTER_TILE_LOG2=5`
(32x32), already this project's own default, is confirmed and settled as
the project's tile size going forward, not merely left as a default nobody
had checked.

WU-17's own deferred denser `vld4q_u32` v210 scheme and WU-18's own
deferred `downsampleRowNeon` load-count reduction: with 5.3-5.8x headroom
at the architecture's own named worker count, at both tile sizes, there is
no evidence either is a bottleneck worth chasing — both stay exactly as
deferred as ADR-042/043 already left them, not reopened by this result, and
not scheduled.

---

## Phase 5 — Live capture

### WU-20a — Ring buffer: portable SPSC handle queue `green` (`wu-20a-green`)
See `DECISIONS.md` ADR-046 for the full design and for why this splits from
the single bare WU-20 line above (this session's own first job, per this
project's own established practice for new hardware surfaces: read the real
SDK's own `IDeckLinkInput`/`IDeckLinkInputCallback`/`IDeckLinkVideoInputFrame`
shape and the SDK's own three real capture samples — `CaptureStills`,
`InputLoopThrough`, `CapturePreview` — before scoping, the same
reading-before-scoping discipline ADR-031/032 established for WU-14/WU-15a).
That reading found architecture.md 6's own capture-callback-thread
requirement ("never blocks, never allocates") is stricter than any real
sample's own frame-handoff mechanism provides — none of the three samples
implements a lock-free ring at all — so the ring buffer is this project's
own design, not adapted from an SDK idiom, and it is the one piece of
WU-20's three named pieces (format-detection-aware `EnableVideoInput`, a
capture callback implementing `IDeckLinkInputCallback`, a ring buffer) with
zero DeckLink/platform dependency — ordinary portable C++20, buildable and
genuinely runnable, including under ThreadSanitizer with a real concurrent
producer and consumer, in this project's own Linux cloud sandbox, unlike the
other two. See ADR-046 for the full split reasoning and for WU-20b's own
scope sketch, below.
**Files:** `src/core/ring_buffer.hpp` (new), `tests/test_ring_buffer.cpp`
(new); plus `CMakeLists.txt` (`test_ring_buffer` added, same
`scatter_test()` pattern as `test_threading`/`test_persistent_pool` —
CMakeLists.txt edits have never counted against the "3 source files" cap in
any earlier unit either).
**Accept:** a fixed-capacity, single-producer/single-consumer,
allocation-free `RingBuffer<T, Capacity>` preserves FIFO order across
push/pop; a full ring drops rather than blocks, incrementing
`droppedCount()`, without corrupting or losing any already-accepted entry;
`capacity()` reports the usable slot count (`Capacity`, not the
`Capacity + 1` backing storage — the always-one-slot-empty technique);
10000 push/pop cycles against a 4-slot ring wrap the internal indices
around the backing array's own bound many times over with no leaked or
duplicated instance (checked via an instrumented move-only type's own
live/constructed counters); and, the accept criterion this unit cares
about most, a genuine two-`std::thread` single-producer/single-consumer
run of 200000 items — deliberately far more than the ring's own 16-slot
capacity — loses no item, preserves arrival order, and shows no data race
under ThreadSanitizer, checked empirically rather than trusted on
reasoning alone (the same "check concurrency empirically, not just by
inspection" standard WU-16a/ADR-040 established for this project's first
concurrent code). No DeckLink dependency anywhere in this unit's own
files — `EnableVideoInput()`/`IDeckLinkInputCallback`/format detection are
WU-20b's own job, not built this session.
*Status:* implemented and verified in a Linux cloud sandbox — GCC 13 and
Clang 18, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5 (eight
configurations, all eighteen tests green — the seventeen carried over
unchanged plus `test_ring_buffer`, 20036 checks, zero warnings under
the project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set), plus GCC 13 and Clang 18 both with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes
(clean, no ASan or UBSan report) and, the accept criterion this unit cares
about most, GCC 13 **and** Clang 18 both with `-fsanitize=thread` at both
tile sizes (clean, no data race reported against the real concurrent
producer/consumer test) — the first unit in this project to verify a
concurrent data structure under TSan with two different compilers' own
sanitizer runtimes, not just GCC's (Clang's `libclang_rt.tsan`/`.asan`
needed a manual `apt-get download`/`dpkg -i` workaround for an unrelated
broken transitive dependency in the sandbox's own package mirror; the
target runtime libraries themselves unpacked and linked cleanly — not a
project code issue, not logged in `CORRECTIONS.md`). This unit touches no
Apple-only or DeckLink-only surface at all — unlike WU-14/WU-15a/WU-17/
WU-18/WU-20b, there is no piece of it this sandbox could not already fully
verify. Confirmed `green`, tagged `wu-20a-green` (verified directly against
the real repository's own `git tag`/`git describe` this session — see
`HANDOFF.md`).

### WU-20b — DeckLink capture: format-detection-aware `EnableVideoInput`,
`IDeckLinkInputCallback` implementation `green` (`wu-20b-green`)
See `DECISIONS.md` ADR-047 for the full design and for why this session
built WU-20b as one unit rather than splitting it further (this session's
own first job, per this project's established practice for DeckLink work:
re-read the real SDK's `IDeckLinkInput`/`IDeckLinkInputCallback`/
`IDeckLinkVideoInputFrame` shape and the three real capture samples again,
not just ADR-046's own summary of last session's reading, before finalizing
scope). Targets the **UltraStudio Recorder 3G** by name — see `DECISIONS.md`
ADR-039. Mirrors `io/decklink_output.hpp`'s own `LoopedFramePlayback` shape
(ADR-032): implements `IDeckLinkInputCallback` directly, real `IUnknown`
refcounting, `ComPtr::adopt()` on `create()`.
**Files:** `src/io/decklink_input.hpp`, `src/io/decklink_input.cpp` (both
new), `tests/test_decklink_input.cpp` (new); plus `CMakeLists.txt`
(`decklink_input.cpp` added to the existing `scatter-decklink` target,
`test_decklink_input` added alongside `test_decklink_device`/
`test_decklink_output` — CMakeLists.txt edits have never counted against the
"3 source files" cap in any earlier unit either).
**Accept:** `CaptureSource::create()` against a real device that both
supports capture and reports `BMDDeckLinkSupportsInputFormatDetection`
succeeds (a device without format-detection support is out of scope for
this unit and `create()` is designed to fail on one, not proceed silently);
over a bounded several-second run, with the UltraStudio Monitor 3G's own SDI
output physically patched into the Recorder 3G's SDI input (a genuine
loopback signal — both devices are already this project's real target
hardware, ADR-037, so this needs no third piece of equipment), arrived
frames are retained correctly (no use-after-release, no leaked reference —
architecture.md 12's own "reference-count leaks lock the device" risk) and
pushed into the ring without loss under normal-rate arrival, with
`stats().framesArrived` nonzero and the unconditional accounting invariant
`framesPushed + ring.droppedCount() <= framesArrived` holding throughout;
`stop()` returns cleanly. Without the loopback connected, `create()`/`stop()`
still must run cleanly and the accounting invariant still must hold — the
same "the mechanics are what a session's own automated checks gate on, a
human (here, a cable) supplies the rest" division of labour WU-15a's own
`Accept:` already used for its own by-eye clause. Does not include, and does
not claim, anything about `VideoInputFormatChanged()`'s own real-mode-change
behaviour with a live changing source, or reading pixel bytes out of a
retained frame — that is WU-21's job ("Full loop through at 576i25"), not
this one's.
*Status:* implemented in a Linux cloud sandbox with no Blackmagic SDK and no
AppleClang/Xcode toolchain at all, the same shape ADR-031/032/046 already
used for WU-14/WU-15a/WU-20a's own DeckLink-dependent half — reasoned
through against the real SDK headers and the three real capture samples
re-read this session. Steve's own real-terminal build caught one genuine
first-compile defect this sandbox could not (`-Wsign-conversion` on
`GetFlags() & bmdFrameHasNoInputSource`, `_BMDFrameFlags` to `BMDFrameFlags`
— see `CORRECTIONS.md` C-016, both for the fix and for a session-mechanics
error in how it was first delivered); fixed, confirmed written to the real
repository via the device bridge and re-read from there. With the
UltraStudio Monitor 3G's own SDI output physically patched into the
Recorder 3G's input, `cmake --build build` is now clean and
`ctest --test-dir build --output-on-failure` shows `test_decklink_input`
passing alongside the rest of the suite (`test_decklink_device`'s
`foundDuplexDevice` failure is ADR-035's own already-accepted exception,
unrelated). Confirmed `green`, tagged `wu-20b-green` (verified directly
against the real repository's own `git tag`/`git describe` this session —
see `HANDOFF.md`) — this session's own sandbox verified none of it by
compiling or running it; Steve's real terminal verified the build and the
test, and has since tagged it.
### WU-21a — `runFrameBytes()`: the in-memory sibling of `runFrame()`/
`runFrameFile()` `green` (`wu-21a-green`)
See `DECISIONS.md` ADR-048 for the full design and for why this session split
WU-21 into three pieces (a/b/c) rather than building "full loop through at
576i25" as one unit — the same "portable piece now, DeckLink-specific piece
next" shape ADR-046 already used to split WU-20a/WU-20b, applied here because
WU-21's own first job (drain WU-20b's own `CaptureFrameRing`,
`src/io/decklink_input.hpp`, on a consumer thread, and read real pixel bytes
out of a retained `IDeckLinkVideoInputFrame` for the first time anywhere in
this project) genuinely splits into a portable byte-conversion half this
sandbox can build and verify for real, and two DeckLink-specific halves
(frame-byte extraction/ring-drain; continuous SDI re-output scheduling) it
cannot. This session's own first job, per established practice: re-read the
real SDK's `IDeckLinkVideoInputFrame`/`IDeckLinkVideoBuffer`/`StartAccess`/
`EndAccess`/`GetBytes()` shape and the three real capture samples again (not
just ADR-047's own summary), confirming none of the three samples reads
pixel bytes via `IDeckLinkVideoBuffer` — this is genuinely new ground for the
project. Mirrors `io/decklink_output.hpp`'s own `LoopedFramePlayback::
fillFrameBuffer()` write-direction `StartAccess`/`GetBytes`/`EndAccess`
pattern (WU-15a, ADR-032), the one this unit's own eventual WU-21b half will
extend to the read direction.
**Files:** `src/core/resolve.hpp`, `src/core/pipeline.cpp` (both extended:
new `runFrameBytes()`, declared between `runFrame()` and `runFrameFile()`),
`tests/test_pipeline_bytes.cpp` (new); plus `CMakeLists.txt`
(`test_pipeline_bytes` registered via the existing `scatter_test()` function,
alongside `test_zoneplate`/`test_threading` — CMakeLists.txt edits have never
counted against the "3 source files" cap in any earlier unit either).
**Accept:** two checks, both run for real in this sandbox, not reasoned
through: (1) `runFrameBytes()` and `runFrameFile()` produce byte-identical
output for the same lattice/source/params, checked exactly against a genuine
off-centre affine warp (0.7x compression over a zone plate) — not the
identity map's degenerate one-fragment-per-cell case; (2) `runFrameBytes()`
itself satisfies I7 (identity map round-trips bit-exactly) directly, against
flat-chroma content (`testpat::makeZonePlate()`, not a ramp — see
`CORRECTIONS.md` C-006), the same foundational property
`tests/test_zoneplate.cpp`'s own `testI7Pattern()` already established for
`runFrameFile()`. Deliberately does not refactor `runFrameFile()` to call
`runFrameBytes()` internally (see ADR-048 for the reasoning) and does not
include, and does not claim, anything about draining `CaptureFrameRing`,
reading bytes out of a retained `IDeckLinkVideoInputFrame`, or continuous SDI
output — those are WU-21b's and WU-21c's own jobs, not this one's.
*Status:* implemented and verified for real in this project's own Linux
cloud sandbox — unlike WU-14/WU-15a/WU-20b, this unit touches no DeckLink or
Apple-only surface at all, so there is no piece of it this sandbox could not
already fully verify. GCC 13.3.0 and Clang 18.1.3, Release and Debug, at
`SCATTER_TILE_LOG2` 4 and 5 (8 configurations total), all 19 project tests
green; plus GCC 13 and Clang 18 both with
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile sizes,
clean. TSan not run — this unit introduces no new concurrency. One test bug
caught and fixed within this same session, before any claim was made (not
logged in `CORRECTIONS.md`, matching the C-015/ADR-043 precedent for routine
iteration, not a design/reasoning error — see ADR-048): the I7 check first
used `testpat::makeRamp()` and failed, because a ramp's own non-flat chroma
does not survive `chroma::upsampleImage()`/`downsampleImage()` unchanged
regardless of the lattice (C-006); fixed by switching to
`testpat::makeZonePlate()`. Confirmed at Steve's own real terminal:
`cmake --build build` clean, `ctest --test-dir build --output-on-failure`
shows `test_pipeline_bytes` passing alongside the rest of the suite
(`test_decklink_device`'s `foundDuplexDevice` failure is ADR-035's own
already-accepted exception, unrelated). `./tools/close.sh 21a` itself refused
to tag, exactly as expected, because it has no knowledge of ADR-035 and saw
that one failing test — the manual `git tag -a wu-21a-green ...` step every
unit since ADR-035 has actually used was run instead. Confirmed `green`,
tagged `wu-21a-green` (verified directly against the real repository's own
`git tag` — see `HANDOFF.md`).

### WU-21b — DeckLink capture-side pixel read: `CaptureConsumer` drains
`CaptureFrameRing` on a consumer thread, obtains `IDeckLinkVideoBuffer` via
`QueryInterface`, brackets `GetBytes()` with `StartAccess`/`EndAccess`, and
feeds the mapped bytes into WU-21a's own `runFrameBytes()` `green`
(`wu-21b-green`)
See `DECISIONS.md` ADR-049 for the full design and for the four questions
ADR-048 explicitly left open for this unit, all decided there:
consumer-thread ownership/lifetime is fully independent of `CaptureSource`'s
own; extracted bytes are handed to `runFrameBytes()` synchronously, inside
the `StartAccess`/`EndAccess` bracket, never copied into a pool buffer; the
ring's own drain policy while empty is a short (1ms) poll sleep, `RingBuffer`
having no blocking pop; a captured frame's own reported
`GetWidth()`/`GetHeight()`/`GetRowBytes()` are trusted directly, per frame,
never checked against the display mode `CaptureSource::create()` was given.
This session's own first job, per established practice for a new hardware
surface: re-read the real SDK's `IDeckLinkVideoBuffer`/`IDeckLinkVideoFrame`/
`IDeckLinkVideoInputFrame` shape directly again (not just ADR-048's own
summary), plus `io/decklink_output.cpp`'s own `fillFrameBuffer()` (the
write-direction pattern this unit mirrors), `io/decklink_input.hpp`/`.cpp`
(WU-20b, the ring this unit drains), `core/ring_buffer.hpp` (WU-20a),
`core/resolve.hpp`'s `runFrameBytes()`/`PipelineParams` (WU-21a), and
`docs/architecture.md` sections 3, 6, 7, 9, 12 plus ADR-037/039.
**Files:** `src/io/decklink_capture_consumer.hpp`, `src/io/
decklink_capture_consumer.cpp` (both new: `CaptureConsumerStats`,
`CaptureConsumer`), `tests/test_decklink_capture_consumer.cpp` (new); plus
`CMakeLists.txt` (`decklink_capture_consumer.cpp` added to the existing
`scatter-decklink` library's source list; new
`target_link_libraries(scatter-decklink PRIVATE scatter-core)`, the first
time that library has needed one; new `test_decklink_capture_consumer`
executable registered alongside `test_decklink_device`/`test_decklink_output`/
`test_decklink_input`, linking both `scatter-decklink` and `scatter-core` —
CMakeLists.txt edits have never counted against the "3 source files" cap in
any earlier unit either).
**Accept:** hardware-gated, same shape as `test_decklink_input.cpp`'s own —
not run in this session's own Linux cloud sandbox, which has neither the
Blackmagic SDK nor an AppleClang/Xcode toolchain, and not yet run at Steve's
own real terminal either. At the real terminal, with the Monitor 3G →
Recorder 3G SDI loopback connected (`test_decklink_input.cpp`'s own
documented setup): `CaptureSource::create()` and `CaptureConsumer::start()`/
`stop()` run cleanly for a bounded 5-second window; the accounting invariants
`framesProcessed + framesFailed == framesPopped` and `framesPopped <=`
capture's own `framesPushed` hold; and — only if the loopback is actually
connected — `framesProcessed` is nonzero and `copyLatestFrame()` returns a
buffer of exactly `v210::rowBytesMin(kWidth) * kHeight` bytes. With nothing
physically connected, the same mechanics (clean start/stop, the two
accounting invariants) are still real Accept criteria and the test warns
rather than fails on zero frames processed, the same "nothing plugged in
right now is a real, honestly reportable state, not a defect" convention
`test_decklink_input.cpp` already established. Deliberately reuses an
identity-map lattice, not a genuine warp — proving the
`StartAccess`/`GetBytes`/`runFrameBytes`/`EndAccess` mechanics against real
captured bytes is this unit's own job; `runFrameBytes()`'s own warp
correctness is already genuinely verified in the cloud sandbox at WU-21a and
this unit does not re-prove it. Does not include, and does not claim,
anything about rescheduling the produced bytes onto `IDeckLinkOutput` — that
is WU-21c's own job, not this one's.
*Status:* delivered as reasoned-through-only, same shape as WU-14/WU-15a/
WU-20b before it — drafted and written via the device bridge to the real
repository, re-read back from there to confirm each write landed correctly
(`SESSION-PROTOCOL.md` anti-drift rule 8) — then genuinely built, tested,
and run at Steve's own real terminal within this same session:
`cmake --build build` clean (the SDK found, `scatter-decklink` and every
DeckLink-gated target configured), `ctest --test-dir build
--output-on-failure` showing 24 of 25 tests passing (the sole failure the
already-accepted `test_decklink_device`/`foundDuplexDevice` exception,
ADR-035, unrelated), and `./build/test_decklink_capture_consumer` run
directly against the real Monitor 3G → Recorder 3G SDI loopback:
`framesArrived=123 framesPushed=89 | framesPopped=81 framesProcessed=81
framesFailed=0` over its own bounded 5-second window, all 7 automated checks
passing, including the `copyLatestFrame()` size check (only reached because
`framesProcessed > 0`) — genuine, real-signal confirmation that a captured
frame's own pixel bytes can be read through `IDeckLinkVideoBuffer` and fed
into `runFrameBytes()` end to end, not merely reasoned through. See
`DECISIONS.md` ADR-049's own real-hardware verification addendum for the
full numbers and the new `kCaptureRingCapacity` data point they surface
(123 arrived vs. 89 pushed — a real, now-measured gap, not conclusively
diagnosed this session). Confirmed `green`, tagged `wu-21b-green`
(confirmed present on `git tag` at Steve's own real terminal).

### WU-21c — Continuous SDI re-output: `LiveFramePlayback` schedules a
live-produced frame stream (WU-21b's own `CaptureConsumer::copyLatestFrame()`)
onto `IDeckLinkOutput` — a fixed pool of frame buffers, round-robin refilled
and rescheduled exactly once per completion, in place of `LoopedFramePlayback`'s
own single-static-buffer design `green` (`wu-21c-green`)
See `DECISIONS.md` ADR-050 for the full design: the pool-sizing reasoning
(exactly `round(frameRate / 2)` buffers, ADR-032's own preroll convention
reused rather than invented fresh), the round-robin refill policy (relying on
the SDK's own FIFO completion guarantee, no separate in-flight bookkeeping),
and — this unit's own concern for the first time, ADR-037's second follow-up,
open since it was first named — the no-explicit-synchronisation
genlock/clock-domain decision: every refill uses whatever
`CaptureConsumer::copyLatestFrame()` returns right now, with no timestamp
alignment against the output's own fixed schedule, so a capture/process rate
mismatch shows as repeated frames (`framesRepeated()`, this unit's own new
counter) or silently superseded ones, never a growing backlog. This session's
own first job, per established practice for a new hardware surface: re-read
the real SDK's `IDeckLinkOutput`/`IDeckLinkVideoOutputCallback` shape directly
again (confirmed unchanged from ADR-032/033), plus the real SDK's own
`FilePlayback` sample specifically for how it schedules genuinely *changing*
content across completions (`DeckLinkPlaybackDevice::scheduleVideo()` obtains
a fresh frame object every call — the real precedent this unit's own pool
design extends, distinct from `LoopedFramePlayback`'s "content never
changes" case), plus `io/decklink_output.hpp`/`.cpp` (the preroll idiom and
`fillFrameBuffer()` pattern this unit extends from one buffer to a pool),
`io/decklink_capture_consumer.hpp`/`.cpp` (WU-21b, this unit's own upstream
source of frames), `io/decklink_input.hpp`/`.cpp` (WU-20b, touched only
indirectly via `CaptureConsumer`), and `docs/architecture.md` sections 3, 6,
7, 9, 12 plus ADR-010/032/037/039.
**Files:** `src/io/decklink_live_output.hpp`, `src/io/decklink_live_output.cpp`
(both new: `LiveFramePlayback`), `tests/test_decklink_live_output.cpp` (new);
plus `CMakeLists.txt` (`decklink_live_output.cpp` added to the existing
`scatter-decklink` library's source list — no new `target_link_libraries`
needed, `scatter-decklink` already privately links `scatter-core` as of
WU-21b; `test_decklink_live_output` added alongside the other three DeckLink
tests, linking both `scatter-decklink` and `scatter-core` — CMakeLists.txt
edits have never counted against the "3 source files" cap in any earlier unit
either).
**Accept:** with the same Monitor 3G → Recorder 3G SDI loopback WU-20b's/
WU-21b's own tests already document (no new physical setup), `CaptureSource`
captures, `CaptureConsumer` warps (identity map — this unit's own job is the
pool-refill/reschedule mechanics, not re-proving `runFrameBytes()`'s own warp
correctness or WU-21b's own read-side mechanics, both already genuinely
verified), and `LiveFramePlayback` continuously reschedules the result onto
the Monitor 3G's own SDI output, over a bounded 5-second run:
`stats().completed > 0`, `stats().displayedLate == 0`, `stats().dropped == 0`;
`CaptureConsumer`'s and `CaptureSource`'s own accounting invariants
(`framesProcessed + framesFailed == framesPopped`, `framesPopped <=
framesPushed`) hold, unchanged from WU-21b's own criteria; clean `stop()` on
all three objects. Without the loopback connected, the same mechanics
(clean create/stop, zero dropped/late — every pool buffer is scheduled
regardless of whether `copyLatestFrame()` ever succeeds) are still real
`Accept:` criteria; the test warns rather than fails on zero frames
processed, the same convention `test_decklink_input.cpp`/
`test_decklink_capture_consumer.cpp` already use. Does not include, and does
not claim, the literal one-hour endurance run or a by-eye confirmation that
the SDI output genuinely shows live, changing content — both a future WU-21d's
own job (not yet scheduled), the same category WU-15b/WU-19b already were for
a comparable unattended-hardware criterion.
*Status:* delivered as reasoned-through-only, drafted and written via the
device bridge to the real repository, re-read back from there to confirm
each write landed correctly (`SESSION-PROTOCOL.md` anti-drift rule 8) — then,
unlike every prior DeckLink-touching unit, genuinely built, tested, and run
at Steve's own real terminal within this same session: `cmake --build build`
clean, `ctest --test-dir build --output-on-failure` showing 25 of 26 tests
passing (the sole failure the already-accepted
`test_decklink_device`/`foundDuplexDevice` exception, ADR-035, unrelated),
and `./build/test_decklink_live_output` run directly with a live source
patched into the Recorder 3G's own SDI input and the Monitor 3G's own
HDMI-mirrored output watched live: `completed=124 displayedLate=0 dropped=0
framesRepeated=18` over its own bounded 5-second window, all 10 automated
checks passing. See `DECISIONS.md` ADR-050's own same-session verification
addendum for the full numbers, the second `kCaptureRingCapacity` data point,
and a previously unanticipated cold-start green-frame finding (named, not
fixed). Confirmed `green` in substance; the `wu-21c-green` git tag itself is
Steve's own next action at his own real terminal.

### WU-21d — Cold-start black fill for `LiveFramePlayback`'s own pool `green` (`wu-21d-green`)
See `DECISIONS.md` ADR-064 for the full design and for this session's own
scoping work: the "confirmed DeckLink-linked before assuming either way"
check (`CMakeLists.txt`'s own `scatter-decklink` target, `if(APPLE AND
BLACKMAGIC_SDK_DIR AND EXISTS ...)`, and this session's own cloud-sandbox
`cmake -B build` output confirming `src/io/decklink_live_output.cpp` is not
compiled there at all), the portable/DeckLink-linked split (`scatter::v210::
packBlackFrame()`, new, alongside `packRow()`/`packImage()` — portable,
sandbox-buildable/testable; `decklink_live_output.cpp`'s own change —
calling it once per `create()`, filling every pool buffer immediately after
`CreateVideoFrame()` — reasoned-through-only, same as every DeckLink-linked
unit before it), and the scope decision explicitly narrowing this unit to
the black-fill fix alone, leaving the endurance-run/`framesRepeated`-rate
candidate territory this entry originally also named undecided, exactly as
`DECISIONS.md` ADR-050's own "Not decided here, deliberately" paragraph
already left it.

WU-21c's own real-hardware verification (`DECISIONS.md` ADR-050's own
same-session addendum) found that `LiveFramePlayback`'s pool buffers,
scheduled during `startWith()`'s own preroll loop before `CaptureConsumer`
has produced its own first output, are left holding whatever
`CreateVideoFrame()` first allocated them with — effectively zero-filled
`v210` — which decodes as a strongly saturated green once converted for
display, seen by eye on the Monitor 3G's own HDMI-mirrored output as a few
seconds of solid green before real content appears. This unit builds
ADR-050's own named candidate fix: fill each pool buffer with black
immediately after `CreateVideoFrame()`, before the preroll loop schedules
any of them.

**Files:** `src/video/v210.hpp` (new `packBlackFrame()` declaration,
additive), `src/video/v210.cpp` (new `packBlackFrame()` definition, plus two
new standard-library includes), `src/io/decklink_live_output.cpp`
(`startWith()`'s pool-creation loop black-fills each buffer via the
already-existing local `fillFrameBuffer()` helper; `create()` fails cleanly
if a fill fails, same as every other `startWith()` step); `tests/
test_v210.cpp` (new `testPackBlackFrame()`, three widths). No
`CMakeLists.txt` change. +117/-0 lines across the four files — within
`SESSION-PROTOCOL.md`'s "3 source files plus its test, ~400 lines" cap.

**Accept:** `packBlackFrame()` unpacks back to `kBlack`/`kChromaZero` at
every sample, at three widths (12, 14, 720), with every row's own packed
bytes identical to row 0's — `tests/test_v210.cpp`'s `testPackBlackFrame()`.
Every mechanical criterion `tests/test_decklink_live_output.cpp` already
checks is unchanged and must still hold. Unautomatable, Steve's own by-eye
job (see `DECISIONS.md` ADR-064's own reasoning for why no new automated
check was added instead): with a live source patched into the Recorder 3G's
own SDI input, the Monitor 3G's own HDMI-mirrored output should show black,
not solid green, for the first few seconds after `LiveFramePlayback::
create()` starts scheduled playback, before real captured content arrives.

*Status:* `wip` — the portable half (`packBlackFrame()`, `video/v210.*`,
`tests/test_v210.cpp`) built and tested in this session's own cloud sandbox:
fresh clone confirmed at `wu-26-green`/`4381823` before any file was
touched, full 8-configuration matrix (GCC 13.3/Clang 18.1, Release/Debug,
tile 4/5) plus GCC 13 ASan+UBSan all green, zero warnings, `ctest` 22/22 in
every configuration, `test_v210` itself 5117 checks passing. The
DeckLink-linked half (`decklink_live_output.cpp`) is reasoned-through-only,
written via the device bridge and re-read back to confirm the write landed
— confirmed via this session's own cloud-sandbox `cmake -B build` output
that it is not compiled there at all (no Blackmagic SDK, no AppleClang/
Xcode toolchain), so the DeckLink-linked half was genuinely untested by
this session.

**Status-line correction, WU-36 (this sweep):** `git tag`/`git log` against
the real repository confirm `wu-21d-green` exists, on commit `bba3634`
("WU-21d: cold-start black fill for `LiveFramePlayback`'s own pool") —
Steve's own real-terminal build/`ctest`/by-eye confirmation this entry was
waiting on already happened and was never reflected in this line's own
status word. Header updated above from `wip` to `green`. Not touching any
source file.

### WU-21e — First live-warped-video demo: a real sphere lattice
(`core/shapes/sphere.cpp`, WU-11/ADR-027) through the live pipeline WU-21c
already verified, instead of that unit's own identity lattice `wip`
Combines two already-independently-verified pieces; adds no new algorithmic
capability. `buildSphereLattice()` has its own geometric proof and passing
test (`test_shapes.cpp`, ADR-027 — every control vertex lands exactly on the
configured sphere, for any `angleSpanH`/`angleSpanV`). `CaptureSource`
(WU-20b), `CaptureConsumer` (WU-21b) and `LiveFramePlayback` (WU-21c) are
all already lattice-agnostic — `CaptureConsumer`'s own constructor takes a
`Lattice` by value, not a hardcoded identity map; `LiveFramePlayback` never
inspects what produced the bytes it schedules. WU-21c's own real-hardware
run already proved the mechanics (pool refill/reschedule, no dropped/late
frames) end to end; this unit's own job is solely swapping which lattice
`CaptureConsumer` is built with, to put a real warp on the wire for the
first time, and giving Steve a working artifact to look at.
**Files:** `tests/test_decklink_live_sphere.cpp` (new — duplicated from
WU-21c's own `test_decklink_live_output.cpp` per `SESSION-PROTOCOL.md` rule
2, one line materially different: `CaptureConsumer` built from
`scatter::shapes::buildSphereLattice()` instead of an identity lattice);
plus `CMakeLists.txt` (new `test_decklink_live_sphere` executable,
registered identically to `test_decklink_live_output`, same dual-link
reason — `CMakeLists.txt` edits have never counted against the "3 source
files" cap in any earlier unit either). No `src/` files touched — nothing
about `LiveFramePlayback`/`CaptureConsumer`/`CaptureSource` themselves
changes; this unit is entirely a demo-harness recombination of already-
shipped code.
**Accept:** identical mechanical criteria to WU-21c's own (`stats().completed
> 0`, `stats().displayedLate == 0`, `stats().dropped == 0`; the
`CaptureConsumer`/`CaptureSource` accounting invariants), over a bounded
10-second run (longer than WU-21c's own 5, to give a real by-eye look more
time) — swapping the lattice changes nothing about what these particular
checks measure, since they were never about warp correctness to begin with.
The genuinely new criterion this unit adds is not automatable: with a live
source patched into the Recorder 3G's own SDI input, the Monitor 3G's own
output (SDI, mirrored to HDMI — confirmed this project's own UltraStudio
Monitor 3G does this by design) should visibly show that source mapped onto
a sphere, not a flat picture. Steve's own by-eye job, the same division of
labour every "does this look right" criterion in this project has used
since WU-15b/WU-19b.
*Status:* **superseded by WU-21f before ever being built, run, committed or
tagged.** Steve found this unit's own first-cut sphere geometry (radius
220, `angleSpanH` 1.2, `angleSpanV` 1.0) cropped vertically and did not look
fully wrapped, and separately asked for a keypress-driven run length and
two-axis rotation — enough new scope, per this project's own a/b/c/.../f
splitting discipline, to become its own unit rather than a quiet in-place
edit. `tests/test_decklink_live_sphere.cpp`'s own content is now WU-21f's,
not this text's — see below. Nothing from this entry was ever run for
real, so nothing here needed a correction; see `DECISIONS.md` ADR-052.

### WU-21f — Rotating live sphere demo: fixes WU-21e's own geometry (equal,
larger `angleSpanH`/`angleSpanV`), runs until a keypress instead of a fixed
duration, and rotates the sphere in two axes at once via a new
`CaptureConsumer::setLattice()` `wip`
See `DECISIONS.md` ADR-052 for the full design: why WU-21e's own unequal,
modest angular spans produced unequal on-screen extents by construction (not
a pipeline bug), the fix (`angleSpanH == angleSpanV == 2.0` rad, comfortably
inside the `+/-pi/2` half-angle fold threshold `test_shapes.cpp`'s own
comment documents); the rotation design (a rigid yaw-then-pitch rotation of
the already-built lattice's own control points around the sphere's true
centre `(centerX, centerY, radius)`, implemented entirely in this test file,
not in `core/shapes/sphere.cpp` — `buildSphereLattice()` itself stays
untouched and still exactly as `test_shapes.cpp` proved it); the one real,
unverified risk this approach carries (a large enough rotation can produce
negative depth, `z < 0`, which no shape in this project has ever produced
before — bounded rotation amplitude sidesteps it here, not resolves it); and
the keypress-until-stop design (a dedicated thread blocking on a single raw
terminal keypress via `termios`, falling back to a bounded 60-second run if
stdin is not a real TTY).
**Files:** `src/io/decklink_capture_consumer.hpp`/`.cpp` (edited — new
`CaptureConsumer::setLattice(Lattice)`, thread-safe via a new dedicated
`m_latticeMutex` separate from the existing `m_mutex` that guards
`m_latestFrame`; `processOne()` now takes a snapshot copy of the current
lattice under that lock before touching the capture frame's own buffer,
using the snapshot for `runFrameBytes()` instead of `m_lattice` directly);
`tests/test_decklink_live_sphere.cpp` (rewritten in place — WU-21e's own
content superseded, see above). No `CMakeLists.txt` change needed: the
`test_decklink_live_sphere` target WU-21e already registered points at the
same filename, whose content this unit replaces.
**Accept:** the same mechanical criteria WU-21c's/WU-21e's own tests already
use (`stats().completed > 0`, `stats().displayedLate == 0`, `stats().dropped
== 0`; the `CaptureConsumer`/`CaptureSource` accounting invariants) — a
changing, rotating lattice does not change what these checks measure, same
reasoning WU-21e's own `Accept:` already gave for a static sphere lattice.
Unautomatable, Steve's own by-eye job, same as WU-21e: with a live source
patched into the Recorder 3G's own SDI input, the Monitor 3G's own output
should show the source wrapped fully onto a sphere (not vertically cropped)
that visibly tumbles in two axes at once, not a static or single-axis
motion; pressing a key should stop the run promptly. Does not claim negative-
depth rotation amplitudes are safe — deliberately not attempted here (see
above).
*Status:* **built, run, and given real feedback at Steve's own real
terminal this session — the tumble was visible ("interesting"), but the
wrap read as roughly 120-180 degrees of the sphere with the video stopping
short of the poles, and Steve asked for one continuous rotation axis
instead of two oscillating ones.** Not a defect in anything this unit
built (`CaptureConsumer::setLattice()` worked; the mechanical `Accept:`
criteria were never reported as failing) — the angular-span parameters
were this unit's own demo choice, not a pipeline limit. Its own
`tests/test_decklink_live_sphere.cpp` content is now superseded by WU-21g,
the same "superseded before being tagged" treatment WU-21e received one
unit earlier — `CaptureConsumer::setLattice()`/`m_latticeMutex` themselves
are unaffected and carry forward unchanged. See `DECISIONS.md` ADR-052's
own addendum and `CORRECTIONS.md` C-017 (this unit's own rotation-
amplitude reasoning did not hold up to the sphere's own documented
invariant — corrected there, not anything built).

### WU-21g — Full sphere wrap (pole to pole, seamless 360 degrees
azimuthally), one continuous rotation axis, one oscillating axis `wip`
See `DECISIONS.md` ADR-053 for the full design: why `angleSpanV == pi`
reaches the poles exactly without crossing into the folding regime (`sin`
is monotonic on the closed interval up to and including its own endpoint);
why `angleSpanH == 2*pi` gives a mathematically seamless full wrap (source
left/right edges meet exactly at the sphere's own back seam) at the cost
of deliberately entering `tests/test_shapes.cpp`'s own documented folding
regime for most of the lattice (front/back hemispheres both exist and can
visibly overlap on screen with no occlusion sorting, `WU-28`'s k-buffer not
built yet — named, not solved, Steve's own next by-eye report); and the
corrected rotation-safety reasoning (`CORRECTIONS.md` C-017): rotation
about the sphere's own true centre can never produce negative depth, for
any amplitude, so yaw is now a continuous, unbounded spin (one revolution
every 8 seconds) while pitch keeps oscillating, back-and-forth, at a
larger amplitude than WU-21f's own (1.0 rad, was 0.35) now that the
conservatism C-017 corrects no longer applies.
**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
— WU-21f's own content superseded, see above). No `src/` or
`CMakeLists.txt` change — `CaptureConsumer::setLattice()` (WU-21f) already
supports an arbitrarily-changing lattice; the executable target already
exists and points at this same filename.
**Accept:** the same mechanical criteria every live-pipeline test since
WU-21c has used, unchanged (a changing lattice and rotation schedule does
not change what pool-refill/reschedule mechanics they measure).
Unautomatable, Steve's own by-eye job: the video should now visibly wrap
all the way to the poles (with the expected pinch there) and seamlessly at
the back; yaw should spin continuously in one direction, pitch should rock
back and forth; and — genuinely not predicted here — how the un-occluded
front/back overlap during a full wrap actually reads on screen.
*Status:* **built, run, and given real feedback at Steve's own real
terminal this session — "hugely better."** The pole-to-pole/360-degree wrap
geometry is confirmed good and carries forward unchanged into WU-21h.
Front/back overlap where the wrap folds is visible, as this unit's own
`Accept:` above expected without predicting how it would look — recorded
as a backlog item on WU-28's own entry above, not a defect in this unit.
Steve asked for manual keyboard control in place of the automatic
rotation schedule; this unit's own `tests/test_decklink_live_sphere.cpp`
content is superseded by WU-21h, the same "superseded before being tagged"
treatment WU-21e and WU-21f each received one unit earlier — nothing in
this entry's own geometry fix was wrong, only its own animation mechanism
is being replaced.

### WU-21h — Rudimentary interactive UI: cursor keys rotate (manual yaw/
pitch), shift+cursor keys reposition, I/O resize, Q quits — replaces
WU-21g's own automatic rotation schedule `wip`
See `DECISIONS.md` ADR-054 for the full design: the xterm escape-sequence
parsing this needs for cursor and shift+cursor keys (`readKey()`, honestly
flagged as terminal-dependent and unverified in this session); why input
handling moved from WU-21g's own separate keypress-wait thread plus 80ms
polling timer to a single event-driven blocking-read loop on the main
thread, now that there is no idle animation to keep advancing between
keypresses; why `makeSphereLattice()`/`rotateLattice()` both now take
radius/centre as parameters instead of file-level constants, rebuilt fresh
on every state-changing keypress (cheap enough per this project's own
"once per frame" cost reasoning, `shapes.hpp`'s own header comment); and
the non-interactive fallback (a short static bounded run, not a hang) that
keeps an unattended `ctest` run from blocking forever on a keypress that
can never arrive. The full pole-to-pole/360-degree wrap geometry itself
(`angleSpanH == 2*pi`, `angleSpanV == pi`) is unchanged from WU-21g.
**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
— WU-21g's own content superseded, see above). No `src/` or
`CMakeLists.txt` change — `CaptureConsumer::setLattice()` (WU-21f) already
supports an arbitrarily-changing lattice on an arbitrary schedule,
including one driven by keypresses instead of a timer; the executable
target already exists and points at this same filename.
**Accept:** the same mechanical criteria every live-pipeline test since
WU-21c has used, unchanged. Unautomatable, Steve's own by-eye/by-hand job:
each cursor key visibly rotates the sphere on the expected axis, each
shift+cursor key visibly moves it, I/O visibly shrink/grow it, Q exits
cleanly; and, honestly not guaranteed here, whether shift+cursor is
correctly distinguished from plain cursor on Steve's own actual terminal
emulator — worth reporting either way.
*Status:* **built and run at Steve's own real terminal this session —
plain cursor-key rotation worked; shift+cursor repositioning did not.** See
`CORRECTIONS.md` C-018: this unit's own `ESC [ 1 ; 2 <letter>` xterm
shift-sequence claim, already flagged as unverified when written, turned
out wrong on Steve's own real setup. `CaptureConsumer::setLattice()`
(WU-21f) and the event-driven input model this unit introduced both worked
correctly — only the shift-modifier key detection is being replaced.
Superseded by WU-21i, same "superseded before being tagged" treatment
WU-21e/f/g each received one unit earlier.

### WU-21i — Letter-key manual controls (X/x, Y/y, Z/z), replacing
WU-21h's own broken shift+cursor scheme `green` (`wu-21i-green`)
Status line corrected doc-only, by this session while re-verifying
repository state before starting WU-21d — `wu-21i-green` confirmed present
directly via `git tag` against the real repository, not assumed from this
line's own prior text (stale `wip` for nine sessions, per Session 35's own
note below and unchanged since). Not touching any of this unit's own source
files, the same kind of correction Session 35 made for WU-28a's own stale
status line and Session 37 made for WU-28b's.
See `DECISIONS.md` ADR-055 and `CORRECTIONS.md` C-018 for the full story:
plain arrow-key rotation is unchanged (it worked); shift+cursor
repositioning and `I`/`O` resizing are both replaced by six ordinary
letter keys for one consistent scheme — `X`/`x` increment/decrement
`centerX`, `Y`/`y` increment/decrement `centerY`, `Z`/`z` increment/
decrement `radius` (`Z` bigger/"out", `z` smaller/"in", the same sense
WU-21h's own `O`/`I` had) — avoiding modifier-key escape sequences
entirely rather than debugging why Steve's own terminal did not send the
one WU-21h assumed.
**Files:** `tests/test_decklink_live_sphere.cpp` only (rewritten in place
— WU-21h's own content superseded). No `src/`/`CMakeLists.txt` change.
**Accept:** the same mechanical criteria every live-pipeline test since
WU-21c has used, unchanged. Unautomatable, Steve's own by-hand job: each
of the six letter keys visibly does what it says, plain cursor-key
rotation still works as it already did, Q still exits cleanly.
*Status:* delivered as reasoned-through-only, drafted and written via the
device bridge to the real repository, re-read back from there to confirm
each write landed correctly (`SESSION-PROTOCOL.md` anti-drift rule 8) — not
yet built or run at Steve's own real terminal. No Blackmagic SDK and no
AppleClang/Xcode toolchain exist in the Linux cloud sandbox this session
drafted this in, the same gap every DeckLink-touching unit before it has
named.

### WU-22a — Diagnostic coverage view: opt-in full-frame weight-capture
plumbing (`PipelineParams::weightOut`) `green` (`wu-22a-green`)
See `DECISIONS.md` ADR-056 for the full design: the weight-only (det J
deferred)/portable-plumbing-vs-Mac-only-display splits — this unit is the
first (WU-22b, not started, covers the second); why `PipelineParams::
weightOut` is a non-owning, caller-owned, opt-in pointer field mirroring
`PipelineParams::pool` (WU-19a/ADR-044) rather than a second output raster
threaded through every `runFrame()`/`runFrameBytes()`/`runFrameFile()`
signature; and the `CORRECTIONS.md` C-008(a) edge-derivative-damping
finding hit while writing this unit's own tests — an already-documented
codebase property (ADR-022's edge-replication clamp), not a new bug, so no
new `CORRECTIONS.md` entry was logged.
**Files:** `src/core/resolve.hpp` (`PipelineParams::weightOut` field
added, default `nullptr`), `src/core/pipeline.cpp` (`resolveOneTile()`
write site, one `if (params.weightOut != nullptr)` block immediately
after the existing `dest.Y/Cb/Cr` writes), `tests/test_coverage_capture.cpp`
(new, four tests), `CMakeLists.txt` (`scatter_test(test_coverage_capture)`
registration).
**Accept:** `test_coverage_capture` passes across the project's full
matrix (Clang 18 / GCC 13 x Release/Debug x tile 2^4/2^5, plus GCC
ASan/UBSan) with zero behaviour change to any existing caller
(side-effect-freedom checked directly), a bit-for-bit cross-check against
an independent recomputation via the public `generateFragments()`/
`splatTile()`/`sumBanks()` path, and I6 thread-count invariance.
*Status:* built, tested, and verified across the full 8-config +
ASan/UBSan matrix in the Linux cloud sandbox this session (all green);
delivered to the real repository via the device bridge and confirmed
written correctly (sha256 match — `device_stage_files` itself was blocked
this session by a stale device sign-in, see `HANDOFF.md`). Confirmed at
Steve's own real terminal: `cmake --build build` clean (`ninja: no work
to do` — already configured), `ctest --test-dir build --output-on-failure`
27 of 28 passing, `test_coverage_capture` itself green, the sole failure
`test_decklink_device`'s `foundDuplexDevice` check — ADR-035's own
already-accepted exception (Monitor 3G only, not full duplex), unrelated
to this unit. `./tools/close.sh 22a` itself correctly refused to tag,
having no knowledge of ADR-035; the manual `git tag -a wu-22a-green ...`
step every DeckLink-touching unit since ADR-035 has used was run instead.
Confirmed `green`, tagged `wu-22a-green` (verified directly against the
real repository's own `git show wu-22a-green --stat`: exactly the seven
files above — see `HANDOFF.md`). `git push origin HEAD --tags` reported
no `origin` remote configured; the tag and commit are local-only, the same
as `close.sh` itself would have silently accepted.

### WU-22b — Diagnostic coverage view: Metal window display on the Mac's
own display `green` (`wu-22b-green`)
See `DECISIONS.md` ADR-057 for the full design: the scoping conversation
(launch mechanism, offline-vs-live-wired split, colour mapping, window
sizing/resizability, refresh-rate, and `PipelineParams::weightOut`'s own
threading needs); why Steve's own "lowest overhead in processing terms"
criterion picked in-process (a future flag on the live-sphere demo, not
built this unit) over any IPC-based launch design, and how that is
reconciled with this unit's own offline/static-data-driven first cut (an
already-established portable-piece-now/platform-piece-next split,
ADR-046/048/056, not a contradiction); the black-at-0/white-at-
`kWeightUnity`/clipped-above grayscale colour mapping; the fixed-size,
non-resizable, redraw-on-new-data window design; the double-buffer/
`dispatch_async`-to-main-thread threading design recorded as intent for
WU-22c below, not implemented here; and the five known risk points flagged
for whoever builds this first (shader UV/vertical-flip convention, ARC
correctness, inline runtime shader compilation, the `[NSApp stop:]`-plus-
dummy-event quit mechanism, and the hand-mirrored `kWeightUnityLocal`
constant).
**Files:** `src/diag/coverage_view.hpp` (new — platform-independent public
interface, `CoverageWindowConfig`/`CoverageWindow`, pimpl'd, no Apple
framework `#include`), `src/diag/coverage_view.mm` (new — this project's
first Objective-C++ translation unit: `NSWindow`, `MTKView`/
`MTKViewDelegate`, Metal device/texture/pipeline state, an inline MSL
shader compiled at runtime), `tools/coverage_view_demo.cpp` (new — a
hand-run tool, not a test, the same `add_executable`-only shape
`tools/make_testpat.cpp` established at WU-03; builds one sphere-warped
frame with `weightOut` capture enabled and opens a `CoverageWindow` on
it), plus `CMakeLists.txt` (`if(APPLE)` block: `enable_language(OBJCXX)`,
the `scatter-diag` static library, the `coverage_view_demo` executable).
**Accept:** no programmatic accept criterion (see above and ADR-057 —
there is no automatable pass/fail test for "does a GUI window look
right"); acceptance is Steve, at his own real terminal, confirming
`cmake --build` succeeds, `./build/coverage_view_demo` opens a window
showing a visibly non-uniform grayscale sphere-coverage image, and that
`Q` or closing the window cleanly exits it.
*Status:* fully scoped and drafted reasoned-through-only, then confirmed
working at Steve's own real terminal the same session: `cmake --build
build` clean, zero warnings; `./build/coverage_view_demo` opened a 512x512
window showing a visibly non-uniform grayscale dome shape, edges brighter
than centre, matching `CORRECTIONS.md` C-011's own prediction exactly;
both quit paths (`Q` with the window focused, and the window's own close
control) close the window and return the shell promptly, no hang. None of
`DECISIONS.md` ADR-057's own five flagged known risk points turned out to
be real defects — see that ADR's own verification addendum for the detail
on each. Committed (`4db0517`); `./tools/close.sh 22b` refused to
auto-tag on the same already-accepted `test_decklink_device`/ADR-035
duplex-check failure `wu-22a-green` also hit (27/28, unrelated to this
unit), so the manual `git tag -a wu-22b-green ...` fallback was used
instead — confirmed directly against the real repository: `git show
wu-22b-green --stat` lists exactly the seven files this unit's own commit
touched (`CMakeLists.txt`, `DECISIONS.md`, `HANDOFF.md`, `WORK-UNITS.md`,
`src/diag/coverage_view.hpp`, `src/diag/coverage_view.mm`,
`tools/coverage_view_demo.cpp`), `git status --short` clean, no stale
`.git/index.lock`. `git log` shows no `origin` remote push attempted (none
configured, same as every earlier unit) — the commit and tag are
local-only. **WU-22b is genuinely `green`.**

### WU-22c — Diagnostic coverage view: wire `CoverageWindow` into the live
capture/output pipeline `green`
**Confirmed `green` this session (Session 33), not built or tested by it:**
`wu-22c-green` exists in the real repository and `git status -sb` reads
`## main...origin/main` with no ahead/behind marker — Steve's own
build/run/verify/commit/tag/push (`HANDOFF.md`'s own Session 32 "Steve's
own next steps") completed between sessions. This entry's own status line
was still `wip` until this fix — the same staleness `WU-21i`'s own entry
carried for three sessions running (Session 31's `HANDOFF.md`, Flagged item
1) — fixed directly per that same file's own standing instruction to fix a
confirmed-stale status line on sight rather than carry it forward again.
See `DECISIONS.md` ADR-058 for the full design: the scoping conversation
(flag name/behavior, how the terminal keypress loop and Cocoa's own
main-thread run loop coexist, which thread produces frames and whether
ADR-057's own double-buffer/`dispatch_async` sketch survived contact with
the real threading structure, what `Q` in the coverage window should quit,
redraw cadence); the `--show-coverage` flag (Steve's own choice to keep
ADR-057's own working name); the unified-main-thread-loop design (a
`DISPATCH_SOURCE_TYPE_READ` dispatch source on `STDIN_FILENO`, queued onto
`dispatch_get_main_queue()`, alongside `CoverageWindow::run()`'s own
`[NSApp run]`, both using GCD's function-pointer APIs only — no
Objective-C Blocks — so this stays a plain `.cpp` file); `IncrementalKeyParser`,
a new non-blocking per-byte state machine replacing `readKey()` for the
flag-on path only (`readKey()` and the flag-off loop are untouched);
`CaptureConsumer`'s new opt-in `CoverageCallback` hook (default `nullptr`,
zero cost when absent, mirroring `PipelineParams::pool`/`weightOut`'s own
convention); the per-frame heap-allocated `dispatch_async_f` hand-off that
implements ADR-057's own "double-buffer" design intent (a fresh buffer per
frame via `std::vector` move semantics, not a literal reusable pair — see
ADR-058 for why); the new `CoverageWindow::requestQuit()` method (one
additive, non-breaking forward into WU-22b's own already-`green` internals)
that lets a terminal `Q` also end the coverage window's own run loop, so
one `Q`, from either channel, quits the whole session; and why the coverage
window never opens in a non-interactive run (no quit signal would ever
reach it, hanging an unattended `ctest` run).
**Files:** `tests/test_decklink_live_sphere.cpp` (rewritten — `argc`/`argv`
parsing, `coverageActive` gate, `IncrementalKeyParser`,
`CoverageInputContext`/`handleCoverageStdinReadable()`,
`CoverageDispatchContext`/`applyCoverageOnMainThread()`, the flag-on unified
main-thread loop alongside the untouched flag-off blocking loop, and —
same-session follow-up, see below — `mapCoverageWindowKey()` plus a
`coverageWindow->setKeyHandler()` wire-up), `src/diag/coverage_view.hpp`/
`.mm` (additive — new public `CoverageWindow::requestQuit()`, and — same
follow-up — `SpecialKey`/`setKeyHandler()`; WU-22b's own existing surface
otherwise unchanged), `src/io/decklink_capture_consumer.hpp`/`.cpp`
(additive — new optional `CoverageCallback` constructor parameter and its
wiring inside `processOne()`, WU-21b's own existing behavior unchanged
when absent), and `CMakeLists.txt` (the `scatter-diag` block moved ahead of
the `scatter-decklink` block so `add_library(scatter-diag ...)` is seen
before `test_decklink_live_sphere`'s own `target_link_libraries`
references it; `test_decklink_live_sphere` gains `scatter-diag` on that
one line).
**Same-session follow-up, after Steve's first real build/run:** the
coverage window did not respond to any control key but `Q` — macOS
keyboard focus is per-window, so the stdin channel above only ever sees a
keystroke typed while the *terminal* has focus; `ScatterCoverageMTKView`'s
own `-keyDown:` (WU-22b) never recognized anything but `Q`. Asked directly,
Steve chose to make the coverage window itself fully interactive too
(rather than leave it display-plus-quit-only) — see `DECISIONS.md`
ADR-058's own addendum for the fix (`CoverageWindow::setKeyHandler()`,
`SpecialKey`, `mapCoverageWindowKey()`), folded into this same entry since
nothing had been committed yet.
**Accept:** no programmatic accept criterion for the coverage window itself
(same reason as WU-22b — no automatable pass/fail test for "does a GUI
window look right"); acceptance is Steve, at his own real terminal,
confirming (a) `cmake --build build` succeeds with `test_decklink_live_sphere`
now also linking `scatter-diag`, (b) running the test **without**
`--show-coverage` behaves exactly as it already did (WU-21i, unchanged),
(c) running it **with** `--show-coverage` opens a coverage window
alongside live SDI playback, updating roughly once per live frame, and
that all ten controls (six letters, four arrows) move the sphere and are
reflected in the coverage window from **either** the terminal or the
coverage window itself having focus, and (d) `Q`, pressed either at the
terminal or in the coverage window, cleanly ends both the window and the
whole test run (capture/consumer/playback all stop, stats print, process
exits) with no hang.
*Status:* fully scoped and drafted reasoned-through-only this session —
**UNVERIFIED IN FULL**, per this project's own established convention for
every DeckLink-and/or-Metal/Cocoa-touching unit: this sandbox has no
Blackmagic SDK, no Cocoa, no Metal, no AppleClang/Xcode toolchain, so none
of this was built or run by the session that wrote it — this includes the
same-session `setKeyHandler()` follow-up above, which came from Steve's own
real-terminal report of the *first* build but has not itself been rebuilt
and reverified yet. See `DECISIONS.md` ADR-058's own closing paragraph and
its own addendum for the two most likely first problems to check. Needs
`cmake --build build`, `ctest` (or a direct interactive run both with and
without the new flag), and real-hardware verification at Steve's own real
terminal before this unit can be called `green`.

---

## Phase 6 — Scale up

### WU-23 — Interlace and field mode
This session (the scoping session this bare line asked for) split it into
three real units once the actual code paths were read: de-interlace-to-frame
and field mode are genuinely different mechanisms (`DECISIONS.md` ADR-075),
and field mode itself split again once building began, when
`core/binner.hpp`'s own v-parameter denominator turned out to be load-bearing
for field mode's correctness, not just a `video/`-layer data shuffle. Steve's
own stated preference — if deinterlacing is pursued, the route is Weston
3-field, for period accuracy, not a simpler bob/weave or line-doubling
approach — carries forward unchanged onto WU-23b below.

### WU-23a — Field mode: field split and interleave `green` (`wu-23a-green`, pending Steve's manual tag)
See `DECISIONS.md` ADR-075 for the full design, including why this is only
field mode's own data-layout half (`video/interlace.hpp`'s own file comment
has the complete reasoning) and not yet the lattice/warp-aware half
(WU-23a2, below) — discovered while building, not before this session, once
`core/binner.hpp`'s own `generateFragmentsRowRange()` comment ("every u/v
lattice-parameter calculation stays keyed to src.width/src.height in full")
made clear that a field-native `SourceRaster` half the frame's own height
would renormalise the v-parameter across only that field's own extent,
erasing the half-line vertical phase between the two fields that makes
interlace look like interlace rather than two independently-scaled
progressive images.

**Files:** `src/video/interlace.hpp` (new — `FieldParity`, `fieldRowCount()`,
`extractField()`, `interleaveFields()`), `src/video/interlace.cpp` (new),
`tests/test_interlace.cpp` (new); `CMakeLists.txt` (registration only —
`src/video/interlace.cpp` added to `scatter-core`'s unconditional source
list, `scatter_test(test_interlace)` added alongside the other core-only
tests — not counted against the 3-source-file cap, the same
"registration, not implementation" accounting this project's own prior
units have used). +297/-0 lines across the four files — within
`SESSION-PROTOCOL.md`'s "3 source files plus its test, ~400 lines" cap (2
non-test source files, well under 3, leaving headroom WU-23a2 will need).

**Accept:** a synthetic marked frame (every row, every plane, a distinct
value, so a row landing in the wrong place or one plane's row coming from
the wrong source row both show up as a mismatch) split into its own Top and
Bottom fields via `extractField()` reproduces each field's own source rows
bit-exactly, for both an even and an odd frame height; `interleaveFields()`
recombines two independently-produced field rasters back into the original
frame bit-for-bit, every plane, every row, for an even frame height, an odd
frame height, and this project's own two real geometries (720×576,
1920×1080); `fieldRowCount()` accounts for every row of a frame exactly
once between the two parities, no row lost or double-counted, checked
directly rather than only inferred from the round-trip. Deliberately not
exercised: any lattice/warp involvement at all — see WU-23a2.

**Status:** built and tested in this session's own Linux cloud sandbox
(Ubuntu 24.04). No Blackmagic SDK or Apple toolchain needed — this unit
touches no platform-specific surface at all, unlike WU-17/WU-18's own NEON
work, so no AArch64 cross-compile was run either, the same "portable-only
unit, portable-only matrix" scope WU-16a/WU-19a/WU-26/WU-28c already used.
Full 8-configuration matrix — GCC 13.3.0 and Clang 18.1.3, Release and
Debug, `SCATTER_TILE_LOG2` 4 and 5 — all green, zero warnings under this
project's full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion
-Werror` set, plus GCC 13 with `-fsanitize=address,undefined
-fno-sanitize-recover=all` at both tile sizes: clean, no sanitizer report.
`ctest`: 24 of 24 targets passing in every configuration (23 pre-existing,
unaffected, plus this unit's own new `test_interlace`). `test_interlace`
alone: 302 checks passing. Steve's own real-terminal build/`ctest` and
`./tools/close.sh 23a` are the remaining steps — see `HANDOFF.md`.

### WU-23a2 — Field mode: lattice-aware per-field fragment generation
This session's own scoping pass (continuing straight from WU-23a) found
that the real warp half of field mode needs two things that together
exceed `SESSION-PROTOCOL.md`'s 3-source-file cap for one unit: a new
`core/binner.hpp`/`.cpp` sibling entry point (2 files), and a new
orchestration entry point declared in `core/resolve.hpp` and implemented
in `core/pipeline.cpp` (2 more files, per that header's own comment on
why `runFrame()`/`runFrameFile()` live there and not in a `pipeline.hpp`
of their own) — the same "exceeds the cap, split it" shape that already
separated WU-23a from WU-23a2 itself. Split into WU-23a2a/WU-23a2b below,
confirmed with Steve directly before either was scoped in detail. See
`DECISIONS.md` ADR-076 for the full reasoning, including the
`extractField()`-usage question this split settles.

### WU-23a2a — Field mode: field-parity row visitation in core/binner.hpp/.cpp `green` (`wu-23a2a-green`, pending Steve's manual tag)
See `DECISIONS.md` ADR-076 for the full design. New `core/binner.hpp`/`.cpp`
sibling entry point, `generateFragmentsFieldRows()`, generating fragments
for one field's own rows (stride 2, offset 0 or 1) while keeping the
v-parameter's denominator at the *full frame* height — the fix
`DECISIONS.md` ADR-075 named and left for this unit. Settles this unit's
own open design question (was `extractField()` needed on the input side?
no — see ADR-076) from the real code, not assumed.

**Files:** `core/binner.hpp` (new `generateFragmentsFieldRows()`
declaration), `core/binner.cpp` (new `generateFragmentsFieldRows()`
definition; `rowStep` added to the shared, private
`generateFragmentsRowRangeImpl()`, all three existing call sites updated
to pass it explicitly, unchanged behaviour), `tests/test_binner.cpp`
(two new test functions plus three small helpers). No `CMakeLists.txt`
change — `test_binner` is already registered.

**Accept:** one `generateFragmentsRowRange()` call per row of a field's
own parity, accumulated into one `TileBins`, equals one
`generateFragmentsFieldRows()` call byte-for-byte, tile by tile,
fragment by fragment, in the same order (not merely as a multiset), for
both parities; separately, `extractField()`-then-`generateFragments()`
(the naive plan ADR-075 already rejected) is shown to reproduce frame
row 0's own destination for *both* fields' own row 0 — Bottom's own row
0 is actually frame row 1 — where `generateFragmentsFieldRows()` places
the two one pixel apart, the correct half-line phase, demonstrating the
fix's necessity directly rather than only by equivalence to row-range
calls.

**Status:** built and tested in this session's own Linux cloud sandbox
(Ubuntu 24.04), a fresh clone of `origin/main` at `wu-23a-green`/`46e9240`,
confirmed clean before any file was touched. Full 8-configuration matrix
(GCC 13.3.0 / Clang 18.1.3, Release/Debug, `SCATTER_TILE_LOG2` 4/5) all
green, zero warnings under this project's full `-Wall -Wextra -Wpedantic
-Wconversion -Wsign-conversion -Werror` set, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes, clean. No AArch64 cross-compile — this unit touches no
platform-specific surface, the same scope WU-16a/WU-19a/WU-26/WU-28c's
own portable-only units already used. `ctest`: 24 of 24 (unchanged count
— `test_binner` is extended, not new). `test_binner` alone: 38 727
checks passing. Steve's own real-terminal build/`ctest` and manual
tag/push are the remaining steps — see `HANDOFF.md`.

### WU-23a2b — Field mode: runFrame()-level driver `green` (`wu-23a2b-green`, pending Steve's manual tag)
See `DECISIONS.md` ADR-077 for the full design. Wires
`generateFragmentsFieldRows()` (WU-23a2a) and `video/interlace.hpp`'s
`extractField()`/`interleaveFields()` (WU-23a) together into
`runFrameField()`: run the field-aware binner/splat/resolve once per
parity into a full-resolution destination raster, `extractField()` each
down to its own parity rows (sized against `params.destHeight`, not
`src.height` — ADR-077), then `interleaveFields()` to recombine into the
final interlaced frame — see `DECISIONS.md` ADR-076 for why both
functions are needed on the output side. Declared in `core/resolve.hpp`,
implemented in `core/pipeline.cpp`, per ADR-026's own precedent for new
orchestration entry points. Single-threaded only, this unit —
`params.threads`/`params.pool` are not consulted; a threaded field-mode
path is deferred, not scheduled. `params.kBufferMode` must be `Off` and
`params.weightOut` must be `nullptr` — both `runFrame()`-level extras
whose own semantics assume exactly one PASS-2 resolve per frame, not
decided here for field mode's own two independently-resolved parities
sharing one destination index space (ADR-077).

**Files:** `core/resolve.hpp` (new `runFrameField()` declaration),
`core/pipeline.cpp` (new `#include "video/interlace.hpp"`; new
`runFrameField()` definition, reusing the existing private
`resolveOneTile()` unchanged — no new PASS-2 arithmetic), `tests/
test_field_pipeline.cpp` (new); plus `CMakeLists.txt`
(`scatter_test(test_field_pipeline)` registered — not counted against the
file cap).

**Accept:** the check WU-23a's own first Accept: criterion deliberately
deferred (ADR-075, `HANDOFF.md`'s Session-43 entry) — a marked interlaced
test frame, warped through field mode under the identity lattice,
reproduces the original frame bit-exactly (all three planes — this path
never leaves 4:4:4, so unlike a v210 round trip there is no chroma
filter to make Cb/Cr merely legal rather than exact) once both fields'
outputs are combined via `runFrameField()`; and a wiring/accounting
check under a real, off-centre, magnifying affine warp, confirming the
driver's own assembly (two `generateFragmentsFieldRows()` calls, two
PASS-2 resolves, two `extractField()` calls, one `interleaveFields()`
call) does not silently duplicate or drop rows, checked against an
independent recomputation through the same public primitives
(`generateFragmentsFieldRows()`, `splatTile()`, `sumBanks()`,
`composite()`), both as a whole-frame equality and row by row against
each parity's own independently-extracted reference.

*Status:* built and verified in this session's own Linux cloud sandbox
(Ubuntu 24.04, a fresh clone of `origin/main` at `wu-23a2a-green`/
`c07f38b`, confirmed clean before any file was touched). Full
8-configuration matrix (GCC 13.3.0 / Clang 18.1.3, Release/Debug,
`SCATTER_TILE_LOG2` 4/5) all green, zero warnings under this project's
full `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Werror`
set, plus GCC 13 `-fsanitize=address,undefined -fno-sanitize-recover=all`
at both tile sizes, clean. No AArch64 cross-compile — this unit touches
no platform-specific surface, the same scope WU-16a/WU-19a/WU-23a/
WU-23a2a's own portable-only units already used. `ctest`: 25 of 25 (24
pre-existing, unaffected, plus this unit's own new `test_field_pipeline`).
`test_field_pipeline` alone: 27654 checks passing. Steve's own
real-terminal build/`ctest` and manual tag/push are the remaining steps —
see `HANDOFF.md`, and see `CORRECTIONS.md` C-024 for why this is a manual
tag, not `./tools/close.sh 23a2b`.

**Phase 6's own field-mode thread (WU-23a/WU-23a2a/WU-23a2b) is
complete.** WU-23b split into WU-23b1 (filter core, `video::Deinterlacer`,
scoped `DECISIONS.md` ADR-078, built `DECISIONS.md` ADR-079, tagged
`wu-23b1-green`) and WU-23b2 (live-capture wiring), itself split into
WU-23b2a (the new `runFrameBytesDeinterlaced()` orchestration entry
point — scoped `DECISIONS.md` ADR-080, built and cloud-sandbox green,
Steve's own real-terminal build/tag/push landed as `wu-23b2a-green`) and
WU-23b2b (`CaptureConsumer` wiring — scoped `DECISIONS.md` ADR-080,
extended by this session's own ADR-081, written this session against
WU-23b2a's own actual built interface, entirely unbuilt — this unit is
DeckLink-linked, so no compiler in this project's own cloud sandbox can
check it; Steve's own real-terminal build is its first compile of any
kind).

### WU-23b1 — Weston 3-field de-interlace: filter core, `video::Deinterlacer` `built, unverified`
Built this session (`DECISIONS.md` ADR-079). Scoped in the immediately
preceding session (`DECISIONS.md` ADR-078) after confirming the real
source (`libavfilter/vf_w3fdif.c`, BBC R&D's Weston 3-field algorithm —
Jim Easterbrook's implementation of Martin Weston's process, not
`vf_bwdif.c`, a materially more complex, later filter Steve confirmed is
out of scope) and resolving the multi-frame-history question
ADR-075/`HANDOFF.md`'s own Session-45 entry left open: no persistent
cross-frame state exists anywhere in this project today
(`io/decklink_capture_consumer.cpp`'s `processOne()` and every
`core/resolve.hpp` entry point are stateless per call), so this unit is
a new, standalone, DeckLink-independent component owning its own
three-frame (`prev`/`cur`/`next`) history internally.

**Design, corrected this session against the real source
(`CORRECTIONS.md` C-025, `DECISIONS.md` ADR-079) from the immediately
preceding session's own scoping, which described a field-native,
single-parity input that could not actually implement the algorithm:**
`push()` takes one full-height ("weave") interlaced `Raster444` per call
— the same shape `video::extractField()`'s own `frame` parameter already
uses, both parities' rows genuinely present, not a half-height
already-split field. A row already at the output's own fixed anchor
parity (`FieldParity`, chosen at construction) is copied through
unchanged; a missing-parity row is reconstructed as a spatial low-pass
over `cur`'s own nearby anchor-parity rows plus a temporal high-pass
summing `cur`'s and `prev`'s own missing-parity rows at the same
positions — always `prev`, never `next`, in this project's frame-rate-only
mode (`next` is buffered only to become `cur` on the following push) —
both coefficient sets (simple: 2 low-pass + 3 high-pass taps; complex: 4
low-pass + 5 high-pass taps) scaled 2^15 per the real source, descaled by
a round-half-up 15-bit shift (the same idiom `core/types.hpp`'s
`toCode10()` and `video/chroma.cpp`'s `roundShift()` already use, and the
same "narrow, do not clamp" I2-compliant handling `video/chroma.hpp`
already documents for a negative-lobe integer filter); accumulation in a
signed 64-bit accumulator per I4/I6. Edge handling: out-of-range
full-frame row indices reflected back into range (repeatedly ±2),
adopted as-is, scoped to this filter alone — not reconciled with
ADR-018/020/022's own different conventions, which govern different
subsystems. All three planes (Y, Cb, Cr) treated identically, matching
the real source. Frame-rate output (one reconstructed full-height
`Raster444` per input weave frame, once two frames' worth of history
exist, `push()`'s own `bool` return signalling whether `outFrame` was
written), not field-rate — confirmed against `docs/architecture.md`
section 3's own signal-path diagram.

**Files:** `video/deinterlace.hpp` (new), `video/deinterlace.cpp` (new),
`tests/test_deinterlace.cpp` (new); plus `CMakeLists.txt`
(`scatter_test(test_deinterlace)` registered — not counted against the
file cap).

**Accept:** the coefficient sets, the low-pass/high-pass row-offset
structure and the edge-reflection logic all re-verified directly
against the real `libavfilter/vf_w3fdif.c` source at build time (not
taken from ADR-078's own paraphrase alone) before being frozen in
committed code — done this session, `DECISIONS.md` ADR-079, with the
coefficient sum properties (low-pass taps sum to unity, high-pass taps
sum to zero) additionally encoded as `static_assert`s in
`video/deinterlace.cpp` itself. A synthetic weave-frame-sequence test
constructed independently of `video::Deinterlacer`'s own implementation
(a separately-written reference function in `tests/test_deinterlace.cpp`,
not calling into `video/deinterlace.cpp`) matches its full-frame output
exactly, for both coefficient sets, over a 5+-push sequence. The
three-history-frame state machine is checked directly: the first frame
pushed produces no output; the second produces one reconstructed frame;
the same 5+-frame sequence produces exactly one output per input from
the second onward, with `prev`/`cur`/`next` verified (via a
distinguishing per-frame marker baked into each pushed frame's own
content) to hold the correct three frames at each step — `cur`/`prev`
directly, since both are read into every output; `next` indirectly, by
confirming it becomes `cur` (visible via that push's own anchor-row
output) on the following call, since it is never read directly into any
single call's own result (`CORRECTIONS.md` C-025) — not just
plausible-looking output. Edge rows (top and bottom of a frame) reflect
correctly, checked against a directly-computed expected value for at
least one edge case per coefficient set, both as part of the full-frame
independent-reference comparison above and as an explicit standalone
check.

*Status:* built this session, unverified — needs a real build/`ctest`
run (GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 2^4 and 2^5,
plus GCC 13 ASan/UBSan at both tile sizes, this project's own standard
portable-unit matrix) before `wu-23b1-green`. See `HANDOFF.md`.

### WU-23b2a — Weston 3-field de-interlace: `runFrameBytesDeinterlaced()` orchestration entry point `built, cloud-sandbox green`
Scoped this session (`DECISIONS.md` ADR-080), splitting WU-23b2 per
`SESSION-PROTOCOL.md`'s 3-source-file cap — the full wiring touches four
source files, one over the cap, and splits along the seam ADR-080's own
design already drew: the new orchestration entry point first, the
`CaptureConsumer` wiring after (WU-23b2b, below), the same "scope/build
the component before the thing that wires it" sequencing
WU-23a2a/WU-23a2b and WU-23b1/WU-23b2 already used one level up.

**Design (`DECISIONS.md` ADR-080):** `CaptureConsumer::processOne()`
cannot call `video::Deinterlacer::push()` "ahead of the existing warp"
as WU-23b2's own original (pre-split) scoping stub assumed —
`processOne()` calls `scatter::runFrameBytes()` exactly once, and that
function's own chroma-upsampled weave `Raster444` (exactly `push()`'s
own required input shape) is a local variable never exposed to any
caller (`CORRECTIONS.md` C-027). This unit adds a new sibling entry
point instead — `bool runFrameBytesDeinterlaced(video::Deinterlacer&
deinterlacer, const Lattice& lattice, const std::uint8_t* srcBytes,
std::ptrdiff_t srcRowBytes, int srcWidth, int srcHeight, const
PipelineParams& params, std::uint8_t* dstBytes, std::ptrdiff_t
dstRowBytes)` — that unpacks v210 and upsamples chroma exactly as
`runFrameBytes()` already does, pushes the resulting weave frame into
`deinterlacer`, returns `false` with `dstBytes` untouched if `push()`
returns `false` (mirrors `push()`'s own contract and `runFrameFile()`'s
own bool-return precedent, ADR-079), otherwise runs `runFrame()` on the
reconstructed progressive frame and chroma-downsamples/packs into
`dstBytes` exactly as `runFrameBytes()` already does. `deinterlacer` is
a reference parameter, not a new `PipelineParams` field — a
single-caller-only field there would repeat the scope creep ADR-078
already declined by giving `Deinterlacer` its own file. The output-side
"[re-interlace]" stage (`docs/architecture.md` section 3) is *not*
implemented as an `extractField()`/`interleaveFields()` call — ADR-080
proves that composition is an exact no-op for this project's own
frame-rate-only mode (section 5's "Interlace" note frames
de-interlace-to-frame and field mode as alternatives, never combined),
so `warped` goes straight to chroma downsample with a comment citing the
ADR.

**Files:** `core/resolve.hpp` (edited: new `#include
"video/deinterlace.hpp"`, new `runFrameBytesDeinterlaced()`
declaration), `core/pipeline.cpp` (edited: new definition); plus
`tests/test_pipeline_bytes.cpp` (edited, not counted against the cap —
WU-21a's own portable `runFrameBytes()` test file, the natural home for
its new sibling).

**Accept:**
- A freshly constructed `Deinterlacer`'s first push through
  `runFrameBytesDeinterlaced()` leaves `dstBytes` completely unchanged
  from its own pre-call contents and returns `false` — checked byte for
  byte, not just "the call doesn't crash".
- From the second push onward, output matches an independently-composed
  reference built in the test file itself (`unpackImage` →
  `upsampleImage` → `deinterlacer.push()` → `runFrame()` →
  `downsampleImage` → `packImage`, called by hand, not through the unit
  under test) byte for byte — the same "check against an independently
  composed sequence, not the unit's own internals" discipline
  `test_pipeline_bytes.cpp`'s existing `runFrameBytes()`-vs-
  `runFrameFile()` check already uses.
- The re-interlace no-op finding is checked directly, not merely
  assumed from the ADR's own proof: building the same output via an
  explicit `extractField()` ×2 + `interleaveFields()` pass over
  `runFrame()`'s own warped output produces byte-identical `dstBytes` to
  the no-op path actually shipped.
- I7 does not apply directly (a de-interlaced round trip is lossy by
  construction — the non-anchor parity's real rows are discarded and
  reconstructed, ADR-079) — substituted with an anchor-parity-only
  round-trip check: for a synthetic weave frame whose anchor-parity rows
  already equal a known test pattern exactly, those rows survive
  `runFrameBytesDeinterlaced()` unchanged under an identity lattice,
  isolating this unit's own wiring correctness from WU-23b1's own
  already-tested reconstruction math.
- 625i50 geometry (720×576) exercised directly, per Steve's own
  stay-in-SD-domain scope decision — not 1080i.

*Status:* built and tested this session — `core/resolve.hpp`/`core/pipeline.cpp`
implement the frozen ADR-080 design exactly (no redesign, no new
`DECISIONS.md`/`CORRECTIONS.md` entries needed), and
`tests/test_pipeline_bytes.cpp` was extended with four new checks
covering every bullet of this entry's own `Accept:` line above,
including a direct check of the re-interlace no-op claim against an
explicit `extractField()`x2 + `interleaveFields()` pass (not merely
trusting ADR-080's own algebra) and a 625i50 (720x576) geometry pass,
per Steve's own stay-in-SD-domain scope decision. All 10 configurations
of this project's own standard portable-unit matrix (GCC 13.3.0 and
Clang 18.1.3, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5, plus GCC
13 `-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes) are green in this project's own Linux cloud sandbox. Cloud-sandbox
green is not real-terminal green (`SESSION-PROTOCOL.md`'s own "sandbox
edits are not delivered until pushed" discipline) — needs Steve's own
real-terminal `cmake --build`/`ctest` run and manual tag before
`wu-23b2a-green`. See `HANDOFF.md`.

### WU-23b2b — Weston 3-field de-interlace: `CaptureConsumer` wiring `written, entirely unbuilt`
Scoped this session (`DECISIONS.md` ADR-080) alongside WU-23b2a, above —
genuinely depends on WU-23b2a's own actual `runFrameBytesDeinterlaced()`
signature, now built and cloud-sandbox green. Written this same session,
once WU-23b2a's own interface was confirmed directly against the real
header. **This unit is DeckLink-linked (`io/decklink_capture_consumer.hpp`/
`.cpp` link the Blackmagic DeckLink SDK) — no compiler anywhere in the
cloud sandbox this session drafted it in can see this code, at all, under
any configuration. "Written" here means exactly that and nothing more:
reasoned through against the real, current header/source and against
WU-23b2a's own actual built interface, not built, not tested, not
cloud-sandbox green. Steve's own real-terminal `cmake --build` is this
code's first compile of any kind.** See `HANDOFF.md` for the full
account.

**Design (`DECISIONS.md` ADR-080, extended by ADR-081):** `CaptureConsumer`
owns exactly one `video::Deinterlacer` member — not two; a single
instance already produces one full progressive frame per push with every
row present (anchor parity verbatim, the other reconstructed), and a
second, opposite-anchor instance would only earn its keep for field-rate
output, which ADR-078 already ruled out — constructed with
`FieldParity::Top` (matching `video/interlace.hpp`'s own "Top is
first-transmitted field" convention already used without incident
through WU-23a/WU-23a2; this project has never independently confirmed
`bmdModePAL`'s own SDK-reported field dominance against real hardware,
but which parity is "anchor" is a labelling choice, not a correctness
requirement) and whichever `DeinterlaceCoefficients` the constructor is
given. **`DeinterlaceCoefficients`: `Complex`, Steve's own explicit
choice, raised and settled this session per ADR-080's own instruction not
to default it silently — see `DECISIONS.md` ADR-081.** `CaptureConsumer`'s
constructor takes it as a required parameter, no default, so every
caller (including `tests/test_decklink_capture_consumer.cpp`) states it
explicitly. `processOne()` calls `scatter::runFrameBytesDeinterlaced()`
in place of `scatter::runFrameBytes()`; its own `false` return (stream
start — the very first popped frame of this consumer's own lifetime
only, per ADR-080's trace of `Deinterlacer`'s own state machine, never a
recurring cost) is communicated to `run()` as a genuine third outcome of
a new private `ProcessResult` enum class (`Processed`/`Failed`/
`StreamStart`) that `processOne()` now returns in place of `bool` —
ADR-081 records why an enum was picked over a second `bool` or an
out-parameter. On `StreamStart`, `m_latestFrame` is left completely
untouched (extending, not inventing, `copyLatestFrame()`'s own existing
"nothing produced yet" semantics) and no coverage callback fires; `run()`
counts it against a new, fourth `CaptureConsumerStats` counter,
`framesStreamStart` — not `framesFailed` — this is not an error and is
never retried.

**Files:** `io/decklink_capture_consumer.hpp` (edited: new owned
`video::Deinterlacer` member, new stats counter, new
`DeinterlaceCoefficients` constructor parameter), `io/decklink_capture_consumer.cpp`
(edited: constructor, `processOne()`); plus
`tests/test_decklink_capture_consumer.cpp` (edited, not counted against
the cap — already real-hardware-gated exactly as this unit needs).

**Accept:**
- Real-hardware run (UltraStudio Monitor 3G → Recorder 3G loopback, this
  project's own established rig, ADR-037) at 625i50 — Steve's own
  stay-in-SD-domain standard, not 1080i: the new stream-start counter
  increments exactly once, before `framesProcessed` ever increments, for
  a freshly constructed `CaptureConsumer`, and `copyLatestFrame()`
  returns `false` until it does.
- Without a loopback connected, `CaptureSource::create()`/
  `CaptureConsumer::start()`/`stop()` still run cleanly (the mechanics
  this unit's own automated checks gate on) — the same "nothing plugged
  in right now is a real, honestly reportable state" convention
  `tests/test_decklink_capture_consumer.cpp`'s own existing header
  comment already documents; `stats()` may legitimately stay at zero in
  that case, warned about rather than failed on.

*Status:* written this session — `io/decklink_capture_consumer.hpp`/`.cpp`
implement the frozen ADR-080 design plus this session's own ADR-081
(`DeinterlaceCoefficients::Complex`, the `ProcessResult` enum and
`framesStreamStart` counter), and `tests/test_decklink_capture_consumer.cpp`
was updated for the new required constructor parameter and the
three-term accounting invariant. **Entirely unbuilt — no compiler in
this project's own Linux cloud sandbox can see DeckLink-linked code at
all; this is reasoned-through-against-the-real-source, not verified.**
Steve's own real-terminal `cmake --build`/`ctest` is this code's first
compile of any kind — a clean pass is not guaranteed the way it was for
WU-23b2a. See `HANDOFF.md` for the exact build/test/commit/tag/push
sequence.

### WU-24 — Adaptive supersampling `todo`
### WU-25 — 1080p25, then 1080p50; tile-size tuning `todo`

---

## Phase 7 — Starlight

### WU-26 — Normals from lattice `green` (`wu-26-green`)
Built and cloud-sandbox-tested this session (Session 37); not yet Steve's
own real-terminal `green` — see *Status* below. See `DECISIONS.md` ADR-063
for the full design and derivation. Hard
prerequisite for WU-28c (self-fold facing tag, below in Phase 7's own
listing): `core/lattice.hpp`'s own header comment already reserved dz/du,
dz/dv and the cross-product normal for this unit specifically (see
`DECISIONS.md` ADR-062 for how that dependency was found); this unit builds
exactly that.

Design: `Jacobian` gains `dzdu`/`dzdv`, populated in `core/lattice.cpp`'s
`jacobian()` by storing the z component of the `du`/`dv` blends it already
computes (no new lattice evaluation). `core/jacobian.hpp` gains
`surfaceNormal(const Jacobian&)`, the third Jacobian-derived quantity that
file's own header comment already reserved a slot for — the cross product
`Tv x Tu` of the two 3D tangent vectors, *not* `Tu x Tv`: this project's
z-increases-into-screen convention (`core/shapes/shapes.hpp`, ADR-027) needs
a front-facing point's normal to read `normal.z < 0`, and `Tv x Tu` is the
order that gives that sign, checked against a real `buildSphereLattice()`
lattice at its front-most and (fully self-folded, `angleSpanH == 2*pi`)
antipodal control vertices — see ADR-063 for the derivation. Not normalised
to unit length: the one consumer scoped so far (WU-28c, now `green`) needs
only the sign of `normal.z`.

**Files:** `src/core/lattice.hpp` (two new, additive `Jacobian` fields),
`src/core/lattice.cpp` (`jacobian()` stores what it already computes rather
than discarding it), `src/core/jacobian.hpp` (new `surfaceNormal()`,
additive, alongside `densityCompensation()`/`ewaFootprint()`);
`tests/test_jacobian.cpp` (existing `checkJacobianAt()` extended to dz/du,
dz/dv; two new test functions for `surfaceNormal()`). No `CMakeLists.txt`
change needed — `test_jacobian` already links the full `scatter-core`
library, `src/core/shapes/sphere.cpp` included. +184/-27 lines across the
four files.

**Accept:** `jacobian()`'s analytic dz/du, dz/dv agree with central
differences of `eval().z` to 1e-6 relative, across the lattice interior, its
edges, corners and interior knots — WU-06's own method and tolerance,
extended to z. `surfaceNormal(j).z` equals `-(j.dxdu * j.dydv - j.dxdv *
j.dydu)` (the existing 2x2 Jacobian determinant, sign-flipped) for arbitrary
lattice content, an algebraic identity of 3D cross products independent of
dz/du, dz/dv. Against a real `buildSphereLattice()` lattice with
`angleSpanH == 2*pi`: `surfaceNormal()` at the front-most control vertex has
`normal.z < 0`; at the antipodal (self-folded) control vertex, `normal.z >=
0`.

*Status:* `green` (`wu-26-green`) — green in the cloud sandbox Session 37 —
fresh clone of `origin/main` confirmed at `wu-28b-green`/`5ba60b3` before any
file was touched; `cmake --build build` clean; full portable `ctest` suite
22/22
passing, no regressions; `test_jacobian` itself 551 checks passing.

**Status-line correction, WU-36 (this sweep):** this line still said
`wip`, awaiting real-terminal confirmation. `git tag`/`git log` against the
real repository confirm `wu-26-green` exists on commit `4381823` ("WU-26:
normals from lattice (ADR-063); C-021 tag-before-commit correction") — the
same commit `CORRECTIONS.md` C-021 already documents as the *corrected*
tag target (the original tag-before-commit mistake was fixed same
session). Session 41's own handoff already fixed a stale parenthetical
elsewhere in this entry but did not update this header word; done here.
Not touching any source file.

**Note (WU-32, documentation-only, `docs/sources/WU-SM-02.md` §6):** the
`∂z` this unit adds to `Jacobian` gates three separate downstream
consumers, not one — Starlight shading's surface **normal** (WU-27), the
front/back source select's normal **sign** (already consumed by WU-28c;
ADR-073's video-source pair, WU-33, will need it too), and a future sheet-
arbitration depth **gradient** for same-sheet tolerance (ADR-072, WU-35).
Recorded here so a future session does not have to rediscover why this
small, already-shipped unit turned out to matter this much. Not a change to
this unit's own `Files:`/`Accept:` or `Status:` above — WU-26 is not
reopened.

### WU-27 — Phong lighting evaluator (closed-form, no coarse grid) `green`
**Renamed Session 41 (WU-32, `DECISIONS.md` ADR-069) — was "Blinn-Phong,
linear light, two-sided"; re-scoped and built this session (`DECISIONS.md`
ADR-082) — was "Phong, linear light, two-sided".** The "two-sided" half of
the old title described back-facing surfaces still splatting (I8), which is
a `core/binner.hpp` concern, not this unit's own — dropped from the name to
avoid implying this unit itself does anything with facing.

**Scope, settled this session (ADR-082):** WU-27 is the closed-form Phong
illumination *evaluator* alone — `Light`, `Zone`, the `model(L, zone)` LUT
interface (ADR-071), and `shade(scene, P, N) -> I` (ADR-069) — with **no**
`core/binner.hpp` wiring and **no** coarse-grid facet evaluation. ADR-070's
own per-coarse-grid-facet mechanism (filtering ladder, grid shift, and the
facet-normal choice below) is a genuinely different mechanism from a plain
per-sample call, and is WU-34's own job — exactly what WU-34's entry below
already said before this session touched anything ("the coarse grid is
*how* WU-27's Phong formula gets evaluated across a raster, not a separate
lighting model"). This keeps WU-27 at 2 new source files, comfortably under
`SESSION-PROTOCOL.md`'s 3-file cap, with zero changes to
`core/binner.hpp`/`.cpp` this unit.

**Files:** `src/core/lighting.hpp` (new), `src/core/lighting.cpp` (new),
`tests/test_lighting.cpp` (new, doesn't count against the cap). Also
`src/core/jacobian.hpp` (one comment line only, `surfaceNormal()`'s stale
"WU-27's own two-sided Blinn-Phong shading" reference corrected — no
behavioural change; `docs/wu-audit-2026-08.md`'s own finding 4 named this as
the natural moment to fix it), `CMakeLists.txt` (adds `src/core/lighting.cpp`
to `scatter-core` and `scatter_test(test_lighting)`).

**Accept:** `tests/test_lighting.cpp` passes — fixtures 9, 12, 13, 15, 16
(adapted), 17, 18 and 26 from `tests/fixtures-historical.md`
(`docs/sources/WU-SM-01.md` §8), plus `defaultSpecularCurve()` bounds/
monotonicity checks. Fixtures 14 and 16 are adapted, not literal: both
depend on the six-named-lights/two-gantry default control-tree topology
(fixture 8), which needs the axis tree ADR-067 already records as unscoped
— each test instead exercises the structural property WU-27's own scope
actually owns (a Parallel light's direction is fixed and P-independent; a
light's own `zoneIndex` genuinely selects its own zone's curve, independent
of any other light) — see `tests/test_lighting.cpp`'s own header comment.
Full portable matrix green in the cloud sandbox (GCC 13.3.0 and Clang
18.1.3, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes — 10 configurations, all 27 tests passing in each, no sanitizer
findings) — Steve's own real-terminal run is still the final word per
`SESSION-PROTOCOL.md`.

**Design choices this session made and did not have to escalate** (ADR-082
has the full account): straight per-light summation with a single ambient
floor (WU-SM-01 §4.6.4's own flagged-open "obvious reading, but a reading"
— the only reading fixtures 9/12 are consistent with); Point/Parallel
clamp cosA and cosB to `[0, inf)`, Beam uses `|cosA|`/`|cosB|` unclamped
(fixture 13's own bidirectionality, the one part of the general clamping
question WU-SM-01 §4.6.4 does *not* leave open); specular gated by the same
clamped cosA being positive.

**Escalated to Steve, both settled before building (ADR-082):** (1) the
coarse-grid facet normal (ADR-070's own open question) — historical
finite-difference reconstruction, not WU-26's analytic `surfaceNormal()`;
binds WU-34, not this unit's own code, but logged here since it was decided
during this unit's own scoping session. (2) the `model(L, zone)` LUT —
build it now with placeholder curves for all eight named models; the
primary patent obtained this session (EP 0248626/US 4,899,295) corroborates
the base formula but does not itself name or tabulate the eight models, so
the blocking Deliverable 5 gap is unchanged.

**Out of scope, left for later units:** wiring `shade()` into
`core/binner.hpp`'s per-sample loop via a coarse-grid facet (WU-34); the
real tabulated curves for the eight named specular models (WU-37,
unblocked in mechanism only, not content, by this session's patent read);
how a caller applies the returned intensity factor to RGB — multiply vs.
S3's own "Spectral MULTIPLY vs ADD" alternative (WU-34's own first
question, not decided here).

**Predecessors:** WU-26 (hard, `surfaceNormal()` — already available).

### WU-28a — k-buffer storage: tag-keyed depth slots (PASS 2 accumulate) `green`
See `DECISIONS.md` ADR-059 for the full scoping session (Session 33 — not
a decision made until now) and the design fork it resolved. Splits from
the single `WU-28` line this replaces the same way WU-16a/b split PASS 2
from PASS 1 (ADR-040/041) and WU-19a/b split the persistent pool from the
diagnostic side channel: the full k-buffer design does not fit
`SESSION-PROTOCOL.md`'s "3 source files" cap without reopening
`core/resolve.hpp`/`.cpp` too, so the accumulate side (this unit) and the
resolve/composite side (WU-28b, below) are separate units, following the
same splat/resolve seam WU-09/WU-10 already established.

Design, per ADR-059: a new, additive per-cell storage shape — up to
`kBufferK` (4) depth-tagged slots, keyed by `Frag::tag`, alongside (not
replacing) today's single-`AccumCell`-per-cell path, so every unit that has
never needed multi-surface handling (every existing shape and test) is
unaffected. Same-tag contributions accumulate into their tag's own slot
with exactly today's `accumulateCorner()` arithmetic (order-independent
sum, I6 unchanged) — this sidesteps the tied-`z` determinism risk ADR-059
found, since ties within one surface's own quantised `z` never need
breaking at all. When a fragment's tag has no existing slot in a cell and
all `kBufferK` slots are already occupied by other tags, the farthest-so-far
occupied slot (by first-seen `z`) is evicted and replaced. ADR-059 records
this as an explicit, accepted caveat, chosen deliberately over the fully
order-independent no-cap/truncate-at-resolve alternative: this specific
eviction's outcome, in the case of more than `kBufferK` distinct tags
genuinely overlapping one destination cell, is not proven order-independent
across threading paths.

**Files:** `src/core/types.hpp` (new k-slot record, e.g. `KSlot` — tag,
first-seen `z`, one `AccumCell`; a compile-time `kBufferK` constant),
`src/core/splat.hpp`, `src/core/splat.cpp` (new, additive entry points
alongside — not replacing — `splatTile()`/`sumBanks()`, e.g.
`splatTileKBuffer()`/`sumBanksKBuffer()`, per `SESSION-PROTOCOL.md` rule 2:
existing entry points' names and contracts are frozen), `tests/test_kbuffer_storage.cpp`
(new); plus `CMakeLists.txt` (`test_kbuffer_storage` target added).

**Accept:** for a synthetic tile where multiple tags' fragments land in the
same cells, `--threads 1` output (each cell's set of occupied tag-slots and
their accumulated Y/Cb/Cr/w) is byte-identical to an 8-thread run's output,
for at least one fragment order exercising same-tag ties at identical
quantised `z` and one exercising the >`kBufferK`-distinct-tags eviction
case (this second case's own outcome is checked only for run-to-run
self-consistency at a given thread count, not against an independent
oracle — exactly what ADR-059's own documented caveat concedes it cannot
promise). A cell where only one tag is ever present reproduces today's
plain `sumBanks()` `AccumCell` exactly.

*Status:* `green` — Steve's own real-terminal build/run/commit/tag/push
landed since Session 34 (this status line itself was left at `wip` in that
commit; corrected here, doc-only, by Session 35 while re-verifying
repository state before starting WU-28b, not by touching any of this
unit's own source files). Confirmed directly via the device bridge, not
assumed from this file's own prior text: `git tag` lists `wu-28a-green`;
`git log --oneline -10` shows it at `HEAD` (`5ba1086`, "WU-28a: k-buffer
storage, tag-keyed depth slots (ADR-059/ADR-060)"); `git status -sb` reads
`## main...origin/main` with no ahead/behind marker. Built and tested in
the cloud sandbox first (DECISIONS.md ADR-060): full portable `ctest`
suite (21 targets) green, no regressions, `test_kbuffer_storage` itself
1082 checks passing after ADR-060's own zero-weight-corner fix. See
`HANDOFF.md`.

### WU-28b — k-buffer resolve: depth-ordered opaque/blend composite `green`
See `DECISIONS.md` ADR-059. Consumes WU-28a's per-cell occupied-slot set.
Two related but distinct outcomes, per Steve's own original framing of this
unit's scope (recorded in `WU-28`'s original single-line entry, preserved
here in spirit): opaque occlusion (front tag's slot wins outright, the rest
discarded — the same read-replace-write shape `compositeLayered()` already
has, ADR-028/029, but now driven by depth order among up to k tag-slots
instead of by caller-supplied layer order) and transparency (a
user-controlled blend across the occupied slots, sorted front-to-back by
each slot's first-seen `z` — the blend ratio/formula itself is this unit's
own job to design against real code, not fixed by this scoping session).
Both modes are new, additive `PipelineParams` fields (opt-in, default-off,
zero-cost-when-absent, the same shape `pool`/`weightOut` already
established), not a change to `composite()`'s or `compositeLayered()`'s own
existing contracts.

**Files:** `src/core/resolve.hpp`, `src/core/resolve.cpp` (new function
alongside `composite()`/`compositeLayered()`; new `PipelineParams` fields
for opacity mode and blend control), `src/core/pipeline.cpp`
(`resolveOneTile()` wired to call the new k-buffer resolve path when
WU-28a's storage is in use), `tests/test_kbuffer_resolve.cpp` (new); plus
`CMakeLists.txt`.

**Accept:** for a synthetic multi-tag cell set (from WU-28a's storage),
opaque mode's output matches the nearest tag's own resolved `AccumCell`
exactly; blend mode's output matches the chosen blend formula for a known
ratio; the full pipeline (`runFrame()` end to end) at `--threads 1` is
byte-identical to an 8-thread run for a real folding-sphere frame (WU-21g/h's
own geometry), satisfying I6 for the completed feature, not just WU-28a's
storage step in isolation.

*Status:* built and tested this session (DECISIONS.md ADR-061) — new
`KBufferResolveMode` enum (`Off`/`Opaque`/`Blend`) and the additive
`PipelineParams::kBufferMode` field, `compositeKBuffer()` alongside
`composite()`/`compositeLayered()`, `resolveOneTile()` wired to the new
path in both the `threads<=1` oracle branch and the threaded PASS-2 path.
Configured, built and run directly in this project's cloud sandbox across
GCC 13.3 and Clang 18.1, Release and Debug, tile 2^4 and 2^5, plus
AddressSanitizer+UndefinedBehaviorSanitizer and ThreadSanitizer builds: all
22 portable `ctest` targets green in every configuration, no regressions,
no sanitizer reports. `--threads 1` vs `--threads {2,3,8}` byte-identical
for a real WU-21g/h folding-sphere frame in `Blend` mode (I6, per-pixel
per-channel `CHECK_ONCE`). Sizing ran over `SESSION-PROTOCOL.md`'s own
"~400 lines" figure — 243 insertions across `src/core/resolve.hpp`,
`src/core/resolve.cpp`, `src/core/pipeline.cpp` and `CMakeLists.txt`, plus
333 lines in the new `tests/test_kbuffer_resolve.cpp`, 576 total after one
trimming pass from an initial 638 — flagged plainly rather than cut further
against correctness coverage; see `HANDOFF.md`. **Now confirmed `green`:**
this status line itself was left at `wip` in that commit (`2b9eea4`);
corrected here, doc-only, by Session 37 while re-verifying repository state
before starting WU-26 (the same kind of correction Session 35 made for
WU-28a's own stale status line, not by touching any of this unit's own
source files). Confirmed directly via the device bridge, not assumed from
this file's own prior text: `git tag` lists `wu-28b-green`; `git status -sb`
reads `## main...origin/main` with no ahead/behind marker (pushed, not
local-only). `ctest`'s one failure was the already-accepted
`test_decklink_device`/ADR-035 duplex-check exception (the Monitor 3G is
playback-only), so the manual `git tag -a wu-28b-green ...` fallback was
used, per `SESSION-PROTOCOL.md`, on top of commit `5ba60b3` — the WU-28
scoping commit, not `2b9eea4` itself, since Steve's own close-out ran after
that scoping session landed; this is expected (`close.sh`/the manual
fallback tag whatever `HEAD` is at close time, not a unit's own original
commit) and does not change what the tag certifies about WU-28b's own code.

**Real-content gap found after this session (CORRECTIONS.md C-020,
DECISIONS.md ADR-062): neither WU-28a nor WU-28b's own k-buffer mechanism
is reachable by any real content in this codebase today.** `KBufferResolveMode`
is opt-in and nothing sets it away from `Off` in any live/demo entry point
(`tests/test_decklink_live_sphere.cpp` included), and — the deeper issue —
every `Frag` generated within one `generateFragments()`/`runFrame()` call
carries the same single `PipelineParams::tag`/`std::uint8_t tag` value
(`core/binner.cpp`'s own `frag.tag = tag;`), so a single self-folding
surface's front and back, which the k-buffer keys apart *by tag*, always
collide into the same slot regardless of resolve mode. WU-28a/WU-28b's own
`Files:`/`Accept:` above are unaffected and remain correct for what they
actually cover (synthetic multi-tag slot sets, and I6 across the new
threaded code path) — the gap is a missing piece neither sub-unit's own
scope included: something has to assign *different* tags to a folding
surface's own front and back before the k-buffer has anything to separate.
See WU-28c/WU-28d below.

Both sub-units stay entirely inside `scatter-core` (`core/types.hpp`,
`core/splat.hpp`/`.cpp`, `core/resolve.hpp`/`.cpp`, `core/pipeline.cpp` —
no Blackmagic SDK, no Metal/Cocoa anywhere in the touched set) — per
Steve's own explicit answer during ADR-059's scoping conversation, both are
buildable, runnable and testable directly in the cloud sandbox once someone
actually writes them, unlike every DeckLink/Cocoa-touching unit before
them.

### WU-28c — self-fold facing tag: per-fragment tag from surface normals `green`
See `DECISIONS.md` ADR-062 for the original real-content gap this unit
closes and ADR-065 for this session's own scoping and build. Closes the
real-content gap flagged on WU-28b above: gives a self-folding surface's
front and back *different* `Frag::tag` values, so WU-28a's k-buffer has
something to key apart and WU-28b's resolve modes have more than one
occupied slot to ever act on. **WU-26 (Normals from lattice), this unit's
hard prerequisite, is now genuinely closed** (`wu-26-green` confirmed as an
ancestor of `HEAD` this session, per standing device-bridge discipline) —
the "not yet scoped or built" line this entry previously carried is now
stale and is corrected here, not carried forward.

Facing — front-facing (visible, `normal.z < 0` in this project's
z-increases-into-screen convention) versus back-facing (folded away from
camera, `normal.z >= 0`) — is exactly the sign `core/jacobian.hpp`'s
`surfaceNormal()` (WU-26) supplies per source sample. This unit does not
reopen or duplicate WU-26's own job (computing the normal correctly,
analytically, from the lattice's tangents) — it consumes that result,
called on `lattice.jacobian(u, v)`'s own direct output rather than on the
`pixelJacobian()`-converted value already in scope at the per-sample loop's
tag-assignment point, per ADR-063's own explicit warning about that trap
(see `DECISIONS.md` ADR-065 for why this matters even though, in this
unit's own case, the wrong input would have happened to give the same
sign).

**Files:** `src/core/binner.hpp` (two new, additive declarations —
`generateFragmentsRowRangeTagByFacing()`, `generateFragmentsTagByFacing()`
— alongside today's unchanged single-scalar-`tag` `generateFragments()`/
`generateFragmentsRowRange()`), `src/core/binner.cpp` (the shared
per-sample loop extracted into a new private function template,
`generateFragmentsRowRangeImpl()`, tag-selector-templated so the existing
functions' own behaviour is unchanged and the two new ones reuse the
identical loop with a facing-based selector instead — no new lattice
evaluation), `tests/test_binner.cpp` (one new test,
`test_self_fold_front_and_back_get_different_tags()`, against a real
`buildSphereLattice()` self-fold, `angleSpanH == 2*pi` — the same
front-most and antipodal-fold-boundary control vertices `DECISIONS.md`
ADR-063 and `tests/test_jacobian.cpp` already hand-derive and check the
*sign* at, this time checking the *tag* `generateFragmentsTagByFacing()`
assigns). +196/-14 lines across the three files — within
`SESSION-PROTOCOL.md`'s "3 source files plus its test, ~400 lines" cap.

**Accept:** a real, self-folding `buildSphereLattice()` lattice
(`angleSpanH == 2*pi`), run through `generateFragmentsTagByFacing()`, gives
every fragment decoded back to the front-most source sample (phi == psi ==
0) the caller-supplied `frontTag`, and every fragment decoded back to the
antipodal fold-boundary source sample (phi == -pi, psi == 0) the
caller-supplied `backTag` — checked directly against `Frag::tag`, not
inferred. `generateFragmentsRowRangeTagByFacing()` covering the same rows
in one call agrees exactly (`BinStats` fields identical) with the
whole-raster wrapper, the same row-range/whole-raster equivalence WU-16b/
ADR-041 already established for the plain-tag functions. Every pre-existing
`test_binner` check (WU-08's own three accept criteria, 4.6's supersampling
thresholds, off-raster dropping) still passes unchanged, confirming
`generateFragmentsRowRangeImpl()`'s refactor did not alter the plain-tag
path's own behaviour.

*Status:* `green`. Built and cloud-sandbox-tested Session 39: fresh clone of
`origin/main` confirmed at `wu-21d-green`/`bba3634` before any file was
touched; full 8-configuration matrix (GCC 13.3.0 / Clang 18.1.3, Release/
Debug, tile 2^4/2^5) clean, zero warnings, plus GCC+ASan/UBSan clean; full
portable `ctest` suite 22/22 passing in every configuration, no
regressions; `test_binner` itself 38401 checks passing. **Confirmed at
Steve's own real-terminal, Session 41:** `cmake --build build` clean on
AppleClang/ARM64; `ctest --test-dir build --output-on-failure` 30 of 31
passing, the sole failure `test_decklink_device`'s own `foundDuplexDevice`
check, ADR-035's already-accepted exception, unrelated to this unit.
Independently re-verified this same session in the cloud sandbox against a
fresh clone of `origin/main` at `wu-32-green`/`3798a5f` with these three
files overlaid: GCC 13.3.0 and Clang 18.1.3, Release and Debug, tile 2^4 and
2^5, plus GCC+ASan/UBSan — all clean, `test_binner` 38401 checks passing on
every GCC/Clang Release/tile-2^5 run (10177 on Debug/tile-2^4, a real and
expected count difference by tile size, not a discrepancy), no sanitizer
report. `wu-28c-green` tagged and pushed at Steve's own real terminal this
session — see `HANDOFF.md`.

### WU-28d — wire self-fold occlusion into the live sphere demo `todo`
See `DECISIONS.md` ADR-062. Once WU-28c exists, turns the feature on where
Steve can actually see it: `tests/test_decklink_live_sphere.cpp` currently
constructs `scatter::PipelineParams params;` (around its own
`test_live_playback_manual_sphere_control_letter_keys()`) and never sets
`kBufferMode` away from `Off` — this unit sets it (`Opaque` or `Blend`,
possibly a new letter-key toggle between the two, consistent with WU-21i's
own letter-key control scheme) once WU-28c's per-fragment facing tags make
doing so meaningful. Deliberately kept separate from WU-28c: WU-28c is
core-only (`core/binner.hpp`/`.cpp`, sandbox-buildable/testable, same as
every WU-28-adjacent unit before it), while this unit touches a
Blackmagic-SDK-linked test file and therefore can only be reasoned through
and handed off via the device bridge, never built or run in the cloud
sandbox — the same DeckLink/Cocoa split every other unit in this project
already respects (ADR-059's own explicit reason WU-28a/WU-28b stayed
core-only in the first place).

**Files:** `tests/test_decklink_live_sphere.cpp` only, expected — no other
file should need touching if WU-28c's own API is additive, per the pattern
every k-buffer-adjacent unit so far has kept.

**Accept:** provisional, pending WU-28c's own real API — expected to be a
by-eye criterion (Steve's own real-hardware run: does the folding sphere's
back half now visibly disappear behind its front half, in the same spirit
WU-21i's own by-eye accept criteria for letter-key controls already are),
since no automated test can observe an SDI monitor. Fixed for real once
WU-28c lands.

*Status:* named this session so the second half of the gap (feature built
but never switched on for real content) is not lost either. No code
written. See `HANDOFF.md`.

### WU-29 — Environment map `todo`
**Premise check, WU-36 (this sweep, `docs/wu-audit-2026-08.md`):** F2/ADR-069
fixes the Starlight view vector as orthographic and per-effect-fixed, not
per-pixel. An environment map conventionally implies a reflection direction
that depends on view position, which a fixed view vector may make degenerate
or at least historically inauthentic. Whoever scopes WU-29 must decide
whether a historically-faithful environment map is even meaningful under
ADR-069's fixed view vector, or whether this unit is a deliberate, flagged
departure from strict Starlight reproduction — a design decision, not
resolved by this documentation-only sweep. Not scoped past this note.

### WU-33 — Front/back source pair, selected by facing `todo`
**New Session 41 (WU-32, `DECISIONS.md` ADR-073).** Historical finding:
front and back are two independently freezable video sources selected per
sample by the sign of the surface normal, not a transition A/B pair
(`docs/sources/WU-SM-01.md` §3.9.4.3). scatter-dve already has half of the
mechanism — WU-28c's `generateFragmentsTagByFacing()` computes exactly this
facing sign, today to choose a k-buffer tag rather than a video source. Not
scoped past this note: whoever picks this unit up needs to decide how a
second source raster enters the pipeline (a second `SourceRaster` per
`runFrame()`/`runFrameBytes()` call, most likely) and how `binner.cpp`
samples front vs. back per fragment, consuming WU-26/WU-28c's own facing
signal rather than duplicating it. Depends on WU-26 and WU-28c.

**Scope amendment, WU-36 (this sweep) — F6 confirmed by reading the real
code, not inferred:** every current ingest entry point (`core/binner.hpp`'s
`generateFragments()`/`generateFragmentsRowRange()`/`*TagByFacing()`,
`core/resolve.hpp`'s `runFrame()`/`runFrameBytes()`/`runFrameFile()`,
`core/pipeline.cpp`'s `runThreaded()`) takes exactly one `SourceRaster`.
This unit's real file footprint therefore includes *modifying* those
already-green, frozen-signature files — a new `SourceRaster` parameter
threaded through each — not only adding new ones beside them, which is a
materially larger and more central change than every other Phase-7 unit
built so far (all of which have been strictly additive). On the live-capture
side, `io/decklink_input.hpp`/`.cpp` and `io/decklink_capture_consumer.hpp`/
`.cpp` (WU-20b/WU-21b, both green) would need a second, independent
capture-source-and-consumer pair per ADR-073's own "two independently
freezable" requirement — `CaptureConsumer` today holds exactly one
`Lattice` by value (WU-21f's `setLattice()`) with no notion of a second
video feed at all. None of this is a defect in what shipped; it is why this
unit was flagged as one of the three most consequential open scoping
questions in this sweep's checkpoint summary. Per Ground Rule 3, this does
not move WU-33 earlier or renumber anything — it is recorded here so
whoever picks WU-33 up scopes it against its real footprint from the start
rather than discovering it mid-session.

### WU-34a — Coarse-grid shading field: facet normals, filtering ladder, grid shift `green`
**Split from the single WU-34 line this replaces (`DECISIONS.md` ADR-083),
the same "build the computation module first, wire it in second" seam this
project has now used four times (WU-23a2→a2a/a2b; WU-23b→b1/b2→b2a/b2b;
WU-27/WU-34 itself, ADR-082; this pair).** Historical finding (WU-32,
`DECISIONS.md` ADR-070): Starlight's shading is evaluated once per
coarse-grid facet (a three-adjacent-sample normal, computed in a shifting
window) and interpolated to pixels, with a literal filtering control
(`−2`…`+3`, default `−1`) and a grid-shift control (`0`…`2`) governing how
much interpolation happens and where a discontinuity's shading value
lands. This unit builds exactly that field as a standalone computation —
`core/coarse_shading.hpp`'s `CoarseShadingGrid` — with **zero** changes to
`core/binner.hpp`/`.cpp`; wiring its own `sample()` output into the
per-sample loop is WU-34b below.

**Coarse-grid cell size, settled this session (ADR-083):** the existing
129×129 `core::Lattice` geometry control lattice, not a second,
separately-sized grid — no held source states a shading-specific cell size
(`docs/sources/WU-SM-01.md` §3.9.1), but the same section ties the shading
coarse grid to the same "coarse grid" S1's own shape diagnostics name,
which `docs/architecture.md` §4.1 already identifies with the 129×129
lattice. This was this unit's own first open question, left undecided by
WU-32/WU-27's own sessions.

**Facet normal, settled WU-27's own build session (ADR-082, extending
ADR-070's own open note): historical finite-difference reconstruction, not
WU-26's exact analytic `surfaceNormal()`** — Steve's own explicit choice,
raised directly per ADR-070's own instruction not to default it, matching
this project's demonstrated preference for faithful period reproduction
elsewhere (Weston 3-field over bwdif; `Complex` over `Simple` deinterlace
coefficients). This is what makes the grid-shift control meaningful at all
— it corrects the one-cell attribution artefact this construction
(deliberately) reproduces. **The exact stencil — which two neighbours of P
form the facet — is settled this session (ADR-083): P plus its immediate
forward +u/+v neighbours, backward at the lattice's own last row/column,
attributed to P** — no source names the real stencil, so this is this
session's own reasoned default, the same escalation tier as the cell-size
decision above, not a faithful-reproduction-vs-better tradeoff.

**Files:** `src/core/coarse_shading.hpp` (new), `src/core/coarse_shading.cpp`
(new), `tests/test_coarse_shading.cpp` (new, doesn't count against the
cap). `CMakeLists.txt` (adds `src/core/coarse_shading.cpp` to
`scatter-core` and `scatter_test(test_coarse_shading)`).

**Accept:** `tests/test_coarse_shading.cpp` passes — fixtures 10 (filtering
ladder), 11 (grid shift) and 19 (three-sample facet normal, attributed to P
not centroid) from `tests/fixtures-historical.md`
(`docs/sources/WU-SM-01.md` §8), each checked against an independently
hand-mirrored reimplementation of the production formula, not by calling
into `coarse_shading.cpp`'s own internal helpers (the same discipline
`tests/test_jacobian.cpp` already uses for its own central-difference
checks). 305 checks, all passing. Full portable matrix green in the cloud
sandbox (GCC 13.3.0 and Clang 18.1.3, Release and Debug,
`SCATTER_TILE_LOG2` 4 and 5, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes — 10 configurations, all 28 tests passing in each, no sanitizer
findings) — Steve's own real-terminal run is still the final word per
`SESSION-PROTOCOL.md`.

**Design choices this session made and did not have to escalate** (ADR-083
has the full account): the raw → grid-shift → filter ordering (grid shift
applied to the raw per-vertex field before the filtering ladder groups or
blurs it, not after); the Flat1/Flat2x2/Flat3x3 block-anchor choice
(lower-u/lower-v corner of each block, matching the facet-normal's own
P-not-centroid attribution); the Smooth1/Smooth2 box-blur radius (1 and 2
respectively — an explicit placeholder, the same tier as
`core/lighting.hpp`'s `defaultSpecularCurve()`).

**Out of scope, left for WU-34b:** wiring `CoarseShadingGrid::sample()`'s
output into `core/binner.hpp`'s per-sample loop ahead of `Frag`
construction (I10); applying the returned `I` to RGB — multiply (this
session's own settled default, below) vs. S3's own "Spectral MULTIPLY vs
ADD" alternative; building and owning a per-frame `LightingScene` +
`CoarseShadingConfig` the real pipeline populates (today's only caller is
`tests/test_coarse_shading.cpp`'s own synthetic scenes).

**MULTIPLY-vs-ADD scope, settled this session (ADR-083):** build MULTIPLY
only — matches the well-evidenced base circuit (S5 FIG. 1's own
`STARLIGHT (RGB × I)` stage, `docs/sources/WU-SM-01.md` §3.9.2). ADD mode
(S3's own alternative operator setting) is documented future work, not
built this session — this affects WU-34b's own scope, not this unit's,
logged here because it was decided during this unit's own scoping session,
the same pattern ADR-082 used for the facet-normal decision it recorded.

**Predecessors:** WU-26 (hard — `surfaceNormal()`'s own sign convention is
what `crossTvTu()` deliberately matches, though this unit's own facet
normal is finite-difference, not WU-26's analytic one), WU-27 (hard,
`shade()` — already available).

### WU-34b — Wire coarse-grid shading into `core/binner.hpp`'s per-sample loop `green`
**Built this session (`DECISIONS.md` ADR-084), the half of the original
WU-34 line WU-34a did not build.** Multiplies `CoarseShadingGrid::sample()`'s
own interpolated `I` into a source sample's colour inside
`generateFragmentsRowRangeImpl()` (`core/binner.cpp`), ahead of `Frag`
construction — I10's own binding location, "the *shaded* value... is what
gets splatted" (ADR-068). MULTIPLY only, per WU-34a's own settled scope;
ADD mode is not this unit's job either unless re-scoped.

**The RGB-vs-YCbCr application question this unit's own continuation
prompt flagged as unresolved by any held source is now settled, by Steve
directly, not defaulted (ADR-084 has the full account):** Mirage was
RGB-native internally throughout, with the 4:2:2 YCbCr and analogue/
composite I/O converted to/from RGB at the boundary alone — Steve's own
domain knowledge, not inferred from a document. So "multiply RGB by `I`"
(S5 FIG. 1's own `STARLIGHT (RGB × I)` stage) is implemented as a real,
narrow RGB round trip — convert this one sample's Y/Cb/Cr to RGB, multiply
by `I`, convert back — entirely in the double-precision stretch of this
loop that already exists ahead of quantisation (`sampleBilinear()` through
`toSample()`/`toWeight()`), not a YCbCr approximation of one. Does not
touch I3/I4/I6: those invariants govern the *stored* Y/Cb/Cr and the
integer accumulation path, not this already-floating-point intermediate.
Coefficients: BT601 (Steve's own choice — "this is an SD machine"), with
BT709 built and selectable (`core/binner.hpp`'s new `ColourStandard` enum)
for whenever real HD output work needs it, not used by any caller yet.

**Files:** `src/core/binner.hpp` (edited: forward-declares
`CoarseShadingGrid`, adds `ColourStandard`, adds two new optional trailing
parameters — `const CoarseShadingGrid* shadingGrid = nullptr`,
`ColourStandard shadingStandard = ColourStandard::BT601` — to all five
public entry points), `src/core/binner.cpp` (edited: `applyShading()`, the
RGB round-trip; the multiply call site ahead of `Frag` construction; the
same two parameters threaded through `generateFragmentsRowRangeImpl()` and
all five wrappers). `tests/test_binner.cpp` (edited, doesn't count against
the cap). No `CMakeLists.txt` change needed — both files were already
registered.

**Accept:** `tests/test_binner.cpp`'s two new tests —
`test_shading_multiplies_rgb_intensity_ahead_of_frag_construction()`
(checked against an independent hand-mirrored BT601 RGB round trip, not by
calling `applyShading()`, the same discipline `test_coarse_shading.cpp`
already uses; verified this session to actually catch a regression, not
pass vacuously, by temporarily disabling the multiply and confirming the
test fails, then restoring it) and
`test_shading_grid_defaults_to_null_and_preserves_existing_output()` (an
explicit-nullptr call byte-for-byte identical to the implicit-default
call) — both pass, alongside every pre-existing test in the file
(unmodified, still exercising the default-nullptr path throughout, itself
a regression guard against this unit changing any existing caller's
output). Full portable matrix green in the cloud sandbox (GCC 13.3.0 and
Clang 18.1.3, Release and Debug, `SCATTER_TILE_LOG2` 4 and 5, plus GCC 13
`-fsanitize=address,undefined -fno-sanitize-recover=all` at both tile
sizes — 10 configurations, all 28 tests passing in each, no sanitizer
findings) — Steve's own real-terminal run is still the final word per
`SESSION-PROTOCOL.md`.

**The three open questions this unit's own prior scoping note left are
now resolved, not deferred silently:**

1. **Per-frame `LightingScene`/`CoarseShadingConfig` ownership: genuinely
   deferred, on purpose — new WU-34c below.** This unit's own file-count
   budget (`binner.hpp`/`.cpp` alone) has no room for also wiring
   `core/pipeline.cpp`; `tests/test_binner.cpp` is this unit's only real
   caller, exactly as `WORK-UNITS.md`'s own prior note anticipated.
2. **Threading: resolved, no new synchronisation needed.** Verified
   directly against `core/pipeline.cpp`'s real current code this session
   (not from `core/resolve.hpp`'s own comment, which turned out to be
   stale — see `CORRECTIONS.md` C-031): PASS 1 already runs on
   `params.threads` row-band workers when threads > 1 (WU-16b/ADR-041),
   each reading the one caller-supplied `const Lattice&` concurrently with
   no synchronisation of its own. A `CoarseShadingGrid` follows the
   identical shape — built once by whichever caller owns it (WU-34c),
   passed in as `const CoarseShadingGrid*`, read concurrently by every
   row-band worker exactly like `Lattice` already is. Nothing in
   `binner.cpp` needed new threading code.
3. **Insertion point: confirmed exactly as anticipated.**
   `generateFragmentsRowRangeImpl()` is still the one shared loop behind
   all five public entry points; the hook sits right after
   `sampleBilinear()` returns (still a plain-`double` `Colour`, pre-
   quantisation) and before `Frag frag{}` is built.

Depends on WU-34a (`green`) and WU-27 (`green`, `core/lighting.hpp`).

### WU-34c — Own a per-frame `LightingScene`/`CoarseShadingConfig` in `core/pipeline.cpp`; wire a real `runFrame()` caller `todo`
**New this session, deferred from WU-34b above by its own file-count
budget.** `core/binner.hpp`'s five entry points can now take a caller-
supplied `const CoarseShadingGrid*` (WU-34b, ADR-084), but nothing outside
`tests/test_binner.cpp` builds one — `PipelineParams` (`core/resolve.hpp`)
has no shading fields, and `core/pipeline.cpp`'s `runFrame()`/
`runFrameField()`/`runFrameBytes()`/`runFrameBytesDeinterlaced()`/
`runFrameFile()` do not construct or thread one through to
`generateFragmentsRowRange()`. The natural shape, following the same
"optional, caller-owned, default-off, zero-cost-when-absent" convention
`PipelineParams::pool`/`weightOut`/`kBufferMode` already established: new
`const LightingScene* lightingScene = nullptr` and
`CoarseShadingConfig shadingConfig{}` (plus a `ColourStandard`, defaulting
BT601) fields on `PipelineParams`, with `runFrame()` building one
`CoarseShadingGrid` per call (WU-34b's own threading finding above: no
special treatment needed, same shape as `Lattice`) when `lightingScene` is
non-null, and passing it through to whichever `generateFragments*()` call
that entry point already makes. Not scoped past this note — re-read
`core/pipeline.cpp`'s real current structure directly before writing
`Files:`/`Accept:`, the same discipline this unit's own predecessor used.

### WU-35 — Sheet arbitration v2: transparency-coefficient resolve behind a swappable interface `todo`
**New this session (WU-32, `DECISIONS.md` ADR-072/ADR-074), forward-looking
only — does not touch WU-28a/WU-28b/WU-28c/WU-28d's own files or status.**
scatter-dve's shipped k-buffer resolve (`WU-28a`/`WU-28b`, `ADR-059`–
`ADR-061`) was invented from scratch before this research existed — ADR-059
itself records that the real Mirage patent discloses no general
multi-surface mechanism — and its shipped blend formula has no
transparency coefficient, no `Manual`/`Auto Transp`/`Ext. Key` distinction,
and no depth-gradient sheet tolerance. This unit is where that gets
reconciled: (a) put the arbitration stage behind one swappable interface
(`ADR-074`'s binding requirement — M1/M2/hybrid substitutable without
touching the splat); (b) implement `ADR-072`'s transparency-coefficient
rule (`T` from `Manual`/`Auto`/`Ext. Key`, `Opaque` = `T = 0`) behind it;
(c) derive the same-sheet tolerance from the Jacobian depth gradient
(`ADR-072` §4.1) rather than `WU-28a`'s tag-keyed same-tag assumption alone.
Not scoped past this note — in particular, whether this supersedes
`WU-28b`'s `compositeKBuffer()` outright or sits alongside it as a second,
opt-in resolve mode is this unit's own first design question, not decided
here. **Do not implement any part of this unit as a side effect of scoping
it** — `ADR-074`'s M1/M2 choice is not yet settled (blocked on
`docs/sources/WU-SM-02.md` Task A1, UK 2,158,671 in full) and building
ahead of that would risk exactly the "silently make `Trail` and `Opaque`
work together" mistake `docs/sources/WU-SM-02.md` fixture 31 exists to
catch. Depends on WU-26 (depth gradient), WU-28a/WU-28b (the k-buffer
storage this either extends or sits beside), and (soft dependency, for
validation) WU-33 (front/back source pair — fixture 21/23 need real
front/back content to exercise arbitration against, not just tags).

**Scope amendment, WU-36 (this sweep, `docs/wu-audit-2026-08.md`):** the
continuation prompt that opened this sweep suggested a specific Phase 7
shape — "k=1 soft-z with an equal-depth blend band first; full k-buffer as
a follow-on unit" — as a minimum for a unit named `WU-28`. That describes a
design that was never built; what actually shipped and is green is a k=4,
tag-keyed buffer (`WU-28a`–`WU-28d`, `ADR-059`–`ADR-062`/`ADR-065`), already
reconciled with these findings by WU-32/this sweep's own audit (see
`docs/wu-audit-2026-08.md`, Deliverable 1). Per Ground Rule 2, WU-28a/b/c
are not reopened; per Ground Rule 3, they are not renumbered or restated as
a k=1 design. **This unit (WU-35) remains the correct vehicle** for
reconciling ADR-072/074 with the shipped k=4 design — the continuation
prompt's suggested shape should be treated as superseded context, not
actioned. One further scope item found this sweep: `WU-12b`'s
`compositeLayered()` (page-turn priority-tag opacity) is a *second*,
already-shipped, order-driven arbitration mechanism that ADR-074's
swappable-interface requirement does not mention. Whoever scopes WU-35
should explicitly decide whether `compositeLayered()` moves behind the
future interface too, or is documented as a permanently separate,
page-turn-specific mechanism outside ADR-074's scope — not resolved here.

### WU-37 — Specular model LUTs, stubbed pending the real Starlight patent `todo`
**New WU-36 (this sweep, `docs/wu-audit-2026-08.md`), proposed numbering —
adjust if it collides with a unit named between this sweep and whenever it
is picked up.** Historical finding (ADR-069, WU-SM-01.md §4.6.3, [C] but
well-supported): `model(L, zone)` is almost certainly a look-up table
indexed on `cos B`, matching S5's own chisel circuit's "function
generator... may include a look-up table" idiom. Eight named models are
attested (`Model 1`…`4`, `Ramp`, `Posterise`, `2 ring`, `4 ring`) but their
actual tabulated curves are not — that requires the real Starlight patent,
**EP 0248626 / US 4,899,295**, identified by this project (F1/ADR-069) but
not yet obtained (see Deliverable 5, blocked-work register).

**What can be built now, per the blocked-work register:** the
`model(L, zone)` interface itself — an enum/id selecting one of eight
slots, each a pluggable `cos B -> intensity` LUT (or closed-form stand-in)
— with placeholder curves (e.g. a linear ramp for `Ramp`, a stepped
function for `Posterise`, uniform rings for `2 ring`/`4 ring`) clearly
marked provisional. This unblocks WU-27's own closed-form Phong evaluator
(ADR-069's equation, falloff and fixed view vector are already [A]/settled
independent of the LUT contents) from waiting on the patent at all; only
the *exact tabulated curves* are blocked, not the mechanism.

**Not scoped past this note:** file list, exact placeholder curve shapes,
and how a caller selects a model per `(light, zone)` (ADR-071) are all real
scoping work for whoever picks this up, informed by whatever WU-27 itself
settles on for its own file layout. Depends on WU-27 (or scope together,
per WU-34's own precedent for an overlapping pair) for the interface point
the LUTs plug into.

---

## Phase 8 — Authoring

### WU-30 — Embedded Lua shape programs `todo`
### WU-31 — OSC or WebSocket control `todo`

---

## Phase 9 — RGB-native internal colour (hard cutover)

New this session (WU-38, `DECISIONS.md` ADR-085). Internal colour
representation becomes native RGB throughout the pipeline, superseding
I3's YCbCr-internal design — the 4:2:2 v210 I/O boundary is unaffected
(ADR-005 stands). Decided as a hard cutover, not an incremental/
shadow-path migration: **this phase's internal units (`WU-39`–`WU-43`)
are not expected to leave the build green at the end of each one** — a
deliberate, accepted consequence of that decision (ADR-085 §5), not a
failure to flag as one. "Green after every unit" resumes once `WU-44`
lands.

### WU-38 — Phase 9 kickoff: accept ADR-085, finalise I3/I4, open Phase 9 `green`
Documentation-only, per its own opening brief, the same shape WU-32/WU-36
already established for this kind of unit: Steve reviewed and accepted
`docs/proposals/ADR-085-draft-RGB-native.md` directly this session, over
the incremental/shadow-path alternative, as a hard cutover; this unit
formally enters that decision into the project's own state files before
any production code changes.

**Files:** `DECISIONS.md` (ADR-085 appended — the draft's content accepted
as written, except I4, re-derived this session directly against
`core/types.hpp` rather than assumed either way — see that entry for the
full derivation), `INVARIANTS.md` (I3 replaced with the draft's own RGB
text; I4 replaced with the re-derived text — same numeric bound and
headroom as today, unchanged, not a coincidence: re-derived from first
principles, see ADR-085), `WORK-UNITS.md` (this Phase 9 heading and entry;
`WU-39` through `WU-44` stubbed `todo` from ADR-085 §6's breakdown, `WU-39`
additionally scoped with a real, grepped file list — see its own entry),
`docs/proposals/ADR-085-draft-RGB-native.md` (already existed, uncommitted,
at this session's own start — see `DECISIONS.md` ADR-085's own account;
committed for the first time as part of this unit's own close-out,
alongside the four files above), `HANDOFF.md`.

**Accept:** the four-configuration build matrix (Release/Debug ×
`SCATTER_TILE_LOG2` 4/5) confirmed green in the cloud sandbox this
session — 28/28 tests, all four configurations, actually run rather than
assumed from "no source file touched" alone, matching WU-32/WU-36's own
precedent. See `HANDOFF.md` for the run.

**Not done, and explicitly not this unit's job:** touches no `src/`/
`tests/` file — no part of ADR-085 §6's breakdown is implemented here,
only scoped into the real work-unit entries below (`WU-39`–`WU-44`); does
not rewrite `docs/architecture.md` (`WU-43`'s own job); does not resolve
any of ADR-085 §7's open sub-questions beyond I4 (resolved this session,
see `DECISIONS.md`) — the rest stay open for whoever starts each
downstream unit.

*Status:* `green` — no build-affecting change; see `HANDOFF.md` for the
run.

### WU-39 — `core/types.hpp`: `Frag`/`AccumCell` `Y`/`Cb`/`Cr` → `R`/`G`/`B` rename `todo`
**New this session (WU-38, ADR-085 §6 item 1) — scoped, not built,
including a real repository-wide grep for its own blast radius (not
estimated from ADR-085's own draft, which counted a broader "touches
`Y`/`Cb`/`Cr`" query, not this rename specifically — see `DECISIONS.md`
ADR-085's own scope section for that broader count).** I3/I4 are already
finalised (`DECISIONS.md` ADR-085, `INVARIANTS.md`) — this unit is the
mechanical rename alone: `Frag::Y/Cb/Cr` → `R/G/B`, `AccumCell::Y/Cb/Cr` →
`R/G/B`, `kMaxFragContribution`'s own comment/derivation carried forward
unchanged (I4's bound does not move). `kChromaZero` (`types.hpp`) becomes
dead once no channel needs an achromatic mid-point offset — remove it or
keep it as a documented historical constant is this unit's own small open
call, not decided here.

**Real files referencing `Frag` and/or `AccumCell` directly, grepped this
session (`grep -rlw 'Frag'`/`'AccumCell'` across `src/` and `tests/`,
cross-checked line by line against each match's real context; one false
positive excluded — `src/io/com_ptr.hpp` only *mentions* "AccumCell" in a
naming-convention comment, never uses the type):**
production (10; 2 comment-only) — `src/core/types.hpp`,
`src/core/binner.cpp`, `src/core/binner.hpp`,
`src/core/coarse_shading.hpp` (comment-only: "Frag::Y/Cb/Cr" in its own
file-header prose), `src/core/pipeline.cpp`, `src/core/pipeline.hpp`
(comment-only), `src/core/resolve.cpp`, `src/core/resolve.hpp`,
`src/core/splat.cpp`, `src/core/splat.hpp`;
tests (12) — `tests/test_binner.cpp`, `tests/test_coverage_capture.cpp`,
`tests/test_field_pipeline.cpp`, `tests/test_kbuffer_resolve.cpp`,
`tests/test_kbuffer_storage.cpp`, `tests/test_layered_composite.cpp`,
`tests/test_pageturn.cpp`, `tests/test_row_band.cpp`,
`tests/test_scan_order_invariance.cpp`, `tests/test_smoke.cpp`,
`tests/test_splat.cpp`, `tests/test_zoneplate.cpp`. Smaller, and
differently shaped, than ADR-085's own "21 of 35" figure, which counted
every file touching *any* `Y`/`Cb`/`Cr`-shaped data — including
`Raster444`'s own same-named `Y`/`Cb`/`Cr` planes (`src/video/raster.hpp`),
a distinct struct this unit does not rename (that conversion is `WU-40`'s
own job, at the v210/chroma boundary) — not this rename's own blast
radius specifically.

**Not scoped past this note:** exact field-by-field diff per file, and
whether `tests/test_row_band.cpp`'s `decode()`-by-signature helper (`f.Y`,
`f.Cb` — C-015) needs a shape change beyond the rename, are real scoping
work for whoever picks this up. Depends on nothing upstream; every
downstream unit (`WU-40`–`WU-44`) depends on this one landing first, per
ADR-085 §5's own ordering.

### WU-40 — `src/video/v210.cpp`/`chroma.cpp`/`.hpp`: RGB boundary conversion, both directions `todo`
**New this session (WU-38, ADR-085 §6 item 2).** Adds the new conversion
stage ADR-085 describes: v210 unpack → chroma upsample → **new:
YCbCr→RGB** on the input side; **new: RGB→YCbCr** → chroma downsample →
v210 pack on the output side. `Raster444` (`src/video/raster.hpp`) keeps
its own `Y`/`Cb`/`Cr` planes exactly as today up to and including chroma
upsample (unaffected by `WU-39`'s rename — a distinct struct) — this unit
is where a new RGB-shaped container gets produced from it, once, at the
boundary; whether that is a new struct, a repurposed `Raster444`, or
something else is this unit's own first design question, not decided
here. I2's clip-to-protocol-limits behaviour almost certainly stays
exactly where it is today, at the YCbCr boundary immediately around pack/
unpack, since v210's protocol limits (codes 4–1019) are inherently YCbCr
code values with no literal RGB equivalent — flagged for confirmation,
not decided here either. Depends on WU-39.

### WU-41 — `src/core/binner.cpp`/`.hpp`: `sampleBilinear()` reads RGB; `applyShading()` simplifies `todo`
**New this session (WU-38, ADR-085 §6 item 3).** `sampleBilinear()` reads
RGB directly once `WU-40` delivers an RGB-shaped source. WU-34b's
`applyShading()` (ADR-084) simplifies from a full YCbCr→RGB→multiply→
YCbCr round trip to a bare per-channel multiply by intensity, since the
colour arriving at that call site is already RGB. `ColourStandard`/
`coeffsFor()` (BT601/BT709, ADR-084) no longer belong solely to shading —
the I/O boundary (`WU-40`) needs them too; where they should live
(promoted to a shared colour-conversion module near `types.hpp` is
ADR-085's own suggestion, not decided here) is this unit's own first
design question. Depends on WU-39, WU-40.

### WU-42 — `src/core/resolve.hpp`/`.cpp`: PASS 2 reshaped to R/G/B `todo`
**New this session (WU-38, ADR-085 §6 item 4).** PASS 2's splat/
accumulate/normalise/composite math is written in terms of `Y`/`Cb`/`Cr`
fields today; reshapes to `R`/`G`/`B`. The arithmetic *shape* (weighted
accumulate, divide by weight, offset-binary-safe) does not change per
ADR-085/I4's own re-derivation — only what the three channels mean does.
Depends on WU-39.

### WU-43 — `docs/architecture.md`: Design invariants table (§2) and signal-path diagram (§3) rewritten for RGB `todo`
**New this session (WU-38, ADR-085 §6 item 5).** Documentation-only
against already-landed code — picked up once `WU-39`–`WU-42` are in, so
the document describes what actually shipped rather than the plan.
Depends on WU-39, WU-40, WU-41, WU-42.

### WU-44 — ~21 dependent test files re-derived for RGB, worked through in natural clusters `todo`
**New this session (WU-38, ADR-085 §6 item 6) — this phase's own last
unit; the "green after every unit" suspension (ADR-085 §5) ends here.**
Every fixture whose expected values encode the colour space under test
needs re-deriving, not just recompiling — matching WU-34b's own "mirror
the math independently, never call the production function" test-design
precedent (ADR-084), per ADR-085's own stated preference over a
mechanical fixture transform. Worked through in clusters rather than one
file at a time, since fixture values need re-deriving together per
cluster (ADR-085 §6): v210/chroma; binner/EWA/jacobian; resolve/kbuffer/
layered-composite; pipeline/threading/row-band/field; lighting/
coarse-shading. The real file list is `WU-39`'s own grep plus whichever
further files `WU-40`–`WU-42` touch beyond `Frag`/`AccumCell` directly
(`Raster444`-based fixtures in particular) — not re-counted here; whoever
starts this unit should re-grep rather than trust either this note or
ADR-085's own "21 of 35" estimate. Depends on WU-39–WU-42 (the production
code every fixture's expected value is checked against). Phase 9 is
complete, and "green after every unit" resumes, once this unit lands.

---

## Cross-cutting documentation units

Units that import external research into `DECISIONS.md`/`INVARIANTS.md`/
`WORK-UNITS.md` rather than building a phase's own feature. Numbered in the
same sequence as every other unit (`SESSION-PROTOCOL.md` was not amended to
add a separate series for these), listed here rather than under any one
phase because their own content spans several.

### WU-32 — Import WU-SM-01/WU-SM-02 historical findings `green`
Documentation-only, per its own opening brief: promoted the
implementation-binding subset of two research documents
(`docs/sources/WU-SM-01.md` draft 0.3, `docs/sources/WU-SM-02.md` draft 0.1)
derived from primary Quantel sources into this repository's own state
files, plus one standing regression test — no other production code.

**Files:** `DECISIONS.md` (ADR-066–ADR-074), `INVARIANTS.md` (I8–I11),
`CORRECTIONS.md` (C-022, C-023), `WORK-UNITS.md` (this Phase 7 restructure:
WU-26 note, WU-27 renamed, WU-33/WU-34/WU-35 added, this entry),
`docs/architecture.md` (§0/§4.7/§10/§13 corrected per C-022),
`docs/sources/WU-SM-01.md`, `docs/sources/WU-SM-02.md` (dropped in verbatim,
own numbering intact), `docs/sources/README.md` (ADR-SM-nnn → ADR-nnn
mapping table), `tests/fixtures-historical.md` (new: the 32 fixtures from
both research documents, fixture → owning WU → status), plus one test:
`tests/test_scan_order_invariance.cpp` (new; `CMakeLists.txt` gains its
`scatter_test()` registration).

**Accept:** the four-configuration build matrix (Release/Debug ×
`SCATTER_TILE_LOG2` 4/5) green under `-Werror -Wconversion
-Wsign-conversion`, trivially, since no existing production source file is
touched; `test_scan_order_invariance` passing; no change to any other
test's result. See that test's own file header for what it does and,
importantly, does not cover — full four-direction `(u, v)` scan-order
invariance (`docs/sources/WU-SM-02.md` fixture 29) needs a column-traversal
parameter `core/binner.cpp` does not expose today, and adding one would be
production code beyond this unit's own "documentation-only" scope; this
unit's test covers the row-traversal axis only, honestly narrowed rather
than overclaimed.

**Not done, and explicitly not this unit's job:** does not implement the
arbitration stage (ADR-074); does not touch WU-28a/WU-28b/WU-28c/WU-28d's
own files or status; does not promote `docs/sources/WU-SM-01.md`'s other
proposed ADR-SM entries (`001`, `002`, `004`–`008`, `010`, `013`) — only the
nine items the opening brief named as implementation-binding were promoted,
each with its own `ADR-0NN` above; the rest stay `Proposed` in the source
document only, not mirrored into `DECISIONS.md`, until a future unit needs
them.

*Status:* `green` — no build-affecting change, so the sandbox matrix and
`test_scan_order_invariance` are the whole test surface; see `HANDOFF.md`
for the run.

### WU-36 — Work-unit re-plan sweep against WU-32's findings `green`
Documentation-only, per its own opening brief: audited every one of the 53
units then in `WORK-UNITS.md` against F1–F10 (WU-32's own findings),
produced an audit register with a verdict per unit, a revised Phase 7 plan,
a green-tag review-candidate list, a dependency graph, and a blocked-work
register. Read the real source tree directly for every finding that could
only be confirmed that way (F5 leakage, F6 architecture, F4 pipeline order,
F7 culling, F10 ordering) rather than reasoning from the ADRs alone. No
green tag reopened; no production code touched.

**Files:** `docs/wu-audit-2026-08.md` (new — the full audit register plus
Deliverables 3/4/5), `WORK-UNITS.md` (this Phase 7 amendment: WU-27 scoped
for the first time, WU-29/WU-33/WU-35 amended with scope notes, WU-37
added, this entry), `HANDOFF.md`.

**Accept:** the four-configuration build matrix unchanged and green (no
source file touched); no test result altered; every one of the 53
pre-existing units carries a verdict in `docs/wu-audit-2026-08.md`'s
register.

**Not done, and explicitly not this unit's job:** does not resolve F9 (the
M1/M2/M3 arbitration mechanism choice stays open, and WU-35's own arbitration
interface stays swappable); does not reopen any green tag — WU-12b, WU-19b,
WU-22c and WU-26 are named as green-tag review candidates, not corrected;
does not implement any part of WU-27/WU-33/WU-35/WU-37 beyond the scope
notes above.

*Status:* `green` — no build-affecting change; see `docs/wu-audit-2026-08.md`
and `HANDOFF.md` for the full sweep.
