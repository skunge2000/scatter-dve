// WU-21c -- continuous SDI re-output smoke test: closes architecture.md 10's
// own Phase 5 "done when" line ("live SDI in, warped, SDI out") end to end
// for the first time -- CaptureSource (WU-20b) captures on the UltraStudio
// Recorder 3G, CaptureConsumer (WU-21b) warps each arriving frame via
// runFrameBytes() (WU-21a), and this unit's own LiveFramePlayback
// continuously reschedules the most recently produced bytes onto the
// UltraStudio Monitor 3G's own SDI output.
//
// Runs only against real hardware -- same reason as every DeckLink-touching
// test before it: no Blackmagic SDK and no AppleClang/Xcode toolchain exist
// in the Linux cloud sandbox this session drafted this in (see
// CMakeLists.txt's BLACKMAGIC_SDK_DIR guard, and DECISIONS.md ADR-050).
//
// Real-hardware setup: the same Monitor 3G SDI-out -> Recorder 3G SDI-in
// loopback cable test_decklink_input.cpp's and
// test_decklink_capture_consumer.cpp's own header comments already document
// (ADR-037) -- no new physical setup for this unit. With this test running,
// that cable now carries a genuine closed loop: whatever signal is patched
// into the Recorder 3G is captured, warped (identity map here -- see below),
// and re-output via the Monitor 3G, which feeds back into the Recorder 3G
// again. An identity map through a live feedback loop is expected to look
// like whatever the original source was, not build up visible drift each
// pass -- worth a look by eye, not asserted by this test's own automated
// CHECKs (see this file's own "Accept" note below and HANDOFF.md).
//
// This test's own job is the mechanics -- clean create/stop, no dropped or
// late frames scheduling a continuously-refilled pool, the accounting
// invariants both the capture and consumer sides already establish (WU-20b/
// WU-21b) -- not the literal one-hour endurance criterion architecture.md
// 10's own Phase 5 "done when" line names, nor a by-eye confirmation of
// genuinely live (not frozen) content on the wire. Both are Steve's own
// hands-on job at the real terminal, the same division of labour WU-15a/
// WU-15b and WU-19a/WU-19b already established.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "io/decklink_live_output.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

using namespace scatter::io;

namespace {

// Same display mode and geometry every DeckLink-touching test since WU-15a
// uses (ADR-007/ADR-033): this project's own confirmed-working 576i25/576p25
// development standard.
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;
constexpr int kWidth = 720;
constexpr int kHeight = 576;

// Duplicated locally from test_decklink_capture_consumer.cpp's own helper of
// the same name -- SESSION-PROTOCOL.md rule 2: one unit, one test, no shared
// fixture across test translation units.
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

// Duplicated locally from test_decklink_output.cpp's own helper of the same
// name -- same rule as above.
ComPtr<IDeckLinkOutput> firstPlaybackCapableOutput(const std::vector<DeviceInfo>& devices) {
    for (const auto& d : devices) {
        if (!d.supportsPlayback) continue;
        ComPtr<IDeckLinkOutput> candidate(IID_IDeckLinkOutput, d.device);
        if (candidate) return candidate;
    }
    return {};
}

// Identity lattice, matching test_decklink_capture_consumer.cpp's own
// makeIdentityLattice() -- duplicated locally per SESSION-PROTOCOL.md rule
// 2. An identity map is the right choice here too: this test's own job is
// proving the pool-refill/reschedule mechanics work end to end, not
// re-proving runFrameBytes()'s own warp correctness (WU-21a) or the
// StartAccess/GetBytes/EndAccess read-side mechanics (WU-21b) -- both
// already genuinely verified.
scatter::Lattice makeIdentityLattice() {
    scatter::Lattice lat;
    for (int row = 0; row < scatter::kLatticeSize; ++row) {
        for (int col = 0; col < scatter::kLatticeSize; ++col) {
            scatter::Vec3& p = lat.at(row, col);
            p.x = double(col) * double(kWidth - 1) / double(scatter::kLatticeMax);
            p.y = double(row) * double(kHeight - 1) / double(scatter::kLatticeMax);
            p.z = 0.0;
        }
    }
    return lat;
}

}  // namespace

static void test_live_playback_reschedules_continuously_with_no_dropped_or_late_frames() {
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

    CaptureConsumer consumer(ring, makeIdentityLattice(), params);
    consumer.start();

    auto playback = LiveFramePlayback::create(output, kDisplayMode, kWidth, kHeight, consumer);
    CHECK(bool(playback));
    if (playback) {
        // Bounded run -- same order of magnitude every DeckLink-touching
        // test's own smoke run already uses (ADR-032/038/047/049).
        std::this_thread::sleep_for(std::chrono::seconds(5));

        playback->stop();

        CHECK(playback->stats().completed.load() > 0);
        CHECK(playback->stats().displayedLate.load() == 0);
        CHECK(playback->stats().dropped.load() == 0);

        std::fprintf(stderr,
                     "test_decklink_live_output: completed=%d displayedLate=%d dropped=%d "
                     "flushed=%d framesRepeated=%d over a 5-second bounded run\n",
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
                 "test_decklink_live_output: framesArrived=%d framesPushed=%d | framesPopped=%d "
                 "framesProcessed=%d framesFailed=%d\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 consumerStats.framesPopped.load(), consumerStats.framesProcessed.load(),
                 consumerStats.framesFailed.load());

    if (consumerStats.framesProcessed.load() == 0) {
        std::fprintf(stderr,
                     "test_decklink_live_output: NOTE -- zero frames processed. If the Monitor 3G's SDI "
                     "output is not patched into the Recorder 3G's SDI input, this is expected, not a "
                     "defect -- the playback mechanics above (clean create/stop, zero dropped/late) are "
                     "still real evidence even with nothing captured, since every pool buffer is scheduled "
                     "regardless of whether copyLatestFrame() ever succeeds. With the loopback connected, "
                     "framesProcessed should be nonzero -- worth a second look otherwise.\n");
    }
}

int main() {
    test_live_playback_reschedules_continuously_with_no_dropped_or_late_frames();
    return scatter::test::summary("test_decklink_live_output");
}
