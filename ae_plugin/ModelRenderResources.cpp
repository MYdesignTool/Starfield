#include "ModelRenderResources.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <new>
#include <set>

namespace starfield::adapter {
static_assert(sizeof(SfModelPosition)==32&&sizeof(SfModelAttribute)==24&&sizeof(SfModelTriangle)==36);
namespace {
PF_Err failure(PF_OutData* out,const char* detail,PF_Err error=PF_Err_BAD_CALLBACK_PARAM) noexcept {
    if(out&&error!=PF_Interrupt_CANCEL)std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield Model: %s",detail);return error;
}
PF_Err core_failure(PF_OutData* out,core::CoreError error) noexcept {
    return failure(out,error.detail,error.code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:
        error.code==core::ErrorCode::cancelled?PF_Interrupt_CANCEL:PF_Err_BAD_CALLBACK_PARAM);
}
bool same_bounds(const core::ModelBounds& a,const core::ModelBounds& b) noexcept {
    return a.minimum.x==b.minimum.x&&a.minimum.y==b.minimum.y&&a.minimum.z==b.minimum.z&&
        a.maximum.x==b.maximum.x&&a.maximum.y==b.maximum.y&&a.maximum.z==b.maximum.z;
}
}
PF_Err prepare_model_render_resources(PF_InData* data,PF_OutData* output,PF_PreRenderExtra* extra,
    const core::Graph& graph,const MotionExposure& motion,A_long height,double par,const core::Cancellation& cancel,
    ModelRenderResources& prepared) noexcept try {
    prepared={};auto recipes=model_mirror_recipes(graph);if(!recipes.has_value())return core_failure(output,recipes.error());
    if(recipes.value().empty())return PF_Err_NONE;
    if(!data||!data->time_scale)return failure(output,"invalid renderer clock");
    std::set<core::ModelResourceId> requested;
    const auto plan=[&](const core::Graph& scene,core::RationalTime time)->PF_Err {
        auto evaluated=core::evaluate_particle_graph(scene,time,cancel,{double(height),par});if(!evaluated.has_value())return core_failure(output,evaluated.error());
        for(const auto& particle:evaluated.value().particles) {
            if(particle.shape!=4||!particle.model_style_index)continue;
            if(particle.model_style_index>evaluated.value().model_styles.size())return failure(output,"invalid evaluated Model group");
            for(const auto& instance:evaluated.value().model_styles[particle.model_style_index-1].instances)
                if(instance.resource!=core::ModelResourceId{})requested.insert(instance.resource);
        }
        return PF_Err_NONE;
    };
    if(motion.enabled) {
        for(const auto& sample:motion.samples){auto decoded=core::deserialize_graph(sample.graph,core::particle_node_registry());
            if(!decoded.has_value())return failure(output,"invalid shutter Model graph");
            if(const auto error=plan(decoded.value(),sample.time);error)return error;}
    } else if(const auto error=plan(graph,{data->current_time,data->time_scale});error)return error;
    if(requested.empty())return PF_Err_NONE;
    if(!data->inter.checkout_param||!data->inter.checkin_param||!extra||!extra->cb||!extra->cb->GuidMixInPtr)
        return failure(output,"Model checkout/cache callbacks unavailable");
    PF_ParamDef count{};auto error=PF_CHECKOUT_PARAM(data,kModelMirrorCountIndex,data->current_time,data->time_step,data->time_scale,&count);
    if(error)return error;const bool valid_count=count.param_type==PF_Param_SLIDER&&count.u.sd.value==static_cast<A_long>(recipes.value().size());
    const auto checked_count=PF_CHECKIN_PARAM(data,&count);if(!valid_count)return failure(output,"Model mirror count does not match graph");if(checked_count)return checked_count;
    ModelRenderResources candidate;candidate.buffers.reserve(requested.size());candidate.sources.reserve(requested.size());std::uint64_t bytes{};
    for(const auto& id:requested) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        const auto found=std::find_if(recipes.value().begin(),recipes.value().end(),[&](const auto& recipe){return recipe.node.value.bytes==id;});
        if(found==recipes.value().end())return failure(output,"Model resource has no renderer mirror");
        const auto slot=static_cast<A_long>(found-recipes.value().begin());PF_ParamDef parameter{};
        error=PF_CHECKOUT_PARAM(data,kModelMirrorFirstIndex+slot,data->current_time,data->time_step,data->time_scale,&parameter);if(error)return error;
        auto value=parameter.param_type==PF_Param_ARBITRARY_DATA?read_model_mirror_parameter(data,parameter.u.arb_d.value,cancel):
            core::Result<ModelMirrorValue>::failure(core::ErrorCode::invalid_request,"invalid Model mirror parameter type");
        const auto checked=PF_CHECKIN_PARAM(data,&parameter);if(!value.has_value())return core_failure(output,value.error());if(checked)return checked;
        if(value.value().id!=id||value.value().revision!=found->revision||!same_bounds(value.value().geometry.bounds,found->bounds))
            return failure(output,"Model mirror identity/revision/bounds do not match graph");
        const auto size=core::model_geometry_encoded_size(value.value().geometry);if(!size.has_value())return core_failure(output,size.error());
        bytes+=size.value();if(bytes>core::kMaxModelSourceBytes)return failure(output,"Model source byte budget exceeded");
        const auto& mesh=value.value().geometry;candidate.buffers.emplace_back();auto& storage=candidate.buffers.back();
        storage.positions.reserve(mesh.positions.size());storage.texture_coordinates.reserve(mesh.texture_coordinates.size());
        storage.normals.reserve(mesh.normals.size());storage.triangles.reserve(mesh.triangles.size());
        std::size_t work{};const auto stopped=[&](){return ++work%256==0&&cancel.is_cancelled();};
        for(const auto& p:mesh.positions){if(stopped())return PF_Interrupt_CANCEL;storage.positions.push_back({p.value.x,p.value.y,p.value.z,p.weight});}
        for(const auto& p:mesh.texture_coordinates){if(stopped())return PF_Interrupt_CANCEL;storage.texture_coordinates.push_back({p.x,p.y,p.z});}
        for(const auto& p:mesh.normals){if(stopped())return PF_Interrupt_CANCEL;storage.normals.push_back({p.x,p.y,p.z});}
        for(const auto& t:mesh.triangles){if(stopped())return PF_Interrupt_CANCEL;SfModelTriangle triangle{};
            for(unsigned c=0;c<3;++c)triangle.corners[c]={t.corners[c].position,t.corners[c].texture,t.corners[c].normal};storage.triangles.push_back(triangle);}
        SfModelSource source{};source.struct_size=sizeof(source);std::copy(id.begin(),id.end(),source.resource_id);
        source.position_count=static_cast<std::uint32_t>(storage.positions.size());source.positions=storage.positions.data();
        source.texture_coordinate_count=static_cast<std::uint32_t>(storage.texture_coordinates.size());source.texture_coordinates=storage.texture_coordinates.data();
        source.normal_count=static_cast<std::uint32_t>(storage.normals.size());source.normals=storage.normals.data();
        source.triangle_count=static_cast<std::uint32_t>(storage.triangles.size());source.triangles=storage.triangles.data();candidate.sources.push_back(source);
        const auto mix=[&](const void* address,std::size_t size)->PF_Err {return size?extra->cb->GuidMixInPtr(data->effect_ref,static_cast<A_u_long>(size),address):PF_Err_NONE;};
        const std::uint32_t identity[]{value.value().revision,source.position_count,source.texture_coordinate_count,source.normal_count,source.triangle_count};
        if((error=mix(source.resource_id,sizeof(source.resource_id)))||(error=mix(identity,sizeof(identity)))||
            (error=mix(storage.positions.data(),storage.positions.size()*sizeof(SfModelPosition)))||
            (error=mix(storage.texture_coordinates.data(),storage.texture_coordinates.size()*sizeof(SfModelAttribute)))||
            (error=mix(storage.normals.data(),storage.normals.size()*sizeof(SfModelAttribute)))||
            (error=mix(storage.triangles.data(),storage.triangles.size()*sizeof(SfModelTriangle))))return error;
    }
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;prepared=std::move(candidate);return PF_Err_NONE;
} catch(const std::bad_alloc&){return failure(output,"Model resource allocation failed",PF_Err_OUT_OF_MEMORY);}
catch(...){return failure(output,"Model resource preparation failed",PF_Err_INTERNAL_STRUCT_DAMAGED);}
}
