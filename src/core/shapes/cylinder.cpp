// scatter-dve — WU-11: see core/shapes/shapes.hpp for the design note and
// DECISIONS.md ADR-027 for the parametrisation this file implements.
#include "core/shapes/shapes.hpp"

#include <cmath>

namespace scatter::shapes {

Lattice buildCylinderLattice(const CylinderParams& params) {
    Lattice lat;
    for (int row = 0; row < kLatticeSize; ++row) {
        // t: normalised fraction across the control-vertex grid's v axis,
        // independent of any source raster's actual resolution (ADR-024
        // already normalises core/binner.cpp's pixelToLattice() this way).
        const double t = double(row) / double(kLatticeMax);
        // The cylinder's axis does not curve — only its cross-section
        // does (ADR-027) — so y is linear in t, exactly like the affine
        // (plane) case's own vertical mapping.
        const double y = params.centerY + (t - 0.5) * params.heightSpan;

        for (int col = 0; col < kLatticeSize; ++col) {
            const double s = double(col) / double(kLatticeMax);
            const double theta = (s - 0.5) * params.angleSpan;

            Vec3& p = lat.at(row, col);
            p.x = params.centerX + params.radius * std::sin(theta);
            p.y = y;
            // 0 at theta == 0 (the front-facing point, s == 0.5), always
            // >= 0 elsewhere: 1 - cos(theta) never goes negative. Matches
            // core/binner.cpp's toDepth() "near == 0" saturate-at-zero
            // assumption by construction, not by clamping here.
            p.z = params.radius * (1.0 - std::cos(theta));
        }
    }
    return lat;
}

}  // namespace scatter::shapes
