#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include <cstdint>
#include <type_traits>

namespace starfield::adapter {
inline constexpr char graph_commit_command_name[] = "Starfield Commit Node Graph";
// Borrowed handles live only during EffectCallGeneric. No scripts, retained
// pointers, render calls, or persistent record changes cross this UI boundary.
struct GraphCommitRequest {
    std::uint32_t magic{0x53464731};
    std::uint32_t bytes{sizeof(GraphCommitRequest)};
    std::uint32_t version{1};
    AEGP_PluginID plugin{};
    AEGP_EffectRefH renderer{};
    AEGP_LayerH layer{};
    A_long nonce{};
    std::uint32_t acknowledged{};
    PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
};
static_assert(std::is_standard_layout_v<GraphCommitRequest> && std::is_trivially_copyable_v<GraphCommitRequest>);
}
