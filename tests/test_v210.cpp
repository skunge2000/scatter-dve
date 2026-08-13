// scatter-dve — WU-02 tests: v210 unpack and pack
//
// A round trip alone would pass just as happily with the component order
// wrong, provided it were wrong in both directions, so the first real test
// here is a hand-computed byte vector. Everything after that leans on it.

#include <cstdint>
#include <cstring>
#include <vector>

#include "harness.hpp"
#include "video/v210.hpp"

using scatter::Sample;
using scatter::fromCode10;
using scatter::kCode10Max;
using scatter::kCode10Min;
using scatter::toCode10;
namespace v210 = scatter::v210;

namespace {

// Deterministic across standard libraries, which std::uniform_int_distribution
// is not. Nothing here needs statistical quality.
class Rng {
public:
    explicit Rng(std::uint64_t seed) noexcept : s_(seed | 1u) {}
    std::uint32_t next() noexcept {
        s_ ^= s_ << 13;
        s_ ^= s_ >> 7;
        s_ ^= s_ << 17;
        return std::uint32_t(s_ >> 32);
    }
    // Uniform-ish over the legal protocol range, codes 4 to 1019 inclusive.
    std::uint16_t legalCode() noexcept {
        const std::uint32_t span = std::uint32_t(kCode10Max - kCode10Min + 1);
        return std::uint16_t(kCode10Min + std::uint16_t(next() % span));
    }
private:
    std::uint64_t s_;
};

struct Planes {
    std::vector<Sample> Y, Cb, Cr;
    int width  = 0;
    int height = 0;

    Planes(int w, int h, Sample fill = 0)
        : Y(std::size_t(w) * std::size_t(h), fill),
          Cb(std::size_t(v210::chromaWidth(w)) * std::size_t(h), fill),
          Cr(std::size_t(v210::chromaWidth(w)) * std::size_t(h), fill),
          width(w), height(h) {}

    std::ptrdiff_t yStride() const noexcept { return width; }
    std::ptrdiff_t cStride() const noexcept { return v210::chromaWidth(width); }
};

// ---------------------------------------------------------------------------
// 1. Geometry
// ---------------------------------------------------------------------------

void testGeometry() {
    CHECK(v210::isSupportedWidth(720));
    CHECK(v210::isSupportedWidth(1920));
    CHECK(!v210::isSupportedWidth(721));
    CHECK(!v210::isSupportedWidth(0));
    CHECK(!v210::isSupportedWidth(-6));

    CHECK(v210::chromaWidth(720) == 360);
    CHECK(v210::chromaWidth(1920) == 960);

    CHECK(v210::groupsPerRow(6) == 1);
    CHECK(v210::groupsPerRow(8) == 2);
    CHECK(v210::groupsPerRow(12) == 2);
    CHECK(v210::groupsPerRow(720) == 120);
    CHECK(v210::groupsPerRow(1920) == 320);

    // The two strides the project actually uses. Both exact.
    CHECK(v210::rowBytesMin(720) == 1920);
    CHECK(v210::rowBytesMin(1920) == 5120);
    CHECK(v210::rowBytesAligned(720) == 1920);
    CHECK(v210::rowBytesAligned(1920) == 5120);
    CHECK(v210::rowBytesMin(720) % 128 == 0);
    CHECK(v210::rowBytesMin(1920) % 128 == 0);

    // ...and a width where they differ, which is the whole reason stride is a
    // parameter rather than something derived from width.
    CHECK(v210::rowBytesMin(8) == 32);
    CHECK(v210::rowBytesAligned(8) == 128);
    CHECK(v210::rowBytesMin(2) == 16);
    CHECK(v210::rowBytesAligned(2) == 128);

    static_assert(v210::rowBytesMin(720) == 1920);
    static_assert(v210::rowBytesMin(1920) == 5120);
    static_assert(v210::chromaWidth(720) == 360);
}

// ---------------------------------------------------------------------------
// 2. Known byte vector — the independent check on component order
//
//   Y  = 0x102 0x104 0x106 0x108 0x10A 0x10C
//   Cb = 0x101 0x105 0x109
//   Cr = 0x103 0x107 0x10B
//
//   w0 = 0x101 | 0x102<<10 | 0x103<<20 = 0x10340901
//   w1 = 0x104 | 0x105<<10 | 0x106<<20 = 0x10641504
//   w2 = 0x107 | 0x108<<10 | 0x109<<20 = 0x10942107
//   w3 = 0x10A | 0x10B<<10 | 0x10C<<20 = 0x10C42D0A
//
// little-endian.
// ---------------------------------------------------------------------------

const std::uint8_t kVectorBytes[16] = {
    0x01, 0x09, 0x34, 0x10,
    0x04, 0x15, 0x64, 0x10,
    0x07, 0x21, 0x94, 0x10,
    0x0A, 0x2D, 0xC4, 0x10,
};

const std::uint16_t kVectorY[6]  = {0x102, 0x104, 0x106, 0x108, 0x10A, 0x10C};
const std::uint16_t kVectorCb[3] = {0x101, 0x105, 0x109};
const std::uint16_t kVectorCr[3] = {0x103, 0x107, 0x10B};

void testKnownVector() {
    Sample Y[6] = {}, Cb[3] = {}, Cr[3] = {};
    v210::unpackRow(kVectorBytes, 6, Y, Cb, Cr);

    for (int i = 0; i < 6; ++i) CHECK_ONCE(Y[i] == fromCode10(kVectorY[i]));
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cb[i] == fromCode10(kVectorCb[i]));
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cr[i] == fromCode10(kVectorCr[i]));

    std::uint8_t out[16];
    std::memset(out, 0xA5, sizeof out);
    v210::packRow(Y, Cb, Cr, 6, out);
    CHECK(std::memcmp(out, kVectorBytes, 16) == 0);
}

