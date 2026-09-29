#pragma once

#include "AE_Effect.h"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

// Registers the effect with AEGP so supervised callbacks can address their own
// ordinary parameter streams. Failure disables the CEP graph carrier only; render
// and the legacy AE Controls path remain available.
[[nodiscard]] PF_Err register_graph_carrier(PF_InData* in_data) noexcept;

// Called before replacing the canonical arbitrary-data graph in a supervised AE
// callback. The new expression mirror is prepared first so failure leaves graph bytes
// untouched; it contains only the bounded, project-saved snapshot and revision.
[[nodiscard]] PF_Err write_graph_snapshot(PF_InData* in_data, const core::Graph& graph,
                                          A_long* new_revision = nullptr) noexcept;

// Handles the single commit trigger used by the CEP transaction bridge.
[[nodiscard]] PF_Err commit_graph_request(PF_InData* in_data, PF_OutData* out_data,
                                          PF_ParamDef* params[],
                                          PF_UserChangedParamExtra* extra) noexcept;

} // namespace starfield::adapter
