// scatter-dve — WU-21b: DeckLink capture-side pixel read. Drains WU-20b's
// own CaptureFrameRing on a private consumer thread, obtains each retained
// frame's own IDeckLinkVideoBuffer via QueryInterface, brackets GetBytes()
// with StartAccess(bmdBufferAccessRead)/EndAccess(bmdBufferAccessRead), and
// feeds the mapped bytes/GetRowBytes() into WU-21a's own runFrameBytes()
// while still inside that bracket.
//
// See DECISIONS.md ADR-048 for the sketch this completes (the WU-21a/b/c
// split, and the open questions named below) and this unit's own new ADR
// entry for the scoping decided this session. Mirrors io/decklink_output.cpp's
// own fillFrameBuffer() (WU-15a, ADR-032) -- the same StartAccess/GetBytes/
// EndAccess bracket, read direction instead of write, extended to input for
// the first time (ADR-046/047/048 already found IDeckLinkVideoBuffer is the
// same interface both directions; this is the first unit to actually read
// through it).
//
// Four questions ADR-048 explicitly left open for this unit, decided now:
//
// - **Consumer-thread ownership/lifetime is entirely independent of
//   CaptureSource's own (WU-20b).** The two objects share only the
//   CaptureFrameRing between them -- the same "caller owns the ring, both
//   sides just reference it" shape WU-20a already established for the
//   producer side. Starting or stopping a CaptureConsumer does not start or
//   stop the CaptureSource feeding it, and vice versa; a caller that wants a
//   clean shutdown stops both itself, in whichever order it prefers -- a
//   frame still sitting in the ring when this consumer stops is simply
//   abandoned, its ComPtr releasing the retained reference normally when the
//   ring itself is destroyed or a slot is next overwritten, the same
//   "no leak either way" property WU-20b's own VideoInputFrameArrived()
//   comment already relies on for a failed tryPush().
// - **Extracted bytes are handed to runFrameBytes() while still inside the
//   StartAccess/EndAccess bracket, never copied into a separate pool
//   buffer first.** A captured frame's own buffer contents are only
//   guaranteed valid between StartAccess and EndAccess (the same
//   requirement io/decklink_output.cpp's own fillFrameBuffer() honours on
//   the write side); runFrameBytes() itself is synchronous and retains
//   nothing past its own call, so calling it directly against the mapped
//   pointer, before EndAccess, is both correct and needs no extra pool
//   buffer or copy this unit would otherwise have to invent and size.
// - **The ring's own drain policy while empty: poll with a short
//   (1ms) sleep.** core/ring_buffer.hpp's RingBuffer (WU-20a, frozen) has no
//   blocking pop -- tryPop() is documented to never block, matching
//   architecture.md 6's own capture-callback-thread requirement, but that
//   requirement is about the *producer* side; this consumer thread is not
//   the driver's own callback thread and is free to wait. A short sleep
//   keeps this thread from spinning a full CPU core while the ring is
//   empty, without needing a new blocking-pop capability on a class WU-20a
//   already froze.
// - **A captured frame's own reported GetWidth()/GetHeight()/GetRowBytes()
//   are trusted directly, per frame, never checked against the display mode
//   CaptureSource::create() was given.** The same "ask the SDK, do not
//   assume this project's own computation agrees with it" rule
//   architecture.md 7 already states for row bytes specifically (extended to
//   width/height here for the same reason) -- and after a format-change
//   restart (io/decklink_input.cpp's own VideoInputFormatChanged()) a
//   genuinely different geometry is exactly what a new frame could
//   legitimately report; trusting it directly needs no separate consistency
//   check to stay correct. Destination geometry is the opposite: fixed once,
//   at this consumer's own construction (PipelineParams::destWidth/
//   destHeight), the same "geometry fixed for the object's own lifetime"
//   shape LoopedFramePlayback already uses for its own m_width/m_height on
//   the output side -- WU-21c's own job (continuous SDI re-output,
//   DECISIONS.md ADR-048) is where a caller might want that to vary, not
//   this unit's.
//
// Does not reschedule the produced bytes onto IDeckLinkOutput -- that is
// WU-21c's own job (DECISIONS.md ADR-048), a materially different mechanism
// from LoopedFramePlayback's own "loop one static frame forever" design.
// This unit stops at making the most recently produced destination frame's
// own bytes available to whatever caller asks for them (copyLatestFrame()).
//
// WU-22c (ADR-058) adds the optional CoverageCallback below: an opt-in hook
// into the same per-frame processOne() this unit already runs, so a caller
// that wants a live coverage view (src/diag/coverage_view.hpp, WU-22b) does
// not need its own separate capture path or a second runFrame() call against
// the same frame. Mirrors PipelineParams::pool/weightOut's own established
// "non-owning, default nullptr/empty, exactly zero added cost or behaviour
// change when the caller does not opt in" shape (ADR-044/056) -- extended
// here from a raw pointer to a std::function, since what WU-22c needs handed
// across is ownership of a freshly filled buffer, not a pointer into memory
// this consumer thread is about to reuse next frame.
//
// WU-23b2b (DECISIONS.md ADR-080, extended by ADR-081) wires this project's
// own Weston 3-field de-interlace filter (video::Deinterlacer, WU-23b1) into
// this class: processOne() below now calls scatter::runFrameBytesDeinterlaced()
// (core/resolve.hpp, WU-23b2a) in place of scatter::runFrameBytes(), driving
// one owned Deinterlacer member across this consumer's own whole lifetime.
// runFrameBytesDeinterlaced()'s own false return (stream start -- the very
// first popped frame this consumer's own Deinterlacer ever sees, never a
// recurring cost) is a genuine third processOne() outcome, distinct from
// both success and failure -- see ProcessResult below and ADR-081 for why an
// enum return was picked over the other shapes ADR-080 left open, and the
// new framesStreamStart counter on CaptureConsumerStats.

