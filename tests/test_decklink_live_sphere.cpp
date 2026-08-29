// WU-22c -- wires src/diag/coverage_view.hpp's CoverageWindow (WU-22b,
// tagged wu-22b-green) into this test's own live capture/output pipeline,
// behind a new command-line flag, --show-coverage. See DECISIONS.md ADR-058
// for the full scoping conversation and design; the load-bearing points are
// repeated here, close to the code they explain:
//
// - **Flag off (the default): this file's own pre-WU-22c behavior is
//   byte-for-byte unchanged.** No CoverageWindow is constructed, consumer's
//   own coverageCallback stays nullptr (CaptureConsumer::processOne(), per
//   ADR-058/decklink_capture_consumer.hpp, allocates and fills nothing extra
//   when it is null), and the interactive keypress loop below is the same
//   blocking std::getchar()-based readKey() loop WU-21i already established
//   -- not touched, not refactored, by this unit.
// - **Flag on: one main-thread loop, not two.** Cocoa's own requirement that
//   CoverageWindow::run()/updateWeights() execute on the main thread, and
//   this test's own pre-existing terminal-keypress loop, both want the one
//   thread this process actually has at the point the interactive loop
//   starts (no thread has been spawned for either purpose before this).
//   Rather than inventing a second thread for one of them, both are unified
//   onto that single main thread using GCD dispatch sources: a
//   DISPATCH_SOURCE_TYPE_READ source on STDIN_FILENO, queued onto
//   dispatch_get_main_queue(), interleaves with CoverageWindow::run()'s own
//   Cocoa run loop on that same thread. This is also why the flag-on path
//   cannot reuse readKey(): a blocking std::getchar() call inside a
//   dispatch source's own event handler would freeze the Cocoa run loop
//   itself on a stray ESC keypress, not just terminal input -- see
//   IncrementalKeyParser below, a non-blocking incremental counterpart.
//   Function-pointer GCD APIs only (dispatch_source_set_event_handler_f,
//   dispatch_async_f) -- no Objective-C Blocks -- so this stays a plain
//   .cpp file, not a .mm, the same boundary com_ptr.hpp/decklink_device.hpp
//   already keep for the Blackmagic SDK and coverage_view.hpp/.mm now keep
//   for Cocoa/Metal (ADR-031/057).
// - **One Q, from either channel, quits the whole session.** Pressing Q (or
//   closing the window) inside the coverage window already made
//   CoverageWindow::run() return, via the existing -requestQuit ObjC method
//   (coverage_view.mm, WU-22b). WU-22c adds the other direction: the stdin
//   dispatch source's own event handler, on recognizing Q from the
//   terminal, calls the new CoverageWindow::requestQuit() (ADR-058) so that
//   channel also ends run(). Either channel firing makes run() return, at
//   which point this function's own existing post-loop cleanup
//   (playback->stop(); consumer.stop(); capture->stop(); stats) runs
//   exactly as it already did before this unit -- "quits everything" falls
//   out of that existing structure rather than needing new shutdown code.
// - **The coverage window never opens in a non-interactive run.** The same
//   "unattended is a real, honestly reportable state, not a defect"
//   fallback WU-21i already uses when stdin is not a real terminal (e.g. an
//   unattended `ctest` run) stays exactly as it was: no interactive control,
//   a short bounded sleep. A Cocoa window has no quit signal available to
//   it in that fallback (nothing will ever type Q), so opening one there
//   would hang `ctest` forever instead of completing after ten seconds --
//   --show-coverage is therefore silently downgraded to off whenever stdin
//   is not a real terminal (a NOTE is still printed to stderr so this is
//   visible, not silent-silent). See coverageActive below.
// - **Redraw cadence: every processed live frame, no throttling.** Matches
//   CoverageWindow's own MTKView enableSetNeedsDisplay=YES/paused=YES mode
//   (ADR-057) -- the window only redraws when told to, and this unit tells
//   it once per frame CaptureConsumer successfully processes, the same
//   "every frame, not sampled" cadence the rest of this live pipeline
//   already uses end to end.
//
// Everything below this comment that is not new for WU-22c -- letter-key
// control scheme, sphere geometry, rotation mathematics, non-interactive
// fallback -- is WU-21i's own content (see git history / DECISIONS.md
// ADR-053/054/055 for that unit's own reasoning), carried forward unchanged
// except where this comment says otherwise.
//
// Runs only against real hardware -- no Blackmagic SDK, no AppleClang/
// Xcode toolchain, no Cocoa/Metal, in the Linux cloud sandbox this was
// drafted in. UNVERIFIED, same as coverage_view.hpp/.mm (WU-22b): reasoned
// through, not built or run by the session that wrote it. Build and run
// this at your own real terminal.
//
// Real-hardware setup: unchanged since WU-21e -- a live source patched
// directly into the Recorder 3G's own SDI input, Monitor 3G's own SDI
// output mirrored to HDMI for a separate by-eye display (ADR-050's own
// addendum). Run interactively at a real terminal.
//
// Duplicated from WU-21c's own test_decklink_live_output.cpp per
// SESSION-PROTOCOL.md rule 2 (one unit, one test, no shared fixture across
// test translation units) -- same convention every DeckLink test in this
// project already follows.
//
// WU-23b2b (DECISIONS.md ADR-080, extended by ADR-081): CaptureConsumer's
// constructor now takes a required DeinterlaceCoefficients parameter --
// this file's own construction call below passes Complex explicitly
// (Steve's own choice, ADR-081), inserted ahead of the existing
// coverageCallback parameter (WU-22c), which stays last. The accounting
// invariant below genuinely does widen to a third term, framesStreamStart
// (CORRECTIONS.md C-029 corrects an earlier, wrong claim in this same
// comment that it would not).
//
// WU-35a2 (DECISIONS.md ADR-088, WORK-UNITS.md WU-35a2), [P]-tier per
// ADR-087/ADR-088 (inherited, not decided here): two new letter keys,
// T/t, adjust PipelineParams::manualTransp (core/resolve.hpp, WU-35a1/
// wu-35a1-green) live via the same increment/decrement scheme WU-21i
// established for X/x/Y/y/Z/z -- see kTranspStep and the Key::TInc/TDec
// arms below. params.kBufferMode is also set to Blend here for the first
// time in this file (WU-28d's/WU-35a's own long-scoped intent, unblocked
// by WU-28c, wu-28c-green): without it, compositeKBuffer()'s own Blend
// fold never runs and manualTransp has no effect at all (Opaque mode
// ignores it outright, core/resolve.hpp). **Read before assuming this
// makes the sweep visible on a real SDI monitor:** CaptureConsumer's own
// m_params (io/decklink_capture_consumer.hpp) is copied in once at
// construction and held const, with no live-update path the way
// setLattice() (WU-21f) gives the lattice -- so pressing T/t updates this
// file's own local manualTransp variable (and, via it, what gets printed)
// but not yet the consumer thread's own copy of PipelineParams. Making
// the sweep actually reach the output needs a small, separate addition to
// io/decklink_capture_consumer.hpp/.cpp (a manualTransp counterpart to
// setLattice()) -- out of this unit's own one-file scope (WORK-UNITS.md's
// own WU-35a2 `Files:` line) and not implemented here. See this session's
// own HANDOFF.md for the full account.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "diag/coverage_view.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "io/decklink_live_output.hpp"
#include "video/deinterlace.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <dispatch/dispatch.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

