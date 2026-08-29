// scatter-dve — WU-34c tests: PipelineParams::lightingScene/shadingConfig
// (core/resolve.hpp; DECISIONS.md ADR-091) -- a real runFrame()/
// runFrameField() caller for WU-34a/34b's own CoarseShadingGrid
// (core/coarse_shading.hpp, ADR-083/084).
//
// This unit's own job is wiring, not shading math: CoarseShadingGrid's own
// per-vertex evaluation (filtering ladder, grid shift) is already checked
// by tests/test_coarse_shading.cpp, and the per-fragment RGB multiply
// (applyShading(), core/binner.cpp) is already checked by
// tests/test_binner.cpp's own test_shading_multiplies_rgb_intensity_
// ahead_of_frag_construction(). What neither of those exercises is whether
// a `PipelineParams::lightingScene` actually reaches a CoarseShadingGrid
// built from the right (lattice, scene, config) and gets threaded through
// to whichever of core/binner.hpp's six generateFragments*() entry points
// runFrame()/runFrameField() call -- that is what the tests below check,
// each against an independent recomputation through the same public
// primitives (CoarseShadingGrid::build(), generateFragments()/
// generateFragmentsFieldRows(), splatTile(), sumBanks(), composite() --
// core/coarse_shading.hpp, core/binner.hpp, core/splat.hpp,
// core/resolve.hpp), never calling core/pipeline.cpp's own private
// resolveOneTile()/runThreaded() -- the same "independent recomputation
// through public primitives, not a copy of the private one" shape
// tests/test_field_pipeline.cpp's own resolveParityIndependently() and
// tests/test_coverage_capture.cpp's own
// test_capture_matches_independent_recomputation() already use.
//
// No shared identity-lattice/affine-lattice/parallel-light-scene test
// helper exists to reuse -- tests/test_binner.cpp, test_coarse_shading.cpp
// and test_field_pipeline.cpp each duplicate their own locally
// (SESSION-PROTOCOL.md rule 2); this file does the same.
#include "core/coarse_shading.hpp"
#include "core/lighting.hpp"
#include "core/resolve.hpp"
#include "core/splat.hpp"
#include "harness.hpp"
#include "video/interlace.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;

