// scatter-dve — WU-34a: see core/coarse_shading.hpp for the design note
// and DECISIONS.md ADR-083 for the choices this file makes that the
// sources leave open.
#include "core/coarse_shading.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace scatter {

namespace {

Vec3 sub(const Vec3& a, const Vec3& b) noexcept {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

// tv x tu, not tu x tv -- matching core/jacobian.hpp's surfaceNormal() own
// cross-product order exactly, so a finite-difference facet normal and
// WU-26's analytic one agree on front-facing sign (normal.z < 0,
// ADR-027/ADR-063) for the same underlying surface. tu approximates the
// +u (column) tangent, tv the +v (row) tangent -- see surfaceNormal()'s
// own comment for the full sign derivation; this is the identical
// convention, applied to finite-difference edge vectors instead of
// analytic tangents.
Vec3 crossTvTu(const Vec3& tu, const Vec3& tv) noexcept {
    return Vec3{
        tv.y * tu.z - tv.z * tu.y,
        tv.z * tu.x - tv.x * tu.z,
        tv.x * tu.y - tv.y * tu.x,
    };
}

// docs/sources/WU-SM-01.md §3.9.1 (S5 FIG. 3): "three image elements
// adjacent to the image point P... define a small facet... including the
// point P" -- attributed to P, not the batch's centroid (fixture 19). This
// project's own reconstruction (ADR-083, not itself a literal historical
// value -- no source names which two of a vertex's neighbours the real
// retiming buffer held): P plus its immediate +u and +v neighbours,
// forward differences, falling back to a backward difference at the
// lattice's own last row/column so every vertex gets a well-defined facet
// without a special "no neighbour" case. This is what grid shift's own
// "horizontal, toward the previous grid" correction (ADR-070) is shaped to
// correct: the facet is attributed to its own lower-u/lower-v corner (P),
// one full cell short of the discontinuity it is meant to describe, at
// exactly the coordinate grid shift moves the pattern away from.
Vec3 facetNormal(const Lattice& lattice, int row, int col) noexcept {
    const Vec3& p = lattice.at(row, col);

    const Vec3 tu = (col < kLatticeMax)
                        ? sub(lattice.at(row, col + 1), p)
                        : sub(p, lattice.at(row, col - 1));
    const Vec3 tv = (row < kLatticeMax)
                        ? sub(lattice.at(row + 1, col), p)
                        : sub(p, lattice.at(row - 1, col));

    return crossTvTu(tu, tv);
}

std::size_t index(int row, int col) noexcept {
    return std::size_t(row) * std::size_t(kLatticeSize) + std::size_t(col);
}

// ADR-070: grid shift applies a coarse grid's own computed pattern to the
// previous grid (or the one before that), horizontally. Read as: the value
// displayed at column `col` is the *raw* facet value actually computed at
// column `col - shift` -- shifting the whole pattern `shift` cells toward
// +u, correcting the lower-u attribution bias facetNormal() above
// deliberately reproduces. Clamped at the low edge (col - shift < 0
// saturates to column 0) rather than wrapping -- the same edge convention
// this codebase's other boundary lookups use (e.g.
// core/binner.cpp's sampleBilinear()).
int shiftedColumn(int col, int shift) noexcept {
    return std::max(0, col - shift);
}

// Filtering ladder, applied after grid shift (ADR-083's own ordering
// choice: grid shift corrects *which* raw facet value belongs at a given
// cell before the ladder groups or smooths those corrected values --
// applying the ladder first would flatten/blur across the very
// attribution error grid shift exists to fix, defeating its own documented
// purpose of following it at filtering +1..+3).
//
// Flat1/Flat2x2/Flat3x3 ("posterisation/mosaic", WU-SM-01 §3.9.1): every
// vertex within one blockSize x blockSize group of coarse-grid cells reads
// the single raw (post-shift) value at that block's own lower-u/lower-v
// anchor vertex -- an arbitrary but consistent anchor choice (matching
// facetNormal()'s own P-not-centroid attribution above), not itself a
// literal historical value.
//
// Smooth1/Smooth2 ("smoothing filter limiting the rate of change... more
// filtered" [A] for existence and relative strength, [C] for exact shape):
// a box blur over a (2*radius+1) x (2*radius+1) neighbourhood of raw
// (post-shift) values, radius 1 for Smooth1 and 2 for Smooth2 -- an
// explicit placeholder radius, the same "provisional, not a best-effort
// reconstruction" tier as core/lighting.hpp's defaultSpecularCurve(). Full
// applies no filtering at all: sample() bilinearly interpolates the raw
// (post-shift) field directly.
std::vector<double> applyFilter(const std::vector<double>& shifted, ShadingFilter filter) {
    if (filter == ShadingFilter::Full) {
        return shifted;
    }

    std::vector<double> out(shifted.size());

    if (filter == ShadingFilter::Smooth1 || filter == ShadingFilter::Smooth2) {
        const int radius = (filter == ShadingFilter::Smooth1) ? 1 : 2;
        for (int row = 0; row < kLatticeSize; ++row) {
            for (int col = 0; col < kLatticeSize; ++col) {
                double sum = 0.0;
                int n = 0;
                for (int dr = -radius; dr <= radius; ++dr) {
                    const int r = std::clamp(row + dr, 0, kLatticeMax);
                    for (int dc = -radius; dc <= radius; ++dc) {
                        const int c = std::clamp(col + dc, 0, kLatticeMax);
                        sum += shifted[index(r, c)];
                        ++n;
                    }
                }
                out[index(row, col)] = sum / double(n);
            }
        }
        return out;
    }

    // Flat1/Flat2x2/Flat3x3.
    const int blockSize = (filter == ShadingFilter::Flat1)   ? 1
                         : (filter == ShadingFilter::Flat2x2) ? 2
                                                               : 3;
    for (int row = 0; row < kLatticeSize; ++row) {
        const int anchorRow = (row / blockSize) * blockSize;
        for (int col = 0; col < kLatticeSize; ++col) {
            const int anchorCol = (col / blockSize) * blockSize;
            out[index(row, col)] = shifted[index(anchorRow, anchorCol)];
        }
    }
    return out;
}

}  // namespace

CoarseShadingGrid CoarseShadingGrid::build(const Lattice& lattice, const LightingScene& scene,
                                            const CoarseShadingConfig& config) {
    std::vector<double> raw(std::size_t(kLatticeSize) * std::size_t(kLatticeSize));
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const Vec3 n = facetNormal(lattice, row, col);
            raw[index(row, col)] = shade(scene, lattice.at(row, col), n);
        }
    }

    std::vector<double> shifted(raw.size());
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            shifted[index(row, col)] = raw[index(row, shiftedColumn(col, config.gridShift))];
        }
    }

    CoarseShadingGrid grid;
    grid.filter_ = config.filter;
    grid.values_ = applyFilter(shifted, config.filter);
    return grid;
}

