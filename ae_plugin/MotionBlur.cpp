#include "MotionBlur.hpp"
#include "Camera.hpp"
#include "EmitterHistory.hpp"
#include "AE_EffectCB.h"
#include "Param_Utils.h"
#include "AE_EffectSuites.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <new>

namespace starfield::adapter {
namespace {
PF_Err failure(PF_OutData* out,const char* detail,PF_Err error=PF_Err_BAD_CALLBACK_PARAM) {
    if(out && error!=PF_Interrupt_CANCEL)std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield motion blur: %s",detail);
    return error;
}
bool host_time(double seconds,core::RationalTime& time) {
    if(!std::isfinite(seconds) || std::abs(seconds)>double(std::numeric_limits<A_long>::max())-1)return false;
    const auto scale=std::int64_t(std::min(1000000.0,std::floor((double(std::numeric_limits<A_long>::max())-1)/std::max(1.0,std::abs(seconds)))));
    time={std::llround(seconds*scale),scale};return scale>0;
}
struct CompSuites {
    SPBasicSuite* basic{};const AEGP_PFInterfaceSuite1* pf{};
    const AEGP_LayerSuite9* layer{};const AEGP_CompSuite11* comp{};
    ~CompSuites() {
        if(comp)basic->ReleaseSuite(kAEGPCompSuite,kAEGPCompSuiteVersion11);
        if(layer)basic->ReleaseSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        if(pf)basic->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    }
};
PF_Err comp_window(PF_InData* data,bool& enabled,double& start,double& duration) {
    CompSuites suites{data->pica_basicP};if(!suites.basic)return PF_Err_BAD_CALLBACK_PARAM;
    auto error=suites.basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&suites.pf));
    if(!error)error=suites.basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&suites.layer));
    if(!error)error=suites.basic->AcquireSuite(kAEGPCompSuite,kAEGPCompSuiteVersion11,reinterpret_cast<const void**>(&suites.comp));
    AEGP_LayerH layer{};AEGP_CompH comp{};AEGP_LayerFlags lf{};AEGP_CompFlags cf{};
    if(!error)error=suites.pf->AEGP_GetEffectLayer(data->effect_ref,&layer);
    if(!error)error=suites.layer->AEGP_GetLayerParentComp(layer,&comp);
    if(!error)error=suites.layer->AEGP_GetLayerFlags(layer,&lf);
    if(!error)error=suites.comp->AEGP_GetCompFlags(comp,&cf);
    if(error)return static_cast<PF_Err>(error);
    enabled=(lf&AEGP_LayerFlag_MOTION_BLUR) && (cf&AEGP_CompFlag_ENABLE_MOTION_BLUR);
    if(!enabled)return PF_Err_NONE;
    A_Time now{},next{},opening{},length{};
    if(std::int64_t(data->current_time)+data->time_step>std::numeric_limits<A_long>::max())return PF_Err_BAD_CALLBACK_PARAM;
    error=suites.pf->AEGP_ConvertEffectToCompTime(data->effect_ref,data->current_time,data->time_scale,&now);
    if(!error)error=suites.pf->AEGP_ConvertEffectToCompTime(data->effect_ref,data->current_time+data->time_step,data->time_scale,&next);
    if(!error)error=suites.comp->AEGP_GetCompShutterFrameRange(comp,&now,&opening,&length);
    if(error)return static_cast<PF_Err>(error);
    if(!now.scale || !next.scale || !opening.scale || !length.scale)return PF_Err_BAD_CALLBACK_PARAM;
    const double c=double(now.value)/now.scale;
    const double slope=(double(next.value)/next.scale-c)*data->time_scale/data->time_step;
    if(!std::isfinite(slope) || std::abs(slope)<1e-12)return PF_Err_BAD_CALLBACK_PARAM;
    // Query the host's actual shutter interval; no assumption about A_Ratio units.
    start=double(data->current_time)/data->time_scale+(double(opening.value)/opening.scale-c)/slope;
    duration=double(length.value)/length.scale/slope;
    if(duration<0) {start+=duration;duration=-duration;}
    return PF_Err_NONE;
}
PF_Err result_error(PF_OutData* out,SfCoreStatus status,const char* detail) {
    if(status==SF_CORE_CANCELLED)return PF_Interrupt_CANCEL;
    return failure(out,detail,status==SF_CORE_ALLOCATION_FAILED?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM);
}
}
PF_Err append_motion_parameters(PF_InData* in,PF_OutData*) noexcept {
    const char* labels[8]{"Motion Blur","Shutter Angle","Shutter Phase","Type","Levels","Linear Accuracy","Opacity Boost","Disregard"};
    const char* options[8]{"Off|Comp Settings|On",nullptr,nullptr,"Linear|Subframe Sample",nullptr,nullptr,nullptr,"Nothing|Camera Motion"};
    PF_ParamDef group{};group.param_type=PF_Param_GROUP_START;group.uu.id=1640;
    group.flags=PF_ParamFlag_START_COLLAPSED;std::snprintf(group.name,sizeof(group.name),"Motion Blur");
    auto error=PF_ADD_PARAM(in,-1,&group);if(error)return error;
    for(std::size_t i=0;i<8;++i) {
        PF_ParamDef def{};def.uu.id=static_cast<A_long>(1641+i);def.flags=PF_ParamFlag_SUPERVISE;
        std::snprintf(def.name,sizeof(def.name),"%s",labels[i]);
        if(motion_popup(i)) {
            def.param_type=PF_Param_POPUP;def.flags|=PF_ParamFlag_CANNOT_TIME_VARY;
            def.u.pd.num_choices=static_cast<A_short>(kMotionMaximum[i]+1);
            def.u.pd.value=def.u.pd.dephault=static_cast<A_short>(kMotionDefaults[i]+1);
            def.u.pd.u.namesptr=options[i];
        } else {
            def.param_type=PF_Param_FLOAT_SLIDER;def.flags|=PF_ParamFlag_START_COLLAPSED;
            def.u.fs_d.valid_min=def.u.fs_d.slider_min=static_cast<PF_FpShort>(kMotionMinimum[i]);
            def.u.fs_d.valid_max=def.u.fs_d.slider_max=static_cast<PF_FpShort>(kMotionMaximum[i]);
            def.u.fs_d.value=kMotionDefaults[i];def.u.fs_d.dephault=static_cast<PF_FpShort>(kMotionDefaults[i]);def.u.fs_d.precision=PF_Precision_INTEGER;
            if(i==6)def.u.fs_d.display_flags=PF_ValueDisplayFlag_PERCENT;
        }
        if(i==1 || i==2 || i==4)def.ui_flags|=PF_PUI_DISABLED;
        error=PF_ADD_PARAM(in,-1,&def);if(error)return error;
    }
    group={};group.param_type=PF_Param_GROUP_END;group.uu.id=1649;return PF_ADD_PARAM(in,-1,&group);
}
PF_Err update_motion_ui(PF_InData* data,PF_ParamDef* params[]) noexcept {
    if(!data || !data->pica_basicP || !params || !params[kMotionParameterIds[0]] || !params[kMotionParameterIds[3]])return PF_Err_BAD_CALLBACK_PARAM;
    const PF_ParamUtilsSuite3* suite{};
    auto error=data->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite));
    if(error)return static_cast<PF_Err>(error);
    const bool off=params[kMotionParameterIds[0]]->u.pd.value==1,on=params[kMotionParameterIds[0]]->u.pd.value==3,
        linear=params[kMotionParameterIds[3]]->u.pd.value==1;
    for(std::size_t i=1;i<8 && !error;++i) {
        const auto id=kMotionParameterIds[i];if(!params[id]) {error=PF_Err_BAD_CALLBACK_PARAM;break;}
        auto def=*params[id];const bool disabled=off || ((i==1 || i==2)&&!on) || (i==4&&linear) || (i==5&&!linear);
        const auto flags=disabled?def.ui_flags|PF_PUI_DISABLED:def.ui_flags&~PF_PUI_DISABLED;
        if(flags!=def.ui_flags) {def.ui_flags=flags;error=suite->PF_UpdateParamUI(data->effect_ref,id,&def);}
    }
    data->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);return static_cast<PF_Err>(error);
}
PF_Err prepare_motion_exposure(PF_InData* data,PF_OutData* out,const core::Graph& graph,
    A_long width,A_long height,const core::Cancellation& cancel,MotionExposure& exposure) noexcept try {
    if(!data || !data->time_scale)return PF_Err_BAD_CALLBACK_PARAM;
    width=std::max<A_long>(1,width>0?width:data->width);
    height=std::max<A_long>(1,height>0?height:data->height);
    exposure={};exposure.values=kMotionDefaults;std::uint32_t limit=1000000;
    for(const auto& node:graph.nodes)if(node.type_key==core::graph_keys::kOutputNode)
        for(const auto& p:node.parameters) {
            if(p.key==core::graph_keys::kParticleCount)limit=std::get<std::uint32_t>(p.value);
            for(std::size_t i=0;i<8;++i)if(p.key==kMotionParameterKeys[i])exposure.values[i]=motion_popup(i)?double(std::get<std::uint32_t>(p.value)):std::get<double>(p.value);
        }
    for(std::size_t i=0;i<8;++i)if(!std::isfinite(exposure.values[i]) || exposure.values[i]<kMotionMinimum[i] || exposure.values[i]>kMotionMaximum[i])return PF_Err_BAD_CALLBACK_PARAM;
    const auto& v=exposure.values;if(v[0]==0 || data->time_step<=0)return PF_Err_NONE;
    const double nominal=double(data->current_time)/data->time_scale,frame=double(data->time_step)/data->time_scale;
    double start=nominal+frame*v[2]/360,duration=frame*v[1]/360;
    exposure.enabled=true;
    if(v[0]==1) {const auto e=comp_window(data,exposure.enabled,start,duration);if(e)return failure(out,"cannot read composition shutter",e);}
    if(!exposure.enabled || duration<=1e-10) {exposure.enabled=false;return PF_Err_NONE;}
    if(!std::isfinite(start) || !std::isfinite(duration))return PF_Err_BAD_CALLBACK_PARAM;
    exposure.gain=1+v[6]/100;
    bool linear=v[3]==0;
    // An endpoint approximation can miss a particle whose entire life is inside
    // the exposure. Use exact samples for known short-life graphs.
    for(const auto& node:graph.nodes)if(node.type_key==core::graph_keys::kParticleNode) {
        double life=3,random=0;
        for(const auto& p:node.parameters) {
            if(p.key==core::graph_keys::kParticleLifetimeSeconds)life=std::get<double>(p.value);
            if(p.key==core::graph_keys::kLifeRandom)random=std::get<double>(p.value);
        }
        if(life*(1-random/100)<=duration)linear=false;
    }
    const unsigned count=v[3]==0?static_cast<unsigned>(2+std::lround(v[5]*14/100)):static_cast<unsigned>(std::lround(v[4]));
    std::vector<core::RationalTime> times;times.reserve(count);
    for(unsigned i=0;i<count;++i) {core::RationalTime time;if(!host_time(start+duration*(i+.5)/count,time))return failure(out,"shutter time exceeds AE range");times.push_back(time);}
    auto capture_times=times;
    if(linear) {
        capture_times.resize(2);
        if(!host_time(start,capture_times[0]) || !host_time(start+duration,capture_times[1]))return PF_Err_BAD_CALLBACK_PARAM;
    }
    std::vector<CapturedParticleFrame> frames;
    auto error=capture_motion_particles(data,out,graph,width,height,capture_times,cancel,frames);if(error)return error;
    SfCoreRenderRequest nominal_camera{};error=capture_camera(data,nominal_camera);if(error)return error;
    std::size_t byte_count=0;
    for(unsigned i=0;i<count;++i) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        auto particles=linear?core::interpolate_motion_particles(frames[0].particles,frames[1].particles,
            frames[0].simulation_seconds,frames[1].simulation_seconds,(i+.5)/count,limit,cancel):core::Result<core::EvaluatedGraph>::success(std::move(frames[i].particles));
        if(!particles.has_value()) {
            if(particles.error().code==core::ErrorCode::cancelled)return PF_Interrupt_CANCEL;
            return failure(out,particles.error().detail,particles.error().code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM);
        }
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        auto record=core::encode_evaluated_particles(particles.value(),times[i],&cancel);if(!record.has_value()) {
            if(record.error().code==core::ErrorCode::cancelled)return PF_Interrupt_CANCEL;
            return failure(out,record.error().detail,record.error().code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM);
        }
        auto frozen=graph;frozen.optional_records.push_back(record.take_value());
        auto bytes=core::serialize_graph(frozen,core::particle_node_registry());if(!bytes.has_value())return failure(out,bytes.error().detail);
        byte_count+=bytes.value().size();if(byte_count>kMotionByteBudget)return failure(out,"exposure exceeds memory budget",PF_Err_OUT_OF_MEMORY);
        MotionSample sample;sample.time=times[i];sample.graph=bytes.take_value();
        PF_InData sampled=*data;sampled.current_time=static_cast<A_long>(times[i].value);sampled.time_scale=static_cast<A_u_long>(times[i].scale);
        error=capture_camera(&sampled,sample.camera,v[7]==1?data:nullptr);if(error)return error;
        // Output coordinates refer to the nominal layer plane, before AE's later
        // layer transform. Reversing each sample's transform would cancel blur.
        std::copy(std::begin(nominal_camera.image_to_layer),std::end(nominal_camera.image_to_layer),sample.camera.image_to_layer);
        exposure.samples.push_back(std::move(sample));
    }
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
void apply_motion_sample(SfCoreRenderRequest& request,const MotionSample& sample) noexcept {
    request.frame.time_value=sample.time.value;request.frame.time_scale=sample.time.scale;
    request.graph_bytes=sample.graph.data();request.graph_byte_count=sample.graph.size();
    request.camera_enabled=sample.camera.camera_enabled;
    std::copy(std::begin(sample.camera.layer_to_view),std::end(sample.camera.layer_to_view),request.layer_to_view);
    std::copy(std::begin(sample.camera.image_to_layer),std::end(sample.camera.image_to_layer),request.image_to_layer);
    request.focal_x=sample.camera.focal_x;request.focal_y=sample.camera.focal_y;
    request.center_x=sample.camera.center_x;request.center_y=sample.camera.center_y;request.near_clip=sample.camera.near_clip;
}
PF_Err render_motion_cpu(PF_InData*,PF_OutData* out,const SfCoreApi& api,SfCoreRenderRequest request,
    const MotionExposure& exposure,const WorldLayout& layout,PF_EffectWorld* world,HostBitDepth depth,const core::Cancellation& cancel) noexcept try {
    const auto region=request.frame.roi;
    const auto width=std::int64_t(region.right)-region.left,height=std::int64_t(region.bottom)-region.top;
    if(width<0 || height<0 || width>32768 || height>32768 || std::uint64_t(width)*height>kMotionByteBudget/32 || exposure.samples.empty())return PF_Err_OUT_OF_MEMORY;
    const std::size_t pixels=std::size_t(width)*height;
    std::vector<float> accumulated(pixels*4,0);
    const auto original_alpha=request.frame.alpha_mode;
    request.frame.pixel_format=2;request.frame.alpha_mode=1;
    for(const auto& sample:exposure.samples) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        apply_motion_sample(request,sample);
        SfCoreRenderResult result{};result.struct_size=sizeof(result);
        struct Release {const SfCoreApi& api;SfCoreRenderResult& result;~Release(){api.release_render_result(&result);}}release{api,result};
        const auto status=api.render(&request,&result);
        if(status!=SF_CORE_OK || result.status!=SF_CORE_OK)return result_error(out,status!=SF_CORE_OK?status:result.status,result.detail);
        if(result.pixel_format!=2 || std::memcmp(&result.region,&region,sizeof(region)) || result.row_bytes<std::uint64_t(width)*16 ||
            (pixels && (!result.pixels || result.pixel_byte_count<std::uint64_t(result.row_bytes)*height)))return failure(out,"invalid sample buffer");
        for(std::int64_t y=0;y<height;++y)for(std::int64_t x=0;x<width;++x) {
            const auto index=std::size_t(y*width+x)*4;
            if((index&16383)==0 && cancel.is_cancelled())return PF_Interrupt_CANCEL;
            float rgba[4];std::memcpy(rgba,static_cast<const std::byte*>(result.pixels)+y*result.row_bytes+x*16,16);
            for(unsigned c=0;c<4;++c) {if(!std::isfinite(rgba[c]))return failure(out,"nonfinite sample pixel");accumulated[index+c]+=rgba[c]/exposure.samples.size();}
        }
    }
    const unsigned channel_bytes=depth==HostBitDepth::bpc8?1:depth==HostBitDepth::bpc16?2:4;
    std::vector<std::byte> packed(pixels*4*channel_bytes);
    for(std::size_t p=0;p<pixels;++p) {
        if((p&4095)==0 && cancel.is_cancelled())return PF_Interrupt_CANCEL;
        auto* rgba=accumulated.data()+p*4;const float alpha=rgba[3];
        const float adjusted=static_cast<float>(std::clamp(alpha*exposure.gain,0.0,1.0));
        for(unsigned c=0;c<3;++c)rgba[c]=alpha>0?rgba[c]*(original_alpha==0?1/alpha:adjusted/alpha):0;
        rgba[3]=adjusted;
        for(unsigned c=0;c<4;++c) {
            auto* target=packed.data()+(p*4+c)*channel_bytes;
            if(channel_bytes==4)std::memcpy(target,&rgba[c],4);
            else if(channel_bytes==2) {const auto value=static_cast<std::uint16_t>(std::lround(std::clamp(rgba[c],0.f,1.f)*32768));std::memcpy(target,&value,2);}
            else *target=std::byte(static_cast<unsigned char>(std::lround(std::clamp(rgba[c],0.f,1.f)*255)));
        }
    }
    const OutputView view{{region.left,region.top,region.right,region.bottom},static_cast<std::uint32_t>(width*4*channel_bytes),pixel_format_for(depth),packed};
    if(!write_output(view,layout,*world,depth,cancel))return cancel.is_cancelled()?PF_Interrupt_CANCEL:PF_Err_INTERNAL_STRUCT_DAMAGED;
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
PF_Err prepare_motion_gpu(PF_InData*,PF_OutData* out,const SfCoreApi& api,SfCoreRenderRequest request,
    const MotionExposure& exposure,SfCoreGpuSceneResult& merged,MotionGpuStorage& storage,const core::Cancellation& cancel) noexcept try {
    merged={};merged.struct_size=sizeof(merged);merged.status=SF_CORE_OK;
    for(const auto& sample:exposure.samples) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        apply_motion_sample(request,sample);SfCoreGpuSceneResult scene{};scene.struct_size=sizeof(scene);
        struct Release {const SfCoreApi& api;SfCoreGpuSceneResult& scene;~Release(){api.release_gpu_scene(&scene);}}release{api,scene};
        const auto status=api.prepare_gpu_scene(&request,&scene);
        if(status!=SF_CORE_OK || scene.status!=SF_CORE_OK)return result_error(out,status!=SF_CORE_OK?status:scene.status,scene.detail);
        if(storage.offsets.empty()) {merged.region=scene.region;merged.tile_size=scene.tile_size;merged.tiles_x=scene.tiles_x;merged.tiles_y=scene.tiles_y;}
        const auto tiles=std::size_t(scene.tiles_x)*scene.tiles_y;
        if(scene.tile_size!=16 || scene.tiles_x!=merged.tiles_x || scene.tiles_y!=merged.tiles_y ||
            std::memcmp(&scene.region,&merged.region,sizeof(scene.region)) || (tiles && !scene.tile_offsets) ||
            (scene.sprite_count && !scene.sprites) || (scene.index_count && !scene.tile_indices) ||
            (scene.cloud_circle_count && !scene.cloud_circles))return failure(out,"invalid sample scene");
        const auto sprite_base=storage.sprites.size(),index_base=storage.indices.size(),cloud_base=storage.cloud_circles.size();
        const auto bytes=(sprite_base+scene.sprite_count)*sizeof(SfGpuSprite)+(index_base+scene.index_count+storage.offsets.size()+tiles+1)*4+
            (cloud_base+scene.cloud_circle_count)*sizeof(SfGpuCloudCircle);
        if(bytes>kMotionByteBudget || sprite_base+scene.sprite_count>8000000 || index_base+scene.index_count>32u*1024u*1024u ||
            cloud_base+scene.cloud_circle_count>core::kMaxCloudMembers)return failure(out,"GPU exposure exceeds memory budget",PF_Err_OUT_OF_MEMORY);
        if(scene.cloud_circle_count)storage.cloud_circles.insert(storage.cloud_circles.end(),scene.cloud_circles,scene.cloud_circles+scene.cloud_circle_count);
        for(std::uint32_t i=0;i<scene.sprite_count;++i) {
            if((i&4095)==0 && cancel.is_cancelled())return PF_Interrupt_CANCEL;
            auto sprite=scene.sprites[i];
            if(sprite.reserved[2]) {
                if(sprite.shape!=2 || sprite.reserved[2]>core::kMaxCloudCircles || sprite.reserved[1]>scene.cloud_circle_count ||
                    sprite.reserved[2]>scene.cloud_circle_count-sprite.reserved[1])return PF_Err_BAD_CALLBACK_PARAM;
                sprite.reserved[1]+=static_cast<std::uint32_t>(cloud_base);
            } else if(sprite.reserved[1])return PF_Err_BAD_CALLBACK_PARAM;
            storage.sprites.push_back(sprite);
        }
        for(std::size_t i=0;i<tiles+1;++i)storage.offsets.push_back(static_cast<std::uint32_t>(index_base+(tiles?scene.tile_offsets[i]:0)));
        for(std::uint32_t i=0;i<scene.index_count;++i) {
            if((i&4095)==0 && cancel.is_cancelled())return PF_Interrupt_CANCEL;
            if(scene.tile_indices[i]>=scene.sprite_count)return PF_Err_BAD_CALLBACK_PARAM;
            storage.indices.push_back(static_cast<std::uint32_t>(sprite_base+scene.tile_indices[i]));
        }
    }
    merged.sprite_count=static_cast<std::uint32_t>(storage.sprites.size());merged.index_count=static_cast<std::uint32_t>(storage.indices.size());
    merged.sprites=storage.sprites.data();merged.tile_offsets=storage.offsets.data();merged.tile_indices=storage.indices.data();
    merged.cloud_circle_count=static_cast<std::uint32_t>(storage.cloud_circles.size());merged.cloud_circles=storage.cloud_circles.data();
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
