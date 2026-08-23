// scatter-dve — WU-21a: runFrameBytes(), the in-memory sibling of
// runFrame()/runFrameFile() (core/resolve.hpp/core/pipeline.cpp).
//
// See DECISIONS.md ADR-048 for the full design and for why this exists:
// WU-21's own first job is draining WU-20b's CaptureFrameRing and reading
// real pixel bytes out of a retained IDeckLinkVideoInputFrame for the first
// time anywhere in this project, and those bytes live in a DeckLink-owned
// buffer for the duration of one StartAccess/EndAccess bracket, never a
// file — so runFrameFile()'s own file-to-file path cannot be the route from
// a captured frame into this project's own warp pipeline. This unit builds
// and genuinely verifies the portable half of that route (ADR-048's own
// WU-21a/WU-21b split, the same "portable piece now, DeckLink-specific
// piece next" shape ADR-046 already used for WU-20a/WU-20b): no DeckLink or
// platform dependency at all, so — unlike WU-14/15a/20b — this is built AND
// run for real in this project's own Linux cloud sandbox, not merely
// reasoned through against SDK headers.
//
// Two checks:
//   1. runFrameBytes() and runFrameFile() produce byte-identical output for
//      the same lattice/source/params, checked exactly, not by inspection
//      of the two functions' own (independently written, per ADR-048)
//      bodies — a real warp (an off-centre affine scale over a zone plate,
//      the same kind of content test_zoneplate.cpp's own checks use),
//      never an identity map, so this exercises genuine fragment
//      generation, splat and resolve, not just the degenerate case.
//   2. runFrameBytes() itself satisfies I7 (identity map round-trips
//      bit-exactly) directly, the same foundational property
//      tests/test_zoneplate.cpp's own testI7Pattern() already established
//      for runFrameFile() — proving the new entry point is not merely
//      "equivalent to the old one" but independently honest against this
//      project's own foundation test.
//
// WU-23b2a (DECISIONS.md ADR-080) extends this file with
// runFrameBytesDeinterlaced()'s own checks, per WORK-UNITS.md's own
// WU-23b2a Accept line:
//
//   3. A freshly constructed video::Deinterlacer's first push leaves
//      dstBytes completely unchanged and returns false.
//   4. From the second push onward, output matches an independently
//      composed reference (unpackImage -> upsampleImage -> push() ->
//      runFrame() -> downsampleImage -> packImage, called by hand in this
//      file, never through the unit under test) — the same discipline
//      check 1 above already uses.
//   5. The output-side re-interlace no-op finding (ADR-080) is checked
//      directly: an explicit video::extractField() x2 + interleaveFields()
//      pass over runFrame()'s own warped output produces byte-identical
//      dstBytes to the no-op path core/pipeline.cpp actually ships.
//   6. Anchor-parity round trip: a synthetic weave frame's own
//      anchor-parity rows survive runFrameBytesDeinterlaced() unchanged
//      under an identity lattice — I7 does not apply directly to a
//      de-interlaced round trip (it is lossy by construction), so this
//      substitutes for it, isolating this unit's own wiring correctness
//      from WU-23b1's own already-tested reconstruction math
//      (tests/test_deinterlace.cpp).
//   7. 625i50 geometry (720x576), this project's own stay-in-SD-domain
//      scope decision (HANDOFF.md), exercised directly — not 1080i.

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/types.hpp"
#include "video/chroma.hpp"
#include "video/deinterlace.hpp"
#include "video/interlace.hpp"
#include "video/raster.hpp"
#include "video/v210.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <vector>

using namespace scatter;

namespace {

// Same technique as tests/test_zoneplate.cpp's own makeAffineLattice() /
// tests/test_binner.cpp's own makePixelAffineLattice() — duplicated
// locally rather than shared across test translation units
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

// Reads a whole file's raw bytes -- used only to compare runFrameFile()'s
// own on-disk output against runFrameBytes()'s own in-memory output
// exactly, byte for byte. Deliberately not video::readV210File(): that
// function unpacks as it reads, and this check needs the packed bytes
// themselves, unexamined.
bool readRawFile(const std::string& path, std::vector<std::uint8_t>& out) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    if (!in) return false;
    const auto size = in.tellg();
    if (size < 0) return false;
    out.resize(std::size_t(size));
    in.seekg(0);
    in.read(reinterpret_cast<char*>(out.data()), size);
    return bool(in);
}

}  // namespace

