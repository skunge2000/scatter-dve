// WU-33c2b -- DeckLink back-source smoke test: selectFormatDetectionCapableInput()
// (io/decklink_back_source.hpp) generalizing this repository's own
// firstFormatDetectionCapableInput() convention from "always the first
// match" to "caller-selected, by index or name," and DeckLinkBackSource
// draining a CaptureFrameRing into OwnedSourceRaster snapshots via
// unpackSourceRaster() (WU-33c1) -- no runFrame(), no output v210 bytes, the
// sibling of io/file_back_source.hpp's own FileBackSource (WU-33c2a) behind
// the same conceptual accessor (see io/decklink_back_source.hpp's own file
// comment for why this one returns std::optional<OwnedSourceRaster>, not
// the plain OwnedSourceRaster FileBackSource returns).
//
// DECISIONS.md ADR-096 for the full design account.
//
// Runs only against real hardware -- no Blackmagic SDK and no AppleClang/
// Xcode toolchain exist in the Linux cloud sandbox or the device bridge's
// own Linux VM this file was drafted in (see CMakeLists.txt's own
// BLACKMAGIC_SDK_DIR guard). Written, not built, not run, not confirmed
// against real hardware by this or any prior session -- do not claim any of
// this works until Steve has run it, at his own real terminal on the
// MacBook Pro with the UltraStudio 4K Mini (and whatever else may be
// attached), and said so.
//
// Four checks, the first three needing only enumerateDeckLinkDevices() (no
// stream ever opened, so they run regardless of what is or is not patched
// into any SDI input):
//
//   1. A default-constructed DeckLinkBackDeviceSelector (neither index nor
//      nameSubstring set) selects the exact same IDeckLinkInput this
//      repository's own existing firstFormatDetectionCapableInput()
//      convention would -- confirmed by comparing the selected ComPtr's own
//      get() against a locally re-implemented first-match scan, the same
//      "never call the trusted path itself, only its own already-
//      independently-tested building blocks" shape this project's own
//      unpack/back-source tests already use (test_unpack_source_raster.cpp
//      check 1, test_file_back_source.cpp check 1) -- here applied to
//      selection instead of pixel content.
//   2. Selecting by explicit index 0 selects that same first match again --
//      confirms the index path is wired correctly even on a single-device
//      machine, where index 0 and "first match" are the only entry there is
//      to select.
//   3. Selecting by an out-of-range index (one past the number of real
//      matches) returns a null ComPtr, never a stale or wrapped-around
//      match.
//   4. Real hardware, bounded 5-second run (the same order of magnitude
//      test_decklink_capture_consumer.cpp's own bounded smoke test already
//      uses): DeckLinkBackSource, given a CaptureSource/CaptureFrameRing
//      pair against the first format-detection-capable device (this test
//      does not require a second physical device or sub-device to exist --
//      see the note below), starts and stops cleanly, and its own
//      framesPopped/framesProcessed/framesFailed accounting invariant
//      holds. Warns rather than fails if framesProcessed stays at zero --
//      the same "nothing plugged in right now is a real, honestly
//      reportable state, not a defect" convention
//      test_decklink_capture_consumer.cpp already uses.
//
// **Not covered by any check here, and not covered by any check in this
// repository as of this session:** WORK-UNITS.md's own WU-33c2b Accept
// criterion also names "a second, explicit check that the selection
// mechanism actually distinguishes two real sub-devices or two real
// physical devices when both are attached." Checks 1-3 above exercise the
// selection *mechanism* (index/name/default precedence) honestly against
// whatever enumerateDeckLinkDevices() reports on whatever machine this runs
// on -- including a single-device machine, where they still pass, correctly,
// without proving multi-device selection specifically. A real multi-
// device/multi-sub-device confirmation needs a second capture-capable
// device or sub-device physically attached, which this session cannot
// confirm exists on Steve's own machine (HANDOFF.md's own Session 79 entry
// already flagged this as unanswered -- his UltraStudio 4K Mini's own
// single capture path is confirmed, whether anything else attached also
// enumerates as capture-capable is not). Whoever runs this at a real
// terminal with two such devices attached should add that check then,
// against the real enumerateDeckLinkDevices() output on that machine, per
// this project's own "re-derive against the real current state, do not
// assume this note's own sketch still matches" convention -- not invented
// here without hardware to confirm it against.

#include "core/resolve.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_back_source.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

using namespace scatter::io;