#pragma once

#include "core/resolve.hpp"
#include "io/com_ptr.hpp"
#include "io/decklink_input.hpp"
#include "video/deinterlace.hpp"
#include "video/v210.hpp"

#include "DeckLinkAPI.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace scatter::io {

// Same std::atomic, relaxed-read convention every other stats struct in this
// project's own io/ and core/ files already uses (io/decklink_output.hpp's
// PlaybackStats, io/decklink_input.hpp's CaptureStats, core/ring_buffer.hpp's
// droppedCount()): a caller may read these from any thread at any time; no
// claim that reading more than one of them is a single atomic snapshot.
struct CaptureConsumerStats {
    std::atomic<int> framesPopped{0};     // every successful CaptureFrameRing::tryPop()
    std::atomic<int> framesProcessed{0};  // StartAccess/GetBytes/runFrameBytesDeinterlaced/EndAccess all succeeded and produced a frame
    std::atomic<int> framesFailed{0};     // popped but QueryInterface/StartAccess/GetBytes/EndAccess failed, or bad geometry
    // WU-23b2b (ADR-080/081): popped and every step through EndAccess
    // succeeded, but this consumer's own Deinterlacer had no prior frame
    // yet -- the very first popped frame of this instance's own lifetime
    // only (runFrameBytesDeinterlaced() returned false; core/resolve.hpp's
    // own documented contract). Not counted as framesFailed: this is not an
    // error and is never retried. m_latestFrame is left completely
    // untouched for this one frame -- see processOne()'s own
    // ProcessResult::StreamStart, below.
    std::atomic<int> framesStreamStart{0};
};

class CaptureConsumer {
public:
    // WU-22c (ADR-058): invoked once per successfully processed frame (i.e.
    // exactly when framesProcessed is incremented, never on a failed frame),
    // from CaptureConsumer's own dedicated consumer thread (the same thread
    // run()/processOne() already execute on -- NOT the DeckLink driver's own
    // callback thread, and NOT any Cocoa/main thread). Handed ownership of a
    // freshly filled std::vector<WeightAccum> of exactly
    // params.destWidth * params.destHeight elements, row-major, matching
    // PipelineParams::weightOut's own documented layout (ADR-056) --
    // moved-from, not borrowed, so a callback that itself hands the buffer
    // off again (e.g. into a dispatch_async block bound for Cocoa's main
    // thread, WU-22c's own use) never has to copy it first. Must not throw:
    // the same "fn must not throw" precondition this project already places
    // on every callback it invokes from inside a tight per-frame loop
    // (ThreadPool::runOnAll(), core/pipeline.hpp).
    using CoverageCallback = std::function<void(std::vector<WeightAccum>)>;

