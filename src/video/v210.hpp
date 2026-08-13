// scatter-dve — v210 unpack and pack, scalar reference
//
// v210 is the only transport in and out of the pipeline (ADR-005). Layout is
// exactly 6 pixels per 16 bytes: four 32-bit little-endian words, three 10-bit
// components each in the low 30 bits, bits 30 and 31 unused.
//
//   word 0 : Cb0 | Y0 << 10 | Cr0 << 20
//   word 1 : Y1  | Cb2 << 10 | Y2 << 20
//   word 2 : Cr2 | Y3 << 10 | Cb4 << 20
//   word 3 : Y4  | Cr4 << 10 | Y5 << 20
//
// Planes are 4:2:2 at this stage: the luma plane is `width` samples wide, the
// two chroma planes are `width / 2`, co-sited with even luma. Upsampling to
// 4:4:4 is WU-04 and happens after unpack, not during it.
//
// I2. `pack` is the one and only clamp site in the whole pipeline: toCode10
// clamps to v210 protocol limits, 10-bit codes 4 to 1019, because codes 0-3
// and 1020-1023 are reserved for TRS and writing them would synthesise
// spurious SAV/EAV. Nothing here legalises. Sub-black, super-white and filter
// ringing between those bounds pass through untouched.
//
// Scalar only. WU-17 adds a NEON implementation which must be diffed against
// this one and produce bit-identical results.

#pragma once

#include <cstddef>
#include <cstdint>

#include "core/types.hpp"

namespace scatter::v210 {

inline constexpr int kPixelsPerGroup = 6;
inline constexpr int kBytesPerGroup  = 16;

// ---------------------------------------------------------------------------
// Geometry
//
// Row stride is NOT derivable from width in general, which is why every entry
// point below takes it as a parameter. These helpers give the two quantities
// worth naming; a caller that has a stride from a file header or from the
// DeckLink SDK should pass that instead of computing one.
// ---------------------------------------------------------------------------

// Width must be even: 4:2:2 pairs each Cb/Cr with two luma samples, and an odd
// width would leave a luma sample with no chroma partner. Width need not be a
// multiple of 6 — a short final group is padded, see kPad* below.
constexpr bool isSupportedWidth(int width) noexcept {
    return width > 0 && (width % 2) == 0;
}

// Number of pixels in the two chroma planes.
constexpr int chromaWidth(int width) noexcept {
    return width / 2;
}

// Whole 16-byte groups needed for one row, rounding up.
constexpr int groupsPerRow(int width) noexcept {
    return (width + kPixelsPerGroup - 1) / kPixelsPerGroup;
}

// Bytes occupied by one row with no trailing padding.
constexpr std::size_t rowBytesMin(int width) noexcept {
    return std::size_t(groupsPerRow(width)) * std::size_t(kBytesPerGroup);
}

// rowBytesMin rounded up to `alignment`, which must be a positive power of
// two. For 720 and 1920 rowBytesMin is already 1920 and 5120, both exact
// multiples of 128, so this returns them unchanged — but that is a property of
// those two widths, not a general one, so do not rely on it elsewhere.
constexpr std::size_t rowBytesAligned(int width,
                                      std::size_t alignment = 128) noexcept {
    const std::size_t n = rowBytesMin(width);
    return (n + alignment - 1) & ~(alignment - 1);
}

// Padding written into a short final group. Deterministic so that
// unpack -> pack is byte-identical for widths that are not a multiple of 6.
inline constexpr std::uint16_t kPadLuma   = kCode10Black;
inline constexpr std::uint16_t kPadChroma = kCode10ChromaZero;

// ---------------------------------------------------------------------------
// Row operations — the primitives WU-17 replaces
//
// Preconditions, none of them checked at runtime: isSupportedWidth(width);
// `src` holds at least rowBytesMin(width) bytes; the plane pointers hold at
// least width, chromaWidth(width) and chromaWidth(width) samples.
// ---------------------------------------------------------------------------

// Reads rowBytesMin(width) bytes. Bits 30 and 31 of every word are ignored.
// Components beyond `width` in a short final group are discarded.
void unpackRow(const std::uint8_t* src, int width,
               Sample* Y, Sample* Cb, Sample* Cr) noexcept;

// Writes exactly rowBytesMin(width) bytes and nothing past them, so trailing
// stride padding in the destination is left alone. Bits 30 and 31 are written
// zero. Rounds to nearest and applies the protocol clamp (I2).
void packRow(const Sample* Y, const Sample* Cb, const Sample* Cr, int width,
             std::uint8_t* dst) noexcept;

// ---------------------------------------------------------------------------
// Image operations
//
// Byte strides for the packed side, sample strides for the planar side. Both
// may exceed the natural row size; neither may be negative here — bottom-up
// rasters are the caller's problem, not this layer's.
// ---------------------------------------------------------------------------

void unpackImage(const std::uint8_t* src, std::ptrdiff_t srcStrideBytes,
                 int width, int height,
                 Sample* Y, std::ptrdiff_t yStrideSamples,
                 Sample* Cb, Sample* Cr, std::ptrdiff_t cStrideSamples) noexcept;

void packImage(const Sample* Y, std::ptrdiff_t yStrideSamples,
               const Sample* Cb, const Sample* Cr, std::ptrdiff_t cStrideSamples,
               int width, int height,
               std::uint8_t* dst, std::ptrdiff_t dstStrideBytes) noexcept;

}  // namespace scatter::v210
