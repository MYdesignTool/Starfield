#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "GraphProjection.hpp"
#include <type_traits>
namespace starfield::adapter {
// Private synchronous read. Caller owns the output; no STL/host lifetimes cross
// modules. Expected receipt prevents warnings from an Undo branch being reused.
struct GraphDiagnosticsRequest {
    std::uint32_t magic{0x53464431},bytes{sizeof(GraphDiagnosticsRequest)},version{1};
    AEGP_PluginID plugin{};AEGP_EffectRefH renderer{};
    A_long revision{};std::uint32_t checksum{},acknowledged{},count{};
    PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
    std::array<graph_projection::Issue,graph_projection::max_edges> issues{};
};
static_assert(std::is_standard_layout_v<GraphDiagnosticsRequest> && std::is_trivially_copyable_v<GraphDiagnosticsRequest>);
PF_Err read_graph_diagnostics(PF_InData*,GraphDiagnosticsRequest&) noexcept;
}
