// scatter-dve — v210 unpack and pack, scalar reference
//
// Deliberately plain. This is the oracle WU-17's NEON path is diffed against,
// so it is written to be read and believed rather than to be fast. Words are
// assembled byte by byte: no type punning, no alignment assumption, no host
// endianness assumption. Both compilers in use fold each of those into a
// single load or store.

#include "video/v210.hpp"

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

}  // namespace scatter::v210
