#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"

namespace starfield::core {

using ParticleSpriteBasis = std::array<double,9>;
inline constexpr std::size_t kMaxParticleSpriteBases = 4096;
inline constexpr double kMaxParticleSpriteBasisCoefficient = 1e12;
[[nodiscard]] bool valid_particle_sprite_basis(const ParticleSpriteBasis&) noexcept;

// Immutable, precomputed once per sampled node, never once per particle. This
// stage owns the math only; graph/native/CEP integration is tracked in M3-11.
class CompiledParticleTransform {
public:
    [[nodiscard]] Vec3 position(Vec3 value) const noexcept;
    [[nodiscard]] Vec3 velocity(Vec3 value) const noexcept;
    // Local system Scale changes positions. Particles Scale changes sprite size.
    // Inherited affine motion also carries its full linear basis, retaining
    // reflection/shear instead of attempting a lossy Euler decomposition.
    [[nodiscard]] Vec3 particle_axis(Vec3 value) const noexcept;
    [[nodiscard]] double particle_scale() const noexcept { return particle_scale_; }
    [[nodiscard]] double particle_opacity() const noexcept { return particle_opacity_; }
    [[nodiscard]] const std::array<double,16>& world_matrix() const noexcept { return world_; }
    [[nodiscard]] const ParticleSpriteBasis& particle_basis() const noexcept { return particle_basis_; }
    CompiledParticleTransform(const CompiledParticleTransform&) = default;
    CompiledParticleTransform(CompiledParticleTransform&&) = default;
    CompiledParticleTransform& operator=(const CompiledParticleTransform&) = default;
    CompiledParticleTransform& operator=(CompiledParticleTransform&&) = default;
    ~CompiledParticleTransform() = default;
private:
    CompiledParticleTransform() = default;
    std::array<double,16> world_{};
    ParticleSpriteBasis particle_basis_{};
    double particle_scale_{1.0}, particle_opacity_{1.0};
    friend Result<CompiledParticleTransform> compile_particle_transform(const ParticleTransformSettings&) noexcept;
    friend Result<CompiledParticleTransform> compose_particle_transforms(
        const CompiledParticleTransform&,const CompiledParticleTransform&) noexcept;
};

// Reject malformed/non-affine input before any particle or host state changes.
[[nodiscard]] Result<CompiledParticleTransform> compile_particle_transform(
    const ParticleTransformSettings&) noexcept;

// Apply before, then after. Centre and sprite maps compose independently.
[[nodiscard]] Result<CompiledParticleTransform> compose_particle_transforms(
    const CompiledParticleTransform& before,const CompiledParticleTransform& after) noexcept;

} // namespace starfield::core