double CoarseShadingGrid::sample(double u, double v) const noexcept {
    const double cu = std::clamp(u, 0.0, double(kLatticeMax));
    const double cv = std::clamp(v, 0.0, double(kLatticeMax));

    if (filter_ == ShadingFilter::Flat1 || filter_ == ShadingFilter::Flat2x2 ||
        filter_ == ShadingFilter::Flat3x3) {
        // Nearest-block lookup, not bilinear: a block boundary must read as
        // a hard step (WU-SM-01 §3.9.1's own "posterisation/mosaic
        // effect"), and bilinearly blending two already-flattened blocks'
        // shared vertex values would soften exactly that step back into a
        // one-cell ramp -- values_ is already block-flattened (applyFilter()
        // above), but sample() still needs to avoid interpolating across a
        // block seam, so it rounds to the block anchor directly rather than
        // blending its four surrounding corners.
        const int blockSize = (filter_ == ShadingFilter::Flat1)   ? 1
                             : (filter_ == ShadingFilter::Flat2x2) ? 2
                                                                    : 3;
        const int col = std::min((int(cu) / blockSize) * blockSize, kLatticeMax);
        const int row = std::min((int(cv) / blockSize) * blockSize, kLatticeMax);
        return values_[index(row, col)];
    }

    const int col0 = int(std::floor(cu));
    const int row0 = int(std::floor(cv));
    const int col1 = std::min(col0 + 1, kLatticeMax);
    const int row1 = std::min(row0 + 1, kLatticeMax);
    const double fc = cu - double(col0);
    const double fr = cv - double(row0);

    const double top = values_[index(row0, col0)] * (1.0 - fc) + values_[index(row0, col1)] * fc;
    const double bot = values_[index(row1, col0)] * (1.0 - fc) + values_[index(row1, col1)] * fc;
    return top * (1.0 - fr) + bot * fr;
}

}  // namespace scatter
