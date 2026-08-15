// scatter-dve — WU-20b: DeckLink capture: format-detection-aware
// EnableVideoInput, IDeckLinkInputCallback implementation.
//
// See io/decklink_input.hpp and DECISIONS.md ADR-046/ADR-047 for the design.

#include "io/decklink_input.hpp"
#include "core/ring_buffer.hpp"
#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <cstring>
#include <utility>

namespace scatter::io {

CaptureSource::CaptureSource(ComPtr<IDeckLinkInput> input, CaptureFrameRing& ring)
    : m_input(std::move(input)), m_ring(ring) {}

ComPtr<CaptureSource> CaptureSource::create(ComPtr<IDeckLinkInput> input, BMDDisplayMode initialDisplayMode,
                                             CaptureFrameRing& ring) {
    ComPtr<CaptureSource> self;
    if (!input) return self;

    // Adopts the one reference the constructor's own m_refCount{1} already
    // represents -- the same "already one reference, take ownership, no
    // AddRef" case ComPtr::adopt() exists for (ADR-031), applied here to
    // this project's own object rather than an SDK factory return, exactly
    // as LoopedFramePlayback::create() already does on the output side
    // (ADR-032).
    self.adopt(new CaptureSource(std::move(input), ring));

    if (!self->startWith(initialDisplayMode)) {
        self = nullptr;  // drops this function's own reference; Release() deletes at zero
        return self;
    }
    return self;
}

bool CaptureSource::startWith(BMDDisplayMode initialDisplayMode) {
    // WU-20's own "format-detection-aware" premise (WORK-UNITS.md) makes
    // this a hard requirement of this unit, not a soft fallback -- a device
    // that cannot report detected formats is out of scope for this unit's
    // own job, and this test's own precondition check (tests/
    // test_decklink_input.cpp) confirms the real UltraStudio Recorder 3G
    // reports it before relying on create() succeeding at all.
    ComPtr<IDeckLinkProfileAttributes> attributes(IID_IDeckLinkProfileAttributes, m_input);
    bool supportsFormatDetection = false;
    if (!attributes || attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportsFormatDetection) != S_OK ||
        !supportsFormatDetection)
        return false;

    // Same precautionary check LoopedFramePlayback::startWith() already
    // makes on the output side (ADR-032) before enabling anything.
    bool supported = false;
    if (m_input->DoesSupportVideoMode(bmdVideoConnectionUnspecified, initialDisplayMode, bmdFormat10BitYUV,
                                       bmdNoVideoInputConversion, bmdSupportedVideoModeDefault, nullptr,
                                       &supported) != S_OK ||
        !supported)
        return false;

    // CaptureStills'/CapturePreview's own call order: SetCallback before
    // EnableVideoInput, then StartStreams.
    if (m_input->SetCallback(this) != S_OK) return false;

    if (m_input->EnableVideoInput(initialDisplayMode, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection) !=
        S_OK) {
        m_input->SetCallback(nullptr);
        return false;
    }

    if (m_input->StartStreams() != S_OK) {
        m_input->SetCallback(nullptr);
        m_input->DisableVideoInput();
        return false;
    }

    return true;
}

void CaptureSource::stop() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true))
        return;  // already stopped, or a concurrent stop()/callback-driven stop got there first

    m_input->StopStreams();
    m_input->SetCallback(nullptr);
    m_input->DisableVideoInput();
}

void CaptureSource::stopFromCallback() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true)) return;

    // Best-effort only -- see the header's own note on stopFromCallback():
    // called from inside a callback the SDK itself is invoking, which this
    // sandbox has no way to build or run to confirm is actually safe.
    // Reasoned through, not verified; ADR-047.
    m_input->StopStreams();
    m_input->SetCallback(nullptr);
    m_input->DisableVideoInput();
}

