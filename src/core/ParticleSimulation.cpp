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

struct DragIntegrals {
    double velocity_displacement{0.0};
    double acceleration_displacement{0.0};
};

// For dv/dt = gravity - drag * velocity, return the exact displacement factors
// multiplying initial velocity and gravity over `age`. The series avoids
// cancellation in (t - (1-exp(-kt))/k) when k*t is close to zero.
DragIntegrals drag_integrals(double drag, double age) noexcept {
    if (!(drag > 0.0)) return DragIntegrals{age, 0.5 * age * age};

    const double z = drag * age;
    if (z < 1.0e-4) {
        const double z2 = z * z;
        const double z3 = z2 * z;
        const double z4 = z3 * z;
        const double velocity_factor = age * (1.0 - z / 2.0 + z2 / 6.0 - z3 / 24.0 + z4 / 120.0);
        const double acceleration_factor = age * age *
            (0.5 - z / 6.0 + z2 / 24.0 - z3 / 120.0 + z4 / 720.0);
        return DragIntegrals{velocity_factor, acceleration_factor};
    }

    const double velocity_factor = -std::expm1(-z) / drag;
    const double acceleration_factor = (age - velocity_factor) / drag;
    return DragIntegrals{velocity_factor, acceleration_factor};
}

// --- Emission direction model (M3-04) ---------------------------------------
constexpr double kEmissionPi = 3.14159265358979323846;

Vec3 normalize_direction(Vec3 value) noexcept {
    const double length = std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
    if (!(length > 0.0)) return Vec3{0.0, 1.0, 0.0};
    return Vec3{value.x / length, value.y / length, value.z / length};
}

