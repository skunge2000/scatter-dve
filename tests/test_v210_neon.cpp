// scatter-dve — WU-17 tests: NEON v210 unpack/pack vs the scalar reference
//
// This unit's whole accept criterion (WORK-UNITS.md) is "bit-identical to
// scalar reference" -- so unlike test_v210.cpp, which proves the scalar
// codec correct against a hand-computed byte vector, this file never
// hand-derives an expected value. Every check here is a direct diff between
// v210::unpackRow/packRow/unpackImage/packImage (unchanged since WU-02) and
// their NEON-suffixed siblings (WU-17), over the same input. Only built when
// __ARM_NEON is defined -- CMakeLists.txt gates this executable's own
// add_executable() on CMAKE_SYSTEM_PROCESSOR, the same way test_decklink_*
// is gated on BLACKMAGIC_SDK_DIR (ADR-031) -- so no #ifdef is needed inside
// this file itself; see DECISIONS.md ADR-042.

#include <cstdint>
#include <cstring>
#include <vector>

#include "harness.hpp"
#include "video/v210.hpp"

using scatter::Sample;
using scatter::fromCode10;
using scatter::kCode10Max;
using scatter::kCode10Min;
namespace v210 = scatter::v210;

namespace {

// Same small deterministic RNG test_v210.cpp uses -- duplicated locally
// rather than shared, per this project's own "one unit, one test, no shared
// fixture header" convention (see DECISIONS.md ADR-030's own test_morph.cpp
// precedent).
class Rng {
public:
    explicit Rng(std::uint64_t seed) noexcept : s_(seed | 1u) {}
    std::uint32_t next() noexcept {
        s_ ^= s_ << 13;
        s_ ^= s_ >> 7;
        s_ ^= s_ << 17;
        return std::uint32_t(s_ >> 32);
    }
    // Full 16-bit Sample domain -- packRowNeon's toCode10 clamp (I2) is
    // exercised at and around its bounds (codes 0-3/1020-1023), not just
    // inside the legal range, the same way testPackRowMatchesScalar below
    // uses it.
    Sample anySample() noexcept { return Sample(next()); }
private:
    std::uint64_t s_;
};

struct Planes {
    std::vector<Sample> Y, Cb, Cr;
    int width = 0, height = 0;

    Planes(int w, int h, Sample fill = 0)
        : Y(std::size_t(w) * std::size_t(h), fill),
          Cb(std::size_t(v210::chromaWidth(w)) * std::size_t(h), fill),
          Cr(std::size_t(v210::chromaWidth(w)) * std::size_t(h), fill),
          width(w), height(h) {}

