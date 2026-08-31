// scatter-dve — WU-33c4: a real, observable front/back demo, closing the
// dependency chain WU-33a/33b/33c1/33c2a/33c2b/33c3 built (DECISIONS.md
// ADR-092 through ADR-097) into something Steve can actually watch on the
// Monitor 3G output — the same "wire the already-built mechanism into
// something visible" split WU-28d/WU-35a2 already used once each
// (DECISIONS.md ADR-094 split this unit out of WU-33c for exactly that
// reason). See DECISIONS.md ADR-098 for the full design account; the
// load-bearing points are repeated here, close to the code they explain.
//
// **Scope, decided this session, not assumed from WORK-UNITS.md's own
// prior sketch (Session 78, written before WU-33c2a/WU-33c2b/WU-33c3 had
// even split or decided their own real shapes):** this unit wires
// io/file_back_source.hpp's own FileBackSource (WU-33c2a) into a real
// front CaptureConsumer, alone. It does **not** wire
// io/decklink_back_source.hpp's own DeckLinkBackSource (WU-33c2b) — that
// needs a second, genuinely distinct capture-and-format-detection-capable
// DeckLink input, which Sessions 79 through 82 have all been unable to
// confirm Steve's own hardware actually enumerates (no session run so far
// has had real hardware access to check enumerateDeckLinkDevices()'s own
// output). That two-device half is real, separate, hardware-gated work,
// split out to its own sibling unit, WU-33c5 (WORK-UNITS.md), rather than
// guessed at here or silently folded into this one — the same "named,
// split out, not silently expanded" move this project's whole WU-33c
// lineage has used at every prior fork (WU-33c1 off WU-33c, ADR-094;
// WU-33c2a/WU-33c2b off WU-33c2, ADR-095).
//
// **Back-side content: a real WU-03 zone plate (tools/testpat.hpp),
// generated and written to a temp .v210 file at startup, then read back
// through FileBackSource::create() — not a solid colour.** Mirrors
// tests/test_decklink_output.cpp's own writeWarpedTestFrame(), which
// already uses exactly this generate-then-write-then-read-back shape for
// its own front-side source. A zone plate is unmistakably not whatever
// the live front feed happens to show, which is the actual point of this
// unit's own by-eye Accept criterion — a solid colour (e.g.
// video::packBlackFrame, WU-33c3's own test) risks coincidentally
// matching a real signal's own average field, and proves nothing about
// content by eye the way a zone plate's own rings do.
//
// **No keyboard interactivity, no CoverageWindow, no manualTransp/T-t
// controls, and no kBufferMode/frontTag/backTag configuration.**
// tests/test_decklink_live_sphere.cpp already owns the interactive letter-
// key control surface (WU-21h/21i/22c/35a2/35a3) — duplicating it here
// would not serve this unit's own job, which is the backSrc wiring proof,
// not another control surface. And confirmed directly against the real
// current code this session, not assumed: core/pipeline.cpp's own
// runFrame()/runFrameField() forward params.backSrc to whichever of
// core/binner.hpp's six generateFragments*() entry points they call
// *unconditionally* — the plain, single-scalar-tag, kBufferMode==Off path
// included (see core/pipeline.cpp's own comment above its PASS-1 call
// sites, and generateFragments()'s own call at the bottom of runFrame()'s
// threads<=1 branch: `generateFragments(lattice, src, params.maxK,
// params.supersample, params.tag, bins, shadingGrid, params.backSrc)`).
// backSrc substitution and k-buffer facing-tag/blend are two independent
// mechanisms (core/binner.hpp's own file comment on backSrc says so
// explicitly) — this unit needs only the former, so default
// PipelineParams (kBufferMode already Off, tag already 0) is genuinely
// enough; no special resolve-mode setup is a real finding this session
// made, not an oversight.
//
// **Rotation: automatic, one full 2*pi yaw sweep over kDemoSeconds, not
// operator-driven.** The sphere lattice itself is
// tests/test_decklink_live_sphere.cpp's own makeSphereLattice()/
// rotateLattice(), duplicated locally per SESSION-PROTOCOL.md rule 2 (one
// unit, one test, no shared fixture across test translation units — the
// same convention every DeckLink test file in this project already
// follows) — not a new shape. Confirmed directly this session, not
// assumed: this sphere is a full pole-to-pole (kAngleSpanV == M_PI),
// full-360-wrap (kAngleSpanH == 2*M_PI) closed shape, so at any single
// orientation roughly half its own visible control vertices already
// satisfy core/binner.hpp's own surfaceNormal(rawJ).z >= 0.0 back-facing
// selection and roughly half do not — unlike WU-33c3's own test fixture,
// a flat identity lattice, which never does. An automatic sweep, rather
// than requiring an operator to drive rotation by hand, is this unit's
// own explicit choice: it guarantees every region of the sphere's own
// surface crosses the front/back boundary at least once during any single
// run, which is what actually exercises this unit's own Accept criterion
// (front and back showing genuinely different content) without depending
// on Steve holding a cursor key down for the right duration.
//
// Real-hardware setup: the same UltraStudio Recorder 3G SDI input / Monitor
// 3G SDI output this whole live-demo lineage already uses (WU-21e/ADR-051
// onward) — a live source patched into the Recorder 3G's own SDI input,
// Monitor 3G's own SDI output to a real display. No second capture device
// needed for this unit specifically (see the scope note above).
//
// Runs only against real hardware — no Blackmagic SDK, no AppleClang/Xcode
// toolchain, no Cocoa, in the Linux cloud sandbox this was drafted in.
// UNVERIFIED: written and reasoned through this session, not built or run
// by it — see this repository's own HANDOFF.md for the exact reason
// (DeckLink-linked, device-bridge hand-off only). Build and run this at
// your own real terminal.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "io/decklink_live_output.hpp"
#include "io/file_back_source.hpp"
#include "video/deinterlace.hpp"
#include "video/v210.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <thread>

