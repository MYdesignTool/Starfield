#pragma once

#include <cstdint>

// Deterministic particle randomness (ADR 0002). Every value is a pure function of
// the effect seed, the persistent particle id, and a stream-purpose id, so a frame
// can be evaluated in any order, repeatedly, and on any thread without shared
// state. Per-particle variation is what turns a steady emitter into motion: with
// identical particles a continuously emitting trail looks frozen because births
// keep replacing the particles that leave.

namespace starfield::core {

// Stream purposes stay numbered forever once released; add new values, never reuse.
enum class RandomPurpose : std::uint64_t {
    position_x = 1,
    position_y = 2,
    position_z = 3,
    velocity_x = 4,
    velocity_y = 5,
    velocity_z = 6,
    size = 7,
    opacity = 8,
    // Emission direction sampling: two stream values place a direction inside a cone
    // (or on the sphere), one jitters the emitted speed.
    direction_u1 = 9,
    direction_u2 = 10,
    emission_speed = 11,
    auxiliary_chance = 12,
    force_gravity = 13,
    preview_chance = 14,
    particle_color = 15,
    particle_life = 16, particle_angle = 17, particle_spin = 18,
    particle_texture = 19, particle_cloud = 20,
};

// splitmix64 finalizer: cheap, well distributed, and identical on every platform.
[[nodiscard]] constexpr std::uint64_t mix64(std::uint64_t value) noexcept {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}

// One independent bit stream per (seed, particle, purpose).
[[nodiscard]] std::uint64_t stream_bits(std::uint32_t seed, std::uint64_t particle_id,
                                        RandomPurpose purpose) noexcept;

// [0, 1)
[[nodiscard]] double unit_from_bits(std::uint64_t bits) noexcept;

// [-1, 1)
[[nodiscard]] double symmetric_from_bits(std::uint64_t bits) noexcept;

// Convenience wrappers for one stream.
[[nodiscard]] double unit_value(std::uint32_t seed, std::uint64_t particle_id, RandomPurpose purpose) noexcept;
[[nodiscard]] double symmetric_value(std::uint32_t seed, std::uint64_t particle_id,
                                     RandomPurpose purpose) noexcept;

} // namespace starfield::core