#include <cerrno>
#include <fcntl.h>
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
// sphere from holding "z" down; no ceiling clamp (a radius large enough to
// push control vertices off the destination raster is simply dropped
// there, ADR-024's own "off-raster drop", not a crash -- the same
// unclamped-above convention every shape parameter in this project
// already has).
constexpr double kMinRadius = 20.0;

constexpr double kRotationStep = 0.05;   // radians per cursor keypress, ~2.9 degrees
constexpr double kPositionStep = 10.0;   // output pixels per X/x/Y/y keypress
constexpr double kRadiusStep   = 10.0;   // output pixels per Z/z keypress

// WU-35a2: manualTransp (core/resolve.hpp PipelineParams::manualTransp,
// WU-35a1/DECISIONS.md ADR-088) is Weight -- 1.15 fixed point, range
// [0, kWeightUnity] per resolve.hpp's own documented contract -- not an
// output-pixel quantity like the three steps above, so its own step is
// expressed directly in that fixed-point unit rather than converted
// from/to double. kWeightUnity / 16 divides the whole range into 16
// keypresses each direction and lands exactly on fixture 30's own three
// checkpoints (docs/sources/WU-SM-02.md fixture 30): T=0 at rest,
// T=0.5 (kWeightUnity/2) at exactly 8 T presses, T=1 (kWeightUnity) at
// exactly 16 -- no rounding drift at any of the three.
constexpr int kTranspStep = int(scatter::kWeightUnity) / 16;  // 2048

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

