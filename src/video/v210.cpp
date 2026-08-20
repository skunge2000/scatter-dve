// scatter-dve — v210 unpack and pack, scalar reference
//
// Deliberately plain. This is the oracle WU-17's NEON path is diffed against,
// so it is written to be read and believed rather than to be fast. Words are
// assembled byte by byte: no type punning, no alignment assumption, no host
// endianness assumption. Both compilers in use fold each of those into a
// single load or store.

#include "video/v210.hpp"

#include <cstring>
#include <vector>

#if defined(__ARM_NEON)
#include <arm_neon.h>  // file scope, not nested in any namespace -- WU-17, ADR-042
#endif

namespace scatter::v210 {
namespace {

constexpr std::uint32_t kComponentMask = 0x3FFu;

inline std::uint32_t rd32le(const std::uint8_t* p) noexcept {
    return std::uint32_t(p[0])
         | (std::uint32_t(p[1]) << 8)
         | (std::uint32_t(p[2]) << 16)
         | (std::uint32_t(p[3]) << 24);
}

inline void wr32le(std::uint8_t* p, std::uint32_t v) noexcept {
    p[0] = std::uint8_t(v & 0xFFu);
    p[1] = std::uint8_t((v >> 8) & 0xFFu);
    p[2] = std::uint8_t((v >> 16) & 0xFFu);
    p[3] = std::uint8_t((v >> 24) & 0xFFu);
}

inline std::uint16_t component(std::uint32_t word, int shift) noexcept {
    return std::uint16_t((word >> shift) & kComponentMask);
}

// Bits 30 and 31 are unused and written zero.
inline std::uint32_t assemble(std::uint16_t a, std::uint16_t b,
                              std::uint16_t c) noexcept {
    return (std::uint32_t(a) & kComponentMask)
         | ((std::uint32_t(b) & kComponentMask) << 10)
         | ((std::uint32_t(c) & kComponentMask) << 20);
}

// One 16-byte group as 10-bit codes, in raster order.
struct Group {
    std::uint16_t y[6];
    std::uint16_t cb[3];
    std::uint16_t cr[3];
};

inline Group readGroup(const std::uint8_t* p) noexcept {
    const std::uint32_t w0 = rd32le(p);
    const std::uint32_t w1 = rd32le(p + 4);
    const std::uint32_t w2 = rd32le(p + 8);
    const std::uint32_t w3 = rd32le(p + 12);

    Group g{};
    g.cb[0] = component(w0,  0);
    g.y[0]  = component(w0, 10);
    g.cr[0] = component(w0, 20);

    g.y[1]  = component(w1,  0);
    g.cb[1] = component(w1, 10);
    g.y[2]  = component(w1, 20);

    g.cr[1] = component(w2,  0);
    g.y[3]  = component(w2, 10);
    g.cb[2] = component(w2, 20);

    g.y[4]  = component(w3,  0);
    g.cr[2] = component(w3, 10);
    g.y[5]  = component(w3, 20);
    return g;
}

inline void writeGroup(std::uint8_t* p, const Group& g) noexcept {
    wr32le(p,      assemble(g.cb[0], g.y[0],  g.cr[0]));
    wr32le(p + 4,  assemble(g.y[1],  g.cb[1], g.y[2]));
    wr32le(p + 8,  assemble(g.cr[1], g.y[3],  g.cb[2]));
    wr32le(p + 12, assemble(g.y[4],  g.cr[2], g.y[5]));
}

}  // namespace

void unpackRow(const std::uint8_t* src, int width,
               Sample* Y, Sample* Cb, Sample* Cr) noexcept {
    const int groups = groupsPerRow(width);
    const int cw     = chromaWidth(width);

    for (int gi = 0; gi < groups; ++gi) {
        const Group g = readGroup(src + std::ptrdiff_t(gi) * kBytesPerGroup);
        const int y0 = gi * kPixelsPerGroup;
        const int c0 = gi * 3;

        if (y0 + kPixelsPerGroup <= width) {
            Y[y0 + 0] = fromCode10(g.y[0]);
            Y[y0 + 1] = fromCode10(g.y[1]);
            Y[y0 + 2] = fromCode10(g.y[2]);
            Y[y0 + 3] = fromCode10(g.y[3]);
            Y[y0 + 4] = fromCode10(g.y[4]);
            Y[y0 + 5] = fromCode10(g.y[5]);

            Cb[c0 + 0] = fromCode10(g.cb[0]);
            Cb[c0 + 1] = fromCode10(g.cb[1]);
            Cb[c0 + 2] = fromCode10(g.cb[2]);

            Cr[c0 + 0] = fromCode10(g.cr[0]);
            Cr[c0 + 1] = fromCode10(g.cr[1]);
            Cr[c0 + 2] = fromCode10(g.cr[2]);
        } else {
            // Short final group. Everything past the end of the row is
            // padding the encoder chose and is discarded here.
            for (int k = 0; k < 6; ++k) {
                if (y0 + k < width) Y[y0 + k] = fromCode10(g.y[k]);
            }
            for (int k = 0; k < 3; ++k) {
                if (c0 + k < cw) {
                    Cb[c0 + k] = fromCode10(g.cb[k]);
                    Cr[c0 + k] = fromCode10(g.cr[k]);
                }
            }
        }
    }
}

void packRow(const Sample* Y, const Sample* Cb, const Sample* Cr, int width,
             std::uint8_t* dst) noexcept {
    const int groups = groupsPerRow(width);
    const int cw     = chromaWidth(width);

    for (int gi = 0; gi < groups; ++gi) {
        const int y0 = gi * kPixelsPerGroup;
        const int c0 = gi * 3;

        Group g{};
        if (y0 + kPixelsPerGroup <= width) {
            g.y[0] = toCode10(Y[y0 + 0]);
            g.y[1] = toCode10(Y[y0 + 1]);
            g.y[2] = toCode10(Y[y0 + 2]);
            g.y[3] = toCode10(Y[y0 + 3]);
            g.y[4] = toCode10(Y[y0 + 4]);
            g.y[5] = toCode10(Y[y0 + 5]);

            g.cb[0] = toCode10(Cb[c0 + 0]);
            g.cb[1] = toCode10(Cb[c0 + 1]);
            g.cb[2] = toCode10(Cb[c0 + 2]);

            g.cr[0] = toCode10(Cr[c0 + 0]);
            g.cr[1] = toCode10(Cr[c0 + 1]);
            g.cr[2] = toCode10(Cr[c0 + 2]);
        } else {
            for (int k = 0; k < 6; ++k) {
                g.y[k] = (y0 + k < width) ? toCode10(Y[y0 + k]) : kPadLuma;
            }
            for (int k = 0; k < 3; ++k) {
                const bool have = (c0 + k) < cw;
                g.cb[k] = have ? toCode10(Cb[c0 + k]) : kPadChroma;
                g.cr[k] = have ? toCode10(Cr[c0 + k]) : kPadChroma;
            }
        }

        writeGroup(dst + std::ptrdiff_t(gi) * kBytesPerGroup, g);
    }
}

void unpackImage(const std::uint8_t* src, std::ptrdiff_t srcStrideBytes,
                 int width, int height,
                 Sample* Y, std::ptrdiff_t yStrideSamples,
                 Sample* Cb, Sample* Cr,
                 std::ptrdiff_t cStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        unpackRow(src + r * srcStrideBytes, width,
                  Y + r * yStrideSamples,
                  Cb + r * cStrideSamples,
                  Cr + r * cStrideSamples);
    }
}

