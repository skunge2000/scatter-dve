// scatter-dve — core types
//
// Encodes invariants I2, I3 and I4 as compile-time constants and assertions.
// If a static_assert in this file fires, an invariant has been violated;
// read INVARIANTS.md before changing anything here.

#pragma once

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace scatter {

// ---------------------------------------------------------------------------
// Colour representation (I3)
//
// 16-bit offset-binary. The 10-bit source code is shifted left 6, so black
// sits at its BT.601/709 code and chroma at its achromatic code. No signed
// arithmetic anywhere in the colour path.
//
// Note: the 64 offset is NOT a pedestal. Pedestal is analogue NTSC setup,
// 7.5 IRE above blanking, absent from PAL and from all digital component
// representations. Here black and blanking are the same level and the space
// below code 64 is footroom. See C-004.
// ---------------------------------------------------------------------------

using Sample = std::uint16_t;

inline constexpr int kSourceBits  = 10;
inline constexpr int kSampleBits  = 16;
inline constexpr int kSampleShift = kSampleBits - kSourceBits;  // 6

// 10-bit code points.
inline constexpr std::uint16_t kCode10Black       = 64;    // and blanking
inline constexpr std::uint16_t kCode10WhiteNominal = 940;  // nominal, not a limit
inline constexpr std::uint16_t kCode10ChromaZero  = 512;   // achromatic

// v210 protocol limits (I2). Codes 0-3 and 1020-1023 are reserved for TRS;
// writing them would synthesise spurious SAV/EAV and break the raster. This
// is the ONLY permitted clamp in the entire pipeline, and it applies at the
// pack stage alone. Sub-black and super-white between these bounds pass
// through untouched. There is no legalisation anywhere.
inline constexpr std::uint16_t kCode10Min = 4;
inline constexpr std::uint16_t kCode10Max = 1019;

// 16-bit equivalents.
inline constexpr Sample kBlack      = Sample(kCode10Black << kSampleShift);       //  4096
inline constexpr Sample kChromaZero = Sample(kCode10ChromaZero << kSampleShift);  // 32768
inline constexpr Sample kSampleMin  = Sample(kCode10Min << kSampleShift);         //   256
inline constexpr Sample kSampleMax  = Sample(kCode10Max << kSampleShift);         // 65216

static_assert(kBlack == 4096);
static_assert(kChromaZero == 32768);

// 10-bit -> 16-bit. Lossless.
constexpr Sample fromCode10(std::uint16_t code) noexcept {
    return Sample(code << kSampleShift);
}

// 16-bit -> 10-bit, truncating. Exact inverse of fromCode10.
constexpr std::uint16_t toCode10Trunc(Sample s) noexcept {
    return std::uint16_t(s >> kSampleShift);
}

// 16-bit -> 10-bit, round to nearest, then clamp to protocol limits.
// Rounding does not disturb I7: identity-path values are exact multiples of
// (1 << kSampleShift), for which rounding and truncation agree.
constexpr std::uint16_t toCode10(Sample s) noexcept {
    const std::uint32_t rounded =
        (std::uint32_t(s) + (1u << (kSampleShift - 1))) >> kSampleShift;
    return std::uint16_t(rounded < kCode10Min ? kCode10Min
                        : rounded > kCode10Max ? kCode10Max
                                               : rounded);
}

// ---------------------------------------------------------------------------
// Weight — coverage, 1.15 fixed point
//
// Unity is 32768, so the representable range is [0, 2.0) in steps of 2^-15.
// Values above unity occur legitimately when a filter footprint overlaps.
// ---------------------------------------------------------------------------

using Weight = std::uint16_t;
inline constexpr std::uint32_t kWeightUnity = 32768;
inline constexpr std::uint32_t kWeightMax   = 65535;

// ---------------------------------------------------------------------------
// Sub-pixel position — 12.4 fixed point, tile-local
// ---------------------------------------------------------------------------

using SubPos = std::uint16_t;
inline constexpr int kSubPixelBits = 4;
inline constexpr int kSubPixelOne  = 1 << kSubPixelBits;  // 16

// ---------------------------------------------------------------------------
// Fragment record — architecture.md section 4.3
//
// Exactly 16 bytes. Fragment traffic is the dominant memory cost of pass 1,
// so the size is load-bearing, not incidental.
// ---------------------------------------------------------------------------

struct Frag {
    SubPos        x, y;         // destination, 12.4 fixed, tile-local
    Sample        R, G, B;      // 16-bit offset-binary (I3, ADR-085: RGB,
                                 // renamed from Y/Cb/Cr this session, WU-39)
    Weight        w;            // coverage, 1.15 fixed
    std::uint16_t z;            // depth, near = 0
    std::uint8_t  tag;          // priority / surface id
    std::uint8_t  reserved;
};

static_assert(sizeof(Frag) == 16, "Frag must stay 16 bytes; see architecture.md 4.3");
static_assert(std::is_trivially_copyable_v<Frag>);
static_assert(std::is_standard_layout_v<Frag>);

// ---------------------------------------------------------------------------
// Accumulators (I4, I6)
//
// Integer throughout. Integer addition is associative, so accumulation is
// bit-identical regardless of thread count, tile size or scheduling order.
// That property makes the single-threaded build a permanent oracle (ADR-015).
// Never introduce floating point into this path.
// ---------------------------------------------------------------------------

using ColourAccum = std::int64_t;
using WeightAccum = std::int32_t;

struct AccumCell {
    ColourAccum  R, G, B;  // I3, ADR-085: RGB, renamed from Y/Cb/Cr this
                            // session, WU-39
    WeightAccum  w;
    std::int32_t reserved;
};

static_assert(sizeof(AccumCell) == 32);
static_assert(alignof(AccumCell) == 8);

// Largest contribution a single fragment can make to one colour accumulator.
inline constexpr ColourAccum kMaxFragContribution =
    ColourAccum(65535) * ColourAccum(kWeightMax);  // 4 294 836 225

// C-001. One fragment fits in 32 bits with only 131 070 to spare; two do not.
// Under 32:1 compression the patent describes, of order 1000 fragments land in
// a single cell. 64-bit accumulators are not defensive, they are required.
static_assert(2 * kMaxFragContribution > ColourAccum(UINT32_MAX),
              "C-001: two full-weight fragments overflow 32 bits");
static_assert(kMaxFragContribution <= INT64_MAX / 1000000,
              "int64 must absorb a million full-weight fragments per cell");

// ---------------------------------------------------------------------------
// K-buffer storage (WU-28a; DECISIONS.md ADR-059) — front/back
// occlusion/transparency accumulation
//
// New, additive alongside AccumCell above. A cell touched by more than one
// surface (e.g. a folding sphere's own front and back) needs up to
// kBufferK distinct Frag::tag slots instead of one. Keyed by tag, not by
// z: Frag::z's 16-bit quantisation makes exact ties between same-surface
// fragments routine, so keying by z during accumulation would make
// eviction order-sensitive on ties within one surface. Same-tag
// contributions accumulate into their one shared slot via today's
// order-independent accumulateCorner() (I4/I6, unchanged); z is used only
// once, at resolve time (WU-28b), to sort the occupied slots front-to-back.
// ---------------------------------------------------------------------------

// Fixed per-cell ceiling on distinct simultaneously-tracked tags (ADR-059's
// "headroom beyond the minimal front/back case"). Raisable by a later unit.
inline constexpr int kBufferK = 4;

// One tag-keyed slot in a cell's k-buffer. `occupied` false means free --
// unclaimed, or freed by eviction. `firstSeenZ` is the depth of whichever
// fragment first claimed this slot for its tag; splat.cpp's
// splatTileKBuffer() uses it only to pick the farthest-so-far occupied
// slot to evict on overflow, never to break ties within one tag's own sum.
struct KSlot {
    std::uint8_t  tag = 0;
    bool          occupied = false;
    std::uint16_t firstSeenZ = 0;
    AccumCell     cell{};
};

// ---------------------------------------------------------------------------
// Tiling (ADR-002, open question Q1)
//
// Tile size is a compile-time constant so WU-09 can benchmark both candidates
// without restructuring. Q1 is deliberately undecided:
//   kTileLog2 = 4  ->  16x16,  32 KB across four banks, fits M1 L1D
//   kTileLog2 = 5  ->  32x32, 128 KB, exactly L1D, less edge replication
// ---------------------------------------------------------------------------

#ifndef SCATTER_TILE_LOG2
#define SCATTER_TILE_LOG2 5
#endif

inline constexpr int kTileLog2  = SCATTER_TILE_LOG2;
inline constexpr int kTileSize  = 1 << kTileLog2;
inline constexpr int kTilePixels = kTileSize * kTileSize;

static_assert(kTileLog2 >= 3 && kTileLog2 <= 7, "implausible tile size");

// Four banks, per ADR-002: a single accumulator needs four sequential
// read-modify-writes per fragment and serialises on store-to-load latency.
// Four banks give four independent chains that pipeline. Quantel's
// decomposition, retained for the modern equivalent of the original reason.
inline constexpr int kBanks = 4;

inline constexpr std::size_t kTileAccumBytes =
    std::size_t(kTilePixels) * kBanks * sizeof(AccumCell);

// SubPos is 12.4, so tile-local coordinates must fit 12 integer bits.
static_assert(kTileSize <= 4096);

}  // namespace scatter