namespace {

// Same technique as tests/test_binner.cpp's own makePixelAffineLattice() /
// tests/test_field_pipeline.cpp's own makeAffineLattice() -- duplicated
// locally per SESSION-PROTOCOL.md rule 2. Flat (z == 0 everywhere): a
// constant facet normal across the whole coarse grid, the same choice
// test_binner.cpp's own shading test makes, so a Parallel light's
// CoarseShadingGrid::sample() returns one unambiguous value I everywhere,
// regardless of which coarse-grid vertex a given source pixel lands
// nearest -- this test's own ground truth does not depend on the facet-
// normal/filtering-ladder math CoarseShadingGrid's own tests already cover.
Lattice makeFlatAffineLattice(double scaleX, double scaleY, double offX,
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

// Same fixture scene tests/test_binner.cpp's own mirrorParallelLightScene()
// / tests/test_coarse_shading.cpp's own oneParallelLightScene() use
// (redefined here -- each lives in a different translation unit's own
// anonymous namespace, SESSION-PROTOCOL.md rule 2).
LightingScene parallelLightScene() {
    LightingScene s;
    s.Ia = 0.0;
    s.Ka = 1.0;
    Light light;
    light.type = LightType::Parallel;
    light.direction = Vec3{0.3, 0.2, 0.9};
    light.intensity = 2.0;
    light.Kd = 1.0;
    light.Ks = 0.0;
    s.lights.push_back(light);
    return s;
}

// A small, uniform, non-degenerate RGB source raster -- avoids bilinear
// interpolation entirely and isolates the wiring this file's own tests
// care about, the same reason test_binner.cpp's own shading test picks a
// uniform source colour.
struct UniformSource {
    UniformSource(int w, int h, Sample r, Sample g, Sample b)
        : width(w), height(h),
          rPlane(std::size_t(w) * std::size_t(h), r),
          gPlane(std::size_t(w) * std::size_t(h), g),
          bPlane(std::size_t(w) * std::size_t(h), b) {}

    SourceRaster view() const noexcept {
        SourceRaster src;
        src.width = width;
        src.height = height;
        src.r = rPlane.data();
        src.g = gPlane.data();
        src.b = bPlane.data();
        return src;
    }

    int width, height;
    std::vector<Sample> rPlane, gPlane, bPlane;
};

// Independent recomputation of runFrame()'s own whole-frame PASS 1 + PASS 2
// resolve, through the same public primitives core/pipeline.cpp's own
// (private) resolveOneTile()/runFrame() use internally, but never calling
// either of those -- the same shape tests/test_field_pipeline.cpp's own
// resolveParityIndependently() already uses for runFrameField(). shadingGrid
// is threaded straight into generateFragments(), exactly the parameter
// runFrame()'s own threads<=1 branch (core/pipeline.cpp) passes it to --
// this helper's whole point is to be a second, independent path to the same
// answer, not a shortcut through runFrame() itself.
video::Raster444 resolveFrameIndependently(const Lattice& lattice, const SourceRaster& src,
                                            const PipelineParams& params,
                                            const CoarseShadingGrid* shadingGrid) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragments(lattice, src, params.maxK, params.supersample, params.tag, bins,
                       shadingGrid);

    video::Raster444 dest(params.destWidth, params.destHeight);
    const int tilesX = tileCount(params.destWidth);
    const int tilesY = tileCount(params.destHeight);
    // Bound to a named std::size_t first: `std::vector<T> v(std::size_t(
    // kTilePixels))` with a plain identifier inside the inner parentheses
    // parses as a function declaration (-Wvexing-parse, -Werror under
    // ADR-017) -- the same pitfall core/pipeline.cpp's own runThreaded()
    // (numWorkersN) and tests/test_threading.cpp already document and work
    // around.
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
                    dest.Y[idx] = out.R;
                    dest.Cb[idx] = out.G;
                    dest.Cr[idx] = out.B;
                }
            }
        }
    }
    return dest;
}

// Same technique as resolveFrameIndependently() above, for one field
// parity -- the field-mode counterpart, mirroring
// tests/test_field_pipeline.cpp's own resolveParityIndependently() with
// shadingGrid threaded into generateFragmentsFieldRows() the way
// runFrameField()'s own resolveOneParity() (core/pipeline.cpp) does.
video::Raster444 resolveParityIndependently(const Lattice& lattice, const SourceRaster& src,
                                             const PipelineParams& params, int rowOffset,
                                             const CoarseShadingGrid* shadingGrid) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragmentsFieldRows(lattice, src, params.maxK, params.supersample, params.tag,
                                rowOffset, bins, shadingGrid);

    video::Raster444 full(params.destWidth, params.destHeight);
    const int tilesX = tileCount(params.destWidth);
    const int tilesY = tileCount(params.destHeight);
    // Bound to a named std::size_t first: `std::vector<T> v(std::size_t(
    // kTilePixels))` with a plain identifier inside the inner parentheses
    // parses as a function declaration (-Wvexing-parse, -Werror under
    // ADR-017) -- the same pitfall core/pipeline.cpp's own runThreaded()
    // (numWorkersN) and tests/test_threading.cpp already document and work
    // around.
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
                    full.Y[idx] = out.R;
                    full.Cb[idx] = out.G;
                    full.Cr[idx] = out.B;
                }
            }
        }
    }
    return full;
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. Default (lightingScene == nullptr) is byte-for-byte unaffected.
// ---------------------------------------------------------------------------

