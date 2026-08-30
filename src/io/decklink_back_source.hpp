// scatter-dve — WU-33c2b: a Blackmagic capture-sub-device as another
// pluggable "back" source, sibling to WU-33c2a's own FileBackSource
// (io/file_back_source.hpp) — standing in for a second live capture feed on
// the back side of a self-folding surface (PipelineParams::backSrc,
// WU-33a/WU-33b, DECISIONS.md ADR-092/ADR-093).
//
// DECISIONS.md ADR-095 (Session 79) amended WU-33c2's original scope (a
// single, hard-wired second live DeckLink device) to a pluggable back
// source, config-selectable among at least two concrete producers, and
// fixed the shared shape every producer exposes: a real, owned
// OwnedSourceRaster snapshot (core/resolve.hpp, WU-33c1), never a cheap
// view, because a live-capture producer's own buffer is concurrently
// mutated by its own consumer thread. DECISIONS.md ADR-096 (this unit)
// re-derives this producer's own exact shape against the real current code,
// per WORK-UNITS.md's own WU-33c2b note's explicit instruction not to trust
// its sketch — see that ADR for the full account of every decision below,
// not repeated in full here.
//
// **Reused unchanged, confirmed by direct re-read this session:**
// io/decklink_device.hpp's own enumerateDeckLinkDevices() (WU-14, ADR-031)
// and io/decklink_input.hpp's own CaptureSource/CaptureFrameRing (WU-20b) —
// neither needs a single line changed for this unit. A caller constructs a
// CaptureSource against a selected IDeckLinkInput and a caller-owned
// CaptureFrameRing exactly the way every existing capture test in this
// repository already does; DeckLinkBackSource below only ever touches the
// ring, never IDeckLinkInput or CaptureSource directly — the exact
// "caller owns the ring, both sides just reference it" shape WU-20a/WU-21b
// already established for the front side (io/decklink_capture_consumer.hpp),
// mirrored here rather than reinvented.
//
// **What this unit actually adds, two things:**
//
// 1. selectFormatDetectionCapableInput() — generalizes the
//    firstFormatDetectionCapableInput() convention duplicated, unchanged,
//    across this repository's own four existing DeckLink test files
//    (confirmed by direct grep this session: tests/test_decklink_input.cpp,
//    test_decklink_capture_consumer.cpp, test_decklink_live_output.cpp,
//    test_decklink_live_sphere.cpp — exactly four, still, no fifth call
//    site added by this unit) from "always return the first match" to
//    "return the match a caller-supplied selector names," by zero-based
//    index into the filtered (capture + format-detection-capable) results
//    or by a modelName/displayName substring — DeviceInfo already carries
//    both, so no new field is needed on that struct
//    (io/decklink_device.hpp, untouched). This is genuine, reusable
//    production code, not a test-local convenience duplicated per
//    translation unit the way firstFormatDetectionCapableInput() itself is
//    (SESSION-PROTOCOL.md's own per-test-file fixture isolation, applied to
//    a self-contained test fixture — not to a selection mechanism at least
//    two real, non-test callers are already expected to need: this unit's
//    own test below, and WU-33c4's future command-line flag,
//    WORK-UNITS.md's own WU-33c4 note) — so it lives here, once, where
//    whoever picks up either of those future call sites can find and reuse
//    it directly rather than duplicate it a fifth and sixth time. A
//    default-constructed selector (neither index nor nameSubstring set)
//    reproduces today's existing "first match" behaviour exactly — a
//    caller that does not opt into the new mechanism sees no change.
//
// 2. DeckLinkBackSource — the back-side consumer WU-33c2's own original
//    note (Session 78) already sketched: drains a CaptureFrameRing on its
//    own private thread, brackets IDeckLinkVideoBuffer::GetBytes() with
//    StartAccess(bmdBufferAccessRead)/EndAccess(bmdBufferAccessRead) exactly
//    the way io/decklink_capture_consumer.hpp's own CaptureConsumer
//    (WU-21b) already does, but calls unpackSourceRaster() (WU-33c1,
//    core/resolve.hpp/core/pipeline.cpp) in place of
//    runFrameBytesDeinterlaced() — this class never calls runFrame() or
//    produces output v210 bytes of its own; it stops exactly where
//    OwnedSourceRaster construction already stops, matching
//    unpackSourceRaster()'s own documented "third caller" role
//    (core/resolve.hpp's own comment on that function). Needs no
//    PipelineParams, no Lattice, and no destination geometry at all —
//    unlike CaptureConsumer, which warps into a fixed dest raster,
//    unpackSourceRaster() only ever needs the captured frame's own
//    GetWidth()/GetHeight()/GetRowBytes(), trusted directly, per frame, the
//    same "ask the SDK, do not assume" convention
//    io/decklink_capture_consumer.cpp's own processOne() already uses for
//    exactly these three values (io/decklink_capture_consumer.hpp's own
//    file comment).
//
// **Deliberately, explicitly out of scope this unit: deinterlace.**
// unpackSourceRaster() (WU-33c1) has no video::Deinterlacer hook of its
// own — confirmed by direct re-read of core/resolve.hpp/core/pipeline.cpp
// this session, not assumed: it reproduces only runFrameBytes()'s first
// three steps (v210 unpack, chroma upsample, RGB boundary conversion),
// never runFrameBytesDeinterlaced()'s own deinterlace step, which sits
// between chroma upsample and the RGB conversion and needs its own
// Deinterlacer instance threaded through a new core-level function
// unpackSourceRaster() does not have. Giving this back source the same
// deinterlace quality the front side already has (io/decklink_capture_
// consumer.hpp's own m_deinterlacer, WU-23b2b/ADR-080/081) would mean
// adding that new core/resolve.hpp/core/pipeline.cpp function first — real,
// separate, sandbox-buildable-and-testable scatter-core work, not a
// device-bridge-hand-off-only io/ addition, and well past this unit's own
// two-new-file budget (SESSION-PROTOCOL.md's "at most 3 source files plus
// its test" cap, already spent on this file plus its .cpp). WORK-UNITS.md's
// own WU-33c2b note (Session 79) already named this exact question and left
// it unresolved ("remains an open question this note does not resolve");
// re-derived directly against the real code this session, not assumed, and
// resolved the same way WU-33c2a's own looped-playback question was
// resolved one level up (DECISIONS.md ADR-095): named, split out, not
// silently expanded into this unit. A genuinely interlaced back signal (the
// same 576i25 signal this project's whole test suite already targets, ADR-
// 007/033) will therefore show real comb artifacts through this producer
// until that follow-on unit exists — not a silent defect, a named limit,
// the same "static frame only, looped playback is a later unit" shape
// WU-33c2a's own header comment already documents for its own scope limit.
// Whoever picks up that follow-on unit should weigh a new
// unpackSourceRasterDeinterlaced() (core/resolve.hpp/core/pipeline.cpp,
// mirroring runFrameBytesDeinterlaced()'s own false-on-stream-start
// contract) against the real code by then, the same "re-derive, do not
// assume this note's own sketch still matches" convention this whole ADR-095
// lineage already follows.
//
// **A second real gap this unit's own design surfaced, not previously
// visible: DECISIONS.md ADR-095 fixed every back-source producer's own
// accessor at the plain std::function<OwnedSourceRaster()> shape — no
// std::optional, no "not ready yet" case — because FileBackSource
// (WU-33c2a) can always satisfy it: create() itself fails (returns
// std::nullopt) if the static file can't be read, so a live FileBackSource
// instance always already holds a real frame from construction onward, with
// no in-between state. A live DeckLink capture producer has no equivalent
// guarantee: DeckLinkBackSource::start() returns before any frame has
// necessarily arrived (format detection, cable state, and this project's
// own existing tests' "stats().framesProcessed may legitimately stay at
// zero" caveat — test_decklink_input.cpp's own header comment — all still
// apply identically here), and OwnedSourceRaster itself has no default
// constructor to fabricate a placeholder from (core/resolve.hpp's own
// comment on why). currentSourceRaster() below therefore returns
// std::optional<OwnedSourceRaster>, not OwnedSourceRaster — a genuine,
// logged divergence from FileBackSource's own signature, and from the exact
// std::function<OwnedSourceRaster()> shape ADR-095 fixed. Reconciling the
// two (block until ready with a timeout, substitute a caller-chosen
// fallback frame, widen the front consumer's own callable contract to
// tolerate std::optional, or something else) is left to WU-33c3 — the unit
// that actually calls this producer's own accessor and is positioned to
// decide what "the back source is not ready yet" should mean for the front
// pipeline, a decision this single-producer unit should not make on its
// own. Not built, run, or confirmed against real hardware this session, or
// any prior one — do not claim this works against Steve's own UltraStudio/
// DeckLink hardware until he has run it and said so.

