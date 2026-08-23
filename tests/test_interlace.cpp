// scatter-dve — WU-23a tests: field split and interleave (video/interlace.hpp)
//
// Three claims, each checked directly rather than only inferred from the
// others: fieldRowCount() accounts for every row of a frame exactly once
// (no row lost or double-counted, core/binner.hpp's BinStats-style
// discipline applied here); extractField() reproduces each field's own
// source rows bit-exactly, for both parities; interleaveFields() is
// extractField()'s own exact inverse, for both an even and an odd frame
// height (this project's own real formats -- 576i, 1080i -- are always
// even, but fieldRowCount()'s own odd-height behaviour is documented, so it
// is tested here, not merely assumed).
//
// Deliberately not exercised here: any lattice/warp involvement at all --
// this unit is field mode's own data-layout half only, not yet wired into
// runFrame()/the lattice (see video/interlace.hpp's own file comment and
// WORK-UNITS.md's WU-23a2).

#include <cstddef>
#include <cstdint>

#include "harness.hpp"
#include "video/interlace.hpp"
#include "video/raster.hpp"

using scatter::Sample;
namespace video = scatter::video;

namespace {

// Every row (and, within a row, every plane) gets a distinct value, so a
// row landing in the wrong place, or one plane's row being copied from a
// different row than the other two, both show up as a mismatch rather than
// coincidentally matching.
video::Raster444 makeMarkedFrame(int width, int height) {
    video::Raster444 f(width, height);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx = std::size_t(y) * std::size_t(width) + std::size_t(x);
            const int base = y * 1000 + x;
            f.Y[idx]  = Sample(base);
            f.Cb[idx] = Sample(base + 1);
            f.Cr[idx] = Sample(base + 2);
        }
    }
    return f;
}

// ---------------------------------------------------------------------------
// 1. fieldRowCount() accounts for every row exactly once.
// ---------------------------------------------------------------------------

void testFieldRowCount() {
    // Even frame height: both fields equal.
    CHECK(video::fieldRowCount(8, video::FieldParity::Top) == 4);
    CHECK(video::fieldRowCount(8, video::FieldParity::Bottom) == 4);

    // Odd frame height: Top carries the extra row.
    CHECK(video::fieldRowCount(7, video::FieldParity::Top) == 4);
    CHECK(video::fieldRowCount(7, video::FieldParity::Bottom) == 3);

    // Every height this test exercises has its rows accounted for exactly
    // once between the two parities -- no row lost, none double-counted --
    // including this project's own two real geometries (576, 1080).
    for (int h : {2, 3, 7, 8, 16, 17, 576, 1080}) {
        const int top = video::fieldRowCount(h, video::FieldParity::Top);
        const int bottom = video::fieldRowCount(h, video::FieldParity::Bottom);
        CHECK_ONCE(top + bottom == h);
        CHECK_ONCE(top == bottom || top == bottom + 1);
    }
}

// ---------------------------------------------------------------------------
// 2. extractField() reproduces each field's own source rows bit-exactly.
// ---------------------------------------------------------------------------

void testExtractFieldMatchesSourceRows(int width, int height) {
    const video::Raster444 frame = makeMarkedFrame(width, height);

    for (video::FieldParity parity :
         {video::FieldParity::Top, video::FieldParity::Bottom}) {
        const int rows = video::fieldRowCount(height, parity);
        video::Raster444 field(width, rows);
        video::extractField(frame, parity, field);

        const int rowOffset = (parity == video::FieldParity::Bottom) ? 1 : 0;
        for (int fy = 0; fy < rows; ++fy) {
            const int fr = 2 * fy + rowOffset;
            for (int x = 0; x < width; ++x) {
                const std::size_t srcIdx =
                    std::size_t(fr) * std::size_t(width) + std::size_t(x);
                const std::size_t dstIdx =
                    std::size_t(fy) * std::size_t(width) + std::size_t(x);
                CHECK_ONCE(field.Y[dstIdx]  == frame.Y[srcIdx]);
                CHECK_ONCE(field.Cb[dstIdx] == frame.Cb[srcIdx]);
                CHECK_ONCE(field.Cr[dstIdx] == frame.Cr[srcIdx]);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// 3. interleaveFields() is extractField()'s own exact inverse: split then
//    recombine reproduces the original frame bit-for-bit, every plane,
//    every row -- this unit's own first Accept: criterion.
// ---------------------------------------------------------------------------

void testInterleaveIsInverseOfExtract(int width, int height) {
    const video::Raster444 frame = makeMarkedFrame(width, height);

    video::Raster444 top(width, video::fieldRowCount(height, video::FieldParity::Top));
    video::Raster444 bottom(width, video::fieldRowCount(height, video::FieldParity::Bottom));
    video::extractField(frame, video::FieldParity::Top, top);
    video::extractField(frame, video::FieldParity::Bottom, bottom);

    video::Raster444 dest(width, height);
    video::interleaveFields(top, bottom, dest);

    CHECK(dest.Y  == frame.Y);
    CHECK(dest.Cb == frame.Cb);
    CHECK(dest.Cr == frame.Cr);
}

}  // namespace

int main() {
    testFieldRowCount();

    testExtractFieldMatchesSourceRows(6, 8);  // even frame height
    testExtractFieldMatchesSourceRows(6, 7);  // odd frame height

    testInterleaveIsInverseOfExtract(6, 8);       // even frame height
    testInterleaveIsInverseOfExtract(6, 7);       // odd frame height
    testInterleaveIsInverseOfExtract(720, 576);   // this project's own 576i geometry
    testInterleaveIsInverseOfExtract(1920, 1080); // this project's own 1080i geometry

    return scatter::test::summary("test_interlace");
}
