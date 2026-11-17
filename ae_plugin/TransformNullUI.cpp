#include "TransformNullUI.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "TextureLayerInventory.hpp"
#include "UiExclusionClient.hpp"
#include "AE_EffectCBSuites.h"
#include "AE_EffectCB.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace starfield::adapter {
namespace {
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;A_long version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v) {
        if(basic)(void)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));
    }
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->()const{return value;}
    explicit operator bool()const{return value!=nullptr;}
};

}

PF_Err read_transform_layers(PF_InData* data,std::vector<TransformLayerChoice>& choices,
                             AEGP_LayerIDVal& selected,bool full_inventory,A_long parameter_index) noexcept try {
    choices.clear();selected=0;const auto plugin=node_graph_sync_plugin_id();
    if(!data || !data->pica_basicP || !data->effect_ref || !plugin)return PF_Err_BAD_CALLBACK_PARAM;
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_LayerSuite9> layers(data->pica_basicP,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_MemorySuite1> memory(data->pica_basicP,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    Suite<AEGP_ItemSuite9> items(data->pica_basicP,kAEGPItemSuite,kAEGPItemSuiteVersion9);
    if(!pf || !layers || !streams || !effects || !memory)return PF_Err_BAD_CALLBACK_PARAM;
    if(parameter_index!=1 && !items)return PF_Err_BAD_CALLBACK_PARAM;
    struct Refs {const AEGP_StreamSuite6* streams;const AEGP_EffectSuite4* effects;
        AEGP_EffectRefH effect{};AEGP_StreamRefH source{};
        ~Refs(){if(source)streams->AEGP_DisposeStream(source);if(effect)effects->AEGP_DisposeEffect(effect);}
    } refs{streams.value,effects.value};
    AEGP_LayerH owner{};AEGP_CompH comp{};A_long count{};
    auto error=pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
    if(!error)error=layers->AEGP_GetLayerParentComp(owner,&comp);
    if(!error)error=layers->AEGP_GetCompNumLayers(comp,&count);
    if(error || !owner || !comp || count<0 || count>4096)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    error=pf->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&refs.effect);
    if(!error)error=streams->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,parameter_index,&refs.source);
    AEGP_StreamType type{};
    if(!error)error=streams->AEGP_GetStreamType(refs.source,&type);
    if(error || type!=AEGP_StreamType_LAYER_ID)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    // The Layer selector is constant structure; DRAW need not supply frame timing.
    const A_Time time{0,1};AEGP_StreamValue2 value{};
    if(!error)error=streams->AEGP_GetNewStreamValue(plugin,refs.source,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
    if(error)return static_cast<PF_Err>(error);
    selected=value.val.layer_id;streams->AEGP_DisposeStreamValue(&value);
    choices.push_back({0,u"None"});
    if(!full_inventory && !selected)return PF_Err_NONE;
    if(full_inventory && parameter_index!=1) {
        EffectUiExclusion exclusion;if(!exclusion)return PF_Err_BAD_CALLBACK_PARAM;
        Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
        if(utility && utility->AEGP_ExecuteScript) {
            Suite<AEGP_CompSuite11> comps(data->pica_basicP,kAEGPCompSuite,kAEGPCompSuiteVersion11);
            if(!comps || !items)return PF_Err_BAD_CALLBACK_PARAM;
            AEGP_ItemH item{};A_long comp_id{};AEGP_LayerIDVal owner_id{};
            error=comps->AEGP_GetItemFromComp(comp,&item);
            if(!error)error=items->AEGP_GetItemID(item,&comp_id);
            if(!error)error=layers->AEGP_GetLayerID(owner,&owner_id);
            if(error || comp_id<=0 || owner_id<=0)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
            struct ResultMemory {
                const AEGP_MemorySuite1* memory;AEGP_MemHandle result{},diagnostic{};bool locked{};
                ~ResultMemory(){if(locked)memory->AEGP_UnlockMemHandle(result);
                    if(result)memory->AEGP_FreeMemHandle(result);if(diagnostic)memory->AEGP_FreeMemHandle(diagnostic);}
            } owned{memory.value};
            const auto script=texture_layer_inventory_script(comp_id,owner_id);
            error=utility->AEGP_ExecuteScript(plugin,script.c_str(),FALSE,&owned.result,&owned.diagnostic);
            if(error || !owned.result)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
            void* result{};error=memory->AEGP_LockMemHandle(owned.result,&result);owned.locked=!error;
            if(error || !result)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
            AEGP_MemSize size{};error=memory->AEGP_GetMemHandleSize(owned.result,&size);
            if(error || size<1 || size>kTextureInventoryMaxBytes+1)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
            const auto* text=static_cast<const char*>(result);std::size_t length=0;
            while(length<size && text[length])++length;
            if(length==size)return PF_Err_BAD_CALLBACK_PARAM;
            return parse_texture_layer_inventory({text,length},comp_id,owner_id,choices);
        }
    }
    for(A_long i=0;i<count;++i) {
        AEGP_LayerH layer{};AEGP_LayerIDVal id{};
        error=layers->AEGP_GetCompLayerByIndex(comp,i,&layer);
        if(!error)error=layers->AEGP_GetLayerID(layer,&id);
        if(error)return static_cast<PF_Err>(error);
        if(!full_inventory && id!=selected)continue;
        if(full_inventory && parameter_index!=1) {
            AEGP_ItemH item{};AEGP_LayerFlags flags{};AEGP_ItemFlags item_flags{};
            AEGP_ItemType item_type{};
            if(layer==owner)continue;
            error=layers->AEGP_GetLayerFlags(layer,&flags);
            if(!error)error=layers->AEGP_GetLayerSourceItem(layer,&item);
            if(!error && item && !(flags&AEGP_LayerFlag_NULL_LAYER)) {
                error=items->AEGP_GetItemType(item,&item_type);
                if(!error && item_type==AEGP_ItemType_FOOTAGE)
                    error=items->AEGP_GetItemFlags(item,&item_flags);
            }
            if(error)return static_cast<PF_Err>(error);
            // A precomp is a renderable source even when it has no video track.
            // Track flags distinguish footage from audio; they are not a comp test.
            if(!item || (flags&AEGP_LayerFlag_NULL_LAYER) ||
               (item_type!=AEGP_ItemType_COMP &&
                (item_type!=AEGP_ItemType_FOOTAGE || !(item_flags&AEGP_ItemFlag_HAS_VIDEO))))continue;
        }
        AEGP_MemHandle name{};
        error=layers->AEGP_GetLayerName(plugin,layer,&name,nullptr);
        if(error)return static_cast<PF_Err>(error);
        struct Name {const AEGP_MemorySuite1* memory;AEGP_MemHandle handle;bool locked{};
            ~Name(){if(locked)memory->AEGP_UnlockMemHandle(handle);if(handle)memory->AEGP_FreeMemHandle(handle);}
        } owned{memory.value,name};
        if(!name)return PF_Err_BAD_CALLBACK_PARAM;
        void* text{};error=memory->AEGP_LockMemHandle(name,&text);owned.locked=!error;
        if(error || !text)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
        choices.push_back({id,reinterpret_cast<const char16_t*>(text)});
        if(!full_inventory)break;
    }
    return PF_Err_NONE;
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}

namespace {
PF_Err change_transform_null(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],
                            const AEGP_LayerIDVal* existing,A_long parameter_index=1) noexcept try {
    const auto guard_index=native_nodes::sync_guard_index(parameter_index==1?native_nodes::Kind::transform:native_nodes::Kind::particle);
    const auto plugin=node_graph_sync_plugin_id();
    if(!data || !out || !data->pica_basicP || !data->effect_ref || !plugin || !params || !params[parameter_index] ||
       params[parameter_index]->param_type!=PF_Param_LAYER || !params[guard_index] ||
       params[guard_index]->param_type!=PF_Param_FLOAT_SLIDER || params[guard_index]->u.fs_d.value!=0)
        return PF_Err_BAD_CALLBACK_PARAM;
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_LayerSuite9> layers(data->pica_basicP,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(data->pica_basicP,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!pf || !layers || !comps || !streams || !effects || !utility)return PF_Err_BAD_CALLBACK_PARAM;
    struct Refs {
        const AEGP_StreamSuite6* streams;const AEGP_EffectSuite4* effects;
        AEGP_EffectRefH effect{};AEGP_StreamRefH source{},guard{};
        ~Refs(){if(source)streams->AEGP_DisposeStream(source);if(guard)streams->AEGP_DisposeStream(guard);
            if(effect)effects->AEGP_DisposeEffect(effect);}
    } refs{streams.value,effects.value};
    AEGP_LayerH owner{};AEGP_CompH comp{};A_long layer_count{};
    A_Err error=pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
    if(!error)error=layers->AEGP_GetLayerParentComp(owner,&comp);
    if(!error)error=layers->AEGP_GetCompNumLayers(comp,&layer_count);
    if(error || !owner || !comp || layer_count<0 || layer_count>4096 || (!existing && layer_count==4096))
        return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    error=pf->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&refs.effect);
    if(!error)error=streams->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,parameter_index,&refs.source);
    if(!error)error=streams->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,guard_index,&refs.guard);
    AEGP_StreamType source_type{},guard_type{};
    if(!error)error=streams->AEGP_GetStreamType(refs.source,&source_type);
    if(!error)error=streams->AEGP_GetStreamType(refs.guard,&guard_type);
    if(error || source_type!=AEGP_StreamType_LAYER_ID || guard_type!=AEGP_StreamType_OneD)
        return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    const A_Time time{0,1};AEGP_StreamValue2 previous{};
    error=streams->AEGP_GetNewStreamValue(plugin,refs.source,AEGP_LTimeMode_LayerTime,&time,TRUE,&previous);
    if(error)return static_cast<PF_Err>(error);
    const auto previous_id=previous.val.layer_id;streams->AEGP_DisposeStreamValue(&previous);
    error=streams->AEGP_GetNewStreamValue(plugin,refs.guard,AEGP_LTimeMode_LayerTime,&time,TRUE,&previous);
    if(error)return static_cast<PF_Err>(error);
    const auto previous_guard=previous.val.one_d;streams->AEGP_DisposeStreamValue(&previous);
    if(previous_guard!=0)return PF_Err_BAD_CALLBACK_PARAM;
    if(existing && *existing) {
        bool found=false;
        for(A_long i=0;i<layer_count && !error;++i) {
            AEGP_LayerH layer{};AEGP_LayerIDVal id{};
            error=layers->AEGP_GetCompLayerByIndex(comp,i,&layer);
            if(!error)error=layers->AEGP_GetLayerID(layer,&id);
            if(!error && id==*existing){found=true;break;}
        }
        if(error || !found)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    }
    error=utility->AEGP_StartUndoGroup(parameter_index!=1?"Starfield: Select Texture Layer":existing?"Starfield: Select Transform Null":"Starfield: Create Transform Null");
    if(error)return static_cast<PF_Err>(error);
    struct Undo {const AEGP_UtilitySuite6* utility;~Undo(){utility->AEGP_EndUndoGroup();}} undo{utility.value};
    const auto guard=[&](double value) {
        AEGP_StreamValue2 v{};v.streamH=refs.guard;v.val.one_d=value;
        const auto e=streams->AEGP_SetStreamValue(plugin,refs.guard,&v);
        if(!e)params[guard_index]->u.fs_d.value=value;
        return e;
    };
    const auto select=[&](AEGP_LayerIDVal id) {
        AEGP_StreamValue2 v{};v.streamH=refs.source;v.val.layer_id=id;
        return streams->AEGP_SetStreamValue(plugin,refs.source,&v);
    };
    AEGP_LayerH created{};const char* stage="create Null";
    AEGP_LayerIDVal id=existing?*existing:0;
    if(!existing) {
    error=comps->AEGP_CreateNullInComp(reinterpret_cast<const A_UTF16Char*>(u"Starfield Transform Null"),comp,nullptr,&created);
    if(!error && !created)error=PF_Err_BAD_CALLBACK_PARAM;
    if(!error){stage="enable 3D";error=layers->AEGP_SetLayerFlag(created,AEGP_LayerFlag_LAYER_IS_3D,TRUE);}
    if(!error){stage="read Null ID";error=layers->AEGP_GetLayerID(created,&id);if(!error && id<=0)error=PF_Err_BAD_CALLBACK_PARAM;}
    if(!error){
        stage="name Null";
        std::array<char,96> name{};std::array<A_UTF16Char,96> utf{};
        std::snprintf(name.data(),name.size(),"Starfield Transform Null %ld",static_cast<long>(id));
        for(std::size_t i=0;name[i] && i+1<utf.size();++i)utf[i]=static_cast<A_UTF16Char>(name[i]);
        error=layers->AEGP_SetLayerName(created,utf.data());
    }
    }
    if(!error){stage="guard selection";error=guard(1);}
    if(!error){stage="reference Null";error=select(id);}
    if(!error){stage="release selection guard";error=guard(0);}
    if(!error){
        stage="publish layer binding";PF_UserChangedParamExtra changed{};changed.param_index=parameter_index;
        error=sync_node_graph_parameter(data,out,params,&changed);
    }
    if(error) {
        A_Err rollback=guard(1);const auto restore=select(previous_id);if(!rollback)rollback=restore;
        if(created){const auto remove=layers->AEGP_DeleteLayer(created);if(!rollback)rollback=remove;}
        const auto reset=guard(previous_guard);if(!rollback)rollback=reset;
        if(!out->return_msg[0] || rollback)std::snprintf(out->return_msg,sizeof(out->return_msg),
            "Starfield Create Null failed: %s (error %ld, rollback %ld).",stage,static_cast<long>(error),static_cast<long>(rollback));
        return static_cast<PF_Err>(error);
    }
    out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;return PF_Err_NONE;
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
PF_Err create_transform_null(PF_InData* data,PF_OutData* out,PF_ParamDef* params[]) noexcept {
    constexpr auto count=native_nodes::parameter_count(native_nodes::Kind::transform);
    constexpr auto guard=native_nodes::sync_guard_index(native_nodes::Kind::transform);
    if(!data || !out)return PF_Err_BAD_CALLBACK_PARAM;
    if(data->num_params==count && params)return change_transform_null(data,out,params,nullptr);
    // AE can deliver native buttons without params[] (as with Model Import).
    // Recover the saved numeric controls/metadata; never checkout/render a Layer.
    if(!data->inter.checkout_param || !data->inter.checkin_param)return PF_Err_INVALID_CALLBACK;
    std::array<PF_ParamDef,count> owned{};std::array<PF_ParamDef*,count> delivered{};
    for(A_long i=0;i<count;++i)delivered[i]=&owned[i];
    owned[1].param_type=PF_Param_LAYER;
    struct Checkins {
        PF_InData* data;decltype(owned)& values;A_long last=1;
        ~Checkins(){for(A_long i=2;i<=last;++i)(void)PF_CHECKIN_PARAM(data,&values[i]);}
    } checkins{data,owned};
    for(A_long i=2;i<=guard;++i) {
        const auto error=PF_CHECKOUT_PARAM(data,i,data->current_time,data->time_step,
            data->time_scale?data->time_scale:1,&owned[i]);
        if(error)return error;checkins.last=i;
    }
    auto recovered=*data;recovered.num_params=count;
    return change_transform_null(&recovered,out,delivered.data(),nullptr);
}
PF_Err set_transform_null_source(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],AEGP_LayerIDVal id) noexcept {
    if(id<0)return PF_Err_BAD_CALLBACK_PARAM;
    return change_transform_null(data,out,params,&id);
}

}
