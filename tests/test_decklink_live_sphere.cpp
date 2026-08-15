// WU-21h -- rudimentary interactive UI for the live sphere demo. Supersedes
// WU-21g's own automatic-rotation content (built and run for real this
// session -- the full wrap read as "hugely better"). Replaces the
// automatic continuous-yaw/oscillating-pitch schedule with manual keyboard
// control: cursor keys rotate (left/right = yaw, up/down = pitch), shifted
// cursor keys reposition the sphere (shift+left/right = x, shift+up/down =
// y), I/O shrink/grow it (I = "in" to the screen, smaller; O = "out" of the
// screen, larger), Q quits. The full pole-to-pole/360-degree wrap geometry
// WU-21g established (angleSpanH == 2*pi, angleSpanV == pi) is unchanged --
// Steve's own feedback was about the wrap, not the geometry, so this entry
// does not touch it.
//
// Steve separately asked, not for this unit to solve but to record for
// later: how this project handles transparency and front/back switching
// when a wrap like this one folds the sphere's own back hemisphere onto
// the same screen area as its front. See DECISIONS.md ADR-054 and
// WORK-UNITS.md's own WU-28 entry, which this session added a note to
// rather than inventing a new backlog entry for the same open problem
// ADR-053 already named.
//
// rotateLattice() is unchanged in its own rotation mathematics from
// WU-21f/g (a rigid yaw-then-pitch rotation about the sphere's own true
// centre, CORRECTIONS.md C-017: cannot produce negative depth for any
// angle) but now takes that centre and radius as parameters instead of
// file-level constants, since I/O and shifted cursor keys change them at
// runtime; makeSphereLattice() likewise takes radius/centre as parameters
// and is called fresh whenever either changes (every control-vertex
// rebuild -- 16,641 vertices, a handful of trig calls each -- is cheap
// enough per keypress that this project's own "once per frame, not in the
// fixed-point path" cost reasoning, shapes.hpp's own header comment,
// covers it without a new argument).
//
// Input handling is now event-driven, not timer-driven: WU-21g's own
// separate keypress-wait thread and 80ms polling loop are both gone,
// replaced by a single blocking read loop on the main thread -- there is
// no more idle animation to keep advancing between keypresses, so nothing
// needs to run on a timer. Raw terminal mode (ICANON/ECHO cleared, restored
// on exit) is unchanged from WU-21g's own approach. Cursor keys and
// shift+cursor arrive as multi-byte xterm-style escape sequences (ESC [ A/
// B/C/D for plain arrows; ESC [ 1 ; 2 A/B/C/D for shift+arrow, the common
// xterm "modifyOtherKeys" convention both macOS Terminal.app and iTerm2
// send by default) -- parsed by readKey() below. This is genuinely
// terminal-dependent and this session cannot verify it against a real
// terminal emulator; if shift+arrow is not recognised on Steve's own
// terminal, the fallback is harmless (it reads as an unrecognised sequence
// and is ignored, not misinterpreted as something destructive), and is
// worth reporting back so the parsing can be adjusted. A bare ESC keypress
// (not part of an arrow sequence) is a known rough edge: readKey() blocks
// waiting for the bytes that would normally follow ESC in an arrow
// sequence, so a standalone ESC appears to do nothing until another key is
// pressed -- acceptable for a "rudimentary UI", per Steve's own word for
// it, not engineered around here.
//
// If stdin is not a real terminal (tcgetattr fails -- e.g. an unattended
// `ctest` run), this test does not attempt interactive control at all: it
// falls back to a short bounded run with the sphere static at its own
// initial position, the same "unattended is a real, honestly reportable
// state, not a defect" convention this project's own DeckLink tests
// already use -- and, concretely, so `ctest --test-dir build
// --output-on-failure` never blocks the whole suite waiting for a keypress
// that can never arrive.
//
// Duplicated from WU-21c's own test_decklink_live_output.cpp per
// SESSION-PROTOCOL.md rule 2 (one unit, one test, no shared fixture across
// test translation units) -- same convention every DeckLink test in this
// project already follows.
//
// Runs only against real hardware -- no Blackmagic SDK, no AppleClang/
// Xcode toolchain in the Linux cloud sandbox this was drafted in.
//
// Real-hardware setup: unchanged since WU-21e -- a live source patched
// directly into the Recorder 3G's own SDI input, Monitor 3G's own SDI
// output mirrored to HDMI for a separate by-eye display (ADR-050's own
// addendum). Run interactively at a real terminal.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "io/decklink_live_output.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

