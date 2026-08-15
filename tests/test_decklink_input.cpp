// WU-20b -- DeckLink capture smoke test: format-detection-aware
// EnableVideoInput, IDeckLinkInputCallback, retained frames pushed into a
// ring buffer (WU-20a), against the real UltraStudio Recorder 3G.
//
// Runs only against real hardware, on the M1 Max with the UltraStudio
// Recorder 3G attached -- this cannot be built or tested in the Linux cloud
// sandbox at all (no Blackmagic SDK there; see CMakeLists.txt's
// BLACKMAGIC_SDK_DIR guard and DECISIONS.md ADR-046/ADR-047). This unit's
// own scope stops at retaining and queueing arrived frames -- reading pixel
// bytes into this project's own pipeline is WU-21's job, not this one's.
//
// Real-hardware setup this test's own Accept criteria assume: the
// UltraStudio Monitor 3G's SDI output patched directly into the Recorder
// 3G's SDI input, a genuine loopback signal. Both devices are already this
// project's real target hardware (ADR-037), so this needs no third piece of
// equipment -- just a BNC/SDI cable between the two units already in hand.
// Without that loopback connected, CaptureSource::create() and stop() still
// need to run cleanly (the mechanics this test's automated CHECKs gate on),
// but stats().framesArrived may legitimately stay at zero -- this test warns
// rather than fails in that case, since "nothing is plugged into this
// specific input right now" is a real, honestly reportable state, not a
// defect in this unit's own code. See DECISIONS.md ADR-047.

#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "io/com_ptr.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

using namespace scatter::io;

namespace {

// Matches this project's own already-confirmed-working SD display mode on
// the output side (ADR-033) -- the natural pick for a Recorder 3G capturing
// a signal fed by the Monitor 3G's own output, both of them exercising this
// project's own 576i25/576p25 development standard (ADR-007).
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;

// Selects the first enumerated device that both supports capture
// (DeviceInfo::supportsCapture, WU-14) and reports
// BMDDeckLinkSupportsInputFormatDetection -- WU-20's own "format-detection-
// aware" premise (WORK-UNITS.md) makes the latter a hard requirement of this
// unit, checked here directly (CapturePreview's own checked-not-assumed
// pattern; ADR-046) rather than left for CaptureSource::create() to fail on
// silently.
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

}  // namespace

static void test_capture_lifecycle_and_format_detection_support() {
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkInput> input = firstFormatDetectionCapableInput(devices);
    CHECK(bool(input));
    if (!input) return;

    CaptureFrameRing ring;
    auto capture = CaptureSource::create(input, kDisplayMode, ring);
    CHECK(bool(capture));
    if (!capture) return;

    // Bounded run -- a few seconds, the same order of magnitude WU-15a's own
    // bounded playback smoke test used (ADR-032/038), long enough for a real
    // signal (if the loopback cable described in this file's own header
    // comment is connected) to produce several frames, short enough for one
    // session's own terminal loop.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    capture->stop();

    const auto& stats = capture->stats();

    // Holds unconditionally, real signal or not: an arrived frame is either
    // pushed, dropped by a full ring, or filtered before ever attempting a
    // push (a no-input-source frame, or the one skipped frame at a
    // signal-recovery restart) -- never double-counted, never unaccounted
    // for. This is the one invariant this test can check without assuming
    // anything about what is or isn't physically connected.
    const std::size_t attempted = std::size_t(stats.framesPushed.load()) + ring.droppedCount();
    CHECK(attempted <= std::size_t(stats.framesArrived.load()));

    std::fprintf(stderr,
                 "test_decklink_input: framesArrived=%d framesPushed=%d noInputSourceFrames=%d "
                 "formatChanges=%d ringDropped=%zu over a 5-second bounded run\n",
                 stats.framesArrived.load(), stats.framesPushed.load(), stats.noInputSourceFrames.load(),
                 stats.formatChanges.load(), ring.droppedCount());

    if (stats.framesArrived.load() == 0) {
        std::fprintf(
            stderr,
            "test_decklink_input: NOTE -- zero frames arrived. If the Monitor 3G's SDI output is "
            "not patched into the Recorder 3G's SDI input (see this file's own header comment), "
            "this is expected, not a defect -- the mechanics above (clean create()/stop(), the "
            "accounting invariant just checked) are still the evidence this test actually gates on. "
            "With the loopback connected, this should be nonzero -- worth a second look otherwise.\n");
    } else if (stats.noInputSourceFrames.load() > 0) {
        std::fprintf(stderr,
                     "test_decklink_input: %d of %d arrived frames had no input source -- if the "
                     "loopback cable is connected and the Monitor 3G is actively playing something, "
                     "this is worth a second look.\n",
                     stats.noInputSourceFrames.load(), stats.framesArrived.load());
    }
}

int main() {
    test_capture_lifecycle_and_format_detection_support();
    return scatter::test::summary("test_decklink_input");
}
