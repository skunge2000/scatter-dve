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
using scatter::kBlack;
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

// ---------------------------------------------------------------------------
// 5. RGB boundary conversion (WU-40, DECISIONS.md ADR-085) —
//    ycbcrToRgbRow/rgbToYcbcrRow and their Image wrappers. WU-44a
//    (WORK-UNITS.md): these four functions had zero test coverage of any
//    kind before this unit, despite being in real production use since
//    WU-40 (src/core/pipeline.cpp calls both Image variants at every
//    boundary crossing) — this section is a coverage gap being closed, not
//    a previously-red check turning green: this file's own checks above
//    were, and remain, green, and none of the suite's three genuinely red
//    tests (test_binner, test_zoneplate, test_pipeline_bytes — see
//    WORK-UNITS.md's own WU-44a entry) has a failing check in this file.
//
//    Every expected value below is hand-derived independently from
//    chroma.hpp's own stated BT.601 formula (Kr=0.299, Kg=0.587, Kb=0.114),
//    worked separately from chroma.cpp's implementation and cross-checked
//    numerically before being written here — never by calling
//    ycbcrToRgbRow/rgbToYcbcrRow themselves and trusting the result. Same
//    "mirror the math independently, never call the production function"
//    discipline WU-34b established (ADR-084), which ADR-085 itself names as
//    its own stated preference over a mechanical fixture transform.
// ---------------------------------------------------------------------------

// Achromatic identity: Cb == Cr == kChromaZero collapses both chroma deltas
// to zero, so r = y + 0 = y and b = y + 0 = y directly (no floating-point
// question either way — the delta term is exactly 0.0), and
// g = (y - Kr*y - Kb*y) / Kg = y*(1 - Kr - Kb) / Kg. This is NOT exactly y
// bit-for-bit in every intermediate: Kr + Kg + Kb == 0.299 + 0.587 + 0.114
// is 0.9999999999999999 in IEEE double, one ULP short of 1.0 — checked
// directly rather than assumed, since the project has been burned before
// (CORRECTIONS.md) by carrying forward an unverified arithmetic claim.
// Despite that, round-to-nearest recovers the exact integer Y at every
// value checked below (the raw g before rounding is always within ~1e-11
// of the true y, far under the 0.5 rounding threshold) — confirmed by
// separate calculation for each value in this list, not assumed to hold
// for values not checked.
void testAchromaticIdentity() {
    const Sample ys[] = {0, kBlack, kChromaZero, 40000, 65535};
    for (Sample y : ys) {
        const Sample Yin[1] = {y}, Cb[1] = {kChromaZero}, Cr[1] = {kChromaZero};
        Sample R[1], G[1], B[1];
        chroma::ycbcrToRgbRow(Yin, Cb, Cr, 1, R, G, B);
        CHECK_ONCE(R[0] == y);
        CHECK_ONCE(G[0] == y);
        CHECK_ONCE(B[0] == y);

        Sample Y2[1], Cb2[1], Cr2[1];
        chroma::rgbToYcbcrRow(R, G, B, 1, Y2, Cb2, Cr2);
        CHECK_ONCE(Y2[0] == y);
        CHECK_ONCE(Cb2[0] == kChromaZero);
        CHECK_ONCE(Cr2[0] == kChromaZero);
    }
}

// Known vector, forward direction, no clamp engaged: Y = 32768,
// Cb = 40960 (delta +8192 from kChromaZero), Cr = 28672 (delta -4096) — an
// arbitrary off-achromatic point, chosen only so the arithmetic is easy to
// state and lands well inside Sample's range on every channel.
//
//   crDelta = -4096, cbDelta = +8192
//   r = 32768 + 2*(1-0.299)*(-4096) = 32768 - 1.402*4096
//     = 32768 - 5742.592 = 27025.408 -> round -> 27025
//   b = 32768 + 2*(1-0.114)*8192    = 32768 + 1.772*8192
//     = 32768 + 14516.224 = 47284.224 -> round -> 47284
//   g = (32768 - 0.299*27025.408 - 0.114*47284.224) / 0.587
//     = 32873.9377717... -> round -> 32874
// (g's own formula uses the *unrounded* r/b, matching ycbcrToRgbRow's real
// order of operations: rounding happens once, at the end, on assignment to
// Sample, never mid-formula.)
void testKnownVectorForward() {
    const Sample Y[1] = {32768}, Cb[1] = {40960}, Cr[1] = {28672};
    Sample R[1], G[1], B[1];
    chroma::ycbcrToRgbRow(Y, Cb, Cr, 1, R, G, B);
    CHECK(R[0] == 27025);
    CHECK(G[0] == 32874);
    CHECK(B[0] == 47284);

    // Round trip: none of (27025, 32874, 47284) is anywhere near a clamp
    // boundary, so the reverse conversion recovers the exact original
    // triple — confirmed by an independent calculation carried at full
    // double precision (y = 32767.889..., cb = 40959.936..., cr =
    // 28671.788..., each within rounding distance of the original integer),
    // not assumed from "it round-trips so it must be fine."
    Sample Y2[1], Cb2[1], Cr2[1];
    chroma::rgbToYcbcrRow(R, G, B, 1, Y2, Cb2, Cr2);
    CHECK(Y2[0] == 32768);
    CHECK(Cb2[0] == 40960);
    CHECK(Cr2[0] == 28672);
}

