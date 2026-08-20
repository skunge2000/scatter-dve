// scatter-dve — WU-21c: continuous SDI re-output.
//
// See io/decklink_live_output.hpp and DECISIONS.md ADR-050 for the design.
// WU-21d/ADR-064 (startWith()'s pool-creation loop, below) fills the
// cold-start green finding ADR-050's own same-session addendum named: every
// pool buffer is now filled black immediately after CreateVideoFrame(),
// before any of them are scheduled.

#include "io/decklink_live_output.hpp"
#include "io/com_ptr.hpp"
#include "video/v210.hpp"

#include "DeckLinkAPI.h"

#include <cmath>
#include <cstring>
#include <utility>
#include <vector>

namespace scatter::io {

namespace {

// Same StartAccess/GetBytes/EndAccess bracket io/decklink_output.cpp's own
// fillFrameBuffer() (WU-15a) already uses. Duplicated here rather than
// shared across the two .cpp files — the same "a few lines of straight-line
// SDK calls, duplicating is simpler and safer than a cross-unit refactor"
// reasoning ADR-048 already used for not routing runFrameFile() through
// runFrameBytes() internally.
bool fillFrameBuffer(const ComPtr<IDeckLinkMutableVideoFrame>& frame, const std::vector<std::uint8_t>& bytes) {
    ComPtr<IDeckLinkVideoBuffer> buffer(IID_IDeckLinkVideoBuffer, frame);
    if (!buffer) return false;

    if (buffer->StartAccess(bmdBufferAccessWrite) != S_OK) return false;

    void* mem = nullptr;
    if (buffer->GetBytes(&mem) != S_OK || mem == nullptr) {
        buffer->EndAccess(bmdBufferAccessWrite);
        return false;
    }

    std::memcpy(mem, bytes.data(), bytes.size());
    return buffer->EndAccess(bmdBufferAccessWrite) == S_OK;
}

}  // namespace

LiveFramePlayback::LiveFramePlayback(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode, int width,
                                      int height, const CaptureConsumer& consumer)
    : m_output(std::move(output)), m_displayMode(displayMode), m_width(width), m_height(height),
      m_consumer(consumer) {}

ComPtr<LiveFramePlayback> LiveFramePlayback::create(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode,
                                                     int width, int height, const CaptureConsumer& consumer) {
    ComPtr<LiveFramePlayback> self;
    if (!output || width <= 0 || height <= 0) return self;

    // Adopts the one reference the constructor's own m_refCount{1} already
    // represents — the same case ComPtr::adopt() exists for (ADR-031),
    // applied here exactly as LoopedFramePlayback::create() already does
    // (ADR-032).
    self.adopt(new LiveFramePlayback(std::move(output), displayMode, width, height, consumer));

    if (!self->startWith()) {
        self = nullptr;  // drops this function's own reference; Release() deletes at zero
        return self;
    }
    return self;
}

bool LiveFramePlayback::startWith() {
    bool supported = false;
    if (m_output->DoesSupportVideoMode(bmdVideoConnectionUnspecified, m_displayMode, bmdFormat10BitYUV,
                                        bmdNoVideoOutputConversion, bmdSupportedVideoModeDefault, nullptr,
                                        &supported) != S_OK ||
        !supported)
        return false;

    if (m_output->EnableVideoOutput(m_displayMode, bmdVideoOutputFlagDefault) != S_OK) return false;

    ComPtr<IDeckLinkDisplayMode> mode;
    if (m_output->GetDisplayMode(m_displayMode, mode.releaseAndGetAddressOf()) != S_OK ||
        mode->GetFrameRate(&m_frameDuration, &m_timeScale) != S_OK || m_frameDuration <= 0) {
        m_output->DisableVideoOutput();
        return false;
    }

    // ADR-032's own consistency expectation (bmdFormat10BitYUV literally is
    // v210) enforced here directly, not left to a caller's own separate test
    // the way WU-15a's test_v210_rowbytes_matches_project_own_computation()
    // checks it — every pool buffer below is refilled from
    // CaptureConsumer's own video::v210::rowBytesMin()-sized bytes
    // (io/decklink_capture_consumer.cpp's own m_dstRowBytes), so a mismatch
    // here would misalign every live frame, not just fail one check.
    std::int32_t rowBytes = 0;
    if (m_output->RowBytesForPixelFormat(bmdFormat10BitYUV, m_width, &rowBytes) != S_OK || rowBytes <= 0 ||
        std::size_t(rowBytes) != scatter::v210::rowBytesMin(m_width)) {
        m_output->DisableVideoOutput();
        return false;
    }

    // Pool size: round(frameRate / 2), ADR-032's own half-second preroll
    // convention (computed from the negotiated display mode's own frame
    // rate, not hardcoded), reused here to size the pool rather than the
    // preroll depth of a single mechanism — see DECISIONS.md ADR-050 for why
    // this is not a coincidence: the pool must hold exactly as many buffers
    // as are concurrently in flight, which is exactly the preroll depth.
    const double framesPerSecond = double(m_timeScale) / double(m_frameDuration);
    const auto poolSize = std::size_t(std::lround(framesPerSecond / 2.0));
    if (poolSize == 0) {
        m_output->DisableVideoOutput();
        return false;
    }

    // WU-21d, DECISIONS.md ADR-064: every pool buffer is filled black
    // immediately after CreateVideoFrame(), before the preroll loop below
    // schedules any of them. Left unfilled, a buffer carries whatever
    // CreateVideoFrame() first allocated it with -- effectively zero-filled
    // v210, which decodes as solid green on a real monitor for however long
    // it takes CaptureConsumer to produce its own first output (this file's
    // own "nothing fresher -- leave existing content unchanged" refill
    // policy, above, never overwrites it before then). Built once, not once
    // per buffer: every pool buffer's own cold-start content is identical,
    // and scatter::v210::packBlackFrame() is not cheap enough to redo
    // poolSize times for the same result.
    std::vector<std::uint8_t> blackFrame(scatter::v210::rowBytesMin(m_width) * std::size_t(m_height));
    scatter::v210::packBlackFrame(m_width, m_height, blackFrame.data());

    m_pool.resize(poolSize);
    for (auto& frame : m_pool) {
        if (m_output->CreateVideoFrame(m_width, m_height, rowBytes, bmdFormat10BitYUV, bmdFrameFlagDefault,
                                        frame.releaseAndGetAddressOf()) != S_OK) {
            m_output->DisableVideoOutput();
            return false;
        }
        if (!fillFrameBuffer(frame, blackFrame)) {
            m_output->DisableVideoOutput();
            return false;
        }
    }

    if (m_output->SetScheduledFrameCompletionCallback(this) != S_OK) {
        m_output->DisableVideoOutput();
        return false;
    }

    m_nextScheduleTime = 0;
    m_nextPoolIndex = 0;
    for (std::size_t i = 0; i < m_pool.size(); ++i) {
        if (!refillAndSchedule(i)) {
            m_output->SetScheduledFrameCompletionCallback(nullptr);
            m_output->DisableVideoOutput();
            return false;
        }
    }

    if (m_output->StartScheduledPlayback(0, m_timeScale, 1.0) != S_OK) {
        m_output->SetScheduledFrameCompletionCallback(nullptr);
        m_output->DisableVideoOutput();
        return false;
    }

    return true;
}

bool LiveFramePlayback::refillAndSchedule(std::size_t poolIndex) {
    std::vector<std::uint8_t> bytes;
    bool refreshed = false;
    if (m_consumer.copyLatestFrame(bytes) &&
        bytes.size() == scatter::v210::rowBytesMin(m_width) * std::size_t(m_height)) {
        // A size mismatch (a caller-supplied width/height that does not
        // actually match consumer's own PipelineParams::destWidth/
        // destHeight) is treated the same as "nothing new" below — leave
        // this buffer's existing content untouched rather than writing a
        // partial/misaligned frame, the same "do not fabricate, skip
        // instead" convention this project uses throughout (ADR-024's
        // off-raster drop).
        refreshed = fillFrameBuffer(m_pool[poolIndex], bytes);
    }
    if (!refreshed) ++m_framesRepeated;
    // If nothing fresher was available, this buffer's own existing content
    // (whatever CreateVideoFrame() allocated it with, or its own previous
    // refill) is scheduled unchanged below — the natural, un-glitchy
    // consequence of "always output the latest available produced frame"
    // when nothing new has arrived since the last check. See this file's
    // own header comment for why this is the chosen answer to ADR-037's own
    // genlock/clock-domain-drift follow-up, for this unit specifically.

    if (m_output->ScheduleVideoFrame(m_pool[poolIndex].get(), m_nextScheduleTime, m_frameDuration, m_timeScale) !=
        S_OK)
        return false;
    m_nextScheduleTime += m_frameDuration;
    return true;
}

void LiveFramePlayback::stop() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true))
        return;  // already stopped, or a concurrent stop() got there first

    m_output->StopScheduledPlayback(0, nullptr, m_timeScale);
    m_output->SetScheduledFrameCompletionCallback(nullptr);
    m_output->DisableVideoOutput();
}

