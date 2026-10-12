#include "MotionLightTiming.hpp"
#include "SPBasic.h"
#include "starfield/core/Render.hpp"
#include <array>
#include <utility>

namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic{};
    std::array<const char*,6> names{};
    std::array<A_long,6> versions{};
    std::size_t count{};
    template<class T>A_Err get(const char* name,A_long version,const T*& suite) noexcept {
        const auto error=basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite));
        if(error)return error;
        names[count]=name;versions[count++]=version;
        return suite?0:PF_Err_BAD_CALLBACK_PARAM;
    }
    A_Err close() noexcept {
        A_Err first{};
        while(count){--count;const auto error=basic->ReleaseSuite(names[count],versions[count]);if(!first)first=error;}
        return first;
    }
    ~Suites(){(void)close();}
};
struct Streams {
    const AEGP_StreamSuite6* suite{};
    std::array<AEGP_StreamRefH,4> refs{};
    A_Err close() noexcept {
        A_Err first{};
        for(auto it=refs.rbegin();it!=refs.rend();++it)if(*it){
            const auto ref=*it;*it=nullptr;const auto error=suite->AEGP_DisposeStream(ref);if(!first)first=error;
        }
        return first;
    }
    ~Streams(){(void)close();}
};
}
PF_Err capture_motion_light_timing(PF_InData* data,AEGP_PluginID plugin,std::uint32_t id,
    const core::Cancellation& cancel,MotionLightTiming& output) noexcept {
    if(!data||!data->effect_ref||plugin<=0||!id||id>0x7fffffffu||!data->pica_basicP||
       !data->pica_basicP->AcquireSuite||!data->pica_basicP->ReleaseSuite)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    Suites suites{data->pica_basicP};
    const AEGP_PFInterfaceSuite1* pf{};const AEGP_LayerSuite9* layer{};const AEGP_LightSuite2* light{};
    const AEGP_StreamSuite6* stream{};const AEGP_DynamicStreamSuite4* dynamic{};const AEGP_KeyframeSuite5* keys{};
    auto error=suites.get(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,pf);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,layer);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPLightSuite,kAEGPLightSuiteVersion2,light);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,stream);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4,dynamic);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5,keys);if(error)return static_cast<PF_Err>(error);
    if(!pf->AEGP_GetEffectLayer||!layer->AEGP_GetLayerParentComp||!layer->AEGP_GetLayerFromLayerID||
       !layer->AEGP_GetLayerID||!layer->AEGP_GetLayerObjectType||!light->AEGP_GetLightType||
       !stream->AEGP_GetNewLayerStream||!stream->AEGP_DisposeStream||!stream->AEGP_GetStreamType||
       !dynamic->AEGP_IsSeparationLeader||!dynamic->AEGP_AreDimensionsSeparated||!dynamic->AEGP_GetSeparationFollower||
       !keys->AEGP_GetStreamNumKFs||!keys->AEGP_GetKeyframeTime)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    AEGP_LayerH owner{},source{};AEGP_CompH comp{},source_comp{};AEGP_LayerIDVal actual{};
    error=pf->AEGP_GetEffectLayer(data->effect_ref,&owner);if(error)return static_cast<PF_Err>(error);
    if(!owner)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerParentComp(owner,&comp);if(error)return static_cast<PF_Err>(error);
    if(!comp)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerFromLayerID(comp,static_cast<AEGP_LayerIDVal>(id),&source);if(error)return static_cast<PF_Err>(error);
    if(!source)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerParentComp(source,&source_comp);if(error)return static_cast<PF_Err>(error);
    error=layer->AEGP_GetLayerID(source,&actual);if(error)return static_cast<PF_Err>(error);
    if(source_comp!=comp||actual!=static_cast<AEGP_LayerIDVal>(id))return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_ObjectType type{};AEGP_LightType light_type{};
    error=layer->AEGP_GetLayerObjectType(source,&type);if(error)return static_cast<PF_Err>(error);
    if(type!=AEGP_ObjectType_LIGHT)return PF_Err_BAD_CALLBACK_PARAM;
    error=light->AEGP_GetLightType(source,&light_type);if(error)return static_cast<PF_Err>(error);
    if(light_type!=AEGP_LightType_POINT)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    Streams owned{stream};
    error=stream->AEGP_GetNewLayerStream(plugin,source,AEGP_LayerStream_POSITION,&owned.refs[0]);if(error)return static_cast<PF_Err>(error);
    if(!owned.refs[0])return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_StreamType stream_type{};
    error=stream->AEGP_GetStreamType(owned.refs[0],&stream_type);if(error)return static_cast<PF_Err>(error);
    if(stream_type!=AEGP_StreamType_ThreeD&&stream_type!=AEGP_StreamType_ThreeD_SPATIAL)return PF_Err_BAD_CALLBACK_PARAM;
    A_Boolean leader{},separated{};
    error=dynamic->AEGP_IsSeparationLeader(owned.refs[0],&leader);if(error)return static_cast<PF_Err>(error);
    if(leader){error=dynamic->AEGP_AreDimensionsSeparated(owned.refs[0],&separated);if(error)return static_cast<PF_Err>(error);}
    MotionLightTiming result;result.layer_id=id;result.separated=separated!=0;
    const auto scan=[&](AEGP_StreamRefH ref)->A_Err {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        A_long count{};auto e=keys->AEGP_GetStreamNumKFs(ref,&count);if(e)return e;
        if(count<0)return PF_Err_BAD_CALLBACK_PARAM;
        if(!count)return 0;
        A_Time first{},last{};
        e=keys->AEGP_GetKeyframeTime(ref,0,AEGP_LTimeMode_CompTime,&first);if(e)return e;
        if(count==1)last=first;
        else{e=keys->AEGP_GetKeyframeTime(ref,count-1,AEGP_LTimeMode_CompTime,&last);if(e)return e;}
        auto a=core::make_rational(first.value,first.scale),b=core::make_rational(last.value,last.scale);
        if(!a||!b||(count>1&&core::compare(*a,*b)==0))return PF_Err_BAD_CALLBACK_PARAM;
        if(core::compare(*a,*b)>0)std::swap(a,b); // composition-time range, including reversed time mapping
        if(!result.position_keyframes||core::compare(*a,result.first_keyframe)<0)result.first_keyframe=*a;
        if(!result.position_keyframes||core::compare(*b,result.last_keyframe)>0)result.last_keyframe=*b;
        result.position_keyframes+=static_cast<std::uint64_t>(count);return 0;
    };
    if(separated)for(A_short dimension=0;dimension<3;++dimension){
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        error=dynamic->AEGP_GetSeparationFollower(owned.refs[0],dimension,&owned.refs[dimension+1]);if(error)return static_cast<PF_Err>(error);
        if(!owned.refs[dimension+1])return PF_Err_BAD_CALLBACK_PARAM;
        AEGP_StreamType follower_type{};
        error=stream->AEGP_GetStreamType(owned.refs[dimension+1],&follower_type);if(error)return static_cast<PF_Err>(error);
        if(follower_type!=AEGP_StreamType_OneD)return PF_Err_BAD_CALLBACK_PARAM;
        error=scan(owned.refs[dimension+1]);if(error)return static_cast<PF_Err>(error);
    }else{error=scan(owned.refs[0]);if(error)return static_cast<PF_Err>(error);}
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    error=owned.close();if(error)return static_cast<PF_Err>(error);
    error=suites.close();if(error)return static_cast<PF_Err>(error);
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    output=result;return PF_Err_NONE;
}
}
