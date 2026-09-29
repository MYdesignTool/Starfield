#pragma once

#include "starfield/core/Render.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace starfield::core {

// One evaluated particle for the requested absolute time. Positions are world
// space (ADR 0003): origin at the layer center, unit = one layer height.
struct ParticleInstance {
    std::uint64_t id{0};
    double age_seconds{0.0};
    double lifetime_seconds{0.0};
    double size_pixels{0.0};
    double opacity{0.0};
    Vec3 color{1.0, 1.0, 1.0};
    Vec3 position{};
};

// The contiguous global emission-slot interval alive at one absolute time.
// A zero count means no particles are alive.
struct ParticleSlotRange {
    std::uint64_t first_slot{0};
    std::uint64_t count{0};
};

// Deterministic particle evaluation for one absolute time.
//
// Emission slot `k` is born at `k / birth_rate` seconds on the effect clock, which
// is anchored at host time zero, and lives for `particle_lifetime_seconds`. A slot
// is visible while `0 <= age < lifetime`; the population is capped at
// `particle_count` by keeping the newest slots. Every call evaluates the requested
// absolute time from scratch, so frame order, repeats, and negative comp time
// cannot leak state between renders (ADR 0002).
//
// Per-particle birth offsets follow `emitter_shape` within `emitter_size`, and each
// particle gets an independent velocity jitter of +/- `velocity_spread` per axis.
// Gravity and linear drag use closed-form integration, so no frame stepping or
// render history is required. Size, opacity, and RGB color interpolate linearly
// over normalized particle age. All results are pure functions of settings, seed,
// particle slot, and absolute time.
//
// `time_seconds` is the only lossy conversion allowed across this boundary.
[[nodiscard]] Result<std::vector<ParticleInstance>> simulate_particles(const ValidatedSettings& settings,
                                                                     double time_seconds,
                                                                     const Cancellation& cancellation);

// Evaluate only global emission slots assigned to one deterministic branch.
// Slot identity, birth time, random streams, and the emitter-wide live cap stay
// unchanged: slot k is included when k % partition_count == partition_index.
// This lets graph Particle nodes split one emitter without simulating the full
// population once per branch.
[[nodiscard]] Result<std::vector<ParticleInstance>> simulate_particles_partition(
    const ValidatedSettings& settings, double time_seconds, std::uint32_t partition_count,
    std::uint32_t partition_index, const Cancellation& cancellation);

// Return the bounded global slot interval used by every branch of an emitter.
[[nodiscard]] Result<ParticleSlotRange> live_particle_slot_range(
    const ValidatedSettings& settings, double time_seconds);

// Write one deterministic branch directly into a pre-sized, global-slot-ordered
// buffer. Call once for every partition index. The destination size must equal
// live_particle_slot_range(settings, time_seconds).count.
[[nodiscard]] Result<std::size_t> simulate_particles_partition_into(
    const ValidatedSettings& settings, double time_seconds, std::uint32_t partition_count,
    std::uint32_t partition_index, std::span<ParticleInstance> destination,
    const Cancellation& cancellation);

} // namespace starfield::core
