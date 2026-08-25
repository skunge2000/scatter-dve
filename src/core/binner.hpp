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

// Forward-declared, not #include "core/coarse_shading.hpp": every entry
// point below only ever holds a `const CoarseShadingGrid*` to call
// sample() on inside binner.cpp -- a pointer type needs no complete type
// here, the same minimal-footprint judgement core/resolve.hpp's own
// forward-declared ThreadPool already applies (ADR-021/026/030). A caller
// that actually builds a CoarseShadingGrid to pass in must already
// #include "core/coarse_shading.hpp" itself to do so.
class CoarseShadingGrid;

// WU-34b (DECISIONS.md ADR-084) introduced `ColourStandard`/`coeffsFor()`
// so applyShading() (binner.cpp) could pick which RGB<->YCbCr coefficient
// set to use for a real (double-precision, pre-quantisation) round trip
// when multiplying a coarse-grid shading intensity I into a source sample
// -- at the time, this file's own source samples were still YCbCr, so
// "multiply RGB by I" needed a conversion in and back out again.
//
// WU-41 (DECISIONS.md ADR-085, this session): `SourceRaster`/
// `sampleBilinear()` below are now RGB-native (fed directly from
// `video::RasterRGB`, WU-40), so applyShading()'s own colour argument is
// already RGB at the call site -- no conversion, therefore no coefficient
// choice, is needed there any more. Repository-wide grep before removing
// this enum (not assumed): its only real uses were `coeffsFor()` and
// `applyShading()`'s own now-deleted `standard` parameter, both in
// binner.cpp, plus two `tests/test_binner.cpp` call sites (updated this
// session to drop the now-nonexistent trailing argument) -- nothing else
// in the tree reads `core::ColourStandard`. `video/chroma.hpp`'s own RGB
// boundary conversion (WU-40) deliberately hardcodes its BT.601 literals
// rather than taking this enum (see that file's own comment on why); this
// unit does not change that. See WORK-UNITS.md's own WU-41 entry for the
// full "where should ColourStandard/coeffsFor live" reasoning ADR-085 §7
// left open: the answer this session reached is that neither shading nor
// the I/O boundary needs a *shared* one any more, so the enum is deleted
// rather than relocated. A future unit that genuinely needs a selectable
// coefficient set at the I/O boundary can reintroduce one at that point,
// against a real caller, rather than this unit keeping a now-unused type
// alive against a hypothetical one.

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
// RGB-converted 4:4:4-shaped planes (WU-41, DECISIONS.md ADR-085).
// architecture.md section 3's signal path runs "chroma upsample
// 4:2:2->4:4:4" then the RGB boundary conversion (WU-40) before "PASS 1:
// fragment generation", so binner.hpp never sees 4:2:2 data, and — since
// this unit — never sees YCbCr-labelled data either: both conversions
// (src/video/chroma.hpp) have already happened by the time this runs, and
// core/pipeline.cpp feeds this struct directly from a `video::RasterRGB`
// (WU-40) rather than round-tripping back to `video::Raster444`. Row-major,
// width*height samples per plane, no stride padding.
struct SourceRaster {
    int width = 0;
    int height = 0;
    const Sample* r = nullptr;
    const Sample* g = nullptr;
    const Sample* b = nullptr;
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

// Pass 1, row-range variant — WU-16b (DECISIONS.md ADR-041). Generates and
// bins fragments only for source rows in [rowStart, rowEnd) (0 <= rowStart
// <= rowEnd <= src.height, unchecked — same unchecked-precondition
// convention every other tile/row-range index in this codebase already
// uses, e.g. Lattice::at()), while every u/v lattice-parameter calculation
// stays keyed to src.width/src.height in full — never to rowEnd - rowStart
// or to a caller-shortened SourceRaster — so a caller partitioning
// [0, src.height) into disjoint row bands and calling this once per band
// produces exactly the fragments generateFragments() below would for each
// of those same rows in one whole-raster call: same (u, v) per source
// pixel, same supersampling decisions, same destination positions. See
// ADR-040 for why the naive alternative (calling generateFragments() once
// per band against a SourceRaster whose own height field was shortened to
// the band's extent) corrupts the v-parameter's denominator instead, and
// ADR-041 for this entry point's own design.
//
// Every other parameter has the same meaning as generateFragments()'s
// own. Bins are written into outBins, which the caller sizes for the
// destination raster — WU-16b's own per-worker "generation-time bin
// arena" is one whole-frame TileBins per row-band-generating worker, not a
// partial one, since a row band's own fragments can land in any tile
// depending on the warp; core/pipeline.cpp is where that arena actually
// lives.
//
// WU-34b (DECISIONS.md ADR-084): shadingGrid, when non-null, is sampled at
// each sub-sample's own (u, v) -- the same lattice-parameter coordinate
// this loop already computes for lattice.eval(), no extra evaluation --
// and multiplied into that sub-sample's colour, ahead of Frag construction
// (I10's own binding location). WU-41: that colour is now genuine RGB
// (src is RGB-native), so the multiply is a bare per-channel scale with no
// coefficient choice involved -- the `ColourStandard shadingStandard`
// parameter this comment used to describe is gone, see this file's own
// comment above `SourceRaster`'s definition removing `ColourStandard`.
// Default nullptr: every existing caller keeps compiling and behaving
// exactly as before, byte for byte -- the same "optional, caller-owned,
// default-off, zero-cost-when-absent" shape
// PipelineParams::pool/weightOut/kBufferMode (core/resolve.hpp) already
// established, applied here to individual function parameters instead of
// a struct field since this file has no per-call config struct of its
// own. Building and owning the CoarseShadingGrid (and the LightingScene it
// comes from) for a real frame is not this unit's job -- see
// WORK-UNITS.md's own WU-34b entry for what is deferred.
BinStats generateFragmentsRowRange(const Lattice& lattice, const SourceRaster& src,
                                    double maxK, const SupersampleConfig& ss,
                                    std::uint8_t tag, int rowStart, int rowEnd,
                                    TileBins& outBins,
                                    const CoarseShadingGrid* shadingGrid = nullptr);

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
//
// WU-16b: now a thin wrapper around generateFragmentsRowRange() above,
// covering the whole raster in one row range — this function's own
// signature and behaviour are exactly WU-08's frozen ones, unchanged (see
// ADR-041; SESSION-PROTOCOL.md rule 2, "never rename or refactor... names
// in headers are fixed" — read the same way ADR-026/030 already read it,
// as forbidding a change to an existing function's own contract, not as
// forbidding a new sibling entry point next to it).
BinStats generateFragments(const Lattice& lattice, const SourceRaster& src,
                            double maxK, const SupersampleConfig& ss,
                            std::uint8_t tag, TileBins& outBins,
                            const CoarseShadingGrid* shadingGrid = nullptr);

// WU-28c (DECISIONS.md ADR-065): per-fragment facing tag, row-range
// variant. New, additive tag mode alongside generateFragmentsRowRange()
// above — that function's own signature and behaviour are unchanged, the
// same "new sibling entry point, not a changed one" pattern WU-16b already
// established for generateFragments() itself (see its own comment). Every
// parameter has the same meaning as generateFragmentsRowRange()'s own,
// except tag is split into frontTag and backTag: each emitted Frag's own
// tag is frontTag if its source sample is front-facing (core/jacobian.hpp's
// surfaceNormal().z < 0, this project's own convention, ADR-027/ADR-063) or
// backTag otherwise. This is what closes the gap ADR-062/C-020 found —
// WU-28a's k-buffer keys its slots by Frag::tag, so a self-folding surface's
// front and back need different tag values to ever land in different slots
// at all; before this, every fragment from one generateFragments() call
// carried the identical scalar tag regardless of facing.
//
// Facing is computed once per source pixel, from the same lattice.jacobian()
// call the supersampling decision (chooseSupersample()) already makes at
// that pixel's own centre (u0, v0), and reused for every one of that
// pixel's sub-samples when supersampled (n > 1) — no extra lattice
// evaluation, the same reuse-not-duplicate reasoning ADR-062/ADR-063 already
// applied to dz/du, dz/dv. surfaceNormal() is called on lattice.jacobian()'s
// own direct output, before pixelJacobian()'s conversion strips dz/du,
// dz/dv — ADR-063's own explicit warning about the one trap a future WU-28c
// session could otherwise rediscover by a wrong result.
BinStats generateFragmentsRowRangeTagByFacing(const Lattice& lattice, const SourceRaster& src,
                                               double maxK, const SupersampleConfig& ss,
                                               std::uint8_t frontTag, std::uint8_t backTag,
                                               int rowStart, int rowEnd, TileBins& outBins,
                                               const CoarseShadingGrid* shadingGrid = nullptr);

// WU-28c (DECISIONS.md ADR-065): per-fragment facing tag, whole-raster
// variant. A thin wrapper around generateFragmentsRowRangeTagByFacing()
// above, exactly the relationship generateFragments() already has to
// generateFragmentsRowRange() (WU-16b, ADR-041).
BinStats generateFragmentsTagByFacing(const Lattice& lattice, const SourceRaster& src,
                                       double maxK, const SupersampleConfig& ss,
                                       std::uint8_t frontTag, std::uint8_t backTag,
                                       TileBins& outBins,
                                       const CoarseShadingGrid* shadingGrid = nullptr);

// WU-23a2a (DECISIONS.md ADR-076): field-parity row visitation, the
// lattice-aware half of field mode (video/interlace.hpp's own file comment;
// DECISIONS.md ADR-075's v-parameter finding). New, additive sibling entry
// point alongside generateFragmentsRowRange() above -- that function's own
// signature and behaviour are unchanged, the same "new sibling entry point,
// not a changed one" pattern WU-16b/WU-28c already established for this
// file.
//
// Generates and bins fragments only for src's own rows of one field's
// parity -- row rowOffset, rowOffset + 2, rowOffset + 4, ... < src.height
// (rowOffset 0 selects an interlaced frame's Top field rows, 1 selects
// Bottom -- video/interlace.hpp's own FieldParity row convention, taken
// here as a plain int rather than that enum: core/binner.hpp has never
// depended on video/, and this one caller-visible int keeps that true. A
// caller holding a FieldParity converts it at the call site --
// core/pipeline.cpp's field-mode driver, WU-23a2b, is the one place that
// needs both types in scope) -- while every u/v lattice-parameter
// calculation stays keyed to src.width/src.height in full, exactly
// generateFragmentsRowRange()'s own discipline (ADR-041), extended here
// from a contiguous row range to a strided one. This is the fix ADR-075
// named and left for this unit: a field-native SourceRaster half the
// frame's own height would renormalise the v-parameter across only that
// field's own extent, erasing the half-line vertical phase between the two
// fields that makes interlace look like interlace; visiting the *full*
// frame's own rows at stride 2 instead keeps src.height (and therefore the
// v-parameter's denominator) exactly what generateFragments() would use for
// a whole-frame call.
//
// rowOffset must be 0 or 1 (unchecked -- the same convention every other
// row/tile index in this codebase already uses, e.g. Lattice::at()); every
// other parameter has the same meaning as generateFragmentsRowRange()'s
// own.
BinStats generateFragmentsFieldRows(const Lattice& lattice, const SourceRaster& src,
                                     double maxK, const SupersampleConfig& ss,
                                     std::uint8_t tag, int rowOffset,
                                     TileBins& outBins,
                                     const CoarseShadingGrid* shadingGrid = nullptr);

}  // namespace scatter
