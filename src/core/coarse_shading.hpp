// scatter-dve — WU-34a: coarse-grid shading field (filtering ladder, grid
// shift) — DECISIONS.md ADR-070/082/083; docs/sources/WU-SM-01.md §3.9.1
// (coarse-grid corroboration), §4.6.3 (control mapping table).
//
// This file is WU-34's own "build the coarse-grid shading value computation
// module first" half, split from "wire it into core/binner.hpp's per-sample
// loop" (WU-34b, not yet built) the same way WU-27/WU-34 itself split the
// pure Phong evaluator from its own coarse-grid wiring (ADR-082) -- see
// ADR-083 for the full scoping account this session's own split rests on.
// CoarseShadingGrid below is a pure computation over a Lattice and a
// LightingScene, producing an interpolatable field of shade()'s own return
// value I; it knows nothing about fragments, tiles or Frag::Y/Cb/Cr --
// tests/test_coarse_shading.cpp is its only caller today, the same
// "evaluator with no wiring yet" shape core/lighting.hpp had at WU-27.
//
// ADR-083's own coarse-grid-cell-size decision, restated here because it is
// this file's central design fact: the coarse grid IS core/lattice.hpp's
// existing 129x129 geometry control lattice, not a second, separately-sized
// grid. docs/sources/WU-SM-01.md §3.9.1 ties Starlight's shading coarse
// grid to the same "coarse grid" S1's own shape diagnostics refer to: this
// project's own architecture.md 4.1 already identifies that concept with
// the 129x129 lattice ("the direct descendant of the Mirage address map...
// every 8th pixel"). No held source states a shading-specific cell size
// distinct from the geometry lattice's own resolution, and reusing it
// avoids inventing a second, unevidenced granularity -- see ADR-083 for the
// full reasoning and the (unescalated, per the continuation prompt's own
// tiering) engineering-judgement call this rests on.
#pragma once

#include "core/lattice.hpp"
#include "core/lighting.hpp"

#include <cstdint>
#include <vector>

namespace scatter {

// S3's own filtering control range (docs/sources/WU-SM-01.md §3.9.1/§4.6.3;
// ADR-070), used as the enum's own underlying values so a future
// control-surface layer can map an operator-facing integer directly, no
// translation table needed.
enum class ShadingFilter : std::int8_t {
    Smooth2 = -2,  // as Smooth1, more filtered
    Smooth1 = -1,  // DEFAULT: smoothing filter limiting the rate of change
                   // of light intensity across coarse-grid cells
    Full    =  0,  // full interpolation across coarse grids, no extra
                   // smoothing beyond ordinary bilinear
    Flat1   =  1,  // flat (posterised) shading, one value per coarse-grid
                   // cell
    Flat2x2 =  2,  // flat shading across 2x2 coarse-grid cells
    Flat3x3 =  3,  // flat shading across 3x3 coarse-grid cells
};

// Grid shift (ADR-070): 0 (Zero, default), 1 or 2 coarse-grid cells,
// horizontal only -- S3's own control has no vertical equivalent
// (docs/sources/WU-SM-01.md §3.9.1). Unchecked precondition on the value
// being 0/1/2, the same "caller's own bug, not guarded against here"
// convention this codebase already uses throughout (e.g. Lattice::at()'s
// row/col).
struct CoarseShadingConfig {
    ShadingFilter filter = ShadingFilter::Smooth1;
    int gridShift = 0;
};

// The coarse-grid shading field for one frame: shade()'s own return value
// I, evaluated once per lattice control vertex (ADR-083: the coarse grid
// IS the geometry lattice) using a finite-difference facet normal -- NOT
// core/jacobian.hpp's analytic surfaceNormal() -- per ADR-082's binding
// facet-normal decision -- then post-processed per config's filtering
// ladder and grid shift (ADR-070), and queryable at any continuous
// lattice-parameter (u, v) via sample() below.
//
// Same cost class and layering as core/lattice.hpp/core/jacobian.hpp:
// built once per frame on one thread (kLatticeSize x kLatticeSize shade()
// calls, architecture.md 4.1's own "16 641 vertices, negligible cost"),
// double precision throughout, no floating point anywhere near the I4/I6
// fixed-point accumulation path -- this field is consumed (WU-34b, not yet
// built) ahead of Frag construction, before any quantisation happens.
class CoarseShadingGrid {
public:
    // Builds the field: raw per-vertex shade() evaluation, then grid shift,
    // then the filtering ladder, in that order -- see coarse_shading.cpp's
    // own file comment for why grid shift is applied to the raw field
    // before filtering rather than after (ADR-083).
    //
    // Not noexcept: allocates a kLatticeSize*kLatticeSize std::vector<double>,
    // the same reason Lattice's own constructor is not noexcept.
    static CoarseShadingGrid build(const Lattice& lattice, const LightingScene& scene,
                                    const CoarseShadingConfig& config);

    // Interpolated shading intensity I at continuous lattice-parameter
    // (u, v) -- the same domain and edge-clamping convention as
    // Lattice::eval()/jacobian() (inputs clamped to [0, kLatticeMax]
    // first). Full/Smooth1/Smooth2 modes bilinearly interpolate an
    // already-filtered field; Flat1/Flat2x2/Flat3x3 modes look up one flat
    // value per coarse-grid-cell block instead (no interpolation across a
    // block boundary -- see coarse_shading.cpp for why Flat modes cannot
    // share the bilinear path and still look flat). config is baked in at
    // build() time; sample() never re-reads it beyond the filter-mode
    // branch above.
    double sample(double u, double v) const noexcept;

private:
    CoarseShadingGrid() = default;

    ShadingFilter filter_ = ShadingFilter::Smooth1;
    std::vector<double> values_;  // kLatticeSize * kLatticeSize, row-major,
                                   // same (row, col) indexing as Lattice::at()
                                   // -- grid shift and filtering already
                                   // applied; see build().
};

}  // namespace scatter