HRESULT CaptureSource::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                                IDeckLinkDisplayMode* newDisplayMode,
                                                BMDDetectedVideoInputFormatFlags /*detectedSignalFlags*/) {
    ++m_stats.formatChanges;

    if (m_stopping.load()) return S_OK;

    // Restart at the newly detected mode -- CaptureStills'/CapturePreview's
    // own pattern (ADR-046), not InputLoopThrough's non-restarting one.
    // Pixel format stays fixed at bmdFormat10BitYUV regardless of
    // detectedSignalFlags -- see io/decklink_input.hpp's own class comment
    // and ADR-047 for why.
    if ((notificationEvents & (bmdVideoInputDisplayModeChanged | bmdVideoInputColorspaceChanged)) == 0) return S_OK;

    if (m_input->StopStreams() != S_OK) {
        stopFromCallback();
        return S_OK;
    }

    const BMDDisplayMode detectedMode = newDisplayMode->GetDisplayMode();
    if (m_input->EnableVideoInput(detectedMode, bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection) != S_OK) {
        stopFromCallback();
        return S_OK;
    }

    if (m_input->StartStreams() != S_OK) {
        stopFromCallback();
        return S_OK;
    }

    return S_OK;
}

HRESULT CaptureSource::VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame,
                                               IDeckLinkAudioInputPacket* /*audioPacket*/) {
    if (m_stopping.load()) return S_OK;
    if (videoFrame == nullptr) return S_OK;

    ++m_stats.framesArrived;

    const bool inputFrameValid =
        (videoFrame->GetFlags() & static_cast<BMDFrameFlags>(bmdFrameHasNoInputSource)) == 0;

    if (!inputFrameValid) {
        ++m_stats.noInputSourceFrames;
        m_prevInputSourceValid.store(false);
        return S_OK;
    }

    // Signal-recovery restart: CaptureStills' own pattern (ADR-046) -- on
    // the first valid frame after an invalid one, restart the stream before
    // accepting frames as good, and do not push this particular (possibly
    // still-transient) frame. Genuinely different from the format-change
    // restart above: this is signal *presence* recovering, not signal
    // *format* changing, and can happen with no VideoInputFormatChanged()
    // call at all (a cable unplugged and replugged at the same format).
    if (!m_prevInputSourceValid.exchange(true)) {
        if (m_input->StopStreams() != S_OK || m_input->FlushStreams() != S_OK || m_input->StartStreams() != S_OK)
            stopFromCallback();
        return S_OK;
    }

    // Retain: this frame pointer is only valid for the duration of this call
    // (architecture.md 7) unless AddRef'd. ComPtr's existing borrowing
    // constructor (ADR-031) does exactly that -- the first time this
    // project has exercised it against a genuinely borrowed callback
    // parameter rather than a factory/enumerator result (ADR-046).
    ComPtr<IDeckLinkVideoInputFrame> retained(videoFrame);
    if (m_ring.tryPush(std::move(retained)))
        ++m_stats.framesPushed;
    // else: dropped. m_ring.droppedCount() already tracks this (WU-20a); on
    // a failed tryPush() the moved-from argument -- still bound to the local
    // `retained` above -- is left untouched (core/ring_buffer.hpp's own
    // documented contract), so `retained` still owns the one AddRef this
    // function took and releases it normally when it goes out of scope
    // here. No leak either way.

    return S_OK;
}

HRESULT CaptureSource::QueryInterface(REFIID iid, LPVOID* ppv) {
    if (ppv == nullptr) return E_POINTER;

    const CFUUIDBytes iunknown = CFUUIDGetUUIDBytes(IUnknownUUID);
    if (std::memcmp(&iid, &iunknown, sizeof(REFIID)) == 0) {
        *ppv = static_cast<IDeckLinkInputCallback*>(this);
        AddRef();
        return S_OK;
    }
    if (std::memcmp(&iid, &IID_IDeckLinkInputCallback, sizeof(REFIID)) == 0) {
        *ppv = static_cast<IDeckLinkInputCallback*>(this);
        AddRef();
        return S_OK;
    }

    *ppv = nullptr;
    return E_NOINTERFACE;
}

ULONG CaptureSource::AddRef() { return ++m_refCount; }

ULONG CaptureSource::Release() {
    const ULONG newCount = --m_refCount;
    if (newCount == 0) delete this;
    return newCount;
}

}  // namespace scatter::io