enum class Key { Up, Down, Left, Right, XInc, XDec, YInc, YDec, ZInc, ZDec, TInc, TDec, Quit, Unknown };

// Blocks for exactly one logical keypress (which may be several raw bytes,
// for an escape-sequence arrow key) and returns what it means. See this
// file's own header comment for the xterm escape-sequence convention this
// parses (plain arrows only, since WU-21h's own shift+arrow lookahead did
// not work on real hardware -- C-018), and the known bare-ESC rough edge.
//
// WU-22c: used only by the flag-off (--show-coverage absent) branch below,
// unchanged -- see IncrementalKeyParser further down for the flag-on
// branch's own non-blocking counterpart, which does not call this.
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

// WU-22c: the same key -> state-change mapping the flag-off loop below
// applies inline, factored out here so the flag-on loop's own incremental
// handler (handleCoverageStdinReadable, below) can apply it too. Not used
// by the flag-off loop itself -- that loop's own inline switch stays
// exactly as WU-21i wrote it, untouched, per this file's own header
// comment on why the flag-off path is byte-for-byte unchanged. Duplicating
// this small mapping rather than routing the flag-off loop through it too
// is deliberate: this project's own established convention is "one unit,
// one test, no shared fixture" across whole test files (this file's own
// header comment); applied here at finer grain, between this one file's
// two loops, for the same reason -- the two loops are different enough in
// shape (blocking single evaluation vs incremental per-byte, main-thread
// dispatch-source-driven) that forcing them through one shared function
// would need its own abstraction, not remove one, and would put the
// flag-off path's own untouched-since-WU-21i status at risk for no benefit.
bool applyKey(Key key, double& yaw, double& pitch, double& centerX, double& centerY, double& radius,
              scatter::Weight& manualTransp) {
    switch (key) {
        case Key::Left:    yaw -= kRotationStep; return true;
        case Key::Right:   yaw += kRotationStep; return true;
        case Key::Up:      pitch -= kRotationStep; return true;
        case Key::Down:    pitch += kRotationStep; return true;
        case Key::XInc:    centerX += kPositionStep; return true;
        case Key::XDec:    centerX -= kPositionStep; return true;
        case Key::YInc:    centerY += kPositionStep; return true;
        case Key::YDec:    centerY -= kPositionStep; return true;
        case Key::ZInc:    radius += kRadiusStep; return true;
        case Key::ZDec:    radius = std::max(kMinRadius, radius - kRadiusStep); return true;
        // WU-35a2 (DECISIONS.md ADR-088, WORK-UNITS.md WU-35a2): clamped to
        // [0, kWeightUnity] both directions -- resolve.hpp's own documented
        // contract for manualTransp ("a value outside that range is a
        // caller error"), not a new range decision made here. Both
        // operands promote to int regardless of Weight's uint16_t storage
        // (ordinary C++ integer promotion), so a T-below-kTranspStep
        // decrement clamps at 0 instead of wrapping.
        case Key::TInc:
            manualTransp = scatter::Weight(std::min(int(scatter::kWeightUnity), int(manualTransp) + kTranspStep));
            return true;
        case Key::TDec:
            manualTransp = scatter::Weight(std::max(0, int(manualTransp) - kTranspStep));
            return true;
        case Key::Quit:    return false;
        case Key::Unknown: return false;
    }
    return false;
}

