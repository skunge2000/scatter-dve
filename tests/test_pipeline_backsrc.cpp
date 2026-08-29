// scatter-dve — WU-33b tests: PipelineParams::backSrc (core/resolve.hpp;
// DECISIONS.md ADR-093) -- a real runFrame()/runFrameField() caller for
// WU-33a's own core/binner.hpp six-entry-point backSrc parameter
// (ADR-092).
//
// This unit's own job is wiring, not facing-selection math: the actual
// per-sample raster choice (front-facing reads src, back-facing reads
// *backSrc, both keyed off the same surfaceNormal(rawJ).z sign
// generateFragmentsTagByFacing()'s own tag choice already uses, and
// applied unconditionally inside generateFragmentsRowRangeImpl() --
// core/binner.cpp -- independent of tag differentiation) is already
// checked by tests/test_binner.cpp's own
// test_back_src_front_and_back_facing_sample_different_rasters(). What
// that file's own tests do not exercise is whether a real
// PipelineParams::backSrc actually reaches whichever of core/binner.hpp's
// six generateFragments*() entry points runFrame()/runFrameField() call --
// that is what the tests below check, each against an independent
// recomputation through the same public primitives
// (generateFragments()/generateFragmentsFieldRows(), splatTile(),
// sumBanks(), composite() -- core/binner.hpp, core/splat.hpp,
// core/resolve.hpp), never calling core/pipeline.cpp's own private
// resolveOneTile()/runThreaded() -- the same "independent recomputation
// through public primitives, not a copy of the private one" shape
// tests/test_pipeline_shading.cpp's own resolveFrameIndependently()/
// resolveParityIndependently() already use for lightingScene's own,
// structurally identical wiring question.
//
// No shared flat-lattice/self-fold-sphere/uniform-source test helper
// exists to reuse -- tests/test_binner.cpp, test_pipeline_shading.cpp and
// this file each duplicate their own locally (SESSION-PROTOCOL.md rule 2).
#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "core/splat.hpp"
#include "harness.hpp"
#include "video/interlace.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace scatter;

namespace {

constexpr double kPi = 3.14159265358979323846;

// Same technique as tests/test_pipeline_shading.cpp's own
// makeFlatAffineLattice() -- duplicated locally per SESSION-PROTOCOL.md
// rule 2. Flat (z == 0 everywhere): used only by the two "default nullptr
// preserves output" tests below, which need no genuine front/back facing
// split at all -- a byte-for-byte-unchanged check doesn't care what the
// facing sign happens to be, only that backSrc's absence changes nothing.
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

// Same self-folding sphere lattice as tests/test_binner.cpp's own
// test_back_src_front_and_back_facing_sample_different_rasters() --
// angleSpanH == 2*pi wraps the source raster all the way round a sphere,
// so both the front-facing near side and the back-facing far side of the
// fold land somewhere in a 700x700 destination raster at these same
// centerX/centerY/radius (proven directly by that test); reused here
// unchanged rather than re-derived, since this file's own job is only
// whether PipelineParams::backSrc reaches this already-proven mechanism,
// not whether the mechanism itself is correct.
//
// Deliberately paired below with a denser source raster than that test's
// own 3x3, though: that test only ever decodes individual Frag colours,
// so a single back-facing fragment landing right at the fold's own
// silhouette boundary -- where density-compensation weight collapses
// towards zero, this codebase's own splat-weight convention for a near-
// singular Jacobian -- is already enough to prove the point at the
// fragment level. A pipeline-level test needs the *composited* pixels to
// visibly differ post-splat/resolve, which a single, near-zero-weight
// back-facing fragment does not guarantee (checked directly this session:
// the 3x3 source used at test_binner.cpp's own fixture size produces
// zero differing destination pixels end to end, this file's own first
// draft of this test having caught exactly that). A 20x20 source instead
// puts enough distinct back-facing samples on the sphere's own visible
// far side, away from the razor's-edge fold boundary alone, that their
// combined splat weight is not negligible.
Lattice makeSelfFoldSphereLattice() {
    using scatter::shapes::SphereParams;
    using scatter::shapes::buildSphereLattice;
    SphereParams p;
    p.angleSpanH = 2.0 * kPi;
    p.centerX = 300.0;
    p.centerY = 300.0;
    return buildSphereLattice(p);
}

// A small, uniform, non-degenerate RGB source raster -- same shape as
// tests/test_pipeline_shading.cpp's own UniformSource, duplicated locally
// (SESSION-PROTOCOL.md rule 2). Avoids bilinear interpolation entirely and
// isolates the wiring this file's own tests care about: a solid front
// colour and a solid, distinct back colour make "did this pixel's colour
// come from src or from *backSrc" visible directly in the final
// composited output, with no fragment-level decoding needed.
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
// either of those -- the same shape tests/test_pipeline_shading.cpp's own
// resolveFrameIndependently() already uses for shadingGrid; backSrc is
// threaded straight into generateFragments() here instead, exactly the
// parameter runFrame()'s own threads<=1 branch (core/pipeline.cpp) passes
// it to. No frontTag/backTag differentiation in any test below (both left
// at their shared default, 0) -- deliberately: backSrc's own selection
// (core/binner.cpp's generateFragmentsRowRangeImpl()) is unconditional,
// independent of tag differentiation (ADR-092), so exercising it through
// the plain, more commonly used generateFragments() is exactly as strong a
// check of this unit's own wiring and additionally covers the branch every
// caller not opting into k-buffer tag differentiation actually takes.
video::Raster444 resolveFrameIndependently(const Lattice& lattice, const SourceRaster& src,
                                            const PipelineParams& params,
                                            const SourceRaster* backSrc) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragments(lattice, src, params.maxK, params.supersample, params.tag, bins,
                       /*shadingGrid=*/nullptr, backSrc);

