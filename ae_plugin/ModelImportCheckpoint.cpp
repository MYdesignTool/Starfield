#include "ModelImportCheckpoint.hpp"
#include "ModelGeometryParameter.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include <cmath>
#include <cstring>
#include <limits>
#include <new>

namespace starfield::adapter {
namespace {
namespace layout=native_nodes::model_layout;
template<class T>struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
bool same_time(std::int32_t a,std::uint32_t as,std::int32_t b,std::uint32_t bs) noexcept {
    return as && bs && std::int64_t(a)*bs==std::int64_t(b)*as;
}
}
PF_Err capture_model_import_checkpoint(PF_InData* data,ModelImportCheckpoint& out) noexcept try {
    out={};const auto plugin=node_graph_sync_plugin_id();
    if(!data || !data->pica_basicP || !data->effect_ref || !data->time_scale || !plugin ||
       (data->in_flags&PF_InFlag_PROJECT_IS_RENDER_ONLY))return PF_Err_BAD_CALLBACK_PARAM;
    auto* basic=data->pica_basicP;
    Suite<AEGP_PFInterfaceSuite1> pf(basic,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_EffectSuite4> effects(basic,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(basic,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_ProjSuite6> projects(basic,kAEGPProjSuite,kAEGPProjSuiteVersion6);
    Suite<AEGP_ItemSuite9> items(basic,kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(basic,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    if(!pf.value || !effects.value || !streams.value || !projects.value || !items.value || !comps.value || !layers.value)
        return PF_Err_BAD_CALLBACK_PARAM;
    ModelImportCheckpoint current;AEGP_ProjectH project{};AEGP_ItemH root{},item{};AEGP_LayerH layer{};AEGP_CompH comp{};
    AEGP_EffectRefH effect{};
    struct Effect {const AEGP_EffectSuite4* suite;AEGP_EffectRefH& ref;~Effect(){if(ref)suite->AEGP_DisposeEffect(ref);}} owned{effects.value,effect};
    auto error=pf.value->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&effect);
    if(error || !effect)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    AEGP_InstalledEffectKey key{};A_char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};AEGP_EffectFlags flags{};A_long count{};A_Time time{};
    if((error=effects.value->AEGP_GetInstalledKeyFromLayerEffect(effect,&key)) ||
       (error=effects.value->AEGP_GetEffectMatchName(key,match)) || std::strcmp(match,"org.starfieldfx.node.model") ||
       (error=effects.value->AEGP_GetEffectFlags(effect,&flags)) ||
       (error=streams.value->AEGP_GetEffectNumParamStreams(effect,&count)) || count!=101 ||
       (error=projects.value->AEGP_GetProjectByIndex(0,&project)) || !project ||
       (error=projects.value->AEGP_GetProjectRootFolder(project,&root)) || !root ||
       (error=items.value->AEGP_GetItemID(root,&current.project)) || current.project<=0 ||
       (error=pf.value->AEGP_GetEffectLayer(data->effect_ref,&layer)) || !layer ||
       (error=layers.value->AEGP_GetLayerParentComp(layer,&comp)) || !comp ||
       (error=comps.value->AEGP_GetItemFromComp(comp,&item)) || !item ||
       (error=items.value->AEGP_GetItemID(item,&current.comp)) || current.comp<=0 ||
       (error=layers.value->AEGP_GetLayerID(layer,&current.layer)) || current.layer<=0 ||
       (error=layers.value->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time)) ||
       !same_time(time.value,time.scale,data->current_time,data->time_scale))
        return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    current.flags=flags;current.time_value=time.value;current.time_scale=time.scale;
    for(A_long index=1;index<count;++index){
        if(index==layout::import_obj)continue;
        AEGP_StreamRefH ref{};AEGP_StreamValue2 value{};bool read{};
        struct Stream {const AEGP_StreamSuite6* suite;AEGP_StreamRefH& ref;AEGP_StreamValue2& value;bool& read;
            ~Stream(){if(read)suite->AEGP_DisposeStreamValue(&value);if(ref)suite->AEGP_DisposeStream(ref);}} stream{streams.value,ref,value,read};
        error=streams.value->AEGP_GetNewEffectStreamByIndex(plugin,effect,index,&ref);
        if(error || !ref)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
        AEGP_StreamType type{};error=streams.value->AEGP_GetStreamType(ref,&type);
        if(error || type!=(index==layout::mesh?AEGP_StreamType_ARB:AEGP_StreamType_OneD))
            return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
        error=streams.value->AEGP_GetNewStreamValue(plugin,ref,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
        if(error)return static_cast<PF_Err>(error);read=true;
        if(index==layout::mesh){auto bytes=copy_model_geometry_parameter_bytes(data,reinterpret_cast<PF_ArbitraryH>(value.val.arbH));
            if(!bytes.has_value())return bytes.error().code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM;
            current.mesh=std::move(bytes.value());
        }else {if(!std::isfinite(value.val.one_d))return PF_Err_BAD_CALLBACK_PARAM;current.numbers[index]=value.val.one_d;}
        error=streams.value->AEGP_DisposeStreamValue(&value);read=false;if(error)return static_cast<PF_Err>(error);
    }
    const auto source=current.numbers[layout::source],revision=current.numbers[layout::revision];
    constexpr auto uuid=native_nodes::uuid_first_index(native_nodes::Kind::model);
    bool nonzero{};
    for(A_long index=uuid;index<uuid+8;++index){const auto part=current.numbers[index];
        if(part<0 || part>65535 || std::floor(part)!=part)return PF_Err_BAD_CALLBACK_PARAM;nonzero|=part!=0;}
    if(!nonzero || current.numbers[native_nodes::sync_guard_index(native_nodes::Kind::model)]!=0 ||
       (source!=1 && source!=2) || revision<0 || revision>=(std::numeric_limits<A_long>::max)() || std::floor(revision)!=revision)
        return PF_Err_BAD_CALLBACK_PARAM;
    out=std::move(current);return PF_Err_NONE;
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
PF_Err validate_model_import_checkpoint(PF_InData* data,const ModelImportCheckpoint& expected) noexcept {
    ModelImportCheckpoint current;const auto error=capture_model_import_checkpoint(data,current);if(error)return error;
    return current.project==expected.project && current.comp==expected.comp && current.layer==expected.layer &&
        same_time(current.time_value,current.time_scale,expected.time_value,expected.time_scale) && current.flags==expected.flags &&
        current.numbers==expected.numbers && current.mesh==expected.mesh?PF_Err_NONE:PF_Err_BAD_CALLBACK_PARAM;
}
}
