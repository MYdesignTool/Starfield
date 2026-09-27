#include "starfield/core/ParticleSimulation.hpp"

#include "starfield/core/Random.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <new>

namespace starfield::core {
namespace {

constexpr double kPi = 3.14159265358979323846;

// Deterministic per-particle birth offset for the selected emitter shape. `extent`
// is the cube edge length for Box and the diameter for Sphere/Disc, in layer heights.
Vec3 birth_offset(EmitterShape shape, double extent, std::uint32_t seed, std::uint64_t id) noexcept {
    const double half = 0.5 * extent;
    if (!(half > 0.0)) {
        return Vec3{};
    }

    switch (shape) {
        case EmitterShape::box:
            return Vec3{symmetric_value(seed, id, RandomPurpose::position_x) * half,
                        symmetric_value(seed, id, RandomPurpose::position_y) * half,
                        symmetric_value(seed, id, RandomPurpose::position_z) * half};
        case EmitterShape::sphere: {
            // Uniform inside the volume: cube-root radius with an isotropic direction.
            const double radius = half * std::cbrt(unit_value(seed, id, RandomPurpose::position_x));
            const double cos_theta = symmetric_value(seed, id, RandomPurpose::position_y);
            const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
            const double phi = unit_value(seed, id, RandomPurpose::position_z) * 2.0 * kPi;
            return Vec3{radius * sin_theta * std::cos(phi), radius * sin_theta * std::sin(phi),
                        radius * cos_theta};
        }
        case EmitterShape::disc: {
            // Uniform over the area: square-root radius inside the XY plane.
            const double radius = half * std::sqrt(unit_value(seed, id, RandomPurpose::position_x));
            const double angle = unit_value(seed, id, RandomPurpose::position_y) * 2.0 * kPi;
            return Vec3{radius * std::cos(angle), radius * std::sin(angle), 0.0};
        }
        case EmitterShape::point:
            break;
    }
    return Vec3{};
}

// Integers above this magnitude are no longer exactly representable as doubles, so
// slot indices - and therefore particle identity - would stop being deterministic.
constexpr double kMaxExactSlot = 9007199254740992.0; // 2^53

constexpr std::uint64_t kCancellationCheckInterval = 4096;

struct SlotRange {
    std::uint64_t first{0};
    std::uint64_t last{0};
    bool valid{false}; // false means "no live slots"
};

// Slots alive at `time_seconds`: k <= floor(t * rate) because a particle is not
// born before its birth time, and k > floor((t - lifetime) * rate) because the
// lifetime interval is half-open. Negative comp time yields no slots, and the
// population cap keeps the newest slots (identical birth ordering every time).
SlotRange live_slot_range(double time_seconds, double birth_rate, double lifetime, std::uint32_t population_cap) {
    SlotRange range;
    if (!(birth_rate > 0.0) || population_cap == 0 || !(time_seconds >= 0.0)) {
        return range;
    }

    const double last_slot = std::floor(time_seconds * birth_rate);
    const double first_slot = std::floor((time_seconds - lifetime) * birth_rate) + 1.0;
    if (last_slot < 0.0 || last_slot > kMaxExactSlot || first_slot > kMaxExactSlot) {
        return range;
    }
    if (last_slot < first_slot) {
        return range;
    }

    auto first = static_cast<std::uint64_t>(first_slot > 0.0 ? first_slot : 0.0);
    const auto last = static_cast<std::uint64_t>(last_slot);

    const std::uint64_t alive = last - first + 1;
    if (alive > population_cap) {
        first = last - population_cap + 1;
    }

    range.first = first;
    range.last = last;
    range.valid = true;
    return range;
}

} // namespace

Result<std::vector<ParticleInstance>> simulate_particles(const ValidatedSettings& settings, double time_seconds,
                                                        const Cancellation& cancellation) {
    const Settings& values = settings.value;

    if (!std::isfinite(time_seconds)) {
        return Result<std::vector<ParticleInstance>>::failure(ErrorCode::invalid_time,
                                                             "non-finite frame time reached the simulation");
    }

    const SlotRange range = live_slot_range(time_seconds, values.birth_rate,
                                            values.particle_lifetime_seconds, values.particle_count);
    if (!range.valid) {
        if (values.birth_rate > 0.0 && values.particle_count > 0 &&
            std::floor(time_seconds * values.birth_rate) > kMaxExactSlot) {
            return Result<std::vector<ParticleInstance>>::failure(
                ErrorCode::invalid_time, "emission slot index exceeds the exactly representable range");
        }
        return Result<std::vector<ParticleInstance>>::success({});
    }

    const double birth_rate = values.birth_rate;
    const double slots_elapsed = time_seconds * birth_rate;
    const auto slot_count = static_cast<std::size_t>(range.last - range.first + 1);

    std::vector<ParticleInstance> particles;
    try {
        particles.resize(slot_count);
    } catch (const std::bad_alloc&) {
        return Result<std::vector<ParticleInstance>>::failure(ErrorCode::allocation_failed,
                                                             "particle list allocation failed");
    }

    const Vec3 origin = values.emitter_origin;
    for (std::size_t i = 0; i < slot_count; ++i) {
        if ((i % kCancellationCheckInterval) == 0 && cancellation.is_cancelled()) {
            return Result<std::vector<ParticleInstance>>::failure(ErrorCode::cancelled,
                                                                 "cancelled during particle simulation");
        }

        const auto slot = range.first + static_cast<std::uint64_t>(i);
        // Age is derived from the slot distance instead of `t - k / rate`, which
        // keeps full relative precision for long comps and high birth rates.
        double age = (slots_elapsed - static_cast<double>(slot)) / birth_rate;
        if (age < 0.0) {
            age = 0.0; // knife-edge rounding at the newest slot
        }

        ParticleInstance& particle = particles[i];
        particle.id = slot;
        particle.age_seconds = age;
        particle.lifetime_seconds = values.particle_lifetime_seconds;
        particle.size_pixels = values.particle_size;
        particle.opacity = values.opacity;
        // Per-particle birth offset and velocity come from deterministic streams keyed
        // by (seed, id, purpose). This variation is what makes a steady emitter move:
        // with identical particles, births continuously replace the particles that
        // leave, so a correctly computed sequence still looks frozen on playback.
        const Vec3 birth = birth_offset(values.emitter_shape, values.emitter_size, values.seed, slot);
        Vec3 particle_velocity = values.velocity;
        if (values.velocity_spread > 0.0) {
            particle_velocity.x +=
                symmetric_value(values.seed, slot, RandomPurpose::velocity_x) * values.velocity_spread;
            particle_velocity.y +=
                symmetric_value(values.seed, slot, RandomPurpose::velocity_y) * values.velocity_spread;
            particle_velocity.z +=
                symmetric_value(values.seed, slot, RandomPurpose::velocity_z) * values.velocity_spread;
        }

        // Straight-line integration: any absolute time can be evaluated without
        // stepping, so frame order cannot matter. Gravity, drag, and curves arrive as
        // graph nodes in M3-02.
        particle.position.x = origin.x + birth.x + particle_velocity.x * age;
        particle.position.y = origin.y + birth.y + particle_velocity.y * age;
        particle.position.z = origin.z + birth.z + particle_velocity.z * age;
    }

    return Result<std::vector<ParticleInstance>>::success(std::move(particles));
}

} // namespace starfield::core
