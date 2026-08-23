// scatter-dve — WU-23a2b tests: field mode's own runFrame()-level driver,
// runFrameField() (core/resolve.hpp/core/pipeline.cpp; DECISIONS.md
// ADR-077).
//
// Two claims, each checked directly rather than only inferred from the
// other, matching WU-23a2b's own WORK-UNITS.md Accept: criteria:
//
// 1. Identity round-trip — the check WU-23a's own first Accept: criterion
//    deliberately deferred to this unit (ADR-075, HANDOFF.md's Session-43
//    entry): an interlaced test frame with a per-row marker, warped
//    through field mode under the identity lattice, reproduces the
//    original frame bit-exactly once both fields' outputs are combined.
//    Unlike tests/test_zoneplate.cpp's own I7 check, this exercises
//    runFrameField() directly against an already-4:4:4 SourceRaster/
//    Raster444 pair — no v210 4:2:2 round trip anywhere in this path — so
//    all three planes (not luma alone) are expected exact, not just legal.
//
// 2. Wiring/accounting — a real, off-centre, magnifying affine warp,
//    confirming the driver's own assembly (two generateFragmentsFieldRows()
//    calls, two PASS-2 resolves, two extractField() calls, one
//    interleaveFields() call) does not silently duplicate or drop rows.
//    Checked against an independent recomputation through the same public
//    primitives (generateFragmentsFieldRows(), splatTile(), sumBanks(),
//    composite() — core/binner.hpp, core/splat.hpp, core/resolve.hpp),
//    never calling core/pipeline.cpp's own private resolveOneTile() —
//    the same "independent recomputation through public primitives, not
//    a copy of the private one" shape tests/test_coverage_capture.cpp's
//    own test_capture_matches_independent_recomputation() already used.
//
// No shared identity-lattice/affine-lattice test helper exists to reuse —
// tests/test_binner.cpp, tests/test_zoneplate.cpp and
// tests/test_coverage_capture.cpp each duplicate their own locally
// (test_zoneplate.cpp's own comment says this is deliberate,
// SESSION-PROTOCOL.md rule 2); this file does the same.
#include "core/binner.hpp"
#include "core/resolve.hpp"
#include "core/splat.hpp"
#include "harness.hpp"
#include "video/interlace.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;

namespace {

// Same technique as tests/test_coverage_capture.cpp's own
// makeAffineLattice() / tests/test_zoneplate.cpp's own — duplicated
// locally per SESSION-PROTOCOL.md rule 2. dest.x = offX + scaleX * px,
// dest.y = offY + scaleY * py; exact for any continuous (u, v) since
// Catmull-Rom reproduces an affine function of its control points exactly.
Lattice makeAffineLattice(double scaleX, double scaleY, double offX,
                           double offY, int srcWidth, int srcHeight) {
    Lattice lat;
    const double su = (srcWidth  > 1) ? scaleX * double(srcWidth  - 1) / double(kLatticeMax) : 0.0;
    const double sv = (srcHeight > 1) ? scaleY * double(srcHeight - 1) / double(kLatticeMax) : 0.0;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            Vec3& p = lat.at(row, col);
            p.x = offX + su * double(col);
            p.y = offY + sv * double(row);
            p.z = 0.0;
        }
    }
    return lat;
}

// Same marker technique as tests/test_interlace.cpp's own makeMarkedFrame()
// — duplicated locally per SESSION-PROTOCOL.md rule 2. Every row (and,
// within a row, every plane) gets a distinct value, so a row landing in the
// wrong place, or one plane's row being copied from a different row than
// the other two, both show up as a mismatch rather than coincidentally
// matching.
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

SourceRaster viewOf(const video::Raster444& r) noexcept {
    SourceRaster src;
    src.width = r.width;
    src.height = r.height;
    src.y = r.Y.data();
    src.cb = r.Cb.data();
    src.cr = r.Cr.data();
    return src;
}

