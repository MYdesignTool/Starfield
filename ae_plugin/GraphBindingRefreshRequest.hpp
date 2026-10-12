#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include <cstdint>
#include <type_traits>
namespace starfield::adapter {
// Separate private UI-only message; the existing graph commit wire is unchanged.
// Caller borrows the freshly acquired final-parade refs for this generic call.
struct GraphBindingRefreshRequest {
    std::uint32_t magic{0x53465231},bytes{sizeof(GraphBindingRefreshRequest)},version{1};
    AEGP_PluginID plugin{};AEGP_EffectRefH renderer{};AEGP_LayerH layer{};
    std::uint32_t acknowledged{};PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
};
static_assert(std::is_standard_layout_v<GraphBindingRefreshRequest> && std::is_trivially_copyable_v<GraphBindingRefreshRequest>);
}