void packImage(const Sample* Y, std::ptrdiff_t yStrideSamples,
               const Sample* Cb, const Sample* Cr,
               std::ptrdiff_t cStrideSamples,
               int width, int height,
               std::uint8_t* dst, std::ptrdiff_t dstStrideBytes) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        packRow(Y + r * yStrideSamples,
                Cb + r * cStrideSamples,
                Cr + r * cStrideSamples,
                width,
                dst + r * dstStrideBytes);
    }
}

// packBlackFrame -- WU-21d, DECISIONS.md ADR-064. See video/v210.hpp's own
// declaration comment for the full design.
void packBlackFrame(int width, int height, std::uint8_t* dst) {
    const int cw = chromaWidth(width);
    std::vector<Sample> Y(std::size_t(width), kBlack);
    std::vector<Sample> Cb(std::size_t(cw), kChromaZero);
    std::vector<Sample> Cr(std::size_t(cw), kChromaZero);

    const std::size_t stride = rowBytesMin(width);
    packRow(Y.data(), Cb.data(), Cr.data(), width, dst);
    for (int row = 1; row < height; ++row) {
        std::memcpy(dst + std::size_t(row) * stride, dst, stride);
    }
}

// ---------------------------------------------------------------------------
// NEON — WU-17, ADR-042
//
// readGroup/writeGroup above decode/encode one 16-byte group's twelve 10-bit
// fields with no dependency on `width` at all (the short-final-group logic
// lives entirely in unpackRow/packRow, deciding how much of an already-
// decoded group to write out or pad); readGroupNeon/writeGroupNeon below are
// therefore drop-in, width-independent replacements for exactly that one
// step, and unpackRowNeon/packRowNeon are unpackRow/packRow's own loop
// bodies with only that one substitution — same short-group branching,
// same fromCode10/toCode10 calls, copied rather than shared to keep this
// block self-contained under its own #if.
//
// v210's layout puts 4 words per group, one word per NEON lane in a single
// 128-bit register: mask+shift extracts each word's three 10-bit fields for
// all 4 words in three vector ops, replacing readGroup's 12 scalar
// component() calls. Which field (A/B/C, i.e. bits 0/10/20) holds which of
// Y/Cb/Cr differs per lane (see the per-lane comments below) — an
// irregular interleave with no clean vector shuffle across it, the same
// "no scatter instruction" reasoning architecture.md already gives for the
// splat (see docs/architecture.md, bin-traffic section) — so the field ->
// Y/Cb/Cr placement itself is 12 scalar lane reads/writes, same count as
// readGroup's own 12 component() calls, but operating on values already
// mask/shift-computed in bulk rather than one word at a time.
// ---------------------------------------------------------------------------

