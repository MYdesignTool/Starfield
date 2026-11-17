#pragma once
#include "starfield/core/Graph.hpp"
#include "starfield/core/Error.hpp"
#include "PluginVersion.h"
#include <cstdio>
#include <cstring>
#include <string_view>

namespace starfield::adapter::editor_diagnostics {
// These identify a plug-in condition. The returned PF_Err remains the legal SDK
// status; an AE516 is supplementary context, not our diagnostic identity.
struct Description {const char* code;const char* summary;const char* action;};
inline Description binding(const char* stage) noexcept {
    const std::string_view name=stage?stage:"";
    if(name=="binding unavailable")return {"SF-B101","binding unavailable","Check source/expression. Removed source: Diagnostics > Repair Selected Graph."};
    if(name=="nonfinite parameter value")return {"SF-B102","nonfinite input","Check this node's expression and numeric values."};
    if(name=="parameter type")return {"SF-B103","unexpected input type","Check the installed plug-in pairing and source control."};
    if(name=="parameter checkout")return {"SF-B104","AE input checkout failed","Inspect the source control and the AE status in details."};
    if(name=="parameter checkin")return {"SF-B105","AE input release failed","Stop editing and inspect the AE status in details."};
    return {"SF-B199","animation input preparation failed","Inspect the stage and source node in details."};
}
inline const char* node_name(const core::Graph& graph,core::NodeId id) noexcept {
    using namespace core::graph_keys;
    for(const auto& node:graph.nodes)if(node.id==id){
        const auto& t=node.type_key;
        if(t==kEmitterNode){for(const auto& p:node.parameters)if(p.key==kAuxiliarySource)if(const auto* v=std::get_if<std::uint32_t>(&p.value);v && *v==1)return "Auxiliary";return "Emitter";}if(t==kParticleNode)return "Particle";
        if(t==kForceNode)return "Force";if(t==kTransformNode)return "Transform";
        if(t==kMotionNode)return "Motion";if(t==kTurbulenceNode)return "Turbulence";
        if(t==kModelNode)return "Model";if(t==kMaterialNode)return "Material";return "Node";
    }
    return "Node";
}
inline void binding_message(char* text,std::size_t capacity,const core::Graph& graph,
    core::NodeId id,long parameter,long component,long stream,long ae,const char* stage) noexcept {
    if(!text || !capacity)return;
    char uuid[33]{};constexpr char hex[]="0123456789abcdef";
    for(std::size_t i=0;i<id.value.bytes.size();++i){uuid[2*i]=hex[id.value.bytes[i]>>4];uuid[2*i+1]=hex[id.value.bytes[i]&15];}
    const auto d=binding(stage);
    std::snprintf(text,capacity,"Starfield [%s]: %s %s p%ld.c%ld: %s (AE%ld, stream%ld, native%d). %s",
        d.code,node_name(graph,id),uuid,parameter,component,d.summary,ae,stream,STARFIELD_VERSION_BUILD,d.action);
}
inline const char* graph_code(core::ErrorCode code) noexcept {
    switch(code){
        case core::ErrorCode::invalid_request:return "SF-G901";
        case core::ErrorCode::invalid_time:return "SF-R102";
        case core::ErrorCode::unsupported_format:return "SF-R103";
        case core::ErrorCode::allocation_failed:return "SF-R104";
        case core::ErrorCode::work_limit_exceeded:return "SF-R105";
        case core::ErrorCode::cancelled:return "SF-R106";
        case core::ErrorCode::internal_failure:return "SF-R199";
    }
    return "SF-R199";
}
}