// Forward clamp, low side: Y = 4096 (kBlack), Cb = kChromaZero (cbDelta=0),
// Cr = 65535 (crDelta = +32767, the maximum possible). r and b stay
// in-range (r = 4096 + 1.402*32767 = 50035.334 -> 50035; b = y = 4096
// exactly), but g's raw value is 4096 - 0.299*50035.334 - 0.114*4096, all
// over 0.587 = -19304.10..., well below Sample's [0, 65535] floor — this is
// chroma.hpp's own documented "a YCbCr triple whose implied RGB falls
// outside [0, 65535]... clips here for real" case, on the low side.
void testClampLowG() {
    const Sample Y[1] = {4096}, Cb[1] = {32768}, Cr[1] = {65535};
    Sample R[1], G[1], B[1];
    chroma::ycbcrToRgbRow(Y, Cb, Cr, 1, R, G, B);
    CHECK(R[0] == 50035);
    CHECK(G[0] == 0);
    CHECK(B[0] == 4096);
}

// Forward clamp, high side: Y = 65535, Cb = 65535 (cbDelta = +32767),
// Cr = kChromaZero (crDelta=0). r = y = 65535 exactly (no clamp needed);
// g's raw value, 54258.686..., stays in range -> 54259; b's raw value,
// 65535 + 1.772*32767 = 123598.124, is far above the ceiling and clamps to
// 65535 — the high-side counterpart to testClampLowG above.
void testClampHighB() {
    const Sample Y[1] = {65535}, Cb[1] = {65535}, Cr[1] = {32768};
    Sample R[1], G[1], B[1];
    chroma::ycbcrToRgbRow(Y, Cb, Cr, 1, R, G, B);
    CHECK(R[0] == 65535);
    CHECK(G[0] == 54259);
    CHECK(B[0] == 65535);
}

// Reverse clamp: the forward direction is not the only place a clamp can
// engage — chroma.hpp flags the boundary conversion as clamping "on both
// the forward and the round-trip-back conversion," and a pure-primary RGB
// triple (a real, legal RGB value — nothing exotic) proves it. R = 65535,
// G = 0, B = 0: y = 0.299*65535 = 19594.965 -> 19595; cb = (0 - 19594.965)/
// 1.772 + 32768 = 21709.893... -> 21710 (in range); cr = (65535 -
// 19594.965)/1.402 + 32768 = 65535.5 exactly — half a code above Sample's
// own ceiling, clamped down to 65535 rather than rounded up to 65536 (which
// would silently wrap to 0). The symmetric case, B = 65535 alone, clamps cb
// the same way; G = 65535 alone needs no clamp at all, included as the
// "ordinary" reverse case for contrast.
void testReverseClamp() {
    {
        const Sample R[1] = {65535}, G[1] = {0}, B[1] = {0};
        Sample Y[1], Cb[1], Cr[1];
        chroma::rgbToYcbcrRow(R, G, B, 1, Y, Cb, Cr);
        CHECK(Y[0] == 19595);
        CHECK(Cb[0] == 21710);
        CHECK(Cr[0] == 65535);
    }
    {
        const Sample R[1] = {0}, G[1] = {0}, B[1] = {65535};
        Sample Y[1], Cb[1], Cr[1];
        chroma::rgbToYcbcrRow(R, G, B, 1, Y, Cb, Cr);
        CHECK(Y[0] == 7471);
        CHECK(Cb[0] == 65535);
        CHECK(Cr[0] == 27439);
    }
    {
        // No clamp: y = 38469.045, cb = 11058.606..., cr = 5329.308..., all
        // comfortably inside [0, 65535].
        const Sample R[1] = {0}, G[1] = {65535}, B[1] = {0};
        Sample Y[1], Cb[1], Cr[1];
        chroma::rgbToYcbcrRow(R, G, B, 1, Y, Cb, Cr);
        CHECK(Y[0] == 38469);
        CHECK(Cb[0] == 11059);
        CHECK(Cr[0] == 5329);
    }
}

