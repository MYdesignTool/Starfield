#include "starfield/core/Settings.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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

ValidatedSettings validate_settings(Settings settings) {
    ValidatedSettings result{settings, {}};
    auto& value = result.value;

    constexpr std::uint32_t max_particles = 2'000'000;
    if (value.particle_count > max_particles) {
        value.particle_count = max_particles;
        result.notices.push_back({ValidationCode::particle_count_clamped, "particle_count"});
    }

    value.birth_rate = finite_or(value.birth_rate, 30.0, "birth_rate", result.notices);
    value.birth_rate = clamp(value.birth_rate, 0.0, 1'000'000.0,
                             ValidationCode::birth_rate_clamped, "birth_rate", result.notices);

    value.particle_lifetime_seconds = finite_or(value.particle_lifetime_seconds, 2.0,
                                                "particle_lifetime_seconds", result.notices);
    value.particle_lifetime_seconds = clamp(value.particle_lifetime_seconds, 0.0, 1'000'000.0,
                                            ValidationCode::lifetime_clamped,
                                            "particle_lifetime_seconds", result.notices);

    value.velocity.x = finite_or(value.velocity.x, 0.0, "velocity.x", result.notices);
    value.velocity.y = finite_or(value.velocity.y, 0.0, "velocity.y", result.notices);
    value.velocity.z = finite_or(value.velocity.z, 0.0, "velocity.z", result.notices);

    value.particle_size = finite_or(value.particle_size, 8.0, "particle_size", result.notices);
    value.particle_size = clamp(value.particle_size, 0.0, 100'000.0,
                                ValidationCode::size_clamped, "particle_size", result.notices);

    value.opacity = finite_or(value.opacity, 1.0, "opacity", result.notices);
    value.opacity = clamp(value.opacity, 0.0, 1.0,
                          ValidationCode::opacity_clamped, "opacity", result.notices);

    if (static_cast<unsigned>(value.emitter_shape) > static_cast<unsigned>(EmitterShape::disc)) {
        value.emitter_shape = EmitterShape::point;
    }

    return result;
}

} // namespace starfield::core
