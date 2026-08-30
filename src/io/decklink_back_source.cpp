// scatter-dve — WU-33c2b: DeckLinkBackSource / selectFormatDetectionCapableInput().
// See io/decklink_back_source.hpp and DECISIONS.md ADR-096 for the full
// design account; not repeated here.

#include "io/decklink_back_source.hpp"
#include "core/resolve.hpp"
#include "io/com_ptr.hpp"

#include "DeckLinkAPI.h"

#include <chrono>
#include <cstddef>
#include <mutex>
#include <thread>
#include <utility>

namespace scatter::io {

ComPtr<IDeckLinkInput> selectFormatDetectionCapableInput(const std::vector<DeviceInfo>& devices,
                                                           const DeckLinkBackDeviceSelector& selector) {
    // Same two-step filter every one of this repository's own four existing
    // DeckLink test files already duplicates as
    // firstFormatDetectionCapableInput() (see io/decklink_back_source.hpp's
    // own file comment) — generalized here from "return the first match"
    // to "collect every match, in order, then let selector pick."
    struct Match {
        const DeviceInfo* info;
        ComPtr<IDeckLinkInput> input;
    };
    std::vector<Match> matches;
    for (const auto& d : devices) {
        if (!d.supportsCapture) continue;

        ComPtr<IDeckLinkInput> candidate(IID_IDeckLinkInput, d.device);
        if (!candidate) continue;

        ComPtr<IDeckLinkProfileAttributes> attributes(IID_IDeckLinkProfileAttributes, d.device);
        bool supportsFormatDetection = false;
        if (attributes &&
            attributes->GetFlag(BMDDeckLinkSupportsInputFormatDetection, &supportsFormatDetection) == S_OK &&
            supportsFormatDetection) {
            matches.push_back(Match{&d, candidate});
        }
    }

    if (selector.index) {
        if (*selector.index >= matches.size()) return {};
        return matches[*selector.index].input;
    }

    if (!selector.nameSubstring.empty()) {
        for (const auto& m : matches) {
            if (m.info->modelName.find(selector.nameSubstring) != std::string::npos ||
                m.info->displayName.find(selector.nameSubstring) != std::string::npos)
                return m.input;
        }
        return {};
    }

    // Default-constructed selector: first match, exactly
    // firstFormatDetectionCapableInput()'s own existing behaviour.
    if (matches.empty()) return {};
    return matches.front().input;
}

DeckLinkBackSource::DeckLinkBackSource(CaptureFrameRing& ring) : m_ring(ring) {}

DeckLinkBackSource::~DeckLinkBackSource() { stop(); }

void DeckLinkBackSource::start() {
    if (m_started) return;
    m_started = true;
    m_thread = std::thread(&DeckLinkBackSource::run, this);
}

void DeckLinkBackSource::stop() {
    bool expected = false;
    if (!m_stopping.compare_exchange_strong(expected, true))
        return;  // already stopped, or a concurrent stop() got there first

    if (m_thread.joinable()) m_thread.join();
}

std::optional<OwnedSourceRaster> DeckLinkBackSource::currentSourceRaster() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_latest;  // std::optional copy: empty if no frame processed yet
}

void DeckLinkBackSource::run() {
    while (!m_stopping.load()) {
        auto item = m_ring.tryPop();
        if (!item) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }

        ++m_stats.framesPopped;
        if (processOne(std::move(*item))) {
            ++m_stats.framesProcessed;
        } else {
            ++m_stats.framesFailed;
        }
    }
}

bool DeckLinkBackSource::processOne(ComPtr<IDeckLinkVideoInputFrame> frame) {
    // CaptureSource (WU-20b) only ever requests bmdFormat10BitYUV — checked
    // here anyway, the same defensive "ask rather than assume" convention
    // io/decklink_capture_consumer.cpp's own processOne() already applies
    // for the identical reason.
    if (frame->GetPixelFormat() != bmdFormat10BitYUV) return false;

    // Trusted directly, per frame — see io/decklink_back_source.hpp's own
    // file comment for why this class needs no fixed destination geometry
    // of its own to check these against.
    const int srcWidth = int(frame->GetWidth());
    const int srcHeight = int(frame->GetHeight());
    const auto srcRowBytes = std::ptrdiff_t(frame->GetRowBytes());
    if (srcWidth <= 0 || srcHeight <= 0 || srcRowBytes <= 0) return false;

    // Same interface, same QueryInterface, as
    // io/decklink_capture_consumer.cpp's own processOne() (ADR-032's own
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

    // Still inside the StartAccess/EndAccess bracket — the mapped buffer is
    // only guaranteed valid between the two calls, the same requirement
    // io/decklink_capture_consumer.cpp's own processOne() already honours.
    // unpackSourceRaster() (WU-33c1, core/resolve.hpp/core/pipeline.cpp) is
    // synchronous and retains nothing past its own call, so calling it
    // directly against the mapped pointer, before EndAccess, needs no extra
    // pool buffer or copy. No deinterlace step — see
    // io/decklink_back_source.hpp's own file comment for why that is this
    // unit's own deliberate, named scope limit, not an oversight.
    OwnedSourceRaster raster = scatter::unpackSourceRaster(
        static_cast<const std::uint8_t*>(mem), srcRowBytes, srcWidth, srcHeight);

    if (buffer->EndAccess(bmdBufferAccessRead) != S_OK) return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_latest = std::move(raster);
    }
    return true;
}

}  // namespace scatter::io
