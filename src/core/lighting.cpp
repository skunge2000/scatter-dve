// scatter-dve — WU-27: see core/lighting.hpp for the design note and
// DECISIONS.md ADR-082 for the choices this file makes that the sources
// leave open.
#include "core/lighting.hpp"

#include <algorithm>
#include <cmath>

namespace scatter {

namespace {

double dot(const Vec3& a, const Vec3& b) noexcept {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

double length(const Vec3& v) noexcept {
    return std::sqrt(dot(v, v));
}

Vec3 sub(const Vec3& a, const Vec3& b) noexcept {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

Vec3 scaled(const Vec3& v, double s) noexcept {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

// Zero-length input (a degenerate normal, or a Point light exactly at the
// surface point) returns the zero vector rather than dividing by zero --
// the resulting cosA/cosB of 0 makes that light/facet contribute nothing,
// which is the only sane behaviour for an otherwise-undefined direction.
Vec3 normalized(const Vec3& v) noexcept {
    const double len = length(v);
    return (len > 0.0) ? scaled(v, 1.0 / len) : Vec3{0.0, 0.0, 0.0};
}

// R = 2*(N.L)*N - L: the standard Phong reflection of L about N, with L
// pointing away from the surface toward the light -- this file's own
// "toLight" convention throughout (core/lighting.hpp's Light::direction
// comment). Both N and L must already be unit length; every call site
// below passes unitN and a toLight already built from a normalized() or
// caller-normalised source.
Vec3 reflect(const Vec3& L, const Vec3& N) noexcept {
    return sub(scaled(N, 2.0 * dot(N, L)), L);
}

}  // namespace

double defaultSpecularCurve(SpecularModel model, double cosB) noexcept {
    // Every branch below is a provisional placeholder -- see
    // SpecularModel's own comment in core/lighting.hpp. Clamped to [0, 1]
    // here so every caller (including shade() below) can pass a raw,
    // possibly-negative cosine without pre-clamping.
    const double c = std::clamp(cosB, 0.0, 1.0);
    switch (model) {
        case SpecularModel::Model1:
            return std::pow(c, 2.0);
        case SpecularModel::Model2:
            return std::pow(c, 4.0);
        case SpecularModel::Model3:
            return std::pow(c, 8.0);
        case SpecularModel::Model4:
            return std::pow(c, 16.0);
        case SpecularModel::Ramp:
            // WU-SM-01 §4.6.3: "linear in cos B, i.e. n = 1 with no
            // power" -- the one named model this session is reasonably
            // confident is not just a placeholder guess.
            return c;
        case SpecularModel::Posterise: {
            // Stepped quantisation of Ramp -- kSteps is an arbitrary
            // placeholder, not a historical value.
            constexpr int kSteps = 4;
            const double step =
                std::min(std::floor(c * double(kSteps)), double(kSteps - 1));
            return step / double(kSteps - 1);
        }
        case SpecularModel::Ring2:
        case SpecularModel::Ring4: {
            // "Non-physical by construction... two lobes -> two
            // concentric highlight rings" (WU-SM-01 §4.6.3). Placeholder
            // shape only: a rectified sinusoid in the angle acos(cosB),
            // oscillating through `lobes` cycles from grazing to direct.
            const double lobes = (model == SpecularModel::Ring2) ? 2.0 : 4.0;
            return std::fabs(std::sin(lobes * std::acos(c)));
        }
    }
    return 0.0;  // unreachable for a valid enumerator
}

double shade(const LightingScene& scene, const Vec3& P, const Vec3& N) noexcept {
    const Vec3 unitN = normalized(N);
    const Vec3 view = normalized(scene.viewDirection);

    double total = scene.Ia * scene.Ka;

    for (const Light& light : scene.lights) {
        Vec3 toLight{0.0, 0.0, 0.0};
        double falloff = light.intensity;

        if (light.type == LightType::Point) {
            const Vec3 delta = sub(light.position, P);
            const double r = length(delta);
            toLight = normalized(delta);
            falloff = light.intensity / (r + light.k);
        } else {
            // Parallel and Beam: WU-SM-01 §4.6.3, "parallel makes SP
            // constant and drops the 1/(r+k) term" -- falloff stays
            // exactly light.intensity, no division. Beam shares this same
            // fixed-direction, no-falloff shape, differing only in the
            // facing clamp below.
            toLight = light.direction;
        }

        const double rawCosA = dot(unitN, toLight);
        const double cosA = (light.type == LightType::Beam)
                                 ? std::fabs(rawCosA)
                                 : std::max(0.0, rawCosA);
        if (cosA <= 0.0) {
            // Facing away from a Point/Parallel light: nothing to add.
            // Beam's own cosA is already |.|, so this only triggers at an
            // exact grazing angle for Beam.
            continue;
        }

        double specularFactor = 0.0;
        if (light.Ks != 0.0) {
            const Vec3 R = reflect(toLight, unitN);
            const double rawCosB = dot(R, view);
            const double cosB = (light.type == LightType::Beam)
                                     ? std::fabs(rawCosB)
                                     : std::max(0.0, rawCosB);
            specularFactor =
                defaultSpecularCurve(scene.zones[std::size_t(light.zoneIndex)].model, cosB);
        }

        total += falloff * (light.Kd * cosA + light.Ks * specularFactor);
    }

    return total;
}

}  // namespace scatter
