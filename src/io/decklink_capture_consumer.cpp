// scatter-dve — WU-21b: DeckLink capture-side pixel read.
//
// See io/decklink_capture_consumer.hpp and DECISIONS.md ADR-048 for the
// design. WU-22c (ADR-058) adds the coverageCallback plumbing below -- see
// decklink_capture_consumer.hpp's own doc comment on CoverageCallback for
// the design; this file only wires it into the constructor and processOne().

#include "io/decklink_capture_consumer.hpp"
#include "core/resolve.hpp"
#include "io/com_ptr.hpp"
#include "video/v210.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <mutex>
#include <thread>
#include <utility>

namespace scatter::io {

CaptureConsumer::CaptureConsumer(CaptureFrameRing& ring, Lattice lattice, PipelineParams params,
                                  CoverageCallback coverageCallback)
    : m_ring(ring),
      m_lattice(std::move(lattice)),
      m_params(params),
      m_dstRowBytes(scatter::v210::rowBytesMin(m_params.destWidth)),
      m_coverageCallback(std::move(coverageCallback)) {}

CaptureConsumer::~CaptureConsumer() { stop(); }

void CaptureConsumer::start() {
    if (m_started) return;
    m_started = true;
    m_thread = std::thread(&CaptureConsumer::run, this);
}

void CaptureConsumer::stop() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true))
        return;  // already stopped, or a concurrent stop() got there first

    if (m_thread.joinable()) m_thread.join();
}

bool CaptureConsumer::copyLatestFrame(std::vector<std::uint8_t>& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_latestFrame.empty()) return false;
    out = m_latestFrame;
    return true;
}

void CaptureConsumer::setLattice(Lattice lattice) {
    std::lock_guard<std::mutex> lock(m_latticeMutex);
    m_lattice = std::move(lattice);
}

void CaptureConsumer::run() {
    while (!m_stopping.load()) {
        auto item = m_ring.tryPop();
        if (!item) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        ++m_stats.framesPopped;
        if (processOne(std::move(*item)))
            ++m_stats.framesProcessed;
        else
            ++m_stats.framesFailed;
    }
}

bool CaptureConsumer::processOne(ComPtr<IDeckLinkVideoInputFrame> frame) {
    // CaptureSource (WU-20b) only ever requests bmdFormat10BitYUV, on the
    // initial EnableVideoInput() call and on every format-change restart
    // alike (io/decklink_input.hpp's own class comment, ADR-047) -- so every
    // frame reaching this ring is already known 10-bit YUV. Checked here
    // anyway, defensively, the same "ask rather than assume" caution this
    // project applies throughout its own DeckLink-touching code, not because
    // any code path upstream is expected to hand this class anything else.
    if (frame->GetPixelFormat() != bmdFormat10BitYUV) return false;

    // Trusted directly, per frame -- see this file's own header comment for
    // why this is not checked against the display mode CaptureSource::create()
    // was given.
    const int srcWidth = int(frame->GetWidth());
    const int srcHeight = int(frame->GetHeight());
    const auto srcRowBytes = std::ptrdiff_t(frame->GetRowBytes());
    if (srcWidth <= 0 || srcHeight <= 0 || srcRowBytes <= 0) return false;

    // WU-21f: a local copy of the current lattice, taken before touching the
    // capture frame's own buffer at all -- keeps the StartAccess/EndAccess
    // bracket below exactly as tight as it was before setLattice() existed,
    // not held open any longer while a few hundred KB of control vertices
    // get copied under a separate lock. A caller's own setLattice() call
    // that lands between this copy and the next frame's own simply takes
    // effect starting next frame, never retroactively (this file's own
    // header comment on setLattice()).
    Lattice latticeSnapshot;
    {
        std::lock_guard<std::mutex> lock(m_latticeMutex);
        latticeSnapshot = m_lattice;
    }

    // Same interface, same QueryInterface, as io/decklink_output.cpp's own
    // fillFrameBuffer() -- read direction instead of write (ADR-032's own
    // finding, confirmed unchanged for input by ADR-046/047/048's own
    // re-reads of the real SDK headers).
    ComPtr<IDeckLinkVideoBuffer> buffer(IID_IDeckLinkVideoBuffer, frame);
    if (!buffer) return false;

    if (buffer->StartAccess(bmdBufferAccessRead) != S_OK) return false;

    void* mem = nullptr;
    if (buffer->GetBytes(&mem) != S_OK || mem == nullptr) {
        buffer->EndAccess(bmdBufferAccessRead);
        return false;
    }

    // WU-22c (ADR-058): a per-call copy of m_params, only ever diverging from
    // it in the one field a coverage-observing caller needs --
    // callParams.weightOut -- and only when m_coverageCallback is actually
    // set. m_params itself is never mutated (it stays const, as it always
    // was); this local copy is cheap (PipelineParams is a small
    // trivially-copyable-shaped struct, ADR-044) and needed because
    // weightOut must point at a buffer sized and owned by *this* call, not
    // shared across frames the way the rest of m_params is -- runFrameBytes()
    // itself has no notion of "this call wants weightOut, that one doesn't"
    // beyond whatever pointer it is handed each time.
    PipelineParams callParams = m_params;
    std::vector<WeightAccum> coverageBuf;
    if (m_coverageCallback) {
        coverageBuf.assign(std::size_t(m_params.destWidth) * std::size_t(m_params.destHeight), WeightAccum(0));
        callParams.weightOut = coverageBuf.data();
    }

    // The mapped buffer is only guaranteed valid between StartAccess and
    // EndAccess, so this call happens here, before EndAccess below -- not
    // after, and not against a copy taken out of the bracket first. See this
    // file's own header comment for why no pool buffer is used instead.
    std::vector<std::uint8_t> dst(m_dstRowBytes * std::size_t(m_params.destHeight));
    scatter::runFrameBytes(latticeSnapshot, static_cast<const std::uint8_t*>(mem), srcRowBytes,
                            srcWidth, srcHeight, callParams, dst.data(),
                            std::ptrdiff_t(m_dstRowBytes));

    if (buffer->EndAccess(bmdBufferAccessRead) != S_OK) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latestFrame = std::move(dst);
    }

    // WU-22c (ADR-058): fired only after the frame is fully processed and
    // published above -- coverageBuf was filled by the same runFrameBytes()
    // call that produced m_latestFrame, so a callback observing this
    // coverage buffer is always looking at coverage for the same frame
    // copyLatestFrame() would hand back if called right now. Moved, not
    // copied -- see decklink_capture_consumer.hpp's own doc comment on
    // CoverageCallback for why ownership transfer, not a borrowed pointer,
    // is this hook's own shape.
    if (m_coverageCallback) m_coverageCallback(std::move(coverageBuf));

    return true;
}

}  // namespace scatter::io
