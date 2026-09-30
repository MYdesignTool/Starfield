#include "starfield/core/Settings.hpp"
#include "starfield/core/AgeCurve.hpp"

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

    value.particle_size_random_percent = finite_or(value.particle_size_random_percent, 0.0,
                                                    "particle_size_random_percent", notices);
    value.particle_size_random_percent = clamp(value.particle_size_random_percent, 0.0,
        kMaxParticleRandomPercent, ValidationCode::particle_size_random_clamped,
        "particle_size_random_percent", notices);
    value.opacity_random_percent = finite_or(value.opacity_random_percent, 0.0,
                                              "opacity_random_percent", notices);
    value.opacity_random_percent = clamp(value.opacity_random_percent, 0.0,
        kMaxParticleRandomPercent, ValidationCode::opacity_random_clamped,
        "opacity_random_percent", notices);

    value.emitter_size = finite_or(value.emitter_size, 0.05, "emitter_size", notices);
    value.emitter_size = clamp(value.emitter_size, 0.0, kMaxEmitterSize,
                               ValidationCode::emitter_size_clamped, "emitter_size", notices);

    value.emitter_size_pixels.x = finite_or(value.emitter_size_pixels.x, 100.0,
                                             "emitter_size_pixels.x", notices);
    value.emitter_size_pixels.y = finite_or(value.emitter_size_pixels.y, 100.0,
                                             "emitter_size_pixels.y", notices);
    value.emitter_size_pixels.z = finite_or(value.emitter_size_pixels.z, 100.0,
                                             "emitter_size_pixels.z", notices);
    value.emitter_size_pixels.x = clamp(value.emitter_size_pixels.x, 0.0, kMaxEmitterSizePixels,
                                        ValidationCode::emitter_size_pixels_clamped,
                                        "emitter_size_pixels.x", notices);
    value.emitter_size_pixels.y = clamp(value.emitter_size_pixels.y, 0.0, kMaxEmitterSizePixels,
                                        ValidationCode::emitter_size_pixels_clamped,
                                        "emitter_size_pixels.y", notices);
    value.emitter_size_pixels.z = clamp(value.emitter_size_pixels.z, 0.0, kMaxEmitterSizePixels,
                                        ValidationCode::emitter_size_pixels_clamped,
                                        "emitter_size_pixels.z", notices);

    value.velocity_spread = finite_or(value.velocity_spread, 0.0, "velocity_spread", notices);
    value.velocity_spread = clamp(value.velocity_spread, 0.0, kMaxVelocitySpread,
                                  ValidationCode::velocity_spread_clamped, "velocity_spread", notices);

    value.gravity.x = finite_or(value.gravity.x, 0.0, "gravity.x", notices);
    value.gravity.y = finite_or(value.gravity.y, 0.0, "gravity.y", notices);
    value.gravity.z = finite_or(value.gravity.z, 0.0, "gravity.z", notices);
    value.gravity.x = clamp(value.gravity.x, -kMaxGravityMagnitude, kMaxGravityMagnitude,
                            ValidationCode::gravity_clamped, "gravity.x", notices);
    value.gravity.y = clamp(value.gravity.y, -kMaxGravityMagnitude, kMaxGravityMagnitude,
                            ValidationCode::gravity_clamped, "gravity.y", notices);
    value.gravity.z = clamp(value.gravity.z, -kMaxGravityMagnitude, kMaxGravityMagnitude,
                            ValidationCode::gravity_clamped, "gravity.z", notices);

    value.linear_drag = finite_or(value.linear_drag, 0.0, "linear_drag", notices);
    value.linear_drag = clamp(value.linear_drag, 0.0, kMaxLinearDrag,
                              ValidationCode::linear_drag_clamped, "linear_drag", notices);

    const auto validate_color = [&notices](Vec3& color, const char* field) {
        const auto validate_channel = [&notices, field](double& channel, const char* suffix) {
            std::string name(field);
            name += suffix;
            channel = finite_or(channel, 1.0, name.c_str(), notices);
            channel = clamp(channel, 0.0, kMaxParticleColor,
                            ValidationCode::color_clamped, name.c_str(), notices);
        };
        validate_channel(color.x, ".r");
        validate_channel(color.y, ".g");
        validate_channel(color.z, ".b");
    };
    validate_color(value.color_start, "color_start");
    validate_color(value.color_end, "color_end");

    value.particle_size_end = finite_or(value.particle_size_end, value.particle_size,
                                        "particle_size_end", notices);
    value.particle_size_end = clamp(value.particle_size_end, 0.0, kMaxParticleSize,
                                    ValidationCode::end_size_clamped, "particle_size_end", notices);
    value.opacity_end = finite_or(value.opacity_end, value.opacity,
                                  "opacity_end", notices);
    value.opacity_end = clamp(value.opacity_end, 0.0, 1.0,
                              ValidationCode::end_opacity_clamped, "opacity_end", notices);

    if (value.size_over_life.count != 0 && !valid_age_curve(value.size_over_life, 0.0, kMaxParticleSize)) {
        notices.push_back({ValidationCode::age_curve_invalid, "size_over_life"});
    }
    if (value.opacity_over_life.count != 0 && !valid_age_curve(value.opacity_over_life, 0.0, 1.0)) {
        notices.push_back({ValidationCode::age_curve_invalid, "opacity_over_life"});
    }

    // Emission direction model (M3-04). Speed is a magnitude, so it clamps to [0, max];
    // angles wrap into a normal range instead of being clamped away, and the span is a
    // half-cone that saturates at 180 degrees (a full sphere).
    value.emission_speed = finite_or(value.emission_speed, 0.0, "emission_speed", notices);
    value.emission_speed = clamp(value.emission_speed, 0.0, kMaxEmissionSpeed,
                                 ValidationCode::emission_speed_clamped, "emission_speed", notices);
    value.emission_speed_random = finite_or(value.emission_speed_random, 0.0,
                                            "emission_speed_random", notices);
    value.emission_speed_random = clamp(value.emission_speed_random, 0.0, kMaxEmissionSpeed,
                                        ValidationCode::emission_speed_random_clamped,
                                        "emission_speed_random", notices);
    value.emission_angles_degrees.x = finite_or(value.emission_angles_degrees.x, 0.0,
                                                "emission_angle_x", notices);
    value.emission_angles_degrees.y = finite_or(value.emission_angles_degrees.y, 0.0,
                                                "emission_angle_y", notices);
    value.emission_angles_degrees.z = finite_or(value.emission_angles_degrees.z, 0.0,
                                                "emission_angle_z", notices);
    value.emission_angles_degrees.x = clamp(value.emission_angles_degrees.x,
                                            -kMaxEmissionAngleDegrees, kMaxEmissionAngleDegrees,
                                            ValidationCode::emission_angle_clamped, "emission_angle_x", notices);
    value.emission_angles_degrees.y = clamp(value.emission_angles_degrees.y,
                                            -kMaxEmissionAngleDegrees, kMaxEmissionAngleDegrees,
                                            ValidationCode::emission_angle_clamped, "emission_angle_y", notices);
    value.emission_angles_degrees.z = clamp(value.emission_angles_degrees.z,
                                            -kMaxEmissionAngleDegrees, kMaxEmissionAngleDegrees,
                                            ValidationCode::emission_angle_clamped, "emission_angle_z", notices);
    value.direction_span_degrees = finite_or(value.direction_span_degrees, 60.0,
                                             "direction_span", notices);
    value.direction_span_degrees = clamp(value.direction_span_degrees, 0.0, kMaxDirectionSpanDegrees,
                                         ValidationCode::direction_span_clamped, "direction_span", notices);
    if (static_cast<unsigned>(value.direction_mode) > static_cast<unsigned>(DirectionMode::uniform)) {
        value.direction_mode = DirectionMode::directional;
        notices.push_back({ValidationCode::direction_mode_replaced, "direction_mode"});
    }

    if (static_cast<unsigned>(value.emitter_shape) > static_cast<unsigned>(EmitterShape::disc)) {
        value.emitter_shape = EmitterShape::point;
        notices.push_back({ValidationCode::emitter_shape_replaced, "emitter_shape"});
    }

    return result;
}

} // namespace starfield::core
