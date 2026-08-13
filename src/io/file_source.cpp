// scatter-dve — WU-05: raw .v210 file source
//
// See video/raster.hpp for the declaration and the preconditions/behaviour
// contract. This file only does the file-handling half; all sample-level
// work is v210::unpackImage, already proven at WU-02.

#include "video/raster.hpp"

#include <cstdint>
#include <fstream>
#include <ios>
#include <vector>

namespace scatter::video {

bool readV210File(const std::string& path, int width, int height,
                   Plane Y, Plane Cb, Plane Cr) {
    if (!v210::isSupportedWidth(width) || height <= 0) return false;

    const std::size_t stride = v210::rowBytesMin(width);
    const std::size_t total  = stride * std::size_t(height);

    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    std::vector<std::uint8_t> packed(total);
    in.read(reinterpret_cast<char*>(packed.data()), std::streamsize(total));
    if (std::size_t(in.gcount()) != total) return false;

    v210::unpackImage(packed.data(), std::ptrdiff_t(stride), width, height,
                       Y.data, Y.strideSamples,
                       Cb.data, Cr.data, Cb.strideSamples);
    return true;
}

}  // namespace scatter::video