// ---------------------------------------------------------------------------
// 3. Bits 30 and 31 are unused: ignored on read, zero on write
// ---------------------------------------------------------------------------

void testUnusedBits() {
    std::uint8_t dirty[16];
    std::memcpy(dirty, kVectorBytes, 16);
    for (int w = 0; w < 4; ++w) {
        dirty[std::size_t(w) * 4 + 3] = std::uint8_t(dirty[std::size_t(w) * 4 + 3] | 0xC0u);
    }

    Sample Y[6] = {}, Cb[3] = {}, Cr[3] = {};
    v210::unpackRow(dirty, 6, Y, Cb, Cr);
    for (int i = 0; i < 6; ++i) CHECK_ONCE(Y[i] == fromCode10(kVectorY[i]));
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cb[i] == fromCode10(kVectorCb[i]));
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cr[i] == fromCode10(kVectorCr[i]));

    std::uint8_t out[16];
    std::memset(out, 0xFF, sizeof out);
    v210::packRow(Y, Cb, Cr, 6, out);
    CHECK(std::memcmp(out, kVectorBytes, 16) == 0);
}

// ---------------------------------------------------------------------------
// 4. I2 — protocol limits survive, everything between them is untouched,
//    and pack is the only thing that clamps
// ---------------------------------------------------------------------------