#pragma once

#include "core/resolve.hpp"        // OwnedSourceRaster, unpackSourceRaster()
#include "io/com_ptr.hpp"
#include "io/decklink_device.hpp"  // DeviceInfo
#include "io/decklink_input.hpp"   // CaptureFrameRing

#include "DeckLinkAPI.h"

#include <atomic>
#include <cstddef>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace scatter::io {

// Selects among enumerateDeckLinkDevices()'s own filtered (capture + input
// format-detection capable) results generically — by zero-based index, or
// by a modelName/displayName substring — never by "device 1 vs device 2" as
// physically distinct concepts (DECISIONS.md ADR-095/ADR-096). At most one
// of index/nameSubstring should be set by a caller; if index is set it
// takes priority and nameSubstring is ignored, not treated as an error —
// see selectFormatDetectionCapableInput()'s own doc comment for the exact
// precedence and the default (neither set) selector's own behaviour.
struct DeckLinkBackDeviceSelector {
    std::optional<std::size_t> index;
    std::string nameSubstring;  // substring match against modelName OR displayName; empty = unused
};

// Filters `devices` to those that support both capture and input format
// detection — DeviceInfo::supportsCapture plus, separately,
// IDeckLinkProfileAttributes::GetFlag(BMDDeckLinkSupportsInputFormatDetection)
// — the exact two-step check every one of this repository's own four
// existing DeckLink test files already duplicates as
// firstFormatDetectionCapableInput() (confirmed by direct grep this
// session; io/decklink_device.hpp's own enumerateDeckLinkDevices() itself
// needs no change, ADR-095) — then selects among that filtered list:
//
// - selector.index set: returns the filtered entry at that zero-based
//   position, or a null ComPtr if index is out of range.
// - selector.index unset, selector.nameSubstring non-empty: returns the
//   first filtered entry whose modelName or displayName contains
//   nameSubstring, or a null ComPtr if none match.
// - Neither set (a default-constructed selector): returns the first
//   filtered entry — exactly firstFormatDetectionCapableInput()'s own
//   existing behaviour at every one of today's four call sites, unchanged.
ComPtr<IDeckLinkInput> selectFormatDetectionCapableInput(
    const std::vector<DeviceInfo>& devices,
    const DeckLinkBackDeviceSelector& selector = {});

