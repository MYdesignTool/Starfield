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
    emitter_origin = 6,
    velocity_x = 7,
    velocity_y = 8,
    velocity_z = 9,
    size = 10,
    opacity = 11,
    emitter_size = 12,
    velocity_spread = 13,
};

enum class EmitterShape : std::uint8_t {
    point,
    box,
    sphere,
    disc,
};

// Bounds are owned by schema/parameters.json and enforced by validate_settings.
// Later stages (simulation, rasterizer) may rely on them without re-checking.
inline constexpr std::uint32_t kMaxParticleCount = 2'000'000;
inline constexpr double kMaxBirthRate = 1'000'000.0;
inline constexpr double kMaxLifetimeSeconds = 1'000'000.0;
inline constexpr double kMaxParticleSize = 100'000.0;
inline constexpr std::uint32_t kMaxSeed = 2'147'483'647;
inline constexpr std::uint32_t kEmitterShapeCount = 4;
// World-space movement bounds, in layer heights and layer heights per second.
inline constexpr double kMaxEmitterOffset = 100.0;
inline constexpr double kMaxVelocity = 1'000.0;
inline constexpr double kMaxEmitterSize = 10.0;
inline constexpr double kMaxVelocitySpread = 100.0;

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
    // World space for all three: origin at the layer centre, +X right, +Y up,
    // +Z toward the viewer, one unit = one layer height (ADR 0003). The host's
    // "% of layer" point control is converted by the adapter.
    Vec3 emitter_origin{};
    // Layer heights per second; the default keeps a fresh instance visibly alive.
    Vec3 velocity{0.0, 0.3, 0.0};
    double particle_size{8.0};
    double opacity{1.0};
    // Extent of the box/sphere/disc emitters: cube edge length resp. diameter, in
    // layer heights. Ignored by the point emitter.
    double emitter_size{0.05};
    // Per-axis uniform jitter applied to each particle's velocity, in layer heights
    // per second. This is what makes a steady emitter animate: identical particles
    // produce a stationary pattern, varied ones produce visible motion.
    double velocity_spread{0.15};
};

// Host popup controls are one-based while the core enum is zero-based. Out-of-range
// indices fall back to `point`; see docs/parameter-mapping.md for the mapping table.
[[nodiscard]] EmitterShape emitter_shape_from_index(std::uint32_t index) noexcept;

enum class ValidationCode : std::uint8_t {
    particle_count_clamped,
    birth_rate_clamped,
    seed_clamped,
    lifetime_clamped,
    size_clamped,
    opacity_clamped,
    emitter_shape_replaced,
    emitter_origin_clamped,
    velocity_clamped,
    emitter_size_clamped,
    velocity_spread_clamped,
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
