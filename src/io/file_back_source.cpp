// scatter-dve — WU-33c2a: FileBackSource. See file_back_source.hpp and
// DECISIONS.md ADR-095 for the full design account; not repeated here.

#include "io/file_back_source.hpp"

#include "video/v210.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <utility>
#include <vector>

namespace scatter::io {

std::optional<FileBackSource> FileBackSource::create(const std::string& path, int width, int height) {
    if (!scatter::v210::isSupportedWidth(width) || height <= 0) return std::nullopt;

    // Raw packed-v210 byte read — deliberately not video::readV210File()
    // (video/raster.hpp, WU-05): that function unpacks straight into
    // caller-supplied 4:2:2 planes via video::v210::unpackImage, with no way
    // to hand the raw packed bytes back to a caller. unpackSourceRaster()
    // (core/resolve.hpp/core/pipeline.cpp, WU-33c1) needs exactly those raw
    // bytes as its own precondition, not already-unpacked planes, so this is
    // a short, independent duplication of readV210File()'s own file-read
    // half (stride/total sizing, ifstream open/read/short-read check) rather
    // than a refactor of that already-tested, already-green function —
    // SESSION-PROTOCOL.md's anti-drift rule 2 ("never rename or refactor
    // across module boundaries"), read the same way ADR-092's WU-33a session
    // and ADR-094's WU-33c1 session already read it for a structurally
    // identical choice: an addition next to tested code, not a change to
    // it, carries this unit's own risk alone.
    const std::size_t stride = scatter::v210::rowBytesMin(width);
    const std::size_t total  = stride * std::size_t(height);

    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;

    std::vector<std::uint8_t> packed(total);
    in.read(reinterpret_cast<char*>(packed.data()), std::streamsize(total));
    if (std::size_t(in.gcount()) != total) return std::nullopt;

    return FileBackSource(scatter::unpackSourceRaster(packed.data(), std::ptrdiff_t(stride), width, height));
}

OwnedSourceRaster FileBackSource::currentSourceRaster() const {
    return m_frame;
}

FileBackSource::FileBackSource(OwnedSourceRaster frame) : m_frame(std::move(frame)) {}

}  // namespace scatter::io