static void test_pipeline_shading_default_null_preserves_runframe_output() {
    const int W = 12, H = 9;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(0.7, 0.7, 3.0, 2.0, W, H);

    PipelineParams implicitParams;
    implicitParams.destWidth = W;
    implicitParams.destHeight = H;
    implicitParams.maxK = 1000.0;

    PipelineParams explicitParams = implicitParams;
    explicitParams.lightingScene = nullptr;  // spelled out, same as the default

    video::Raster444 implicitDest(W, H);
    video::Raster444 explicitDest(W, H);
    runFrame(lat, src.view(), implicitParams, implicitDest);
    runFrame(lat, src.view(), explicitParams, explicitDest);

    CHECK(implicitDest.Y == explicitDest.Y);
    CHECK(implicitDest.Cb == explicitDest.Cb);
    CHECK(implicitDest.Cr == explicitDest.Cr);
}

static void test_pipeline_shading_default_null_preserves_runframefield_output() {
    const int W = 12, H = 8;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(0.7, 0.7, 3.0, 2.0, W, H);

    PipelineParams implicitParams;
    implicitParams.destWidth = W;
    implicitParams.destHeight = H;
    implicitParams.maxK = 1000.0;

    PipelineParams explicitParams = implicitParams;
    explicitParams.lightingScene = nullptr;

    video::Raster444 implicitDest(W, H);
    video::Raster444 explicitDest(W, H);
    runFrameField(lat, src.view(), implicitParams, implicitDest);
    runFrameField(lat, src.view(), explicitParams, explicitDest);

    CHECK(implicitDest.Y == explicitDest.Y);
    CHECK(implicitDest.Cb == explicitDest.Cb);
    CHECK(implicitDest.Cr == explicitDest.Cr);
}

// ---------------------------------------------------------------------------
// 2. A real lightingScene produces genuinely shaded output, checked
//    against an independent recomputation.
// ---------------------------------------------------------------------------

static void test_pipeline_shading_runframe_lighting_scene_shades_output() {
    const int W = 10, H = 10;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(1.0, 1.0, 15.0, 15.0, W, H);

    const LightingScene scene = parallelLightScene();
    CoarseShadingConfig cfg;
    cfg.filter = ShadingFilter::Full;
    cfg.gridShift = 0;

    PipelineParams params;
    params.destWidth = W * 4;
    params.destHeight = H * 4;
    params.maxK = 1000.0;
    params.lightingScene = &scene;
    params.shadingConfig = cfg;

    video::Raster444 dest(params.destWidth, params.destHeight);
    runFrame(lat, src.view(), params, dest);

    // Independent recomputation: build the grid ourselves (the same public
    // CoarseShadingGrid::build() call runFrame() itself makes from
    // params.lightingScene/shadingConfig -- core/resolve.hpp's own doc
    // comment on that field) and drive generateFragments()/splatTile()/
    // sumBanks()/composite() directly, never calling runFrame() a second
    // time and never touching core/pipeline.cpp's own private
    // resolveOneTile().
    const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);
    const video::Raster444 expected = resolveFrameIndependently(lat, src.view(), params, &grid);

    CHECK(dest.Y == expected.Y);
    CHECK(dest.Cb == expected.Cb);
    CHECK(dest.Cr == expected.Cr);

    // And this must not degenerate to the unshaded case -- otherwise a
    // runFrame() that silently dropped params.lightingScene on the floor
    // (i.e. never built a grid at all, shadingGrid always nullptr) could
    // still pass the two CHECKs above by producing the same output the
    // independent recomputation would produce for a nullptr grid, since
    // both sides would be making the identical mistake.
    const video::Raster444 unshaded = resolveFrameIndependently(lat, src.view(), params, nullptr);
    CHECK(dest.Y != unshaded.Y);
}