#include <termios.h>
#include <unistd.h>

using namespace scatter::io;

namespace {

constexpr BMDDisplayMode kDisplayMode = bmdModePAL;
constexpr int kWidth = 720;
constexpr int kHeight = 576;

// Full pole-to-pole/360-degree wrap, unchanged from WU-21g (ADR-053) --
// Steve's own feedback ("hugely better") was about this geometry working,
// not about changing it further; this entry does not touch it.
constexpr double kAngleSpanH = 2.0 * M_PI;  // full 360 degrees, seamless
constexpr double kAngleSpanV = M_PI;        // pole to pole, exact fold boundary

// Initial state -- all four now mutable at runtime via the keyboard
// controls below, unlike WU-21g's own file-level constants.
constexpr double kInitialRadius  = 260.0;
constexpr double kInitialCenterX = double(kWidth) / 2.0;
constexpr double kInitialCenterY = double(kHeight) / 2.0;

// Radius floor -- purely a sanity clamp against a degenerate zero/negative
// sphere from holding "I" down; no ceiling clamp (a radius large enough to
// push control vertices off the destination raster is simply dropped
// there, ADR-024's own "off-raster drop", not a crash -- the same
// unclamped-above convention every shape parameter in this project
// already has).
constexpr double kMinRadius = 20.0;

constexpr double kRotationStep = 0.05;   // radians per cursor keypress, ~2.9 degrees
constexpr double kPositionStep = 10.0;   // output pixels per shift+cursor keypress
constexpr double kRadiusStep   = 10.0;   // output pixels per I/O keypress

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

// radius/centerX/centerY are runtime state now (WU-21h), not file-level
// constants -- see this file's own header comment. angleSpanH/V stay fixed
// at the full pole-to-pole/360-degree wrap WU-21g established.
scatter::Lattice makeSphereLattice(double radius, double centerX, double centerY) {
    scatter::shapes::SphereParams sp;
    sp.radius     = radius;
    sp.angleSpanH = kAngleSpanH;
    sp.angleSpanV = kAngleSpanV;
    sp.centerX    = centerX;
    sp.centerY    = centerY;
    return scatter::shapes::buildSphereLattice(sp);
}

// Rigid rotation of every control vertex in base around (pivotX, pivotY,
// pivotZ), yaw first (x/z plane) then pitch (y/z plane) -- same
// mathematics WU-21f/g already used (CORRECTIONS.md C-017: cannot produce
// negative depth for any angle, since a rotation about a sphere's own true
// centre preserves every point's own distance from it, and every point is
// already exactly radius from that centre by construction). Takes the
// pivot as parameters now, not file-level constants, so it stays correct
// as radius/centre change under manual control. base is not modified; a
// fresh Lattice is returned, matching every shape-builder in this
// project's own "populate a fresh Lattice, return by value" convention.
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

enum class Key { Up, Down, Left, Right, ShiftUp, ShiftDown, ShiftLeft, ShiftRight, In, Out, Quit, Unknown };

// Blocks for exactly one logical keypress (which may be several raw bytes,
// for an escape-sequence arrow key) and returns what it means. See this
// file's own header comment for the xterm escape-sequence convention this
// parses, and the known bare-ESC rough edge.
Key readKey() {
    const int c = std::getchar();
    if (c == EOF) return Key::Quit;  // stdin closed under us -- treat as quit, not a spin
    if (c == 'q' || c == 'Q') return Key::Quit;
    if (c == 'i' || c == 'I') return Key::In;
    if (c == 'o' || c == 'O') return Key::Out;
    if (c != 27) return Key::Unknown;  // not ESC -- not a sequence this UI understands

    if (std::getchar() != '[') return Key::Unknown;
    const int c3 = std::getchar();
    switch (c3) {
        case 'A': return Key::Up;
        case 'B': return Key::Down;
        case 'C': return Key::Right;
        case 'D': return Key::Left;
        case '1': break;  // possible shift+arrow: "1;2<letter>" follows
        default:  return Key::Unknown;
    }

    if (std::getchar() != ';') return Key::Unknown;
    if (std::getchar() != '2') return Key::Unknown;
    switch (std::getchar()) {
        case 'A': return Key::ShiftUp;
        case 'B': return Key::ShiftDown;
        case 'C': return Key::ShiftRight;
        case 'D': return Key::ShiftLeft;
        default:  return Key::Unknown;
    }
}

}  // namespace

