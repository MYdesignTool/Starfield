#pragma once
#include "ModelGeometryParameter.hpp"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {
inline constexpr A_long kModelMirrorFirstIndex=755, kModelMirrorCapacity=256;
inline constexpr A_long kModelMirrorFirstDiskId=1900, kModelMirrorCountIndex=1011, kModelMirrorCountDiskId=2200;
static_assert(kModelMirrorCapacity==core::kMaxModelSources && kModelMirrorFirstIndex+kModelMirrorCapacity==kModelMirrorCountIndex);
struct ModelMirrorValue {
    core::ModelResourceId id{};
    std::uint32_t revision{};
    core::ModelGeometry geometry;
};
struct ModelMirrorRecipe { core::NodeId node; std::uint32_t revision{}; core::ModelBounds bounds; };
[[nodiscard]] core::Result<std::vector<ModelMirrorRecipe>> model_mirror_recipes(const core::Graph&) noexcept;
[[nodiscard]] core::Result<ModelMirrorValue> read_model_mirror_parameter(
    PF_InData*,PF_ArbitraryH,const core::Cancellation&) noexcept;
[[nodiscard]] PF_Err create_model_mirror_parameter(PF_InData*,const ModelMirrorValue&,
    PF_ArbitraryH*,const core::Cancellation&) noexcept;
[[nodiscard]] PF_Err model_mirror_arbitrary_callback(PF_InData*,PF_ArbParamsExtra*) noexcept;
[[nodiscard]] PF_Err register_model_mirror_parameters(PF_InData*) noexcept;
}
