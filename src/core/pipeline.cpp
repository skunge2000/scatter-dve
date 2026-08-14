// scatter-dve — WU-10: single-frame pipeline orchestration.
// Entry points declared in core/resolve.hpp (see that file and
// DECISIONS.md ADR-026 for why this unit has no pipeline.hpp of its own).
//
// This is architecture.md section 3's signal path, assembled end to end
// for the first time: v210 unpack -> chroma upsample -> PASS 1 (lattice
// eval, Jacobian, fragment generation and tile binning -- WU-06/07/08) ->
// PASS 2 (four-bank splat and bank-resolve, WU-09; normalise and
// composite, this unit) -> chroma downsample -> v210 pack. runFrame()
// covers PASS 1 and PASS 2 over already-4:4:4 rasters; runFrameFile() adds
// the v210/chroma stages either side of it.
#include "core/resolve.hpp"

#include "core/splat.hpp"
#include "video/chroma.hpp"
#include "video/v210.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

namespace scatter {

void runFrame(const Lattice& lattice, const SourceRaster& src,
              const PipelineParams& params, video::Raster444& dest) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragments(lattice, src, params.maxK, params.supersample,
                       params.tag, bins);

    // One tile's worth of fully-summed AccumCells, reused across every
    // tile -- same "one scratch buffer, cleared/overwritten per tile"
    // convention core/splat.hpp's own TileAccum::clear() comment
    // anticipates for WU-16's future per-worker arenas, applied here to
    // this unit's single-threaded loop instead.
    const std::size_t tilePixelsN = std::size_t(kTilePixels);
    std::vector<AccumCell> tileCells(tilePixelsN);

    for (int ty = 0; ty < bins.tilesY(); ++ty) {
        for (int tx = 0; tx < bins.tilesX(); ++tx) {
            TileAccum accum;  // zero-initialised by its own constructor
            splatTile(bins.tile(tx, ty), accum);
            sumBanks(accum, tileCells.data());

            const int originX = tx * kTileSize;
            const int originY = ty * kTileSize;
            // A tile at the destination raster's own right/bottom edge can
            // extend past destWidth/destHeight (TileBins sizes tilesX_/
            // tilesY_ by tileCount()'s ceiling division, core/binner.hpp);
            // cells beyond the raster have no destination pixel to write
            // and are simply not visited, the same "no fragment lost or
            // duplicated, nothing fabricated past the edge" discipline
            // ADR-024's off-raster-drop choice already uses one stage
            // earlier.
            const int localWidth =
                std::min(kTileSize, params.destWidth - originX);
            const int localHeight =
                std::min(kTileSize, params.destHeight - originY);

            for (int ly = 0; ly < localHeight; ++ly) {
                for (int lx = 0; lx < localWidth; ++lx) {
                    const AccumCell& cell = tileCells[std::size_t(ly) *
                                                        std::size_t(kTileSize) +
                                                        std::size_t(lx)];
                    const CompositedCell out = composite(cell, params.background);

                    const int dx = originX + lx;
                    const int dy = originY + ly;
                    const std::size_t idx = std::size_t(dy) *
                                             std::size_t(dest.width) +
                                             std::size_t(dx);
                    dest.Y[idx]  = out.Y;
                    dest.Cb[idx] = out.Cb;
                    dest.Cr[idx] = out.Cr;
                }
            }
        }
    }
}

bool runFrameFile(const Lattice& lattice, const std::string& srcPath,
                   int srcWidth, int srcHeight, const PipelineParams& params,
                   const std::string& dstPath) {
    video::Raster422 in(srcWidth, srcHeight);
    if (!video::readV210File(srcPath, srcWidth, srcHeight, in.planeY(),
                              in.planeCb(), in.planeCr())) {
        return false;
    }

    // Chroma upsample 4:2:2 -> 4:4:4 (ADR-005); luma is never touched by
    // it and passes straight through, same as tests/test_ramp_roundtrip.cpp's
    // own identity chain.
    video::Raster444 full(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), full.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples,
                           srcWidth, srcHeight,
                           full.Cb.data(), full.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples,
                           srcWidth, srcHeight,
                           full.Cr.data(), full.planeCr().strideSamples);

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = full.Y.data();
    src.cb = full.Cb.data();
    src.cr = full.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    // Chroma downsample 4:4:4 -> 4:2:2 (ADR-005) before pack; luma again
    // passes straight through.
    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(warped.Y.begin(), warped.Y.end(), out.Y.begin());
    chroma::downsampleImage(warped.Cb.data(), warped.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(warped.Cr.data(), warped.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    return video::writeV210File(dstPath, params.destWidth, params.destHeight,
                                 out.planeY(), out.planeCb(), out.planeCr());
}

}  // namespace scatter
