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
// Row operations — the scalar reference WU-17's NEON path is diffed against
//
// Preconditions, none of them checked at runtime: isSupportedWidth(width);
// `src` holds at least rowBytesMin(width) bytes; the plane pointers hold at
// least width, chromaWidth(width) and chromaWidth(width) samples. WU-17 adds
// NEON-suffixed siblings below with the same preconditions and the same
// short-final-group behaviour (ADR-018) — these scalar entry points are
// never modified or replaced; see ADR-042.
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

// ---------------------------------------------------------------------------
// packBlackFrame — WU-21d, DECISIONS.md ADR-064
//
// Writes a full black-and-neutral-chroma frame: kCode10Black (core/types.hpp,
// via kBlack) at every Y sample, kCode10ChromaZero (via kChromaZero) at every
// Cb/Cr sample — the offset-binary "no signal yet" colour (I3), not a
// caller-supplied pattern. `dst` must hold at least rowBytesMin(width) *
// height bytes, tight-packed (row stride is exactly rowBytesMin(width), no
// alignment padding) — the same layout LiveFramePlayback's own pool buffers
// and CaptureConsumer::copyLatestFrame() already use
// (src/io/decklink_live_output.cpp/decklink_capture_consumer.cpp), which is
// this function's own first caller: a LiveFramePlayback pool buffer,
// scheduled during preroll before CaptureConsumer has produced its own first
// output, otherwise carries whatever CreateVideoFrame() first allocated it
// with — effectively zero-filled v210, which decodes as solid green on a
// real monitor, not black.
//
// Every row's packed bytes are identical by construction (the same three
// planar values at every sample, on every row), so only the first row is
// actually packed by packRow; the remaining rows are memcpy'd from it, not
// repacked. Applies I2's protocol clamp exactly as packRow always does — both
// kCode10Black and kCode10ChromaZero already sit inside [kCode10Min,
// kCode10Max], so the clamp never actually engages here, but this function
// does not special-case around it either. Not noexcept: unlike every other
// entry point in this file, it allocates one row's worth of planar scratch
// space internally rather than taking caller-owned buffers, the same
// allocating-convenience trade-off Raster422/Raster444 (video/raster.hpp)
// already make elsewhere in this project.
// ---------------------------------------------------------------------------

void packBlackFrame(int width, int height, std::uint8_t* dst);

// ---------------------------------------------------------------------------
// NEON operations — WU-17, ADR-042
//
// Present only when __ARM_NEON is defined (AArch64 always defines it; no
// -mfpu flag needed, unlike 32-bit ARM). Absent from any x86_64 build —
// scatter-core's default Linux-sandbox matrix is unaffected, exactly as
// ADR-031's BLACKMAGIC_SDK_DIR guard keeps that matrix unaffected by a
// different Apple/ARM-only surface.
//
// Same signatures, same preconditions, same short-final-group behaviour
// (ADR-018) as the scalar functions immediately above — these vectorise the
// v210 bit-layout codec only (the 10-bit-field <-> 32-bit-word packing).
// toCode10/fromCode10 (the I2 round-and-clamp / offset-binary shift) are
// called identically on both paths, not reimplemented here. Bit-identical to
// the scalar reference is this unit's entire accept criterion
// (WORK-UNITS.md); tests/test_v210_neon.cpp checks it directly.
// ---------------------------------------------------------------------------

#if defined(__ARM_NEON)

void unpackRowNeon(const std::uint8_t* src, int width,
                   Sample* Y, Sample* Cb, Sample* Cr) noexcept;

void packRowNeon(const Sample* Y, const Sample* Cb, const Sample* Cr, int width,
                 std::uint8_t* dst) noexcept;

void unpackImageNeon(const std::uint8_t* src, std::ptrdiff_t srcStrideBytes,
                     int width, int height,
                     Sample* Y, std::ptrdiff_t yStrideSamples,
                     Sample* Cb, Sample* Cr, std::ptrdiff_t cStrideSamples) noexcept;

void packImageNeon(const Sample* Y, std::ptrdiff_t yStrideSamples,
                   const Sample* Cb, const Sample* Cr, std::ptrdiff_t cStrideSamples,
                   int width, int height,
                   std::uint8_t* dst, std::ptrdiff_t dstStrideBytes) noexcept;

#endif  // __ARM_NEON

}  // namespace scatter::v210