    video::Raster444 dest(params.destWidth, params.destHeight);
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
                    dest.Y[idx] = out.R;
                    dest.Cb[idx] = out.G;
                    dest.Cr[idx] = out.B;
                }
            }
        }
    }
    return dest;
}

// Field-parity counterpart of resolveFrameIndependently() above -- same
// shape as tests/test_pipeline_shading.cpp's own
// resolveParityIndependently(), with backSrc threaded into
// generateFragmentsFieldRows() instead, the way runFrameField()'s own
// resolveOneParity() (core/pipeline.cpp) does.
video::Raster444 resolveParityIndependently(const Lattice& lattice, const SourceRaster& src,
                                             const PipelineParams& params, int rowOffset,
                                             const SourceRaster* backSrc) {
    TileBins bins(params.destWidth, params.destHeight);
    generateFragmentsFieldRows(lattice, src, params.maxK, params.supersample, params.tag,
                                rowOffset, bins, /*shadingGrid=*/nullptr, backSrc);

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
// 1. Default (backSrc == nullptr) is byte-for-byte unaffected.
// ---------------------------------------------------------------------------

static void test_pipeline_backsrc_default_null_preserves_runframe_output() {
    const int W = 12, H = 9;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(0.7, 0.7, 3.0, 2.0, W, H);

    PipelineParams implicitParams;
    implicitParams.destWidth = W;
    implicitParams.destHeight = H;
    implicitParams.maxK = 1000.0;

    PipelineParams explicitParams = implicitParams;
    explicitParams.backSrc = nullptr;  // spelled out, same as the default

    video::Raster444 implicitDest(W, H);
    video::Raster444 explicitDest(W, H);
    runFrame(lat, src.view(), implicitParams, implicitDest);
    runFrame(lat, src.view(), explicitParams, explicitDest);

    CHECK(implicitDest.Y == explicitDest.Y);
    CHECK(implicitDest.Cb == explicitDest.Cb);
    CHECK(implicitDest.Cr == explicitDest.Cr);
}

static void test_pipeline_backsrc_default_null_preserves_runframefield_output() {
    const int W = 12, H = 8;
    UniformSource src(W, H, Sample(20000), Sample(40000), Sample(25000));
    const Lattice lat = makeFlatAffineLattice(0.7, 0.7, 3.0, 2.0, W, H);

    PipelineParams implicitParams;
    implicitParams.destWidth = W;
    implicitParams.destHeight = H;
    implicitParams.maxK = 1000.0;

    PipelineParams explicitParams = implicitParams;
    explicitParams.backSrc = nullptr;

    video::Raster444 implicitDest(W, H);
    video::Raster444 explicitDest(W, H);
    runFrameField(lat, src.view(), implicitParams, implicitDest);
    runFrameField(lat, src.view(), explicitParams, explicitDest);

    CHECK(implicitDest.Y == explicitDest.Y);
    CHECK(implicitDest.Cb == explicitDest.Cb);
    CHECK(implicitDest.Cr == explicitDest.Cr);
}

// ---------------------------------------------------------------------------
// 2. A real backSrc produces genuinely different output on a self-folding
//    surface, checked against an independent recomputation.
// ---------------------------------------------------------------------------

static void test_pipeline_backsrc_runframe_selects_back_raster_on_self_fold() {
    const int W = 20, H = 20;
    UniformSource front(W, H, Sample(20000), Sample(5000), Sample(5000));  // reddish
    UniformSource back(W, H, Sample(5000), Sample(5000), Sample(20000));   // bluish
    const SourceRaster backView = back.view();
    const Lattice lat = makeSelfFoldSphereLattice();

    PipelineParams params;
    params.destWidth = 700;
    params.destHeight = 700;
    params.maxK = 1.0e6;
    params.backSrc = &backView;

    video::Raster444 dest(params.destWidth, params.destHeight);
    runFrame(lat, front.view(), params, dest);

    const video::Raster444 expected =
        resolveFrameIndependently(lat, front.view(), params, &backView);
    CHECK(dest.Y == expected.Y);
    CHECK(dest.Cb == expected.Cb);
    CHECK(dest.Cr == expected.Cr);

    // Non-vacuous: a runFrame() that silently dropped params.backSrc on the
    // floor (backSrc always nullptr internally) could still pass the two
    // CHECKs above by producing the same output an independent
    // recomputation with backSrc == nullptr would also produce, since both
    // sides would be making the identical mistake.
    const video::Raster444 frontOnly =
        resolveFrameIndependently(lat, front.view(), params, nullptr);
    CHECK(dest.Y != frontOnly.Y);
}

static void test_pipeline_backsrc_runframefield_selects_back_raster_on_self_fold() {
    const int W = 20, H = 20;
    UniformSource front(W, H, Sample(20000), Sample(5000), Sample(5000));
    UniformSource back(W, H, Sample(5000), Sample(5000), Sample(20000));
    const SourceRaster backView = back.view();
    const Lattice lat = makeSelfFoldSphereLattice();

    PipelineParams params;
    params.destWidth = 700;
    params.destHeight = 700;
    params.maxK = 1.0e6;
    params.backSrc = &backView;

    video::Raster444 dest(params.destWidth, params.destHeight);
    runFrameField(lat, front.view(), params, dest);

    const video::Raster444 topFullRef =
        resolveParityIndependently(lat, front.view(), params, 0, &backView);
    const video::Raster444 bottomFullRef =
        resolveParityIndependently(lat, front.view(), params, 1, &backView);

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

    const video::Raster444 topFrontOnly =
        resolveParityIndependently(lat, front.view(), params, 0, nullptr);
    CHECK(topFullRef.Y != topFrontOnly.Y);
}

// ---------------------------------------------------------------------------
// 3. Threaded path: backSrc reaches every row-band worker through
//    runThreaded()'s new trailing parameter, byte-identical to the
//    threads<=1 oracle (I6/ADR-015, the same determinism property every
//    other PipelineParams field already preserves) -- the direct check of
//    this unit's own runThreaded() wiring.
// ---------------------------------------------------------------------------

static void test_pipeline_backsrc_threaded_matches_single_threaded() {
    const int W = 20, H = 20;
    UniformSource front(W, H, Sample(18000), Sample(30000), Sample(5000));
    UniformSource back(W, H, Sample(5000), Sample(10000), Sample(28000));
    const SourceRaster backView = back.view();
    const Lattice lat = makeSelfFoldSphereLattice();

    PipelineParams params;
    params.destWidth = 700;
    params.destHeight = 700;
    params.maxK = 1.0e6;
    params.backSrc = &backView;

    PipelineParams threadedParams = params;
    threadedParams.threads = 4;

    video::Raster444 singleThreaded(params.destWidth, params.destHeight);
    video::Raster444 threaded(params.destWidth, params.destHeight);
    runFrame(lat, front.view(), params, singleThreaded);
    runFrame(lat, front.view(), threadedParams, threaded);

    CHECK(singleThreaded.Y == threaded.Y);
    CHECK(singleThreaded.Cb == threaded.Cb);
    CHECK(singleThreaded.Cr == threaded.Cr);
}

int main() {
    test_pipeline_backsrc_default_null_preserves_runframe_output();
    test_pipeline_backsrc_default_null_preserves_runframefield_output();
    test_pipeline_backsrc_runframe_selects_back_raster_on_self_fold();
    test_pipeline_backsrc_runframefield_selects_back_raster_on_self_fold();
    test_pipeline_backsrc_threaded_matches_single_threaded();

    return scatter::test::summary("test_pipeline_backsrc");
}