#if defined(__ARM_NEON)

namespace {

inline Group readGroupNeon(const std::uint8_t* p) noexcept {
    // One group is exactly 4 little-endian uint32 words = 16 bytes = one
    // 128-bit NEON register. AArch64 is little-endian only (both the M1 Max
    // and every other AArch64 target this project builds for), so this load
    // matches rd32le's own explicit little-endian assumption rather than
    // adding a new one.
    const uint32x4_t words = vld1q_u32(reinterpret_cast<const std::uint32_t*>(p));
    const uint32x4_t mask  = vdupq_n_u32(kComponentMask);
    const uint32x4_t fieldA = vandq_u32(words, mask);
    const uint32x4_t fieldB = vandq_u32(vshrq_n_u32(words, 10), mask);
    const uint32x4_t fieldC = vandq_u32(vshrq_n_u32(words, 20), mask);

    std::uint32_t a[4], b[4], c[4];
    vst1q_u32(a, fieldA);
    vst1q_u32(b, fieldB);
    vst1q_u32(c, fieldC);

    Group g{};
    // lane 0 = word0: a=Cb0 b=Y0 c=Cr0  (component(w0,0/10/20))
    g.cb[0] = std::uint16_t(a[0]); g.y[0]  = std::uint16_t(b[0]); g.cr[0] = std::uint16_t(c[0]);
    // lane 1 = word1: a=Y1 b=Cb1 c=Y2
    g.y[1]  = std::uint16_t(a[1]); g.cb[1] = std::uint16_t(b[1]); g.y[2]  = std::uint16_t(c[1]);
    // lane 2 = word2: a=Cr1 b=Y3 c=Cb2
    g.cr[1] = std::uint16_t(a[2]); g.y[3]  = std::uint16_t(b[2]); g.cb[2] = std::uint16_t(c[2]);
    // lane 3 = word3: a=Y4 b=Cr2 c=Y5
    g.y[4]  = std::uint16_t(a[3]); g.cr[2] = std::uint16_t(b[3]); g.y[5]  = std::uint16_t(c[3]);
    return g;
}

inline void writeGroupNeon(std::uint8_t* p, const Group& g) noexcept {
    // Gather into the same per-word field-A/B/C layout assemble() reads,
    // then vectorise the shift/or that builds all 4 words at once.
    std::uint32_t a[4], b[4], c[4];
    a[0] = g.cb[0]; b[0] = g.y[0];  c[0] = g.cr[0];
    a[1] = g.y[1];  b[1] = g.cb[1]; c[1] = g.y[2];
    a[2] = g.cr[1]; b[2] = g.y[3];  c[2] = g.cb[2];
    a[3] = g.y[4];  b[3] = g.cr[2]; c[3] = g.y[5];

    const uint32x4_t mask = vdupq_n_u32(kComponentMask);
    uint32x4_t wordsVec = vandq_u32(vld1q_u32(a), mask);
    wordsVec = vorrq_u32(wordsVec, vshlq_n_u32(vandq_u32(vld1q_u32(b), mask), 10));
    wordsVec = vorrq_u32(wordsVec, vshlq_n_u32(vandq_u32(vld1q_u32(c), mask), 20));

    // Byte-exact little-endian store via wr32le, matching writeGroup exactly
    // -- not a direct vst1q_u32 to `p`, which would depend on the store
    // itself being little-endian rather than stating it (same discipline
    // the scalar assemble()/wr32le pair already uses).
    std::uint32_t w[4];
    vst1q_u32(w, wordsVec);
    wr32le(p,      w[0]);
    wr32le(p + 4,  w[1]);
    wr32le(p + 8,  w[2]);
    wr32le(p + 12, w[3]);
}

}  // namespace

