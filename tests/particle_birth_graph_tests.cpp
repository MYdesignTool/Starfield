#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ParticleBirth.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};
NeverCancelled never;
void check(bool value,const char* label){++checks;if(!value)throw std::runtime_error(label);}
template<class T>T take(Result<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
template<class T>T take(SequenceResult<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
NodeId nid(unsigned value){NodeId id;id.value.bytes.back()=static_cast<std::uint8_t>(value);return id;}
EdgeId eid(unsigned value){EdgeId id;id.value.bytes.back()=static_cast<std::uint8_t>(value);return id;}
GraphNode& node(Graph& graph,unsigned id){for(auto& n:graph.nodes)if(n.id==nid(id))return n;throw std::runtime_error("node missing");}
const ParameterValue& get(const GraphNode& n,ParameterKey key){for(const auto& p:n.parameters)if(p.key==key)return p.value;throw std::runtime_error("key missing");}
void set(GraphNode& n,ParameterKey key,ParameterValue value){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(value);return;}n.parameters.push_back({key,std::move(value)});}
void controls(Graph& g,unsigned id=2,std::int32_t shift=0,double chance=100){set(node(g,id),kParticleSeedShift,shift);set(node(g,id),kParticleBirthChance,chance);}
Graph base(double rate=4,std::uint32_t cap=128) {
    Settings settings;settings.birth_rate=rate;settings.particle_count=cap;settings.seed=7;settings.particle_lifetime_seconds=2;
    settings.emitter_shape=EmitterShape::box;settings.emitter_size_pixels={80,100,60};
    settings.velocity={.2,.1,.15};settings.velocity_spread=.7;settings.emission_speed=150;
    settings.emission_speed_random=35;settings.direction_span_degrees=90;settings.particle_size_random_percent=60;
    settings.opacity_random_percent=30;
    return take(make_emitter_particle_output_graph(settings,nid(1),nid(2),nid(3),eid(1),eid(2)));
}
EvaluatedGraph evaluate(const Graph& graph,RationalTime time={1,1}){return take(evaluate_particle_graph(graph,time,never));}
class Sampler:public TemporalGraphSampler {
    Graph graph_;
public:
    bool animate_chance{},animate_shift{},mutate_membership{},certify_zero{};
    unsigned samples{};
    explicit Sampler(Graph g):graph_(std::move(g)){}
    Result<GraphNode> node(NodeId id,double time)override {
        ++samples;
        for(const auto& n:graph_.nodes)if(n.id==id){auto result=n;
            if(n.type_key==kParticleNode) {
                if(animate_chance)set(result,kParticleBirthChance,time<.5?100.0:0.0);
                if(animate_shift)set(result,kParticleSeedShift,std::int32_t(time<.5?0:5));
                if(mutate_membership)set(result,kParticleBirthChance,50.0);
            }
            return Result<GraphNode>::success(std::move(result));
        }
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"sampler missing node");
    }
    Result<double> rate(NodeId id,double)override {for(const auto& n:graph_.nodes)if(n.id==id)return Result<double>::success(std::get<double>(get(n,kBirthRate)));
        return Result<double>::failure(ErrorCode::invalid_request,"sampler missing rate");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override {auto value=rate(id,0);if(!value.has_value())return Result<std::optional<EmissionRateProfile>>::failure(value.error());
        EmissionRateProfile profile;profile.constant=value.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(profile));}
    std::optional<double> lifetime_upper_bound(NodeId id)override {for(const auto& n:graph_.nodes)if(n.id==id)return std::get<double>(get(n,kParticleLifetimeSeconds));return {};}
    std::optional<double> constant_birth_chance(NodeId)override {return certify_zero?std::optional<double>{0}:std::nullopt;}
};
EvaluatedGraph temporal(const Graph& g,RationalTime time={1,1}){Sampler sampler(g);return take(evaluate_temporal_particle_graph(g,time,never,{},sampler));}
void close(double a,double b,const char* label){check(std::abs(a-b)<1.e-8,label);}
void same_particle(const ParticleInstance& a,const ParticleInstance& b,bool identity=true) {
    if(identity)check(a.id==b.id && a.emitter_id==b.emitter_id,"stable particle identity");
    close(a.age_seconds,b.age_seconds,"age unchanged");close(a.lifetime_seconds,b.lifetime_seconds,"life unchanged");
    close(a.position.x,b.position.x,"source motion X");close(a.position.y,b.position.y,"source motion Y");close(a.position.z,b.position.z,"source motion Z");
    close(a.velocity.x,b.velocity.x,"source velocity X");close(a.velocity.y,b.velocity.y,"source velocity Y");close(a.velocity.z,b.velocity.z,"source velocity Z");
    close(a.size_pixels,b.size_pixels,"random size unchanged");close(a.opacity,b.opacity,"random opacity unchanged");
    close(a.color.x,b.color.x,"random color X");close(a.color.y,b.color.y,"random color Y");close(a.color.z,b.color.z,"random color Z");
    close(a.rotation_degrees.x,b.rotation_degrees.x,"random rotation X");
    close(a.rotation_degrees.y,b.rotation_degrees.y,"random rotation Y");
    close(a.rotation_degrees.z,b.rotation_degrees.z,"random rotation Z");
    check(a.texture_random_key==b.texture_random_key && a.cloud_random_key==b.cloud_random_key,"source style random keys unchanged");
}
void same_scene(const EvaluatedGraph& a,const EvaluatedGraph& b){check(a.particles.size()==b.particles.size(),"scene populations match");
    for(std::size_t i=0;i<a.particles.size();++i)same_particle(a.particles[i],b.particles[i]);}
