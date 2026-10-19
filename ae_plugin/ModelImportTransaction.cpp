#include "ModelImportUI.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "SPBasic.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
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
    AEGP_StreamRefH ref{};AEGP_StreamValue2 value{};bool read{};
};
PF_Err compare_mesh(PF_InData* data,PF_ArbitraryH a,PF_ArbitraryH b) noexcept {
    PF_ArbParamsExtra extra{};extra.id=static_cast<A_short>(layout::disk_id(layout::mesh));extra.which_function=PF_Arbitrary_COMPARE_FUNC;
    PF_ArbCompareResult equal{};extra.u.compare_func_params.a_arbH=a;extra.u.compare_func_params.b_arbH=b;extra.u.compare_func_params.compareP=&equal;
    const auto error=model_geometry_arbitrary_callback(data,&extra,layout::disk_id(layout::mesh));
    return error?error:equal==PF_ArbCompare_EQUAL?PF_Err_NONE:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
}
PF_Err import_model_obj_text(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],
    std::string_view text,const core::Cancellation& cancel,const ModelImportCheckpoint* checkpoint,
    const EffectUiExclusion* owner) noexcept try {
    if(owner && (!*owner || !ui_transaction_active()))return PF_Err_BAD_CALLBACK_PARAM;
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
    core::ModelBounds bounds{};ModelObjDiagnostic diagnostic;PF_ArbitraryH candidate{};
    auto error=prepare_model_obj_parameter(data,text,&candidate,cancel,&bounds,&diagnostic);
    if(error){if(error!=PF_Interrupt_CANCEL)std::snprintf(out->return_msg,sizeof(out->return_msg),
        "Starfield OBJ: %s (line %llu, error %d).",diagnostic.detail?diagnostic.detail:"Mesh preparation failed",
        static_cast<unsigned long long>(diagnostic.line),int(error));return error;}
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
        void clear() noexcept {for(auto& field:fields){if(field.read)streams->AEGP_DisposeStreamValue(&field.value);
            if(field.ref)streams->AEGP_DisposeStream(field.ref);field={};}if(effect)effects->AEGP_DisposeEffect(effect);effect=nullptr;}
        ~Refs(){clear();}
    } refs{streams.value,effects.value};
    constexpr std::array<A_long,10> indices{guard,layout::mesh,layout::revision,layout::source,
        layout::author_bounds_first,layout::author_bounds_first+1,layout::author_bounds_first+2,
        layout::author_bounds_first+3,layout::author_bounds_first+4,layout::author_bounds_first+5};
    A_Err ae{};
    const A_Time time{data->current_time,data->time_scale};
    constexpr auto uuid_first=native_nodes::uuid_first_index(native_nodes::Kind::model);
    std::array<std::uint16_t,8> uuid{};bool nonzero{};
    for(A_long i=0;i<8;++i){const auto* p=params[uuid_first+i];
        if(!p || p->param_type!=PF_Param_FLOAT_SLIDER)return PF_Err_BAD_CALLBACK_PARAM;
        const auto v=p->u.fs_d.value;if(!std::isfinite(v)||v<0||v>65535||std::floor(v)!=v)return PF_Err_BAD_CALLBACK_PARAM;
        uuid[i]=static_cast<std::uint16_t>(v);nonzero|=v!=0;
    }
    if(!nonzero)return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_LayerH layer{};ae=pf.value->AEGP_GetEffectLayer(data->effect_ref,&layer);
    if(ae || !layer)return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
    const auto resolve_source=[&]()->PF_Err {
        A_long count{};auto e=effects.value->AEGP_GetLayerNumEffects(layer,&count);
        if(e || count<0 || count>10000)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
        for(A_long slot=0;slot<count;++slot){
            AEGP_EffectRefH candidate_effect{};e=effects.value->AEGP_GetLayerEffectByIndex(plugin,layer,slot,&candidate_effect);
            AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            if(!e && !candidate_effect)e=PF_Err_BAD_CALLBACK_PARAM;
            if(!e)e=effects.value->AEGP_GetInstalledKeyFromLayerEffect(candidate_effect,&key);
            if(!e)e=effects.value->AEGP_GetEffectMatchName(key,match);
            bool matched=!e && !std::strcmp(match,"org.starfieldfx.node.model");
            for(A_long i=0;matched && i<8;++i){
                AEGP_StreamRefH ref{};AEGP_StreamType type{};AEGP_StreamValue2 v{};bool read{};
                e=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,candidate_effect,uuid_first+i,&ref);
                if(!e && !ref)e=PF_Err_BAD_CALLBACK_PARAM;
                if(!e)e=streams.value->AEGP_GetStreamType(ref,&type);
                if(!e && type!=AEGP_StreamType_OneD)e=PF_Err_BAD_CALLBACK_PARAM;
                if(!e){e=streams.value->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&v);read=!e;}
                matched=!e && v.val.one_d==uuid[i];
                if(read)streams.value->AEGP_DisposeStreamValue(&v);if(ref)streams.value->AEGP_DisposeStream(ref);
            }
            if(e || (matched && refs.effect)){if(candidate_effect)effects.value->AEGP_DisposeEffect(candidate_effect);
                return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);}
            if(matched)refs.effect=candidate_effect;else if(candidate_effect)effects.value->AEGP_DisposeEffect(candidate_effect);
        }
        if(!refs.effect)return PF_Err_BAD_CALLBACK_PARAM;
        for(unsigned i=0;i<indices.size();++i){
            e=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,indices[i],&refs.fields[i].ref);
            if(e || !refs.fields[i].ref)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
            AEGP_StreamType type{};e=streams.value->AEGP_GetStreamType(refs.fields[i].ref,&type);
            if(e || type!=(i==1?AEGP_StreamType_ARB:AEGP_StreamType_OneD))return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
        }
        return PF_Err_NONE;
    };
    error=resolve_source();if(error)return error;
    for(unsigned i=0;i<indices.size();++i){auto& field=refs.fields[i];
        AEGP_StreamType type{};ae=streams.value->AEGP_GetStreamType(field.ref,&type);
        if(ae||type!=(i==1?AEGP_StreamType_ARB:AEGP_StreamType_OneD))return static_cast<PF_Err>(ae?ae:PF_Err_BAD_CALLBACK_PARAM);
        ae=streams.value->AEGP_GetNewStreamValue(plugin,field.ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&field.value);
        if(ae)return static_cast<PF_Err>(ae);field.read=true;
    }
    // Own before-values independently of the SDK stream/effect lifetime. No
    // source reference or AEGP-owned ARB value may cross graph publication.
    std::array<double,10> before{};std::array<bool,10> changed_fields{};
    for(unsigned i=0;i<before.size();++i)if(i!=1)before[i]=refs.fields[i].value.val.one_d;
    PF_ArbitraryH old_mesh{};PF_ArbParamsExtra clone{};clone.id=static_cast<A_short>(layout::disk_id(layout::mesh));
    clone.which_function=PF_Arbitrary_COPY_FUNC;
    clone.u.copy_func_params.src_arbH=reinterpret_cast<PF_ArbitraryH>(refs.fields[1].value.val.arbH);
    clone.u.copy_func_params.dst_arbPH=&old_mesh;
    error=model_geometry_arbitrary_callback(data,&clone,layout::disk_id(layout::mesh));if(error)return error;
    Mesh previous_mesh{data,old_mesh};
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
        changed_fields[i]=true;const auto e=streams.value->AEGP_SetStreamValue(plugin,value.streamH,&value);
        return e?static_cast<PF_Err>(e):verify(i,value);
    };
    params[guard]->u.fs_d.value=1;const char* stage="write author controls";
    const double components[]{bounds.minimum.x,bounds.minimum.y,bounds.minimum.z,bounds.maximum.x,bounds.maximum.y,bounds.maximum.z};
    error=write(0,1);if(!error)error=write(1,0,candidate);
    for(unsigned axis=0;axis<6&&!error;++axis)error=write(4+axis,components[axis]);
    if(!error)error=write(2,revision+1);if(!error)error=write(3,2);
    if(!error){stage="release author guard";error=write(0,0);}
    if(!error){params[guard]->u.fs_d.value=0;params[layout::source]->u.pd.value=2;params[layout::revision]->u.sd.value=static_cast<A_long>(revision+1);
        refs.clear();
        stage="publish Model graph";PF_UserChangedParamExtra changed{};changed.param_index=layout::source;
        error=owner?sync_model_import_graph_parameter(data,out,params,&changed,*owner):
            sync_node_graph_parameter(data,out,params,&changed);}
    if(error){
        refs.clear();params[guard]->u.fs_d.value=1;PF_Err rollback=resolve_source();
        if(!rollback){rollback=write(0,1);
            for(unsigned i=static_cast<unsigned>(refs.fields.size());i-->1;)if(changed_fields[i]){
                const auto restored=write(i,before[i],i==1?old_mesh:nullptr);if(restored)rollback=restored;}
            if(!rollback){const auto reset=write(0,before[0]);if(reset)rollback=reset;}
        }
        params[guard]->u.fs_d.value=rollback?1:before[0];
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