// WU-22c's own same-session follow-up: maps a keydown the coverage window
// itself received (`CoverageWindow::setKeyHandler()`, coverage_view.hpp)
// to this file's own Key enum -- Steve's own real-terminal build reported
// the coverage window not responding to anything but Q (macOS keyboard
// focus is per-window; the terminal's own stdin channel never sees a
// keystroke typed while the coverage window itself has focus), so this is
// the coverage-window-focused counterpart to IncrementalKeyParser's own
// stdin-focused byte parsing below -- much simpler, since Cocoa's own
// NSEvent has already fully decoded the key by the time it reaches here,
// leaving nothing for a state machine to do. A third small duplicate of
// the same six-letter/four-arrow mapping (after readKey() and
// IncrementalKeyParser::mapLetter), for the same reason both of those stay
// separate from each other: three different-shaped call sites (blocking
// per-keypress, incremental per-byte, one fully-decoded Cocoa event at a
// time), no shared abstraction worth inventing to save eleven lines twice.
Key mapCoverageWindowKey(char asciiChar, scatter::diag::SpecialKey special) {
    switch (special) {
        case scatter::diag::SpecialKey::ArrowUp:    return Key::Up;
        case scatter::diag::SpecialKey::ArrowDown:  return Key::Down;
        case scatter::diag::SpecialKey::ArrowLeft:  return Key::Left;
        case scatter::diag::SpecialKey::ArrowRight: return Key::Right;
        case scatter::diag::SpecialKey::None:       break;
    }
    switch (asciiChar) {
        case 'X': return Key::XInc;
        case 'x': return Key::XDec;
        case 'Y': return Key::YInc;
        case 'y': return Key::YDec;
        case 'Z': return Key::ZInc;
        case 'z': return Key::ZDec;
        case 'T': return Key::TInc;
        case 't': return Key::TDec;
        default:  return Key::Unknown;
    }
}

// WU-22c: non-blocking incremental counterpart to readKey() above, for the
// flag-on path's own stdin dispatch source (main-thread event handler;
// must never block -- see this file's own header comment for why blocking
// here would freeze the whole Cocoa event loop, not just terminal input,
// on a single stray ESC keypress). Same state machine readKey() already
// runs inline (ESC, then '[', then the letter), reified as explicit states
// here since bytes now arrive from a non-blocking read() a few at a time
// instead of via consecutive blocking std::getchar() calls.
class IncrementalKeyParser {
public:
    struct Outcome {
        bool complete = false;  // true: a full logical keypress was just decided (may be Key::Unknown)
        Key key = Key::Unknown;
    };

    // Feeds one raw input byte. complete=true means a full logical keypress
    // has just been decided -- a plain letter key, Quit, a fully recognized
    // arrow-key escape sequence, or Key::Unknown for a byte/sequence this
    // UI does not recognize (the caller ignores an Unknown outcome, same as
    // the blocking readKey() loop already does for its own Key::Unknown).
    // complete=false means this byte started or continued an escape
    // sequence and more bytes are awaited -- nothing to act on yet.
    Outcome feed(unsigned char c) {
        switch (m_state) {
            case State::kIdle:
                if (c == 27) {  // ESC
                    m_state = State::kSawEsc;
                    return {false, Key::Unknown};
                }
                return {true, mapLetter(c)};

            case State::kSawEsc:
                m_state = State::kIdle;
                if (c == '[') {
                    m_state = State::kSawEscBracket;
                    return {false, Key::Unknown};
                }
                // ESC not followed by '[' -- not a sequence this UI
                // understands (same as readKey()'s own fallthrough).
                return {true, Key::Unknown};

            case State::kSawEscBracket:
                m_state = State::kIdle;
                switch (c) {
                    case 'A': return {true, Key::Up};
                    case 'B': return {true, Key::Down};
                    case 'C': return {true, Key::Right};
                    case 'D': return {true, Key::Left};
                    default:  return {true, Key::Unknown};
                }
        }
        return {true, Key::Unknown};  // unreachable; silences -Wreturn-type
    }

private:
    enum class State { kIdle, kSawEsc, kSawEscBracket };

    static Key mapLetter(unsigned char c) {
        if (c == 'q' || c == 'Q') return Key::Quit;
        if (c == 'X') return Key::XInc;
        if (c == 'x') return Key::XDec;
        if (c == 'Y') return Key::YInc;
        if (c == 'y') return Key::YDec;
        if (c == 'Z') return Key::ZInc;
        if (c == 'z') return Key::ZDec;
        if (c == 'T') return Key::TInc;
        if (c == 't') return Key::TDec;
        return Key::Unknown;
    }

