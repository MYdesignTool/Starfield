#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <stdexcept>

using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};
NeverCancelled never;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void near(double a,double b,const char* message,double tolerance=1e-9){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance,message);}
template<class T>T take(Result<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
template<class T>T take(SequenceResult<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
NodeId nid(unsigned value){NodeId id;id.value.bytes[14]=static_cast<std::uint8_t>(value>>8);id.value.bytes[15]=static_cast<std::uint8_t>(value);return id;}
EdgeId eid(unsigned value){EdgeId id;id.value.bytes[14]=static_cast<std::uint8_t>(value>>8);id.value.bytes[15]=static_cast<std::uint8_t>(value);return id;}
GraphNode& node(Graph& graph,unsigned value){for(auto& item:graph.nodes)if(item.id==nid(value))return item;throw std::runtime_error("node missing");}
GraphNode turbulence(unsigned id,double amplitude=.25,double frequency=1.7,std::uint32_t levels=3){
    return {nid(id),kTurbulenceNode,1,{{kTurbulenceOrigin,Vec3{}},{kTurbulenceAmplitude,amplitude},
        {kTurbulenceFrequency,frequency},{kTurbulenceLevels,levels},{kTurbulenceEvolutionSpeed,.2},
        {kTurbulenceOffset,Vec3{.31,-.27,.13}}}};
}
Graph base(){Settings settings;settings.particle_count=64;settings.birth_rate=4;settings.particle_lifetime_seconds=2;
    settings.emitter_origin={.35,-.15,.2};settings.velocity={.16,-.09,.04};settings.emission_speed=0;settings.velocity_spread=0;
    return take(make_emitter_particle_output_graph(settings,nid(1),nid(2),nid(255),eid(1),eid(2)));}
Graph attach_turbulence(Graph graph,unsigned id=3){
    auto edge=std::find_if(graph.edges.begin(),graph.edges.end(),[](const auto& value){return value.source_node==nid(2)&&value.destination_node==nid(255);});
    if(edge==graph.edges.end())throw std::runtime_error("Particle to Output edge missing");
    edge->destination_node=nid(id);edge->destination_port=kTurbulenceParticlesIn;
    graph.nodes.push_back(turbulence(id));graph.edges.push_back({eid(20),nid(id),kTurbulenceParticlesOut,nid(255),kOutputParticles});
    return graph;
}
EvaluatedGraph evaluate(const Graph& graph,double seconds=1){return take(evaluate_particle_graph(graph,{static_cast<std::int64_t>(seconds),1},never));}
void positions_near(const EvaluatedGraph& a,const EvaluatedGraph& b){
    check(a.particles.size()==b.particles.size(),"same particle count");
    for(std::size_t i=0;i<a.particles.size();++i){near(a.particles[i].position.x,b.particles[i].position.x,"x position");
        near(a.particles[i].position.y,b.particles[i].position.y,"y position");near(a.particles[i].position.z,b.particles[i].position.z,"z position");}
}
void field_is_applied_deterministically(){
    const auto plain=evaluate(base());auto graph=attach_turbulence(base());const auto first=evaluate(graph),again=evaluate(graph);
    check(first.particles.size()==plain.particles.size()&&!first.particles.empty(),"field preserves population");
    bool moved=false;for(std::size_t i=0;i<first.particles.size();++i){const auto& a=first.particles[i].position;const auto& b=plain.particles[i].position;
        moved|=std::abs(a.x-b.x)>1e-8||std::abs(a.y-b.y)>1e-8||std::abs(a.z-b.z)>1e-8;}
    check(moved,"nonzero turbulence displaces particles");positions_near(first,again);
    auto zero=graph;for(auto& item:zero.nodes)if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceAmplitude)parameter.value=0.0;
    positions_near(evaluate(zero),plain);
}
void graph_storage_and_input_validation(){
    auto graph=attach_turbulence(base());const auto bytes=take(serialize_graph(graph,particle_node_registry()));
    auto restored=take(deserialize_graph(bytes,particle_node_registry()));positions_near(evaluate(restored),evaluate(graph));
    auto bad=graph;for(auto& item:bad.nodes)if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceLevels)parameter.value=std::uint32_t{9};
    check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"levels above bounded range rejected");
    bad=graph;for(auto& item:bad.nodes)if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceFrequency)parameter.value=std::numeric_limits<double>::quiet_NaN();
    check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"nonfinite frequency rejected");
    bad=graph;for(auto& item:bad.nodes)if(item.type_key==kTurbulenceNode)
        item.parameters.push_back({kTurbulenceMode,static_cast<std::uint32_t>(TurbulenceMode::spherical)});
    check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"unimplemented spherical mode is rejected");
}
void axis_modes_constrain_displacement(){
    const auto plain=evaluate(base());
    auto axis_graph=attach_turbulence(base());
    for(auto& item:axis_graph.nodes)if(item.type_key==kTurbulenceNode) {
        item.parameters.push_back({kTurbulenceMode,static_cast<std::uint32_t>(TurbulenceMode::axis)});
        item.parameters.push_back({kTurbulenceAxis,std::uint32_t{1}});
    }
    const auto axis=evaluate(axis_graph);bool changed=false;
    for(std::size_t i=0;i<axis.particles.size();++i) {
        near(axis.particles[i].position.x,plain.particles[i].position.x,"axis mode preserves X");
        near(axis.particles[i].position.z,plain.particles[i].position.z,"axis mode preserves Z");
        changed|=std::abs(axis.particles[i].position.y-plain.particles[i].position.y)>1e-8;
    }
    check(changed,"axis mode displaces along the selected axis");

    auto one_side=axis_graph;for(auto& item:one_side.nodes)if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceMode)
            parameter.value=static_cast<std::uint32_t>(TurbulenceMode::one_side_axis);
    const auto positive=evaluate(one_side);
    for(std::size_t i=0;i<positive.particles.size();++i)
        check(positive.particles[i].position.y+1e-9>=plain.particles[i].position.y,
            "one-side axis never displaces against its selected direction");
}
void sphere_affect_is_bounded_and_feathered(){
    const auto plain=evaluate(base());
    auto outside=attach_turbulence(base());
    for(auto& item:outside.nodes)if(item.type_key==kTurbulenceNode) {
        item.parameters.push_back({kTurbulenceAffect,static_cast<std::uint32_t>(TurbulenceAffect::feathered_sphere)});
        item.parameters.push_back({kTurbulenceSphereSize,.01});
        item.parameters.push_back({kTurbulenceSphereFeather,100.0});
    }
    positions_near(evaluate(outside),plain);

    auto soft=attach_turbulence(base()),hard=soft;
    for(auto& item:soft.nodes)if(item.type_key==kTurbulenceNode) {
        item.parameters.push_back({kTurbulenceAffect,static_cast<std::uint32_t>(TurbulenceAffect::feathered_sphere)});
        item.parameters.push_back({kTurbulenceSphereSize,1.0});
        item.parameters.push_back({kTurbulenceSphereFeather,100.0});
    }
    for(auto& item:hard.nodes)if(item.type_key==kTurbulenceNode) {
        item.parameters.push_back({kTurbulenceAffect,static_cast<std::uint32_t>(TurbulenceAffect::feathered_sphere)});
        item.parameters.push_back({kTurbulenceSphereSize,1.0});
        item.parameters.push_back({kTurbulenceSphereFeather,0.0});
    }
    const auto soft_result=evaluate(soft),hard_result=evaluate(hard);bool feather_changes_result=false;
    for(std::size_t i=0;i<soft_result.particles.size();++i) {
        const auto& a=soft_result.particles[i].position;const auto& b=hard_result.particles[i].position;
        feather_changes_result|=std::abs(a.x-b.x)>1e-8||std::abs(a.y-b.y)>1e-8||std::abs(a.z-b.z)>1e-8;
    }
    check(feather_changes_result,"sphere feather fades the field toward its boundary");

    auto invalid=outside;for(auto& item:invalid.nodes)if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceSphereSize)parameter.value=0.0;
    check(!evaluate_particle_graph(invalid,{1,1},never).has_value(),"nonpositive sphere size rejected");
}
void randomize_varies_per_particle_but_is_repeatable(){
    auto graph=attach_turbulence(base());for(auto& item:graph.nodes)if(item.type_key==kTurbulenceNode)
        item.parameters.push_back({kTurbulenceRandomize,100.0});
    const auto first=evaluate(graph),again=evaluate(graph),baseline=evaluate(base()),
        unrandomized=evaluate(attach_turbulence(base()));
    positions_near(first,again);
    check(first.particles.size()==baseline.particles.size()&&first.particles.size()==unrandomized.particles.size(),
        "randomization preserves particle count");
    bool varied=false;std::optional<double> first_scale;
    for(std::size_t i=0;i<first.particles.size();++i) {
        const auto& random_position=first.particles[i].position;
        const auto& full_position=unrandomized.particles[i].position;
        const auto& base_position=baseline.particles[i].position;
        const double full_x=full_position.x-base_position.x,full_y=full_position.y-base_position.y,
            full_z=full_position.z-base_position.z;
        const double random_x=random_position.x-base_position.x,random_y=random_position.y-base_position.y,
            random_z=random_position.z-base_position.z;
        const double full_length=std::sqrt(full_x*full_x+full_y*full_y+full_z*full_z);
        if(full_length<=1e-8)continue;
        const double ratio=std::sqrt(random_x*random_x+random_y*random_y+random_z*random_z)/full_length;
        if(!first_scale)first_scale=ratio;
        else if(std::abs(ratio-*first_scale)>1e-6)varied=true;
    }
    check(first_scale.has_value(),"randomize test has nonzero baseline displacement");
    check(varied,"randomize varies displacement amount between particle identities");
}
void route_order_and_fanout_are_isolated(){
    auto first=attach_turbulence(base(),3);
    GraphNode circle{nid(4),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{}},
        {kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,1.2}}};
    auto turb_to_circle=std::find_if(first.edges.begin(),first.edges.end(),[](const auto& edge){return edge.source_node==nid(3)&&edge.destination_node==nid(255);});
    check(turb_to_circle!=first.edges.end(),"Turbulence output exists");turb_to_circle->destination_node=nid(4);turb_to_circle->destination_port=kMotionParticlesIn;
    first.nodes.push_back(circle);first.edges.push_back({eid(21),nid(4),kMotionParticlesOut,nid(255),kOutputParticles});
    auto reverse=base();auto particle_edge=std::find_if(reverse.edges.begin(),reverse.edges.end(),[](const auto& edge){return edge.source_node==nid(2)&&edge.destination_node==nid(255);});
    particle_edge->destination_node=nid(4);particle_edge->destination_port=kMotionParticlesIn;reverse.nodes.push_back(circle);
    reverse.edges.push_back({eid(22),nid(4),kMotionParticlesOut,nid(3),kTurbulenceParticlesIn});reverse.nodes.push_back(turbulence(3));
    reverse.edges.push_back({eid(23),nid(3),kTurbulenceParticlesOut,nid(255),kOutputParticles});
    const auto ordered_a=evaluate(first),ordered_b=evaluate(reverse);bool different=false;
    for(std::size_t i=0;i<ordered_a.particles.size();++i){const auto& a=ordered_a.particles[i].position;const auto& b=ordered_b.particles[i].position;
        different|=std::abs(a.x-b.x)>1e-8||std::abs(a.y-b.y)>1e-8||std::abs(a.z-b.z)>1e-8;}
    check(different,"upstream to downstream order changes the result");

    auto turbulence_then_transform=attach_turbulence(base(),3);
    auto terminal=std::find_if(turbulence_then_transform.edges.begin(),turbulence_then_transform.edges.end(),[](const auto& edge){return edge.source_node==nid(3)&&edge.destination_node==nid(255);});
    terminal->destination_node=nid(5);terminal->destination_port=kTransformParticlesIn;
    turbulence_then_transform.nodes.push_back({nid(5),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.4,.2,0}},
        {kTransformRotation,Vec3{}},{kTransformSystemScale,Vec3{100,100,100}},{kTransformParticleScale,100.},{kTransformParticleOpacity,100.}}});
    turbulence_then_transform.edges.push_back({eid(24),nid(5),kTransformParticlesOut,nid(255),kOutputParticles});
    auto transform_then_turbulence=base();auto source=std::find_if(transform_then_turbulence.edges.begin(),transform_then_turbulence.edges.end(),[](const auto& edge){return edge.source_node==nid(2)&&edge.destination_node==nid(255);});
    source->destination_node=nid(5);source->destination_port=kTransformParticlesIn;
    transform_then_turbulence.nodes.push_back({nid(5),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.4,.2,0}},
        {kTransformRotation,Vec3{}},{kTransformSystemScale,Vec3{100,100,100}},{kTransformParticleScale,100.},{kTransformParticleOpacity,100.}}});
    transform_then_turbulence.edges.push_back({eid(25),nid(5),kTransformParticlesOut,nid(3),kTurbulenceParticlesIn});
    transform_then_turbulence.nodes.push_back(turbulence(3));transform_then_turbulence.edges.push_back({eid(26),nid(3),kTurbulenceParticlesOut,nid(255),kOutputParticles});
    const auto tf_before=evaluate(transform_then_turbulence),tf_after=evaluate(turbulence_then_transform);bool transform_order_visible=false;
    for(std::size_t i=0;i<tf_before.particles.size();++i){const auto& a=tf_before.particles[i].position;const auto& b=tf_after.particles[i].position;
        transform_order_visible|=std::abs(a.x-b.x)>1e-8||std::abs(a.y-b.y)>1e-8||std::abs(a.z-b.z)>1e-8;}
    check(transform_order_visible,"Transform and Turbulence follow edge order");

    auto fanout=base();fanout.edges.clear();fanout.edges.push_back({eid(30),nid(1),kEmitterParticles,nid(2),kParticleParticlesIn});
    fanout.nodes.push_back(turbulence(3,.2));fanout.nodes.push_back(turbulence(4,.6));
    fanout.edges.push_back({eid(31),nid(2),kParticleParticlesOut,nid(3),kTurbulenceParticlesIn});
    fanout.edges.push_back({eid(32),nid(2),kParticleParticlesOut,nid(4),kTurbulenceParticlesIn});
    fanout.edges.push_back({eid(33),nid(3),kTurbulenceParticlesOut,nid(255),kOutputParticles});
    fanout.edges.push_back({eid(34),nid(4),kTurbulenceParticlesOut,nid(255),kOutputParticles});
    const auto branched=evaluate(fanout);check(branched.particles.size()==2*evaluate(base()).particles.size(),"each Output route retains its own population");
}
struct Sampler final:TemporalGraphSampler {
    Graph graph;std::function<void(GraphNode&,double)> change;std::map<std::pair<NodeId,double>,unsigned> calls;
    explicit Sampler(Graph value):graph(std::move(value)){}
    Result<GraphNode> node(NodeId id,double time)override{for(const auto& item:graph.nodes)if(item.id==id){auto copy=item;
        if(copy.type_key==kTurbulenceNode)++calls[{id,time}];if(change)change(copy,time);return Result<GraphNode>::success(std::move(copy));}
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"sampler node missing");}
    Result<double> rate(NodeId id,double time)override{auto sampled=node(id,time);if(!sampled.has_value())return Result<double>::failure(sampled.error());
        for(const auto& parameter:sampled.value().parameters)if(parameter.key==kBirthRate)return Result<double>::success(std::get<double>(parameter.value));
        return Result<double>::failure(ErrorCode::invalid_request,"rate missing");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override{auto sampled=node(id,0);if(!sampled.has_value())return Result<std::optional<EmissionRateProfile>>::failure(sampled.error());
        for(const auto& parameter:sampled.value().parameters)if(parameter.key==kBirthRate){EmissionRateProfile p;p.constant=std::get<double>(parameter.value);
            return Result<std::optional<EmissionRateProfile>>::success(p);}return Result<std::optional<EmissionRateProfile>>::failure(ErrorCode::invalid_request,"rate profile missing");}
};
void temporal_sampling_uses_one_frame_value_per_node(){
    auto graph=attach_turbulence(base());Sampler sampler(graph);sampler.change=[](GraphNode& item,double time){if(item.type_key==kTurbulenceNode)
        for(auto& parameter:item.parameters)if(parameter.key==kTurbulenceOffset)parameter.value=Vec3{.31+time,-.27,.13};};
    const auto output=take(evaluate_temporal_particle_graph(graph,{1,1},never,{},sampler));
    check(!output.particles.empty(),"temporal Turbulence emits particles");
    unsigned frame_reads=0;for(const auto& [key,count]:sampler.calls)if(key.first==nid(3)&&key.second==1.0)frame_reads+=count;
    check(frame_reads==1,"animated Turbulence node sampled once per frame and reused across particles");
    auto expected=graph;for(auto& item:expected.nodes)if(item.id==nid(3))for(auto& parameter:item.parameters)
        if(parameter.key==kTurbulenceOffset)parameter.value=Vec3{1.31,-.27,.13};
    positions_near(output,evaluate(expected));
}
}
int main(){try{field_is_applied_deterministically();graph_storage_and_input_validation();axis_modes_constrain_displacement();sphere_affect_is_bounded_and_feathered();randomize_varies_per_particle_but_is_repeatable();route_order_and_fanout_are_isolated();temporal_sampling_uses_one_frame_value_per_node();
    std::printf("Turbulence graph tests passed (%u checks).\n",checks);return 0;}catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}}