using namespace scatter::io;

namespace {

// Same display mode every other real-hardware DeckLink test in this
// project already uses (ADR-007/033).
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;
constexpr int kWidth = 720;
constexpr int kHeight = 576;

// Sphere geometry: identical values to
// tests/test_decklink_live_sphere.cpp's own kInitialRadius/kInitialCenterX/
// kInitialCenterY (WU-21g/21h) — this unit is not exploring a new shape,
// only proving backSrc's own wiring against an already-confirmed-working
// self-folding one.
constexpr double kRadius  = 260.0;
constexpr double kCenterX = double(kWidth) / 2.0;
constexpr double kCenterY = double(kHeight) / 2.0;
constexpr double kAngleSpanH = 2.0 * M_PI;  // full 360 degrees, seamless
constexpr double kAngleSpanV = M_PI;        // pole to pole, exact fold boundary

// One full yaw sweep over this many seconds, then the demo ends cleanly on
// its own — no operator input needed. Long enough to watch comfortably at
// a real monitor; short enough not to overstay an unattended `ctest` run
// (this project's own "unattended is a real, honestly reportable state"
// convention, WU-21i) if this binary is ever invoked that way.
constexpr double kDemoSeconds = 30.0;

ComPtr<IDeckLinkInput> firstFormatDetectionCapableInput(const std::vector<DeviceInfo>& devices) {
    for (const auto& d : devices) {
        if (!d.supportsCapture) continue;

        ComPtr<IDeckLinkInput> candidate(IID_IDeckLinkInput, d.device);
        if (!candidate) continue;

        ComPtr<IDeckLinkProfileAttributes> attributes(IID_IDeckLinkProfileAttributes, d.device);
        bool supportsFormatDetection = false;
        if (attributes &&
            attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportsFormatDetection) == S_OK &&
            supportsFormatDetection)
            return candidate;
    }
    return {};
}

ComPtr<IDeckLinkOutput> firstPlaybackCapableOutput(const std::vector<DeviceInfo>& devices) {
    for (const auto& d : devices) {
        if (!d.supportsPlayback) continue;
        ComPtr<IDeckLinkOutput> candidate(IID_IDeckLinkOutput, d.device);
        if (candidate) return candidate;
    }
    return {};
}

// Duplicated from tests/test_decklink_live_sphere.cpp's own
// makeSphereLattice() (SESSION-PROTOCOL.md rule 2) — same geometry, fixed
// constants here instead of runtime-mutable ones, since this unit has no
// operator controls of its own to change them.
scatter::Lattice makeSphereLattice() {
    scatter::shapes::SphereParams sp;
    sp.radius     = kRadius;
    sp.angleSpanH = kAngleSpanH;
    sp.angleSpanV = kAngleSpanV;
    sp.centerX    = kCenterX;
    sp.centerY    = kCenterY;
    return scatter::shapes::buildSphereLattice(sp);
}