static void test_pipeline_shading_runframefield_lighting_scene_shades_output() {
    const int W = 10, H = 10;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(1.0, 1.0, 15.0, 15.0, W, H);

    const LightingScene scene = parallelLightScene();
    CoarseShadingConfig cfg;
    cfg.filter = ShadingFilter::Full;
    cfg.gridShift = 0;

    PipelineParams params;
    params.destWidth = W * 4;
    params.destHeight = H * 4;
    params.maxK = 1000.0;
    params.lightingScene = &scene;
    params.shadingConfig = cfg;

    video::Raster444 dest(params.destWidth, params.destHeight);
    runFrameField(lat, src.view(), params, dest);

    const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);
    const video::Raster444 topFullRef = resolveParityIndependently(lat, src.view(), params, 0, &grid);
    const video::Raster444 bottomFullRef =
        resolveParityIndependently(lat, src.view(), params, 1, &grid);

    const int topRows = video::fieldRowCount(params.destHeight, video::FieldParity::Top);
    const int bottomRows = video::fieldRowCount(params.destHeight, video::FieldParity::Bottom);
    video::Raster444 topFieldRef(params.destWidth, topRows);
    video::Raster444 bottomFieldRef(params.destWidth, bottomRows);
    video::extractField(topFullRef, video::FieldParity::Top, topFieldRef);
    video::extractField(bottomFullRef, video::FieldParity::Bottom, bottomFieldRef);

    video::Raster444 expected(params.destWidth, params.destHeight);
    video::interleaveFields(topFieldRef, bottomFieldRef, expected);

    CHECK(dest.Y == expected.Y);
    CHECK(dest.Cb == expected.Cb);
    CHECK(dest.Cr == expected.Cr);

    const video::Raster444 topUnshaded =
        resolveParityIndependently(lat, src.view(), params, 0, nullptr);
    CHECK(topFullRef.Y != topUnshaded.Y);
}

// ---------------------------------------------------------------------------
// 3. Threaded path: one grid per call, read concurrently by every row-band
//    worker, byte-identical to the threads<=1 oracle (I6/ADR-015, the same
//    determinism property every other PipelineParams field already
//    preserves) -- the direct check of WU-34b's own threading finding
//    (ADR-084) as applied by this unit's own runThreaded() wiring.
// ---------------------------------------------------------------------------

static void test_pipeline_shading_threaded_matches_single_threaded() {
    const int W = 16, H = 16;
    UniformSource src(W, H, Sample(18000), Sample(30000), Sample(50000));
    // A genuine (non-flat) affine warp -- magnifying and off-centre, the
    // same shape tests/test_field_pipeline.cpp's own
    // test_field_mode_matches_independent_recomputation() uses -- so real
    // supersampling and multiple row bands both do genuine work, not just
    // a degenerate single-band case.
    Lattice lat;
    {
        const double su = 1.5 * double(W - 1) / double(kLatticeMax);
        const double sv = 1.5 * double(H - 1) / double(kLatticeMax);
        for (int row = 0; row < kLatticeSize; ++row) {
            for (int col = 0; col < kLatticeSize; ++col) {
                Vec3& p = lat.at(row, col);
                p.x = 4.0 + su * double(col);
                p.y = 2.0 + sv * double(row);
                p.z = 0.0;
            }
        }
    }

    const LightingScene scene = parallelLightScene();
    CoarseShadingConfig cfg;
    cfg.filter = ShadingFilter::Full;

    PipelineParams params;
    params.destWidth = 48;
    params.destHeight = 48;
    params.maxK = 1000.0;
    params.lightingScene = &scene;
    params.shadingConfig = cfg;

    PipelineParams threadedParams = params;
    threadedParams.threads = 4;

    video::Raster444 singleThreaded(params.destWidth, params.destHeight);
    video::Raster444 threaded(params.destWidth, params.destHeight);
    runFrame(lat, src.view(), params, singleThreaded);
    runFrame(lat, src.view(), threadedParams, threaded);

    CHECK(singleThreaded.Y == threaded.Y);
    CHECK(singleThreaded.Cb == threaded.Cb);
    CHECK(singleThreaded.Cr == threaded.Cr);
}

int main() {
    test_pipeline_shading_default_null_preserves_runframe_output();
    test_pipeline_shading_default_null_preserves_runframefield_output();
    test_pipeline_shading_runframe_lighting_scene_shades_output();
    test_pipeline_shading_runframefield_lighting_scene_shades_output();
    test_pipeline_shading_threaded_matches_single_threaded();

    return scatter::test::summary("test_pipeline_shading");
}