Vec3 cross_direction(const Vec3& a, const Vec3& b) noexcept {
    return Vec3{a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

// Rotate +Y (straight up) about X, then Y, then Z, so 0/0/0 points up, which is the
// reference emitter's default look. The rotation order is part of our documented
// convention (docs/reference-parameter-map.md), not an observation of the reference.
Vec3 rotate_emission_axis(const Vec3& degrees) noexcept {
    const double to_radians = kEmissionPi / 180.0;
    const double cx = std::cos(degrees.x * to_radians), sx = std::sin(degrees.x * to_radians);
    const double cy = std::cos(degrees.y * to_radians), sy = std::sin(degrees.y * to_radians);
    const double cz = std::cos(degrees.z * to_radians), sz = std::sin(degrees.z * to_radians);
    Vec3 axis{0.0, 1.0, 0.0};
    axis = Vec3{axis.x, axis.y * cx - axis.z * sx, axis.y * sx + axis.z * cx};
    axis = Vec3{axis.x * cy + axis.z * sy, axis.y, -axis.x * sy + axis.z * cy};
    axis = Vec3{axis.x * cz - axis.y * sz, axis.x * sz + axis.y * cz, axis.z};
    return axis;
}

// Uniform over the cone's solid angle: cos(theta) is uniform between cos(half) and 1, so
// the density is even across the cap instead of clustering at the axis. A span of 180
// degenerates to a whole-sphere sample, which is what `uniform` mode uses, and a span of
// 0 collapses to the axis itself.
Vec3 direction_in_cone(const Vec3& axis, double span_degrees, double u1, double u2) noexcept {
    const double half = std::clamp(span_degrees, 0.0, 180.0) * 0.5;
    // A half-angle of 90 degrees is only a hemisphere, so a full sphere is its own case:
    // cos(theta) then spans [-1, 1] instead of [0, 1].
    const double cos_min = span_degrees >= 180.0 ? -1.0 : std::cos(half * kEmissionPi / 180.0);
    const double cos_theta = 1.0 - u1 * (1.0 - cos_min);
    const double sin_theta = std::sqrt(std::max(0.0, 1.0 - cos_theta * cos_theta));
    const double phi = u2 * 2.0 * kEmissionPi;
    const Vec3 reference = std::abs(axis.y) < 0.9 ? Vec3{0.0, 1.0, 0.0} : Vec3{1.0, 0.0, 0.0};
    const Vec3 right = normalize_direction(cross_direction(reference, axis));
    const Vec3 up = cross_direction(axis, right);
    const double ring_x = right.x * std::cos(phi) + up.x * std::sin(phi);
    const double ring_y = right.y * std::cos(phi) + up.y * std::sin(phi);
    const double ring_z = right.z * std::cos(phi) + up.z * std::sin(phi);
    return Vec3{axis.x * cos_theta + ring_x * sin_theta,
                axis.y * cos_theta + ring_y * sin_theta,
                axis.z * cos_theta + ring_z * sin_theta};
}

Vec3 emission_direction(const Settings& values, std::uint64_t slot) noexcept {
    const double span =
        values.direction_mode == DirectionMode::uniform ? 180.0 : values.direction_span_degrees;
    const double u1 = unit_value(values.seed, slot, RandomPurpose::direction_u1);
    const double u2 = unit_value(values.seed, slot, RandomPurpose::direction_u2);
    return direction_in_cone(rotate_emission_axis(values.emission_angles_degrees), span, u1, u2);
}

// Slots alive at `time_seconds`: k <= floor(t * rate) because a particle is not
// born before its birth time, and k > floor((t - lifetime) * rate) because the
// lifetime interval is half-open. Negative comp time yields no slots, and the
// population cap keeps the newest slots (identical birth ordering every time).
ParticleSlotRange live_slot_range(double time_seconds, double birth_rate, double lifetime,
                                  std::uint32_t population_cap) {
    if (!(birth_rate > 0.0) || population_cap == 0 || !(time_seconds >= 0.0)) {
        return {};
    }

    const double last_slot = std::floor(time_seconds * birth_rate);
    const double first_slot = std::floor((time_seconds - lifetime) * birth_rate) + 1.0;
    if (last_slot < 0.0 || last_slot > kMaxExactSlot || first_slot > kMaxExactSlot) {
        return {};
    }
    if (last_slot < first_slot) {
        return {};
    }

    auto first = static_cast<std::uint64_t>(first_slot > 0.0 ? first_slot : 0.0);
    const auto last = static_cast<std::uint64_t>(last_slot);

    std::uint64_t alive = last - first + 1;
    if (alive > population_cap) {
        first = last - population_cap + 1;
        alive = population_cap;
    }

    return ParticleSlotRange{first, alive};
}

Result<ParticleSlotRange> checked_live_slot_range(const ValidatedSettings& settings, double time_seconds) {
    if (!std::isfinite(time_seconds)) {
        return Result<ParticleSlotRange>::failure(ErrorCode::invalid_time,
                                                  "non-finite frame time reached the simulation");
    }

    const Settings& values = settings.value;
    const ParticleSlotRange range = live_slot_range(time_seconds, values.birth_rate,
                                                    values.particle_lifetime_seconds, values.particle_count);
    if (range.count == 0 && values.birth_rate > 0.0 && values.particle_count > 0 &&
        std::floor(time_seconds * values.birth_rate) > kMaxExactSlot) {
        return Result<ParticleSlotRange>::failure(
            ErrorCode::invalid_time, "emission slot index exceeds the exactly representable range");
    }
    return Result<ParticleSlotRange>::success(range);
}

struct PartitionRange {
    std::uint64_t first_slot{0};
    std::uint64_t count{0};
};

PartitionRange partition_range(ParticleSlotRange range, std::uint32_t partition_count,
                               std::uint32_t partition_index) noexcept {
    if (range.count == 0) return {};

    const std::uint64_t last_slot = range.first_slot + range.count - 1;
    const std::uint64_t span = range.count - 1;
    const std::uint64_t first_remainder = range.first_slot % partition_count;
    const std::uint64_t offset =
        (static_cast<std::uint64_t>(partition_index) + partition_count - first_remainder) % partition_count;
    if (offset > span) return {};

    const std::uint64_t first_slot = range.first_slot + offset;
    return PartitionRange{first_slot, (last_slot - first_slot) / partition_count + 1};
}

ParticleInstance evaluate_particle(const Settings& values, double slots_elapsed, std::uint64_t slot) noexcept {
    ParticleInstance particle;
    const double birth_rate = values.birth_rate;
    // Age is derived from the slot distance instead of `t - k / rate`, which
    // keeps full relative precision for long comps and high birth rates.
    double age = (slots_elapsed - static_cast<double>(slot)) / birth_rate;
    if (age < 0.0) {
        age = 0.0; // knife-edge rounding at the newest slot
    }

    particle.id = slot;
    particle.age_seconds = age;
    particle.lifetime_seconds = values.particle_lifetime_seconds;
    const double age_fraction = values.particle_lifetime_seconds > 0.0
        ? std::clamp(age / values.particle_lifetime_seconds, 0.0, 1.0) : 0.0;
    if (values.appearance_enabled) {
        particle.size_pixels = values.particle_size +
            (values.particle_size_end - values.particle_size) * age_fraction;
        particle.opacity = values.opacity + (values.opacity_end - values.opacity) * age_fraction;
        particle.color = Vec3{
            values.color_start.x + (values.color_end.x - values.color_start.x) * age_fraction,
            values.color_start.y + (values.color_end.y - values.color_start.y) * age_fraction,
            values.color_start.z + (values.color_end.z - values.color_start.z) * age_fraction};
    } else {
        particle.size_pixels = values.particle_size;
        particle.opacity = values.opacity;
        particle.color = Vec3{1.0, 1.0, 1.0};
    }

    // Per-particle birth offset and velocity come from deterministic streams keyed
    // by (seed, id, purpose). This variation is what makes a steady emitter move:
    // with identical particles, births continuously replace the particles that
    // leave, so a correctly computed sequence still looks frozen on playback.
    const Vec3 birth = birth_offset(values.emitter_shape, values.emitter_size, values.seed, slot);
    Vec3 particle_velocity = values.velocity;
    // Emission direction model (M3-04): a per-particle direction on the cone (or the
    // sphere) times the emitted speed. Independent streams keep every frame identical
    // for the same requested time, like the rest of the random variation.
    const double speed_jitter = values.emission_speed_random > 0.0
        ? symmetric_value(values.seed, slot, RandomPurpose::emission_speed) * values.emission_speed_random
        : 0.0;
    const double particle_speed = values.emission_speed + speed_jitter;
    if (particle_speed != 0.0) {
        const Vec3 direction = emission_direction(values, slot);
        particle_velocity.x += direction.x * particle_speed;
        particle_velocity.y += direction.y * particle_speed;
        particle_velocity.z += direction.z * particle_speed;
    }
    if (values.velocity_spread > 0.0) {
        particle_velocity.x += symmetric_value(values.seed, slot, RandomPurpose::velocity_x) * values.velocity_spread;
        particle_velocity.y += symmetric_value(values.seed, slot, RandomPurpose::velocity_y) * values.velocity_spread;
        particle_velocity.z += symmetric_value(values.seed, slot, RandomPurpose::velocity_z) * values.velocity_spread;
    }

    // Closed-form integration for each particle age makes arbitrary-time,
    // out-of-order rendering independent of frame stepping and render history.
    const DragIntegrals factors = drag_integrals(values.linear_drag, age);
    particle.position.x = values.emitter_origin.x + birth.x + particle_velocity.x * factors.velocity_displacement +
                          values.gravity.x * factors.acceleration_displacement;
    particle.position.y = values.emitter_origin.y + birth.y + particle_velocity.y * factors.velocity_displacement +
                          values.gravity.y * factors.acceleration_displacement;
    particle.position.z = values.emitter_origin.z + birth.z + particle_velocity.z * factors.velocity_displacement +
                          values.gravity.z * factors.acceleration_displacement;
    return particle;
}

template <typename StoreParticle>
Result<std::size_t> simulate_partition(const ValidatedSettings& settings, double time_seconds,
                                       std::uint32_t partition_count,
                                       ParticleSlotRange live_range, PartitionRange assigned_range,
                                       const Cancellation& cancellation, StoreParticle&& store_particle) {
    const std::size_t slot_count = static_cast<std::size_t>(assigned_range.count);
    const double slots_elapsed = time_seconds * settings.value.birth_rate;
    for (std::size_t i = 0; i < slot_count; ++i) {
        if ((i % kCancellationCheckInterval) == 0 && cancellation.is_cancelled()) {
            return Result<std::size_t>::failure(ErrorCode::cancelled,
                                                "cancelled during particle simulation");
        }

        const std::uint64_t slot = assigned_range.first_slot +
            static_cast<std::uint64_t>(i) * partition_count;
        store_particle(i, slot, live_range.first_slot,
                       evaluate_particle(settings.value, slots_elapsed, slot));
    }
    return Result<std::size_t>::success(slot_count);
}

} // namespace

Result<std::vector<ParticleInstance>> simulate_particles(const ValidatedSettings& settings, double time_seconds,
                                                        const Cancellation& cancellation) {
    return simulate_particles_partition(settings, time_seconds, 1, 0, cancellation);
}

Result<std::vector<ParticleInstance>> simulate_particles_partition(const ValidatedSettings& settings,
                                                                   double time_seconds,
                                                                   std::uint32_t partition_count,
                                                                   std::uint32_t partition_index,
                                                                   const Cancellation& cancellation) {
    if (partition_count == 0 || partition_index >= partition_count) {
        return Result<std::vector<ParticleInstance>>::failure(ErrorCode::invalid_request,
                                                             "invalid particle slot partition");
    }

    const auto live_result = checked_live_slot_range(settings, time_seconds);
    if (!live_result.has_value()) return Result<std::vector<ParticleInstance>>::failure(live_result.error());
    const ParticleSlotRange live_range = live_result.value();
    const PartitionRange assigned_range = partition_range(live_range, partition_count, partition_index);
    const std::size_t slot_count = static_cast<std::size_t>(assigned_range.count);

    std::vector<ParticleInstance> particles;
    try {
        particles.resize(slot_count);
    } catch (const std::bad_alloc&) {
        return Result<std::vector<ParticleInstance>>::failure(ErrorCode::allocation_failed,
                                                             "particle list allocation failed");
    }

    auto filled = simulate_partition(settings, time_seconds, partition_count,
        live_range, assigned_range, cancellation,
        [&particles](std::size_t index, std::uint64_t, std::uint64_t, ParticleInstance particle) {
            particles[index] = std::move(particle);
        });
    if (!filled.has_value()) return Result<std::vector<ParticleInstance>>::failure(filled.error());

    return Result<std::vector<ParticleInstance>>::success(std::move(particles));
}

Result<ParticleSlotRange> live_particle_slot_range(const ValidatedSettings& settings, double time_seconds) {
    return checked_live_slot_range(settings, time_seconds);
}

Result<std::size_t> simulate_particles_partition_into(const ValidatedSettings& settings, double time_seconds,
                                                      std::uint32_t partition_count,
                                                      std::uint32_t partition_index,
                                                      std::span<ParticleInstance> destination,
                                                      const Cancellation& cancellation) {
    if (partition_count == 0 || partition_index >= partition_count) {
        return Result<std::size_t>::failure(ErrorCode::invalid_request,
                                            "invalid particle slot partition");
    }

    const auto live_result = checked_live_slot_range(settings, time_seconds);
    if (!live_result.has_value()) return Result<std::size_t>::failure(live_result.error());
    const ParticleSlotRange live_range = live_result.value();
    if (live_range.count != static_cast<std::uint64_t>(destination.size())) {
        return Result<std::size_t>::failure(ErrorCode::invalid_request,
                                            "particle destination size does not match the live slot range");
    }

    const PartitionRange assigned_range = partition_range(live_range, partition_count, partition_index);
    return simulate_partition(settings, time_seconds, partition_count,
        live_range, assigned_range, cancellation,
        [destination, first_slot = live_range.first_slot](std::size_t, std::uint64_t slot,
                                                          std::uint64_t, ParticleInstance particle) {
            destination[static_cast<std::size_t>(slot - first_slot)] = std::move(particle);
        });
}

} // namespace starfield::core
