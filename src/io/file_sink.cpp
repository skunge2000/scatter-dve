// scatter-dve — WU-05: raw .v210 file sink
//
// See video/raster.hpp for the declaration and the preconditions/behaviour
// contract. This file only does the file-handling half; all sample-level
// work is v210::packImage, already proven at WU-02. Mirrors
// tools/testpat.hpp's writeV210 (ADR-019's tool/test-only helper), but takes
// plane descriptors rather than that header's own Frame type, so any
// translation unit can call it, not just make_testpat and its test.

#include "video/raster.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <vector>

namespace scatter::video {

bool writeV210File(const std::string& path, int width, int height,
                    ConstPlane Y, ConstPlane Cb, ConstPlane Cr) {
    if (!v210::isSupportedWidth(width) || height <= 0) return false;

    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(stride * std::size_t(height));

    v210::packImage(Y.data, Y.strideSamples,
                     Cb.data, Cr.data, Cb.strideSamples,
                     width, height,
                     packed.data(), std::ptrdiff_t(stride));

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(packed.data()),
              std::streamsize(packed.size()));
    return bool(out);
}

}  // namespace scatter::video