    // ring is caller-owned and must outlive this object -- the same
    // convention CaptureSource::create() already documents for its own ring
    // parameter (WU-20b). lattice and params are copied in, not referenced:
    // this consumer's own consumer thread reads them on every popped frame,
    // for as long as it runs, and this unit does not assume a caller's own
    // lattice storage outlives that thread. params.destWidth/destHeight fix
    // this consumer's own destination geometry for its whole lifetime --
    // unlike the lattice (see setLattice() below, WU-21f), destination
    // geometry has no update path and was never asked for one.
    //
    // coeffs (WU-23b2b, DECISIONS.md ADR-080, extended by ADR-081): which of
    // the real Weston filter's two coefficient sets this consumer's own
    // owned Deinterlacer is constructed with -- Simple (2 low-pass + 3
    // high-pass taps) or Complex (4 + 5 taps). No default: ADR-080
    // deliberately left this choice open rather than picking one silently,
    // so every caller states it explicitly (Steve's own choice, raised and
    // settled this session: Complex -- see ADR-081). Fixed for this
    // consumer's own whole lifetime, the same "no caller has asked for this
    // to vary mid-stream" convention destWidth/destHeight above already
    // use.
    //
    // coverageCallback defaults to nullptr (WU-22c, ADR-058): the opt-in
    // hook described above this class. When null, processOne() does not
    // allocate or fill a coverage buffer at all -- params.weightOut stays
    // exactly whatever the caller's own params.weightOut already was
    // (normally left at its own nullptr default, per ADR-056), so a caller
    // that does not pass this parameter sees zero cost and zero behaviour
    // change versus this unit's own pre-WU-22c shape.
    CaptureConsumer(CaptureFrameRing& ring, Lattice lattice, PipelineParams params,
                     video::DeinterlaceCoefficients coeffs,
                     CoverageCallback coverageCallback = nullptr);
    ~CaptureConsumer();

    CaptureConsumer(const CaptureConsumer&) = delete;
    CaptureConsumer& operator=(const CaptureConsumer&) = delete;

    // Spawns the consumer thread. Safe to call at most once.
    void start();

    // Signals the consumer thread to exit its poll loop and joins it. Safe
    // to call at most once (a second call is a no-op, the same
    // compare-exchange idiom CaptureSource::stop()/LoopedFramePlayback::stop()
    // already use) and safe to call from the destructor if start() was never
    // called (the thread is simply not joinable then).
    void stop();

    const CaptureConsumerStats& stats() const noexcept { return m_stats; }

    // Copies the most recently successfully produced destination frame's own
    // packed v210 bytes into out, replacing its contents, and returns true --
    // or returns false, leaving out unchanged, if no frame has been
    // successfully processed yet. Safe to call from any thread; guarded
    // against the consumer thread's own concurrent write by m_mutex.
    bool copyLatestFrame(std::vector<std::uint8_t>& out) const;

    // WU-21f: replaces the lattice used for every frame processed after this
    // call returns -- an animated caller (e.g. a rotating-shape demo) calls
    // this on its own thread as often as it likes; the consumer thread picks
    // up the new lattice at the start of its own next processOne(), never
    // mid-frame. Guarded by m_latticeMutex, a dedicated mutex separate from
    // m_mutex (which guards m_latestFrame instead) -- the same "each piece of
    // cross-thread state gets its own lock, not one shared lock for
    // everything" shape this class already used for m_latestFrame, extended
    // here rather than reused, so a caller polling copyLatestFrame() and a
    // caller animating setLattice() never contend with each other. Safe to
    // call from any thread, including before start(). Takes effect for
    // frames processed after this call, not retroactively.
    void setLattice(Lattice lattice);