    State m_state = State::kIdle;
};

// WU-22c: context for the flag-on path's own stdin dispatch source event
// handler (handleCoverageStdinReadable, below). Bundles exactly the state
// that handler needs each time it fires. Every field here is touched only
// from the main thread -- the thread dispatch_get_main_queue() always runs
// on, same thread CoverageWindow's own updateWeights()/run()/requestQuit()
// require (ADR-057/058) -- so nothing here needs its own lock.
struct CoverageInputContext {
    IncrementalKeyParser parser;
    CaptureConsumer* consumer = nullptr;
    scatter::diag::CoverageWindow* window = nullptr;
    double* yaw = nullptr;
    double* pitch = nullptr;
    double* centerX = nullptr;
    double* centerY = nullptr;
    double* radius = nullptr;
    scatter::Weight* manualTransp = nullptr;  // WU-35a2
};

// WU-22c: dispatch_source_set_event_handler_f's own callback -- the
// function-pointer variant (no Objective-C Blocks), matching this file's
// own header comment on why it stays a plain .cpp. Runs on the main
// thread, once per stdin-readable event. STDIN_FILENO is set O_NONBLOCK
// before this source is created (see the call site below), so the read()
// loop here never blocks -- a design requirement, not a convenience, since
// blocking would freeze CoverageWindow's own Cocoa run loop, not just
// terminal input.
void handleCoverageStdinReadable(void* rawContext) {
    auto* ctx = static_cast<CoverageInputContext*>(rawContext);

    unsigned char buf[64];
    for (;;) {
        const ssize_t n = ::read(STDIN_FILENO, buf, sizeof(buf));
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                const IncrementalKeyParser::Outcome outcome = ctx->parser.feed(buf[std::size_t(i)]);
                if (!outcome.complete) continue;
                if (outcome.key == Key::Quit) {
                    ctx->window->requestQuit();
                    return;
                }
                if (applyKey(outcome.key, *ctx->yaw, *ctx->pitch, *ctx->centerX, *ctx->centerY, *ctx->radius,
                             *ctx->manualTransp)) {
                    ctx->consumer->setLattice(rotateLattice(
                        makeSphereLattice(*ctx->radius, *ctx->centerX, *ctx->centerY), *ctx->yaw, *ctx->pitch,
                        *ctx->centerX, *ctx->centerY, *ctx->radius));
                }
            }
            if (std::size_t(n) < sizeof(buf)) break;  // drained everything currently available
            continue;                                 // buf was completely full -- more may be waiting
        }
        if (n == 0) {
            // stdin closed under us -- same "treat as quit, not a spin"
            // convention readKey() already uses for EOF.
            ctx->window->requestQuit();
            return;
        }
        // n < 0
        if (errno == EAGAIN || errno == EWOULDBLOCK) break;  // nothing more available right now
        // Any other errno: stop trying to read further from this source --
        // treat as quit rather than spin the event handler on a broken fd.
        ctx->window->requestQuit();
        return;
    }
}

// WU-22c: heap-allocated per-call handoff from CaptureConsumer's own
// dedicated consumer thread to Cocoa's main thread, via dispatch_async_f
// (the function-pointer variant -- no Objective-C Blocks, keeping this a
// plain .cpp file). Each CoverageCallback invocation (below, in the test
// function) allocates exactly one of these; applyCoverageOnMainThread()
// reclaims it. This is the concrete shape ADR-057/058's own "double-buffer
// / dispatch_async hand-off" design intent takes once implemented: rather
// than a fixed pair of reusable buffers, each processed frame's own
// coverage buffer is handed across whole, via std::vector's own move
// semantics plus one small heap allocation for the context struct itself
// -- CoverageWindow still only ever observes one buffer at a time, on the
// main thread, with no locking of its own, the same property a literal
// fixed double-buffer would have given, at the cost of one small
// allocation/free per displayed frame instead of a fixed pair reused
// forever. See ADR-058 for why this was chosen over a literal two-buffer
// pool.
struct CoverageDispatchContext {
    scatter::diag::CoverageWindow* window;
    std::vector<scatter::WeightAccum> weights;
    int width;
    int height;
};

