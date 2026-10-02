#pragma once

#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

namespace node_sync { struct NativeEdit; }

// Reads per-node records from sibling hidden node effects on the supervised
// edit path. It must never be called from SmartFX pre-render or render.
[[nodiscard]] PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                                core::Graph& graph, bool& found_node_effects, AEGP_PluginID plugin_id,
                                                const node_sync::NativeEdit* edit = nullptr) noexcept;

} // namespace starfield::adapter
