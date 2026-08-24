// WU-27: closed-form Phong lighting evaluator (core/lighting.hpp).
// DECISIONS.md ADR-069/082; tests/fixtures-historical.md fixtures 9, 12,
// 13, 15, 16, 17, 18, 26 (all from docs/sources/WU-SM-01.md §8, items 9,
// 12-18, 26).
//
// This unit builds shade() as a pure function -- no coarse grid, no
// core/binner.hpp wiring (that is WU-34's own job, ADR-082). Every fixture
// below is therefore adapted to what a pure (P, N, LightingScene) ->
// double function can actually exercise:
//
// - Fixture 8's default six-light control-tree topology (which lights are
//   under which gantry, the "lights 3&5 = zone 1, 4&6 = zone 2" default
//   wiring fixture 16 names) needs an axis tree ADR-067 already records as
//   unscoped, no implementation anywhere in this project yet. Fixtures 9,
//   14 and 16 below therefore test the *structural* property each fixture
//   actually depends on (summation reduces correctly when only one light
//   is non-zero; a light's own zoneIndex genuinely selects its own zone's
//   curve, independent of any other light) rather than the literal
//   six-named-lights-under-two-gantries default state, which is future
//   work's own scope, not this one's.
// - Fixture 14's own "moving the object" half (an axis-tree-derived light
//   direction that tracks light-origin -> object-origin as the object's
//   own axis moves) is the same unscoped dependency; the part actually
//   inside WU-27's scope -- a Parallel light's direction is a fixed,
//   caller-supplied axis, independent of P -- is checked directly instead,
//   and is also exactly what fixture 26 (orthographic specular) needs.
#include "core/lighting.hpp"
#include "harness.hpp"

#include <cmath>

using namespace scatter;

namespace {

bool relClose(double a, double b, double tol) noexcept {
    const double scale = std::max(std::max(std::fabs(a), std::fabs(b)), 1.0);
    return std::fabs(a - b) <= tol * scale;
}

LightingScene sceneWithOneZone(SpecularModel model) {
    LightingScene s;
    s.Ia = 0.0;
    s.Ka = 1.0;
    s.zones.push_back(Zone{model});
    s.viewDirection = Vec3{0.0, 0.0, 1.0};
    return s;
}

}  // namespace