    std::ptrdiff_t yStride() const noexcept { return width; }
    std::ptrdiff_t cStride() const noexcept { return v210::chromaWidth(width); }
};

// ---------------------------------------------------------------------------
// 1. unpackRowNeon vs unpackRow: random packed bytes, several widths
//    covering every residue mod 6 an even width can take (test_v210.cpp's
//    own set), so both the full-group fast path and the short-final-group
//    path (ADR-018) get diffed.
// ---------------------------------------------------------------------------

void testUnpackRowMatchesScalar(int width, std::uint64_t seed) {
    Rng rng(seed);
    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(stride);
    for (auto& b : packed) b = std::uint8_t(rng.next() & 0xFFu);
    // Dirty bits 30/31 of every word -- unpackRow/unpackRowNeon must both
    // ignore them identically (same property test_v210.cpp's
    // testUnusedBits checks against the scalar's own known vector).
    for (std::size_t w = 3; w < packed.size(); w += 4) packed[w] |= 0xC0u;

    const int cw = v210::chromaWidth(width);
    std::vector<Sample> Ys(std::size_t(width), 0xDEAD), Cbs(std::size_t(cw), 0xDEAD),
                        Crs(std::size_t(cw), 0xDEAD);
    std::vector<Sample> Yn(std::size_t(width), 0xBEEF), Cbn(std::size_t(cw), 0xBEEF),
                        Crn(std::size_t(cw), 0xBEEF);

    v210::unpackRow(packed.data(), width, Ys.data(), Cbs.data(), Crs.data());
    v210::unpackRowNeon(packed.data(), width, Yn.data(), Cbn.data(), Crn.data());

    CHECK(Ys == Yn);
    CHECK(Cbs == Cbn);
    CHECK(Crs == Crn);
}

// ---------------------------------------------------------------------------
// 2. packRowNeon vs packRow: the full 10-bit domain, including codes 0-3 and
//    1020-1023 -- exercising I2's clamp (unchanged, toCode10 itself is not
//    touched by this unit) on both paths identically.
// ---------------------------------------------------------------------------

void testPackRowMatchesScalar(int width, std::uint64_t seed) {
    Rng rng(seed);
    const int cw = v210::chromaWidth(width);
    // Most-vexing-parse note: `Y(std::size_t(width))` here would parse as a
    // function declaration, not a vector construction -- named size_t
    // locals first, so each vector ctor argument is a plain identifier.
    const std::size_t nY = std::size_t(width);
    const std::size_t nC = std::size_t(cw);
    std::vector<Sample> Y(nY), Cb(nC), Cr(nC);
    // Full 16-bit Sample domain, not just the legal-code range -- packRow's
    // own toCode10 clamp (I2) is exercised at, below and above its bounds,
    // the same "belowMin"/"aboveMax" coverage test_v210.cpp's
    // testProtocolLimits already gives the scalar path alone.
    for (auto& s : Y)  s = rng.anySample();
    for (auto& s : Cb) s = rng.anySample();
    for (auto& s : Cr) s = rng.anySample();

    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> outS(stride, 0xA5), outN(stride, 0x5A);
    v210::packRow(Y.data(), Cb.data(), Cr.data(), width, outS.data());
    v210::packRowNeon(Y.data(), Cb.data(), Cr.data(), width, outN.data());

    CHECK(outS == outN);
}

// ---------------------------------------------------------------------------
// 3. unpackImageNeon/packImageNeon vs unpackImage/packImage over a whole
//    576p25 frame (720x576) -- the format Phase 1 actually works in, same
//    frame test_v210.cpp's own testFrame576 exercises for the scalar path
//    alone.
// ---------------------------------------------------------------------------

void testImageMatchesScalar576() {
    const int width = 720, height = 576;
    Rng rng(0x17u);  // WU-17

    Planes src(width, height);
    for (auto& s : src.Y)  s = fromCode10(std::uint16_t(kCode10Min + rng.next() %
                                          (kCode10Max - kCode10Min + 1)));
    for (auto& s : src.Cb) s = fromCode10(std::uint16_t(kCode10Min + rng.next() %
                                          (kCode10Max - kCode10Min + 1)));
    for (auto& s : src.Cr) s = fromCode10(std::uint16_t(kCode10Min + rng.next() %
                                          (kCode10Max - kCode10Min + 1)));

    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packedS(stride * std::size_t(height));
    std::vector<std::uint8_t> packedN(stride * std::size_t(height));
    v210::packImage(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                    src.cStride(), width, height, packedS.data(),
                    std::ptrdiff_t(stride));
    v210::packImageNeon(src.Y.data(), src.yStride(), src.Cb.data(), src.Cr.data(),
                        src.cStride(), width, height, packedN.data(),
                        std::ptrdiff_t(stride));
    CHECK(packedS == packedN);

    Planes dstS(width, height, 0xDEAD), dstN(width, height, 0xBEEF);
    v210::unpackImage(packedS.data(), std::ptrdiff_t(stride), width, height,
                      dstS.Y.data(), dstS.yStride(), dstS.Cb.data(), dstS.Cr.data(),
                      dstS.cStride());
    v210::unpackImageNeon(packedN.data(), std::ptrdiff_t(stride), width, height,
                          dstN.Y.data(), dstN.yStride(), dstN.Cb.data(), dstN.Cr.data(),
                          dstN.cStride());
    CHECK(dstS.Y == dstN.Y);
    CHECK(dstS.Cb == dstN.Cb);
    CHECK(dstS.Cr == dstN.Cr);

    // And NEON's own round trip: unpack -> pack via NEON alone reproduces
    // the packed bytes exactly, the same I7-adjacent property
    // test_v210.cpp's own testRoundTrip checks for the scalar path.
    std::vector<std::uint8_t> repackedN(packedN.size(), 0x11);
    v210::packImageNeon(dstN.Y.data(), dstN.yStride(), dstN.Cb.data(), dstN.Cr.data(),
                        dstN.cStride(), width, height, repackedN.data(),
                        std::ptrdiff_t(stride));
    CHECK(repackedN == packedN);
}

}  // namespace

int main() {
    const int widths[] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 22, 720, 1920};
    std::uint64_t seed = 0x17170017u;
    for (int w : widths) {
        testUnpackRowMatchesScalar(w, seed);
        seed += 0x9E3779B9u;
        testPackRowMatchesScalar(w, seed);
        seed += 0x9E3779B9u;
    }

    testImageMatchesScalar576();

    return scatter::test::summary("test_v210_neon");
}
