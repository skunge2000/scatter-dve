// scatter-dve — WU-23a: field split and interleave. See video/interlace.hpp
// for the design note and for what this unit deliberately does not yet do
// (drive the lattice/warp pipeline per field -- WU-23a2).
#include "video/interlace.hpp"

#include <algorithm>
#include <cstddef>

namespace scatter::video {

namespace {

// One field's own row fy of a frame is frame row (2*fy + rowOffset).
// Shared by extractField() and interleaveFields() so the row-index
// arithmetic exists in exactly one place.
constexpr int frameRowFor(int fy, int rowOffset) noexcept {
    return 2 * fy + rowOffset;
}

void copyRow(const Sample* src, Sample* dst, int width) noexcept {
    std::copy_n(src, std::size_t(width), dst);
}

}  // namespace

void extractField(const Raster444& frame, FieldParity parity, Raster444& outField) {
    const int rowOffset = (parity == FieldParity::Bottom) ? 1 : 0;
    const int rows = fieldRowCount(frame.height, parity);
    const int width = frame.width;

    for (int fy = 0; fy < rows; ++fy) {
        const int fr = frameRowFor(fy, rowOffset);
        const std::size_t srcBase = std::size_t(fr) * std::size_t(width);
        const std::size_t dstBase = std::size_t(fy) * std::size_t(width);
        copyRow(frame.Y.data()  + srcBase, outField.Y.data()  + dstBase, width);
        copyRow(frame.Cb.data() + srcBase, outField.Cb.data() + dstBase, width);
        copyRow(frame.Cr.data() + srcBase, outField.Cr.data() + dstBase, width);
    }
}

void interleaveFields(const Raster444& topField, const Raster444& bottomField,
                       Raster444& dest) {
    const int width = dest.width;

    for (int fy = 0; fy < topField.height; ++fy) {
        const int dr = frameRowFor(fy, 0);
        const std::size_t srcBase = std::size_t(fy) * std::size_t(width);
        const std::size_t dstBase = std::size_t(dr) * std::size_t(width);
        copyRow(topField.Y.data()  + srcBase, dest.Y.data()  + dstBase, width);
        copyRow(topField.Cb.data() + srcBase, dest.Cb.data() + dstBase, width);
        copyRow(topField.Cr.data() + srcBase, dest.Cr.data() + dstBase, width);
    }

    for (int fy = 0; fy < bottomField.height; ++fy) {
        const int dr = frameRowFor(fy, 1);
        const std::size_t srcBase = std::size_t(fy) * std::size_t(width);
        const std::size_t dstBase = std::size_t(dr) * std::size_t(width);
        copyRow(bottomField.Y.data()  + srcBase, dest.Y.data()  + dstBase, width);
        copyRow(bottomField.Cb.data() + srcBase, dest.Cb.data() + dstBase, width);
        copyRow(bottomField.Cr.data() + srcBase, dest.Cr.data() + dstBase, width);
    }
}

}  // namespace scatter::video
