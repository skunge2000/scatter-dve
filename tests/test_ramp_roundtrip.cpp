// scatter-dve — WU-05 tests: file I/O and the identity-passthrough chain
//
// Three things are checked, not one, because the WORK-UNITS.md accept line
// for this unit ("ramp and excursion patterns round-trip bit-exactly through
// unpack -> upsample -> downsample -> pack") turned out, on working it
// through with the coefficients ADR-020 froze at WU-04, to be true for luma
// and for flat chroma fields but false for chroma on a ramp or excursion
// pattern. See CORRECTIONS.md (this session) for the arithmetic and
// WORK-UNITS.md's corrected accept line for this unit. Nothing below relaxes
// I2 or reopens ADR-020: the chroma filters are exactly as frozen, and I2's
// only clamp (v210::pack's protocol-limit clamp) still fires unconditionally
// at the end of every chain here.
//
// 1. File I/O round trip (Raster422 -> writeV210File -> readV210File) is
//    bit-exact on its own, isolated from chroma resampling entirely — this
//    is what "Raw .v210 source and sink" in WORK-UNITS.md actually promises,
//    and it holds for any pattern, not just flat fields.
// 2. The full chain (unpack -> upsample -> downsample -> pack), run
//    file-to-file, is bit-exact for a flat field, exactly as ADR-020 states.
// 3. The same full chain, on ramp and excursion patterns: luma is bit-exact
//    (chroma resampling never touches Y), and chroma stays within the v210
//    protocol range (I2) but is deliberately not checked for equality —
//    the half-band downsample filter is not a perfect-reconstruction pair
//    with the upsample filter, so it need not (and empirically does not)
//    reproduce a non-flat chroma signal exactly. That is intended
//    anti-aliasing behaviour (ADR-020 describes it as "designed to be
//    lossy" for high-frequency content), not a bug in this unit.

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "harness.hpp"
#include "video/chroma.hpp"
#include "video/raster.hpp"
#include "video/v210.hpp"

using scatter::fromCode10;
using scatter::Sample;
namespace video = scatter::video;
namespace v210 = scatter::v210;
namespace chroma = scatter::chroma;

namespace {

// ---------------------------------------------------------------------------
// Small, self-contained pattern generators. Deliberately not
// tools/testpat.hpp: ADR-019 scopes that header to make_testpat and its own
// test (tests/test_testpat.cpp); this unit's patterns are simple enough not
// to need it, and rule 2 in SESSION-PROTOCOL.md (never refactor across
// module boundaries) argues for leaving that header's stated scope alone
// rather than growing it by one more caller.
// ---------------------------------------------------------------------------

video::Raster422 makeFlat(int width, int height, std::uint16_t code) {
    video::Raster422 f(width, height);
    std::fill(f.Y.begin(),  f.Y.end(),  fromCode10(code));
    std::fill(f.Cb.begin(), f.Cb.end(), fromCode10(code));
    std::fill(f.Cr.begin(), f.Cr.end(), fromCode10(code));
    return f;
}

// Linear ramp, code 4 to 1019, independently on Y and on the (shared) chroma
// row, identical on every row.
video::Raster422 makeRamp(int width, int height) {
    video::Raster422 f(width, height);
    const int cw = v210::chromaWidth(width);

    auto rampCode = [](int x, int span) -> std::uint16_t {
        if (span <= 1) return scatter::kCode10Min;
        const std::uint32_t range = std::uint32_t(scatter::kCode10Max - scatter::kCode10Min);
        const std::uint32_t num   = std::uint32_t(x) * range;
        const std::uint32_t den   = std::uint32_t(span - 1);
        const std::uint32_t step  = (num + den / 2) / den;
        return std::uint16_t(scatter::kCode10Min + step);
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            f.Y[std::size_t(y) * std::size_t(width) + std::size_t(x)] =
                fromCode10(rampCode(x, width));
        }
        for (int x = 0; x < cw; ++x) {
            const Sample s = fromCode10(rampCode(x, cw));
            f.Cb[std::size_t(y) * std::size_t(cw) + std::size_t(x)] = s;
            f.Cr[std::size_t(y) * std::size_t(cw) + std::size_t(x)] = s;
        }
    }
    return f;
}