// Duplicated from tests/test_decklink_live_sphere.cpp's own rotateLattice()
// (SESSION-PROTOCOL.md rule 2) — rigid yaw-only rotation about the
// sphere's own true 3D centre (pivotZ == kRadius, not 0 — CORRECTIONS.md
// C-017: a rotation about a sphere's own true centre cannot produce
// negative depth for any angle, since every control vertex is already
// exactly kRadius from that centre by construction). base is not
// modified; a fresh Lattice is returned.
scatter::Lattice rotateLatticeYaw(const scatter::Lattice& base, double yaw) {
    scatter::Lattice out;
    const double cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
    for (int row = 0; row < scatter::kLatticeSize; ++row) {
        for (int col = 0; col < scatter::kLatticeSize; ++col) {
            scatter::Vec3 p = base.at(row, col);
            p.x -= kCenterX;
            p.z -= kRadius;
            const double x = p.x * cosYaw + p.z * sinYaw;
            const double z = -p.x * sinYaw + p.z * cosYaw;
            out.at(row, col) = scatter::Vec3{x + kCenterX, p.y, z + kRadius};
        }
    }
    return out;
}

}  // namespace

static void test_live_backsrc_demo() {
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkInput> input = firstFormatDetectionCapableInput(devices);
    CHECK(bool(input));
    ComPtr<IDeckLinkOutput> output = firstPlaybackCapableOutput(devices);
    CHECK(bool(output));
    if (!input || !output) return;

    CaptureFrameRing ring;
    auto capture = CaptureSource::create(input, kDisplayMode, ring);
    CHECK(bool(capture));
    if (!capture) return;

    // Real, visibly-distinct back-side still — see this file's own header
    // comment for why a zone plate, not a solid colour. Written once,
    // here, then read back through FileBackSource::create() exactly the
    // way a caller with a pre-existing static asset file would use it —
    // this unit generates its own asset only so Steve does not have to
    // prepare one by hand before running this demo.
    const std::string backPath = "/tmp/scatter_dve_wu33c4_backsrc_zoneplate.v210";
    {
        const auto zonePlate = scatter::testpat::makeZonePlate(kWidth, kHeight);
        if (!scatter::testpat::writeV210(zonePlate, backPath)) {
            std::fprintf(stderr,
                         "test_decklink_live_backsrc_demo: could not write back-source zone plate to "
                         "%s -- aborting.\n",
                         backPath.c_str());
            CHECK(false);
            return;
        }
    }

    // Removed immediately after create() -- FileBackSource reads the whole
    // file into memory at construction and never touches the path again,
    // the same "no longer needed once read" cleanup
    // test_capture_consumer_queries_wired_back_source() already uses
    // (tests/test_decklink_capture_consumer.cpp, WU-33c3).
    auto fileBackSource = scatter::io::FileBackSource::create(backPath, kWidth, kHeight);
    CHECK(bool(fileBackSource));
    std::remove(backPath.c_str());
    if (!fileBackSource) return;

    // Wraps FileBackSource's own plain-OwnedSourceRaster-returning
    // accessor to match CaptureConsumer::BackSourceCallback's own
    // std::optional<OwnedSourceRaster>() shape — exactly the wrapping
    // decklink_capture_consumer.hpp's own doc comment on BackSourceCallback
    // documents, and tests/test_decklink_capture_consumer.cpp's own
    // test_capture_consumer_queries_wired_back_source() already uses
    // (WU-33c3, DECISIONS.md ADR-097).
    CaptureConsumer::BackSourceCallback backSource = [&fileBackSource] {
        return std::optional<scatter::OwnedSourceRaster>(fileBackSource->currentSourceRaster());
    };

    scatter::PipelineParams params;
    params.destWidth = kWidth;
    params.destHeight = kHeight;
    // Deliberately nothing else set — see this file's own header comment:
    // backSrc substitution needs no kBufferMode/frontTag/backTag
    // configuration, confirmed directly against core/pipeline.cpp this
    // session.

    // WU-23b2b (ADR-080/081): DeinterlaceCoefficients has no default —
    // Complex, Steve's own explicit choice, same as every other caller in
    // this project. backSource is the new trailing argument WU-33c3 added
    // (DECISIONS.md ADR-097), after coverageCallback, which this unit has
    // no use for and passes as nullptr.
    CaptureConsumer consumer(ring, makeSphereLattice(), params,
                              scatter::video::DeinterlaceCoefficients::Complex,
                              /*coverageCallback=*/nullptr, backSource);
    consumer.start();

    auto playback = LiveFramePlayback::create(output, kDisplayMode, kWidth, kHeight, consumer);
    CHECK(bool(playback));
    if (playback) {
        std::fprintf(stderr,
                     "test_decklink_live_backsrc_demo: running a %.0f-second automatic yaw sweep. "
                     "Watch the Monitor 3G output -- the near (front-facing) hemisphere should show "
                     "the live front feed, the far (back-facing) hemisphere should show the static "
                     "zone plate, and the two should visibly swap as the sphere turns.\n",
                     kDemoSeconds);

        const auto base = makeSphereLattice();
        const auto start = std::chrono::steady_clock::now();
        double lastReportedSecond = -1.0;
        for (;;) {
            const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (elapsed >= kDemoSeconds) break;

            const double yaw = (elapsed / kDemoSeconds) * (2.0 * M_PI);
            consumer.setLattice(rotateLatticeYaw(base, yaw));

            if (elapsed - lastReportedSecond >= 5.0) {
                lastReportedSecond = elapsed;
                std::fprintf(stderr, "test_decklink_live_backsrc_demo: %.0fs / %.0fs (yaw=%.2f rad)\n",
                             elapsed, kDemoSeconds, yaw);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        std::fprintf(stderr,
                     "test_decklink_live_backsrc_demo: sweep complete. completed=%d displayedLate=%d "
                     "dropped=%d flushed=%d framesRepeated=%d\n",
                     playback->stats().completed.load(), playback->stats().displayedLate.load(),
                     playback->stats().dropped.load(), playback->stats().flushed.load(),
                     playback->framesRepeated());
        playback->stop();
    }

    consumer.stop();
    capture->stop();

    const auto& captureStats = capture->stats();
    const auto& consumerStats = consumer.stats();

    // Same accounting invariant every other CaptureConsumer-driving test in
    // this repository already checks (WU-23b2b, ADR-080/081).
    CHECK(std::size_t(consumerStats.framesProcessed.load()) + std::size_t(consumerStats.framesFailed.load()) +
              std::size_t(consumerStats.framesStreamStart.load()) ==
          std::size_t(consumerStats.framesPopped.load()));
    CHECK(std::size_t(consumerStats.framesPopped.load()) <= std::size_t(captureStats.framesPushed.load()));
    // backSourceQueried (WU-33c3): every frame that reached
    // ProcessResult::Processed or ProcessResult::StreamStart already
    // queried m_backSource, unconditionally, since this consumer was
    // constructed with one — same invariant
    // test_capture_consumer_queries_wired_back_source() already checks.
    CHECK(std::size_t(consumerStats.backSourceQueried.load()) >=
          std::size_t(consumerStats.framesProcessed.load()) +
              std::size_t(consumerStats.framesStreamStart.load()));

    std::fprintf(stderr,
                 "test_decklink_live_backsrc_demo: framesArrived=%d framesPushed=%d | framesPopped=%d "
                 "framesProcessed=%d framesFailed=%d framesStreamStart=%d backSourceQueried=%d\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 consumerStats.framesPopped.load(), consumerStats.framesProcessed.load(),
                 consumerStats.framesFailed.load(), consumerStats.framesStreamStart.load(),
                 consumerStats.backSourceQueried.load());

    if (consumerStats.framesProcessed.load() == 0) {
        std::fprintf(stderr,
                     "test_decklink_live_backsrc_demo: NOTE -- zero frames processed. If no live "
                     "source is patched into the Recorder 3G's SDI input, this is expected, not a "
                     "defect -- the accounting invariants above are still the evidence this test's "
                     "own automated CHECKs gate on, but the by-eye front/back claim itself cannot be "
                     "confirmed without a real live front signal driving the near hemisphere.\n");
    }
}

int main() {
    test_live_backsrc_demo();
    return scatter::test::summary("test_decklink_live_backsrc_demo");
}
