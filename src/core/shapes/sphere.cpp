// scatter-dve — WU-11: see core/shapes/shapes.hpp for the design note and
// DECISIONS.md ADR-027 for the parametrisation this file implements,
// including the algebraic proof that this (non-textbook) gimbal-angle
// formula still lands exactly on a sphere of the configured radius.
#include "core/shapes/shapes.hpp"

#include <cmath>

namespace scatter::shapes {

Lattice buildSphereLattice(const SphereParams& params) {
    Lattice lat;
    for (int row = 0; row < kLatticeSize; ++row) {
        const double t = double(row) / double(kLatticeMax);
        const double psi = (t - 0.5) * params.angleSpanV;
        const double sinPsi = std::sin(psi);
        const double cosPsi = std::cos(psi);

        for (int col = 0; col < kLatticeSize; ++col) {
            const double s = double(col) / double(kLatticeMax);
            const double phi = (s - 0.5) * params.angleSpanH;

            Vec3& p = lat.at(row, col);
            p.x = params.centerX + params.radius * std::sin(phi) * cosPsi;
            p.y = params.centerY + params.radius * sinPsi;
            // 0 at phi == psi == 0 (the front-facing point), always >= 0
            // elsewhere: 1 - cos(phi)*cos(psi) never goes negative, since
            // cos(phi)*cos(psi) <= 1 always. Same "near == 0" convention
            // buildCylinderLattice() uses, by construction, not clamping.
            p.z = params.radius * (1.0 - std::cos(phi) * cosPsi);
        }
    }
    return lat;
}

}  // namespace scatter::shapes
