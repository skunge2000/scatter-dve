// scatter-dve — WU-22b: coverage_view_demo, a hand-run tool (NOT a test).
//
// Same "first executable that is not a test" shape make_testpat.cpp already
// established (CMakeLists.txt: add_executable only, never add_test()) --
// there is no automatable pass/fail criterion for "does a GUI window look
// right", and registering this as a ctest would hang any headless/
// unattended `ctest` run waiting on a keypress (Q) that could never arrive.
//
// Builds one sphere-warped destination frame with PipelineParams::weightOut
// capture enabled (WU-22a/ADR-056), then opens a scatter::diag::CoverageWindow
// (this unit, ADR-057) and displays the captured per-cell weight as
// grayscale. Sphere warp is deliberately chosen over a flat/affine lattice
// because it produces genuinely non-uniform coverage to look at -- and
// per CORRECTIONS.md C-011, the sphere's front-facing point is usually the
// *sparsest*-covered point (magnification peaks there), not the densest, so
// a viewer expecting a bright center and dark edges should instead expect
// the opposite, which is itself a useful sanity check that this is really
// reading live data and not a hand-rolled test gradient.
//
// UNVERIFIED, same as coverage_view.mm: this cannot be built or run in this
// project's own Linux cloud sandbox (no Cocoa/Metal/AppleClang toolchain at
// all). Build and run this at your own real terminal.

#include "diag/coverage_view.hpp"

#include "core/resolve.hpp"
#include "core/shapes/shapes.hpp"
#include "core/types.hpp"
#include "video/raster.hpp"

#include <cstdio>
#include <vector>

namespace {

constexpr int kDestWidth = 512;
constexpr int kDestHeight = 512;
constexpr int kSrcSize = 256;

// A flat mid-gray/mid-chroma source raster -- the sphere warp's own coverage
// pattern is what this demo is about, not the source picture content, so a
// flat field keeps the displayed window free of any source-texture detail
// that could be mistaken for a coverage artifact. Raster444's own fields
// (Y/Cb/Cr, video/raster.hpp) back the SourceRaster's non-owning pointers --
// same construction tests/test_coverage_capture.cpp already uses (plain
// std::vector<Sample> planes plus a SourceRaster pointing at their .data()),
// except here backed by a Raster444 the caller keeps alive instead of three
// bare vectors, since main() has no other need to hold them separately.
scatter::video::Raster444 buildFlatSource(int size) {
    scatter::video::Raster444 src(size, size);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const std::size_t i = std::size_t(y) * std::size_t(size) + std::size_t(x);
            src.Y[i] = scatter::fromCode10(512);
            src.Cb[i] = scatter::kChromaZero;
            src.Cr[i] = scatter::kChromaZero;
        }
    }
    return src;
}

}  // namespace

int main() {
    using namespace scatter;

    const video::Raster444 srcRaster = buildFlatSource(kSrcSize);
    SourceRaster src;
    src.width = kSrcSize;
    src.height = kSrcSize;
    src.y = srcRaster.Y.data();
    src.cb = srcRaster.Cb.data();
    src.cr = srcRaster.Cr.data();

    shapes::SphereParams sphereParams;
    sphereParams.radius = 220.0;
    sphereParams.angleSpanH = 1.2;
    sphereParams.angleSpanV = 1.2;
    sphereParams.centerX = double(kDestWidth) / 2.0;
    sphereParams.centerY = double(kDestHeight) / 2.0;
    const Lattice lattice = shapes::buildSphereLattice(sphereParams);

    std::vector<WeightAccum> weightOut(std::size_t(kDestWidth) * std::size_t(kDestHeight), 0);

    PipelineParams params;
    params.destWidth = kDestWidth;
    params.destHeight = kDestHeight;
    params.weightOut = weightOut.data();

    video::Raster444 dest(kDestWidth, kDestHeight);
    runFrame(lattice, src, params, dest);

    std::printf("coverage_view_demo: frame resolved, opening coverage window "
                "(%dx%d) -- press Q or close the window to quit\n",
                kDestWidth, kDestHeight);

    diag::CoverageWindowConfig config;
    config.width = kDestWidth;
    config.height = kDestHeight;
    config.title = "scatter-dve \xE2\x80\x94 coverage view demo (WU-22b)";

    diag::CoverageWindow window(config);
    window.updateWeights(weightOut.data(), kDestWidth, kDestHeight);
    window.run();

    std::printf("coverage_view_demo: window closed, exiting\n");
    return 0;
}