// Deliberate sub-black / super-white excursions, cycling through six codes
// including both v210 protocol limits (I2's boundary values).
video::Raster422 makeExcursion(int width, int height) {
    static constexpr std::uint16_t kCycle[] = {
        scatter::kCode10Min, 20, scatter::kCode10Black,
        scatter::kCode10WhiteNominal, 1000, scatter::kCode10Max,
    };
    constexpr int kCycleLen = int(sizeof(kCycle) / sizeof(kCycle[0]));

    video::Raster422 f(width, height);
    const int cw = v210::chromaWidth(width);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            f.Y[std::size_t(y) * std::size_t(width) + std::size_t(x)] =
                fromCode10(kCycle[x % kCycleLen]);
        }
        for (int x = 0; x < cw; ++x) {
            const Sample s = fromCode10(kCycle[x % kCycleLen]);
            f.Cb[std::size_t(y) * std::size_t(cw) + std::size_t(x)] = s;
            f.Cr[std::size_t(y) * std::size_t(cw) + std::size_t(x)] = s;
        }
    }
    return f;
}

// Runs the full chain — unpack (via readV210File) -> chroma upsample ->
// chroma downsample -> pack (via writeV210File) — file to file, and returns
// the result read back into a fresh Raster422.
video::Raster422 runIdentityChain(const video::Raster422& src,
                                  const std::string& inPath,
                                  const std::string& outPath) {
    CHECK(video::writeV210File(inPath, src.width, src.height,
                               src.planeY(), src.planeCb(), src.planeCr()));

    video::Raster422 in(src.width, src.height);
    CHECK(video::readV210File(inPath, src.width, src.height,
                              in.planeY(), in.planeCb(), in.planeCr()));

    video::Raster444 full(src.width, src.height);
    chroma::upsampleImage(in.Cb.data(), in.planeCb().strideSamples,
                          src.width, src.height,
                          full.Cb.data(), full.planeCb().strideSamples);
    chroma::upsampleImage(in.Cr.data(), in.planeCr().strideSamples,
                          src.width, src.height,
                          full.Cr.data(), full.planeCr().strideSamples);

    video::Raster422 back(src.width, src.height);
    // Luma never enters a chroma filter; it passes from unpack straight to
    // pack, which is the claim section 1's Y checks below rest on.
    chroma::downsampleImage(full.Cb.data(), full.planeCb().strideSamples,
                            src.width, src.height,
                            back.Cb.data(), back.planeCb().strideSamples);
    chroma::downsampleImage(full.Cr.data(), full.planeCr().strideSamples,
                            src.width, src.height,
                            back.Cr.data(), back.planeCr().strideSamples);

    CHECK(video::writeV210File(outPath, src.width, src.height,
                               in.planeY(), back.planeCb(), back.planeCr()));

    video::Raster422 result(src.width, src.height);
    CHECK(video::readV210File(outPath, src.width, src.height,
                              result.planeY(), result.planeCb(), result.planeCr()));
    return result;
}

// ---------------------------------------------------------------------------
// 1. File I/O alone — bit-exact for any pattern, no chroma filtering at all.
// ---------------------------------------------------------------------------

void testFileRoundTrip(int width, int height) {
    for (auto& f : {makeRamp(width, height), makeExcursion(width, height)}) {
        const std::string path = "test_ramp_roundtrip_fileio.v210";
        CHECK(video::writeV210File(path, width, height,
                                   f.planeY(), f.planeCb(), f.planeCr()));

        // Exactly rowBytesMin(width) * height bytes: no header, no padding
        // beyond v210's own short-group padding (ADR-018), which round-trips
        // deterministically and is folded into rowBytesMin already.
        std::FILE* fp = std::fopen(path.c_str(), "rb");
        CHECK(fp != nullptr);
        if (fp) {
            std::fseek(fp, 0, SEEK_END);
            const long size = std::ftell(fp);
            std::fclose(fp);
            const std::size_t expect = v210::rowBytesMin(width) * std::size_t(height);
            CHECK(size >= 0 && std::size_t(size) == expect);
        }

        video::Raster422 back(width, height);
        CHECK(video::readV210File(path, width, height,
                                  back.planeY(), back.planeCb(), back.planeCr()));
        CHECK(back.Y  == f.Y);
        CHECK(back.Cb == f.Cb);
        CHECK(back.Cr == f.Cr);

        std::remove(path.c_str());
    }
}

