#pragma once

#include "NodeGraphSync.hpp"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

// Local helper shared by CEP supervised commits and native node publication.
[[nodiscard]] PF_Err write_graph_snapshot(PF_InData* in_data, PF_ParamDef* params[],
    const core::Graph& graph, A_long* new_revision = nullptr) noexcept;

// Called directly on the native UI edit path, never via another effect selector.
// Reads sibling node records and saves/verifies/restores renderer streams with AEGP.
[[nodiscard]] PF_Err commit_native_graph_edit(node_sync::NativeEdit* edit,
    AEGP_PluginID plugin_id, PF_OutData* out_data) noexcept;

// Explicit UI command: compile saved node streams, atomically publish and
// verify graph plus nonce. Does not rely on USER_CHANGED_PARAM delivery.
[[nodiscard]] PF_Err commit_panel_graph(PF_InData*, AEGP_EffectRefH,
    AEGP_LayerH, A_long nonce, AEGP_PluginID, PF_OutData*) noexcept;

} // namespace starfield::adapter
