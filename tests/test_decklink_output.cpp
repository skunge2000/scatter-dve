// WU-15a -- scheduled playback smoke test: one already-warped frame,
// produced by this project's own pipeline and written to a real .v210 file,
// then played out via LoopedFramePlayback on the real UltraStudio 4K Mini
// for a bounded run. Confirms zero dropped/late frames over that run and a
// clean start/stop -- NOT the one-hour endurance criterion WORK-UNITS.md's
// own WU-15 line originally stated as a single accept criterion; see
// DECISIONS.md ADR-032 for why that splits into WU-15a (this unit) and
// WU-15b (an unattended one-hour hardware run, not implementation work, not
// this session's own job).
//
// Runs only against real hardware, on the M1 Max with the UltraStudio 4K
// Mini attached -- same reason as WU-14's own test_decklink_device.cpp: no
// Blackmagic SDK and no AppleClang/Xcode toolchain exist in the Linux cloud
// sandbox this session drafted this in (see CMakeLists.txt's
// BLACKMAGIC_SDK_DIR guard, and DECISIONS.md ADR-031/ADR-032).
//
// While test_looped_playback_runs_with_no_dropped_or_late_frames() runs,
// the UltraStudio 4K Mini's SDI output is live for about five seconds --
// point a broadcast monitor (or Media Express's own input preview) at it to
// confirm by eye that a warped frame actually appears. The automated checks
// below confirm the DeckLink-side mechanics (no dropped/late frames, a
// clean stop), not what is actually on the wire; that confirmation is
// HANDOFF.md's own job to ask for by hand.

#include "io/decklink_device.hpp"
#include "io/decklink_output.hpp"
#include "io/com_ptr.hpp"
#include "harness.hpp"

#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "testpat.hpp"
#include "video/v210.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

using namespace scatter::io;

namespace {

constexpr int kWidth  = 720;
constexpr int kHeight = 576;

// ADR-007's own "576i25 / 576p25" development standard. ADR-032's own first
// choice was bmdModePALp (progressive, matching this project's own lack of
// de-interlace/field-split machinery -- WU-23, not built) with bmdModePAL
// named as the documented fallback if the real hardware did not support it.
// Confirmed at the real terminal, this session: DoesSupportVideoMode()
// returns S_OK with supported == false for bmdModePALp + bmdFormat10BitYUV
// on this UltraStudio 4K Mini -- not a code defect, exactly the scenario
// ADR-032 anticipated and already reasoned about (a static, motion-free
// frame transmitted as interlaced is visually indistinguishable from
// progressive, since both fields of every frame come from the same
// unchanging buffer). See DECISIONS.md ADR-033, which freezes this as the
// confirmed working choice rather than leaving it as ADR-032's own
// unresolved fallback note.
constexpr BMDDisplayMode kDisplayMode = bmdModePAL;

ComPtr<IDeckLinkOutput> firstPlaybackCapableOutput(const std::vector<DeviceInfo>& devices) {
    for (const auto& d : devices) {
        if (!d.supportsPlayback) continue;
        ComPtr<IDeckLinkOutput> candidate(IID_IDeckLinkOutput, d.device);
        if (candidate) return candidate;
    }
    return {};
}

// Builds one genuinely warped frame -- a cylinder over a zone-plate source,
// reusing WU-11's own shape (core/shapes/shapes.hpp) and WU-10's own
// pipeline entry point (runFrameFile(), core/resolve.hpp), exactly as any
// other unit's own tests already do -- and writes it to a real .v210 file,
// so this test's own "file source" genuinely originated from a file this
// project's own pipeline produced, not an in-memory buffer standing in for
// one. Returns "" on any failure along the way.
std::string writeWarpedTestFrame(const std::string& outPath) {
    const std::string srcPath = outPath + ".src.v210";

    const auto src = scatter::testpat::makeZonePlate(kWidth, kHeight);
    if (!scatter::testpat::writeV210(src, srcPath)) return {};

    scatter::shapes::CylinderParams cylParams;
    cylParams.radius     = 500.0;
    cylParams.angleSpan  = 1.0471975511965976;  // pi/3 -- visibly curved, not a subtle warp
    cylParams.heightSpan = double(kHeight);
    cylParams.centerX    = double(kWidth) / 2.0;
    cylParams.centerY    = double(kHeight) / 2.0;
    const auto lattice = scatter::shapes::buildCylinderLattice(cylParams);

    scatter::PipelineParams params;
    params.destWidth  = kWidth;
    params.destHeight = kHeight;

    if (!scatter::runFrameFile(lattice, srcPath, kWidth, kHeight, params, outPath)) return {};
    return outPath;
}

}  // namespace

static void test_v210_rowbytes_matches_project_own_computation() {
    // ADR-032's own consistency check: io/decklink_output.cpp trusts
    // IDeckLinkOutput::RowBytesForPixelFormat(bmdFormat10BitYUV, ...) rather
    // than this project's own video::v210::rowBytesMin(width) -- matching
    // architecture.md 7's existing "never compute row stride yourself" rule
    // for the input side, applied here to output. bmdFormat10BitYUV is
    // literally FourCC 'v210' (DeckLinkAPIModes.h), so the two are expected
    // to agree; checked here directly, once, rather than assumed silently by
    // the playback test below, which relies on it.
    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkOutput> output = firstPlaybackCapableOutput(devices);
    CHECK(bool(output));
    if (!output) return;

    std::int32_t sdkRowBytes = 0;
    CHECK(output->RowBytesForPixelFormat(bmdFormat10BitYUV, kWidth, &sdkRowBytes) == S_OK);
    CHECK(std::size_t(sdkRowBytes) == scatter::v210::rowBytesMin(kWidth));
}

static void test_looped_playback_runs_with_no_dropped_or_late_frames() {
    const std::string framePath = writeWarpedTestFrame("/tmp/scatter_wu15a_frame.v210");
    CHECK(!framePath.empty());
    if (framePath.empty()) return;

    const auto devices = enumerateDeckLinkDevices();
    CHECK(!devices.empty());
    if (devices.empty()) return;

    ComPtr<IDeckLinkOutput> output = firstPlaybackCapableOutput(devices);
    CHECK(bool(output));
    if (!output) return;

    auto playback = LoopedFramePlayback::create(output, kDisplayMode, framePath, kWidth, kHeight);
    CHECK(bool(playback));
    if (!playback) return;

    // Bounded run -- a few seconds, not WU-15b's own one hour (ADR-032).
    // Long enough that a real dropped-frame problem at this bit rate would
    // show up at least once; short enough for one session's own terminal
    // loop.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    playback->stop();

    CHECK(playback->stats().completed.load() > 0);
    CHECK(playback->stats().displayedLate.load() == 0);
    CHECK(playback->stats().dropped.load() == 0);

    std::fprintf(stderr,
                 "test_decklink_output: completed=%d displayedLate=%d dropped=%d flushed=%d over "
                 "a 5-second bounded run -- see DECISIONS.md ADR-032 for why this is not WU-15b's "
                 "own one-hour endurance criterion\n",
                 playback->stats().completed.load(), playback->stats().displayedLate.load(),
                 playback->stats().dropped.load(), playback->stats().flushed.load());
}

int main() {
    test_v210_rowbytes_matches_project_own_computation();
    test_looped_playback_runs_with_no_dropped_or_late_frames();
    return scatter::test::summary("test_decklink_output");
}