// ---------------------------------------------------------------------------
// 2. Full chain, flat field — bit-exact, per ADR-020.
// ---------------------------------------------------------------------------

void testFullChainFlat(int width, int height) {
    for (std::uint16_t code : {scatter::kCode10Min, scatter::kCode10Black,
                               scatter::kCode10ChromaZero, scatter::kCode10Max}) {
        video::Raster422 f = makeFlat(width, height, code);
        video::Raster422 back =
            runIdentityChain(f, "test_ramp_roundtrip_flat_a.v210",
                             "test_ramp_roundtrip_flat_b.v210");
        CHECK(back.Y  == f.Y);
        CHECK(back.Cb == f.Cb);
        CHECK(back.Cr == f.Cr);
    }
    std::remove("test_ramp_roundtrip_flat_a.v210");
    std::remove("test_ramp_roundtrip_flat_b.v210");
}

// ---------------------------------------------------------------------------
// 3. Full chain, ramp and excursion — luma exact; chroma legal, not equal.
// ---------------------------------------------------------------------------

void testFullChainNonFlat(int width, int height) {
    for (auto& f : {makeRamp(width, height), makeExcursion(width, height)}) {
        video::Raster422 back =
            runIdentityChain(f, "test_ramp_roundtrip_nf_a.v210",
                             "test_ramp_roundtrip_nf_b.v210");

        // Luma is never touched by chroma::upsample/downsampleImage, so it
        // must survive exactly, same as the flat case.
        CHECK(back.Y == f.Y);

        // Chroma: not checked for equality (see file header). I2 is checked
        // instead — the one guarantee that must survive regardless of what
        // the resampling filters do to the value.
        for (Sample s : back.Cb) {
            CHECK_ONCE(s >= fromCode10(scatter::kCode10Min) &&
                       s <= fromCode10(scatter::kCode10Max));
        }
        for (Sample s : back.Cr) {
            CHECK_ONCE(s >= fromCode10(scatter::kCode10Min) &&
                       s <= fromCode10(scatter::kCode10Max));
        }
    }
    std::remove("test_ramp_roundtrip_nf_a.v210");
    std::remove("test_ramp_roundtrip_nf_b.v210");
}

// ---------------------------------------------------------------------------
// 4. readV210File / writeV210File reject bad geometry rather than misbehave.
// ---------------------------------------------------------------------------

void testRejectsBadGeometry() {
    video::Raster422 f = makeFlat(8, 2, scatter::kCode10Black);
    CHECK(!video::writeV210File("test_ramp_roundtrip_bad.v210", 7 /* odd */, 2,
                                f.planeY(), f.planeCb(), f.planeCr()));
    CHECK(!video::writeV210File("test_ramp_roundtrip_bad.v210", 8, 0,
                                f.planeY(), f.planeCb(), f.planeCr()));
    CHECK(!video::readV210File("test_ramp_roundtrip_nonexistent.v210", 8, 2,
                               f.planeY(), f.planeCb(), f.planeCr()));
}

}  // namespace

int main() {
    testFileRoundTrip(8, 2);
    testFileRoundTrip(720, 3);

    testFullChainFlat(8, 2);
    testFullChainFlat(720, 3);

    testFullChainNonFlat(8, 2);
    testFullChainNonFlat(720, 3);

    testRejectsBadGeometry();

    return scatter::test::summary("test_ramp_roundtrip");
}
