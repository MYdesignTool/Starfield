#include "MotionLookAtCapture.hpp"
#include "MotionLightSources.hpp"
#include "MotionPointCapture.hpp"
#include "starfield/core/MotionLookAtTargets.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <memory>
#include <new>

namespace starfield::adapter {
PF_Err capture_motion_look_at_sample_resources(PF_InData* data,AEGP_PluginID plugin,
    core::LayerUnits units,const core::Cancellation& cancel,const core::Graph& graph,
    core::RationalTime time,MotionLookAtTargetResources& output,core::OpaqueBytes& cache_key) noexcept try {
    const auto normalized=core::make_rational(time.value,time.scale);
    if(!data||plugin<=0||!normalized||normalized->value<std::numeric_limits<A_long>::min()||
       normalized->value>std::numeric_limits<A_long>::max()||
       normalized->scale>std::numeric_limits<A_u_long>::max())return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    PF_InData sampled=*data;
    sampled.current_time=static_cast<A_long>(normalized->value);
    sampled.time_scale=static_cast<A_u_long>(normalized->scale);

    std::map<std::u16string,std::shared_ptr<const core::OpaqueBytes>> by_prefix;
    std::vector<MotionLightSource> sources;
    bool captured_sources=false;
    std::vector<std::byte> key;
    MotionLookAtTargetResources resources;
    for(const auto& node:graph.nodes) {
        if(node.type_key!=core::graph_keys::kMotionNode)continue;
        const core::ParameterValue* mode_value=nullptr;
        const std::string* prefix=nullptr;
        for(const auto& parameter:node.parameters) {
            if(parameter.key==core::graph_keys::kMotionMode)mode_value=&parameter.value;
            else if(parameter.key==core::graph_keys::kMotionPathStartingWith)
                prefix=std::get_if<std::string>(&parameter.value);
        }
        const auto* mode=mode_value?std::get_if<std::uint32_t>(mode_value):nullptr;
        if(!mode)return PF_Err_BAD_CALLBACK_PARAM;
        if(*mode!=2)continue;
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        std::u16string wide_prefix;
        if(prefix) {
            const auto converted=motion_light_prefix_to_utf16(*prefix,wide_prefix);
            if(converted)return converted;
        }
        auto found=by_prefix.find(wide_prefix);
        if(found==by_prefix.end()) {
            if(!captured_sources) {
                const auto error=capture_motion_light_sources(&sampled,plugin,cancel,sources);
                if(error)return error;
                captured_sources=true;
            }
            std::vector<std::uint32_t> ids;
            auto error=select_motion_light_source_ids(sources,wide_prefix,ids);
            if(error)return error;
            std::vector<MotionLayerPointRequest> requests;requests.reserve(ids.size());
            for(const auto id:ids)requests.push_back({id,{}});
            std::vector<MotionLayerPoint> points;
            error=capture_motion_layer_points(&sampled,units,requests,cancel,points);
            if(error)return error;
            if(points.size()!=ids.size())return PF_Err_BAD_CALLBACK_PARAM;
            std::vector<core::MotionLookAtTarget> targets;targets.reserve(points.size());
            for(std::size_t index=0;index<points.size();++index)
                targets.push_back({ids[index],points[index].position});
            auto encoded=core::encode_motion_look_at_targets(targets,cancel);
            if(!encoded.has_value())return encoded.error().code==core::ErrorCode::cancelled?
                PF_Interrupt_CANCEL:encoded.error().code==core::ErrorCode::allocation_failed?
                PF_Err_OUT_OF_MEMORY:PF_Err_BAD_CALLBACK_PARAM;
            auto packet=std::make_shared<const core::OpaqueBytes>(encoded.take_value());
            found=by_prefix.emplace(std::move(wide_prefix),std::move(packet)).first;
        }
        if(!found->second)return PF_Err_INTERNAL_STRUCT_DAMAGED;
        resources.push_back({node.id,found->second});
        const auto& bytes=*found->second;
        const auto size=static_cast<std::uint64_t>(bytes.size());
        for(unsigned i=0;i<8;++i)key.push_back(static_cast<std::byte>((size>>(i*8))&0xffu));
        key.insert(key.end(),bytes.begin(),bytes.end());
    }
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    std::sort(resources.begin(),resources.end(),[](const auto& a,const auto& b){return a.node<b.node;});
    output.swap(resources);
    cache_key.swap(key);
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
