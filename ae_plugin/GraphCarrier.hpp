#pragma once

#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "starfield/core/Graph.hpp"
#include "NativeGraphCommit.hpp"

namespace starfield::adapter {

// Registers the effect with AEGP so supervised callbacks can address their own
// ordinary parameter streams. Failure disables the CEP graph carrier only; render
// and the legacy AE Controls path remain available.
[[nodiscard]] PF_Err register_graph_carrier(PF_InData* in_data) noexcept;
[[nodiscard]] AEGP_PluginID graph_carrier_plugin_id() noexcept;

// Handles the single commit trigger used by the CEP transaction bridge.
[[nodiscard]] PF_Err commit_graph_request(PF_InData* in_data, PF_OutData* out_data,
                                          PF_ParamDef* params[],
                                          PF_UserChangedParamExtra* extra) noexcept;

} // namespace starfield::adapter
