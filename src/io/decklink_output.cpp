// scatter-dve — WU-15a: scheduled playback, one looped file-sourced frame to
// SDI out.
//
// See io/decklink_output.hpp and DECISIONS.md ADR-032 for the design.

#include "io/decklink_output.hpp"
#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <cmath>
#include <cstring>
#include <fstream>
#include <ios>
#include <utility>
#include <vector>

namespace scatter::io {

namespace {

// Reads exactly exactBytes from path into out. Returns false, leaving out's
// contents unspecified, if the file cannot be opened or is shorter than
// exactBytes — same convention src/io/file_source.cpp's readV210File()
// already uses (WU-05), applied here to a raw (unpacked-by-this-project)
// byte read rather than a v210::unpackImage call. A file *longer* than
// exactBytes is accepted (only the first exactBytes are read) rather than
// rejected — this unit only ever plays back one frame regardless of how the
// file was produced, and refusing a longer file would gain nothing.
bool readRawFile(const std::string& path, std::size_t exactBytes, std::vector<std::uint8_t>& out) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    out.resize(exactBytes);
    in.read(reinterpret_cast<char*>(out.data()), std::streamsize(exactBytes));
    return std::size_t(in.gcount()) == exactBytes;
}

// Copies bytes into frame's own DMA-friendly buffer. Per the file header:
// GetBytes() is not a method on IDeckLinkVideoFrame/IDeckLinkMutableVideoFrame
// themselves — architecture.md 7's own "CreateVideoFrame() and write into
// GetBytes()" reads as if it were, but the real SDK exposes it only on a
// separate IDeckLinkVideoBuffer, obtained via QueryInterface from the frame
// and bracketed by StartAccess()/EndAccess() — confirmed directly against
// the real SDK's own SignalGenerator sample (SyncController.mm's
// ScopedBufferBytes helper), not assumed from architecture.md's summary.
// See ADR-032.
bool fillFrameBuffer(const ComPtr<IDeckLinkMutableVideoFrame>& frame,
                      const std::vector<std::uint8_t>& bytes) {
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

LoopedFramePlayback::LoopedFramePlayback(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode,
                                          int width, int height)
    : m_output(std::move(output)), m_displayMode(displayMode), m_width(width), m_height(height) {}

ComPtr<LoopedFramePlayback> LoopedFramePlayback::create(ComPtr<IDeckLinkOutput> output,
                                                          BMDDisplayMode displayMode,
                                                          const std::string& v210FilePath,
                                                          int width, int height) {
    ComPtr<LoopedFramePlayback> self;
    if (!output || width <= 0 || height <= 0) return self;

    // Adopts the one reference the constructor's own m_refCount{1} already
    // represents -- the same "already one reference, take ownership, no
    // AddRef" case ComPtr::adopt() exists for (ADR-031), applied here to
    // this project's own object rather than an SDK factory return.
    self.adopt(new LoopedFramePlayback(std::move(output), displayMode, width, height));

    if (!self->startWith(v210FilePath)) {
        self = nullptr;  // drops this function's own reference; Release() deletes at zero
        return self;
    }
    return self;
}

bool LoopedFramePlayback::startWith(const std::string& v210FilePath) {
    bool supported = false;
    if (m_output->DoesSupportVideoMode(bmdVideoConnectionUnspecified, m_displayMode, bmdFormat10BitYUV,
                                        bmdNoVideoOutputConversion, bmdSupportedVideoModeDefault,
                                        nullptr, &supported) != S_OK ||
        !supported)
        return false;

    if (m_output->EnableVideoOutput(m_displayMode, bmdVideoOutputFlagDefault) != S_OK) return false;

    ComPtr<IDeckLinkDisplayMode> mode;
    if (m_output->GetDisplayMode(m_displayMode, mode.releaseAndGetAddressOf()) != S_OK ||
        mode->GetFrameRate(&m_frameDuration, &m_timeScale) != S_OK || m_frameDuration <= 0) {
        m_output->DisableVideoOutput();
        return false;
    }

    // Per the SDK's own rule for the input side (architecture.md 7: "Always
    // use GetBytesPerRow(); never compute row stride yourself") applied
    // symmetrically to output: ask the SDK for bmdFormat10BitYUV's own row
    // bytes at this width rather than assuming this project's own
    // v210::rowBytesMin(width) agrees with it. ADR-032 records why the two
    // are expected to agree (bmdFormat10BitYUV literally is v210) and where
    // that expectation is actually checked (the caller's own test, not
    // here) -- this function only ever uses the SDK's own answer.
    std::int32_t rowBytes = 0;
    if (m_output->RowBytesForPixelFormat(bmdFormat10BitYUV, m_width, &rowBytes) != S_OK || rowBytes <= 0) {
        m_output->DisableVideoOutput();
        return false;
    }

    std::vector<std::uint8_t> fileBytes;
    if (!readRawFile(v210FilePath, std::size_t(rowBytes) * std::size_t(m_height), fileBytes)) {
        m_output->DisableVideoOutput();
        return false;
    }

    ComPtr<IDeckLinkMutableVideoFrame> frame;
    if (m_output->CreateVideoFrame(m_width, m_height, rowBytes, bmdFormat10BitYUV, bmdFrameFlagDefault,
                                    frame.releaseAndGetAddressOf()) != S_OK) {
        m_output->DisableVideoOutput();
        return false;
    }

    if (!fillFrameBuffer(frame, fileBytes)) {
        m_output->DisableVideoOutput();
        return false;
    }
    m_frame = frame;

    if (m_output->SetScheduledFrameCompletionCallback(this) != S_OK) {
        m_output->DisableVideoOutput();
        return false;
    }

    // ADR-032: half a second's worth of frames, matching the real SDK's own
    // FilePlayback ("Preroll 1/2 second of frames", DeckLinkPlaybackDevice
    // ::play()) and SignalGenerator ("Set the preroll to 1/2 second of
    // video frames", SyncController.mm's startRunning()) samples -- neither
    // uses architecture.md 7's own illustrative "3-frame preroll" figure,
    // and this unit follows the real samples' own idiom, not that
    // approximation.
    const double framesPerSecond = double(m_timeScale) / double(m_frameDuration);
    const auto framesToPreroll = static_cast<int>(std::lround(framesPerSecond / 2.0));

    m_nextScheduleTime = 0;
    for (int i = 0; i < framesToPreroll; ++i) {
        if (!scheduleOne()) {
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

bool LoopedFramePlayback::scheduleOne() {
    if (m_output->ScheduleVideoFrame(m_frame.get(), m_nextScheduleTime, m_frameDuration, m_timeScale) != S_OK)
        return false;
    m_nextScheduleTime += m_frameDuration;
    return true;
}

void LoopedFramePlayback::stop() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true))
        return;  // already stopped, or a concurrent stop() got there first

    // StopScheduledPlayback, then DisableVideoOutput -- DisableVideoOutput()
    // itself blocks until every scheduled frame is completed or flushed
    // (SignalGenerator's own stopRunning(), confirmed against the real SDK
    // sample rather than assumed), so no extra wait is needed here beyond
    // that call returning -- see ADR-032 for why this unit does not need
    // FilePlayback's own mutex/condition-variable stop machinery.
    m_output->StopScheduledPlayback(0, nullptr, m_timeScale);
    m_output->SetScheduledFrameCompletionCallback(nullptr);
    m_output->DisableVideoOutput();
}

HRESULT LoopedFramePlayback::ScheduledFrameCompleted(IDeckLinkVideoFrame* /*completedFrame*/,
                                                      BMDOutputFrameCompletionResult result) {
    switch (result) {
        case bmdOutputFrameCompleted:     ++m_stats.completed;      break;
        case bmdOutputFrameDisplayedLate: ++m_stats.displayedLate;  break;
        case bmdOutputFrameDropped:       ++m_stats.dropped;        break;
        case bmdOutputFrameFlushed:       ++m_stats.flushed;        break;
        default: break;
    }

    if (m_stopping.load()) return S_OK;

    // Single-frame refill: exactly one replacement frame scheduled per
    // completion, the same idiom the real SDK's own FilePlayback sample
    // uses (ScheduledFrameCompleted() -> scheduleVideo(), called once) --
    // not a batch refill. Simplest mechanism that keeps the DeckLink-
    // internal queue at a roughly constant depth with no extra bookkeeping;
    // see ADR-032.
    if (!scheduleOne()) m_stopping = true;

    return S_OK;
}

HRESULT LoopedFramePlayback::ScheduledPlaybackHasStopped() { return S_OK; }

HRESULT LoopedFramePlayback::QueryInterface(REFIID iid, LPVOID* ppv) {
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

ULONG LoopedFramePlayback::AddRef() { return ++m_refCount; }

ULONG LoopedFramePlayback::Release() {
    const ULONG newCount = --m_refCount;
    if (newCount == 0) delete this;
    return newCount;
}

}  // namespace scatter::io
