#pragma once

#include "AE_Effect.h"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

// Reads per-node records from sibling hidden node effects on the supervised
// edit path. It must never be called from SmartFX pre-render or render.
[[nodiscard]] PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                                core::Graph& graph, bool& found_node_effects) noexcept;

} // namespace starfield::adapter
