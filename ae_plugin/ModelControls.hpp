#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "ModelLayout.hpp"
#include "starfield/core/Graph.hpp"
#include "starfield/core/ModelResources.hpp"
#include <span>
#include <string_view>

namespace starfield::adapter {
struct ModelAuthorCapture {
    std::uint32_t source{},revision{};
    core::ModelLocalSettings pose{};
    core::ModelGeometry geometry{};
};
// Registers streams1..18 only. The node module appends shared identity/topology.
// No Model module or selector is exposed until the full resource path is ready.
[[nodiscard]] PF_Err register_model_author_controls(PF_InData*) noexcept;
// Capture is UI-side. All returned data is owned numeric storage; no borrowed
// host handle or pointer survives this callback.
[[nodiscard]] core::Result<ModelAuthorCapture> capture_model_author_controls(
    PF_InData*,std::span<PF_ParamDef* const>,const core::Cancellation&) noexcept;
[[nodiscard]] core::Result<core::GraphNode> model_author_graph_node(
    core::NodeId,const ModelAuthorCapture&,const core::Cancellation&) noexcept;
// Prepares a new owned handle, leaves every live control untouched on success
// and failure. Caller must commit through the UI transaction or dispose it.
[[nodiscard]] PF_Err prepare_model_obj_parameter(PF_InData*,std::string_view,
    PF_ArbitraryH*,const core::Cancellation&) noexcept;
}