void applyCoverageOnMainThread(void* rawContext) {
    std::unique_ptr<CoverageDispatchContext> ctx(static_cast<CoverageDispatchContext*>(rawContext));
    ctx->window->updateWeights(ctx->weights.data(), ctx->width, ctx->height);
}

}  // namespace

static void test_live_playback_manual_sphere_control_letter_keys(bool showCoverage) {
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

    // WU-35a2 (DECISIONS.md ADR-088, WORK-UNITS.md WU-35a2): the operator-
    // facing counterpart to PipelineParams::manualTransp (core/resolve.hpp,
    // WU-35a1/wu-35a1-green) -- same [0, kWeightUnity] range and type as
    // the field itself, updated live by the T/t keys below via applyKey().
    // See this file's own header comment (top of file) for why this does
    // NOT yet reach the live composited/SDI output: CaptureConsumer's own
    // m_params is copied in once at construction and held const, unlike
    // the lattice (setLattice(), WU-21f) -- pressing T/t updates this
    // variable (and what gets printed) only, until io/decklink_capture_
    // consumer.hpp/.cpp gets its own small follow-up, out of this unit's
    // one-file scope.
    scatter::Weight manualTransp = 0;

    // WU-35a2: Blend, not WU-28d's own never-built Opaque/Blend toggle --
    // this unit's whole point is making manualTransp (Blend-mode-only per
    // core/resolve.hpp's own compositeKBuffer() doc comment; Opaque
    // ignores it outright) operator-visible, so Blend is the only mode
    // that makes wiring the T/t keys meaningful. Fixed for this
    // CaptureConsumer's whole lifetime, same as destWidth/destHeight above
    // -- kBufferMode itself is not one of this unit's own two new live
    // controls. params.manualTransp seeds the consumer's own initial value
    // from manualTransp above (both 0 here) -- see that variable's own
    // comment for why later T/t presses do not reach this params copy
    // again after construction.
    params.kBufferMode = scatter::KBufferResolveMode::Blend;
    params.manualTransp = manualTransp;

    // WU-22c: whether stdin is a real terminal is queried here, up front --
    // moved earlier than WU-21i's own original placement (previously just
    // inside the `if (playback)` block below) so it is known before
    // CaptureConsumer is constructed, which coverageActive's own value
    // below needs to decide. tcgetattr() itself is a pure query with no
    // side effect on the terminal, so moving the call earlier changes
    // nothing observable about the flag-off path -- byte-identical
    // behavior, just decided a few lines sooner in this function's own
    // source order.
    struct termios oldAttrs {};
    const bool interactive = (tcgetattr(STDIN_FILENO, &oldAttrs) == 0);

    // WU-22c: --show-coverage is silently (but audibly, via stderr)
    // downgraded to off whenever stdin is not a real terminal -- see this
    // file's own header comment for why a Cocoa window must never open in
    // that fallback.
    const bool coverageActive = showCoverage && interactive;
    if (showCoverage && !interactive) {
        std::fprintf(stderr,
                     "test_decklink_live_sphere: --show-coverage given but stdin is not a real "
                     "terminal -- skipping the coverage window (it would never receive a quit "
                     "signal in an unattended run); falling back to the same static run every "
                     "non-interactive run already uses.\n");
    }

    // WU-22c: CoverageWindow, when active, is constructed here -- before
    // CaptureConsumer -- specifically so its address is available to
    // capture into coverageCallback below, which CaptureConsumer's own
    // constructor needs. This also satisfies Cocoa's own main-thread
    // requirement for window/Metal-device creation: nothing has spawned a
    // thread yet at this point in this function, so this is still running
    // on the process's actual main thread.
    std::unique_ptr<scatter::diag::CoverageWindow> coverageWindow;
    CaptureConsumer::CoverageCallback coverageCallback = nullptr;
    if (coverageActive) {
        scatter::diag::CoverageWindowConfig coverageConfig;
        coverageConfig.width = kWidth;
        coverageConfig.height = kHeight;
        coverageWindow = std::make_unique<scatter::diag::CoverageWindow>(coverageConfig);

        // Runs on CaptureConsumer's own dedicated consumer thread (not the
        // main thread, not the DeckLink driver's own callback thread) --
        // see decklink_capture_consumer.hpp's own doc comment on
        // CoverageCallback. Hands the freshly produced coverage buffer to
        // the main thread via dispatch_async_f; see CoverageDispatchContext
        // above for why a heap allocation per frame, not a literal
        // reusable double-buffer.
        scatter::diag::CoverageWindow* windowPtr = coverageWindow.get();
        coverageCallback = [windowPtr](std::vector<scatter::WeightAccum> weights) {
            auto* ctx = new CoverageDispatchContext{windowPtr, std::move(weights), kWidth, kHeight};
            dispatch_async_f(dispatch_get_main_queue(), ctx, &applyCoverageOnMainThread);
        };
    }

    // WU-23b2b (ADR-080/081): DeinterlaceCoefficients has no default --
    // Complex, Steve's own explicit choice, same as every other caller in
    // this project -- inserted ahead of coverageCallback, which stays this
    // constructor's own last (defaulted) parameter.
    CaptureConsumer consumer(ring, makeSphereLattice(radius, centerX, centerY), params,
                              scatter::video::DeinterlaceCoefficients::Complex, coverageCallback);
    consumer.start();

    auto playback = LiveFramePlayback::create(output, kDisplayMode, kWidth, kHeight, consumer);
    CHECK(bool(playback));
    if (playback) {
        if (interactive) {
            struct termios rawAttrs = oldAttrs;
            rawAttrs.c_lflag &= ~(tcflag_t(ICANON) | tcflag_t(ECHO));
            tcsetattr(STDIN_FILENO, TCSANOW, &rawAttrs);

            if (coverageActive) {
                // WU-22c: unified main-thread loop -- see this file's own
                // header comment for the full design. STDIN_FILENO is set
                // non-blocking here (restored below) because the dispatch
                // source's own event handler must never block.
                const int oldStdinFlags = fcntl(STDIN_FILENO, F_GETFL, 0);
                fcntl(STDIN_FILENO, F_SETFL, oldStdinFlags | O_NONBLOCK);

                CoverageInputContext inputContext;
                inputContext.consumer = &consumer;
                inputContext.window = coverageWindow.get();
                inputContext.yaw = &yaw;
                inputContext.pitch = &pitch;
                inputContext.centerX = &centerX;
                inputContext.centerY = &centerY;
                inputContext.radius = &radius;
                inputContext.manualTransp = &manualTransp;

                dispatch_source_t stdinSource = dispatch_source_create(
                    DISPATCH_SOURCE_TYPE_READ, uintptr_t(STDIN_FILENO), 0, dispatch_get_main_queue());
                dispatch_set_context(stdinSource, &inputContext);
                dispatch_source_set_event_handler_f(stdinSource, &handleCoverageStdinReadable);
                dispatch_resume(stdinSource);

                // Seed the window with a real (zero) buffer before run()
                // starts the Cocoa run loop -- coverage_view.hpp's own doc
                // comment on run() calls for updateWeights() to have been
                // called at least once first.
                std::vector<scatter::WeightAccum> zeroWeights(std::size_t(kWidth) * std::size_t(kHeight),
                                                                scatter::WeightAccum(0));
                coverageWindow->updateWeights(zeroWeights.data(), kWidth, kHeight);

                // WU-22c's own same-session follow-up: keyboard focus is
                // per-window in macOS, so the stdin dispatch source set up
                // above only ever sees a keystroke typed while the
                // *terminal* has focus -- clicking into the coverage
                // window instead sends keydowns to that window's own
                // -keyDown:, which otherwise only understands Q. This
                // handler makes the coverage window itself fully
                // interactive too, via the same applyKey()/setLattice()
                // logic the terminal channel already uses -- both
                // channels drive the same state, on the same (main)
                // thread, so no new synchronisation is needed here.
                coverageWindow->setKeyHandler(
                    [&consumer, &yaw, &pitch, &centerX, &centerY, &radius, &manualTransp](
                        char asciiChar, scatter::diag::SpecialKey special) {
                        const Key key = mapCoverageWindowKey(asciiChar, special);
                        if (applyKey(key, yaw, pitch, centerX, centerY, radius, manualTransp)) {
                            consumer.setLattice(rotateLattice(makeSphereLattice(radius, centerX, centerY), yaw,
                                                                pitch, centerX, centerY, radius));
                        }
                    });

                std::fprintf(stderr,
                             "test_decklink_live_sphere: cursor keys rotate (left/right = yaw, up/down = "
                             "pitch); X/x = position right/left, Y/y = position down/up, Z/z = bigger/"
                             "smaller, T/t = more/less transparent (WU-35a2: local/printed only, not "
                             "yet wired to the live output -- see this file's own header comment). Q "
                             "quits. All of this works from either the terminal or the coverage window "
                             "-- click into whichever one you want focused.\n");

                coverageWindow->run();  // blocks until Q (either channel) or window close

                dispatch_source_cancel(stdinSource);
                dispatch_release(stdinSource);
                fcntl(STDIN_FILENO, F_SETFL, oldStdinFlags);
            } else {
                std::fprintf(stderr,
                             "test_decklink_live_sphere: cursor keys rotate (left/right = yaw, up/down = "
                             "pitch); X/x = position right/left, Y/y = position down/up, Z/z = bigger/"
                             "smaller, T/t = more/less transparent (WU-35a2: local/printed only, not yet "
                             "wired to the live output -- see this file's own header comment). Q "
                             "quits.\n");

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
                        // WU-35a2: same clamp semantics as applyKey() above --
                        // this loop does not call that function (this file's
                        // own header comment explains why the flag-off loop's
                        // switch is kept a separate inline copy).
                        case Key::TInc:
                            manualTransp = scatter::Weight(
                                std::min(int(scatter::kWeightUnity), int(manualTransp) + kTranspStep));
                            break;
                        case Key::TDec:
                            manualTransp = scatter::Weight(std::max(0, int(manualTransp) - kTranspStep));
                            break;
                        case Key::Quit:    changed = false; break;
                        case Key::Unknown: changed = false; break;
                    }
                    if (key == Key::Quit) break;
                    if (changed)
                        consumer.setLattice(rotateLattice(makeSphereLattice(radius, centerX, centerY), yaw, pitch,
                                                            centerX, centerY, radius));
                }
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
    // WU-23b2b (ADR-080/081): widened to a third term, framesStreamStart --
    // this file's own earlier header-comment claim that it "never reads
    // CaptureConsumerStats::framesStreamStart" was wrong; this invariant
    // reads it implicitly through the sum, and Steve's own real-terminal
    // ctest run caught the stale two-term version failing (CORRECTIONS.md
    // C-029): a fresh CaptureConsumer's very first successfully-decoded
    // frame is always counted here, not by framesProcessed or
    // framesFailed.
    CHECK(std::size_t(consumerStats.framesProcessed.load()) + std::size_t(consumerStats.framesFailed.load()) +
              std::size_t(consumerStats.framesStreamStart.load()) ==
          std::size_t(consumerStats.framesPopped.load()));
    CHECK(std::size_t(consumerStats.framesPopped.load()) <= std::size_t(captureStats.framesPushed.load()));

    std::fprintf(stderr,
                 "test_decklink_live_sphere: framesArrived=%d framesPushed=%d | framesPopped=%d "
                 "framesProcessed=%d framesFailed=%d framesStreamStart=%d\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 consumerStats.framesPopped.load(), consumerStats.framesProcessed.load(),
                 consumerStats.framesFailed.load(), consumerStats.framesStreamStart.load());

    if (consumerStats.framesProcessed.load() == 0) {
        std::fprintf(stderr,
                     "test_decklink_live_sphere: NOTE -- zero frames processed. If no live source is "
                     "patched into the Recorder 3G's SDI input, this is expected, not a defect.\n");
    }
}

int main(int argc, char** argv) {
    // WU-22c: the only new command-line surface this unit adds. Any other
    // argument is silently ignored, matching this project's other test
    // executables (none of which parse argv today) -- not treated as an
    // error, since ctest may invoke this binary with its own arguments this
    // unit has no reason to reject.
    bool showCoverage = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--show-coverage") == 0) showCoverage = true;
    }

    test_live_playback_manual_sphere_control_letter_keys(showCoverage);
    return scatter::test::summary("test_decklink_live_sphere");
}