Graph two_particles(bool authored) {
    auto graph=base();if(authored)controls(graph);
    auto second=node(graph,2);second.id=nid(4);set(second,kParticleShape,std::uint32_t{1});graph.nodes.push_back(std::move(second));
    graph.edges.push_back({eid(3),nid(1),kEmitterParticles,nid(4),kParticleParticlesIn});
    graph.edges.push_back({eid(4),nid(4),kParticleParticlesOut,nid(3),kOutputParticles});return graph;
}
void static_and_history() {
    auto legacy=base();auto old=evaluate(legacy);check(old.particles.size()==5,"legacy birth count");
    for(std::size_t i=0;i<old.particles.size();++i)check(old.particles[i].id==i,"legacy source ordinals retained");
    auto configured=legacy;controls(configured);auto explicit_defaults=evaluate(configured);
    check(explicit_defaults.particles.size()==old.particles.size(),"explicit defaults preserve single-source population");
    for(std::size_t i=0;i<old.particles.size();++i)same_particle(old.particles[i],explicit_defaults.particles[i],false);
    same_scene(explicit_defaults,temporal(configured));
    controls(configured,2,5);auto shifted=evaluate(configured);
    auto seed_override=configured;set(node(seed_override,1),kSeed,std::uint32_t{12});controls(seed_override,2,0);
    same_scene(shifted,evaluate(seed_override));same_scene(shifted,temporal(configured));
    check(shifted.particles[0].position.x!=explicit_defaults.particles[0].position.x &&
        shifted.particles[0].size_pixels!=explicit_defaults.particles[0].size_pixels,"shift affects source placement and random appearance");
    for(std::size_t i=0;i<shifted.particles.size();++i)check(shifted.particles[i].id==explicit_defaults.particles[i].id,"seed shift preserves output identity");
    for(auto shift:{-100,std::numeric_limits<std::int32_t>::min(),std::numeric_limits<std::int32_t>::max()}) {
        controls(configured,2,shift);same_scene(evaluate(configured),temporal(configured));
    }
    controls(configured,2,0);Sampler history(configured);history.animate_chance=true;
    auto selected=take(evaluate_temporal_particle_graph(configured,{1,1},never,{},history));
    check(selected.particles.size()==2 && selected.particles[0].age_seconds==1 && selected.particles[1].age_seconds==.75,"chance samples original birth time");
    same_scene(selected,take(evaluate_temporal_particle_graph(configured,{1,1},never,{},history)));
    history.animate_chance=false;history.animate_shift=true;
    auto animated=take(evaluate_temporal_particle_graph(configured,{1,1},never,{},history));
    auto all_shifted=configured;controls(all_shifted,2,5);auto all=evaluate(all_shifted);
    for(std::size_t i=0;i<animated.particles.size();++i)same_particle(animated.particles[i],animated.particles[i].age_seconds>.5?explicit_defaults.particles[i]:all.particles[i]);
    Sampler invalid_sampler(legacy);invalid_sampler.mutate_membership=true;
    auto invalid=evaluate_temporal_particle_graph(legacy,{1,1},never,{},invalid_sampler);
    check(!invalid.has_value() && invalid.error().code==ErrorCode::invalid_request,"historical sampler cannot silently enable a different branch contract");
    auto snapshot=take(encode_evaluated_particles(shifted,{1,1}));same_scene(shifted,take(decode_evaluated_particles(snapshot,{1,1})));
    auto wire=take(serialize_graph(configured,particle_node_registry()));same_scene(evaluate(configured),evaluate(take(deserialize_graph(wire,particle_node_registry()))));
}
void random_style() {
    auto graph=base();controls(graph);
    set(node(graph,2),kLifeRandom,35.0);set(node(graph,2),kAngleRandom,40.0);
    set(node(graph,2),kRotationSpeed,Vec3{30,-20,10});set(node(graph,2),kRotationSpeedRandom,25.0);
    set(node(graph,2),kParticleColorMode,std::uint32_t{2});
    set(node(graph,2),kColorStart,Vec3{1,0,0});set(node(graph,2),kColorEnd,Vec3{0,0,1});
    const auto original=evaluate(graph);same_scene(original,temporal(graph));
    controls(graph,2,5);const auto shifted=evaluate(graph);same_scene(shifted,temporal(graph));
    auto emitter_override=graph;set(node(emitter_override,1),kSeed,std::uint32_t{12});controls(emitter_override);
    same_scene(shifted,evaluate(emitter_override));
    check(shifted.particles[0].lifetime_seconds!=original.particles[0].lifetime_seconds,"shift affects Life Random");
    check(shifted.particles[0].color.x!=original.particles[0].color.x,"shift affects random gradient color");
    check(shifted.particles[0].rotation_degrees.x!=original.particles[0].rotation_degrees.x,"shift affects random angle and speed");
    auto bytes=take(encode_evaluated_particles(shifted,{1,1}));same_scene(shifted,take(decode_evaluated_particles(bytes,{1,1})));
}
void subsets_and_caps() {
    auto graph=base(100,1024);controls(graph,2,0,25);auto quarter=evaluate(graph);
    controls(graph,2,0,50);auto half=evaluate(graph);controls(graph,2,0,100);auto full=evaluate(graph);
    check(quarter.particles.size()>10 && quarter.particles.size()<40 && half.particles.size()>30 && half.particles.size()<70 && full.particles.size()==101,"reference chance ranges select source subsets");
    for(const auto& p:quarter.particles){const auto found=std::find_if(half.particles.begin(),half.particles.end(),[&](const auto& q){return p.id==q.id;});check(found!=half.particles.end(),"nested chance subset");same_particle(p,*found);}
    controls(graph,2,0,25);same_scene(quarter,temporal(graph));
    set(node(graph,3),kParticleCount,std::uint32_t{2});auto capped=evaluate(graph);
    check(capped.particles.size()==2,"low chance scans older candidates to fill population cap");
    same_particle(capped.particles[0],quarter.particles[quarter.particles.size()-2]);same_particle(capped.particles[1],quarter.particles.back());
    same_scene(capped,temporal(graph));
    controls(graph,2,0,0);check(evaluate(graph).particles.empty() && temporal(graph).particles.empty(),"zero chance no births");
    set(node(graph,1),kEmittingMode,std::uint32_t{1});controls(graph,2,0,50);auto once=evaluate(graph);
    check(!once.particles.empty() && once.particles.size()==2,"Once respects global cap after chance");same_scene(once,temporal(graph));
    controls(graph,2,0,0);check(evaluate(graph).particles.empty(),"Once zero chance");
}
void branching() {
    auto legacy=two_particles(false);check(evaluate(legacy).particles.size()==5,"missing controls preserve legacy partitioning");
    auto graph=two_particles(true);auto doubled=evaluate(graph);check(doubled.particles.size()==10,"each authored Particle receives the full source");
    std::set<std::uint64_t> identities;for(const auto& p:doubled.particles)identities.insert(p.id);check(identities.size()==10,"sibling output identities unique");
    for(std::size_t i=0;i<doubled.particles.size();i+=2)same_particle(doubled.particles[i],doubled.particles[i+1],false);
    same_scene(doubled,temporal(graph));
    set(node(graph,3),kParticleCount,std::uint32_t{3});same_scene(evaluate(graph),temporal(graph));
    std::reverse(graph.nodes.begin(),graph.nodes.end());std::reverse(graph.edges.begin(),graph.edges.end());same_scene(evaluate(graph),temporal(graph));
    auto other=base();controls(other);auto emitter=node(other,1);emitter.id=nid(6);other.nodes.push_back(emitter);
    other.edges.push_back({eid(8),nid(6),kEmitterParticles,nid(2),kParticleParticlesIn});
    auto sources=evaluate(other);check(sources.particles.size()==10,"shared Particle receives both emitters");same_scene(sources,temporal(other));
    auto motion_graph=two_particles(true);auto first=evaluate(motion_graph,{1,1}),last=evaluate(motion_graph,{5,4});
    auto middle=take(interpolate_motion_particles(first,last,1,1.25,.5,128,never));
    check(middle.particles.size()==10,"shutter pairs sibling streams without overwriting identities");
    std::set<std::uint64_t> motion_ids;for(const auto& p:middle.particles)motion_ids.insert(p.id);
    check(motion_ids.size()==10,"interpolated sibling identities remain distinct");
    for(const auto& p:middle.particles) {
        const auto a=std::find_if(first.particles.begin(),first.particles.end(),[&](const auto& q){return p.id==q.id;});
        const auto b=std::find_if(last.particles.begin(),last.particles.end(),[&](const auto& q){return p.id==q.id;});
        check(a!=first.particles.end() && b!=last.particles.end(),"shutter matched original ordinal in its own branch");
        close(p.position.x,(a->position.x+b->position.x)/2,"shutter interpolates the correct branch trajectory");
    }
}
void force_and_transform() {
    auto graph=base();controls(graph,2,-13,65);
    graph.nodes.push_back({nid(7),kForceNode,3,{{kGravity,Vec3{.03,-.12,.04}},{kLinearDrag,0.0}}});
    graph.nodes.push_back({nid(8),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.2,-.1,.3}},
        {kTransformRotation,Vec3{10,25,40}},{kTransformSystemScale,Vec3{120,80,150}},
        {kTransformParticleScale,75.0},{kTransformParticleOpacity,65.0},{kTransformInheritLayer,std::uint32_t{0}}}});
    graph.edges.pop_back();graph.edges.push_back({eid(7),nid(2),kParticleParticlesOut,nid(7),kForceParticlesIn});
    graph.edges.push_back({eid(8),nid(7),kForceParticlesOut,nid(8),kTransformParticlesIn});
    graph.edges.push_back({eid(9),nid(8),kTransformParticlesOut,nid(3),kOutputParticles});
    auto transformed=evaluate(graph);check(!transformed.particles.empty(),"birth controls retain Force/Transform output");same_scene(transformed,temporal(graph));
    check(!transformed.sprite_bases.empty() && transformed.particles[0].sprite_basis_index!=0,"birth controls retain shared sprite basis");
    auto repeat=evaluate(graph,{3,2});(void)repeat;same_scene(transformed,evaluate(graph));
}
Graph auxiliary() {
    auto graph=base(4,128);auto emitter=node(graph,1);emitter.id=nid(4);set(emitter,kAuxiliarySource,std::uint32_t{1});set(emitter,kEmitterOrigin,Vec3{});
    auto child=node(graph,2);child.id=nid(5);graph.nodes.push_back(std::move(emitter));graph.nodes.push_back(std::move(child));
    graph.edges.pop_back();graph.edges.push_back({eid(3),nid(2),kParticleParticlesOut,nid(4),kEmitterParents});
    graph.edges.push_back({eid(4),nid(4),kEmitterParticles,nid(5),kParticleParticlesIn});
    graph.edges.push_back({eid(5),nid(5),kParticleParticlesOut,nid(3),kOutputParticles});controls(graph,5,5,45);return graph;
}
void auxiliary_and_limits() {
    auto graph=auxiliary();auto children=evaluate(graph);check(!children.particles.empty(),"Auxiliary chance retains children");same_scene(children,temporal(graph));
    controls(graph,5,5,0);check(evaluate(graph).particles.empty() && temporal(graph).particles.empty(),"Auxiliary zero chance");
    controls(graph,5,0,100);auto child=node(graph,5);child.id=nid(6);graph.nodes.push_back(std::move(child));
    graph.edges.push_back({eid(6),nid(4),kEmitterParticles,nid(6),kParticleParticlesIn});graph.edges.push_back({eid(7),nid(6),kParticleParticlesOut,nid(3),kOutputParticles});
    auto pair=evaluate(graph);same_scene(pair,temporal(graph));std::set<std::uint64_t> ids;for(const auto& p:pair.particles)ids.insert(p.id);check(ids.size()==pair.particles.size(),"Auxiliary sibling identities distinct");
    auto bad=base();controls(bad,2,0,101);check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"chance above100 rejects");
    Sampler invalid(bad);check(!evaluate_temporal_particle_graph(bad,{-1,1},never,{},invalid).has_value(),"invalid authored chance rejects on empty negative frames");
    set(node(bad,2),kParticleSeedShift,1.0);check(!validate_graph(bad,particle_node_registry()),"shift must be int32");
    auto huge=base(1'000'000,2);set(node(huge,2),kParticleLifetimeSeconds,10000.0);controls(huge,2,0,0);
    check(evaluate(huge,{10000,1}).particles.empty(),"zero chance skips a huge static source window");
    Sampler zero(huge);zero.certify_zero=true;
    check(take(evaluate_temporal_particle_graph(huge,{10000,1},never,{},zero)).particles.empty() && zero.samples==0,
        "certified zero chance skips a huge historical clock without sampling births");
    auto once=huge;set(node(once,1),kEmittingMode,std::uint32_t{1});
    check(evaluate(once).particles.empty(),"static Once certifies zero chance before million-birth sampling");
    controls(huge,2,0,1.e-300);
    struct Cancel:Cancellation {mutable unsigned polls{};bool is_cancelled()const noexcept override{return ++polls>50;}} cancel;
    auto cancelled=evaluate_particle_graph(huge,{10000,1},cancel);
    check(!cancelled.has_value() && cancelled.error().code==ErrorCode::cancelled,"rejected candidate scan observes cancellation");
    auto bounded=evaluate_particle_graph(huge,{10000,1},never);
    check(!bounded.has_value() && bounded.error().code==ErrorCode::work_limit_exceeded,"rejected candidate scan ends with typed work limit, not silent truncation");
}
}
int main()try {static_and_history();random_style();subsets_and_caps();branching();force_and_transform();auxiliary_and_limits();
    std::printf("particle_birth_graph_tests: %u checks passed; native/CEP/AE2023 gates remain open\n",checks);return 0;
}catch(const std::exception& error){std::printf("FAILED after %u checks: %s\n",checks,error.what());return 1;}