// ---------------------------------------------------------------------------
// 6. RGB boundary Image wrapper: row independence and stride handling,
//    mirroring section 4's own testImageStride pattern. Both rows below use
//    only known vectors already proven above to round-trip exactly with no
//    clamp engaged (the achromatic case, testKnownVectorForward's vector,
//    and one further non-clamping vector, independently computed the same
//    way: Y=50000, Cb=28672 (delta -4096), Cr=36864 (delta +4096) ->
//    R=55743, G=48484, B=42742, confirmed to round-trip back to
//    (50000, 28672, 36864) exactly) — so a mismatch here can only be a
//    stride/indexing bug in ycbcrToRgbImage/rgbToYcbcrImage, never a fresh
//    arithmetic question; the arithmetic and the clamp behaviour are
//    already covered, per-pixel, by the Row-level tests above.
// ---------------------------------------------------------------------------

void testRgbImageStride() {
    const int width  = 2;
    const int height = 2;
    const std::ptrdiff_t inStride  = width + 3;
    const std::ptrdiff_t outStride = width + 5;
    const Sample kSentinel = 0xBEEF;

    const Sample Yin[2][2]  = {{4096, 65535}, {32768, 50000}};
    const Sample Cbin[2][2] = {{32768, 32768}, {40960, 28672}};
    const Sample Crin[2][2] = {{32768, 32768}, {28672, 36864}};
    const Sample expectR[2][2] = {{4096, 65535}, {27025, 55743}};
    const Sample expectG[2][2] = {{4096, 65535}, {32874, 48484}};
    const Sample expectB[2][2] = {{4096, 65535}, {47284, 42742}};

    std::vector<Sample> Y(std::size_t(inStride) * std::size_t(height), kSentinel);
    std::vector<Sample> Cb(std::size_t(inStride) * std::size_t(height), kSentinel);
    std::vector<Sample> Cr(std::size_t(inStride) * std::size_t(height), kSentinel);
    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx =
                std::size_t(row) * std::size_t(inStride) + std::size_t(x);
            Y[idx]  = Yin[row][x];
            Cb[idx] = Cbin[row][x];
            Cr[idx] = Crin[row][x];
        }
    }

    std::vector<Sample> R(std::size_t(outStride) * std::size_t(height), kSentinel);
    std::vector<Sample> G(std::size_t(outStride) * std::size_t(height), kSentinel);
    std::vector<Sample> B(std::size_t(outStride) * std::size_t(height), kSentinel);
    chroma::ycbcrToRgbImage(Y.data(), Cb.data(), Cr.data(), inStride, width, height,
                            R.data(), G.data(), B.data(), outStride);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx =
                std::size_t(row) * std::size_t(outStride) + std::size_t(x);
            CHECK_ONCE(R[idx] == expectR[row][x]);
            CHECK_ONCE(G[idx] == expectG[row][x]);
            CHECK_ONCE(B[idx] == expectB[row][x]);
        }
        for (std::ptrdiff_t x = width; x < outStride; ++x) {
            const std::size_t idx =
                std::size_t(row) * std::size_t(outStride) + std::size_t(x);
            CHECK_ONCE(R[idx] == kSentinel);
            CHECK_ONCE(G[idx] == kSentinel);
            CHECK_ONCE(B[idx] == kSentinel);
        }
    }

    std::vector<Sample> Y2(std::size_t(inStride) * std::size_t(height), kSentinel);
    std::vector<Sample> Cb2(std::size_t(inStride) * std::size_t(height), kSentinel);
    std::vector<Sample> Cr2(std::size_t(inStride) * std::size_t(height), kSentinel);
    chroma::rgbToYcbcrImage(R.data(), G.data(), B.data(), outStride, width, height,
                            Y2.data(), Cb2.data(), Cr2.data(), inStride);

    for (int row = 0; row < height; ++row) {
        for (int x = 0; x < width; ++x) {
            const std::size_t idx =
                std::size_t(row) * std::size_t(inStride) + std::size_t(x);
            CHECK_ONCE(Y2[idx]  == Yin[row][x]);
            CHECK_ONCE(Cb2[idx] == Cbin[row][x]);
            CHECK_ONCE(Cr2[idx] == Crin[row][x]);
        }
        for (std::ptrdiff_t x = width; x < inStride; ++x) {
            const std::size_t idx =
                std::size_t(row) * std::size_t(inStride) + std::size_t(x);
            CHECK_ONCE(Y2[idx]  == kSentinel);
            CHECK_ONCE(Cb2[idx] == kSentinel);
            CHECK_ONCE(Cr2[idx] == kSentinel);
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

    testAchromaticIdentity();
    testKnownVectorForward();
    testClampLowG();
    testClampHighB();
    testReverseClamp();
    testRgbImageStride();

    return scatter::test::summary("test_chroma");
}