int main() {
    // ------------------------------------------------------------------
    // Fixture 17 -- the illumination formula, hand-worked case: unit
    // normal along +z, a Parallel light at 45 deg from the normal
    // (toLight = (sin45, 0, cos45)), fixed view vector == the normal
    // itself, n = 2 (SpecularModel::Model1, cos^2 B). Kd = Ks = 1, Ip = 1,
    // no ambient, so I reduces to exactly (Kd*cosA + Ks*cos^2 B).
    //
    // Hand computation: cosA = cos45 = sqrt(2)/2.
    // R = reflect(toLight, N) = 2*(N.toLight)*N - toLight
    //   = 2*(sqrt(2)/2)*(0,0,1) - (sqrt(2)/2, 0, sqrt(2)/2)
    //   = (-sqrt(2)/2, 0, sqrt(2)/2)
    // cosB = R . view = R . (0,0,1) = sqrt(2)/2  =>  cos^2 B = 0.5
    // I = 1*(sqrt(2)/2) + 1*0.5 = sqrt(2)/2 + 0.5
    //
    // A half-vector (Blinn) substitution, an inverse-square falloff, or a
    // per-pixel view vector each change this number -- see the fixture's
    // own header note in docs/sources/WU-SM-01.md §8 item 17.
    {
        LightingScene s = sceneWithOneZone(SpecularModel::Model1);
        s.viewDirection = Vec3{0.0, 0.0, 1.0};

        Light light;
        light.type = LightType::Parallel;
        const double s45 = std::sqrt(2.0) / 2.0;
        light.direction = Vec3{s45, 0.0, s45};
        light.intensity = 1.0;
        light.Kd = 1.0;
        light.Ks = 1.0;
        light.zoneIndex = 0;
        s.lights.push_back(light);

        const Vec3 N{0.0, 0.0, 1.0};
        const Vec3 P{0.0, 0.0, 0.0};
        const double expected = s45 + 0.5;
        const double got = shade(s, P, N);
        CHECK(relClose(got, expected, 1e-9));

        // A half-vector substitution would give a materially different
        // number at this same configuration -- confirm the two are not
        // accidentally close, so this check is actually discriminating.
        // H = normalize(toLight + view) = normalize((s45, 0, s45+1))
        const Vec3 toLight = light.direction;
        const Vec3 view = s.viewDirection;
        Vec3 h{toLight.x + view.x, toLight.y + view.y, toLight.z + view.z};
        const double hlen = std::sqrt(h.x * h.x + h.y * h.y + h.z * h.z);
        h = Vec3{h.x / hlen, h.y / hlen, h.z / hlen};
        const double cosNH = h.z;  // N = (0,0,1)
        const double blinnI = s45 + cosNH * cosNH;
        CHECK(!relClose(got, blinnI, 1e-3));
    }

    // ------------------------------------------------------------------
    // Fixture 18 -- distance falloff is linear, not inverse-square.
    // Doubling r must multiply the direct (diffuse-only, Ks = 0) term by
    // (r+k)/(2r+k), not by 1/4. Light placed directly along +N from P so
    // cosA == 1 regardless of r, isolating the falloff term exactly.
    {
        LightingScene s = sceneWithOneZone(SpecularModel::Model1);
        s.Ia = 0.0;

        const Vec3 N{0.0, 0.0, 1.0};
        const Vec3 P{0.0, 0.0, 0.0};
        const double k = 1.0;
        const double intensity = 6.0;

        auto diffuseAt = [&](double r) {
            LightingScene local = s;
            Light light;
            light.type = LightType::Point;
            light.position = Vec3{0.0, 0.0, r};
            light.intensity = intensity;
            light.Kd = 1.0;
            light.Ks = 0.0;
            light.k = k;
            local.lights.push_back(light);
            return shade(local, P, N);
        };

        const double r1 = 3.0;
        const double i1 = diffuseAt(r1);
        const double i2 = diffuseAt(2.0 * r1);

        const double expectedRatio = (r1 + k) / (2.0 * r1 + k);
        const double gotRatio = i2 / i1;
        CHECK(relClose(gotRatio, expectedRatio, 1e-9));
        // Not the inverse-square ratio (1/4) -- confirms this isn't
        // accidentally satisfying both.
        CHECK(!relClose(gotRatio, 0.25, 1e-6));

        // Stays finite as r -> 0 (k > 0 keeps the denominator off zero).
        const double atZero = diffuseAt(0.0);
        CHECK(std::isfinite(atZero));
    }

    // ------------------------------------------------------------------
    // Fixture 9 -- default lighting state: with several lights present
    // but all but one at zero intensity, the scene's own shade() must
    // equal what a single-light scene containing just the non-zero light
    // gives. (The literal "six named lights, two under gantry 8, four
    // under gantry 9" default topology is axis-tree scoped work -- see
    // this file's own header comment.)
    {
        LightingScene six = sceneWithOneZone(SpecularModel::Model1);
        six.Ia = 0.3;

        Light active;
        active.type = LightType::Point;
        active.position = Vec3{5.0, 2.0, -3.0};
        active.intensity = 1.0;
        active.Kd = 0.8;
        active.Ks = 0.4;
        active.zoneIndex = 0;

        six.lights.push_back(active);
        for (int i = 0; i < 5; ++i) {
            Light dark = active;
            dark.position = Vec3{double(i), -double(i) * 2.0, 7.0};
            dark.intensity = 0.0;  // lights 2..6 at 0.0
            six.lights.push_back(dark);
        }

        LightingScene one = sceneWithOneZone(SpecularModel::Model1);
        one.Ia = six.Ia;
        one.lights.push_back(active);

        const Vec3 N{0.1, 0.2, 1.0};
        const Vec3 P{1.0, 1.0, 1.0};
        CHECK(relClose(shade(six, P, N), shade(one, P, N), 1e-12));
    }

    // ------------------------------------------------------------------
    // Fixture 12 -- negative-intensity cancellation. Two lights identical
    // in every parameter except intensity sign must produce a net-zero
    // contribution, leaving exactly the ambient floor.
    {
        LightingScene s = sceneWithOneZone(SpecularModel::Model2);
        s.Ia = 0.25;
        s.Ka = 0.6;

        Light plus;
        plus.type = LightType::Point;
        plus.position = Vec3{4.0, -1.0, 2.0};
        plus.intensity = 3.5;
        plus.Kd = 0.7;
        plus.Ks = 0.5;
        plus.zoneIndex = 0;

        Light minus = plus;
        minus.intensity = -3.5;

        s.lights.push_back(plus);
        s.lights.push_back(minus);

        const Vec3 N{0.0, 0.3, 1.0};
        const Vec3 P{0.0, 0.0, 0.0};
        CHECK(relClose(shade(s, P, N), s.Ia * s.Ka, 1e-12));
    }

    // ------------------------------------------------------------------
    // Fixture 13 -- beam bidirectionality. A Beam light must illuminate a
    // surface and its exact opposite-facing counterpart symmetrically;
    // any implementation that clamps to the forward hemisphere fails
    // this. Contrasted directly against a Parallel light sharing the same
    // direction, which *should* clamp (and does, by design -- see
    // core/lighting.hpp's own shade() comment).
    {
        LightingScene s = sceneWithOneZone(SpecularModel::Model1);
        s.Ia = 0.0;

        Light beam;
        beam.type = LightType::Beam;
        beam.direction = Vec3{0.0, 0.0, 1.0};
        beam.intensity = 2.0;
        beam.Kd = 1.0;
        beam.Ks = 0.0;

        const Vec3 P{0.0, 0.0, 0.0};
        const Vec3 Nplus{0.0, 0.0, 1.0};
        const Vec3 Nminus{0.0, 0.0, -1.0};

        LightingScene beamScene = s;
        beamScene.lights.push_back(beam);
        const double beamPlus = shade(beamScene, P, Nplus);
        const double beamMinus = shade(beamScene, P, Nminus);
        CHECK(relClose(beamPlus, beamMinus, 1e-12));
        CHECK(beamPlus > 0.0);

        Light parallel = beam;
        parallel.type = LightType::Parallel;
        LightingScene parallelScene = s;
        parallelScene.lights.push_back(parallel);
        const double parPlus = shade(parallelScene, P, Nplus);
        const double parMinus = shade(parallelScene, P, Nminus);
        CHECK(parPlus > 0.0);
        CHECK(relClose(parMinus, 0.0, 1e-12));  // clamped away: no contribution
        CHECK(!relClose(parPlus, parMinus, 1e-9));  // NOT symmetric, unlike Beam
    }

    // ------------------------------------------------------------------
    // Fixture 15 -- point-source limit: as a point light's distance grows
    // without bound (intensity scaled to hold falloff constant), its
    // result converges on a Parallel light of the same direction and
    // equivalent intensity.
    {
        const Vec3 dir{0.0, 0.0, 1.0};
        const double k = 1.0;
        const double target = 4.0;  // desired falloff == intensity/(r+k)
        const double r = 1.0e8;

        LightingScene pointScene = sceneWithOneZone(SpecularModel::Model1);
        Light pointLight;
        pointLight.type = LightType::Point;
        pointLight.position = Vec3{dir.x * r, dir.y * r, dir.z * r};
        pointLight.intensity = target * (r + k);
        pointLight.Kd = 1.0;
        pointLight.Ks = 0.3;
        pointLight.k = k;
        pointScene.lights.push_back(pointLight);

        LightingScene parallelScene = sceneWithOneZone(SpecularModel::Model1);
        Light parallelLight;
        parallelLight.type = LightType::Parallel;
        parallelLight.direction = dir;
        parallelLight.intensity = target;
        parallelLight.Kd = 1.0;
        parallelLight.Ks = 0.3;
        parallelScene.lights.push_back(parallelLight);

        const Vec3 N{0.2, -0.1, 1.0};
        for (double px : {0.0, 3.0, -5.0}) {
            const Vec3 P{px, 1.0, 0.0};
            const double got = shade(pointScene, P, N);
            const double want = shade(parallelScene, P, N);
            CHECK(relClose(got, want, 1e-6));
        }
    }

    // ------------------------------------------------------------------
    // Fixture 16 -- zone locking (adapted, see this file's own header
    // comment): a light's zoneIndex must select its own zone's specular
    // curve, independent of any other light in the scene.
    {
        LightingScene s;
        s.Ia = 0.0;
        s.Ka = 1.0;
        // Deliberately NOT aligned with N/light direction: at cosB == 1
        // exactly, Model1's cos^2 B and Ramp's cos B coincide (both 1.0),
        // which would make this fixture unable to tell the two zones
        // apart. Tilting the view 30 degrees off the normal gives a cosB
        // (== view.z, since light is directly along N here, making
        // R == N) where the two curves genuinely differ: 0.75 vs 0.866.
        const double c30 = std::sqrt(3.0) / 2.0;
        s.viewDirection = Vec3{0.5, 0.0, c30};
        s.zones.push_back(Zone{SpecularModel::Model1});  // zone 0
        s.zones.push_back(Zone{SpecularModel::Ramp});    // zone 1

        Light zone0;
        zone0.type = LightType::Parallel;
        zone0.direction = Vec3{0.0, 0.0, 1.0};
        zone0.intensity = 1.0;
        zone0.Kd = 0.0;
        zone0.Ks = 1.0;
        zone0.zoneIndex = 0;

        Light zone1 = zone0;
        zone1.zoneIndex = 1;

        const Vec3 N{0.0, 0.0, 1.0};
        const Vec3 P{0.0, 0.0, 0.0};

        LightingScene onlyZone0 = s;
        onlyZone0.lights.push_back(zone0);
        LightingScene onlyZone1 = s;
        onlyZone1.lights.push_back(zone1);

        // Light directly along N => R == N, so cosB == view.z == c30.
        const double expectZone0 = defaultSpecularCurve(SpecularModel::Model1, c30);
        const double expectZone1 = defaultSpecularCurve(SpecularModel::Ramp, c30);
        CHECK(relClose(shade(onlyZone0, P, N), expectZone0, 1e-12));
        CHECK(relClose(shade(onlyZone1, P, N), expectZone1, 1e-12));
        CHECK(!relClose(expectZone0, expectZone1, 1e-9));

        // Both lights together: straight summation (this session's own
        // [C] reading, ADR-082) -- must equal the sum of each alone.
        LightingScene both = s;
        both.lights.push_back(zone0);
        both.lights.push_back(zone1);
        CHECK(relClose(shade(both, P, N),
                        shade(onlyZone0, P, N) + shade(onlyZone1, P, N), 1e-12));
    }

    // ------------------------------------------------------------------
    // Fixture 26 -- orthographic specular: moving the object laterally
    // (P changes) with a fixed Parallel light and fixed view vector must
    // not move the highlight -- a per-pixel view vector would change cosB
    // as P changes even with light/normal held fixed; the fixed view
    // vector this project uses (ADR-069) must not.
    {
        LightingScene s = sceneWithOneZone(SpecularModel::Model2);
        s.viewDirection = Vec3{0.1, 0.0, 1.0};  // deliberately not aligned with N

        Light light;
        light.type = LightType::Parallel;
        light.direction = Vec3{0.3, 0.0, 1.0};
        light.intensity = 1.0;
        light.Kd = 0.5;
        light.Ks = 1.0;
        light.zoneIndex = 0;
        s.lights.push_back(light);

        const Vec3 N{0.0, 0.0, 1.0};
        const double atOrigin = shade(s, Vec3{0.0, 0.0, 0.0}, N);
        const double atFarLeft = shade(s, Vec3{-50.0, 0.0, 0.0}, N);
        const double atFarRight = shade(s, Vec3{50.0, 12.0, 0.0}, N);
        CHECK(relClose(atOrigin, atFarLeft, 1e-15));
        CHECK(relClose(atOrigin, atFarRight, 1e-15));
    }

    // ------------------------------------------------------------------
    // defaultSpecularCurve() sanity: every model stays within [0, 1] for
    // the full cosB domain, and Model1..4/Ramp are monotonically
    // non-decreasing (a power law and a linear ramp both are; Posterise,
    // Ring2 and Ring4 are deliberately not checked for monotonicity here,
    // consistent with their own non-power-law placeholder shapes).
    {
        const SpecularModel monotoneModels[] = {
            SpecularModel::Model1, SpecularModel::Model2,
            SpecularModel::Model3, SpecularModel::Model4,
            SpecularModel::Ramp,
        };
        for (SpecularModel m : monotoneModels) {
            double prev = defaultSpecularCurve(m, -1.0);
            CHECK(prev >= 0.0 && prev <= 1.0);
            for (int i = 1; i <= 20; ++i) {
                const double c = -1.0 + 2.0 * double(i) / 20.0;
                const double v = defaultSpecularCurve(m, c);
                CHECK_ONCE(v >= 0.0 && v <= 1.0);
                CHECK_ONCE(v >= prev - 1e-12);
                prev = v;
            }
            CHECK(relClose(defaultSpecularCurve(m, 1.0), 1.0, 1e-12));
        }

        // Ring/Posterise: bounds only, plus the negative-cosB == 0-cosB
        // clamp every model shares.
        const SpecularModel allModels[] = {
            SpecularModel::Model1,   SpecularModel::Model2,  SpecularModel::Model3,
            SpecularModel::Model4,   SpecularModel::Ramp,    SpecularModel::Posterise,
            SpecularModel::Ring2,    SpecularModel::Ring4,
        };
        for (SpecularModel m : allModels) {
            CHECK_ONCE(relClose(defaultSpecularCurve(m, -0.7),
                                 defaultSpecularCurve(m, 0.0), 1e-12));
        }
    }

    return scatter::test::summary("test_lighting");
}
