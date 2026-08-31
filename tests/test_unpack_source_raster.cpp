// scatter-dve — WU-33c1 tests: unpackSourceRaster()/OwnedSourceRaster
// (core/resolve.hpp; DECISIONS.md ADR-094) — the v210 unpack -> chroma
// upsample -> RGB boundary conversion prefix runFrameBytes()/
// runFrameBytesDeinterlaced() already run as unreachable locals, factored
// out into a reusable, caller-owned form for WU-33c2's own second capture
// consumer (not this unit) to build a real backSrc from.
//
// Portable, zero DeckLink dependency, no new scatter-core source file
// beyond core/resolve.hpp/core/pipeline.cpp (both already build into
// scatter-core) — this file only adds a new test executable.
//
// Four checks (WU-33c6, DECISIONS.md ADR-100, added checks 3/4 below to the
// original two):
//
//   1. unpackSourceRaster()'s own output, fed by hand into runFrame() and
//      the same RGB->YCbCr/chroma-downsample/v210-pack tail
//      runFrameBytes() already runs, reproduces runFrameBytes()'s own
//      output bit-for-bit for a genuine, non-trivial warp (a zone plate,
//      the same content tests/test_pipeline_bytes.cpp's own check 1
//      already uses for the same reason: the identity map's degenerate
//      one-fragment-per-cell case would not exercise real fragment
//      generation, splat and resolve). Proves the duplicated unpack
//      sequence is honest, not merely plausible — never calling
//      runFrameBytes() itself, only its own already-independently-tested
//      building blocks (v210::unpackImage, chroma::upsampleImage,
//      chroma::ycbcrToRgbImage, runFrame(), chroma::rgbToYcbcrImage,
//      chroma::downsampleImage, v210::packImage), the same discipline
//      test_pipeline_bytes.cpp's own check 1 already uses for
//      runFrameFile() vs. runFrameBytes().
//
//   2. OwnedSourceRaster's own view() stays correct after a copy — the
//      hazard its own header comment names directly (core/resolve.hpp): a
//      hand-written r/g/b member cached at construction time would go
//      stale in a copy, since RasterRGB's std::vector members reallocate
//      at a new address on copy. Checked directly: copy an
//      OwnedSourceRaster, destroy the original, and confirm the copy's own
//      view() still reads back the values it was built with. Since WU-33c6
//      this is trivially true (the underlying video::RasterRGB is now
//      reference-counted, so destroying one owner while a copy is alive
//      frees nothing) but is kept unchanged, in intent, per WU-33c6's own
//      Accept: line — it still catches a regression to a hand-written,
//      by-value rgb member.
//
//   3. (WU-33c6) A copy's own view().r/.g/.b compare pointer-*equal*, not
//      merely value-equal, to the original's — the direct, sandbox-
//      checkable proof that copying an OwnedSourceRaster no longer
//      duplicates its underlying buffer.
//
//   4. (WU-33c6) Two independently-constructed OwnedSourceRasters (two
//      separate unpackSourceRaster() calls, not copies of each other)
//      compare pointer-*unequal* — rules out a broken implementation that
//      shares one static buffer across every instance, which would pass
//      check 3 vacuously.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/types.hpp"
#include "video/chroma.hpp"
#include "video/raster.hpp"
#include "video/v210.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <cstdint>
#include <memory>
#include <vector>

using namespace scatter;

namespace {

// Same technique as tests/test_pipeline_bytes.cpp's own makeAffineLattice()
// — duplicated locally rather than shared across test translation units
// (SESSION-PROTOCOL.md rule 2).
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

}  // namespace

// ---------------------------------------------------------------------------
// 1. unpackSourceRaster() + runFrame() + hand-written tail reproduces
//    runFrameBytes() bit-for-bit, for a genuine warp.
// ---------------------------------------------------------------------------