// Independent recomputation of one field parity's own full-resolution PASS
// 1 + PASS 2 resolve, through the same public primitives
// core/pipeline.cpp's own (private) resolveOneTile() uses internally, but
// never calling that function itself — the same "independent
// recomputation, not a copy of the private helper" shape
// tests/test_coverage_capture.cpp's own
// test_capture_matches_independent_recomputation() already established.
video::Raster444 resolveParityIndependently(const Lattice& lattice, const SourceRaster& src,
                                             const PipelineParams& params, int rowOffset) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragmentsFieldRows(lattice, src, params.maxK, params.supersample,
                                params.tag, rowOffset, bins);

    video::Raster444 full(params.destWidth, params.destHeight);
    const int tilesX = tileCount(params.destWidth);
    const int tilesY = tileCount(params.destHeight);
    const std::size_t tilePixelsN = std::size_t(kTilePixels);
    std::vector<AccumCell> tileCells(tilePixelsN);
    TileAccum accum;

    for (int ty = 0; ty < tilesY; ++ty) {
        for (int tx = 0; tx < tilesX; ++tx) {
            accum.clear();
            splatTile(bins.tile(tx, ty), accum);
            sumBanks(accum, tileCells.data());

            const int originX = tx * kTileSize;
            const int originY = ty * kTileSize;
            const int localWidth = std::min(kTileSize, params.destWidth - originX);
            const int localHeight = std::min(kTileSize, params.destHeight - originY);

            for (int ly = 0; ly < localHeight; ++ly) {
                for (int lx = 0; lx < localWidth; ++lx) {
                    const AccumCell& cell =
                        tileCells[std::size_t(ly) * std::size_t(kTileSize) + std::size_t(lx)];
                    const CompositedCell out = composite(cell, params.background);

                    const int dx = originX + lx;
                    const int dy = originY + ly;
                    const std::size_t idx =
                        std::size_t(dy) * std::size_t(params.destWidth) + std::size_t(dx);
                    full.Y[idx] = out.Y;
                    full.Cb[idx] = out.Cb;
                    full.Cr[idx] = out.Cr;
                }
            }
        }
    }
    return full;
}

// ---------------------------------------------------------------------------
// 1. Identity round-trip.
// ---------------------------------------------------------------------------

void test_field_mode_identity_round_trip_bit_exact() {
    // Kept at or below kLatticeSize (129, core/lattice.hpp) and even in
    // height — see CORRECTIONS.md C-008 and tests/test_zoneplate.cpp's own
    // test_i7_identity_full_pipeline() header comment: a source raster
    // wider or taller than kLatticeSize samples more than one pixel per
    // lattice cell inside the Catmull-Rom edge-replication clamp's damped
    // first/last cell (ADR-022), the same already-frozen property that
    // check already routes around by the same choice of dimensions, not
    // something runFrameField()/core/pipeline.cpp can fix. Even height
    // matches this project's own real interlaced geometries (576i, 1080i);
    // fieldRowCount()'s own odd-height accounting is already checked at
    // the data-layout level (tests/test_interlace.cpp), not re-tested here.
    const int W = 16, H = 8;

    const video::Raster444 frame = makeMarkedFrame(W, H);
    const SourceRaster src = viewOf(frame);
    const Lattice identity = makeAffineLattice(1.0, 1.0, 0.0, 0.0, W, H);

    PipelineParams params;
    params.destWidth = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    video::Raster444 dest(W, H);
    runFrameField(identity, src, params, dest);

    // Unlike tests/test_zoneplate.cpp's own I7 check (which round-trips
    // through v210 4:2:2, so only luma is exact and chroma is merely
    // checked in-range), runFrameField() never leaves 4:4:4 — no chroma
    // filter anywhere in this path — so every plane is expected exact.
    CHECK(dest.Y  == frame.Y);
    CHECK(dest.Cb == frame.Cb);
    CHECK(dest.Cr == frame.Cr);
}

// ---------------------------------------------------------------------------
// 2. Wiring/accounting: a real, off-centre, magnifying affine warp.
// ---------------------------------------------------------------------------

