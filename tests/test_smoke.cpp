// WU-01 smoke test.
//
// Everything here is also a static_assert in types.hpp. Re-checking at runtime
// is not redundant: it proves the header was actually compiled and linked into
// the test binary, and it gives a failure message rather than a build error if
// someone weakens a constant later.

#include "core/types.hpp"
#include "harness.hpp"

#include <cstdint>

using namespace scatter;

static void test_colour_constants() {
    CHECK(kSampleShift == 6);
    CHECK(kBlack == 4096);
    CHECK(kChromaZero == 32768);
    CHECK(kSampleMin == 256);
    CHECK(kSampleMax == 65216);

    // Nominal white is nominal, not a ceiling. Super-white lives above it and
    // must remain representable (I2).
    CHECK(fromCode10(kCode10WhiteNominal) < kSampleMax);
}

static void test_code10_roundtrip() {
    // 10 -> 16 -> 10 must be exact across the whole code space, including the
    // reserved values, which fromCode10 does not clamp.
    for (std::uint32_t c = 0; c < 1024; ++c) {
        const Sample s = fromCode10(std::uint16_t(c));
        CHECK_ONCE(toCode10Trunc(s) == c);
    }

    // The rounding converter agrees with truncation on every value that came
    // from fromCode10, which is what keeps I7 intact, and clamps only to the
    // protocol limits.
    for (std::uint32_t c = 0; c < 1024; ++c) {
        const Sample s = fromCode10(std::uint16_t(c));
        const std::uint32_t expect = c < kCode10Min ? kCode10Min
                                   : c > kCode10Max ? kCode10Max
                                                    : c;
        CHECK_ONCE(toCode10(s) == expect);
    }
}

static void test_protocol_clamp_only() {
    // Sub-black and super-white survive. Only the TRS-reserved codes move.
    CHECK(toCode10(fromCode10(4)) == 4);        // just above reserved
    CHECK(toCode10(fromCode10(16)) == 16);      // sub-black
    CHECK(toCode10(fromCode10(63)) == 63);      // sub-black
    CHECK(toCode10(fromCode10(1019)) == 1019);  // super-white, just below reserved
    CHECK(toCode10(fromCode10(0)) == kCode10Min);     // reserved, clamped
    CHECK(toCode10(fromCode10(1023)) == kCode10Max);  // reserved, clamped

    // Rounding at the top of the range must not wrap past the protocol limit.
    CHECK(toCode10(65535) == kCode10Max);
}

static void test_fragment_layout() {
    CHECK(sizeof(Frag) == 16);

    Frag f{};
    CHECK(f.x == 0 && f.y == 0 && f.w == 0 && f.tag == 0);

    f.x = SubPos(3 * kSubPixelOne + 8);  // x = 3.5
    CHECK(f.x >> kSubPixelBits == 3);
    CHECK((f.x & (kSubPixelOne - 1)) == 8);
}

static void test_accumulator_headroom() {
    CHECK(sizeof(AccumCell) == 32);

    // C-001 made executable. One full-weight fragment fits in 32 bits with
    // 131 070 to spare; two do not. Under the 32:1 compression the patent
    // describes, of order 1000 fragments reach a single cell.
    CHECK(kMaxFragContribution < ColourAccum(UINT32_MAX));
    CHECK(2 * kMaxFragContribution > ColourAccum(UINT32_MAX));

    ColourAccum acc = 0;
    for (int i = 0; i < 1000; ++i) acc += kMaxFragContribution;
    CHECK(acc > 0);                       // no overflow to negative
    CHECK(acc == 1000 * kMaxFragContribution);
    CHECK(acc < INT64_MAX / 1000);        // three more orders of headroom
}

static void test_tiling() {
    CHECK(kTileSize == (1 << kTileLog2));
    CHECK(kTilePixels == kTileSize * kTileSize);
    CHECK(kBanks == 4);
    CHECK(kTileAccumBytes ==
          std::size_t(kTilePixels) * kBanks * sizeof(AccumCell));

    // Q1 is open; both candidates must remain buildable and plausible.
    CHECK(kTileAccumBytes == 32u * 1024u || kTileAccumBytes == 128u * 1024u);

    std::printf("  tile %dx%d, %d banks, accumulator %zu KB\n",
                kTileSize, kTileSize, kBanks, kTileAccumBytes / 1024);
}

int main() {
    test_colour_constants();
    test_code10_roundtrip();
    test_protocol_clamp_only();
    test_fragment_layout();
    test_accumulator_headroom();
    test_tiling();
    return scatter::test::summary("test_smoke");
}
