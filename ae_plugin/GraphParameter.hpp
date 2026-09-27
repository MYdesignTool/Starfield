#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"
#include "starfield/core/GraphEvaluation.hpp"

namespace starfield::adapter {

inline constexpr A_short kGraphParameterId = 14;
inline constexpr A_long kControlSourceId = 15;
inline constexpr A_long kCaptureControlsId = 16;
inline constexpr A_long kLegacyControlSource = 1;
inline constexpr A_long kNodeControlSource = 2;

// Stable identities are scoped to a single graph, not the host effect instance.
[[nodiscard]] core::Result<core::Graph> graph_from_controls(const core::Settings& settings);
[[nodiscard]] core::Result<core::Graph> read_graph_parameter(PF_InData* in_data, PF_ArbitraryH handle);
[[nodiscard]] PF_Err create_graph_parameter(PF_InData* in_data, const core::Graph& graph,
                                           PF_ArbitraryH* output) noexcept;
[[nodiscard]] PF_Err graph_arbitrary_callback(PF_InData* in_data, PF_ArbParamsExtra* extra) noexcept;

} // namespace starfield::adapter
