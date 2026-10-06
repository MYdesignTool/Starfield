#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <functional>
#include <iostream>
#include <stdexcept>
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};const NeverCancelled never;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void near(double a,double b,const char* message){check(std::isfinite(a) && std::abs(a-b)<1e-9*std::max(1.,std::abs(b)),message);}
void near(Vec3 a,Vec3 b,const char* message){near(a.x,b.x,message);near(a.y,b.y,message);near(a.z,b.z,message);}
Uuid128 uuid(unsigned char n){Uuid128 id;id.bytes[15]=n;return id;}
void set(GraphNode& node,ParameterKey key,ParameterValue value){for(auto& p:node.parameters)if(p.key==key){p.value=std::move(value);return;}node.parameters.push_back({key,std::move(value)});}
GraphNode& node(Graph& graph,const char* type){return *std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](auto& n){return n.type_key==type;});}
GraphNode transform_node(unsigned char id,const ParticleTransformSettings& settings={}) {
    return {NodeId{uuid(id)},kTransformNode,1,{{kTransformAnchor,settings.anchor},{kTransformPosition,settings.position},
        {kTransformRotation,settings.rotation_degrees},{kTransformSystemScale,settings.scale_percent},
        {kTransformParticleScale,settings.particles_scale_percent},{kTransformParticleOpacity,settings.particles_opacity_percent}}};
}
void matrix(GraphNode& node,const std::array<double,16>& value) {
    OpaqueBytes bytes{std::byte{1},std::byte{0},std::byte{0},std::byte{0}};
    for(double v:value){auto bits=std::bit_cast<std::uint64_t>(v);for(unsigned b=0;b<8;++b)bytes.push_back(static_cast<std::byte>((bits>>(8*b))&255));}
    set(node,kTransformInheritedMatrix,std::move(bytes));
}
void splice(Graph& graph,NodeId source,GraphNode transform,unsigned char edge_id) {
    auto found=std::find_if(graph.edges.begin(),graph.edges.end(),[&](const auto& e){return e.source_node==source;});
    check(found!=graph.edges.end(),"splice edge missing");const auto old=*found;
    found->destination_node=transform.id;found->destination_port=kTransformParticlesIn;
    graph.edges.push_back({EdgeId{uuid(edge_id)},transform.id,kTransformParticlesOut,old.destination_node,old.destination_port});
    graph.nodes.push_back(std::move(transform));
}
Graph base() {
    Settings s;s.birth_rate=1;s.particle_count=100;s.particle_lifetime_seconds=4;s.particle_size=10;s.opacity=1;
    s.emitter_origin={.2,.1,0};s.velocity={1,0,0};s.velocity_spread=0;
    auto result=make_emitter_particle_force_output_graph(s,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(3)},NodeId{uuid(255)},
        EdgeId{uuid(11)},EdgeId{uuid(12)},EdgeId{uuid(13)});
    check(result.has_value(),"construct graph");auto graph=result.take_value();set(node(graph,kForceNode),kGravity,Vec3{0,-1,0});return graph;
}
EvaluatedGraph evaluate(const Graph& graph,RationalTime time={1,1}) {
    auto result=evaluate_particle_graph(graph,time,never);if(!result.has_value())throw std::runtime_error(result.error().detail);
    ++checks;return result.take_value();
}
const ParticleInstance& slot(const EvaluatedGraph& graph,std::uint64_t id) {
    const auto found=std::find_if(graph.particles.begin(),graph.particles.end(),[&](const auto& p){return p.id==id;});
    check(found!=graph.particles.end(),"particle identity missing");return *found;
}
struct Sampler final:TemporalGraphSampler {
    Graph graph;std::function<void(GraphNode&,double)> animate;
    Result<GraphNode> node(NodeId id,double time)override {
        const auto found=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==id;});
        if(found==graph.nodes.end())return Result<GraphNode>::failure(ErrorCode::invalid_request,"fixture identity missing");
        auto value=*found;if(animate)animate(value,time);return Result<GraphNode>::success(std::move(value));
    }
    Result<double> rate(NodeId id,double time)override {
        auto value=node(id,time);if(!value.has_value())return Result<double>::failure(value.error());
        for(const auto& p:value.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));
        return Result<double>::failure(ErrorCode::invalid_request,"fixture rate missing");
    }
};
EvaluatedGraph temporal(Sampler& sampler,RationalTime time={1,1}) {
    auto result=evaluate_temporal_particle_graph(sampler.graph,time,never,{},sampler);
    if(!result.has_value())throw std::runtime_error(result.error().detail);++checks;return result.take_value();
}
void force_order_and_roundtrip() {
    ParticleTransformSettings t;t.rotation_degrees.z=90;
    auto after=base();splice(after,NodeId{uuid(3)},transform_node(4,t),14);
    const auto after_result=evaluate(after);near(slot(after_result,0).position,Vec3{.4,1.2,0},"Force then Transform position");
    near(slot(after_result,0).velocity,Vec3{1,1,0},"Force then Transform velocity");
    auto before=base();splice(before,NodeId{uuid(2)},transform_node(4,t),14);
    const auto before_result=evaluate(before);near(slot(before_result,0).position,Vec3{-.1,.7,0},"Transform then Force position");
    near(slot(before_result,0).velocity,Vec3{},"Transform then Force velocity");
    check(after_result.sprite_bases.size()==1 && before_result.sprite_bases.size()==1,"branch basis shared");
    Sampler sampled;sampled.graph=after;auto sampled_after=temporal(sampled);
    near(slot(sampled_after,0).position,slot(after_result,0).position,"temporal Force then Transform");
    sampled.graph=before;auto sampled_before=temporal(sampled);
    near(slot(sampled_before,0).position,slot(before_result,0).position,"temporal Transform then Force");
    const auto encoded=serialize_graph(after,particle_node_registry());check(encoded.has_value(),"Transform graph encode");
    auto decoded=deserialize_graph(encoded.value(),particle_node_registry());check(decoded.has_value(),"Transform graph decode");
    near(slot(evaluate(decoded.value()),0).position,slot(after_result,0).position,"Transform graph persistence");
    const auto frozen=encode_evaluated_particles(after_result,{1,1});check(frozen.has_value(),"Transform snapshot encode");
    after.optional_records.push_back(frozen.value());near(slot(evaluate(after),0).position,slot(after_result,0).position,"Transform frozen replay");
}
void superposition() {
    auto original=base();set(node(original,kEmitterNode),kBirthRate,10.);
    auto& force=node(original,kForceNode);set(force,kLinearDrag,.8);set(force,kGravity,Vec3{.3,-1,.5});
    set(force,kGravityRandom,70.);set(force,kWind,Vec3{.4,.7,-.2});set(force,kSpin,.25);set(force,kSpinFrequency,.7);
    ParticleTransformSettings t;t.anchor={.1,-.2,.3};t.position={.3,.4,-.2};t.rotation_degrees={50,20,80};
    t.scale_percent={200,-50,0};t.particles_scale_percent=160;t.particles_opacity_percent=35;
    const auto compiled=compile_particle_transform(t);check(compiled.has_value(),"compile mixed affine");
    auto changed=original;splice(changed,NodeId{uuid(3)},transform_node(4,t),14);
    const auto first=evaluate(original,{3,1}),last=evaluate(changed,{3,1});
    check(first.particles.size()==last.particles.size(),"affine changed population");
    for(const auto& p:first.particles) {
        const auto& q=slot(last,p.id);near(q.position,compiled.value().position(p.position),"gravity/wind/spin/drag affine superposition");
        near(q.velocity,compiled.value().velocity(p.velocity),"mapped force velocity");
        near(q.size_pixels,p.size_pixels*1.6,"independent particle scale");near(q.opacity,p.opacity*.35,"independent opacity");
        check(q.sprite_basis_index==1,"matrix duplicated per particle");
    }
    ParticleTransformSettings zero;zero.scale_percent={0,0,0};zero.position={.3,.4,.5};
    changed=base();splice(changed,NodeId{uuid(3)},transform_node(4,zero),14);
    near(slot(evaluate(changed),0).position,zero.position,"singular spacing scale");
    zero.particles_scale_percent=100;near(slot(evaluate(changed),0).size_pixels,10,"spacing scale must not collapse sprite");
    const auto identity_result=evaluate(base());changed=base();splice(changed,NodeId{uuid(3)},transform_node(4),14);
    const auto identity_added=evaluate(changed);
    check(identity_added.sprite_bases.empty(),"identity Transform stores unnecessary matrices");
    for(const auto& p:identity_result.particles)near(slot(identity_added,p.id).position,p.position,"identity Transform changed dynamics");
}
void animation_and_modes() {
    auto original=base();set(node(original,kForceNode),kWind,Vec3{.2,.1,0});
    auto changed=original;splice(changed,NodeId{uuid(3)},transform_node(4),14);
    Sampler first;first.graph=original;first.animate=[](auto& n,double time){if(n.type_key==kForceNode)set(n,kGravity,Vec3{0,-1-time*.2,0});};
    Sampler last;last.graph=changed;last.animate=[](auto& n,double time){
        if(n.type_key==kForceNode)set(n,kGravity,Vec3{0,-1-time*.2,0});
        if(n.type_key==kTransformNode){set(n,kTransformRotation,Vec3{0,0,90*time});set(n,kTransformPosition,Vec3{time*.3,0,0});}
    };
    const auto baseline=temporal(first),animated=temporal(last);
    ParticleTransformSettings pose;pose.rotation_degrees.z=90;pose.position.x=.3;
    const auto transform=compile_particle_transform(pose);
    for(const auto& p:baseline.particles)near(slot(animated,p.id).position,transform.value().position(p.position),"current pose maps animated force history");
    const auto later=temporal(last,{2,1});const auto repeated=temporal(last);
    near(slot(repeated,0).position,slot(animated,0).position,"animated Transform depends on frame order");
    check(later.sprite_bases.size()==1,"animated branch matrix sharing");
    changed=base();splice(changed,NodeId{uuid(3)},transform_node(4,pose),14);set(node(changed,kEmitterNode),kEmittingMode,std::uint32_t{1});
    const auto once=evaluate(changed);check(once.particles.size()==1 && slot(once,0).age_seconds==1,"Once Transform path");
    set(node(changed,kEmitterNode),kEmittingMode,std::uint32_t{0});
    set(node(changed,kParticleNode),kLifeRandom,50.);const auto random_life=evaluate(changed);
    check(!random_life.particles.empty() && random_life.sprite_bases.size()==1,"random-life Transform path");
}
void composed_frames_and_orientation() {
    auto graph=base();ParticleTransformSettings first,last;
    first.rotation_degrees={25,0,90};first.scale_percent={200,50,-100};first.position={.1,.2,.3};
    first.particles_scale_percent=80;first.particles_opacity_percent=50;
    last.rotation_degrees={0,40,-30};last.scale_percent={50,200,0};last.position={-.3,.1,.2};
    last.particles_scale_percent=200;last.particles_opacity_percent=40;
    splice(graph,NodeId{uuid(2)},transform_node(4,first),14);
    splice(graph,NodeId{uuid(3)},transform_node(5,last),15);
    const auto a=compile_particle_transform(first),b=compile_particle_transform(last);
    const auto result=evaluate(graph);Sampler sampler;sampler.graph=graph;const auto sampled=temporal(sampler);
    for(const auto& p:result.particles) {
        const double age=p.age_seconds;auto position=a.value().position({.2,.1,0});const auto velocity=a.value().velocity({1,0,0});
        position={position.x+velocity.x*age,position.y+velocity.y*age-age*age*.5,position.z+velocity.z*age};
        near(p.position,b.value().position(position),"Force uses only its downstream Transform suffix");
        near(p.velocity,b.value().velocity({velocity.x,velocity.y-age,velocity.z}),"composed Force velocity suffix");
        near(p.size_pixels,16,"composed particle scale");near(p.opacity,.2,"composed particle opacity");
        near(slot(sampled,p.id).position,p.position,"temporal composed frames");
    }
    const auto combined=compose_particle_transforms(a.value(),b.value());check(combined.has_value(),"composed frame compile");
    check(result.sprite_bases.size()==1 && result.sprite_bases[0]==combined.value().particle_basis(),"independent composed sprite map");
    for(const unsigned mode:{1u,2u}) {
        auto baseline=base();set(node(baseline,kParticleNode),kOrientTo,mode);
        graph=baseline;ParticleTransformSettings t;t.rotation_degrees.z=90;splice(graph,NodeId{uuid(3)},transform_node(4,t),14);
        const auto original=evaluate(baseline),rotated=evaluate(graph);
        for(const auto& p:original.particles)near(slot(rotated,p.id).rotation_degrees,p.rotation_degrees,"Orient To double applied Transform rotation");
    }
}
Graph auxiliary_graph() {
    Settings settings;settings.birth_rate=1;settings.velocity={1,0,0};settings.velocity_spread=0;
    settings.particle_lifetime_seconds=1.1;settings.particle_count=100;settings.particle_size=8;settings.opacity=1;
    auto made=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)});
    check(made.has_value(),"auxiliary base");auto graph=made.take_value();
    auto emitter=graph.nodes[0];emitter.id=NodeId{uuid(4)};set(emitter,kAuxiliarySource,std::uint32_t{1});set(emitter,kBirthRate,2.);set(emitter,kVelocity,Vec3{});
    set(emitter,kInheritSize,100.);set(emitter,kInheritOpacity,100.);
    auto child=graph.nodes[1];child.id=NodeId{uuid(5)};set(child,kParticleLifetimeSeconds,2.);
    graph.edges[1].destination_node=emitter.id;graph.edges[1].destination_port=kEmitterParents;
    graph.edges.push_back({EdgeId{uuid(13)},emitter.id,kEmitterParticles,child.id,kParticleParticlesIn});
    graph.edges.push_back({EdgeId{uuid(14)},child.id,kParticleParticlesOut,NodeId{uuid(255)},kOutputParticles});
    graph.nodes.push_back(emitter);graph.nodes.push_back(child);
    ParticleTransformSettings parent;parent.position.x=.1;parent.particles_scale_percent=200;parent.particles_opacity_percent=25;
    splice(graph,NodeId{uuid(2)},transform_node(7,parent),15);
    ParticleTransformSettings own;own.rotation_degrees.z=90;own.position.y=.2;own.particles_scale_percent=50;own.particles_opacity_percent=50;
    splice(graph,NodeId{uuid(5)},transform_node(8,own),16);return graph;
}
void auxiliary() {
    auto graph=auxiliary_graph();const auto result=evaluate(graph,{5,4});
    check(result.particles.size()==4,"auxiliary Transform population");
    const auto found=std::find_if(result.particles.begin(),result.particles.end(),[](const auto& p){return std::abs(p.age_seconds-.75)<1e-9;});
    check(found!=result.particles.end(),"child survives parent death");near(found->position,Vec3{0,.8,0},"child retains transformed parent birth");
    near(found->size_pixels,8,"inherited and own particle scales");near(found->opacity,.125,"inherited and own opacity");
    check(result.sprite_bases.size()==1 && found->sprite_basis_index==1,"auxiliary result basis ownership");
    Sampler sampled;sampled.graph=graph;const auto constant=temporal(sampled,{5,4});
    check(constant.particles.size()==4,"temporal auxiliary population");
    sampled.animate=[](auto& n,double time){
        if(n.type_key==kTransformNode && n.id==NodeId{uuid(7)})set(n,kTransformPosition,Vec3{.1+time,0,0});
        if(n.type_key==kTransformNode && n.id==NodeId{uuid(8)})set(n,kTransformPosition,Vec3{0,.2+time,0});
    };
    const auto animated=temporal(sampled,{5,4});
    const auto old=std::find_if(animated.particles.begin(),animated.particles.end(),[](const auto& p){return std::abs(p.age_seconds-.75)<1e-9;});
    check(old!=animated.particles.end(),"animated child birth missing");near(old->position,Vec3{0,2.55,0},"parent at birth, own Transform now");
    auto& child_emitter=*std::find_if(graph.nodes.begin(),graph.nodes.end(),[](auto& n){return n.id==NodeId{uuid(4)};});
    set(child_emitter,kInheritVelocity,100.);
    const auto inherited=evaluate(graph,{5,4});
    const auto child=std::find_if(inherited.particles.begin(),inherited.particles.end(),[](const auto& p){return std::abs(p.age_seconds-.75)<1e-9;});
    check(child!=inherited.particles.end(),"inherited velocity child missing");
    near(child->position,Vec3{0,1.55,0},"own Transform maps inherited velocity displacement");
    near(child->velocity,Vec3{0,1,0},"own Transform maps inherited velocity");
    sampled.graph=graph;sampled.animate={};const auto history=temporal(sampled,{5,4});
    const auto historical=std::find_if(history.particles.begin(),history.particles.end(),[](const auto& p){return std::abs(p.age_seconds-.75)<1e-9;});
    check(historical!=history.particles.end(),"temporal inherited velocity child missing");
    near(historical->position,child->position,"static and temporal inherited velocity position");
    near(historical->velocity,child->velocity,"static and temporal inherited velocity");
}
void branches_and_origins() {
    ParticleTransformSettings t;t.rotation_degrees.z=90;
    auto graph=base();splice(graph,NodeId{uuid(3)},transform_node(4,t),14);
    auto force=node(graph,kForceNode);force.id=NodeId{uuid(6)};set(force,kGravity,Vec3{2,0,0});graph.nodes.push_back(force);
    graph.edges.push_back({EdgeId{uuid(15)},NodeId{uuid(2)},kParticleParticlesOut,force.id,kForceParticlesIn});
    graph.edges.push_back({EdgeId{uuid(16)},force.id,kForceParticlesOut,NodeId{uuid(4)},kTransformParticlesIn});
    const auto parallel=evaluate(graph);near(slot(parallel,0).position,Vec3{.4,2.2,0},"parallel forces merge once before Transform");
    near(slot(parallel,0).velocity,Vec3{1,3,0},"parallel forces velocity");check(parallel.sprite_bases.size()==1,"parallel path duplicated basis");
    Sampler sampler;sampler.graph=graph;near(slot(temporal(sampler),0).position,slot(parallel,0).position,"temporal parallel forces merge once");
    // A separate particle stream may use its own frame at the shared Output.
    graph=base();splice(graph,NodeId{uuid(3)},transform_node(4,t),14);
    auto emitter=node(graph,kEmitterNode);emitter.id=NodeId{uuid(5)};
    auto appearance=node(graph,kParticleNode);appearance.id=NodeId{uuid(6)};
    t.rotation_degrees.z=-90;t.position.x=.1;auto other=transform_node(7,t);
    graph.nodes.push_back(emitter);graph.nodes.push_back(appearance);graph.nodes.push_back(other);
    graph.edges.push_back({EdgeId{uuid(15)},emitter.id,kEmitterParticles,appearance.id,kParticleParticlesIn});
    graph.edges.push_back({EdgeId{uuid(16)},appearance.id,kParticleParticlesOut,other.id,kTransformParticlesIn});
    graph.edges.push_back({EdgeId{uuid(17)},other.id,kTransformParticlesOut,NodeId{uuid(255)},kOutputParticles});
    const auto streams=evaluate(graph);check(streams.particles.size()==4 && streams.sprite_bases.size()==2,"independent Particle frames at Output");
    for(const auto& p:streams.particles) {
        check(p.sprite_basis_index>0 && p.sprite_basis_index<=2,"stream matrix index");
        if(p.id==0)near(p.position,p.emitter_id==NodeId{uuid(1)}?Vec3{.4,1.2,0}:Vec3{.2,-1.2,0},"stream Transform leaked across branch");
    }
    sampler.graph=graph;const auto sampled=temporal(sampler);check(sampled.particles.size()==4 && sampled.sprite_bases.size()==2,"temporal independent Particle frames");
    struct Origins final:EmitterOriginSampler {
        Result<Vec3> sample(NodeId,double birth)override{return Result<Vec3>::success({.2+birth,.1,0});}
    } origins;
    const auto original=base();graph=original;t.position={};t.rotation_degrees.z=90;splice(graph,NodeId{uuid(3)},transform_node(4,t),14);
    const auto baseline=evaluate_particle_graph(original,{1,1},never,{},&origins);
    const auto transformed=evaluate_particle_graph(graph,{1,1},never,{},&origins);
    check(baseline.has_value() && transformed.has_value(),"origin history Transform evaluation");
    const auto compiled=compile_particle_transform(t);
    for(const auto& p:baseline.value().particles)near(slot(transformed.value(),p.id).position,compiled.value().position(p.position),"birth origin delta mapped through Transform");
}
void live_pixels() {
    auto graph=base();set(node(graph,kEmitterNode),kVelocity,Vec3{});set(node(graph,kForceNode),kGravity,Vec3{});
    set(node(graph,kParticleNode),kParticleShape,std::uint32_t{1});set(node(graph,kParticleNode),kSizeY,6.);
    ParticleTransformSettings t;t.rotation_degrees.z=90;t.position.x=.15;splice(graph,NodeId{uuid(3)},transform_node(4,t),14);
    RenderRequest request;request.settings=validate_settings(Settings{});
    request.frame={100,100,100,100,{0,0,100,100},{1,2},{1,24},PixelFormat::rgba32f,
        ColorSpace::ae_working_space,AlphaMode::premultiplied,1,Quality::full};
    request.graph=std::make_shared<const Graph>(std::move(graph));
    const auto output=CpuParticleRenderer{}.render(request,never);check(output.has_value(),"live Transform graph CPU render");
    double alpha=0,x=0,y=0,xx=0,yy=0;
    for(unsigned row=0;row<output.value().height();++row)for(unsigned column=0;column<output.value().width();++column) {
        float pixel[4];std::memcpy(pixel,output.value().pixels.data()+row*output.value().row_bytes+column*16,16);
        const double a=pixel[3],px=column+.5,py=row+.5;alpha+=a;x+=a*px;y+=a*py;xx+=a*px*px;yy+=a*py*py;
    }
    check(alpha>1,"live Transform graph rendered transparent");near(x/alpha,55,"live transformed centre X");near(y/alpha,30,"live transformed centre Y");
    check(yy/alpha-(y/alpha)*(y/alpha)>1.8*(xx/alpha-(x/alpha)*(x/alpha)),"live sprite basis did not rotate rectangular pixels");
}
void invalid_and_inherited() {
    auto graph=base();splice(graph,NodeId{uuid(3)},transform_node(4),14);
    auto& t=node(graph,kTransformNode);matrix(t,{1,.4,0,.3,0,-2,0,-.5,0,0,0,.7,0,0,0,1});
    const auto result=evaluate(graph);near(slot(result,0).position,Vec3{1.34,.3,.7},"inherited shear/reflection centre");
    check(result.sprite_bases.size()==1 && result.sprite_bases[0][1]==.4,"inherited full basis");
    auto& bytes=std::get<OpaqueBytes>(t.parameters.back().value);bytes[1]=std::byte{1};
    check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"reserved inherited header accepted");
    matrix(t,{1,0,0,0,0,1,0,0,0,0,1,0,.1,0,0,1});
    check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"projective inherited matrix accepted");
    graph=base();splice(graph,NodeId{uuid(3)},transform_node(4),14);
    graph.edges.push_back({EdgeId{uuid(19)},NodeId{uuid(3)},kForceParticlesOut,NodeId{uuid(255)},kOutputParticles});
    const auto ambiguous=evaluate_particle_graph(graph,{1,1},never);
    check(!ambiguous.has_value() && ambiguous.error().code==ErrorCode::invalid_request,"Transform bypass silently merged");
    Sampler sampled;sampled.graph=graph;check(!evaluate_temporal_particle_graph(graph,{1,1},never,{},sampled).has_value(),"temporal Transform bypass silently merged");
    graph=base();auto parked=transform_node(4);set(parked,kTransformParticleOpacity,101.);graph.nodes.push_back(parked);
    check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"parked Transform bounds ignored");
    sampled.graph=graph;check(!evaluate_temporal_particle_graph(graph,{1,1},never,{},sampled).has_value(),"temporal parked Transform bounds ignored");
    graph=base();for(unsigned char i=0;i<4;++i) {
        ParticleTransformSettings scale;scale.particles_scale_percent=10000;
        splice(graph,NodeId{uuid(i?static_cast<unsigned char>(3+i):3)},transform_node(static_cast<unsigned char>(4+i),scale),static_cast<unsigned char>(14+i));
    }
    const auto overflow=evaluate_particle_graph(graph,{1,1},never);
    check(!overflow.has_value() && overflow.error().code==ErrorCode::work_limit_exceeded,"derived Transform scale bound ignored");
    struct Stop final:Cancellation {bool is_cancelled()const noexcept override{return true;}} stop;
    const auto cancelled=evaluate_particle_graph(graph,{1,1},stop);
    check(!cancelled.has_value() && cancelled.error().code==ErrorCode::cancelled,"Transform cancellation lost");
    graph=base();splice(graph,NodeId{uuid(3)},transform_node(4),14);sampled.graph=graph;
    sampled.animate=[](auto& n,double){if(n.type_key==kTransformNode)n.schema_version=2;};
    const auto changed_schema=evaluate_temporal_particle_graph(graph,{1,1},never,{},sampled);
    check(!changed_schema.has_value() && changed_schema.error().code==ErrorCode::invalid_request,"sampled Transform schema mismatch accepted");
}
}
int main(){try{force_order_and_roundtrip();superposition();animation_and_modes();composed_frames_and_orientation();auxiliary();branches_and_origins();live_pixels();invalid_and_inherited();
    std::cout<<"Transform graph: "<<checks<<" checks passed\n";return 0;
}catch(const std::exception& error){std::cerr<<"Transform graph failed after "<<checks<<" checks: "<<error.what()<<'\n';return 1;}}