void testProtocolLimits() {
    // Codes 4 and 1019 must survive a round trip exactly.
    const std::uint16_t codes[6] = {kCode10Min, kCode10Max, kCode10Min,
                                    kCode10Max, kCode10Min, kCode10Max};
    Sample Y[6], Cb[3], Cr[3];
    for (int i = 0; i < 6; ++i) Y[i] = fromCode10(codes[i]);
    for (int i = 0; i < 3; ++i) {
        Cb[i] = fromCode10(kCode10Min);
        Cr[i] = fromCode10(kCode10Max);
    }

    std::uint8_t buf[16];
    v210::packRow(Y, Cb, Cr, 6, buf);

    Sample Y2[6] = {}, Cb2[3] = {}, Cr2[3] = {};
    v210::unpackRow(buf, 6, Y2, Cb2, Cr2);
    for (int i = 0; i < 6; ++i) CHECK_ONCE(Y2[i] == Y[i]);
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cb2[i] == Cb[i]);
    for (int i = 0; i < 3; ++i) CHECK_ONCE(Cr2[i] == Cr[i]);

    // Sub-black and super-white inside the protocol range pass through. No
    // legalisation: code 5 is below black, 1000 is well above nominal white,
    // and both come back unchanged.
    const std::uint16_t excursions[6] = {5, 1000, 63, 941, 6, 1018};
    for (int i = 0; i < 6; ++i) Y[i] = fromCode10(excursions[i]);
    v210::packRow(Y, Cb, Cr, 6, buf);
    v210::unpackRow(buf, 6, Y2, Cb2, Cr2);
    for (int i = 0; i < 6; ++i) CHECK_ONCE(Y2[i] == fromCode10(excursions[i]));

    // Below and above the protocol limits, pack clamps — the one permitted
    // clamp in the pipeline. Nothing else in the file may do this.
    const Sample belowMin[6] = {0, 1, 63, 255, 256, 257};
    const std::uint16_t expectLow[6] = {kCode10Min, kCode10Min, kCode10Min,
                                        kCode10Min, kCode10Min, kCode10Min};
    for (int i = 0; i < 6; ++i) Y[i] = belowMin[i];
    v210::packRow(Y, Cb, Cr, 6, buf);
    v210::unpackRow(buf, 6, Y2, Cb2, Cr2);
    for (int i = 0; i < 6; ++i) CHECK_ONCE(Y2[i] == fromCode10(expectLow[i]));

    const Sample aboveMax[6] = {65535, 65472, 65280, 65216, 65215, 65224};
    for (int i = 0; i < 6; ++i) Y[i] = aboveMax[i];
    v210::packRow(Y, Cb, Cr, 6, buf);
    v210::unpackRow(buf, 6, Y2, Cb2, Cr2);
    for (int i = 0; i < 5; ++i) CHECK_ONCE(Y2[i] == fromCode10(kCode10Max));
    // 65215 is 1018.98 and rounds to 1019 without needing the clamp.
    CHECK(Y2[5] == fromCode10(kCode10Max));

    // Rounding is to nearest, not truncation, and does not disturb I7:
    // identity-path values are exact multiples of 64, where the two agree.
    CHECK(toCode10(fromCode10(500)) == 500);
    CHECK(toCode10(Sample(fromCode10(500) + 31)) == 500);
    CHECK(toCode10(Sample(fromCode10(500) + 32)) == 501);
    CHECK(toCode10(Sample(fromCode10(500) - 32)) == 500);
    CHECK(toCode10(Sample(fromCode10(500) - 33)) == 499);
}

// ---------------------------------------------------------------------------
// 5. Random round trip, full rows, several widths
// ---------------------------------------------------------------------------

void testRoundTrip(int width, int height, std::uint64_t seed) {
    Rng rng(seed);

    Planes src(width, height);
    for (auto& s : src.Y)  s = fromCode10(rng.legalCode());
    for (auto& s : src.Cb) s = fromCode10(rng.legalCode());
    for (auto& s : src.Cr) s = fromCode10(rng.legalCode());

    const std::size_t stride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(stride * std::size_t(height), 0xA5);

    v210::packImage(src.Y.data(), src.yStride(),
                    src.Cb.data(), src.Cr.data(), src.cStride(),
                    width, height,
                    packed.data(), std::ptrdiff_t(stride));

    Planes dst(width, height, 0xDEAD);
    v210::unpackImage(packed.data(), std::ptrdiff_t(stride), width, height,
                      dst.Y.data(), dst.yStride(),
                      dst.Cb.data(), dst.Cr.data(), dst.cStride());

    CHECK(dst.Y == src.Y);
    CHECK(dst.Cb == src.Cb);
    CHECK(dst.Cr == src.Cr);

    // And the packed form is byte-identical when re-packed, which is the
    // property WU-05's identity path (I7) will ultimately rest on.
    std::vector<std::uint8_t> repacked(packed.size(), 0x5A);
    v210::packImage(dst.Y.data(), dst.yStride(),
                    dst.Cb.data(), dst.Cr.data(), dst.cStride(),
                    width, height,
                    repacked.data(), std::ptrdiff_t(stride));
    CHECK(repacked == packed);
}

