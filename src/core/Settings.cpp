#include "starfield/core/Settings.hpp"

#include <algorithm>
#include <cmath>

namespace starfield::core {
namespace {

double finite_or(double value, double fallback, const char* field,
                 std::vector<ValidationNotice>& notices) {
    if (std::isfinite(value)) {
        return value;
    }
    notices.push_back({ValidationCode::non_finite_replaced, field});
    return fallback;
}

double clamp(double value, double minimum, double maximum, ValidationCode code,
             const char* field, std::vector<ValidationNotice>& notices) {
    const double bounded = std::clamp(value, minimum, maximum);
    if (bounded != value) {
        notices.push_back({code, field});
    }
    return bounded;
}

} // namespace

EmitterShape emitter_shape_from_index(std::uint32_t index) noexcept {
    switch (index) {
        case 0:
            return EmitterShape::point;
        case 1:
            return EmitterShape::box;
        case 2:
            return EmitterShape::sphere;
        case 3:
            return EmitterShape::disc;
        default:
            break;
    }
    return EmitterShape::point;
}

ValidatedSettings validate_settings(Settings settings) {
    ValidatedSettings result{settings, {}};
    auto& value = result.value;
    auto& notices = result.notices;

    if (value.particle_count > kMaxParticleCount) {
        value.particle_count = kMaxParticleCount;
        notices.push_back({ValidationCode::particle_count_clamped, "particle_count"});
    }

    value.birth_rate = finite_or(value.birth_rate, 30.0, "birth_rate", notices);
    value.birth_rate = clamp(value.birth_rate, 0.0, kMaxBirthRate,
                             ValidationCode::birth_rate_clamped, "birth_rate", notices);

    if (value.seed > kMaxSeed) {
        value.seed = kMaxSeed;
        notices.push_back({ValidationCode::seed_clamped, "seed"});
    }

    value.particle_lifetime_seconds = finite_or(value.particle_lifetime_seconds, 2.0,
                                                "particle_lifetime_seconds", notices);
    value.particle_lifetime_seconds = clamp(value.particle_lifetime_seconds, 0.0, kMaxLifetimeSeconds,
                                            ValidationCode::lifetime_clamped,
                                            "particle_lifetime_seconds", notices);

    value.emitter_origin.x = finite_or(value.emitter_origin.x, 0.0, "emitter_origin.x", notices);
    value.emitter_origin.y = finite_or(value.emitter_origin.y, 0.0, "emitter_origin.y", notices);
    value.emitter_origin.z = finite_or(value.emitter_origin.z, 0.0, "emitter_origin.z", notices);
    value.emitter_origin.x = clamp(value.emitter_origin.x, -kMaxEmitterOffset, kMaxEmitterOffset,
                                   ValidationCode::emitter_origin_clamped, "emitter_origin.x", notices);
    value.emitter_origin.y = clamp(value.emitter_origin.y, -kMaxEmitterOffset, kMaxEmitterOffset,
                                   ValidationCode::emitter_origin_clamped, "emitter_origin.y", notices);
    value.emitter_origin.z = clamp(value.emitter_origin.z, -kMaxEmitterOffset, kMaxEmitterOffset,
                                   ValidationCode::emitter_origin_clamped, "emitter_origin.z", notices);

    value.velocity.x = finite_or(value.velocity.x, 0.0, "velocity.x", notices);
    value.velocity.y = finite_or(value.velocity.y, 0.0, "velocity.y", notices);
    value.velocity.z = finite_or(value.velocity.z, 0.0, "velocity.z", notices);
    value.velocity.x = clamp(value.velocity.x, -kMaxVelocity, kMaxVelocity, ValidationCode::velocity_clamped,
                             "velocity.x", notices);
    value.velocity.y = clamp(value.velocity.y, -kMaxVelocity, kMaxVelocity, ValidationCode::velocity_clamped,
                             "velocity.y", notices);
    value.velocity.z = clamp(value.velocity.z, -kMaxVelocity, kMaxVelocity, ValidationCode::velocity_clamped,
                             "velocity.z", notices);

    value.particle_size = finite_or(value.particle_size, 8.0, "particle_size", notices);
    value.particle_size = clamp(value.particle_size, 0.0, kMaxParticleSize,
                                ValidationCode::size_clamped, "particle_size", notices);

    value.opacity = finite_or(value.opacity, 1.0, "opacity", notices);
    value.opacity = clamp(value.opacity, 0.0, 1.0,
                          ValidationCode::opacity_clamped, "opacity", notices);

    value.emitter_size = finite_or(value.emitter_size, 0.05, "emitter_size", notices);
    value.emitter_size = clamp(value.emitter_size, 0.0, kMaxEmitterSize,
                               ValidationCode::emitter_size_clamped, "emitter_size", notices);

    value.velocity_spread = finite_or(value.velocity_spread, 0.0, "velocity_spread", notices);
    value.velocity_spread = clamp(value.velocity_spread, 0.0, kMaxVelocitySpread,
                                  ValidationCode::velocity_spread_clamped, "velocity_spread", notices);

    if (static_cast<unsigned>(value.emitter_shape) > static_cast<unsigned>(EmitterShape::disc)) {
        value.emitter_shape = EmitterShape::point;
        notices.push_back({ValidationCode::emitter_shape_replaced, "emitter_shape"});
    }

    return result;
}

} // namespace starfield::core
