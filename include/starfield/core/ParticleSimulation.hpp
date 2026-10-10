#pragma once

#include "starfield/core/Render.hpp"
#include "starfield/core/Graph.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace starfield::core {
class CompiledParticleTransform;

// One evaluated particle for the requested absolute time. Positions are world
// space (ADR 0003): origin at the layer center, unit = one layer height.
struct ParticleInstance {
    std::uint64_t id{0};
    // Graph identity is (emitter_id, id). Standalone simulation uses a zero UUID;
    // id remains the emitter-local birth slot and seeds its random streams.
    NodeId emitter_id{};
    double age_seconds{0.0};
    double lifetime_seconds{0.0};
    double size_pixels{0.0};
    double opacity{0.0};
    std::uint32_t shape{0}; // Circle / Rectangle / Cloud / Texture / Model
    double size_y_pixels{0.0}; // 0 means use diameter in standalone settings
    Vec3 rotation_degrees{};
    double feather_percent{0};
    bool limit_to_2d{false};
    double anchor_x_percent{50},anchor_y_percent{50};
    std::uint32_t up_axis{2};
    Vec3 color{1.0, 1.0, 1.0};
    Vec3 position{};
    Vec3 velocity{}; // instantaneous world units/s, used by auxiliary inheritance
    // 0 preserves the identity path. Other indices are 1-based into the owning
    // EvaluatedGraph's shared table; never copy nine doubles per particle.
    std::uint32_t sprite_basis_index{0};
    ParticleTransferMode transfer_mode{ParticleTransferMode::normal};
    // 1-based shared style index; zero is reserved for non-texture particles.
    std::uint32_t texture_style_index{}, texture_random_key{};
    // Zero keeps legacy five-circle Cloud. Members remain one logical particle.
    std::uint32_t cloud_style_index{}, cloud_random_key{};
    // Zero selects the builtin cube; otherwise a 1-based shared geometry group.
    std::uint32_t model_style_index{};
    // Proper rotation in canonical world axes after the authored/shared basis.
    // Separate from shared affine entries; snapshot9 owns its explicit wire form.
    ParticleMotionPose motion_pose{kIdentityMotionPose};
};

// The contiguous global emission-slot interval alive at one absolute time.
// A zero count means no particles are alive.
struct ParticleSlotRange {
    std::uint64_t first_slot{0};
    std::uint64_t count{0};
};

// One branch's alive birth slots form a strided sequence. Count measures
// assigned particles, not the enclosing emitter's contiguous slot interval.
struct ParticleSlotSequence {
    std::uint64_t first_slot{0};
    std::uint64_t count{0};
    std::uint32_t stride{1};
};

struct ParticleSlotTarget {
    std::uint64_t slot{0};
    std::size_t destination{0};
};

// Stable random identity and age are independent of the emission clock.
[[nodiscard]] ParticleInstance simulate_particle_at_age(
    const Settings&, double age_seconds, std::uint64_t identity,
    EmitterDimensionContext = {},const CompiledParticleTransform* birth_transform = nullptr) noexcept;

// Use the branch's own half-open lifetime before applying its candidate cap.
// The graph merges these sequences and applies Output's single population cap.
[[nodiscard]] Result<ParticleSlotSequence> live_particle_branch_slots(
    const ValidatedSettings& settings, double time_seconds, std::uint32_t partition_count,
    std::uint32_t partition_index, bool cap_candidates = true);

// Evaluate only slots selected by the graph's global cap into their final
// positions. Caller supplies distinct destination indices; no allocation/sort.
[[nodiscard]] Result<std::size_t> simulate_selected_particles_into(
    const ValidatedSettings& settings, double time_seconds,
    std::span<const ParticleSlotTarget> targets, std::span<ParticleInstance> destination,
    const Cancellation& cancellation, EmitterDimensionContext dimension_context = {},
    const CompiledParticleTransform* birth_transform = nullptr);

// Deterministic particle evaluation for one absolute time.
//
// Emission slot `k` is born at `k / birth_rate` seconds on the effect clock, which
// is anchored at host time zero, and lives for `particle_lifetime_seconds`. A slot
// is visible while `0 <= age < lifetime`; the population is capped at
// `particle_count` by keeping the newest slots. Every call evaluates the requested
// absolute time from scratch, so frame order, repeats, and negative comp time
// cannot leak state between renders (ADR 0002).
//
// Per-particle birth offsets follow `emitter_shape`; Box/Sphere dimensions are
// expressed in full-resolution layer pixels and converted by `dimension_context`.
// Disc retains its dedicated `emitter_size` diameter. Each
// particle gets an independent velocity jitter of +/- `velocity_spread` per axis.
// Gravity and linear drag use closed-form integration, so no frame stepping or
// render history is required. Size, opacity, and RGB color interpolate linearly
// over normalized particle age. All results are pure functions of settings, seed,
// particle slot, and absolute time.
//
// `time_seconds` is the only lossy conversion allowed across this boundary.
[[nodiscard]] Result<std::vector<ParticleInstance>> simulate_particles(const ValidatedSettings& settings,
                                                                     double time_seconds,
                                                                     const Cancellation& cancellation,
                                                                     EmitterDimensionContext dimension_context = {});

// Evaluate only global emission slots assigned to one deterministic branch.
// Slot identity, birth time, and random streams stay unchanged: slot k is
// included when k % partition_count == partition_index. Output applies the
// global live-particle cap before every branch uses this partition.
// This lets graph Particle nodes split one emitter without simulating the full
// population once per branch.
[[nodiscard]] Result<std::vector<ParticleInstance>> simulate_particles_partition(
    const ValidatedSettings& settings, double time_seconds, std::uint32_t partition_count,
    std::uint32_t partition_index, const Cancellation& cancellation,
    EmitterDimensionContext dimension_context = {});

// Return the bounded global slot interval selected by Output for every branch.
[[nodiscard]] Result<ParticleSlotRange> live_particle_slot_range(
    const ValidatedSettings& settings, double time_seconds);

// Write one deterministic branch directly into a pre-sized, global-slot-ordered
// buffer. Call once for every partition index. The destination size must equal
// live_particle_slot_range(settings, time_seconds).count.
[[nodiscard]] Result<std::size_t> simulate_particles_partition_into(
    const ValidatedSettings& settings, double time_seconds, std::uint32_t partition_count,
    std::uint32_t partition_index, std::span<ParticleInstance> destination,
    const Cancellation& cancellation, EmitterDimensionContext dimension_context = {});

} // namespace starfield::core
