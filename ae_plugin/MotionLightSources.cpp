#include "MotionLightSources.hpp"
#include "SPBasic.h"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <new>
#include <utility>

namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf{};
    const AEGP_LayerSuite9* layer{};
    const AEGP_MemorySuite1* memory{};
    bool pf_acquired{},layer_acquired{},memory_acquired{};
    A_Err close() noexcept {
        A_Err first{};
        const auto release=[&](bool& acquired,const char* name,A_long version){
            if(!acquired)return;acquired=false;
            const auto error=basic->ReleaseSuite(name,version);if(!first)first=error;
        };
        release(memory_acquired,kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
        release(layer_acquired,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        release(pf_acquired,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
        return first;
    }
    ~Suites(){(void)close();}
};
struct NameHandle {
    const AEGP_MemorySuite1* suite{};
    AEGP_MemHandle handle{};
    bool locked{};
    A_Err close() noexcept {
        A_Err first{};
        if(locked){locked=false;first=suite->AEGP_UnlockMemHandle(handle);}
        if(handle){const auto owned=handle;handle=nullptr;const auto error=suite->AEGP_FreeMemHandle(owned);if(!first)first=error;}
        return first;
    }
    ~NameHandle(){(void)close();}
};
A_Err copy_name(Suites& suites,AEGP_PluginID plugin,AEGP_LayerH layer,std::u16string& name){
    NameHandle owned{suites.memory};
    auto error=suites.layer->AEGP_GetLayerName(plugin,layer,&owned.handle,nullptr);
    if(error)return error;
    if(!owned.handle)return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_MemSize bytes{};
    error=suites.memory->AEGP_GetMemHandleSize(owned.handle,&bytes);if(error)return error;
    if(bytes<sizeof(A_UTF16Char)||bytes%sizeof(A_UTF16Char)||
       bytes>(kMaxMotionLightNameUnits+1)*sizeof(A_UTF16Char))return PF_Err_BAD_CALLBACK_PARAM;
    void* pointer{};error=suites.memory->AEGP_LockMemHandle(owned.handle,&pointer);
    if(error)return error;owned.locked=true;
    if(!pointer)return PF_Err_BAD_CALLBACK_PARAM;
    const auto* text=static_cast<const A_UTF16Char*>(pointer);
    const auto capacity=static_cast<std::size_t>(bytes/sizeof(A_UTF16Char));
    std::size_t length{};while(length<capacity&&text[length])++length;
    if(length==capacity||length>kMaxMotionLightNameUnits)return PF_Err_BAD_CALLBACK_PARAM;
    // Preserve UTF-16 code units exactly. No case-folding/normalization policy
    // is inferred from a reference label or from the system locale.
    name.reserve(length);
    for(std::size_t i=0;i<length;++i)name.push_back(static_cast<char16_t>(text[i]));
    return owned.close();
}
}
PF_Err select_motion_light_source_ids(std::span<const MotionLightSource> sources,
    std::u16string_view prefix,std::vector<std::uint32_t>& output) noexcept {
    if(sources.size()>kMaxMotionLightSources||prefix.size()>kMaxMotionLightNameUnits||
       prefix.find(char16_t{})!=std::u16string_view::npos)return PF_Err_BAD_CALLBACK_PARAM;
    for(std::size_t i=0;i<sources.size();++i){
        const auto& source=sources[i];
        if(!source.layer_id||source.layer_id>0x7fffffffu||source.comp_index>=65536||
           source.name.size()>kMaxMotionLightNameUnits||source.name.find(char16_t{})!=std::u16string::npos||
           (i&&source.comp_index<=sources[i-1].comp_index))return PF_Err_BAD_CALLBACK_PARAM;
        for(std::size_t j=0;j<i;++j)if(sources[j].layer_id==source.layer_id)return PF_Err_BAD_CALLBACK_PARAM;
    }
    try {
        std::vector<std::uint32_t> result;result.reserve(sources.size());
        for(const auto& source:sources)if(source.name.starts_with(prefix))result.push_back(source.layer_id);
        output.swap(result);return PF_Err_NONE;
    }catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
    catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
PF_Err capture_motion_light_sources(PF_InData* data,AEGP_PluginID plugin,
    const core::Cancellation& cancellation,std::vector<MotionLightSource>& output) noexcept {
    if(!data||!data->effect_ref||!data->time_scale||plugin<=0||!data->pica_basicP||
       !data->pica_basicP->AcquireSuite||!data->pica_basicP->ReleaseSuite)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
    try {
        Suites suites{data->pica_basicP};
        auto error=suites.basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&suites.pf));
        if(error)return static_cast<PF_Err>(error);suites.pf_acquired=true;
        if(!suites.pf||!suites.pf->AEGP_GetEffectLayer||!suites.pf->AEGP_ConvertEffectToCompTime)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&suites.layer));
        if(error)return static_cast<PF_Err>(error);suites.layer_acquired=true;
        if(!suites.layer||!suites.layer->AEGP_GetLayerParentComp||!suites.layer->AEGP_GetCompNumLayers||
           !suites.layer->AEGP_GetCompLayerByIndex||!suites.layer->AEGP_GetLayerObjectType||
           !suites.layer->AEGP_GetLayerID||!suites.layer->AEGP_GetLayerName||!suites.layer->AEGP_IsVideoActive)
            return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.basic->AcquireSuite(kAEGPMemorySuite,kAEGPMemorySuiteVersion1,reinterpret_cast<const void**>(&suites.memory));
        if(error)return static_cast<PF_Err>(error);suites.memory_acquired=true;
        if(!suites.memory||!suites.memory->AEGP_GetMemHandleSize||!suites.memory->AEGP_LockMemHandle||
           !suites.memory->AEGP_UnlockMemHandle||!suites.memory->AEGP_FreeMemHandle)return PF_Err_BAD_CALLBACK_PARAM;
        A_Time time{};AEGP_LayerH owner{};AEGP_CompH comp{};A_long count{};
        error=suites.pf->AEGP_ConvertEffectToCompTime(data->effect_ref,data->current_time,data->time_scale,&time);
        if(!error)error=suites.pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
        if(error)return static_cast<PF_Err>(error);
        if(!time.scale||!owner)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.layer->AEGP_GetLayerParentComp(owner,&comp);if(error)return static_cast<PF_Err>(error);
        if(!comp)return PF_Err_BAD_CALLBACK_PARAM;
        error=suites.layer->AEGP_GetCompNumLayers(comp,&count);if(error)return static_cast<PF_Err>(error);
        if(count<0||count>65536)return PF_Err_BAD_CALLBACK_PARAM;
        std::vector<MotionLightSource> result;
        for(A_long index=0;index<count;++index){
            if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
            AEGP_LayerH layer{};AEGP_ObjectType type{};
            error=suites.layer->AEGP_GetCompLayerByIndex(comp,index,&layer);if(error)return static_cast<PF_Err>(error);
            if(!layer)return PF_Err_BAD_CALLBACK_PARAM;
            error=suites.layer->AEGP_GetLayerObjectType(layer,&type);if(error)return static_cast<PF_Err>(error);
            if(type!=AEGP_ObjectType_LIGHT)continue;
            if(result.size()==kMaxMotionLightSources)return PF_Err_BAD_CALLBACK_PARAM;
            AEGP_LayerIDVal id{};AEGP_CompH parent{};A_Boolean active{};
            error=suites.layer->AEGP_GetLayerID(layer,&id);
            if(!error)error=suites.layer->AEGP_GetLayerParentComp(layer,&parent);
            if(error)return static_cast<PF_Err>(error);
            if(id<=0||parent!=comp||std::any_of(result.begin(),result.end(),[&](const auto& source){return source.layer_id==static_cast<std::uint32_t>(id);}))
                return PF_Err_BAD_CALLBACK_PARAM;
            error=suites.layer->AEGP_IsVideoActive(layer,AEGP_LTimeMode_CompTime,&time,&active);
            if(error)return static_cast<PF_Err>(error);
            MotionLightSource source{static_cast<std::uint32_t>(id),static_cast<std::uint32_t>(index),{},active!=FALSE};
            error=copy_name(suites,plugin,layer,source.name);if(error)return static_cast<PF_Err>(error);
            result.push_back(std::move(source));
        }
        if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
        error=suites.close();if(error)return static_cast<PF_Err>(error);
        if(cancellation.is_cancelled())return PF_Interrupt_CANCEL;
        output.swap(result);return PF_Err_NONE;
    }catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}
    catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
}
