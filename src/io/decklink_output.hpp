// scatter-dve — WU-15a: scheduled playback, one looped file-sourced frame to
// SDI out.
//
// See DECISIONS.md ADR-032 for the design this header declares, and for why
// WU-15 (WORK-UNITS.md's own "scheduled playback, file source to SDI out")
// splits into WU-15a (this unit: get one already-warped frame, read from a
// file, scheduled and looping on the real hardware, confirmed by eye once on
// a broadcast monitor) and WU-15b (a longer, unattended one-hour endurance
// run of the same mechanism — not implementation work, and not this
// session's own job). Everything here stops at "loop one frame" — decoding a
// real multi-frame file sequence is a later unit's job, not named yet in
// WORK-UNITS.md.
//
// Reads raw packed .v210 bytes directly from disk via <fstream>, not
// video::readV210File()'s unpack-to-Sample-planes path (WU-05): this unit's
// own job is moving bytes from a file into a DeckLink output buffer
// unchanged, and bmdFormat10BitYUV *is* v210 (DeckLinkAPIModes.h's own FourCC
// comment, "'v210'") — unpacking to Sample planes and repacking would be
// pure waste for that job, and would add a dependency on video/v210.hpp this
// unit does not otherwise need. See ADR-032 for the row-bytes consistency
// check this implies, and why it is the caller's test that checks it, not
// this file.

#pragma once

#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <atomic>
#include <cstdint>
#include <string>

namespace scatter::io {

// Counts updated only from the DeckLink completion-callback thread — see
// decklink_output.cpp's own ScheduledFrameCompleted() for why no additional
// locking is needed beyond these being std::atomic (a caller may read them
// from any thread at any time; each individual counter's value is always
// consistent, and this unit makes no claim about reading more than one of
// them as a single atomic snapshot).
struct PlaybackStats {
    std::atomic<int> completed{0};
    std::atomic<int> displayedLate{0};
    std::atomic<int> dropped{0};
    std::atomic<int> flushed{0};
};

// Wraps one IDeckLinkOutput as a looped, single-frame scheduled-playback
// source: the same frame, read once from v210FilePath at create() time, is
// rescheduled every completion, forever, until stop() is called. See
// DECISIONS.md ADR-032 for why one static frame — not a decoded sequence —
// is this unit's own scope, and for the preroll/refill idiom, both taken
// from the real SDK's own FilePlayback and SignalGenerator samples rather
// than architecture.md 7's own illustrative "3-frame preroll" figure.
//
// Implements IDeckLinkVideoOutputCallback directly (this class *is* the
// callback registered with SetScheduledFrameCompletionCallback) — matching
// the shape of the SDK's own DeckLinkOutputCallback/DeckLinkPlaybackDevice
// samples, which both do the same rather than using a separate delegate
// object. A real (non-trivial) IUnknown reference count is required here,
// not a shortcut: SetScheduledFrameCompletionCallback() takes ownership the
// same way any COM interface pointer handed to a callee for storage does,
// so this object must survive exactly as long as something (the SDK, or
// this project's own caller) holds a reference to it, not merely as long as
// create()'s caller happens to keep its own ComPtr alive.
class LoopedFramePlayback : public IDeckLinkVideoOutputCallback {
public:
    // Reads exactly IDeckLinkOutput::RowBytesForPixelFormat(bmdFormat10BitYUV,
    // width, ...) * height bytes from v210FilePath (see the file header for
    // why this is a raw read, not video::readV210File()), confirms
    // displayMode is supported for bmdFormat10BitYUV via DoesSupportVideoMode,
    // enables video output, creates and fills one IDeckLinkMutableVideoFrame
    // from those bytes, prerolls (ADR-032: half of displayMode's own frame
    // rate, rounded, computed from the negotiated IDeckLinkDisplayMode, not
    // hardcoded), then calls StartScheduledPlayback. Returns a null ComPtr,
    // leaving output disabled, on any failure along the way — including a
    // short/missing file, an unsupported display mode, or any SDK call
    // returning other than S_OK.
    static ComPtr<LoopedFramePlayback> create(ComPtr<IDeckLinkOutput> output,
                                               BMDDisplayMode displayMode,
                                               const std::string& v210FilePath,
                                               int width, int height);

    // StopScheduledPlayback, then SetScheduledFrameCompletionCallback(nullptr)
    // and DisableVideoOutput — DisableVideoOutput() itself blocks until
    // every scheduled frame is completed or flushed (confirmed against the
    // real SDK's own SignalGenerator sample, stopRunning()'s own comment;
    // ADR-032), so this unit needs none of FilePlayback's own extra
    // mutex/condition-variable wait for ScheduledPlaybackHasStopped(). Safe
    // to call at most once; a second call is a no-op.
    void stop();

    const PlaybackStats& stats() const noexcept { return m_stats; }

    // IDeckLinkVideoOutputCallback
    HRESULT ScheduledFrameCompleted(IDeckLinkVideoFrame* completedFrame,
                                     BMDOutputFrameCompletionResult result) override;
    HRESULT ScheduledPlaybackHasStopped() override;

    // IUnknown
    HRESULT QueryInterface(REFIID iid, LPVOID* ppv) override;
    ULONG AddRef() override;
    ULONG Release() override;

private:
    LoopedFramePlayback(ComPtr<IDeckLinkOutput> output, BMDDisplayMode displayMode,
                         int width, int height);

    bool startWith(const std::string& v210FilePath);
    bool scheduleOne();

    ComPtr<IDeckLinkOutput> m_output;
    BMDDisplayMode m_displayMode;
    int m_width;
    int m_height;

    ComPtr<IDeckLinkMutableVideoFrame> m_frame;
    BMDTimeValue m_frameDuration = 0;
    BMDTimeScale m_timeScale = 0;
    BMDTimeValue m_nextScheduleTime = 0;
    std::atomic<bool> m_stopping{false};

    std::atomic<ULONG> m_refCount{1};
    PlaybackStats m_stats;
};

}  // namespace scatter::io
