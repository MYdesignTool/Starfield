#pragma once
#include "starfield/core/Graph.hpp"
#include <map>
#include <set>
#include <vector>

namespace starfield::adapter {
// Same closure for numeric aliases and callback-local resource preparation.
// Follow every edge, including geometry/material and Auxiliary parent links.
inline std::set<core::NodeId> graph_output_ancestors(const core::Graph& graph) {
    std::map<core::NodeId,std::vector<core::NodeId>> dependencies;
    for(const auto& edge:graph.edges)dependencies[edge.destination_node].push_back(edge.source_node);
    std::set<core::NodeId> active;std::vector<core::NodeId> pending;
    for(const auto& node:graph.nodes)if(node.type_key==core::graph_keys::kOutputNode) {
        active.insert(node.id);pending.push_back(node.id);
    }
    while(!pending.empty()) {
        const auto id=pending.back();pending.pop_back();
        if(const auto found=dependencies.find(id);found!=dependencies.end())
            for(const auto source:found->second)if(active.insert(source).second)pending.push_back(source);
    }
    return active;
}
inline core::Graph graph_output_working_copy(const core::Graph& graph) {
    const auto active=graph_output_ancestors(graph);core::Graph working;
    working.nodes.reserve(active.size());
    for(const auto& node:graph.nodes)if(active.contains(node.id))working.nodes.push_back(node);
    for(const auto& edge:graph.edges)if(active.contains(edge.source_node) && active.contains(edge.destination_node))working.edges.push_back(edge);
    working.optional_records=graph.optional_records;
    return working;
}
}
