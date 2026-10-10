#include "ModelImportUI.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <new>

namespace starfield::adapter {
namespace {
namespace layout=native_nodes::model_layout;
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){
        if(basic)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct Saved {
    AEGP_StreamRefH ref{};AEGP_StreamValue2 value{};bool read{},changed{};
};
PF_Err compare_mesh(PF_InData* data,PF_ArbitraryH a,PF_ArbitraryH b) noexcept {
    PF_ArbParamsExtra extra{};extra.id=static_cast<A_short>(layout::disk_id(layout::mesh));extra.which_function=PF_Arbitrary_COMPARE_FUNC;
    PF_ArbCompareResult equal{};extra.u.compare_func_params.a_arbH=a;extra.u.compare_func_params.b_arbH=b;extra.u.compare_func_params.compareP=&equal;
    const auto error=model_geometry_arbitrary_callback(data,&extra,layout::disk_id(layout::mesh));
    return error?error:equal==PF_ArbCompare_EQUAL?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
}
PF_Err import_model_obj_text(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],
    std::string_view text,const core::Cancellation& cancel,const ModelImportCheckpoint* checkpoint) noexcept try {
    constexpr auto guard=native_nodes::sync_guard_index(native_nodes::Kind::model);
    const auto plugin=node_graph_sync_plugin_id();
    if(!data||!out||!data->pica_basicP||!data->effect_ref||!plugin||!data->time_scale||!params||
       !params[layout::source]||params[layout::source]->param_type!=PF_Param_POPUP||
       !params[layout::revision]||params[layout::revision]->param_type!=PF_Param_SLIDER||
       !params[layout::mesh]||params[layout::mesh]->param_type!=PF_Param_ARBITRARY_DATA||
       !params[guard]||params[guard]->param_type!=PF_Param_FLOAT_SLIDER||params[guard]->u.fs_d.value!=0)
        return PF_Err_BAD_CALLBACK_PARAM;
    for(int axis=0;axis<6;++axis)if(!params[layout::author_bounds_first+axis]||
        params[layout::author_bounds_first+axis]->param_type!=PF_Param_FLOAT_SLIDER)return PF_Err_BAD_CALLBACK_PARAM;
    core::ModelBounds bounds{};PF_ArbitraryH candidate{};auto error=prepare_model_obj_parameter(data,text,&candidate,cancel,&bounds);
    if(error){if(error!=PF_Interrupt_CANCEL)std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: invalid or oversized mesh (error %d).",int(error));return error;}
    struct Mesh{PF_InData* data;PF_ArbitraryH handle;~Mesh(){data->utils->host_dispose_handle(handle);}} mesh{data,candidate};
    if(checkpoint){const auto checked=validate_model_import_checkpoint(data,*checkpoint);
        if(checked){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: target or Model author changed before commit. Import cancelled.");return checked;}}
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!pf.value||!effects.value||!streams.value||!utility.value)return PF_Err_BAD_CALLBACK_PARAM;
    struct Refs {
        const AEGP_StreamSuite6* streams;const AEGP_EffectSuite4* effects;
        AEGP_EffectRefH effect{};std::array<Saved,10> fields{};
        ~Refs(){for(auto& field:fields){if(field.read)streams->AEGP_DisposeStreamValue(&field.value);
            if(field.ref)streams->AEGP_DisposeStream(field.ref);}if(effect)effects->AEGP_DisposeEffect(effect);}
    } refs{streams.value,effects.value};
    constexpr std::array<A_long,10> indices{guard,layout::mesh,layout::revision,layout::source,
        layout::author_bounds_first,layout::author_bounds_first+1,layout::author_bounds_first+2,
        layout::author_bounds_first+3,layout::author_bounds_first+4,layout::author_bounds_first+5};
    auto ae=pf.value->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&refs.effect);
    if(ae||!refs.effect)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
    const A_Time time{data->current_time,data->time_scale};
    for(unsigned i=0;i<indices.size();++i){auto& field=refs.fields[i];
        ae=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,indices[i],&field.ref);
        if(ae||!field.ref)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
        AEGP_StreamType type{};ae=streams.value->AEGP_GetStreamType(field.ref,&type);
        if(ae||type!=(i==1?AEGP_StreamType_ARB:AEGP_StreamType_OneD))return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
        ae=streams.value->AEGP_GetNewStreamValue(plugin,field.ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&field.value);
        if(ae)return static_cast<PF_Err>(ae);field.read=true;
    }
    const auto revision=refs.fields[2].value.val.one_d,source=refs.fields[3].value.val.one_d;
    if(refs.fields[0].value.val.one_d!=0||!std::isfinite(revision)||revision<0||
       revision>=(std::numeric_limits<A_long>::max)()||std::floor(revision)!=revision||(source!=1&&source!=2)||
       revision!=params[layout::revision]->u.sd.value||source!=params[layout::source]->u.pd.value)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    ae=utility.value->AEGP_StartUndoGroup("Starfield: Import OBJ");if(ae)return static_cast<PF_Err>(ae);
    struct Undo{const AEGP_UtilitySuite6* utility;~Undo(){utility->AEGP_EndUndoGroup();}} undo{utility.value};
    const auto old_source=params[layout::source]->u.pd.value;
    const auto old_revision=params[layout::revision]->u.sd.value;
    const auto verify=[&](unsigned i,const AEGP_StreamValue2& expected)->PF_Err {
        AEGP_StreamValue2 observed{};auto e=streams.value->AEGP_GetNewStreamValue(plugin,refs.fields[i].ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&observed);
        if(e)return static_cast<PF_Err>(e);
        const auto checked=i==1?compare_mesh(data,reinterpret_cast<PF_ArbitraryH>(expected.val.arbH),reinterpret_cast<PF_ArbitraryH>(observed.val.arbH)):
            expected.val.one_d==observed.val.one_d?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
        const auto disposed=streams.value->AEGP_DisposeStreamValue(&observed);return checked?checked:static_cast<PF_Err>(disposed);
    };
    const auto write=[&](unsigned i,double scalar,PF_ArbitraryH handle=nullptr)->PF_Err {
        AEGP_StreamValue2 value{};value.streamH=refs.fields[i].ref;
        if(handle)value.val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(handle);else value.val.one_d=scalar;
        refs.fields[i].changed=true;const auto e=streams.value->AEGP_SetStreamValue(plugin,value.streamH,&value);
        return e?static_cast<PF_Err>(e):verify(i,value);
    };
    params[guard]->u.fs_d.value=1;const char* stage="write author controls";
    const double components[]{bounds.minimum.x,bounds.minimum.y,bounds.minimum.z,bounds.maximum.x,bounds.maximum.y,bounds.maximum.z};
    error=write(0,1);if(!error)error=write(1,0,candidate);
    for(unsigned axis=0;axis<6&&!error;++axis)error=write(4+axis,components[axis]);
    if(!error)error=write(2,revision+1);if(!error)error=write(3,2);
    if(!error){stage="release author guard";error=write(0,0);}
    if(!error){params[guard]->u.fs_d.value=0;params[layout::source]->u.pd.value=2;params[layout::revision]->u.sd.value=static_cast<A_long>(revision+1);
        stage="publish Model graph";PF_UserChangedParamExtra changed{};changed.param_index=layout::source;
        error=sync_node_graph_parameter(data,out,params,&changed);}
    if(error){
        params[guard]->u.fs_d.value=1;PF_Err rollback=write(0,1);
        for(unsigned i=static_cast<unsigned>(refs.fields.size());i-->1;)if(refs.fields[i].changed){const auto e=streams.value->AEGP_SetStreamValue(plugin,refs.fields[i].ref,&refs.fields[i].value);
            const auto checked=e?static_cast<PF_Err>(e):verify(i,refs.fields[i].value);if(checked)rollback=checked;}
        const auto reset=streams.value->AEGP_SetStreamValue(plugin,refs.fields[0].ref,&refs.fields[0].value);
        const auto checked=reset?static_cast<PF_Err>(reset):verify(0,refs.fields[0].value);if(checked)rollback=checked;
        params[guard]->u.fs_d.value=refs.fields[0].value.val.one_d;
        params[layout::source]->u.pd.value=old_source;params[layout::revision]->u.sd.value=old_revision;
        if(!out->return_msg[0]||rollback)std::snprintf(out->return_msg,sizeof(out->return_msg),
            "Starfield OBJ import failed: %s (error %d, rollback %d).",stage,int(error),int(rollback));
        return rollback?PF_Err_INTERNAL_STRUCT_DAMAGED:error;
    }
    params[layout::source]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
    params[layout::revision]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
    for(int axis=0;axis<6;++axis){auto& field=*params[layout::author_bounds_first+axis];field.u.fs_d.value=components[axis];
        field.uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;}
    out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;return PF_Err_NONE;
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
