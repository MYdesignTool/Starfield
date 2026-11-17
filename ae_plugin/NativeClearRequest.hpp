#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "Parameters.hpp"
#include "starfield/core/Graph.hpp"
#include <array>
#include <cstdint>
#include <type_traits>

namespace starfield::adapter {
// Caller-owned, operation-local numeric storage; no allocations or AE handles
// survive a callback or transfer CRT ownership between AEX modules.
struct NativeClearSnapshot {
    A_long revision{},high{},low{};
    A_Time time{};
    std::uint64_t epoch{};
    std::uint32_t selected_count{},removed_count{};
    std::array<core::NodeId,63> selected{},removed{};
    std::array<core::NodeId,kNativeBindingCapacity> owners{};
    std::array<double,kNativeBindingCapacity> values{};
    std::array<bool,kNativeBindingCapacity> present{};
    bool captured{};
};
struct NativeClearRequest {
    std::uint32_t magic{0x53464331}; // SFC1 capture; SFC2 publish
    std::uint32_t bytes{sizeof(NativeClearRequest)},version{1};
    AEGP_PluginID plugin{};
    AEGP_EffectRefH renderer{};
    AEGP_LayerH layer{};
    NativeClearSnapshot* snapshot{};
    A_long nonce{};
    std::uint32_t acknowledged{};
    PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
};
static_assert(std::is_trivially_copyable_v<NativeClearSnapshot> && std::is_standard_layout_v<NativeClearRequest>);
}
