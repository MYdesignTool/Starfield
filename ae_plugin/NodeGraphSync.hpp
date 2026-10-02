#pragma once

#include "AE_Effect.h"
#include "AE_GeneralPlug.h"

#include <cstddef>
#include <cstdint>
#include <array>
#include <cmath>
#include <type_traits>

namespace starfield::adapter::node_sync {

enum class ValueKind : std::uint32_t { scalar, point2, point3, color };

enum class Stage : std::uint32_t { context, controls, compile, snapshot,
    allocation, suites, capture, publish_scalars, publish_graph,
    verify_scalars, verify_graph, rollback, complete };

inline const char* stage_name(Stage stage) noexcept {
    switch (stage) {
        case Stage::context: return "context";
        case Stage::controls: return "main controls";
        case Stage::compile: return "node compile";
        case Stage::snapshot: return "snapshot";
        case Stage::allocation: return "graph allocation";
        case Stage::suites: return "AEGP suites";
        case Stage::capture: return "stream capture";
        case Stage::publish_scalars: return "publish scalars";
        case Stage::publish_graph: return "publish graph";
        case Stage::verify_scalars: return "verify scalars";
        case Stage::verify_graph: return "verify graph";
        case Stage::rollback: return "rollback";
        case Stage::complete: return "complete";
    }
    return "unknown";
}

// Local UI edit context; borrowed refs live only until direct publication returns.
// No cross-effect transport, persistent schema or render-thread state.
struct NativeEdit {
    std::uint32_t node_kind{};
    A_long parameter_index{};
    std::array<std::uint16_t, 8> uuid{};
    ValueKind value_kind{ValueKind::scalar};
    std::array<double, 4> value{};
    PF_Err status{PF_Err_BAD_CALLBACK_PARAM};
    A_long revision{};
    bool accepted{};
    Stage stage{Stage::context};
    mutable A_long stream_index{-1};
    AEGP_EffectRefH renderer{};
    AEGP_LayerH layer{};
    PF_UtilCallbacks* handles{};
    SPBasicSuite* basic{};
    A_long width{}, height{}, time{}, time_scale{};
    PF_RationalScale pixel_aspect{1, 1};
};
static_assert(std::is_trivially_copyable_v<NativeEdit>);

inline bool valid_edit(const NativeEdit& edit) noexcept {
    bool any = false;
    for (const auto word : edit.uuid) any = any || word != 0;
    if (edit.node_kind > 3 ||
        edit.parameter_index <= 0 || edit.parameter_index > 43 || !any ||
        static_cast<std::uint32_t>(edit.value_kind) > 3) return false;
    for (const auto value : edit.value) if (!std::isfinite(value)) return false;
    return true;
}

} // namespace starfield::adapter::node_sync
