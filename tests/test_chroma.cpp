// scatter-dve — WU-04 tests: chroma resampling
//
// Three independent checks, in the spirit of test_v210.cpp: flat fields
// survive exactly (proves the DC/unity-gain claim), hand-computed impulse
// vectors pin down the coefficients themselves — sign included — rather
// than just their sum, and a hand-computed step proves the ringing is
// present, of the documented magnitude, and not clamped back into [a, b] or
// any narrower legal range.

#include <cstddef>
#include <cstdint>
#include <vector>

#include "harness.hpp"
#include "video/chroma.hpp"
#include "video/v210.hpp"

using scatter::Sample;
using scatter::kChromaZero;
namespace chroma = scatter::chroma;
namespace v210 = scatter::v210;

namespace {

// ---------------------------------------------------------------------------
// 1. Flat fields survive exactly, through both directions, at an ordinary
//    width and at the minimum width where every tap on every call clamps to
//    a single element.
// ---------------------------------------------------------------------------

void testFlatField(int width, Sample value) {
    const int cw = v210::chromaWidth(width);

    std::vector<Sample> narrow(std::size_t(cw), value);
    std::vector<Sample> wide(std::size_t(width), 0);
    chroma::upsampleRow(narrow.data(), width, wide.data());
    for (Sample s : wide) CHECK_ONCE(s == value);

    std::vector<Sample> back(std::size_t(cw), 0);
    chroma::downsampleRow(wide.data(), width, back.data());
    for (Sample s : back) CHECK_ONCE(s == value);

    // downsampleRow on its own, independent of whatever upsampleRow produced.
    std::vector<Sample> wideDirect(std::size_t(width), value);
    std::vector<Sample> back2(std::size_t(cw), 0);
    chroma::downsampleRow(wideDirect.data(), width, back2.data());
    for (Sample s : back2) CHECK_ONCE(s == value);
}

void testFlatFields() {
    // 720: the width Phase 1 actually works in. 1920: the other real width.
    // 8: ordinary and small. 2: the minimum — every tap clamps.
    const int widths[] = {2, 4, 8, 720, 1920};
    const Sample values[] = {0, kChromaZero, 65535, 12345};
    for (int w : widths) {
        for (Sample v : values) {
            testFlatField(w, v);
        }
    }
}

// ---------------------------------------------------------------------------
// 2. Known vectors — hand-computed impulse responses. A flat field alone
//    would pass just as happily with a coefficient's sign flipped, provided
//    the row still summed to the same unity gain; this is the check that
//    pins down each individual tap. Filter definitions are in chroma.hpp.
// ---------------------------------------------------------------------------

// Upsample, cw = 8 (width 16), impulse of 16 at chroma index 3. Amplitude 16
// makes every partial sum an exact multiple of the divisor, so there is
// nothing to reason about but the coefficients and their sign.
//
// Co-sited samples (out[2i]) are the impulse verbatim: all zero except
// out[6] = 16.
//
// Halfway samples (out[2i+1]), by which tap of (-1, 9, 9, -1) the impulse
// lands under, for i = 0..7:
//   i=0: none of the four taps sees it                    -> 0
//   i=1: impulse is `d` (in[i+2]=in[3]), coefficient -1    -> -16 -> wraps
//   i=2: impulse is `c` (in[i+1]=in[3]), coefficient +9    -> 144/16 = 9
//   i=3: impulse is `b` (in[i]=in[3]),   coefficient +9    -> 144/16 = 9
//   i=4: impulse is `a` (in[i-1]=in[3]), coefficient -1    -> -16 -> wraps
//   i=5..7: none of the four taps sees it                 -> 0
//
// -16 rounds to exactly -1 (see chroma.hpp on rounding), which as a 16-bit
// unsigned Sample is 65535 — the two's-complement bit pattern for -1, not a
// clamp.
const Sample kUpsampleImpulseOut[16] = {
    0, 0, 0, 65535, 0, 9, 16, 9, 0, 65535, 0, 0, 0, 0, 0, 0,
};

void testUpsampleKnownVector() {
    Sample in[8] = {};
    in[3] = 16;
    Sample out[16] = {};
    chroma::upsampleRow(in, 16, out);
    for (int i = 0; i < 16; ++i) CHECK_ONCE(out[i] == kUpsampleImpulseOut[i]);
}

// Downsample, width 24 (cw 12). The half-band structure means every tap at
// a nonzero *even* offset from a given output's centre (2i) is zero, so an
// impulse at an even (co-sited) 4:4:4 position is seen only through the
// centre tap (+256) of the one output whose centre lands exactly on it.
//
// 512 at the centre tap: (256*512 + 256) >> 9 = 256, at i=6 (2*6=12) alone.
const Sample kDownsampleCositedOut[12] = {0, 0, 0, 0, 0, 0, 256, 0, 0, 0, 0, 0};

void testDownsampleCositedImpulse() {
    Sample in[24] = {};
    in[12] = 512;
    Sample out[12] = {};
    chroma::downsampleRow(in, 24, out);
    for (int i = 0; i < 12; ++i) CHECK_ONCE(out[i] == kDownsampleCositedOut[i]);
}

// An impulse at an *odd* 4:4:4 position exercises the other six
// coefficients instead — 2i is always even, so the centre tap (offset 0)
// never lands on it, and only the six odd-offset taps (+-1, +-3, +-5) can.
// Amplitude 512 again makes every partial sum an exact multiple of the
// divisor.
//
// Position 13 is offset (13 - 2i) from output i's centre; the six outputs
// that see it, and which tap:
//   i=9 (2i=18): offset -5, coefficient   3 ->    3*512/512 =    3
//   i=8 (2i=16): offset -3, coefficient -25 ->  -25*512/512 =  -25 -> wraps
//   i=7 (2i=14): offset -1, coefficient 150 ->  150*512/512 =  150
//   i=6 (2i=12): offset +1, coefficient 150 ->  150*512/512 =  150
//   i=5 (2i=10): offset +3, coefficient -25 ->  -25*512/512 =  -25 -> wraps
//   i=4 (2i=8):  offset +5, coefficient   3 ->    3*512/512 =    3
// Symmetric, as the filter is, and every other output sees none of it.
const Sample kDownsampleImpulseOut[12] = {
    0, 0, 0, 0, 3, 65511, 150, 150, 65511, 3, 0, 0,
};

void testDownsampleKnownVector() {
    Sample in[24] = {};
    in[13] = 512;
    Sample out[12] = {};
    chroma::downsampleRow(in, 24, out);
    for (int i = 0; i < 12; ++i) CHECK_ONCE(out[i] == kDownsampleImpulseOut[i]);
}

// ---------------------------------------------------------------------------
// 3. Ringing on a step: present, of the documented magnitude, and not
//    clamped back into [a, b] or any narrower legal range.
// ---------------------------------------------------------------------------

void testUpsampleStepRinging() {
    // cw = 8: four samples at a, four at b. Both well inside the 16-bit
    // range with headroom either side of the 1/16 * (b-a) = 2000 worst-case
    // excursion this filter produces, so nothing wraps.
    const Sample a = 10000, b = 42000;
    const Sample in[8] = {a, a, a, a, b, b, b, b};
    Sample out[16] = {};
    chroma::upsampleRow(in, 16, out);

    // i=2 (in[1..4] = a,a,a,b): (17a - b + 8) >> 4 = 8000, undershooting a.
    CHECK(out[5] == 8000);
    CHECK(out[5] < a);
    // i=3, straddling the edge itself, is a plain average — no ringing.
    CHECK(out[7] == (a + b) / 2);
    // i=4 (in[3..6] = a,b,b,b): (-a + 17b + 8) >> 4 = 44000, overshooting b.
    CHECK(out[9] == 44000);
    CHECK(out[9] > b);
}

void testDownsampleStepRinging() {
    // width 24: twelve samples at a, twelve at b. b - a = 25600 = 100*256,
    // chosen so the 22/512 worst-case excursion, 1100, is an exact integer.
    // Headroom either side, nothing wraps.
    const Sample a = 15000, b = 40600;
    Sample in[24];
    for (int i = 0; i < 12; ++i) in[std::size_t(i)] = a;
    for (int i = 12; i < 24; ++i) in[std::size_t(i)] = b;
    Sample out[12] = {};
    chroma::downsampleRow(in, 24, out);

    CHECK(out[5] == 13900);
    CHECK(out[5] < a);
    CHECK(out[6] == 34200);  // straddles the edge; in range, no ringing.
    CHECK(out[7] == 41700);
    CHECK(out[7] > b);
}

// ---------------------------------------------------------------------------
// 4. Image wrapper: row independence and stride handling, mirroring
//    test_v210.cpp's testPlaneStride.
// ---------------------------------------------------------------------------

void testImageStride() {
    const int width  = 8;
    const int cw      = v210::chromaWidth(width);
    const int height  = 3;
    const std::ptrdiff_t cStride = cw + 3;
    const std::ptrdiff_t wStride = width + 5;
    const Sample kSentinel = 0xBEEF;

    const Sample rowValue[3] = {1000, 2000, 3000};

    std::vector<Sample> narrow(std::size_t(cStride) * std::size_t(height),
                               kSentinel);
    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < cw; ++x) {
            narrow[std::size_t(row) * std::size_t(cStride) + std::size_t(x)] =
                rowValue[row];
        }
    }

    std::vector<Sample> wide(std::size_t(wStride) * std::size_t(height),
                             kSentinel);
    chroma::upsampleImage(narrow.data(), cStride, width, height, wide.data(),
                          wStride);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            CHECK_ONCE(wide[std::size_t(row) * std::size_t(wStride) +
                            std::size_t(x)] == rowValue[row]);
        }
        for (std::ptrdiff_t x = width; x < wStride; ++x) {
            CHECK_ONCE(wide[std::size_t(row) * std::size_t(wStride) +
                            std::size_t(x)] == kSentinel);
        }
    }

    std::vector<Sample> back(std::size_t(cStride) * std::size_t(height),
                             kSentinel);
    chroma::downsampleImage(wide.data(), wStride, width, height, back.data(),
                            cStride);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < cw; ++x) {
            CHECK_ONCE(back[std::size_t(row) * std::size_t(cStride) +
                            std::size_t(x)] == rowValue[row]);
        }
        for (std::ptrdiff_t x = cw; x < cStride; ++x) {
            CHECK_ONCE(back[std::size_t(row) * std::size_t(cStride) +
                            std::size_t(x)] == kSentinel);
        }
    }
}

}  // namespace

int main() {
    testFlatFields();
    testUpsampleKnownVector();
    testDownsampleCositedImpulse();
    testDownsampleKnownVector();
    testUpsampleStepRinging();
    testDownsampleStepRinging();
    testImageStride();

    return scatter::test::summary("test_chroma");
}
