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
generation-time bin arenas `wip`
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
is untouched — but per this project's own convention the assistant does
not run `close.sh`; the full suite and `./tools/close.sh 16b` still need
a real-terminal run before this can be tagged `wu-16b-green`.

### WU-17 — NEON v210 unpack and pack `green` (`wu-17-green`, pending Steve's manual tag)
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
Steve tagged `wu-17-green` by hand, the same way he did for
`wu-15a-green`/`wu-16a-green`/`wu-16b-green`.
### WU-18 — NEON chroma resampling `green` (`wu-18-green`, pending Steve's manual tag)
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
`wu-15a-green`/`wu-16a-green`/`wu-16b-green`/`wu-17-green` — not yet
confirmed run as of this handoff, see "Next work unit" below. Once tagged,
**Phase 4 (Threading and NEON) is done in full** — thread pool/QoS/
per-worker bin arenas (WU-16a/16b) and both NEON units (WU-17 v210, WU-18
chroma) all green; WU-19 ("Real time at 576i25") is next and is the
phase's only remaining unit, the first whose own job is throughput rather
than correctness. (This session's own "once tagged, Phase 4 is done in
full" was premature, not wrong — WU-19 had not yet been scoped, and real
scoping the next session found it split into WU-19a/19b, below; Phase 4
did not actually finish until both closed out. Same doc-sync slip WU-03's
own status had at WU-04, corrected in place rather than erased.)
### WU-19a — Persistent, caller-owned ThreadPool `wip`
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
could not already fully verify. Still needs Steve's own real-terminal
`cmake --build` + `ctest` + `./tools/close.sh 19a` run before this line can
go `green`, the same procedural reason every other unit's line has had.

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

### WU-21d — Cold-start black fill for `LiveFramePlayback`'s own pool `todo`
Recorded ahead of this unit's own real scoping session so it isn't lost, the
same "Steve's own stated preference, noted here for whoever scopes this
unit" convention WU-23's own entry below already uses — not a decision made
now, and not itself a real `Files:`/`Accept:` scope. WU-21c's own real-
hardware verification (`DECISIONS.md` ADR-050's own same-session addendum)
found that `LiveFramePlayback`'s pool buffers, scheduled during
`startWith()`'s own preroll loop before `CaptureConsumer` has produced its
own first output, are left holding whatever `CreateVideoFrame()` first
allocated them with — effectively zero-filled `v210` — which decodes as a
strongly saturated green once converted for display, seen by eye on the
Monitor 3G's own HDMI-mirrored output as a few seconds of solid green before
real content appears. Candidate fix named there: fill each pool buffer with
black immediately after `CreateVideoFrame()`, before the preroll loop
schedules any of them, so a cold start shows black rather than green. Also
candidate territory for whoever scopes this unit, not yet decided: the
literal one-hour endurance run and by-eye live-content confirmation
`WORK-UNITS.md`'s own WU-21c `Accept:` text deferred here; and real
measurement of whether WU-21c's own measured `framesRepeated` rate (~15% on
one 5-second run) stays acceptable over a longer run, with a possible
timestamp-alignment refinement if not — both already named in ADR-050 as
"not decided here." Whether all of this stays one unit or splits (the same
a/b/c discipline this project already uses when a unit's scope grows past
the 3-file/400-line cap) is that session's own call, not decided here.

### WU-22 — Diagnostic coverage view `todo`

---

## Phase 6 — Scale up

### WU-23 — Interlace and field mode `todo`
Steve's own stated preference, noted here for whoever scopes this unit, not
a decision made now: **if deinterlacing is pursued, the route is Weston
3-field, for period accuracy** — not a simpler bob/weave or line-doubling
approach. Recorded ahead of this unit's own real scoping session so it isn't
lost or re-litigated from scratch when WU-23 actually starts; that session's
own first job is still real `Files:`/`Accept:` scoping against this
preference and against whatever this project's own interlace/field-mode
needs turn out to be by then, same discipline as every other unit.
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
