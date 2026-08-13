// scatter-dve — WU-08: fragment generation and tile binning
// (architecture.md section 3, "PASS 1: fragment generation"; sections 4.3,
// 4.4 and 4.6; module layout section 8 names this core/binner.hpp/.cpp)
//
// This is the first half of the two-pass pipeline the signal path (section
// 3) describes: for every source sample, evaluate the lattice and its
// Jacobian (core/lattice.hpp, WU-06), derive K (core/jacobian.hpp, WU-07),
// decide how many fragments that one sample becomes under magnification
// (4.6), and bin each resulting Frag (core/types.hpp's 16-byte record,
// 4.3) into every destination tile its four-bank splat footprint (4.5)
// touches (4.4). Pass 2 — the splat itself, reading these bins — is WU-09;
// this unit produces exactly what WU-09 will consume and does not
// accumulate anything.
//
// architecture.md leaves several concrete choices open here, the same kind
// of gap ADR-020/022/023 filled for earlier units. This session's choices
// are ADR-024 in DECISIONS.md:
//
// - How a source pixel index maps to a continuous lattice parameter
//   (u, v). 4.1 says only that the lattice "covers the source raster".
// - The tile-local coordinate encoding for a fragment replicated into a
//   neighbouring tile (4.4's "footprint straddles a tile boundary").
//   SubPos is unsigned, so a fragment landing outside its home tile by up
//   to the one pixel the 2x2 splat footprint reaches (4.5) cannot be
//   stored as a negative value directly; see encodeTileLocal() below.
// - The numeric supersampling thresholds and the escalation from 2x2 to
//   4x4 (4.6 fixes the *shape* of the fix, not the numbers), anchored
//   where possible to 4.6's own literal "det J > 1".
// - K and the supersampling decision use the Jacobian with respect to
//   *source pixels*, obtained from core/lattice.hpp's own (u, v)
//   Jacobian by the constant per-axis chain-rule scale factor
//   pixelToLattice() applies going the other way — not the raw
//   lattice-parameter-space Jacobian 4.2's formula names directly, which
//   would make K depend on source resolution instead of on the warp.
//
// Double precision throughout fragment *generation* (lattice eval,
// Jacobian, K, source-colour interpolation), the same layering
// core/lattice.hpp and core/jacobian.hpp already use and for the same
// reason: this runs once per source sample, not in the fixed-point I4/I6
// accumulation path. Quantisation into the Frag record's fixed-point
// fields happens once, at the end, per fragment — after that point
// everything downstream (WU-09's splat) is integer-only, as I6 requires.
#pragma once

#include "core/jacobian.hpp"
#include "core/lattice.hpp"
#include "core/types.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace scatter {

// Number of tiles needed to cover a destination raster of extent `dim`:
// ceil(dim / kTileSize). A raster whose dimension is not a multiple of
// kTileSize leaves the last tile's high-index cells uncovered by any valid
// destination pixel; TileBins does not need to know this, since a
// fragment's own destination coordinate is what determines which tile (and
// which cell within it) it lands in.
constexpr int tileCount(int dim) noexcept {
    return (dim + kTileSize - 1) / kTileSize;
}

// Source raster for fragment generation: three full-rate, already
// chroma-upsampled 4:4:4 planes. architecture.md section 3's signal path
// runs "chroma upsample 4:2:2->4:4:4" before "PASS 1: fragment
// generation", so binner.hpp never sees 4:2:2 data — that conversion
// (src/video/chroma.hpp, WU-04) has already happened by the time this
// runs. Row-major, width*height samples per plane, no stride padding.
struct SourceRaster {
    int width = 0;
    int height = 0;
    const Sample* y  = nullptr;
    const Sample* cb = nullptr;
    const Sample* cr = nullptr;
};

// architecture.md 4.6: "Subdivide each source sample into 2x2 or 4x4
// sub-samples when det J exceeds a threshold... Cap the subdivision
// factor." Two thresholds select among N = 1 (no subdivision), 2 or 4;
// maxSupersample is the hard cap 4.6 requires so unbounded magnification
// cannot generate an unbounded fragment count. See ADR-024 for why
// threshold2x2 defaults to exactly 1.0 (4.6's own literal "det J > 1")
// and threshold4x4 to 4.0.
struct SupersampleConfig {
    double threshold2x2   = 1.0;
    double threshold4x4   = 4.0;
    int    maxSupersample = 4;  // 1, 2 or 4
};

// Per-tile fragment bins covering a destWidth x destHeight destination
// raster. Tiles are addressed by (tx, ty), tx in [0, tilesX), ty in
// [0, tilesY); tile size is the project-wide compile-time constant
// kTileSize (core/types.hpp), not configured here.
class TileBins {
public:
    TileBins(int destWidth, int destHeight);

    int destWidth()  const noexcept { return destWidth_; }
    int destHeight() const noexcept { return destHeight_; }
    int tilesX() const noexcept { return tilesX_; }
    int tilesY() const noexcept { return tilesY_; }

    const std::vector<Frag>& tile(int tx, int ty) const noexcept;
    std::vector<Frag>&       tile(int tx, int ty) noexcept;

private:
    int destWidth_;
    int destHeight_;
    int tilesX_;
    int tilesY_;
    std::vector<std::vector<Frag>> bins_;  // row-major, tilesX_ * tilesY_
};

// Outcome counters from generateFragments(), so tests/test_binner.cpp can
// check WORK-UNITS.md's WU-08 accept criteria directly rather than
// re-deriving them by walking every tile's contents:
//
// - "fragment count equals source samples under compression": compare
//   sourceSamples (and primaryFragments, always equal to it by
//   construction — see below) against width*height when no source sample
//   was subdivided.
// - "boundary straddling replicates into exactly the right neighbours":
//   replicaFragments counts the extra copies 4.4 describes; which tiles
//   they land in is checked against TileBins' contents directly.
// - "no fragment lost or duplicated within a tile": droppedOffRaster
//   accounts for every sub-sample that does not become a primary
//   fragment, so sourceSamples == primaryFragments + droppedOffRaster
//   always; duplication is checked against TileBins' contents.
struct BinStats {
    std::size_t sourceSamples    = 0;  // sub-samples generated: sum of N*N over every source pixel
    std::size_t primaryFragments = 0;  // == sourceSamples - droppedOffRaster, always, by construction
    std::size_t replicaFragments = 0;  // extra copies for tiles a fragment's splat footprint straddles into
    std::size_t droppedOffRaster = 0;  // sub-samples whose destination fell outside the raster; no Frag emitted
};

// Pass 1: generate and bin fragments for an entire source raster.
//
// lattice maps continuous lattice-parameter (u, v) to destination (x, y,
// z) and its Jacobian (core/lattice.hpp); src supplies the per-pixel
// source colour; maxK is densityCompensation()'s compression clamp
// (core/jacobian.hpp, ADR-023), passed through unchanged — WU-08 has no
// more grounds to invent an operating point than WU-07 did; ss configures
// 4.6's adaptive supersampling; tag is copied into every emitted Frag
// unchanged (priority/surface id, 4.7 — not otherwise used here). Bins are
// written into outBins, which the caller sizes for the destination raster.
BinStats generateFragments(const Lattice& lattice, const SourceRaster& src,
                            double maxK, const SupersampleConfig& ss,
                            std::uint8_t tag, TileBins& outBins);

}  // namespace scatter
