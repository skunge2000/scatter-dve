// scatter-dve — WU-33c4: a real, observable front/back demo, closing the
// dependency chain WU-33a/33b/33c1/33c2a/33c2b/33c3 built (DECISIONS.md
// ADR-092 through ADR-097) into something Steve can actually watch on the
// Monitor 3G output — the same "wire the already-built mechanism into
// something visible" split WU-28d/WU-35a2 already used once each
// (DECISIONS.md ADR-094 split this unit out of WU-33c for exactly that
// reason). See DECISIONS.md ADR-098 for the original design account and
// ADR-099 for this same-session follow-up (interactive keyboard controls,
// added at Steve's own request after his first real-terminal run); the
// load-bearing points from both are repeated here, close to the code they
// explain.
//
// **Scope, decided this session, not assumed from WORK-UNITS.md's own
// prior sketch (Session 78, written before WU-33c2a/WU-33c2b/WU-33c3 had
// even split or decided their own real shapes):** this unit wires
// io/file_back_source.hpp's own FileBackSource (WU-33c2a) into a real
// front CaptureConsumer, alone. It does **not** wire
// io/decklink_back_source.hpp's own DeckLinkBackSource (WU-33c2b) — that
// needs a second, genuinely distinct capture-and-format-detection-capable
// DeckLink input. Steve confirmed this session that more than one such
// device is enumerated on his own hardware, but which one is actually fed
// is a separate, parked question (see HANDOFF.md) — this unit still does
// not depend on it, and DeckLinkBackSource's own two-device demo stays
// split out to its own sibling unit, WU-33c5 (WORK-UNITS.md), rather than
// guessed at here — the same "named, split out, not silently expanded"
// move this project's whole WU-33c lineage has used at every prior fork.
//
// **Back-side content: a real WU-03 zone plate (tools/testpat.hpp),
// generated and written to a temp .v210 file at startup, then read back
// through FileBackSource::create() — not a solid colour.** Mirrors
// tests/test_decklink_output.cpp's own writeWarpedTestFrame(). A zone
// plate is unmistakably not whatever the live front feed happens to show,
// which is the actual point of this unit's own by-eye Accept criterion.
//
// **ADR-099 (this session's own follow-up): full interactive keyboard
// control, ported from tests/test_decklink_live_sphere.cpp's own flag-off
// (non-CoverageWindow) loop, at Steve's own explicit request after running
// this demo for the first time.** ADR-098 originally left interactivity
// out entirely, reasoning that this unit's own job was the backSrc wiring
// proof, not another control surface, and that test_decklink_live_sphere.cpp
// already owned that surface. In practice, being able to drive the sphere
// by hand turned out to matter for actually watching the front/back
// distinction (an unattended, fixed-speed automatic sweep is a worse way
// to inspect a live effect than an operator steering it) — so this file
// now carries its own copy of that control surface: Key/readKey()/
// applyKey(), duplicated locally per SESSION-PROTOCOL.md rule 2 (one unit,
// one test, no shared fixture across test translation units), not
// extracted into a shared header. This file does **not** port
// test_decklink_live_sphere.cpp's own CoverageWindow/--show-coverage
// machinery (WU-22c) — that is a materially separate feature (a Cocoa
// window, GCD dispatch sources) this unit has no need for, and porting it
// unasked would be exactly the kind of silent scope expansion this
// project's own conventions warn against. The original automatic sweep is
// kept, not removed — it is now the non-interactive-terminal fallback (the
// same "unattended is a real, honestly reportable state" convention
// WU-21i established), rather than the primary path.
//
// **ADR-099 also adds `params.kBufferMode = Blend` and a live
// `manualTransp`, seeded at 0** — without it, T/t (this file's own newly
// ported controls) would change a variable with no visible effect
// whatsoever: core/resolve.hpp's own compositeKBuffer() only runs its
// Blend fold when kBufferMode != Off, independent of frontTag/backTag
// (confirmed this session, not assumed — see ADR-099). frontTag/backTag
// themselves are left at their PipelineParams default (both 0, i.e. no
// facing-tag differentiation) — this unit's own backSrc-substitution proof
// does not need it, only the Blend resolve itself, which is gated on
// kBufferMode alone.
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

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <thread>