// ---------------------------------------------------------------------------
// 6. Short final group — widths that are not a multiple of 6
// ---------------------------------------------------------------------------

void testShortFinalGroup() {
    // 8 pixels is one full group plus two of six. Padding must be
    // deterministic, so that unpack -> pack is still byte-identical.
    const int width = 8;
    Rng rng(0x5EEDu);

    Planes src(width, 1);
    for (auto& s : src.Y)  s = fromCode10(rng.legalCode());
    for (auto& s : src.Cb) s = fromCode10(rng.legalCode());
    for (auto& s : src.Cr) s = fromCode10(rng.legalCode());

    std::vector<std::uint8_t> packed(v210::rowBytesMin(width), 0xA5);
    v210::packRow(src.Y.data(), src.Cb.data(), src.Cr.data(), width,
                  packed.data());

    // The second group holds luma 6..11 and chroma 3..5. At width 8 the
    // chroma plane is 4 wide, so chroma index 3 is the last real sample:
    // luma 6 and 7 are real, 8..11 are padding, chroma 4 and 5 are padding.
    CHECK(v210::chromaWidth(width) == 4);
    Sample fullY[12] = {}, fullCb[6] = {}, fullCr[6] = {};
    v210::unpackRow(packed.data(), 12, fullY, fullCb, fullCr);
    CHECK(fullY[6] == src.Y[6]);
    CHECK(fullY[7] == src.Y[7]);
    for (int i = 8; i < 12; ++i) {
        CHECK_ONCE(fullY[i] == fromCode10(v210::kPadLuma));
    }
    CHECK(fullCb[3] == src.Cb[3]);
    CHECK(fullCr[3] == src.Cr[3]);
    for (int i = 4; i < 6; ++i) {
        CHECK_ONCE(fullCb[i] == fromCode10(v210::kPadChroma));
        CHECK_ONCE(fullCr[i] == fromCode10(v210::kPadChroma));
    }

    // Unpacking at the true width must not touch samples past it.
    Planes dst(width, 1, 0xDEAD);
    dst.Y.push_back(0xBEEF);
    dst.Cb.push_back(0xBEEF);
    dst.Cr.push_back(0xBEEF);
    v210::unpackRow(packed.data(), width,
                    dst.Y.data(), dst.Cb.data(), dst.Cr.data());
    CHECK(dst.Y.back() == 0xBEEF);
    CHECK(dst.Cb.back() == 0xBEEF);
    CHECK(dst.Cr.back() == 0xBEEF);
    dst.Y.pop_back();
    dst.Cb.pop_back();
    dst.Cr.pop_back();
    CHECK(dst.Y == src.Y);
    CHECK(dst.Cb == src.Cb);
    CHECK(dst.Cr == src.Cr);
}

// ---------------------------------------------------------------------------
// 7. Stride: pack writes rowBytesMin and not one byte more
// ---------------------------------------------------------------------------

void testStridePadding() {
    const int width  = 8;    // rowBytesMin 32, aligned stride 128
    const int height = 4;
    const std::size_t rowMin = v210::rowBytesMin(width);
    const std::size_t stride = v210::rowBytesAligned(width);
    CHECK(stride > rowMin);

    Rng rng(0xC0FFEEu);
    Planes src(width, height);
    for (auto& s : src.Y)  s = fromCode10(rng.legalCode());
    for (auto& s : src.Cb) s = fromCode10(rng.legalCode());
    for (auto& s : src.Cr) s = fromCode10(rng.legalCode());

    std::vector<std::uint8_t> packed(stride * std::size_t(height), 0xA5);
    v210::packImage(src.Y.data(), src.yStride(),
                    src.Cb.data(), src.Cr.data(), src.cStride(),
                    width, height,
                    packed.data(), std::ptrdiff_t(stride));

    for (int row = 0; row < height; ++row) {
        for (std::size_t b = rowMin; b < stride; ++b) {
            CHECK_ONCE(packed[std::size_t(row) * stride + b] == 0xA5);
        }
    }

    Planes dst(width, height, 0xDEAD);
    v210::unpackImage(packed.data(), std::ptrdiff_t(stride), width, height,
                      dst.Y.data(), dst.yStride(),
                      dst.Cb.data(), dst.Cr.data(), dst.cStride());
    CHECK(dst.Y == src.Y);
    CHECK(dst.Cb == src.Cb);
    CHECK(dst.Cr == src.Cr);
}

