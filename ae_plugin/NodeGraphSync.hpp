#pragma once
#include "ParticleLayout.hpp"
#include "NodeRecord.hpp"

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
    std::array<Field,native_nodes::particle_layout::curve_span> additional_fields{};
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
    const char* binding_stage{}; // static diagnostic text, local callback only
    A_long binding_parameter{-1};
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
    if (edit.node_kind > 4 || edit.node_kind == 2 || edit.additional_count>edit.additional_fields.size() ||
        !native_nodes::authored_parameter(static_cast<native_nodes::Kind>(edit.node_kind),edit.parameter_index) || !any ||
        static_cast<std::uint32_t>(edit.value_kind) > 3) return false;
    if(edit.node_kind==4 && (edit.parameter_index>14 || edit.additional_count ||
        edit.value_kind!=(edit.parameter_index==2?ValueKind::point2:ValueKind::scalar)))return false;
    for (const auto value : edit.value) if (!std::isfinite(value)) return false;
    if(edit.node_kind==1 && edit.parameter_index==native_nodes::particle_layout::seed_shift &&
        (edit.value_kind!=ValueKind::scalar || std::floor(edit.value[0])!=edit.value[0] ||
         edit.value[0]<-2147483648.0 || edit.value[0]>2147483647.0))return false;
    if(edit.node_kind==1 && edit.parameter_index==native_nodes::particle_layout::birth_chance &&
        (edit.value_kind!=ValueKind::scalar || edit.value[0]<0 || edit.value[0]>100))return false;
    for(unsigned i=0;i<edit.additional_count;++i) {
        const auto& field=edit.additional_fields[i];
        namespace layout=native_nodes::particle_layout;
        const bool cloud_activation=edit.node_kind==1 && field.index==layout::cloud_enabled &&
            field.kind==ValueKind::scalar && field.value==std::array<double,4>{1,0,0,0} &&
            ((edit.parameter_index>=layout::cloud_circles && edit.parameter_index<=layout::cloud_density) ||
             (edit.parameter_index==layout::shape && edit.value_kind==ValueKind::scalar && edit.value[0]==3));
        const bool birth_activation=edit.node_kind==1 && field.index==layout::birth_enabled &&
            field.kind==ValueKind::scalar && field.value==std::array<double,4>{1,0,0,0} &&
            (edit.parameter_index==layout::seed_shift || edit.parameter_index==layout::birth_chance);
        if(field.index<1 || (field.index>layout::last && !cloud_activation && !birth_activation) || static_cast<unsigned>(field.kind)>3)return false;
        for(double value:field.value)if(!std::isfinite(value))return false;
        for(unsigned j=0;j<i;++j)if(edit.additional_fields[j].index==field.index)return false;
    }
    return true;
}

inline bool gradient_bank_parameter(std::uint32_t kind,A_long index) noexcept {
    namespace layout=native_nodes::particle_layout;
    return kind==1 && index>=layout::gradient && index<=layout::gradient_interpolation;
}
inline bool age_bank_parameter(std::uint32_t kind,A_long index) noexcept {
    namespace layout=native_nodes::particle_layout;
    return kind==1 && layout::curve_base(index)!=0;
}
inline bool capture_age_bank(PF_ParamDef* params[],NativeEdit& edit) noexcept {
    namespace layout=native_nodes::particle_layout;
    if(!params || !age_bank_parameter(edit.node_kind,edit.parameter_index))return false;
    auto candidate=edit;candidate.additional_count=0;
    const auto base=layout::curve_base(edit.parameter_index);
    if(!params[base] || params[base]->param_type!=PF_Param_FLOAT_SLIDER)return false;
    const auto count=params[base]->u.fs_d.value;
    if(!std::isfinite(count) || std::floor(count)!=count || count<0 || count>layout::curve_points || count==1)return false;
    const auto capture=[&](A_long index) {
        const auto* param=params[index];if(!param || param->param_type!=PF_Param_FLOAT_SLIDER)return false;
        NativeEdit::Field field;field.index=index;field.value[0]=param->u.fs_d.value;
        candidate.additional_fields[candidate.additional_count++]=field;
        return true;
    };
    for(A_long index=base;index<=base+2*static_cast<A_long>(count);++index)if(!capture(index))return false;
    if(!capture(layout::curve_interpolation(base)))return false;
    if(!valid_edit(candidate))return false;
    edit=candidate;return true;
}
// A supervised follow-up can arrive before AEGP exposes every newly changed
// component. Capture the entire callback bank for both PF events and supervision.
inline bool capture_gradient_bank(PF_ParamDef* params[],NativeEdit& edit) noexcept {
    namespace layout=native_nodes::particle_layout;
    if(!params || !gradient_bank_parameter(edit.node_kind,edit.parameter_index))return false;
    auto candidate=edit;candidate.additional_count=0;
    for(A_long index=layout::gradient;index<=layout::gradient_interpolation;++index) {
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