// ---------------------------------------------------------------------------
// 1. runFrameBytes() matches runFrameFile(), exactly, for a genuine warp.
// ---------------------------------------------------------------------------

static void test_runframebytes_matches_runframefile_for_a_real_warp() {
    constexpr int W = 64, H = 48;

    const testpat::Frame src = testpat::makeZonePlate(W, H);

    // Pack the same source frame's bytes twice, independently: once via
    // tools/testpat.hpp's own writeV210() (to a file, for runFrameFile()),
    // once directly via v210::packImage (to memory, for runFrameBytes()) --
    // so this check does not depend on the two ever sharing a packed byte
    // buffer.
    const std::string srcPath = "test_pipeline_bytes_warp_src.v210";
    const std::string dstPath = "test_pipeline_bytes_warp_dst.v210";
    CHECK(testpat::writeV210(src, srcPath));

    const std::size_t srcRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(srcRowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(srcRowBytes));

    // An off-centre 0.7x compression -- genuine fragment generation, splat
    // and resolve, not the identity map's degenerate one-fragment-per-cell
    // case (see part 2, below, for that check).
    const Lattice lattice = makeAffineLattice(0.7, 0.7, double(W) * 0.15,
                                               double(H) * 0.15, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    CHECK(runFrameFile(lattice, srcPath, W, H, params, dstPath));
    std::vector<std::uint8_t> fileBytes;
    CHECK(readRawFile(dstPath, fileBytes));

    const std::size_t dstRowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> memBytes(dstRowBytes * std::size_t(H));
    runFrameBytes(lattice, srcBytes.data(), std::ptrdiff_t(srcRowBytes), W, H,
                  params, memBytes.data(), std::ptrdiff_t(dstRowBytes));

    CHECK(memBytes.size() == fileBytes.size());
    CHECK(memBytes == fileBytes);
}

// ---------------------------------------------------------------------------
// 2. I7 -- runFrameBytes() itself round-trips an identity map bit-exactly.
// ---------------------------------------------------------------------------

static void test_runframebytes_identity_round_trips_exactly() {
    constexpr int W = 64, H = 32;

    // Zone plate, not a ramp: CORRECTIONS.md C-006 already established that
    // a *non-flat* chroma signal does not survive
    // chroma::upsampleImage()/downsampleImage() unchanged (the downsample
    // filter is a half-band low-pass, an anti-aliasing stage, not a
    // perfect-reconstruction pair with upsample) -- true regardless of the
    // lattice, including the identity map exercised here, so a ramp (whose
    // chroma planes are themselves ramps, per tools/testpat.hpp) is exactly
    // the wrong pattern for a byte-exact check. tools/testpat.hpp's own
    // makeZonePlate() holds chroma flat (kChromaZero) for precisely this
    // reason (its own file header); flat chroma is C-006's own "what is
    // exact" case, matching tests/test_zoneplate.cpp's own
    // testI7Pattern(..., chromaExpectedExact=true) convention for flat
    // chroma patterns.
    const testpat::Frame src = testpat::makeZonePlate(W, H);

    const std::size_t rowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(rowBytes * std::size_t(H));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                     src.cStride(), W, H, srcBytes.data(),
                     std::ptrdiff_t(rowBytes));

    const Lattice identity = makeAffineLattice(1.0, 1.0, 0.0, 0.0, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    std::vector<std::uint8_t> dstBytes(rowBytes * std::size_t(H));
    runFrameBytes(identity, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
                  params, dstBytes.data(), std::ptrdiff_t(rowBytes));

    // I7: input v210 equals output v210, byte for byte, illegal excursions
    // included. The identity map lands every fragment's full weight on
    // exactly one destination cell (no bilinear split), and luma never
    // enters a chroma filter, so this must hold exactly -- the same
    // property tests/test_zoneplate.cpp's own testI7Pattern() already
    // established for runFrameFile(), checked here directly against
    // runFrameBytes() instead of assumed to carry over.
    CHECK(dstBytes == srcBytes);
}

// ---------------------------------------------------------------------------
// WU-23b2a — runFrameBytesDeinterlaced() (DECISIONS.md ADR-080).
// ---------------------------------------------------------------------------

namespace {

// Hand-composed reference matching what runFrameBytesDeinterlaced() must do
// internally: unpackImage -> upsampleImage -> deinterlacer.push() ->
// runFrame() -> downsampleImage -> packImage. Written independently of
// core/pipeline.cpp's own body, not a copy of it — the same "independently
// composed reference, not the unit's own internals" discipline this file's
// own runFrameBytes()-vs-runFrameFile() check (above) already uses.
// `deinterlacer` is the caller's own instance, exactly as
// runFrameBytesDeinterlaced()'s own contract requires — a caller drives its
// history by feeding it the same sequence of calls this helper is fed.
bool referenceRunFrameBytesDeinterlaced(video::Deinterlacer& deinterlacer,
                                         const Lattice& lattice,
                                         const std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes,
                                         int srcWidth, int srcHeight,
                                         const PipelineParams& params,
                                         std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes) {
    video::Raster422 in(srcWidth, srcHeight);
    v210::unpackImage(srcBytes, srcRowBytes, srcWidth, srcHeight,
                       in.Y.data(), in.planeY().strideSamples,
                       in.Cb.data(), in.Cr.data(), in.planeCb().strideSamples);

    video::Raster444 weave(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), weave.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples, srcWidth, srcHeight,
                           weave.Cb.data(), weave.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples, srcWidth, srcHeight,
                           weave.Cr.data(), weave.planeCr().strideSamples);

    video::Raster444 progressive(srcWidth, srcHeight);
    if (!deinterlacer.push(weave, progressive)) return false;

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = progressive.Y.data();
    src.cb = progressive.Cb.data();
    src.cr = progressive.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(warped.Y.begin(), warped.Y.end(), out.Y.begin());
    chroma::downsampleImage(warped.Cb.data(), warped.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(warped.Cr.data(), warped.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    v210::packImage(out.Y.data(), out.planeY().strideSamples,
                     out.Cb.data(), out.Cr.data(), out.planeCb().strideSamples,
                     params.destWidth, params.destHeight,
                     dstBytes, dstRowBytes);
    return true;
}

// Same as referenceRunFrameBytesDeinterlaced() above, except the output side
// explicitly re-interlaces via video::extractField() (both parities) +
// video::interleaveFields() over runFrame()'s own warped output, instead of
// sending `warped` straight to chroma downsample. ADR-080's own proof (the
// row-index arithmetic in video/interlace.cpp's extractField()/
// interleaveFields() reproduces its input exactly when both parities are
// applied to the same source frame) predicts this produces byte-identical
// dstBytes to the no-op path core/pipeline.cpp actually ships — checked
// directly by test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace()
// below, not merely assumed from the ADR's own algebra.
bool referenceWithExplicitReinterlace(video::Deinterlacer& deinterlacer,
                                       const Lattice& lattice,
                                       const std::uint8_t* srcBytes, std::ptrdiff_t srcRowBytes,
                                       int srcWidth, int srcHeight,
                                       const PipelineParams& params,
                                       std::uint8_t* dstBytes, std::ptrdiff_t dstRowBytes) {
    video::Raster422 in(srcWidth, srcHeight);
    v210::unpackImage(srcBytes, srcRowBytes, srcWidth, srcHeight,
                       in.Y.data(), in.planeY().strideSamples,
                       in.Cb.data(), in.Cr.data(), in.planeCb().strideSamples);

    video::Raster444 weave(srcWidth, srcHeight);
    std::copy(in.Y.begin(), in.Y.end(), weave.Y.begin());
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples, srcWidth, srcHeight,
                           weave.Cb.data(), weave.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples, srcWidth, srcHeight,
                           weave.Cr.data(), weave.planeCr().strideSamples);

    video::Raster444 progressive(srcWidth, srcHeight);
    if (!deinterlacer.push(weave, progressive)) return false;

    SourceRaster src;
    src.width = srcWidth;
    src.height = srcHeight;
    src.y = progressive.Y.data();
    src.cb = progressive.Cb.data();
    src.cr = progressive.Cr.data();

    video::Raster444 warped(params.destWidth, params.destHeight);
    runFrame(lattice, src, params, warped);

    video::Raster444 topField(
        params.destWidth,
        video::fieldRowCount(params.destHeight, video::FieldParity::Top));
    video::Raster444 bottomField(
        params.destWidth,
        video::fieldRowCount(params.destHeight, video::FieldParity::Bottom));
    video::extractField(warped, video::FieldParity::Top, topField);
    video::extractField(warped, video::FieldParity::Bottom, bottomField);
    video::Raster444 recombined(params.destWidth, params.destHeight);
    video::interleaveFields(topField, bottomField, recombined);

    video::Raster422 out(params.destWidth, params.destHeight);
    std::copy(recombined.Y.begin(), recombined.Y.end(), out.Y.begin());
    chroma::downsampleImage(recombined.Cb.data(), recombined.planeCb().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cb.data(), out.planeCb().strideSamples);
    chroma::downsampleImage(recombined.Cr.data(), recombined.planeCr().strideSamples,
                             params.destWidth, params.destHeight,
                             out.Cr.data(), out.planeCr().strideSamples);

    v210::packImage(out.Y.data(), out.planeY().strideSamples,
                     out.Cb.data(), out.Cr.data(), out.planeCb().strideSamples,
                     params.destWidth, params.destHeight,
                     dstBytes, dstRowBytes);
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 3 & 4. First push is a no-op (dst untouched, returns false); from the
// second push onward, output matches referenceRunFrameBytesDeinterlaced()
// above, exactly.
// ---------------------------------------------------------------------------

static void test_deinterlaced_matches_reference_and_first_push_is_a_noop() {
    constexpr int W = 64, H = 48;
    constexpr int kNumPushes = 3;

    const testpat::Frame srcFrame = testpat::makeZonePlate(W, H);
    const std::size_t rowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(rowBytes * std::size_t(H));
    v210::packImage(srcFrame.Y.data(), srcFrame.yStride(), srcFrame.Cb.data(), srcFrame.Cr.data(),
                     srcFrame.cStride(), W, H, srcBytes.data(), std::ptrdiff_t(rowBytes));

    // Same off-centre 0.7x compression as check 1 above — genuine fragment
    // generation, splat and resolve, not the identity map's degenerate case.
    const Lattice lattice = makeAffineLattice(0.7, 0.7, double(W) * 0.15,
                                               double(H) * 0.15, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    // The exact same v210 bytes are pushed every time: this check's own job
    // is wiring correctness (does runFrameBytesDeinterlaced() call push()
    // then runFrame(), in that order, on the right frame), not a
    // re-verification of WU-23b1's own reconstruction math
    // (tests/test_deinterlace.cpp's job) — a repeated, non-degenerate weave
    // frame under a real warp already discriminates a missing or misordered
    // push(), or a skipped runFrame(), from this reference just as well as a
    // varying sequence would.
    video::Deinterlacer dUnderTest(video::FieldParity::Top, video::DeinterlaceCoefficients::Simple);
    video::Deinterlacer dReference(video::FieldParity::Top, video::DeinterlaceCoefficients::Simple);

    for (int k = 0; k < kNumPushes; ++k) {
        std::vector<std::uint8_t> dstUnderTest(rowBytes * std::size_t(H), std::uint8_t(0xAA));
        std::vector<std::uint8_t> dstReference(rowBytes * std::size_t(H), std::uint8_t(0x55));
        const std::vector<std::uint8_t> dstUnderTestBefore = dstUnderTest;

        const bool producedUnderTest = runFrameBytesDeinterlaced(
            dUnderTest, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstUnderTest.data(), std::ptrdiff_t(rowBytes));
        const bool producedReference = referenceRunFrameBytesDeinterlaced(
            dReference, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstReference.data(), std::ptrdiff_t(rowBytes));

        CHECK_ONCE(producedUnderTest == producedReference);

        if (k == 0) {
            // Accept: a freshly constructed Deinterlacer's first push
            // leaves dstBytes completely unchanged — checked byte for byte,
            // not just "the call doesn't crash".
            CHECK(!producedUnderTest);
            CHECK(dstUnderTest == dstUnderTestBefore);
            continue;
        }

        CHECK_ONCE(producedUnderTest);
        CHECK_ONCE(dstUnderTest == dstReference);
    }
}

// ---------------------------------------------------------------------------
// 5. Re-interlace no-op: the shipped path (warped straight to chroma
// downsample) matches an explicit extractField() x2 + interleaveFields()
// pass over the same warped output, byte for byte.
// ---------------------------------------------------------------------------

static void test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace() {
    constexpr int W = 64, H = 48;
    constexpr int kNumPushes = 3;

    const testpat::Frame srcFrame = testpat::makeZonePlate(W, H);
    const std::size_t rowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(rowBytes * std::size_t(H));
    v210::packImage(srcFrame.Y.data(), srcFrame.yStride(), srcFrame.Cb.data(), srcFrame.Cr.data(),
                     srcFrame.cStride(), W, H, srcBytes.data(), std::ptrdiff_t(rowBytes));

    const Lattice lattice = makeAffineLattice(0.7, 0.7, double(W) * 0.15,
                                               double(H) * 0.15, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    // Bottom parity + Complex coefficients here, deliberately different from
    // check 3/4 above's Top/Simple — cheap extra coverage of the same
    // no-op claim under the other parity/coefficient combinations.
    video::Deinterlacer dUnderTest(video::FieldParity::Bottom, video::DeinterlaceCoefficients::Complex);
    video::Deinterlacer dExplicit(video::FieldParity::Bottom, video::DeinterlaceCoefficients::Complex);

    for (int k = 0; k < kNumPushes; ++k) {
        std::vector<std::uint8_t> dstUnderTest(rowBytes * std::size_t(H));
        std::vector<std::uint8_t> dstExplicit(rowBytes * std::size_t(H));

        const bool producedUnderTest = runFrameBytesDeinterlaced(
            dUnderTest, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstUnderTest.data(), std::ptrdiff_t(rowBytes));
        const bool producedExplicit = referenceWithExplicitReinterlace(
            dExplicit, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstExplicit.data(), std::ptrdiff_t(rowBytes));

        CHECK_ONCE(producedUnderTest == producedExplicit);
        if (!producedUnderTest) continue;
        CHECK_ONCE(dstUnderTest == dstExplicit);
    }
}

// ---------------------------------------------------------------------------
// 6. Anchor-parity round trip under an identity lattice. I7 does not apply
// directly here (a de-interlaced round trip is lossy by construction — the
// non-anchor parity's real rows are discarded and reconstructed, ADR-079) —
// this substitutes an anchor-rows-only check instead.
// ---------------------------------------------------------------------------

static void test_deinterlaced_anchor_rows_survive_identity_round_trip() {
    constexpr int W = 64, H = 32;  // even height: Top anchor rows are 0,2,4,...
    constexpr int kNumPushes = 2;  // push 1 (0-indexed) already has its own
                                     // "cur" (video/deinterlace.hpp's own
                                     // state machine) equal to the single
                                     // frame pushed at index 0 — the same
                                     // srcBytes buffer below is pushed every
                                     // time, so which push index becomes
                                     // "cur" makes no difference here.

    const testpat::Frame srcFrame = testpat::makeZonePlate(W, H);
    const std::size_t rowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(rowBytes * std::size_t(H));
    v210::packImage(srcFrame.Y.data(), srcFrame.yStride(), srcFrame.Cb.data(), srcFrame.Cr.data(),
                     srcFrame.cStride(), W, H, srcBytes.data(), std::ptrdiff_t(rowBytes));

    const Lattice identity = makeAffineLattice(1.0, 1.0, 0.0, 0.0, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    video::Deinterlacer d(video::FieldParity::Top, video::DeinterlaceCoefficients::Simple);
    std::vector<std::uint8_t> dst(rowBytes * std::size_t(H));

    bool produced = false;
    for (int k = 0; k < kNumPushes; ++k) {
        produced = runFrameBytesDeinterlaced(
            d, identity, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dst.data(), std::ptrdiff_t(rowBytes));
    }
    CHECK(produced);

    // Top anchor rows (0, 2, 4, ...): identity lattice (no blending, I7),
    // luma passing straight through chroma up/downsample untouched, and
    // flat chroma round-tripping exactly (C-006 — why testpat::makeZonePlate()
    // holds Cb/Cr flat) together mean these rows must reproduce srcBytes'
    // own packed bytes exactly — isolating this unit's own wiring (does
    // push() really copy the anchor parity through unchanged, does the
    // identity lattice really pass it on unchanged) from WU-23b1's own
    // already-tested reconstruction math, which only the non-anchor rows
    // would exercise.
    for (int y = 0; y < H; y += 2) {
        const std::uint8_t* srcRow = srcBytes.data() + std::size_t(y) * rowBytes;
        const std::uint8_t* dstRow = dst.data() + std::size_t(y) * rowBytes;
        CHECK_ONCE(std::memcmp(srcRow, dstRow, rowBytes) == 0);
    }
}

// ---------------------------------------------------------------------------
// 7. 625i50 geometry (720x576), this project's own stay-in-SD-domain scope
// decision (HANDOFF.md) — not 1080i. Reuses the same reference cross-check
// as test_deinterlaced_matches_reference_and_first_push_is_a_noop() above,
// at full SD geometry.
// ---------------------------------------------------------------------------

static void test_deinterlaced_sd_geometry_sanity() {
    constexpr int W = 720, H = 576;
    constexpr int kNumPushes = 3;

    const testpat::Frame srcFrame = testpat::makeZonePlate(W, H);
    const std::size_t rowBytes = v210::rowBytesMin(W);
    std::vector<std::uint8_t> srcBytes(rowBytes * std::size_t(H));
    v210::packImage(srcFrame.Y.data(), srcFrame.yStride(), srcFrame.Cb.data(), srcFrame.Cr.data(),
                     srcFrame.cStride(), W, H, srcBytes.data(), std::ptrdiff_t(rowBytes));

    const Lattice lattice = makeAffineLattice(0.9, 0.9, double(W) * 0.05,
                                               double(H) * 0.05, W, H);
    PipelineParams params;
    params.destWidth  = W;
    params.destHeight = H;
    params.maxK = 1000.0;

    video::Deinterlacer dUnderTest(video::FieldParity::Bottom, video::DeinterlaceCoefficients::Simple);
    video::Deinterlacer dReference(video::FieldParity::Bottom, video::DeinterlaceCoefficients::Simple);

    for (int k = 0; k < kNumPushes; ++k) {
        std::vector<std::uint8_t> dstUnderTest(rowBytes * std::size_t(H));
        std::vector<std::uint8_t> dstReference(rowBytes * std::size_t(H));

        const bool producedUnderTest = runFrameBytesDeinterlaced(
            dUnderTest, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstUnderTest.data(), std::ptrdiff_t(rowBytes));
        const bool producedReference = referenceRunFrameBytesDeinterlaced(
            dReference, lattice, srcBytes.data(), std::ptrdiff_t(rowBytes), W, H,
            params, dstReference.data(), std::ptrdiff_t(rowBytes));

        CHECK_ONCE(producedUnderTest == producedReference);
        if (!producedUnderTest) continue;
        CHECK_ONCE(dstUnderTest == dstReference);
    }
}

int main() {
    test_runframebytes_matches_runframefile_for_a_real_warp();
    test_runframebytes_identity_round_trips_exactly();
    test_deinterlaced_matches_reference_and_first_push_is_a_noop();
    test_deinterlaced_reinterlace_noop_matches_explicit_reinterlace();
    test_deinterlaced_anchor_rows_survive_identity_round_trip();
    test_deinterlaced_sd_geometry_sanity();
    return scatter::test::summary("test_pipeline_bytes");
}
