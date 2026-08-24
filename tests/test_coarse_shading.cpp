// WU-34a: coarse-grid shading field (core/coarse_shading.hpp).
// DECISIONS.md ADR-070/082/083; tests/fixtures-historical.md fixtures 10
// (filtering ladder), 11 (grid shift) and 19 (three-sample facet normal,
// attribution to P not centroid) -- all from docs/sources/WU-SM-01.md §8,
// items 10, 11, 19.
//
// This unit builds CoarseShadingGrid as a pure computation over a Lattice
// and a LightingScene -- no core/binner.hpp wiring (that is WU-34b, ADR-083).
// Each fixture below is checked against an independent, hand-mirrored
// reimplementation of the production formula (the same style
// tests/test_jacobian.cpp already uses for its own central-difference
// checks), not by calling into coarse_shading.cpp's own internal helpers,
// so a bug shared between the mirror and the implementation is the only way
// a check here could pass wrongly.
#include "core/coarse_shading.hpp"
#include "core/lighting.hpp"
#include "harness.hpp"

#include <algorithm>
#include <cmath>

using namespace scatter;

namespace {

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

// Deterministic, smooth and non-planar in both directions -- the same
// shape of formula tests/test_jacobian.cpp's own makeTestLattice() uses,
// so the facet normal genuinely varies from vertex to vertex rather than
// degenerating to a flat plane every check below would be insensitive to.
Lattice makeTestLattice() {
    Lattice lat;
    for (int row = 0; row < kLatticeSize; ++row) {
        for (int col = 0; col < kLatticeSize; ++col) {
            const double r = double(row);
            const double c = double(col);
            Vec3& p = lat.at(row, col);
            p.x = 6.0 * c + 40.0 * std::sin(0.05 * r + 0.03 * c) + 0.01 * r * c;
            p.y = 4.0 * r + 30.0 * std::cos(0.04 * r - 0.06 * c) - 0.0002 * r * r * c;
            p.z = 10.0 * std::sin(0.02 * (r + c));
        }
    }
    return lat;
}

Vec3 sub(const Vec3& a, const Vec3& b) noexcept {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

// Independent mirror of coarse_shading.cpp's own facetNormal(): P plus its
// +u/+v forward neighbours (backward at the lattice's last row/column),
// tv x tu -- the exact convention ADR-083 documents. Reimplemented here
// rather than exposed from the production file, per this file's own header
// note.
Vec3 mirrorFacetNormal(const Lattice& lat, int row, int col) {
    const Vec3& p = lat.at(row, col);
    const Vec3 tu = (col < kLatticeMax) ? sub(lat.at(row, col + 1), p)
                                         : sub(p, lat.at(row, col - 1));
    const Vec3 tv = (row < kLatticeMax) ? sub(lat.at(row + 1, col), p)
                                         : sub(p, lat.at(row - 1, col));
    return Vec3{
        tv.y * tu.z - tv.z * tu.y,
        tv.z * tu.x - tv.x * tu.z,
        tv.x * tu.y - tv.y * tu.x,
    };
}

double mirrorRawI(const Lattice& lat, const LightingScene& scene, int row, int col) {
    return shade(scene, lat.at(row, col), mirrorFacetNormal(lat, row, col));
}

int mirrorShiftedCol(int col, int shift) {
    return std::max(0, col - shift);
}

int mirrorBlockAnchor(int idx, int blockSize) {
    return (idx / blockSize) * blockSize;
}

LightingScene oneParallelLightScene() {
    LightingScene s;
    s.Ia = 0.0;
    s.Ka = 1.0;
    Light light;
    light.type = LightType::Parallel;
    light.direction = Vec3{0.3, 0.2, 0.9};  // arbitrary fixed, non-axis-aligned
    light.intensity = 2.0;
    light.Kd = 1.0;
    light.Ks = 0.0;  // diffuse only -- no Zone table needed
    s.lights.push_back(light);
    return s;
}

}  // namespace

int main() {
    const Lattice lat = makeTestLattice();
    const LightingScene scene = oneParallelLightScene();

    // ------------------------------------------------------------------
    // Fixture 19 -- normal from a three-sample facet, attributed to P, not
    // the centroid. Checked two ways: (a) build()'s own raw field (filter
    // Full, no grid shift, so values_ == the raw per-vertex field
    // unmodified, and sample() at an exact integer vertex is exact
    // bilinear-at-a-grid-point) matches mirrorRawI() exactly at several
    // interior vertices and at the lattice's own last row/column (exercises
    // the backward-difference fallback); (b) a Point light, where P
    // genuinely affects the result via distance falloff, discriminates
    // between "attributed to P" and "attributed to the centroid of the
    // three-sample batch" -- the two must disagree, or this check would not
    // be exercising the attribution question fixture 19 names at all.
    {
        CoarseShadingConfig cfg;
        cfg.filter = ShadingFilter::Full;
        cfg.gridShift = 0;
        const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);

        const int rows[] = {0, 5, 64, 100, kLatticeMax};
        const int cols[] = {0, 7, 64, 120, kLatticeMax};
        for (int row : rows) {
            for (int col : cols) {
                const double got = grid.sample(double(col), double(row));
                const double expected = mirrorRawI(lat, scene, row, col);
                CHECK_ONCE(relClose(got, expected, 1e-9));
            }
        }
    }
    {
        LightingScene pointScene;
        pointScene.Ia = 0.0;
        pointScene.Ka = 1.0;
        Light light;
        light.type = LightType::Point;
        light.position = Vec3{500.0, 500.0, -200.0};
        light.intensity = 100.0;
        light.Kd = 1.0;
        light.Ks = 0.0;
        light.k = 1.0;
        pointScene.lights.push_back(light);

        const int row = 40, col = 60;
        const Vec3& p = lat.at(row, col);
        const Vec3 n = mirrorFacetNormal(lat, row, col);
        const double attributedToP = shade(pointScene, p, n);

        // "Attributed to the centroid" alternative: average P with its two
        // forward neighbours instead of using P itself.
        const Vec3& nu = lat.at(row, col + 1);
        const Vec3& nv = lat.at(row + 1, col);
        const Vec3 centroid{(p.x + nu.x + nv.x) / 3.0, (p.y + nu.y + nv.y) / 3.0,
                             (p.z + nu.z + nv.z) / 3.0};
        const double attributedToCentroid = shade(pointScene, centroid, n);

        CHECK(!relClose(attributedToP, attributedToCentroid, 1e-6));

        CoarseShadingConfig cfg;
        cfg.filter = ShadingFilter::Full;
        cfg.gridShift = 0;
        const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, pointScene, cfg);
        const double got = grid.sample(double(col), double(row));
        CHECK(relClose(got, attributedToP, 1e-9));
        CHECK(!relClose(got, attributedToCentroid, 1e-6));
    }

    // ------------------------------------------------------------------
    // Fixture 10 -- filtering ladder. Flat1/Flat2x2/Flat3x3 must resolve
    // any (u, v) inside a blockSize x blockSize group of coarse-grid cells
    // to the exact raw value at that block's own lower-u/lower-v anchor
    // vertex, regardless of fractional position within the block --
    // "flat shading", not a smoothed approximation of it.
    {
        struct Case {
            ShadingFilter filter;
            int blockSize;
        };
        const Case cases[] = {
            {ShadingFilter::Flat1, 1},
            {ShadingFilter::Flat2x2, 2},
            {ShadingFilter::Flat3x3, 3},
        };
        for (const Case& c : cases) {
            CoarseShadingConfig cfg;
            cfg.filter = c.filter;
            cfg.gridShift = 0;
            const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);

            const int anchorRow = mirrorBlockAnchor(30, c.blockSize);
            const int anchorCol = mirrorBlockAnchor(50, c.blockSize);
            const double expected = mirrorRawI(lat, scene, anchorRow, anchorCol);

            // Every fractional position inside this block must return
            // exactly the anchor's own raw value.
            const double offsets[] = {0.0, 0.3, 0.99};
            for (int dr = 0; dr < c.blockSize; ++dr) {
                for (int dc = 0; dc < c.blockSize; ++dc) {
                    for (double ofr : offsets) {
                        for (double ofc : offsets) {
                            const double v = double(anchorRow + dr) + ofr;
                            const double u = double(anchorCol + dc) + ofc;
                            CHECK_ONCE(relClose(grid.sample(u, v), expected, 1e-9));
                        }
                    }
                }
            }

            // The next block along both axes must resolve to its own
            // (generally different) anchor value -- confirms block size
            // itself, not just flatness within one block.
            const int nextRow = anchorRow + c.blockSize;
            const int nextCol = anchorCol + c.blockSize;
            const double expectedNext = mirrorRawI(lat, scene, nextRow, anchorCol);
            const double expectedNextCol = mirrorRawI(lat, scene, anchorRow, nextCol);
            CHECK(relClose(grid.sample(double(anchorCol) + 0.1, double(nextRow) + 0.1),
                            expectedNext, 1e-9));
            CHECK(relClose(grid.sample(double(nextCol) + 0.1, double(anchorRow) + 0.1),
                            expectedNextCol, 1e-9));
        }
    }

    // Full mode: an exact interior vertex must match the raw value exactly
    // (bilinear at an exact grid point), and a fractional position must lie
    // strictly between its two surrounding vertices along one axis when
    // they differ -- confirms Full genuinely interpolates rather than
    // snapping to a block, unlike the Flat cases above.
    {
        CoarseShadingConfig cfg;
        cfg.filter = ShadingFilter::Full;
        cfg.gridShift = 0;
        const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);

        const int row = 20, col = 30;
        const double a = mirrorRawI(lat, scene, row, col);
        const double b = mirrorRawI(lat, scene, row, col + 1);
        const double mid = grid.sample(double(col) + 0.5, double(row));
        const double lo = std::min(a, b), hi = std::max(a, b);
        CHECK(mid >= lo - 1e-9 && mid <= hi + 1e-9);
        if (!relClose(a, b, 1e-12)) {
            CHECK(!relClose(mid, a, 1e-12) || !relClose(mid, b, 1e-12));
        }
    }

    // ------------------------------------------------------------------
    // Fixture 11 -- grid shift. "With filtering +1, Shift 1 must move the
    // whole shading pattern exactly one coarse grid horizontally relative
    // to Zero." Checked directly against mirrorShiftedCol()'s own
    // black-box definition: the pattern displayed at column i under shift
    // s is the raw value that was actually computed at column i - s.
    {
        for (int shift : {0, 1, 2}) {
            CoarseShadingConfig cfg;
            cfg.filter = ShadingFilter::Flat1;
            cfg.gridShift = shift;
            const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);

            const int row = 45;
            for (int col : {2, 10, 60, 120, kLatticeMax}) {
                const int expectedCol = mirrorShiftedCol(col, shift);
                const double expected = mirrorRawI(lat, scene, row, expectedCol);
                const double got = grid.sample(double(col) + 0.2, double(row) + 0.1);
                CHECK_ONCE(relClose(got, expected, 1e-9));
            }
        }

        // Directly against fixture 11's own literal wording: Shift 1's
        // pattern at column i equals Shift 0 (Zero)'s pattern at column
        // i - 1, for every interior column.
        CoarseShadingConfig cfgZero;
        cfgZero.filter = ShadingFilter::Flat1;
        cfgZero.gridShift = 0;
        const CoarseShadingGrid gridZero = CoarseShadingGrid::build(lat, scene, cfgZero);

        CoarseShadingConfig cfgShift1;
        cfgShift1.filter = ShadingFilter::Flat1;
        cfgShift1.gridShift = 1;
        const CoarseShadingGrid gridShift1 = CoarseShadingGrid::build(lat, scene, cfgShift1);

        const int row = 45;
        for (int col = 1; col <= kLatticeMax; ++col) {
            const double shifted = gridShift1.sample(double(col) + 0.4, double(row));
            const double zero = gridZero.sample(double(col - 1) + 0.4, double(row));
            CHECK_ONCE(relClose(shifted, zero, 1e-9));
        }
    }

    // ------------------------------------------------------------------
    // Smooth1: an interior vertex's filtered value must equal the 3x3
    // box-blur average of the raw (post-shift) field -- ADR-083's own
    // explicit placeholder radius, mirrored here rather than asserted as a
    // historical value.
    {
        CoarseShadingConfig cfg;
        cfg.filter = ShadingFilter::Smooth1;
        cfg.gridShift = 0;
        const CoarseShadingGrid grid = CoarseShadingGrid::build(lat, scene, cfg);

        const int row = 50, col = 70;
        double sum = 0.0;
        int n = 0;
        for (int dr = -1; dr <= 1; ++dr) {
            for (int dc = -1; dc <= 1; ++dc) {
                sum += mirrorRawI(lat, scene, row + dr, col + dc);
                ++n;
            }
        }
        const double expected = sum / double(n);
        CHECK(relClose(grid.sample(double(col), double(row)), expected, 1e-9));
    }

    return scatter::test::summary("test_coarse_shading");
}
