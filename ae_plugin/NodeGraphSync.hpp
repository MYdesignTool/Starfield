#pragma once
#include "ParticleLayout.hpp"

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
    allocation, animation_bindings, suites, capture, publish_scalars, publish_graph,
    verify_scalars, verify_graph, rollback, complete };

inline const char* stage_name(Stage stage) noexcept {
    switch (stage) {
        case Stage::context: return "context";
        case Stage::controls: return "main controls";
        case Stage::compile: return "node compile";
        case Stage::snapshot: return "snapshot";
        case Stage::allocation: return "graph allocation";
        case Stage::animation_bindings: return "animation binding";
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
    struct Field { A_long index{}; ValueKind kind{}; std::array<double,4> value{}; };
    std::array<Field,17> additional_fields{};
    std::uint32_t additional_count{};
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
    if (edit.node_kind > 3 || edit.node_kind == 2 || edit.additional_count>17 ||
        edit.parameter_index <= 0 || edit.parameter_index > 81 || !any ||
        static_cast<std::uint32_t>(edit.value_kind) > 3) return false;
    for (const auto value : edit.value) if (!std::isfinite(value)) return false;
    for(unsigned i=0;i<edit.additional_count;++i) {
        const auto& field=edit.additional_fields[i];
        if(field.index<1 || field.index>81 || static_cast<unsigned>(field.kind)>3)return false;
        for(double value:field.value)if(!std::isfinite(value))return false;
        for(unsigned j=0;j<i;++j)if(edit.additional_fields[j].index==field.index)return false;
    }
    return true;
}

inline bool gradient_bank_parameter(std::uint32_t kind,A_long index) noexcept {
    namespace layout=native_nodes::particle_layout;
    return kind==1 && index>=layout::gradient && index<layout::gradient_first+16;
}
// A supervised follow-up can arrive before AEGP exposes every newly changed
// component. Capture the entire callback bank for both PF events and supervision.
inline bool capture_gradient_bank(PF_ParamDef* params[],NativeEdit& edit) noexcept {
    namespace layout=native_nodes::particle_layout;
    if(!params || !gradient_bank_parameter(edit.node_kind,edit.parameter_index))return false;
    auto candidate=edit;candidate.additional_count=0;
    for(A_long index=layout::gradient;index<layout::gradient_first+16;++index) {
        const auto* param=params[index];if(!param)return false;
        NativeEdit::Field field;field.index=index;
        const bool color=index>=layout::gradient_first && (index-layout::gradient_first)%2==1;
        if(color) {
            if(param->param_type!=PF_Param_COLOR)return false;
            field.kind=ValueKind::color;
            field.value={param->u.cd.value.red/255.0,param->u.cd.value.green/255.0,
                param->u.cd.value.blue/255.0,param->u.cd.value.alpha/255.0};
        } else {
            if(param->param_type!=PF_Param_FLOAT_SLIDER)return false;
            field.kind=ValueKind::scalar;field.value[0]=param->u.fs_d.value;
        }
        candidate.additional_fields[candidate.additional_count++]=field;
    }
    if(!valid_edit(candidate))return false;
    edit=candidate;return true;
}

} // namespace starfield::adapter::node_sync
