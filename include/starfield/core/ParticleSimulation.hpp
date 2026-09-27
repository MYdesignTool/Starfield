#pragma once

#include "starfield/core/Render.hpp"

#include <cstdint>
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

} // namespace starfield::core
