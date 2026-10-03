#include "EmitterHistory.hpp"
#include "NativeNodeGraph.hpp"
#include "NativeTemporalCache.hpp"
#include "Parameters.hpp"
#include "AE_EffectCB.h"
#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <limits>
#include <map>
#include <mutex>
#include <new>

namespace starfield::adapter {
namespace {
std::mutex history_trace_mutex;
NativeHistoryTrace history_trace;
struct TraceScope {
    NativeHistoryTrace trace;
    std::chrono::steady_clock::time_point start{std::chrono::steady_clock::now()};
    explicit TraceScope(const PF_InData* data) {
        trace.path=NativeHistoryPath::failed;
        if(data && data->time_scale)trace.seconds=double(data->current_time)/data->time_scale;
    }
    ~TraceScope() {
        trace.milliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
        std::lock_guard lock(history_trace_mutex);history_trace=trace;
    }
};
class TemporalCapture final : public core::TemporalGraphSampler {
    PF_InData* data;
    const core::Cancellation& cancellation;
    NativeAnimationPlan plan;
    NativeHistoryTrace& trace;
    std::map<std::pair<core::NodeId,double>,core::GraphNode> nodes;
    std::map<std::pair<core::NodeId,double>,double> rates;
    bool context(double seconds,PF_InData& sampled) {
        if(!std::isfinite(seconds) || seconds<0 || seconds>std::numeric_limits<A_long>::max()-1.0) {
            error=PF_Err_BAD_CALLBACK_PARAM;return false;
        }
        sampled=*data;birth=seconds;
        const auto scale=static_cast<A_u_long>(std::min(1'000'000.0,
            std::floor((std::numeric_limits<A_long>::max()-1.0)/std::max(1.0,seconds))));
        sampled.current_time=static_cast<A_long>(std::llround(seconds*scale));
        const double duration=data->time_scale?double(data->time_step)/data->time_scale:0;
        sampled.time_step=static_cast<A_long>(std::clamp(std::round(duration*scale),1.0,double(std::numeric_limits<A_long>::max())));
        sampled.time_scale=scale;return true;
    }
public:
    std::vector<NativeOriginBinding> bindings;
    std::vector<NativeLifetimeBinding> lifetimes;
    PF_Err error{};A_long stream{-1};double birth{};
    TemporalCapture(PF_InData* d,const core::Graph& g,const core::Cancellation& c,A_long w,A_long h,NativeHistoryTrace& t)
        :data(d),cancellation(c),
         plan(g,w,h,d->pixel_aspect_ratio.den?double(d->pixel_aspect_ratio.num)/d->pixel_aspect_ratio.den:1),trace(t) {}
    ~TemporalCapture() {trace.checkouts+=plan.checkout_count();}
    void prepare(bool allow_static_bypass) {
        plan.prepare_constants(data,allow_static_bypass);trace.certified=plan.proofs().size();
        trace.inputs=plan.input_count();trace.constants=plan.constant_count();
    }
    bool fully_constant() const {return plan.fully_constant();}
    core::Result<std::optional<core::EmissionRateProfile>> rate_profile(core::NodeId id) override {
        for(const auto& p:plan.proofs()) if(p.node==id && p.rate)
            return core::Result<std::optional<core::EmissionRateProfile>>::success(p.rate);
        return core::Result<std::optional<core::EmissionRateProfile>>::success(std::nullopt);
    }
    std::optional<double> lifetime_upper_bound(core::NodeId id) override {
        for(const auto& p:plan.proofs()) if(p.node==id && p.life_bound)return p.life_bound;
        return {};
    }
    std::shared_ptr<core::EmissionTimeline> emission_timeline(core::NodeId id,unsigned hz) override {
        for(const auto& b:bindings)if(b.emitter==id)return native_emission_timeline(data,id,b.rate,hz);
        return {};
    }
    core::Result<core::GraphNode> node(core::NodeId id,double seconds) override {
        ++trace.node_queries;
        using R=core::Result<core::GraphNode>;
        if(cancellation.is_cancelled()) return R::failure(core::ErrorCode::cancelled,"temporal node sampling cancelled");
        if(const auto* constant=plan.constant_node(id)) {
            ++trace.constant_node_hits;return R::success(*constant);
        }
        const auto key=std::make_pair(id,seconds);
        if(auto found=nodes.find(key);found!=nodes.end()) return R::success(found->second);
        PF_InData sampled{};if(!context(seconds,sampled)) return R::failure(core::ErrorCode::invalid_time,"historical time exceeds AE range");
        core::GraphNode value;
        ++trace.node_samples;
        error=plan.sample(&sampled,id,value,&stream);
        if(error) return R::failure(core::ErrorCode::invalid_request,"historical node parameter checkout/conversion failed");
        if(nodes.size()>=4096) nodes.clear();
        nodes.emplace(key,value);return R::success(std::move(value));
    }
    core::Result<double> rate(core::NodeId id,double seconds) override {
        ++trace.rate_queries;
        using R=core::Result<double>;
        if(cancellation.is_cancelled()) return R::failure(core::ErrorCode::cancelled,"rate sampling cancelled");
        const auto key=std::make_pair(id,seconds);
        if(auto found=rates.find(key);found!=rates.end()) return R::success(found->second);
        const auto found=std::find_if(bindings.begin(),bindings.end(),[&](const auto& b){return b.emitter==id;});
        if(found==bindings.end()) return R::failure(core::ErrorCode::invalid_request,"emission rate binding missing");
        PF_InData sampled{};if(!context(seconds,sampled)) return R::failure(core::ErrorCode::invalid_time,"historical rate time exceeds AE range");
        stream=found->rate;PF_ParamDef value{};
        ++trace.checkouts;
        error=PF_CHECKOUT_PARAM(&sampled,stream,sampled.current_time,sampled.time_step,sampled.time_scale,&value);
        if(error) return R::failure(core::ErrorCode::invalid_request,"historical rate checkout failed");
        const bool valid=value.param_type==PF_Param_FLOAT_SLIDER && std::isfinite(value.u.fs_d.value) &&
            value.u.fs_d.value!=kNativeBindingUnavailable;
        const double number=valid?value.u.fs_d.value:0;
        error=PF_CHECKIN_PARAM(&sampled,&value);
        if(error || !valid) {if(!error) error=PF_Err_BAD_CALLBACK_PARAM;return R::failure(core::ErrorCode::invalid_request,"historical rate value/checkin failed");}
        if(rates.size()>=64) rates.clear();
        rates.emplace(key,number);return R::success(number);
    }
    core::Result<double> lifetime(core::NodeId id,double seconds) override {
        ++trace.life_queries;
        using R=core::Result<double>;
        if(cancellation.is_cancelled()) return R::failure(core::ErrorCode::cancelled,"Life sampling cancelled");
        for(const auto& p:plan.proofs())if(p.node==id && p.constant && p.life_bound)
            return R::success(*p.life_bound);
        const auto found=std::find_if(lifetimes.begin(),lifetimes.end(),[&](const auto& b){return b.particle==id;});
        if(found==lifetimes.end()) return R::failure(core::ErrorCode::invalid_request,"Life binding missing");
        PF_InData sampled{};if(!context(seconds,sampled)) return R::failure(core::ErrorCode::invalid_time,"historical Life time exceeds AE range");
        stream=found->stream;PF_ParamDef value{};
        ++trace.checkouts;
        error=PF_CHECKOUT_PARAM(&sampled,stream,sampled.current_time,sampled.time_step,sampled.time_scale,&value);
        if(error) return R::failure(core::ErrorCode::invalid_request,"historical Life checkout failed");
        const bool valid=value.param_type==PF_Param_FLOAT_SLIDER && std::isfinite(value.u.fs_d.value) && value.u.fs_d.value!=kNativeBindingUnavailable;
        const double number=valid?value.u.fs_d.value:0;error=PF_CHECKIN_PARAM(&sampled,&value);
        if(error || !valid) {if(!error) error=PF_Err_BAD_CALLBACK_PARAM;return R::failure(core::ErrorCode::invalid_request,"historical Life value/checkin failed");}
        return R::success(number);
    }
};
}
NativeHistoryTrace last_native_history_trace() noexcept {
    std::lock_guard lock(history_trace_mutex);return history_trace;
}
PF_Err capture_emitter_origin_history(PF_InData* data,PF_OutData* output,core::Graph& graph,
    A_long width,A_long height,const core::Cancellation& cancellation) noexcept {
    TraceScope trace(data);
    try {
        if(!data || !data->inter.checkout_param || !data->inter.checkin_param) return PF_Err_BAD_CALLBACK_PARAM;
        TemporalCapture capture(data,graph,cancellation,width>0?width:data->width,height>0?height:data->height,trace.trace);
        auto error=read_native_origin_bindings(graph,capture.bindings);
        if(error || capture.bindings.empty()) {if(!error)trace.trace.path=NativeHistoryPath::unavailable;return error;}
        error=read_native_lifetime_bindings(graph,capture.lifetimes);if(error) return error;
        // The ordinary evaluator selects only the alive slot interval and uses
        // closed-form motion. Never select it from merely equal sample values.
        bool simple=true;
        for(const auto& node:graph.nodes) {
            if(node.type_key==core::graph_keys::kEmitterNode) {
                for(const auto& p:node.parameters)
                    if((p.key==core::graph_keys::kEmittingMode || p.key==core::graph_keys::kAuxiliarySource) && std::get<std::uint32_t>(p.value)!=0)simple=false;
            } else if(node.type_key==core::graph_keys::kParticleNode) {
                for(const auto& p:node.parameters)if(p.key==core::graph_keys::kLifeRandom && std::get<double>(p.value)!=0)simple=false;
            } else if(node.type_key!=core::graph_keys::kOutputNode)simple=false;
        }
        capture.prepare(simple);
        if(simple && capture.fully_constant()) {trace.trace.path=NativeHistoryPath::static_graph;return PF_Err_NONE;}
        // Core CPU/GPU path evaluates a certified static graph only once.
        const core::RationalTime time{data->current_time,data->time_scale};
        const auto evaluated=core::evaluate_temporal_particle_graph(graph,time,cancellation,
            {double(std::max<A_long>(height>0?height:data->height,1)),
             data->pixel_aspect_ratio.den?double(data->pixel_aspect_ratio.num)/data->pixel_aspect_ratio.den:1},capture);
        if(!evaluated.has_value()) {
            if(evaluated.error().code==core::ErrorCode::cancelled) return PF_Interrupt_CANCEL;
            if(output) std::snprintf(output->return_msg,sizeof(output->return_msg),
                "Starfield temporal controls: %s (stream %ld, time %.6f, error %ld).",
                evaluated.error().detail,static_cast<long>(capture.stream),capture.birth,static_cast<long>(capture.error));
            return capture.error?capture.error:evaluated.error().code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM;
        }
        auto encoded=core::encode_evaluated_particles(evaluated.value(),time);
        if(!encoded.has_value()) return PF_Err_BAD_CALLBACK_PARAM;
        trace.trace.particles=evaluated.value().particles.size();
        trace.trace.path=NativeHistoryPath::temporal;
        graph.optional_records.push_back(encoded.take_value());return PF_Err_NONE;
    } catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
      catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
} // namespace starfield::adapter
