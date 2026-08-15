// scatter-dve — WU-18 tests: NEON chroma resampling vs the scalar reference
//
// This unit's whole accept criterion (WORK-UNITS.md) is "bit-identical to
// scalar reference" -- the same shape WU-17's own test_v210_neon.cpp already
// established (DECISIONS.md ADR-042) and this unit's own ADR-043 reuses
// directly. Every check here is a direct diff between
// chroma::upsampleRow/downsampleRow/upsampleImage/downsampleImage
// (unchanged since WU-04) and their NEON-suffixed siblings (WU-18), over the
// same input -- never a round trip through both filters, which C-006
// already established is not bit-exact for non-flat content and is not what
// this unit claims. Only built when __ARM_NEON is defined -- CMakeLists.txt
// gates this executable's own add_executable() on CMAKE_SYSTEM_PROCESSOR,
// the same way test_v210_neon already is (ADR-042) -- so no #ifdef is
// needed inside this file itself; see DECISIONS.md ADR-043.

#include <cstdint>
#include <vector>

#include "harness.hpp"
#include "video/chroma.hpp"
#include "video/v210.hpp"

using scatter::Sample;
namespace chroma = scatter::chroma;
namespace v210 = scatter::v210;

namespace {

// Same small deterministic RNG test_v210_neon.cpp uses -- duplicated
// locally, per this project's own "one unit, one test, no shared fixture
// header" convention (DECISIONS.md ADR-030's own test_morph.cpp precedent).
class Rng {
public:
    explicit Rng(std::uint64_t seed) noexcept : s_(seed | 1u) {}
    std::uint32_t next() noexcept {
        s_ ^= s_ << 13;
        s_ ^= s_ >> 7;
        s_ ^= s_ << 17;
        return std::uint32_t(s_ >> 32);
    }
    // Full 16-bit Sample domain -- chroma's own filters apply no clamp of
    // their own (I2 is enforced only at v210::pack), so the NEON path's
    // int32 accumulation and modulo-65536 narrowing must match the scalar
    // reference across the whole representable range, not just legal
    // chroma content.
    Sample anySample() noexcept { return Sample(next()); }
private:
    std::uint64_t s_;
};

// ---------------------------------------------------------------------------
// 1. upsampleRowNeon vs upsampleRow. Widths chosen so chromaWidth(width)
//    (the interior/edge split's own governing quantity, not width itself)
//    sweeps 1 through 14 -- covering zero interior batches, exactly one, and
//    the transition into two -- plus 720/1920, the two real widths.
// ---------------------------------------------------------------------------

void testUpsampleRowMatchesScalar(int width, std::uint64_t seed) {
    Rng rng(seed);
    const int cw = v210::chromaWidth(width);
    // Most-vexing-parse note (see HANDOFF.md, WU-17's own session): a
    // single-argument std::vector<Sample> in(std::size_t(cw)) parses as a
    // function declaration, not a vector construction -- name the size_t
    // local first, same fix test_v210_neon.cpp's own first draft needed.
    const std::size_t nCw = std::size_t(cw);
    std::vector<Sample> in(nCw);
    for (auto& s : in) s = rng.anySample();

    std::vector<Sample> outS(std::size_t(width), 0xDEAD);
    std::vector<Sample> outN(std::size_t(width), 0xBEEF);
    chroma::upsampleRow(in.data(), width, outS.data());
    chroma::upsampleRowNeon(in.data(), width, outN.data());

    CHECK(outS == outN);
}

// ---------------------------------------------------------------------------
// 2. downsampleRowNeon vs downsampleRow. Same width sweep; downsample's
//    wider 7-tap footprint needs chromaWidth(width) >= 9 before its own
//    interior loop runs at all, so the smaller widths here exercise the
//    fully-scalar path in full, not just an edge case around it.
// ---------------------------------------------------------------------------

void testDownsampleRowMatchesScalar(int width, std::uint64_t seed) {
    Rng rng(seed);
    // Same most-vexing-parse fix as testUpsampleRowMatchesScalar, above.
    const std::size_t nWidth = std::size_t(width);
    std::vector<Sample> in(nWidth);
    for (auto& s : in) s = rng.anySample();

    const int cw = v210::chromaWidth(width);
    std::vector<Sample> outS(std::size_t(cw), 0xDEAD);
    std::vector<Sample> outN(std::size_t(cw), 0xBEEF);
    chroma::downsampleRow(in.data(), width, outS.data());
    chroma::downsampleRowNeon(in.data(), width, outN.data());

    CHECK(outS == outN);
}

// ---------------------------------------------------------------------------
// 3. Image wrappers, over a whole 576p25 frame (720x576) -- the format
//    Phase 1 actually works in, same frame test_v210_neon.cpp's own
//    testImageMatchesScalar576 exercises for the v210 codec.
// ---------------------------------------------------------------------------

void testImageMatchesScalar576() {
    const int width = 720, height = 576;
    const int cw = v210::chromaWidth(width);
    const std::ptrdiff_t cStride = cw;
    const std::ptrdiff_t wStride = width;
    Rng rng(0x18u);  // WU-18

    std::vector<Sample> narrow(std::size_t(cw) * std::size_t(height));
    for (auto& s : narrow) s = rng.anySample();

    std::vector<Sample> wideS(std::size_t(width) * std::size_t(height), 0xDEAD);
    std::vector<Sample> wideN(std::size_t(width) * std::size_t(height), 0xBEEF);
    chroma::upsampleImage(narrow.data(), cStride, width, height, wideS.data(), wStride);
    chroma::upsampleImageNeon(narrow.data(), cStride, width, height, wideN.data(), wStride);
    CHECK(wideS == wideN);

    std::vector<Sample> wide(std::size_t(width) * std::size_t(height));
    for (auto& s : wide) s = rng.anySample();

    std::vector<Sample> backS(std::size_t(cw) * std::size_t(height), 0xDEAD);
    std::vector<Sample> backN(std::size_t(cw) * std::size_t(height), 0xBEEF);
    chroma::downsampleImage(wide.data(), wStride, width, height, backS.data(), cStride);
    chroma::downsampleImageNeon(wide.data(), wStride, width, height, backN.data(), cStride);
    CHECK(backS == backN);
}

}  // namespace

int main() {
    const int widths[] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 720, 1920};
    std::uint64_t seed = 0x18180018u;
    for (int w : widths) {
        testUpsampleRowMatchesScalar(w, seed);
        seed += 0x9E3779B9u;
        testDownsampleRowMatchesScalar(w, seed);
        seed += 0x9E3779B9u;
    }

    testImageMatchesScalar576();

    return scatter::test::summary("test_chroma_neon");
}
