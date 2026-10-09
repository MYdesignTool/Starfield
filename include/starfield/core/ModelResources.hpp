#pragma once
#include "starfield/core/ModelGeometry.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/ParticleTransform.hpp"
#include "starfield/core/Render.hpp"
#include <span>

namespace starfield::core {
[[nodiscard]] bool valid_model_instance(const ParticleModelInstance&) noexcept;
[[nodiscard]] Result<std::array<double,16>> model_local_matrix(const ModelLocalSettings&,
    const ModelBounds& = {{-.5,-.5,-.5},{.5,.5,.5}}) noexcept;
[[nodiscard]] Result<std::size_t> model_geometry_encoded_size(const ModelGeometry&) noexcept;
[[nodiscard]] Result<std::vector<std::byte>> encode_model_geometry(const ModelGeometry&,
    const Cancellation&) noexcept;
[[nodiscard]] Result<ModelGeometry> decode_model_geometry(std::span<const std::byte>,
    const Cancellation&) noexcept;
[[nodiscard]] Result<std::size_t> validate_model_resources(std::span<const ModelResource>,
    const Cancellation&) noexcept;
[[nodiscard]] Result<std::vector<ModelGeometryLease>> compile_model_resources(std::span<const ModelResource>,
    const Cancellation&) noexcept;
[[nodiscard]] Result<std::array<double,16>> model_particle_matrix(const ParticleInstance&,
    const FrameSpec&,std::span<const ParticleSpriteBasis> = {},
    const ParticleModelInstance& = {}) noexcept;
}