void unpackRowNeon(const std::uint8_t* src, int width,
                   Sample* Y, Sample* Cb, Sample* Cr) noexcept {
    const int groups = groupsPerRow(width);
    const int cw     = chromaWidth(width);

    for (int gi = 0; gi < groups; ++gi) {
        const Group g = readGroupNeon(src + std::ptrdiff_t(gi) * kBytesPerGroup);
        const int y0 = gi * kPixelsPerGroup;
        const int c0 = gi * 3;

        if (y0 + kPixelsPerGroup <= width) {
            Y[y0 + 0] = fromCode10(g.y[0]);
            Y[y0 + 1] = fromCode10(g.y[1]);
            Y[y0 + 2] = fromCode10(g.y[2]);
            Y[y0 + 3] = fromCode10(g.y[3]);
            Y[y0 + 4] = fromCode10(g.y[4]);
            Y[y0 + 5] = fromCode10(g.y[5]);

            Cb[c0 + 0] = fromCode10(g.cb[0]);
            Cb[c0 + 1] = fromCode10(g.cb[1]);
            Cb[c0 + 2] = fromCode10(g.cb[2]);

            Cr[c0 + 0] = fromCode10(g.cr[0]);
            Cr[c0 + 1] = fromCode10(g.cr[1]);
            Cr[c0 + 2] = fromCode10(g.cr[2]);
        } else {
            for (int k = 0; k < 6; ++k) {
                if (y0 + k < width) Y[y0 + k] = fromCode10(g.y[k]);
            }
            for (int k = 0; k < 3; ++k) {
                if (c0 + k < cw) {
                    Cb[c0 + k] = fromCode10(g.cb[k]);
                    Cr[c0 + k] = fromCode10(g.cr[k]);
                }
            }
        }
    }
}

void packRowNeon(const Sample* Y, const Sample* Cb, const Sample* Cr, int width,
                 std::uint8_t* dst) noexcept {
    const int groups = groupsPerRow(width);
    const int cw     = chromaWidth(width);

    for (int gi = 0; gi < groups; ++gi) {
        const int y0 = gi * kPixelsPerGroup;
        const int c0 = gi * 3;

        Group g{};
        if (y0 + kPixelsPerGroup <= width) {
            g.y[0] = toCode10(Y[y0 + 0]);
            g.y[1] = toCode10(Y[y0 + 1]);
            g.y[2] = toCode10(Y[y0 + 2]);
            g.y[3] = toCode10(Y[y0 + 3]);
            g.y[4] = toCode10(Y[y0 + 4]);
            g.y[5] = toCode10(Y[y0 + 5]);

            g.cb[0] = toCode10(Cb[c0 + 0]);
            g.cb[1] = toCode10(Cb[c0 + 1]);
            g.cb[2] = toCode10(Cb[c0 + 2]);

            g.cr[0] = toCode10(Cr[c0 + 0]);
            g.cr[1] = toCode10(Cr[c0 + 1]);
            g.cr[2] = toCode10(Cr[c0 + 2]);
        } else {
            for (int k = 0; k < 6; ++k) {
                g.y[k] = (y0 + k < width) ? toCode10(Y[y0 + k]) : kPadLuma;
            }
            for (int k = 0; k < 3; ++k) {
                const bool have = (c0 + k) < cw;
                g.cb[k] = have ? toCode10(Cb[c0 + k]) : kPadChroma;
                g.cr[k] = have ? toCode10(Cr[c0 + k]) : kPadChroma;
            }
        }

        writeGroupNeon(dst + std::ptrdiff_t(gi) * kBytesPerGroup, g);
    }
}

void unpackImageNeon(const std::uint8_t* src, std::ptrdiff_t srcStrideBytes,
                     int width, int height,
                     Sample* Y, std::ptrdiff_t yStrideSamples,
                     Sample* Cb, Sample* Cr,
                     std::ptrdiff_t cStrideSamples) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        unpackRowNeon(src + r * srcStrideBytes, width,
                      Y + r * yStrideSamples,
                      Cb + r * cStrideSamples,
                      Cr + r * cStrideSamples);
    }
}

void packImageNeon(const Sample* Y, std::ptrdiff_t yStrideSamples,
                   const Sample* Cb, const Sample* Cr,
                   std::ptrdiff_t cStrideSamples,
                   int width, int height,
                   std::uint8_t* dst, std::ptrdiff_t dstStrideBytes) noexcept {
    for (int row = 0; row < height; ++row) {
        const std::ptrdiff_t r = row;
        packRowNeon(Y + r * yStrideSamples,
                    Cb + r * cStrideSamples,
                    Cr + r * cStrideSamples,
                    width,
                    dst + r * dstStrideBytes);
    }
}

#endif  // __ARM_NEON

}  // namespace scatter::v210