static void test_live_playback_manual_sphere_control() {
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

    scatter::PipelineParams params;
    params.destWidth = kWidth;
    params.destHeight = kHeight;

    double radius  = kInitialRadius;
    double centerX = kInitialCenterX;
    double centerY = kInitialCenterY;
    double yaw = 0.0, pitch = 0.0;

    CaptureConsumer consumer(ring, makeSphereLattice(radius, centerX, centerY), params);
    consumer.start();

    auto playback = LiveFramePlayback::create(output, kDisplayMode, kWidth, kHeight, consumer);
    CHECK(bool(playback));
    if (playback) {
        struct termios oldAttrs {};
        const bool interactive = (tcgetattr(STDIN_FILENO, &oldAttrs) == 0);

        if (interactive) {
            struct termios rawAttrs = oldAttrs;
            rawAttrs.c_lflag &= ~(tcflag_t(ICANON) | tcflag_t(ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &rawAttrs);

            std::fprintf(stderr,
                         "test_decklink_live_sphere: cursor keys rotate (left/right = yaw, up/down = "
                         "pitch), shift+cursor keys reposition (shift+left/right = x, shift+up/down = "
                         "y), I/O shrink/grow, Q quits.\n");

            for (;;) {
                const Key key = readKey();
                bool changed = true;
                switch (key) {
                    case Key::Left:       yaw -= kRotationStep; break;
                    case Key::Right:      yaw += kRotationStep; break;
                    case Key::Up:         pitch -= kRotationStep; break;
                    case Key::Down:       pitch += kRotationStep; break;
                    case Key::ShiftLeft:  centerX -= kPositionStep; break;
                    case Key::ShiftRight: centerX += kPositionStep; break;
                    case Key::ShiftUp:    centerY -= kPositionStep; break;
                    case Key::ShiftDown:  centerY += kPositionStep; break;
                    case Key::In:         radius = std::max(kMinRadius, radius - kRadiusStep); break;
                    case Key::Out:        radius += kRadiusStep; break;
                    case Key::Quit:       changed = false; break;
                    case Key::Unknown:    changed = false; break;
                }
                if (key == Key::Quit) break;
                if (changed)
                    consumer.setLattice(rotateLattice(makeSphereLattice(radius, centerX, centerY), yaw, pitch,
                                                        centerX, centerY, radius));
            }

            tcsetattr(STDIN_FILENO, TCSANOW, &oldAttrs);
        } else {
            std::fprintf(stderr,
                         "test_decklink_live_sphere: stdin is not a real terminal -- interactive control "
                         "is unavailable this run; falling back to a static 10-second bounded run "
                         "instead.\n");
            std::this_thread::sleep_for(std::chrono::seconds(10));
        }

        playback->stop();

        CHECK(playback->stats().completed.load() > 0);
        CHECK(playback->stats().displayedLate.load() == 0);
        CHECK(playback->stats().dropped.load() == 0);

        std::fprintf(stderr,
                     "test_decklink_live_sphere: completed=%d displayedLate=%d dropped=%d "
                     "flushed=%d framesRepeated=%d\n",
                     playback->stats().completed.load(), playback->stats().displayedLate.load(),
                     playback->stats().dropped.load(), playback->stats().flushed.load(),
                     playback->framesRepeated());
    }

    consumer.stop();
    capture->stop();

    const auto& captureStats = capture->stats();
    const auto& consumerStats = consumer.stats();
    CHECK(std::size_t(consumerStats.framesProcessed.load()) + std::size_t(consumerStats.framesFailed.load()) ==
          std::size_t(consumerStats.framesPopped.load()));
    CHECK(std::size_t(consumerStats.framesPopped.load()) <= std::size_t(captureStats.framesPushed.load()));

    std::fprintf(stderr,
                 "test_decklink_live_sphere: framesArrived=%d framesPushed=%d | framesPopped=%d "
                 "framesProcessed=%d framesFailed=%d\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 consumerStats.framesPopped.load(), consumerStats.framesProcessed.load(),
                 consumerStats.framesFailed.load());

    if (consumerStats.framesProcessed.load() == 0) {
        std::fprintf(stderr,
                     "test_decklink_live_sphere: NOTE -- zero frames processed. If no live source is "
                     "patched into the Recorder 3G's SDI input, this is expected, not a defect.\n");
    }
}

int main() {
    test_live_playback_manual_sphere_control();
    return scatter::test::summary("test_decklink_live_sphere");
}
