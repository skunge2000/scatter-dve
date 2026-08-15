// scatter-dve — WU-20b: DeckLink capture: format-detection-aware
// EnableVideoInput, IDeckLinkInputCallback implementation, retained frames
// pushed into a scatter::RingBuffer (WU-20a).
//
// See DECISIONS.md ADR-046 (the real IDeckLinkInput/IDeckLinkInputCallback
// shape, the WU-20a/WU-20b split, the three real capture samples' own
// disagreeing frame-retention and format-change-restart behaviour) and
// ADR-047 (this unit's own frozen scope: the design questions ADR-046 left
// open -- ring capacity, whether a no-input-source frame is pushed, whether
// to reuse CaptureStills' own signal-recovery restart -- decided here, plus
// why this sandbox cannot compile or run any of it). Targets the
// **UltraStudio Recorder 3G** by name (ADR-039); the code itself selects a
// device generically, by capability, the same "not device-specific"
// convention ADR-034 already established for output.
//
// Mirrors io/decklink_output.hpp's own LoopedFramePlayback shape (ADR-032):
// this class *is* the callback registered with SetCallback, a real
// (non-trivial) IUnknown reference count, ComPtr::adopt() on create() for
// the constructor's own initial reference -- the same pattern this project
// already has one working instance of, applied here to the input side for
// the first time.

#pragma once

#include "core/ring_buffer.hpp"
#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <atomic>
#include <cstddef>
#include <cstdint>

namespace scatter::io {

// Ring capacity: architecture.md 7's own "Expect 3-4 frames end-to-end...
// 60-80ms at 50p" -- this unit's own number to pick, per ADR-046 ("N -- the
// actual capacity -- is this unit's own number to pick, not decided here").
// Chosen: 8, double the high end of that named range -- headroom over the
// documented expectation, not the bare minimum, the same margin WU-15a's own
// preroll (half a second of frames, not the SDK's illustrative "3-frame"
// figure) already used for a comparable buffering decision. Not tuned
// against a real measured capture-to-consume latency -- WU-21's job, once a
// consumer actually drains this ring; see ADR-047.
inline constexpr std::size_t kCaptureRingCapacity = 8;

// A capture-arrived frame handle, retained (ADR-031's own borrowing
// ComPtr constructor) for as long as it sits in the ring. ComPtr<T>'s move
// constructor/assignment are both noexcept (io/com_ptr.hpp), satisfying
// RingBuffer<T, Capacity>'s own requirement (core/ring_buffer.hpp) with no
// further wrapping needed.
using CaptureFrameRing = RingBuffer<ComPtr<IDeckLinkVideoInputFrame>, kCaptureRingCapacity>;

// Stats surfaced from the capture callback thread -- same std::atomic,
// relaxed-read convention io/decklink_output.hpp's PlaybackStats (WU-15a)
// and core/ring_buffer.hpp's droppedCount() already use: a caller may read
// these from any thread at any time; each individual counter's own value is
// always consistent, with no claim that reading more than one of them is a
// single atomic snapshot.
struct CaptureStats {
    std::atomic<int> framesArrived{0};        // every VideoInputFrameArrived() call with a non-null frame
    std::atomic<int> framesPushed{0};         // successfully pushed into the ring
    std::atomic<int> noInputSourceFrames{0};  // bmdFrameHasNoInputSource set -- filtered, never pushed; see ADR-047
    std::atomic<int> formatChanges{0};        // VideoInputFormatChanged() calls, of any kind
};

// Wraps one IDeckLinkInput as a format-detection-aware capture source.
// Implements IDeckLinkInputCallback directly -- real IUnknown refcounting,
// ComPtr::adopt() on create(), the same pattern LoopedFramePlayback already
// has (ADR-032) -- calls EnableVideoInput() with
// bmdVideoInputEnableFormatDetection after confirming the device actually
// reports BMDDeckLinkSupportsInputFormatDetection (CapturePreview's own
// checked-not-assumed pattern; ADR-046), retains each arriving frame via
// ComPtr's existing borrowing constructor and pushes it into a caller-owned
// CaptureFrameRing, and restarts the input stream on
// VideoInputFormatChanged() when the notified event includes
// bmdVideoInputDisplayModeChanged or bmdVideoInputColorspaceChanged
// (CaptureStills'/CapturePreview's own pattern -- not InputLoopThrough's
// non-restarting one; ADR-046).
//
// Two design questions ADR-046 explicitly left for this unit, decided now
// (see ADR-047 for the full reasoning, not repeated here):
//
// - **Pixel format never changes.** Every EnableVideoInput() call this class
//   makes -- the initial one and every format-change restart -- requests
//   bmdFormat10BitYUV, regardless of detectedSignalFlags. Unlike the SDK's
//   own samples (built for arbitrary HDMI sources that can legitimately be
//   RGB or a different bit depth), this project's whole I/O path is v210
//   4:2:2 10-bit only (ADR-005) and its real target hardware is a
//   fixed-format SDI device (UltraStudio Recorder 3G, ADR-039) -- there is
//   no code anywhere in this project that could consume a different pixel
//   format, so adapting to one on the fly would be a silent no-op at best.
// - **A bmdFrameHasNoInputSource frame is filtered, not pushed.** Consistent
//   with this project's "never fabricate a destination the warp never
//   produced" reasoning elsewhere (ADR-024's off-raster drop), a frame
//   carrying no real signal is not real capture content, and handing one to
//   a future consumer indistinguishably from a genuine frame would be
//   exactly that kind of fabrication. CaptureStills' own signal-recovery
//   restart (StopStreams/FlushStreams/StartStreams on the first valid frame
//   after an invalid one, and not pushing that first recovery frame either)
//   is reused directly, not reinvented -- a real, shipped SDK pattern for a
//   real problem (transient garbage immediately after signal reacquisition).
class CaptureSource : public IDeckLinkInputCallback {
public:
    // Confirms input, format-detection is supported (returns a null ComPtr
    // otherwise -- WU-20's own "format-detection-aware" premise makes this a
    // hard requirement of this unit, not a soft fallback), confirms
    // initialDisplayMode is supported for bmdFormat10BitYUV via
    // DoesSupportVideoMode (the same precautionary check
    // LoopedFramePlayback::startWith() already makes on the output side,
    // ADR-032), then SetCallback(this), EnableVideoInput(initialDisplayMode,
    // bmdFormat10BitYUV, bmdVideoInputEnableFormatDetection), StartStreams()
    // -- CaptureStills'/CapturePreview's own call order. Returns a null
    // ComPtr, leaving input disabled, on any failure along the way. ring is
    // caller-owned and must outlive the returned CaptureSource -- matching
    // CaptureFrameRing's own non-copyable, non-movable shape (WU-20a: a
    // RingBuffer is meant to be constructed once by its caller and
    // referenced from both the producer and the consumer side).
    static ComPtr<CaptureSource> create(ComPtr<IDeckLinkInput> input, BMDDisplayMode initialDisplayMode,
                                         CaptureFrameRing& ring);

