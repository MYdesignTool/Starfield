#include "TextureResources.hpp"
#include "Parameters.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/SequenceCodec.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <new>
#include <set>

namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic{};
    const AEGP_PFInterfaceSuite1* pf{};
    const AEGP_LayerSuite9* layer{};
    const AEGP_ItemSuite9* item{};
    const AEGP_CompSuite11* comp{};
    ~Suites() {
        if(comp)basic->ReleaseSuite(kAEGPCompSuite,kAEGPCompSuiteVersion11);
        if(item)basic->ReleaseSuite(kAEGPItemSuite,kAEGPItemSuiteVersion9);
        if(layer)basic->ReleaseSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
        if(pf)basic->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    }
    A_Err acquire() {
        auto e=basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&pf));
        if(!e)e=basic->AcquireSuite(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,reinterpret_cast<const void**>(&layer));
        if(!e)e=basic->AcquireSuite(kAEGPItemSuite,kAEGPItemSuiteVersion9,reinterpret_cast<const void**>(&item));
        if(!e)e=basic->AcquireSuite(kAEGPCompSuite,kAEGPCompSuiteVersion11,reinterpret_cast<const void**>(&comp));
        return e;
    }
};
PF_Err failure(PF_OutData* out,const char* text,PF_Err error=PF_Err_BAD_CALLBACK_PARAM) {
    if(out && error!=PF_Interrupt_CANCEL)std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield texture: %.220s",text);
    return error;
}
PF_Err core_failure(PF_OutData* out,const core::CoreError& error) {
    return failure(out,error.detail,error.code==core::ErrorCode::cancelled?PF_Interrupt_CANCEL:
        error.code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM);
}
double seconds(A_Time time) {return time.scale?double(time.value)/time.scale:std::numeric_limits<double>::quiet_NaN();}
bool host_time(double value,A_u_long scale,A_long& result) {
    const double ticks=value*scale;
    if(!std::isfinite(ticks) || !scale || ticks<std::numeric_limits<A_long>::min() || ticks>std::numeric_limits<A_long>::max())return false;
    result=static_cast<A_long>(std::llround(ticks));return true;
}
}