    // WU-35a3 (CORRECTIONS.md C-035): replaces the manualTransp coefficient
    // used for every frame processed after this call returns -- mirrors
    // setLattice() immediately above exactly, for the same reason: C-035
    // found that PipelineParams::manualTransp being additive (WU-35a1) said
    // nothing about whether a caller holding an already-constructed
    // CaptureConsumer could change it afterward, and it could not --
    // m_params is stored const and read by the consumer thread on every
    // frame, for this object's whole lifetime, with no update path. An
    // animated caller (e.g. tests/test_decklink_live_sphere.cpp's own T/t
    // operator controls, WU-35a2) calls this on its own thread as often as
    // it likes; the consumer thread picks up the new value at the start of
    // its own next processOne(), never mid-frame. Guarded by a new
    // dedicated m_manualTranspMutex, separate from both m_latticeMutex and
    // m_mutex -- the same "each piece of cross-thread state gets its own
    // lock, not one shared lock for everything" shape setLattice() already
    // established, extended here rather than reused, so a caller polling
    // copyLatestFrame(), a caller animating setLattice(), and a caller
    // animating setManualTransp() never contend with each other. Safe to
    // call from any thread, including before start(). Takes effect for
    // frames processed after this call, not retroactively.
    void setManualTransp(Weight manualTransp);

private:
    // WU-23b2b (ADR-080, extended by ADR-081): processOne()'s own three
    // now-possible outcomes, communicated to run() as an enum class return
    // rather than a plain bool, a second bool, or an out-parameter -- the
    // other shapes ADR-080 explicitly left open for this unit to pick
    // between. Chosen because run()'s own dispatch is genuinely a three-way
    // branch on "what happened," not a flag to check alongside a separate
    // yes/no, and a scoped enum return keeps that dispatch a single
    // exhaustive switch (see run(), decklink_capture_consumer.cpp) rather
    // than two conditionals whose combination has an unreachable fourth
    // case to reason about. Private: no caller outside run()/processOne()
    // itself has a use for it. See DECISIONS.md ADR-081 for the full
    // account of this decision.
    enum class ProcessResult {
        Processed,    // StartAccess/GetBytes/runFrameBytesDeinterlaced/EndAccess all succeeded and produced a frame
        Failed,       // popped but QueryInterface/StartAccess/GetBytes/EndAccess failed, or bad geometry
        StreamStart,  // succeeded, but this consumer's own Deinterlacer had no prior frame yet (ADR-080) -- not an error, never retried
    };

    void run();  // consumer thread body
    ProcessResult processOne(ComPtr<IDeckLinkVideoInputFrame> frame);

    CaptureFrameRing& m_ring;

    mutable std::mutex m_latticeMutex;
    Lattice m_lattice;  // guarded by m_latticeMutex; see setLattice() (WU-21f)

    // WU-35a3 (CORRECTIONS.md C-035): mirrors m_latticeMutex/m_lattice
    // immediately above exactly -- see setManualTransp() for why.
    mutable std::mutex m_manualTranspMutex;
    Weight m_manualTransp;  // guarded by m_manualTranspMutex; see setManualTransp() (WU-35a3)

    const PipelineParams m_params;
    const std::size_t m_dstRowBytes;
    const CoverageCallback m_coverageCallback;  // WU-22c; null unless caller opts in

    // WU-23b2b (ADR-080): this consumer's own owned Deinterlacer -- one
    // instance for this object's whole lifetime, not two (ADR-080's own
    // "one instance, not two" decision: a single instance already produces
    // one full progressive frame per push with every row present, and a
    // second, opposite-anchor instance would only earn its keep for
    // field-rate output, which this project has ruled out) -- constructed
    // with FieldParity::Top as its anchor parity (video/interlace.hpp's own
    // "Top is first-transmitted field" convention, already used without
    // incident through WU-23a/WU-23a2) and whichever DeinterlaceCoefficients
    // the constructor above was given. Touched only from this consumer's
    // own consumer thread, inside processOne() -- not guarded by a mutex,
    // the same single-writer-thread convention m_stats already relies on.
    video::Deinterlacer m_deinterlacer;

    std::thread m_thread;
    std::atomic<bool> m_stopping{false};
    bool m_started = false;

    mutable std::mutex m_mutex;
    std::vector<std::uint8_t> m_latestFrame;  // guarded by m_mutex

    CaptureConsumerStats m_stats;
};

}  // namespace scatter::io
