#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include <cstdint>
#include <type_traits>
namespace starfield::adapter {
inline constexpr char graph_repair_command_name[]="Starfield Repair Selected Graph";
struct GraphRepairRequest {
    std::uint32_t magic{0x53465232},bytes{sizeof(GraphRepairRequest)},version{1};
    AEGP_PluginID plugin{};AEGP_LayerH layer{};
    std::uint32_t acknowledged{};PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
    char message[256]{};
};
static_assert(std::is_standard_layout_v<GraphRepairRequest> && std::is_trivially_copyable_v<GraphRepairRequest>);
[[nodiscard]] PF_Err repair_selected_graph(PF_InData*,GraphRepairRequest&,PF_OutData*) noexcept;
}