PF_Err prepare_texture_resources(PF_InData* in,PF_OutData* out,PF_PreRenderExtra* extra,
    const core::Graph& graph,const MotionExposure& motion,A_long height,double par,
    const core::Cancellation& cancel,TexturePreparation& prepared) noexcept try {
    std::set<std::uint32_t> ids;
    for(const auto& node:graph.nodes)if(node.type_key==core::graph_keys::kParticleNode)
        for(const auto& p:node.parameters)if(p.key==core::graph_keys::kTextureFront || p.key==core::graph_keys::kTextureBack) {
            const auto* id=std::get_if<std::uint32_t>(&p.value);
            if(!id)return failure(out,"invalid graph layer resource");
            if(*id)ids.insert(*id);
        }
    if(ids.empty())return PF_Err_NONE;
    if(ids.size()>core::kMaxTextureSources || !in->pica_basicP || !in->time_scale)return failure(out,"resource count/time is invalid");
    Suites suites{in->pica_basicP};if(auto e=suites.acquire();e)return failure(out,"resource suites unavailable",static_cast<PF_Err>(e));
    AEGP_LayerH owner{};AEGP_CompH comp{};A_FpLong fps{};A_Ratio stretch{};
    auto e=suites.pf->AEGP_GetEffectLayer(in->effect_ref,&owner);
    if(!e)e=suites.layer->AEGP_GetLayerParentComp(owner,&comp);
    if(!e)e=suites.comp->AEGP_GetCompFramerate(comp,&fps);
    if(!e)e=suites.layer->AEGP_GetLayerStretch(owner,&stretch);
    if(e || !owner || !comp || !(fps>0) || !stretch.num || !stretch.den)return failure(out,"renderer clock unavailable",static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM));
    const double step=std::abs(double(stretch.den)/stretch.num)/fps;
    for(const auto id:ids) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        AEGP_LayerH source{};AEGP_ItemH item{};A_Time start{},duration{},local_start{},local_end{};
        A_long width{},source_height{};A_Ratio aspect{};
        e=suites.layer->AEGP_GetLayerFromLayerID(comp,static_cast<AEGP_LayerIDVal>(id),&source);
        if(e || !source || source==owner)return failure(out,"source layer is deleted or references the renderer itself");
        if(!e)e=suites.layer->AEGP_GetLayerSourceItem(source,&item);
        if(!e)e=suites.item->AEGP_GetItemDimensions(item,&width,&source_height);
        if(!e)e=suites.item->AEGP_GetItemPixelAspectRatio(item,&aspect);
        if(!e)e=suites.layer->AEGP_GetLayerInPoint(source,AEGP_LTimeMode_CompTime,&start);
        if(!e)e=suites.layer->AEGP_GetLayerDuration(source,AEGP_LTimeMode_CompTime,&duration);
        const double end_seconds=seconds(start)+seconds(duration);A_long end_ticks{};
        if(e || !host_time(end_seconds,start.scale,end_ticks))return failure(out,"source clip metadata is invalid");
        const A_Time end{end_ticks,start.scale};
        if(!e)e=suites.layer->AEGP_ConvertCompToLayerTime(owner,&start,&local_start);
        if(!e)e=suites.layer->AEGP_ConvertCompToLayerTime(owner,&end,&local_end);
        if(e || width<1 || source_height<1 || !aspect.den || aspect.num<=0)return failure(out,"source geometry/time conversion failed",static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM));
        core::TextureSource metadata{id,std::min(seconds(local_start),seconds(local_end)),
            std::max(seconds(local_start),seconds(local_end)),step,static_cast<std::uint32_t>(width),
            static_cast<std::uint32_t>(source_height),double(aspect.num)/aspect.den};
        auto count=core::texture_frame_count(metadata);if(!count.has_value())return core_failure(out,count.error());
        if(seconds(local_start)>seconds(local_end)) {
            // Reversing [comp-in,comp-out) makes the lower layer-clock bound
            // exclusive. Anchor the grid at comp-in, excluding comp-out, rather
            // than sampling the invisible out-point or losing the first frame.
            metadata.start_seconds=seconds(local_start)-(count.value()-1)*step;
            metadata.end_seconds=seconds(local_start)+step;
        }
        prepared.sources.push_back(metadata);
        prepared.abi_sources.push_back({sizeof(SfTextureSource),id,metadata.start_seconds,metadata.end_seconds,
            step,metadata.width,metadata.height,metadata.pixel_aspect_ratio});
    }
    std::map<std::pair<std::uint32_t,std::uint32_t>,core::TextureFrameRequest> frames;
    auto plan=[&](const core::Graph& scene,core::RationalTime time)->PF_Err {
        auto evaluated=core::evaluate_particle_graph(scene,time,cancel,{double(height),par});
        if(!evaluated.has_value())return core_failure(out,evaluated.error());
        auto requests=core::plan_texture_frames(evaluated.value(),prepared.sources,double(time.value)/time.scale,cancel);
        if(!requests.has_value())return core_failure(out,requests.error());
        for(const auto& frame:requests.value())frames.emplace(std::pair{frame.resource_id,frame.frame_index},frame);
        if(frames.size()>core::kMaxTextureFrames)return failure(out,"shutter texture request budget exceeded");
        return PF_Err_NONE;
    };
    if(motion.enabled) {
        for(const auto& sample:motion.samples) {
            auto decoded=core::deserialize_graph(sample.graph,core::particle_node_registry());
            if(!decoded.has_value())return failure(out,"shutter graph is invalid");
            if(auto result=plan(decoded.value(),sample.time);result)return result;
        }
    } else if(auto result=plan(graph,{in->current_time,in->time_scale});result)return result;
    if(!extra->cb->GuidMixInPtr)return failure(out,"SmartFX cache callback unavailable");
    for(const auto& metadata:prepared.sources) {
        // Use explicit fields: padding must never enter the cache identity.
        const double key[]{double(metadata.resource_id),metadata.start_seconds,metadata.end_seconds,
            metadata.frame_seconds,double(metadata.width),double(metadata.height),metadata.pixel_aspect_ratio};
        if(auto result=extra->cb->GuidMixInPtr(in->effect_ref,sizeof(key),key);result)return result;
    }
    for(const auto& [key,frame]:frames) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        const auto found=std::lower_bound(prepared.sources.begin(),prepared.sources.end(),frame.resource_id,
            [](const auto& source,std::uint32_t id){return source.resource_id<id;});
        const auto slot=static_cast<A_long>(found-prepared.sources.begin());
        A_long time{};if(!host_time(frame.seconds,in->time_scale,time))return failure(out,"requested source time exceeds AE range");
        TextureCheckout checkout{frame,static_cast<A_long>(prepared.checkouts.size()+2),slot,{}};
        PF_RenderRequest request=extra->input->output_request;
        request.rect={0,0,static_cast<A_long>(found->width),static_cast<A_long>(found->height)};
        request.preserve_rgb_of_zero_alpha=TRUE;
        if(auto result=extra->cb->checkout_layer(in->effect_ref,kTextureResourceFirstIndex+slot,checkout.id,
            &request,time,in->time_step,in->time_scale,&checkout.result);result)return result;
        prepared.checkouts.push_back(checkout);
    }
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;} catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err stage_texture_resources(PF_InData* in,PF_OutData* out,PF_SmartRenderExtra* extra,
    const TexturePreparation& prepared,HostBitDepth depth,const core::Cancellation& cancel,TextureStaging& staging) noexcept try {
    if(prepared.checkouts.empty())return PF_Err_NONE;
    if(!extra->cb->checkin_layer_pixels)return failure(out,"SmartFX pixel checkin callback unavailable");
    staging.pixels.reserve(prepared.checkouts.size());staging.frames.reserve(prepared.checkouts.size());
    std::uint64_t bytes{};
    for(const auto& checkout:prepared.checkouts) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        PF_EffectWorld* world{};
        if(auto e=extra->cb->checkout_layer_pixels(in->effect_ref,checkout.id,&world);e)return e;
        struct Checkin {PF_InData* in;PF_SmartRenderExtra* extra;A_long id;
            ~Checkin(){extra->cb->checkin_layer_pixels(in->effect_ref,id);}} checkin{in,extra,checkout.id};
        const auto& source=prepared.sources[checkout.slot];WorldLayout layout{};
        double rx=1,ry=1;std::uint32_t width=1,height=1;
        const bool pixels=world && world->data && world->width>0 && world->height>0;
        if(pixels) {
            if(!describe_world(*world,layout))return failure(out,"invalid source world");
            const auto rect=checkout.result.result_rect;
            if(rect.right<=rect.left || rect.bottom<=rect.top)return failure(out,"source checkout geometry mismatch");
            rx=double(layout.width)/(rect.right-rect.left);ry=double(layout.height)/(rect.bottom-rect.top);
            const double w=std::ceil(source.width*rx),h=std::ceil(source.height*ry);
            if(!(w>=1 && w<=32768 && h>=1 && h<=32768))return failure(out,"source staging dimensions exceed limit");
            width=static_cast<std::uint32_t>(w);height=static_cast<std::uint32_t>(h);
        }
        const std::uint64_t values=std::uint64_t(width)*height*4;
        if(values>(core::kMaxTextureBytes-bytes)/sizeof(float))return failure(out,"texture staging byte budget exceeded");
        bytes+=values*sizeof(float);
        staging.pixels.emplace_back(static_cast<std::size_t>(values),0.f);auto& buffer=staging.pixels.back();
        if(pixels) {
            const auto pixel_bytes=depth==HostBitDepth::bpc8?sizeof(PF_Pixel):depth==HostBitDepth::bpc16?sizeof(PF_Pixel16):sizeof(PF_PixelFloat);
            if(layout.row_bytes<std::uint64_t(layout.width)*pixel_bytes)return failure(out,"source row stride is invalid");
            const auto ox=std::llround(world->origin_x*rx),oy=std::llround(world->origin_y*ry);
            for(std::uint32_t y=0;y<layout.height;++y) {
                if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
                const auto dy=oy+y;if(dy<0 || dy>=height)continue;
                const auto* row=reinterpret_cast<const std::byte*>(world->data)+std::size_t(y)*layout.row_bytes;
                for(std::uint32_t x=0;x<layout.width;++x) {
                    const auto dx=ox+x;if(dx<0 || dx>=width)continue;
                    auto* p=buffer.data()+(std::size_t(dy)*width+static_cast<std::size_t>(dx))*4;
                    const auto* from=row+std::size_t(x)*pixel_bytes;
                    if(depth==HostBitDepth::bpc8){PF_Pixel v{};std::memcpy(&v,from,sizeof(v));p[0]=v.red/255.f;p[1]=v.green/255.f;p[2]=v.blue/255.f;p[3]=v.alpha/255.f;}
                    else if(depth==HostBitDepth::bpc16){PF_Pixel16 v{};std::memcpy(&v,from,sizeof(v));p[0]=v.red/32768.f;p[1]=v.green/32768.f;p[2]=v.blue/32768.f;p[3]=v.alpha/32768.f;}
                    else {PF_PixelFloat v{};std::memcpy(&v,from,sizeof(v));p[0]=v.red;p[1]=v.green;p[2]=v.blue;p[3]=v.alpha;}
                }
            }
        }
        staging.frames.push_back({sizeof(SfTextureFrame),checkout.frame.resource_id,checkout.frame.frame_index,
            width,height,width*4,buffer.data(),values});
    }
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;} catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