void test_field_mode_matches_independent_recomputation() {
    const int srcSize = 64;
    const int destSize = 96;

    video::Raster444 frame(srcSize, srcSize);
    for (int yy = 0; yy < srcSize; ++yy) {
        for (int xx = 0; xx < srcSize; ++xx) {
            const std::size_t idx = std::size_t(yy) * std::size_t(srcSize) + std::size_t(xx);
            const int code = kCode10Min +
                              ((xx + yy) * (kCode10Max - kCode10Min)) / (2 * srcSize - 2);
            frame.Y[idx] = fromCode10(std::uint16_t(code));
            frame.Cb[idx] = kChromaZero;
            frame.Cr[idx] = kChromaZero;
        }
    }
    const SourceRaster src = viewOf(frame);

    // Magnifying (1.5x) and off-centre, same construction shape
    // tests/test_coverage_capture.cpp's own
    // test_capture_matches_across_thread_counts() uses and for the same
    // reason: real supersampling and tile-boundary crossings both do
    // genuine work, and the offset means some destination cells at the
    // raster's own far edge are genuinely dropped (off-raster), not just
    // covered ones exercised.
    const Lattice lat = makeAffineLattice(1.5, 1.5, 4.0, 2.0, srcSize, srcSize);

    PipelineParams params;
    params.destWidth = destSize;
    params.destHeight = destSize;
    params.maxK = 1000.0;

    video::Raster444 dest(destSize, destSize);
    runFrameField(lat, src, params, dest);

    const video::Raster444 topFullRef = resolveParityIndependently(lat, src, params, 0);
    const video::Raster444 bottomFullRef = resolveParityIndependently(lat, src, params, 1);

    const int topRows = video::fieldRowCount(destSize, video::FieldParity::Top);
    const int bottomRows = video::fieldRowCount(destSize, video::FieldParity::Bottom);
    video::Raster444 topFieldRef(destSize, topRows);
    video::Raster444 bottomFieldRef(destSize, bottomRows);
    video::extractField(topFullRef, video::FieldParity::Top, topFieldRef);
    video::extractField(bottomFullRef, video::FieldParity::Bottom, bottomFieldRef);

    video::Raster444 destRef(destSize, destSize);
    video::interleaveFields(topFieldRef, bottomFieldRef, destRef);

    CHECK(dest.Y  == destRef.Y);
    CHECK(dest.Cb == destRef.Cb);
    CHECK(dest.Cr == destRef.Cr);

    // Granular accounting check, independent of the whole-frame equality
    // above: every one of dest's own Top-parity rows equals topFieldRef's
    // matching row, and every Bottom-parity row equals bottomFieldRef's —
    // directly checking "no row lost, none duplicated, none from the
    // wrong parity" rather than only inferring it from the whole-frame
    // CHECK above (which a compensating pair of errors could in principle
    // still pass).
    for (int fy = 0; fy < topRows; ++fy) {
        const int dr = 2 * fy;
        for (int x = 0; x < destSize; ++x) {
            const std::size_t drIdx = std::size_t(dr) * std::size_t(destSize) + std::size_t(x);
            const std::size_t fIdx = std::size_t(fy) * std::size_t(destSize) + std::size_t(x);
            CHECK_ONCE(dest.Y[drIdx]  == topFieldRef.Y[fIdx]);
            CHECK_ONCE(dest.Cb[drIdx] == topFieldRef.Cb[fIdx]);
            CHECK_ONCE(dest.Cr[drIdx] == topFieldRef.Cr[fIdx]);
        }
    }
    for (int fy = 0; fy < bottomRows; ++fy) {
        const int dr = 2 * fy + 1;
        for (int x = 0; x < destSize; ++x) {
            const std::size_t drIdx = std::size_t(dr) * std::size_t(destSize) + std::size_t(x);
            const std::size_t fIdx = std::size_t(fy) * std::size_t(destSize) + std::size_t(x);
            CHECK_ONCE(dest.Y[drIdx]  == bottomFieldRef.Y[fIdx]);
            CHECK_ONCE(dest.Cb[drIdx] == bottomFieldRef.Cb[fIdx]);
            CHECK_ONCE(dest.Cr[drIdx] == bottomFieldRef.Cr[fIdx]);
        }
    }
}

}  // namespace

int main() {
    test_field_mode_identity_round_trip_bit_exact();
    test_field_mode_matches_independent_recomputation();

    return scatter::test::summary("test_field_pipeline");
}
