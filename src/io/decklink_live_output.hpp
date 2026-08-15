// scatter-dve — WU-21c: continuous SDI re-output, scheduling a live-produced
// frame stream (WU-21b's own CaptureConsumer::copyLatestFrame()) onto
// IDeckLinkOutput. Closes architecture.md 10's own Phase 5 "done when" line
// ("live SDI in, warped, SDI out") end to end for the first time: live SDI
// in (WU-20b's CaptureSource), warped in memory (WU-21a/b's runFrameBytes()/
// CaptureConsumer), live SDI out (this unit).
//
// See DECISIONS.md ADR-050 for the full design and for why this is a
// materially different mechanism from LoopedFramePlayback's own WU-15a
// design ("loop one static frame forever", ADR-032): that class schedules
// the *same* IDeckLinkMutableVideoFrame object every completion, because its
// own content never changes. Here the whole point is that content DOES
// change, once per output tick, from whatever CaptureConsumer most recently
// produced — which needs more than one frame buffer in flight at a time (a
// buffer currently queued for playback cannot safely be overwritten) and a
// refill step that copies fresh bytes in, rather than rescheduling an
// unchanged pointer. The real SDK's own FilePlayback sample
// (DeckLinkPlaybackDevice::scheduleVideo(), reread this session) is the
// precedent this mirrors: it too schedules a genuinely new frame per
// completion, sourced from an ongoing decode, not a fixed buffer.
//
// Genlock / clock-domain interoperation (ADR-037's own second follow-up,
// open since it was first named, and this unit's own concern for the first
// time — a real capture clock and a real output clock now genuinely have to
// interoperate, not just be named separately): this unit does not attempt
// any synchronisation. Every refill pulls whatever CaptureConsumer reports
// as its own latest successfully produced frame, with no timestamp
// comparison against the output's own schedule. A capture/process rate
// slower than the output's fixed cadence naturally shows as the same bytes
// scheduled more than once in a row (framesRepeated(), below); a faster one
// shows as some produced frames never being sampled before a newer one
// supersedes them in CaptureConsumer's own single-slot "latest" buffer —
// silent, not queued, not measured. Free-running per ADR-010, unchanged;
// this is the specific, narrower decision of how two independently clocked
// producers/consumers meet at one shared buffer, not a genlock solution.
//
// CaptureConsumer's own start()/stop() are entirely this unit's caller's
// responsibility, in whichever order — same "the two objects share only a
// buffer, neither owns the other's lifecycle" independence ADR-048/049
// already established between CaptureSource and CaptureConsumer, extended
// here to CaptureConsumer and this class.

#pragma once

#include "io/com_ptr.hpp"
#include "io/decklink_capture_consumer.hpp"
#include "io/decklink_output.hpp"  // PlaybackStats, reused directly — ADR-029's own
                                    // "reuse a tested type" precedent, not a near-duplicate.

#include "DeckLinkAPI.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace scatter::io {

// Wraps one IDeckLinkOutput as a continuously-refilled scheduled-playback
// sink: a fixed pool of frame buffers, each refilled from
// consumer.copyLatestFrame() and rescheduled exactly once per completion, in
// round-robin order matching the SDK's own FIFO completion guarantee. See
// DECISIONS.md ADR-050 for the pool-sizing and refill-policy reasoning.
//
// Implements IDeckLinkVideoOutputCallback directly, same shape
// LoopedFramePlayback already uses (ADR-032) — a real, non-trivial IUnknown
// reference count, ComPtr::adopt() on create() for the constructor's own
// initial reference.
class LiveFramePlayback : public IDeckLinkVideoOutputCallback {
public:
    // width/height must equal consumer's own PipelineParams::destWidth/
    // destHeight — the geometry this class expects copyLatestFrame() to
    // hand back. consumer must outlive the returned LiveFramePlayback and is
    // referenced, not owned (see this file's own header comment on
    // lifecycle independence). Confirms displayMode is supported for
    // bmdFormat10BitYUV (DoesSupportVideoMode, same precautionary check
    // LoopedFramePlayback::startWith() already makes), confirms the SDK's
    // own RowBytesForPixelFormat() agrees with video::v210::rowBytesMin(width)
    // — enforced here, not left to a caller's own separate test, since a
    // mismatch would misalign every live-produced frame silently, not just
    // fail one static check the way WU-15a's own
    // test_v210_rowbytes_matches_project_own_computation() does — allocates
    // a pool of round(frameRate / 2) frame buffers (ADR-032's own preroll
    // convention, reused for pool sizing rather than invented fresh),
    // prerolls and schedules all of them, then starts scheduled playback.
    // Returns a null ComPtr, leaving output disabled, on any failure along
    // the way.
    static ComPtr<LiveFramePlayback> create(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode, int width,
                                             int height, const CaptureConsumer& consumer);

    // Same shape LoopedFramePlayback::stop() already uses (ADR-032): safe to
    // call at most once; a second call is a no-op.
    void stop();

    const PlaybackStats& stats() const noexcept { return m_stats; }

    // Count of completions whose own refill found no fresher content than
    // what was already scheduled — either consumer.copyLatestFrame()
    // returned false (nothing successfully produced yet) or returned a
    // buffer whose size did not match this class's own expected geometry
    // (a caller-supplied width/height mismatch against consumer's own
    // PipelineParams). The one real, observable data point this unit
    // surfaces for ADR-037's own genlock/clock-domain-drift follow-up — not
    // itself a diagnosis, the same "named, not resolved" treatment
    // kCaptureRingCapacity's own framesArrived/framesPushed gap got at
    // WU-21b (ADR-049).
    int framesRepeated() const noexcept { return m_framesRepeated.load(); }

    // IDeckLinkVideoOutputCallback
    HRESULT ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame,
                                     BMDOutputFrameCompletionResult result) override;
    HRESULT ScheduledPlaybackHasStopped() override;

    // IUnknown
    HRESULT QueryInterface(REFIID iid, LPVOID* ppv) override;
    ULONG AddRef() override;
    ULONG Release() override;

private:
    LiveFramePlayback(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode, int width, int height,
                       const CaptureConsumer& consumer);

    bool startWith();
    bool refillAndSchedule(std::size_t poolIndex);

    ComPtr<IDeckLinkOutput> m_output;
    BMDDisplayMode m_displayMode;
    int m_width;
    int m_height;
    const CaptureConsumer& m_consumer;

    std::vector<ComPtr<IDeckLinkMutableVideoFrame>> m_pool;
    std::size_t m_nextPoolIndex = 0;
    BMDTimeValue m_frameDuration = 0;
    BMDTimeScale m_timeScale = 0;
    BMDTimeValue m_nextScheduleTime = 0;
    std::atomic<bool> m_stopping{false};

    std::atomic<ULONG> m_refCount{1};
    PlaybackStats m_stats;
    std::atomic<int> m_framesRepeated{0};
};

}  // namespace scatter::io