static void test_unpack_source_raster_matches_runframebytes_for_a_real_warp() {
    constexpr int W = 64, H = 48;

    const testpat::Frame src = testpat::makeZonePlate(W, H);

    const std::size_t srcRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(srcRowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(srcRowBytes));

    // An off-centre 0.7x compression -- genuine fragment generation, splat
    // and resolve, not the identity map's degenerate case (same reasoning
    // as test_pipeline_bytes.cpp's own check 1).
    const Lattice lattice = makeAffineLattice(0.7, 0.7, double(W) * 0.15,
                                               double(H) * 0.15, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    // The trusted, already-tested path -- unmodified by this unit.
    const std::size_t dstRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> trustedBytes(dstRowBytes * std::size_t(H));
    runFrameBytes(lattice, srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H,
                  params, trustedBytes.data(), std::ptrdiff_t(dstRowBytes));

    // This unit's own path: unpackSourceRaster() (the unit under test),
    // then runFrame() and the same RGB->YCbCr/downsample/pack tail
    // runFrameBytes() itself runs, called by hand here -- never through
    // runFrameBytes() or runFrameBytesDeinterlaced() themselves.
    const OwnedSourceRaster owned = unpackSourceRaster(
        srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H);
    CHECK(owned.view().width == W);
    CHECK(owned.view().height == H);

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, owned.view(), params, warped);

    video::Raster444 ycbcr(params.destWidth, params.destHeight);
    chroma::rgbToYcbcrImage(warped.Y.data(), warped.Cb.data(), warped.Cr.data(),
                             warped.planeY().strideSamples,
                             params.destWidth, params.destHeight,
                             ycbcr.Y.data(), ycbcr.Cb.data(), ycbcr.Cr.data(),
                             ycbcr.planeY().strideSamples);

    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(ycbcr.Y.begin(), ycbcr.Y.end(), out.Y.begin());
    chroma::downsampleImage(ycbcr.Cb.data(), ycbcr.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(ycbcr.Cr.data(), ycbcr.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    std::vector<std::uint8_t> ownBytes(dstRowBytes * std::size_t(H));
    v210::packImage(out.Y.data(), out.planeY().strideSamples,
                    out.Cb.data(), out.Cr.data(), out.planeCb().strideSamples,
                    params.destWidth, params.destHeight,
                    ownBytes.data(), std::ptrdiff_t(dstRowBytes));

    CHECK(ownBytes.size() == trustedBytes.size());
    CHECK(ownBytes == trustedBytes);
}

// ---------------------------------------------------------------------------
// 2. OwnedSourceRaster::view() stays correct after a copy (the stale-
//    pointer hazard core/resolve.hpp's own header comment names directly).
// ---------------------------------------------------------------------------

static void test_owned_source_raster_view_survives_copy_and_original_destruction() {
    // Same W/H as test 1 above -- already-proven-safe v210/zone-plate
    // dimensions (a multiple of v210's own 6-luma-sample group size), so a
    // failure here is attributable to copy semantics alone, not to an
    // unrelated edge case at an untested frame size.
    constexpr int W = 64, H = 48;

    const testpat::Frame src = testpat::makeZonePlate(W, H);
    const std::size_t srcRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(srcRowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(srcRowBytes));

    // Heap-allocate the original so it can be genuinely destroyed (not just
    // shadowed) before the copy is read -- a stack copy-then-let-the-
    // original-go-out-of-scope-later would not actually prove anything,
    // since the original's own storage might still be sitting untouched on
    // the stack even after its destructor runs.
    auto original = std::make_unique<OwnedSourceRaster>(
        unpackSourceRaster(srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H));
    const SourceRaster originalView = original->view();
    std::vector<Sample> expectedR(originalView.r, originalView.r + std::size_t(W) * std::size_t(H));
    std::vector<Sample> expectedG(originalView.g, originalView.g + std::size_t(W) * std::size_t(H));
    std::vector<Sample> expectedB(originalView.b, originalView.b + std::size_t(W) * std::size_t(H));

    // WU-33c6: no longer a deep copy -- copying an OwnedSourceRaster now
    // increments the shared video::RasterRGB's own refcount, so `copy`
    // aliases the exact same underlying buffer `original` does.
    OwnedSourceRaster copy = *original;

    original.reset();  // original's own std::shared_ptr reference is dropped
                        // here; the underlying storage survives because
                        // `copy` still holds a reference to it (WU-33c6) --
                        // this call no longer frees anything by itself.

    const SourceRaster copyView = copy.view();
    CHECK(copyView.width == W);
    CHECK(copyView.height == H);
    bool rgbMatches = true;
    for (std::size_t i = 0; i < std::size_t(W) * std::size_t(H); ++i) {
        if (copyView.r[i] != expectedR[i] || copyView.g[i] != expectedG[i] ||
            copyView.b[i] != expectedB[i]) {
            rgbMatches = false;
            break;
        }
    }
    CHECK(rgbMatches);
}

// ---------------------------------------------------------------------------
// 3. (WU-33c6, DECISIONS.md ADR-100) A copy's own view().r/.g/.b compare
//    pointer-equal to the original's -- direct, sandbox-checkable proof
//    that copying an OwnedSourceRaster no longer duplicates its underlying
//    buffer.
// ---------------------------------------------------------------------------

static void test_owned_source_raster_copy_shares_storage_pointer_equal() {
    constexpr int W = 64, H = 48;

    const testpat::Frame src = testpat::makeZonePlate(W, H);
    const std::size_t srcRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(srcRowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(srcRowBytes));

    const OwnedSourceRaster original = unpackSourceRaster(
        srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H);
    const OwnedSourceRaster copy = original;  // refcount increment, not a deep copy (WU-33c6)

    const SourceRaster originalView = original.view();
    const SourceRaster copyView = copy.view();
    CHECK(copyView.r == originalView.r);
    CHECK(copyView.g == originalView.g);
    CHECK(copyView.b == originalView.b);
}

// ---------------------------------------------------------------------------
// 4. (WU-33c6, DECISIONS.md ADR-100) Two independently-constructed
//    OwnedSourceRasters compare pointer-unequal -- rules out a broken
//    implementation that shares one static buffer across every instance,
//    which would pass check 3 above vacuously.
// ---------------------------------------------------------------------------

static void test_owned_source_raster_independent_instances_do_not_share_storage() {
    constexpr int W = 64, H = 48;

    const testpat::Frame src = testpat::makeZonePlate(W, H);
    const std::size_t srcRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(srcRowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(srcRowBytes));

    // Two separate unpackSourceRaster() calls -- not a copy of one another.
    const OwnedSourceRaster a = unpackSourceRaster(
        srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H);
    const OwnedSourceRaster b = unpackSourceRaster(
        srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H);

    const SourceRaster aView = a.view();
    const SourceRaster bView = b.view();
    CHECK(aView.r != bView.r);
    CHECK(aView.g != bView.g);
    CHECK(aView.b != bView.b);
}

int main() {
    test_unpack_source_raster_matches_runframebytes_for_a_real_warp();
    test_owned_source_raster_view_survives_copy_and_original_destruction();
    test_owned_source_raster_copy_shares_storage_pointer_equal();
    test_owned_source_raster_independent_instances_do_not_share_storage();

    return scatter::test::summary("test_unpack_source_raster");
}