// ---------------------------------------------------------------------------
// 8. Plane stride larger than width: rows stay independent
// ---------------------------------------------------------------------------

void testPlaneStride() {
    const int width  = 12;
    const int height = 3;
    const std::ptrdiff_t yStride = width + 7;
    const std::ptrdiff_t cStride = v210::chromaWidth(width) + 5;

    Rng rng(0xB16B00B5u);
    std::vector<Sample> Y(std::size_t(yStride) * std::size_t(height), 0xDEAD);
    std::vector<Sample> Cb(std::size_t(cStride) * std::size_t(height), 0xDEAD);
    std::vector<Sample> Cr(std::size_t(cStride) * std::size_t(height), 0xDEAD);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            Y[std::size_t(row * yStride + x)] = fromCode10(rng.legalCode());
        }
        for (int x = 0; x < v210::chromaWidth(width); ++x) {
            Cb[std::size_t(row * cStride + x)] = fromCode10(rng.legalCode());
            Cr[std::size_t(row * cStride + x)] = fromCode10(rng.legalCode());
        }
    }

    const std::size_t packStride = v210::rowBytesMin(width);
    std::vector<std::uint8_t> packed(packStride * std::size_t(height));
    v210::packImage(Y.data(), yStride, Cb.data(), Cr.data(), cStride,
                    width, height, packed.data(), std::ptrdiff_t(packStride));

    std::vector<Sample> Y2(Y.size(), 0), Cb2(Cb.size(), 0), Cr2(Cr.size(), 0);
    v210::unpackImage(packed.data(), std::ptrdiff_t(packStride), width, height,
                      Y2.data(), yStride, Cb2.data(), Cr2.data(), cStride);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            CHECK_ONCE(Y2[std::size_t(row * yStride + x)] ==
                       Y[std::size_t(row * yStride + x)]);
        }
        // Inter-row gap in the destination must be untouched.
        for (std::ptrdiff_t x = width; x < yStride; ++x) {
            CHECK_ONCE(Y2[std::size_t(row * yStride + x)] == 0);
        }
        for (int x = 0; x < v210::chromaWidth(width); ++x) {
            CHECK_ONCE(Cb2[std::size_t(row * cStride + x)] ==
                       Cb[std::size_t(row * cStride + x)]);
            CHECK_ONCE(Cr2[std::size_t(row * cStride + x)] ==
                       Cr[std::size_t(row * cStride + x)]);
        }
    }
}

// ---------------------------------------------------------------------------
// 9. A whole 576p25 frame, which is the format Phase 1 works in
// ---------------------------------------------------------------------------

void testFrame576() {
    testRoundTrip(720, 576, 0x0576u);
}

}  // namespace

int main() {
    testGeometry();
    testKnownVector();
    testUnusedBits();
    testProtocolLimits();

    // Widths covering every residue mod 6 that an even width can take.
    const int widths[] = {2, 4, 6, 8, 10, 12, 14, 16, 18, 22, 720, 1920};
    std::uint64_t seed = 0x1234ABCDu;
    for (int w : widths) {
        testRoundTrip(w, 3, seed);
        seed += 0x9E3779B9u;
    }

    testShortFinalGroup();
    testStridePadding();
    testPlaneStride();
    testFrame576();

    return scatter::test::summary("test_v210");
}