// Stats surfaced from the consumer thread — same std::atomic, relaxed-read
// convention every other stats struct in this project's own io/ files
// already uses (CaptureStats, CaptureConsumerStats): a caller may read
// these from any thread at any time; each individual counter's own value is
// always consistent, with no claim that reading more than one of them is a
// single atomic snapshot. No stream-start counter: unlike CaptureConsumer's
// own Deinterlacer-driven three-way outcome (ADR-080/081),
// unpackSourceRaster() has no state machine and no "first call is special"
// case — every successfully read frame is immediately usable.
struct DeckLinkBackSourceStats {
    std::atomic<int> framesPopped{0};     // every successful CaptureFrameRing::tryPop()
    std::atomic<int> framesProcessed{0};  // StartAccess/GetBytes/unpackSourceRaster/EndAccess all succeeded
    std::atomic<int> framesFailed{0};     // popped but QueryInterface/StartAccess/GetBytes/EndAccess failed, bad geometry, or unexpected pixel format
};

// Drains a caller-owned CaptureFrameRing (io/decklink_input.hpp, WU-20b —
// the same ring a caller-constructed CaptureSource, against a selected
// IDeckLinkInput, is already pushing retained frames into) on its own
// private consumer thread, and makes the most recently successfully
// unpacked frame's own OwnedSourceRaster available via
// currentSourceRaster() below.
//
// ring is caller-owned and must outlive this object — the exact convention
// CaptureConsumer's own constructor already documents for its own ring
// parameter (io/decklink_capture_consumer.hpp, WU-21b), mirrored here
// rather than reinvented: a caller constructs CaptureSource::create(input,
// mode, ring) and DeckLinkBackSource(ring) against the same ring, the same
// two-object/one-shared-ring shape every existing front-side capture test
// in this repository already uses.
class DeckLinkBackSource {
public:
    explicit DeckLinkBackSource(CaptureFrameRing& ring);
    ~DeckLinkBackSource();

    DeckLinkBackSource(const DeckLinkBackSource&) = delete;
    DeckLinkBackSource& operator=(const DeckLinkBackSource&) = delete;

    // Spawns the consumer thread. Safe to call at most once.
    void start();

    // Signals the consumer thread to exit its poll loop and joins it. Safe
    // to call at most once (a second call is a no-op, the same compare-
    // exchange idiom CaptureSource::stop()/CaptureConsumer::stop() already
    // use) and safe to call from the destructor if start() was never called.
    void stop();

    const DeckLinkBackSourceStats& stats() const noexcept { return m_stats; }

    // Returns a copy of the most recently successfully unpacked frame's own
    // OwnedSourceRaster, or std::nullopt if no frame has been successfully
    // processed yet. Safe to call from any thread; guarded against the
    // consumer thread's own concurrent write by m_mutex — mirrors
    // CaptureConsumer::copyLatestFrame()'s own "guarded, safe from any
    // thread" shape, returning the raster itself (by value, under the same
    // std::optional this class's own file comment already explains) rather
    // than copying into a caller-supplied out-parameter, since
    // OwnedSourceRaster (unlike a plain std::vector<uint8_t>) is what every
    // caller of this accessor actually wants back. See this header's own
    // file comment for why this return type is std::optional<OwnedSourceRaster>,
    // not the plain OwnedSourceRaster DECISIONS.md ADR-095 fixed for every
    // back-source producer's accessor, and why reconciling that difference
    // is left to WU-33c3.
    std::optional<OwnedSourceRaster> currentSourceRaster() const;

private:
    void run();  // consumer thread body
    bool processOne(ComPtr<IDeckLinkVideoInputFrame> frame);  // true: m_latest updated

    CaptureFrameRing& m_ring;

    std::thread m_thread;
    std::atomic<bool> m_stopping{false};
    bool m_started = false;

    mutable std::mutex m_mutex;
    std::optional<OwnedSourceRaster> m_latest;  // guarded by m_mutex

    DeckLinkBackSourceStats m_stats;
};

}  // namespace scatter::io