HRESULT LiveFramePlayback::ScheduledFrameCompleted(IDeckLinkVideoFrame* /*completedFrame*/,
                                                    BMDOutputFrameCompletionResult result) {
    switch (result) {
        case bmdOutputFrameCompleted:     ++m_stats.completed;      break;
        case bmdOutputFrameDisplayedLate: ++m_stats.displayedLate;  break;
        case bmdOutputFrameDropped:       ++m_stats.dropped;        break;
        case bmdOutputFrameFlushed:       ++m_stats.flushed;        break;
        default: break;
    }

    if (m_stopping.load()) return S_OK;

    // Completions arrive in exactly the order frames were scheduled (the
    // SDK's own FIFO guarantee — LoopedFramePlayback's own single-buffer
    // refill already relies on the same property), so cycling
    // m_nextPoolIndex in lockstep with each completion always refills and
    // reschedules the one pool buffer that just became safe to overwrite —
    // no longer queued in the driver, since its own completion is the event
    // that just fired — never one still in flight.
    const std::size_t poolIndex = m_nextPoolIndex;
    m_nextPoolIndex = (m_nextPoolIndex + 1) % m_pool.size();

    if (!refillAndSchedule(poolIndex)) m_stopping = true;

    return S_OK;
}

HRESULT LiveFramePlayback::ScheduledPlaybackHasStopped() { return S_OK; }

HRESULT LiveFramePlayback::QueryInterface(REFIID iid, LPVOID* ppv) {
    if (ppv == nullptr) return E_POINTER;

    const CFUUIDBytes iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
    if (std::memcmp(&iid, &iunknown, sizeof(REFIID)) == 0) {
        *ppv = static_cast<IDeckLinkVideoOutputCallback*>(this);
        AddRef();
        return S_OK;
    }
    if (std::memcmp(&iid, &IID_IDeckLinkVideoOutputCallback, sizeof(REFIID)) == 0) {
        *ppv = static_cast<IDeckLinkVideoOutputCallback*>(this);
        AddRef();
        return S_OK;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG LiveFramePlayback::AddRef() { return ++m_refCount; }

ULONG LiveFramePlayback::Release() {
    const ULONG newCount = --m_refCount;
    if (newCount == 0) delete this;
    return newCount;
}

}  // namespace scatter::io
