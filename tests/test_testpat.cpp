// scatter-dve — WU-03 tests: test pattern generator
//
// Checks the exact formulas in tools/testpat.hpp: this is the file that has
// to prove the ramp truly reaches both protocol limits (not just gets
// close), and that the excursion pattern's codes are what I2 says must
// survive the pipeline untouched, not just eyeballed after the fact.

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "harness.hpp"
#include "testpat.hpp"

using scatter::fromCode10;
using scatter::kChromaZero;
using scatter::kCode10Black;
using scatter::kCode10Max;
using scatter::kCode10Min;
using scatter::kCode10WhiteNominal;
using scatter::Sample;
namespace testpat = scatter::testpat;
namespace v210 = scatter::v210;

namespace {

// ---------------------------------------------------------------------------
// 1. Ramp — both endpoints, exactly, on every plane, at more than one width
// ---------------------------------------------------------------------------

void testRampEndpoints(int width, int height) {
    testpat::Frame f = testpat::makeRamp(width, height);
    const int cw = v210::chromaWidth(width);

    CHECK(f.Y.front()                 == fromCode10(kCode10Min));
    CHECK(f.Y[std::size_t(width - 1)] == fromCode10(kCode10Max));
    CHECK(f.Cb.front()                == fromCode10(kCode10Min));
    CHECK(f.Cb[std::size_t(cw - 1)]   == fromCode10(kCode10Max));
    CHECK(f.Cr.front()                == fromCode10(kCode10Min));
    CHECK(f.Cr[std::size_t(cw - 1)]   == fromCode10(kCode10Max));

    // Every row identical: this is a horizontal sweep, not a 2D one.
    for (int y = 1; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            CHECK_ONCE(f.Y[std::size_t(y) * std::size_t(width) + std::size_t(x)] ==
                       f.Y[std::size_t(x)]);
        }
    }
}

void testRampMonotonic(int width) {
    testpat::Frame f = testpat::makeRamp(width, 1);
    for (int x = 1; x < width; ++x) {
        CHECK_ONCE(f.Y[std::size_t(x)] >= f.Y[std::size_t(x - 1)]);
    }
}

// ---------------------------------------------------------------------------
// 2. Zone plate — bounded to the nominal range, chroma flat, not constant
// ---------------------------------------------------------------------------

void testZonePlate(int width, int height) {
    testpat::Frame f = testpat::makeZonePlate(width, height);

    for (std::size_t i = 0; i < f.Y.size(); ++i) {
        CHECK_ONCE(f.Y[i] >= fromCode10(kCode10Black));
        CHECK_ONCE(f.Y[i] <= fromCode10(kCode10WhiteNominal));
    }
    for (std::size_t i = 0; i < f.Cb.size(); ++i) {
        CHECK_ONCE(f.Cb[i] == kChromaZero);
        CHECK_ONCE(f.Cr[i] == kChromaZero);
    }

    // Centre of the plate is r == 0, cos(0) == 1: the single brightest
    // point. Only exact at an integer pixel when width and height are odd,
    // which is why this check uses odd dimensions rather than 720x576.
    const int cx = (width - 1) / 2, cy = (height - 1) / 2;
    const Sample centre = f.Y[std::size_t(cy) * std::size_t(width) + std::size_t(cx)];
    CHECK(centre == fromCode10(kCode10WhiteNominal));

    // Not a flat field: WU-10 has nothing to alias against otherwise.
    bool sawDifferent = false;
    for (std::size_t i = 0; i < f.Y.size(); ++i) {
        if (f.Y[i] != centre) { sawDifferent = true; break; }
    }
    CHECK(sawDifferent);
}

// ---------------------------------------------------------------------------
// 3. Excursion — every cycle code present, and I2 survives a real round trip
// ---------------------------------------------------------------------------

void testExcursionCodesPresent(int width, int height) {
    testpat::Frame f = testpat::makeExcursion(width, height);

    for (std::uint16_t code : testpat::kExcursionCycle) {
        bool foundY = false, foundCb = false, foundCr = false;
        for (Sample s : f.Y)  if (s == fromCode10(code)) foundY  = true;
        for (Sample s : f.Cb) if (s == fromCode10(code)) foundCb = true;
        for (Sample s : f.Cr) if (s == fromCode10(code)) foundCr = true;
        CHECK_ONCE(foundY);
        CHECK_ONCE(foundCb);
        CHECK_ONCE(foundCr);
    }
}

void testExcursionRoundTrip(int width, int height) {
    testpat::Frame f = testpat::makeExcursion(width, height);

    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(stride * std::size_t(height));
    v210::packImage(f.Y.data(), f.yStride(), f.Cb.data(), f.Cr.data(), f.cStride(),
                    width, height, packed.data(), std::ptrdiff_t(stride));

    testpat::Frame back(width, height);
    v210::unpackImage(packed.data(), std::ptrdiff_t(stride), width, height,
                      back.Y.data(), back.yStride(),
                      back.Cb.data(), back.Cr.data(), back.cStride());

    // I2: every excursion code is within [kCode10Min, kCode10Max], so
    // pack's one permitted clamp never fires here. The round trip must be
    // exact — this is what proves nothing downstream re-legalises.
    CHECK(back.Y  == f.Y);
    CHECK(back.Cb == f.Cb);
    CHECK(back.Cr == f.Cr);
}

// ---------------------------------------------------------------------------
// 4. File output — writeV210 produces exactly rowBytesMin(width) * height
//    bytes, and reading them back through unpackImage matches the source.
// ---------------------------------------------------------------------------

void testWriteV210() {
    const int width = 8, height = 2;  // small: this just proves the writer
    testpat::Frame f = testpat::makeExcursion(width, height);

    const std::string path = "test_testpat_output.v210";
    CHECK(testpat::writeV210(f, path));

    std::FILE* fp = std::fopen(path.c_str(), "rb");
    CHECK(fp != nullptr);
    if (fp) {
        std::fseek(fp, 0, SEEK_END);
        const long size = std::ftell(fp);
        std::fseek(fp, 0, SEEK_SET);

        const std::size_t expectStride = v210::rowBytesMin(width);
        CHECK(size >= 0 && std::size_t(size) == expectStride * std::size_t(height));

        std::vector<std::uint8_t> packed(std::size_t(size > 0 ? size : 0));
        const std::size_t got = std::fread(packed.data(), 1, packed.size(), fp);
        CHECK(got == packed.size());
        std::fclose(fp);

        testpat::Frame back(width, height);
        v210::unpackImage(packed.data(), std::ptrdiff_t(expectStride), width, height,
                          back.Y.data(), back.yStride(),
                          back.Cb.data(), back.Cr.data(), back.cStride());
        CHECK(back.Y  == f.Y);
        CHECK(back.Cb == f.Cb);
        CHECK(back.Cr == f.Cr);
    }
    std::remove(path.c_str());
}

}  // namespace

int main() {
    testRampEndpoints(720, 2);
    testRampEndpoints(8, 2);   // small width: endpoints must still land exactly
    testRampMonotonic(720);

    testZonePlate(65, 49);     // odd dimensions: exact integer centre pixel

    testExcursionCodesPresent(720, 2);
    testExcursionRoundTrip(720, 2);

    testWriteV210();

    return scatter::test::summary("test_testpat");
}