    // StopStreams, then SetCallback(nullptr) and DisableVideoInput -- the
    // same shape LoopedFramePlayback::stop() already uses on the output side
    // (ADR-032), for the same reason: this unit has no UI thread or audio
    // stream to coordinate a wait against. Safe to call at most once; a
    // second call is a no-op.
    void stop();

    const CaptureStats& stats() const noexcept { return m_stats; }

    // IDeckLinkInputCallback
    HRESULT VideoInputFormatChanged(BMDVideoInputFormatChangedEvents notificationEvents,
                                     IDeckLinkDisplayMode* newDisplayMode,
                                     BMDDetectedVideoInputFormatFlags detectedSignalFlags) override;
    HRESULT VideoInputFrameArrived(IDeckLinkVideoInputFrame* videoFrame,
                                    IDeckLinkAudioInputPacket* audioPacket) override;

    // IUnknown
    HRESULT QueryInterface(REFIID iid, LPVOID* ppv) override;
    ULONG AddRef() override;
    ULONG Release() override;

private:
    CaptureSource(ComPtr<IDeckLinkInput> input, CaptureFrameRing& ring);

    bool startWith(BMDDisplayMode initialDisplayMode);

    // Best-effort, unverified-until-the-real-terminal shutdown path shared by
    // a failed format-change restart and a failed signal-recovery restart
    // (both inside VideoInputFormatChanged()/VideoInputFrameArrived(), i.e.
    // called from within a callback the SDK itself is invoking -- see
    // ADR-047 for why this is reasoned through, not confirmed safe, in this
    // sandbox). Idempotent with stop() via the same m_stopping flag.
    void stopFromCallback();

    ComPtr<IDeckLinkInput> m_input;
    CaptureFrameRing& m_ring;
    std::atomic<bool> m_stopping{false};
    std::atomic<bool> m_prevInputSourceValid{false};

    std::atomic<ULONG> m_refCount{1};
    CaptureStats m_stats;
};

}  // namespace scatter::io