#include <termios.h>
#include <unistd.h>

using namespace scatter::io;

namespace {

// Same display mode every other real-hardware DeckLink test in this
// project already uses (ADR-007/033).
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;
constexpr int kWidth = 720;
constexpr int kHeight = 576;

// Sphere geometry: identical initial values to
// tests/test_decklink_live_sphere.cpp's own kInitialRadius/kInitialCenterX/
// kInitialCenterY (WU-21g/21h) — this unit is not exploring a new shape,
// only proving backSrc's own wiring against an already-confirmed-working
// self-folding one. Full pole-to-pole/360-wrap, unchanged from that file.
constexpr double kInitialRadius  = 260.0;
constexpr double kInitialCenterX = double(kWidth) / 2.0;
constexpr double kInitialCenterY = double(kHeight) / 2.0;
constexpr double kMinRadius = 20.0;
constexpr double kAngleSpanH = 2.0 * M_PI;  // full 360 degrees, seamless
constexpr double kAngleSpanV = M_PI;        // pole to pole, exact fold boundary

// ADR-099: same step sizes as tests/test_decklink_live_sphere.cpp's own
// (WU-21g/35a2) — this file's own control surface is a direct port, not a
// reinterpretation, so the feel should match exactly.
constexpr double kRotationStep = 0.05;   // radians per cursor keypress
constexpr double kPositionStep = 10.0;   // output pixels per X/x/Y/y keypress
constexpr double kRadiusStep   = 10.0;   // output pixels per Z/z keypress
constexpr int kTranspStep = int(scatter::kWeightUnity) / 16;

// Non-interactive-terminal fallback only (ADR-099 demoted this from the
// primary path to the fallback) — one full yaw sweep over this many
// seconds when stdin is not a real terminal (e.g. an unattended `ctest`
// run), so this binary still does something observable rather than
// hanging or silently sitting idle.
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
// makeSphereLattice() (SESSION-PROTOCOL.md rule 2) — radius/centerX/centerY
// are runtime state now (ADR-099), not fixed constants, since this file has
// its own operator controls to change them.
scatter::Lattice makeSphereLattice(double radius, double centerX, double centerY) {
    scatter::shapes::SphereParams sp;
    sp.radius     = radius;
    sp.angleSpanH = kAngleSpanH;
    sp.angleSpanV = kAngleSpanV;
    sp.centerX    = centerX;
    sp.centerY    = centerY;
    return scatter::shapes::buildSphereLattice(sp);
}

// Duplicated from tests/test_decklink_live_sphere.cpp's own rotateLattice()
// (SESSION-PROTOCOL.md rule 2) — rigid yaw-then-pitch rotation about
// (pivotX, pivotY, pivotZ). base is not modified; a fresh Lattice is
// returned.
scatter::Lattice rotateLattice(const scatter::Lattice& base, double yaw, double pitch, double pivotX, double pivotY,
                                double pivotZ) {
    scatter::Lattice out;
    const double cosYaw = std::cos(yaw), sinYaw = std::sin(yaw);
    const double cosPitch = std::cos(pitch), sinPitch = std::sin(pitch);

    for (int row = 0; row < scatter::kLatticeSize; ++row) {
        for (int col = 0; col < scatter::kLatticeSize; ++col) {
            const scatter::Vec3& p = base.at(row, col);
            double x = p.x - pivotX;
            double y = p.y - pivotY;
            double z = p.z - pivotZ;

            const double x1 = x * cosYaw + z * sinYaw;
            const double z1 = -x * sinYaw + z * cosYaw;

            const double y2 = y * cosPitch - z1 * sinPitch;
            const double z2 = y * sinPitch + z1 * cosPitch;

            scatter::Vec3& q = out.at(row, col);
            q.x = x1 + pivotX;
            q.y = y2 + pivotY;
            q.z = z2 + pivotZ;
        }
    }
    return out;
}

// ADR-099: ported from tests/test_decklink_live_sphere.cpp's own Key enum,
// unchanged.
enum class Key { Up, Down, Left, Right, XInc, XDec, YInc, YDec, ZInc, ZDec, TInc, TDec, Quit, Unknown };

// ADR-099: ported from tests/test_decklink_live_sphere.cpp's own readKey()
// (its flag-off/non-CoverageWindow variant), unchanged — blocks for exactly
// one logical keypress (which may be several raw bytes, for an
// escape-sequence arrow key) and returns what it means. Plain arrows only
// (no shift+arrow lookahead — that did not work on real hardware,
// CORRECTIONS.md C-018).
Key readKey() {
    const int c = std::getchar();
    if (c == EOF) return Key::Quit;  // stdin closed under us -- treat as quit, not a spin
    if (c == 'q' || c == 'Q') return Key::Quit;
    if (c == 'X') return Key::XInc;
    if (c == 'x') return Key::XDec;
    if (c == 'Y') return Key::YInc;
    if (c == 'y') return Key::YDec;
    if (c == 'Z') return Key::ZInc;
    if (c == 'z') return Key::ZDec;
    if (c == 'T') return Key::TInc;
    if (c == 't') return Key::TDec;
    if (c != 27) return Key::Unknown;  // not ESC -- not a sequence this UI understands

    if (std::getchar() != '[') return Key::Unknown;
    switch (std::getchar()) {
        case 'A': return Key::Up;
        case 'B': return Key::Down;
        case 'C': return Key::Right;
        case 'D': return Key::Left;
        default:  return Key::Unknown;
    }
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

    double yaw = 0.0, pitch = 0.0;
    double centerX = kInitialCenterX;
    double centerY = kInitialCenterY;
    double radius  = kInitialRadius;

    // ADR-099: manualTransp is live state now, seeded at 0 (fully opaque,
    // matching test_decklink_live_sphere.cpp's own T/t rest position) and
    // pushed to the running consumer via setManualTransp() on every T/t
    // keypress, below.
    scatter::Weight manualTransp = 0;

    scatter::PipelineParams params;
    params.destWidth = kWidth;
    params.destHeight = kHeight;
    // ADR-099: Blend, not Off -- without this, T/t (this file's own newly
    // ported controls) would change manualTransp with no visible effect at
    // all (compositeKBuffer()'s own Blend fold never runs otherwise,
    // core/resolve.hpp). frontTag/backTag are left at their default (both
    // 0) -- this unit's own backSrc-substitution proof does not need
    // facing-tag differentiation, only the Blend resolve itself, which is
    // gated on kBufferMode alone (confirmed against core/pipeline.cpp this
    // session, ADR-099).
    params.kBufferMode = scatter::KBufferResolveMode::Blend;
    params.manualTransp = manualTransp;

    // WU-23b2b (ADR-080/081): DeinterlaceCoefficients has no default —
    // Complex, Steve's own explicit choice, same as every other caller in
    // this project. backSource is the new trailing argument WU-33c3 added
    // (DECISIONS.md ADR-097), after coverageCallback, which this unit has
    // no use for and passes as nullptr.
    CaptureConsumer consumer(ring, makeSphereLattice(radius, centerX, centerY), params,
                              scatter::video::DeinterlaceCoefficients::Complex,
                              /*coverageCallback=*/nullptr, backSource);
    consumer.start();

    auto playback = LiveFramePlayback::create(output, kDisplayMode, kWidth, kHeight, consumer);
    CHECK(bool(playback));
    if (playback) {
        // ADR-099: whether stdin is a real terminal is queried up front,
        // the same "tcgetattr() is a pure query, moving it earlier changes
        // nothing observable" reasoning
        // tests/test_decklink_live_sphere.cpp's own WU-22c session already
        // used for the identical check.
        struct termios oldAttrs {};
        const bool interactive = (tcgetattr(STDIN_FILENO, &oldAttrs) == 0);

        if (interactive) {
            struct termios rawAttrs = oldAttrs;
            rawAttrs.c_lflag &= ~(tcflag_t(ICANON) | tcflag_t(ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &rawAttrs);

            std::fprintf(stderr,
                         "test_decklink_live_backsrc_demo: cursor keys rotate (left/right = yaw, "
                         "up/down = pitch); X/x = position right/left, Y/y = position down/up, "
                         "Z/z = bigger/smaller, T/t = more/less transparent. Q quits. Watch the "
                         "Monitor 3G output -- the near (front-facing) hemisphere shows the live "
                         "front feed, the far (back-facing) hemisphere shows the static zone plate.\n");

            // ADR-099: direct port of tests/test_decklink_live_sphere.cpp's
            // own flag-off interactive loop (WU-21i/35a2/35a3) -- this file
            // has no CoverageWindow, so only that simpler branch is ported,
            // not the CoverageWindow-driven GCD dispatch-source variant
            // WU-22c added alongside it there.
            for (;;) {
                const Key key = readKey();
                bool changed = true;
                switch (key) {
                    case Key::Left:    yaw -= kRotationStep; break;
                    case Key::Right:   yaw += kRotationStep; break;
                    case Key::Up:      pitch -= kRotationStep; break;
                    case Key::Down:    pitch += kRotationStep; break;
                    case Key::XInc:    centerX += kPositionStep; break;
                    case Key::XDec:    centerX -= kPositionStep; break;
                    case Key::YInc:    centerY += kPositionStep; break;
                    case Key::YDec:    centerY -= kPositionStep; break;
                    case Key::ZInc:    radius += kRadiusStep; break;
                    case Key::ZDec:    radius = std::max(kMinRadius, radius - kRadiusStep); break;
                    case Key::TInc:
                        manualTransp = scatter::Weight(
                            std::min(int(scatter::kWeightUnity), int(manualTransp) + kTranspStep));
                        consumer.setManualTransp(manualTransp);
                        break;
                    case Key::TDec:
                        manualTransp = scatter::Weight(std::max(0, int(manualTransp) - kTranspStep));
                        consumer.setManualTransp(manualTransp);
                        break;
                    case Key::Quit:    changed = false; break;
                    case Key::Unknown: changed = false; break;
                }
                if (key == Key::Quit) break;
                if (changed)
                    consumer.setLattice(rotateLattice(makeSphereLattice(radius, centerX, centerY), yaw, pitch,
                                                        centerX, centerY, radius));
            }

            tcsetattr(STDIN_FILENO, TCSANOW, &oldAttrs);
        } else {
            // ADR-099: demoted from the primary path to the non-interactive
            // fallback -- still a real, observable run when stdin is not a
            // real terminal (e.g. an unattended `ctest`), same "unattended
            // is a real, honestly reportable state" convention WU-21i
            // established.
            std::fprintf(stderr,
                         "test_decklink_live_backsrc_demo: stdin is not a real terminal -- interactive "
                         "control is unavailable this run; falling back to a %.0f-second automatic yaw "
                         "sweep instead.\n",
                         kDemoSeconds);

            const auto base = makeSphereLattice(radius, centerX, centerY);
            const auto start = std::chrono::steady_clock::now();
            double lastReportedSecond = -1.0;
            for (;;) {
                const double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
                if (elapsed >= kDemoSeconds) break;

                const double sweepYaw = (elapsed / kDemoSeconds) * (2.0 * M_PI);
                consumer.setLattice(rotateLattice(base, sweepYaw, 0.0, centerX, centerY, radius));

                if (elapsed - lastReportedSecond >= 5.0) {
                    lastReportedSecond = elapsed;
                    std::fprintf(stderr, "test_decklink_live_backsrc_demo: %.0fs / %.0fs (yaw=%.2f rad)\n",
                                 elapsed, kDemoSeconds, sweepYaw);
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        }

        std::fprintf(stderr,
                     "test_decklink_live_backsrc_demo: run complete. completed=%d displayedLate=%d "
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
