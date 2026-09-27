#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace starfield::core {

// IDs are serialized into the AE parameter layout. Once exposed to users, never
// renumber or reuse an ID; add new IDs and migrate old sequence data instead.
enum class ParameterId : std::uint32_t {
    particle_count = 1,
    birth_rate = 2,
    seed = 3,
    particle_lifetime = 4,
    emitter_shape = 5,
    velocity = 6,
    size = 7,
    opacity = 8,
};

enum class EmitterShape : std::uint8_t {
    point,
    box,
    sphere,
    disc,
};

struct Vec3 {
    double x{};
    double y{};
    double z{};
};

struct Settings {
    std::uint32_t particle_count{1000};
    double birth_rate{30.0};
    std::uint32_t seed{1};
    double particle_lifetime_seconds{2.0};
    EmitterShape emitter_shape{EmitterShape::point};
    Vec3 velocity{};
    double particle_size{8.0};
    double opacity{1.0};
};

enum class ValidationCode : std::uint8_t {
    particle_count_clamped,
    birth_rate_clamped,
    lifetime_clamped,
    size_clamped,
    opacity_clamped,
    non_finite_replaced,
};

struct ValidationNotice {
    ValidationCode code{};
    std::string field;
};

struct ValidatedSettings {
    Settings value;
    std::vector<ValidationNotice> notices;
};

// Converts untrusted host/UI values into bounded, finite render input.
// This function is pure and safe to call independently for every frame.
[[nodiscard]] ValidatedSettings validate_settings(Settings settings);

} // namespace starfield::core
