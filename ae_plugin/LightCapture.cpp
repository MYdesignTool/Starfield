#include "LightCapture.hpp"
#include "SPBasic.h"
#include "AE_EffectCB.h"
#include "starfield/core/ModelLighting.hpp"
#include <algorithm>
#include <cmath>
#include <new>
namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf{};const AEGP_LayerSuite9* layer{};
    const AEGP_StreamSuite6* stream{};const AEGP_LightSuite2* light{};
    ~Suites(){if(!basic)return;if(light)basic->ReleaseSuite(kAEGPLightSuite,kAEGPLightSuiteVersion2);
        if(stream)basic->ReleaseSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
        if(layer)basic->ReleaseSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        if(pf)basic->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);}
    A_Err acquire(){auto e=basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&pf));
        if(!e)e=basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&layer));
        if(!e)e=basic->AcquireSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,reinterpret_cast<const void**>(&stream));
        if(!e)e=basic->AcquireSuite(kAEGPLightSuite,kAEGPLightSuiteVersion2,reinterpret_cast<const void**>(&light));return e;}
};
A_Err value(Suites& s,AEGP_PluginID id,AEGP_LayerH layer,AEGP_LayerStream which,const A_Time& t,double* out,bool color=false){
    AEGP_StreamRefH ref{};auto e=s.stream->AEGP_GetNewLayerStream(id,layer,which,&ref);if(e)return e;
    AEGP_StreamValue2 v{};e=s.stream->AEGP_GetNewStreamValue(id,ref,AEGP_LTimeMode_CompTime,&t,FALSE,&v);
    if(!e){if(color){out[0]=v.val.color.redF;out[1]=v.val.color.greenF;out[2]=v.val.color.blueF;}else *out=v.val.one_d;
        e=s.stream->AEGP_DisposeStreamValue(&v);}
    const auto disposed=s.stream->AEGP_DisposeStream(ref);return e?e:disposed;
}
}
PF_Err capture_lights(PF_InData* data,AEGP_PluginID id,bool needed,LightStaging& result) noexcept try {
    result={};if(!needed)return PF_Err_NONE;
    if(!data||!data->pica_basicP||!data->effect_ref||!id)return PF_Err_BAD_CALLBACK_PARAM;
    Suites s{data->pica_basicP};auto e=s.acquire();if(e)return static_cast<PF_Err>(e);
    A_Time time{};AEGP_LayerH owner{};AEGP_CompH comp{};A_Matrix4 matrix{};A_long count{};
    e=s.pf->AEGP_ConvertEffectToCompTime(data->effect_ref,data->current_time,data->time_scale,&time);
    if(!e)e=s.pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
    if(!e)e=s.layer->AEGP_GetLayerParentComp(owner,&comp);
    if(!e)e=s.layer->AEGP_GetLayerToWorldXform(owner,&time,&matrix);
    if(!e)e=s.layer->AEGP_GetCompNumLayers(comp,&count);
    if(e)return static_cast<PF_Err>(e);if(!time.scale||count<0||count>65536)return PF_Err_BAD_CALLBACK_PARAM;
    std::copy_n(&matrix.mat[0][0],16,result.layer_to_world);
    A_FpLong zoom{};A_short width{},height{};A_Matrix4 camera{};
    const auto camera_error=s.pf->AEGP_GetEffectCameraMatrix(data->effect_ref,&time,&camera,&zoom,&width,&height);
    if(!camera_error && zoom>0 && width>0 && height>0){result.camera_enabled=true;
        for(unsigned i=0;i<3;++i)result.camera_position[i]=camera.mat[3][i];}
    for(A_long i=0;i<count;++i){
        if(data->inter.abort && PF_ABORT(data))return PF_Interrupt_CANCEL;
        AEGP_LayerH layer{};AEGP_ObjectType object{};A_Boolean active{};
        e=s.layer->AEGP_GetCompLayerByIndex(comp,i,&layer);if(!e)e=s.layer->AEGP_GetLayerObjectType(layer,&object);
        if(e)return static_cast<PF_Err>(e);if(object!=AEGP_ObjectType_LIGHT)continue;
        e=s.layer->AEGP_IsVideoActive(layer,AEGP_LTimeMode_CompTime,&time,&active);
        if(e)return static_cast<PF_Err>(e);if(!active)continue;
        AEGP_LightType type{};e=s.light->AEGP_GetLightType(layer,&type);if(e)return static_cast<PF_Err>(e);
        if(type!=AEGP_LightType_AMBIENT&&type!=AEGP_LightType_PARALLEL&&type!=AEGP_LightType_POINT&&type!=AEGP_LightType_SPOT)continue;
        if(result.lights.size()>=core::kMaxModelLights)return PF_Err_BAD_CALLBACK_PARAM;
        SfModelLight light{};light.struct_size=sizeof(light);light.cone_degrees=90;light.cone_feather=.5;
        light.type=type==AEGP_LightType_AMBIENT?0:type==AEGP_LightType_PARALLEL?1:type==AEGP_LightType_POINT?2:3;
        e=value(s,id,layer,AEGP_LayerStream_COLOR,time,light.color,true);
        if(!e)e=value(s,id,layer,AEGP_LayerStream_INTENSITY,time,&light.intensity);
        if(e)return static_cast<PF_Err>(e);light.intensity=std::max(0.,light.intensity/100);
        if(light.type!=0){e=s.layer->AEGP_GetLayerToWorldXform(layer,&time,&matrix);if(e)return static_cast<PF_Err>(e);
            for(unsigned a=0;a<3;++a){light.position[a]=matrix.mat[3][a];light.direction[a]=matrix.mat[2][a];}}
        if(light.type==3){e=value(s,id,layer,AEGP_LayerStream_CONE_ANGLE,time,&light.cone_degrees);
            if(!e)e=value(s,id,layer,AEGP_LayerStream_CONE_FEATHER,time,&light.cone_feather);
            if(e)return static_cast<PF_Err>(e);light.cone_feather/=100;}
        if(light.type==2||light.type==3){double falloff{};e=value(s,id,layer,AEGP_LayerStream_LIGHT_FALLOFF_TYPE,time,&falloff);
            if(e)return static_cast<PF_Err>(e);if(!std::isfinite(falloff)||falloff<static_cast<double>(AEGP_LightFalloff_NONE)||falloff>static_cast<double>(AEGP_LightFalloff_INVERSE_SQUARE_CLAMPED)||std::floor(falloff)!=falloff)return PF_Err_BAD_CALLBACK_PARAM;
            light.falloff=static_cast<unsigned>(falloff);
            if(light.falloff){e=value(s,id,layer,AEGP_LayerStream_LIGHT_FALLOFF_START,time,&light.radius);
                if(!e)e=value(s,id,layer,AEGP_LayerStream_LIGHT_FALLOFF_DISTANCE,time,&light.falloff_distance);
                if(e)return static_cast<PF_Err>(e);}}
        result.lights.push_back(light);
    }
    return PF_Err_NONE;
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
void apply_lights(SfCoreRenderRequest& request,const LightStaging& staging) noexcept {
    request.light_count=static_cast<unsigned>(staging.lights.size());request.lights=staging.lights.data();
    request.light_camera_enabled=staging.camera_enabled?1:0;
    std::copy_n(staging.layer_to_world,16,request.lighting_layer_to_world);
    std::copy_n(staging.camera_position,3,request.lighting_camera_position);
}
}
