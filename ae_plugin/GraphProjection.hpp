#pragma once
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <stdexcept>

namespace starfield::adapter::graph_projection {
inline constexpr std::uint16_t record_tag=0x8010;
inline constexpr std::size_t max_edges=252,detail_bytes=160;
struct Issue {core::EdgeId edge{};core::NodeId source{},destination{};std::uint16_t code{110};std::array<char,detail_bytes> detail{};};
struct Projection {core::Graph graph;std::vector<Issue> issues;std::size_t attempts{};};
inline std::uint16_t issue_code(const char* detail) noexcept {
    // Presentation taxonomy only; admission always uses Core's typed result.
    if(std::strstr(detail,"cycle"))return 102;
    if(std::strstr(detail,"merge") || std::strstr(detail,"ambiguous"))return 103;
    if(std::strstr(detail,"Model") || std::strstr(detail,"Material"))return 104;
    if(std::strstr(detail,"Auxiliary") || std::strstr(detail,"Emitter"))return 101;
    if(std::strstr(detail,"Particle") || std::strstr(detail,"modifier"))return 105;
    return 110;
}
inline core::Result<Projection> project(const core::Graph& raw,const core::Cancellation& cancel) {
    using R=core::Result<Projection>;
    if(raw.nodes.size()>64 || raw.edges.size()>max_edges)return R::failure(core::ErrorCode::work_limit_exceeded,"native author graph capacity exceeded");
    Projection p;p.graph=raw;
    auto checked=core::validate_particle_graph_topology(p.graph,cancel);++p.attempts;
    if(checked.ok)return R::success(std::move(p));
    if(!checked.connection_error)return R::failure(checked.error);
    p.graph.edges.clear();checked=core::validate_particle_graph_topology(p.graph,cancel);++p.attempts;
    if(!checked.ok)return R::failure(checked.error);
    auto edges=raw.edges;
    const auto is_output=[&](const core::GraphEdge& e){return std::any_of(raw.nodes.begin(),raw.nodes.end(),[&](const auto& n){return n.id==e.destination_node && n.type_key==core::graph_keys::kOutputNode;});};
    std::sort(edges.begin(),edges.end(),[&](const auto& a,const auto& b){const bool ao=is_output(a),bo=is_output(b);return ao!=bo?!ao:a.id<b.id;});
    for(const auto& edge:edges) {
        p.graph.edges.push_back(edge);checked=core::validate_particle_graph_topology(p.graph,cancel);++p.attempts;
        if(checked.ok)continue;
        if(!checked.connection_error)return R::failure(checked.error);
        p.graph.edges.pop_back();Issue issue;issue.edge=edge.id;issue.source=edge.source_node;issue.destination=edge.destination_node;
        issue.code=issue_code(checked.error.detail);
        std::memcpy(issue.detail.data(),checked.error.detail,std::min(std::strlen(checked.error.detail),issue.detail.size()-1));p.issues.push_back(issue);
    }
    checked=core::validate_particle_graph_topology(p.graph,cancel);++p.attempts;
    if(!checked.ok)return R::failure(checked.error);
    // A deterministic safe subset, not a maximal-subgraph promise. A warning
    // describes the edge's insertion-time conflict; repairing another edge may
    // make it executable at the next publication.
    return R::success(std::move(p));
}
inline core::OpaqueBytes record(const std::vector<Issue>& issues) {
    if(issues.size()>max_edges)throw std::length_error("projection issue capacity");
    core::OpaqueBytes data;
    const auto word=[&](std::uint32_t n,unsigned count){for(unsigned i=0;i<count;++i)data.push_back(std::byte((n>>(8*i))&255));};
    word(record_tag,2);word(1,2);word(static_cast<std::uint32_t>(12+issues.size()*(50+detail_bytes)),4);word(static_cast<std::uint32_t>(issues.size()),4);
    for(const auto& issue:issues){for(const auto id:{issue.edge.value,issue.source.value,issue.destination.value})for(auto b:id.bytes)data.push_back(std::byte(b));
        word(issue.code,2);for(auto c:issue.detail)data.push_back(std::byte(static_cast<unsigned char>(c)));}
    return data;
}
inline bool read(const core::Graph& graph,std::vector<Issue>& issues) {
    issues.clear();bool seen=false;
    for(const auto& data:graph.optional_records){if(data.size()<2 || data[0]!=std::byte{0x10} || data[1]!=std::byte{0x80})continue;
        if(seen || data.size()<12)return false;seen=true;
        const auto number=[&](std::size_t at,unsigned n){std::uint32_t v=0;for(unsigned i=0;i<n;++i)v|=std::to_integer<std::uint32_t>(data[at+i])<<(i*8);return v;};
        const auto count=number(8,4);
        if(number(2,2)!=1 || count>max_edges || data.size()!=12+count*(50+detail_bytes) || number(4,4)!=data.size())return false;
        for(std::size_t i=0;i<count;++i){Issue issue;const auto at=12+i*(50+detail_bytes);std::size_t offset=at;
            for(auto* uuid:{&issue.edge.value,&issue.source.value,&issue.destination.value})for(auto& b:uuid->bytes)b=std::to_integer<std::uint8_t>(data[offset++]);
            issue.code=static_cast<std::uint16_t>(number(offset,2));offset+=2;
            if(issue.code<101 || issue.code>110 || issue.edge==core::EdgeId{} || issue.source==core::NodeId{} || issue.destination==core::NodeId{})return false;
            for(auto& c:issue.detail)c=static_cast<char>(std::to_integer<unsigned char>(data[offset++]));
            if(issue.detail.back()!=0 || !std::memchr(issue.detail.data(),0,detail_bytes))return false;
            bool ended=false;
            for(auto c:issue.detail){if(!c)ended=true;else if(ended || static_cast<unsigned char>(c)<32 || static_cast<unsigned char>(c)>126)return false;}
            if(std::any_of(issues.begin(),issues.end(),[&](const auto& prior){return prior.edge==issue.edge;}))return false;
            for(const auto id:{issue.source,issue.destination})if(std::none_of(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==id;}))return false;
            issues.push_back(issue);
        }
    }
    return true;
}
}
