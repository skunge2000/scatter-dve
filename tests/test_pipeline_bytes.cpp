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

#include "core/lattice.hpp"
#include "core/resolve.hpp"
#include "core/types.hpp"
#include "video/raster.hpp"
#include "video/v210.hpp"
#include "harness.hpp"
#include "testpat.hpp"

#include <cstdint>
#include <cstdio>
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

int main() {
    test_runframebytes_matches_runframefile_for_a_real_warp();
    test_runframebytes_identity_round_trips_exactly();
    return scatter::test::summary("test_pipeline_bytes");
}
