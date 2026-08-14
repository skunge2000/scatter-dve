// scatter-dve — WU-12a: see core/shapes/shapes.hpp for the design note and
// DECISIONS.md ADR-028 for the parametrisation this file implements,
// including the derivation of why the flat and curled pieces join with
// matching position and tangent at any turnProgress.
#include "core/shapes/shapes.hpp"

#include <cmath>

namespace scatter::shapes {

Lattice buildPageTurnLattice(const PageTurnParams& params) {
    Lattice lat;

    const double spineX = params.centerX - 0.5 * params.width;
    const double flatLen = (1.0 - params.turnProgress) * params.width;

    for (int row = 0; row < kLatticeSize; ++row) {
        // t: normalised fraction across the control-vertex grid's v axis,
        // independent of any source raster's actual resolution — same
        // convention ADR-024/ADR-027 already establish. The spine does not
        // curve — only the sheet's own turn direction does (ADR-028) — so
        // y is linear in t, exactly like the cylinder's own vertical
        // mapping.
        const double t = double(row) / double(kLatticeMax);
        const double y = params.centerY + (t - 0.5) * params.heightSpan;

        for (int col = 0; col < kLatticeSize; ++col) {
            const double s = double(col) / double(kLatticeMax);
            const double sx = s * params.width;  // distance from the spine, along the flat sheet

            Vec3& p = lat.at(row, col);
            p.y = y;

            if (sx <= flatLen) {
                // Still flat, on the table: no curl entered, radius never
                // used. sx == 0 (the spine, col == 0) always lands here,
                // since flatLen >= 0 for any turnProgress in [0, 1].
                p.x = spineX + sx;
                p.z = 0.0;
            } else {
                // Turned: a is the arc-length distance into the curl (the
                // sheet does not stretch as it feeds into the roll), theta
                // the angle that arc length sweeps around a cylinder of
                // the configured radius. 0 at theta == 0 (the flat/curl
                // boundary itself), always >= 0 elsewhere: 1 - cos(theta)
                // never goes negative — same "near == 0" depth convention
                // ADR-027 already uses for buildCylinderLattice/
                // buildSphereLattice.
                const double a = sx - flatLen;
                const double theta = a / params.radius;
                p.x = spineX + flatLen + params.radius * std::sin(theta);
                p.z = params.radius * (1.0 - std::cos(theta));
            }
        }
    }
    return lat;
}

}  // namespace scatter::shapes