namespace {

// Duplicated locally from every other DeckLink test file in this
// repository, per that same file-local-namespace convention (see e.g.
// tests/test_decklink_capture_consumer.cpp) -- used only as this test's own
// independent oracle for check 1 below, never as the mechanism under test.
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

// Same display mode every other DeckLink test in this repository uses
// (ADR-033/ADR-007): this project's own confirmed-working 576i25/576p25
// development standard.
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;

}  // namespace

// ---------------------------------------------------------------------------
// 1. Default selector matches the existing first-match convention.
// ---------------------------------------------------------------------------

static void test_selector_default_matches_first_format_detection_capable_input() {
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkInput> expected = firstFormatDetectionCapableInput(devices);
    ComPtr<IDeckLinkInput> actual = selectFormatDetectionCapableInput(devices);

    CHECK(bool(expected) == bool(actual));
    if (expected && actual)
        CHECK(expected.get() == actual.get());
}

// ---------------------------------------------------------------------------
// 2. Explicit index 0 matches the same first match.
// ---------------------------------------------------------------------------

static void test_selector_index_zero_matches_first_match() {
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkInput> expected = firstFormatDetectionCapableInput(devices);
    if (!expected) return;  // no format-detection-capable device at all -- covered by check 1

    DeckLinkBackDeviceSelector selector;
    selector.index = 0;
    ComPtr<IDeckLinkInput> actual = selectFormatDetectionCapableInput(devices, selector);

    CHECK(bool(actual));
    if (actual) CHECK(expected.get() == actual.get());
}

// ---------------------------------------------------------------------------
// 3. Out-of-range index returns null, never a wrapped-around match.
// ---------------------------------------------------------------------------

static void test_selector_out_of_range_index_returns_null() {
    const auto devices = enumerateDeckLinkDevices();

    DeckLinkBackDeviceSelector selector;
    selector.index = devices.size() + 1000;  // guaranteed past any real match count
    ComPtr<IDeckLinkInput> actual = selectFormatDetectionCapableInput(devices, selector);

    CHECK(!actual);
}

// ---------------------------------------------------------------------------
// 4. Real hardware: drains a live ring into OwnedSourceRaster snapshots.
// ---------------------------------------------------------------------------

static void test_decklink_back_source_drains_ring_and_produces_frames() {
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkInput> input = selectFormatDetectionCapableInput(devices);
    CHECK(bool(input));
    if (!input) return;

    CaptureFrameRing ring;
    auto capture = CaptureSource::create(input, kDisplayMode, ring);
    CHECK(bool(capture));
    if (!capture) return;

    DeckLinkBackSource backSource(ring);
    backSource.start();

    // Bounded run -- the same order of magnitude every other real-hardware
    // DeckLink test in this repository already uses.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    backSource.stop();
    capture->stop();

    const auto& captureStats = capture->stats();
    const auto& backStats = backSource.stats();

    // Holds unconditionally, real signal or not -- run() only ever counts a
    // popped item as processed xor failed, never both, never neither.
    CHECK(std::size_t(backStats.framesProcessed.load()) + std::size_t(backStats.framesFailed.load()) ==
          std::size_t(backStats.framesPopped.load()));
    // The consumer cannot have popped more than the capture side pushed --
    // it may pop fewer, if this test's own bounded run stops before the
    // consumer thread drains everything still sitting in the ring.
    CHECK(std::size_t(backStats.framesPopped.load()) <= std::size_t(captureStats.framesPushed.load()));

    std::fprintf(stderr,
                 "test_decklink_back_source: framesArrived=%d framesPushed=%d | "
                 "framesPopped=%d framesProcessed=%d framesFailed=%d over a 5-second bounded run\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 backStats.framesPopped.load(), backStats.framesProcessed.load(), backStats.framesFailed.load());

    if (backStats.framesProcessed.load() == 0) {
        std::fprintf(stderr,
                      "test_decklink_back_source: NOTE -- zero frames processed. If nothing is patched "
                      "into this device's own SDI input, this is expected, not a defect -- the mechanics "
                      "above (clean start()/stop(), the accounting invariants just checked) are still the "
                      "evidence this test actually gates on. With a real signal connected, this should be "
                      "nonzero -- worth a second look otherwise.\n");
        return;
    }

    const auto raster = backSource.currentSourceRaster();
    CHECK(bool(raster));
    if (raster) {
        const auto view = raster->view();
        CHECK(view.width > 0 && view.height > 0);
    }
}

int main() {
    test_selector_default_matches_first_format_detection_capable_input();
    test_selector_index_zero_matches_first_match();
    test_selector_out_of_range_index_returns_null();
    test_decklink_back_source_drains_ring_and_produces_frames();

    return scatter::test::summary("test_decklink_back_source");
}
