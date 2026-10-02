#include "EmitterHistory.hpp"
#include "NativeNodeGraph.hpp"
#include "Parameters.hpp"
#include "AE_EffectCB.h"
#include "starfield/core/EmitterHistory.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/Geometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <new>

namespace starfield::adapter {
namespace {
class OriginCapture final : public core::EmitterOriginSampler {
public:
    PF_InData* data;
    const core::Cancellation& cancellation;
    core::LayerUnits units;
    std::vector<NativeOriginBinding> bindings;
    std::map<std::pair<core::NodeId,double>,core::Vec3> values;
    PF_Err error{};
    A_long stream{-1}; double birth{};
    OriginCapture(PF_InData* input,const core::Cancellation& cancel,A_long width,A_long height)
        :data(input),cancellation(cancel),units{double(std::max<A_long>(width,1)),double(std::max<A_long>(height,1)),
            input->pixel_aspect_ratio.den ? double(input->pixel_aspect_ratio.num)/input->pixel_aspect_ratio.den : 1.0} {}
    core::Result<core::Vec3> sample(core::NodeId emitter,double seconds) override {
        using R=core::Result<core::Vec3>;
        const auto key=std::make_pair(emitter,seconds);
        if(const auto found=values.find(key);found!=values.end()) return R::success(found->second);
        if(cancellation.is_cancelled()) return R::failure(core::ErrorCode::cancelled,"emitter birth sampling cancelled");
        if(values.size()>=core::kMaxEmitterOriginSamples) return R::failure(core::ErrorCode::work_limit_exceeded,"emitter origin history sample limit exceeded");
        const auto found=std::find_if(bindings.begin(),bindings.end(),[&](const auto& b){return b.emitter==emitter;});
        if(found==bindings.end()) return R::failure(core::ErrorCode::invalid_request,"emitter origin binding is missing");
        if(!std::isfinite(seconds) || seconds<0 || seconds>std::numeric_limits<A_long>::max()-1.0)
            return R::failure(core::ErrorCode::invalid_time,"emitter birth time exceeds host range");
        birth=seconds;
        const auto scale=static_cast<A_u_long>(std::min(1'000'000.0,std::floor((std::numeric_limits<A_long>::max()-1.0)/std::max(1.0,seconds))));
        const auto time=static_cast<A_long>(std::llround(seconds*scale));
        const double duration=data->time_scale ? double(data->time_step)/data->time_scale : 0;
        const auto step=static_cast<A_long>(std::clamp(std::round(duration*scale),1.0,double(std::numeric_limits<A_long>::max())));
        core::Vec3 pixels{};
        double* components[]={&pixels.x,&pixels.y,&pixels.z};
        const A_long indices[]={found->x,found->y,found->z};
        for(int i=0;i<3;++i) {
            stream=indices[i];PF_ParamDef value{};
            error=PF_CHECKOUT_PARAM(data,stream,time,step,scale,&value);
            if(error) return R::failure(core::ErrorCode::invalid_request,"emitter birth parameter checkout failed");
            const bool valid=value.param_type==PF_Param_FLOAT_SLIDER && std::isfinite(value.u.fs_d.value) && value.u.fs_d.value!=kNativeBindingUnavailable;
            *components[i]=valid ? value.u.fs_d.value : 0;
            error=PF_CHECKIN_PARAM(data,&value);
            if(error || !valid) {if(!error) error=PF_Err_BAD_CALLBACK_PARAM;return R::failure(core::ErrorCode::invalid_request,"emitter birth parameter value/checkin failed");}
        }
        const auto origin=core::layer_point_to_world(pixels.x,pixels.y,pixels.z+units.layer_height/2.0,units);
        values.emplace(key,origin);return R::success(origin);
    }
};
}
PF_Err capture_emitter_origin_history(PF_InData* data,PF_OutData* output,core::Graph& graph,
    A_long width,A_long height,const core::Cancellation& cancellation) noexcept {
    try {
        if(!data || !data->inter.checkout_param || !data->inter.checkin_param) return PF_Err_BAD_CALLBACK_PARAM;
        OriginCapture capture(data,cancellation,width>0?width:data->width,height>0?height:data->height);
        auto error=read_native_origin_bindings(graph,capture.bindings);
        if(error || capture.bindings.empty()) return error;
        const auto evaluated=core::evaluate_particle_graph(graph,{data->current_time,data->time_scale},cancellation,
            {capture.units.layer_height,capture.units.pixel_aspect_ratio},&capture);
        if(!evaluated.has_value()) {
            if(evaluated.error().code==core::ErrorCode::cancelled) return PF_Interrupt_CANCEL;
            if(output) std::snprintf(output->return_msg,sizeof(output->return_msg),"Starfield birth position: %s (stream %ld, time %.6f, error %ld).",
                evaluated.error().detail,static_cast<long>(capture.stream),capture.birth,static_cast<long>(capture.error));
            return capture.error ? capture.error : evaluated.error().code==core::ErrorCode::allocation_failed ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
        }
        std::vector<core::EmitterOriginSample> samples;samples.reserve(capture.values.size());
        for(const auto& [key,origin]:capture.values) samples.push_back({key.first,key.second,origin});
        auto encoded=core::encode_emitter_origin_history(std::move(samples));
        if(!encoded.has_value()) return PF_Err_BAD_CALLBACK_PARAM;
        graph.optional_records.push_back(encoded.take_value());return PF_Err_NONE;
    } catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
    catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
} // namespace starfield::adapter
