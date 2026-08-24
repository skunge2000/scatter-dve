// WU-21b -- DeckLink capture-side pixel read smoke test: drains WU-20b's own
// CaptureSource/CaptureFrameRing on a CaptureConsumer thread, reading real
// pixel bytes out of a retained IDeckLinkVideoInputFrame and feeding them
// into WU-21a's own runFrameBytes() for the first time anywhere in this
// project.
//
// Runs only against real hardware, on the M1 Max with the UltraStudio
// Recorder 3G attached -- same reason as WU-20b's own test_decklink_input.cpp:
// no Blackmagic SDK and no AppleClang/Xcode toolchain exist in the Linux
// cloud sandbox this session drafted this in (see CMakeLists.txt's
// BLACKMAGIC_SDK_DIR guard, and DECISIONS.md ADR-046/047/048).
//
// Real-hardware setup this test's own Accept criteria assume: the same
// UltraStudio Monitor 3G SDI output patched directly into the Recorder 3G's
// SDI input that test_decklink_input.cpp's own header comment already
// documents (ADR-037) -- no third piece of equipment needed. Without that
// loopback connected, CaptureSource::create()/CaptureConsumer::start()/stop()
// still need to run cleanly (the mechanics this test's automated CHECKs gate
// on), but stats().framesProcessed may legitimately stay at zero -- this test
// warns rather than fails in that case, the same "nothing plugged in right
// now is a real, honestly reportable state, not a defect" convention
// test_decklink_input.cpp already uses.
//
// This unit's own scope stops at producing warped v210 bytes in memory --
// it does not reschedule them onto IDeckLinkOutput (WU-21c's own job, not
// yet built; DECISIONS.md ADR-048).
//
// WU-23b2b (DECISIONS.md ADR-080, extended by ADR-081) extends this test:
// CaptureConsumer now drives a de-interlace pass (video::Deinterlacer)
// ahead of the warp, so its own constructor takes an explicit
// DeinterlaceCoefficients (Steve's own choice, this session: Complex -- see
// ADR-081), and processOne()'s own new stream-start outcome widens the
// framesProcessed/framesFailed accounting invariant below to a third term,
// framesStreamStart.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_device.hpp"
#include "io/decklink_input.hpp"
#include "video/deinterlace.hpp"
#include "video/v210.hpp"
#include "harness.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>
#include <vector>

using namespace scatter::io;

namespace {

// Same display mode test_decklink_input.cpp already uses (ADR-033/ADR-007):
// this project's own confirmed-working 576i25/576p25 development standard.
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;
constexpr int kWidth = 720;
constexpr int kHeight = 576;

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

// Identity lattice, matching test_pipeline_bytes.cpp's own makeAffineLattice()
// duplicated locally at scale 1 / offset 0 (SESSION-PROTOCOL.md rule 2: one
// unit, one test -- no fixture shared across test translation units). An
// identity map is the right choice here: this test's own job is proving the
// StartAccess/GetBytes/runFrameBytes/EndAccess mechanics work against real
// captured bytes, not re-proving runFrameBytes()'s own warp correctness --
// already genuinely verified in the cloud sandbox at WU-21a.
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

static void test_capture_consumer_drains_ring_and_produces_frames() {
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

    scatter::PipelineParams params;
    params.destWidth = kWidth;
    params.destHeight = kHeight;

    // WU-23b2b (ADR-080/081): DeinterlaceCoefficients has no default --
    // Complex, Steve's own explicit choice this session (ADR-081); this
    // test's own job is the StartAccess/GetBytes/runFrameBytesDeinterlaced/
    // EndAccess mechanics and the accounting invariants below, not
    // re-proving Simple-versus-Complex reconstruction quality (already
    // covered, algorithmically, by WU-23b1's own test_deinterlace.cpp).
    CaptureConsumer consumer(ring, makeIdentityLattice(), params,
                              scatter::video::DeinterlaceCoefficients::Complex);
    consumer.start();

    // Bounded run -- the same order of magnitude test_decklink_input.cpp's
    // own bounded smoke test already uses (ADR-032/038/047).
    std::this_thread::sleep_for(std::chrono::seconds(5));

    consumer.stop();
    capture->stop();

    const auto& captureStats = capture->stats();
    const auto& consumerStats = consumer.stats();

    // Holds unconditionally, real signal or not -- run() only ever counts a
    // popped item as processed xor failed xor stream-start, never more than
    // one of the three, never none (WU-23b2b, ADR-080/081: processOne()'s
    // own new ProcessResult::StreamStart widens the pre-WU-23b2b
    // processed-xor-failed invariant with this third term).
    CHECK(std::size_t(consumerStats.framesProcessed.load()) + std::size_t(consumerStats.framesFailed.load()) +
              std::size_t(consumerStats.framesStreamStart.load()) ==
          std::size_t(consumerStats.framesPopped.load()));
    // The consumer cannot have popped more than the capture side pushed --
    // it may pop fewer, if this test's own bounded run stops before the
    // consumer thread drains everything still sitting in the ring.
    CHECK(std::size_t(consumerStats.framesPopped.load()) <= std::size_t(captureStats.framesPushed.load()));

    // WU-23b2b (ADR-080/081): framesStreamStart increments at most once per
    // CaptureConsumer instance, ever -- Deinterlacer::push()'s own state
    // machine (video/deinterlace.hpp) returns false only on the very first
    // call ever made against a given instance, never again afterward. And
    // whenever any frame has been fully processed, the stream-start frame
    // must already have happened and been counted: the first popped frame
    // that ever reaches m_deinterlacer.push() (i.e. survives
    // QueryInterface/StartAccess/GetBytes) is unconditionally that instance's
    // own stream-start frame, so framesProcessed > 0 is only reachable after
    // exactly one framesStreamStart increment.
    CHECK(consumerStats.framesStreamStart.load() <= 1);
    if (consumerStats.framesProcessed.load() > 0)
        CHECK(consumerStats.framesStreamStart.load() == 1);

    std::fprintf(stderr,
                 "test_decklink_capture_consumer: framesArrived=%d framesPushed=%d | "
                 "framesPopped=%d framesProcessed=%d framesFailed=%d framesStreamStart=%d over a "
                 "5-second bounded run\n",
                 captureStats.framesArrived.load(), captureStats.framesPushed.load(),
                 consumerStats.framesPopped.load(), consumerStats.framesProcessed.load(),
                 consumerStats.framesFailed.load(), consumerStats.framesStreamStart.load());

    if (consumerStats.framesProcessed.load() == 0) {
        std::fprintf(
            stderr,
            "test_decklink_capture_consumer: NOTE -- zero frames processed. If the Monitor 3G's SDI "
            "output is not patched into the Recorder 3G's SDI input (see test_decklink_input.cpp's "
            "own header comment), this is expected, not a defect -- the mechanics above (clean "
            "start()/stop(), the accounting invariants just checked) are still the evidence this test "
            "actually gates on. With the loopback connected, this should be nonzero -- worth a second "
            "look otherwise.\n");
        return;
    }

    std::vector<std::uint8_t> latest;
    CHECK(consumer.copyLatestFrame(latest));
    CHECK(latest.size() == scatter::v210::rowBytesMin(kWidth) * std::size_t(kHeight));
}

int main() {
    test_capture_consumer_drains_ring_and_produces_frames();
    return scatter::test::summary("test_decklink_capture_consumer");
}
