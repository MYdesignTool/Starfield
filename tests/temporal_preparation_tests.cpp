#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
template<class T>T get(Result<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
Uuid128 uid(unsigned char n){Uuid128 v{};v.bytes[15]=n;return v;}
void set(GraphNode& n,ParameterKey key,ParameterValue value){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(value);return;}n.parameters.push_back({key,std::move(value)});}
Graph fixture(unsigned mode,unsigned count=120) {
    Settings s;s.particle_count=count;s.birth_rate=count/2.;s.particle_lifetime_seconds=4;
    s.velocity={.1,.2,-.03};s.velocity_spread=.1;s.gravity={.2,-.3,.1};s.linear_drag=.4;
    s.appearance_enabled=true;s.particle_size=7;s.particle_size_end=40;s.color_start={.3,.8,1};s.color_end={1,.2,.4};
    auto g=get(make_emitter_particle_force_output_graph(s,NodeId{uid(1)},NodeId{uid(2)},NodeId{uid(3)},NodeId{uid(4)},EdgeId{uid(11)},EdgeId{uid(12)},EdgeId{uid(13)}));
    if(mode==5)for(auto& n:g.nodes)if(n.type_key==kForceNode) {
        set(n,kSpin,2.);set(n,kSpinFrequency,.7);set(n,kWind,Vec3{.1,-.05,.02});
    }
    if(mode==1) { // Parallel Force routes deliberately keep historical integration.
        auto f=*std::find_if(g.nodes.begin(),g.nodes.end(),[](const auto& n){return n.type_key==kForceNode;});f.id=NodeId{uid(5)};set(f,kGravity,Vec3{-.2,.1,.3});g.nodes.push_back(f);
        g.edges.push_back({EdgeId{uid(14)},NodeId{uid(2)},kParticleParticlesOut,f.id,kForceParticlesIn});
        g.edges.push_back({EdgeId{uid(15)},f.id,kForceParticlesOut,NodeId{uid(4)},kOutputParticles});
    }
    if(mode==2 || mode==3 || mode==4) {
        GraphNode processor;processor.id=NodeId{uid(5)};processor.schema_version=1;
        PortKey in,out;
        if(mode==2) {
            processor.type_key=kTransformNode;
            processor.parameters={{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.2,.3,.1}},
                {kTransformRotation,Vec3{10,20,30}},{kTransformSystemScale,Vec3{120,120,120}},
                {kTransformParticleScale,80.},{kTransformParticleOpacity,70.}};
            in=kTransformParticlesIn;out=kTransformParticlesOut;
        } else if(mode==3) {
            processor.type_key=kTurbulenceNode;
            processor.parameters={{kTurbulenceOrigin,Vec3{}},{kTurbulenceAmplitude,.2},{kTurbulenceFrequency,1.7},
                {kTurbulenceLevels,std::uint32_t{2}},{kTurbulenceEvolutionSpeed,.2},{kTurbulenceOffset,Vec3{.31,-.27,.13}}};
            in=kTurbulenceParticlesIn;out=kTurbulenceParticlesOut;
        } else {
            processor.type_key=kMotionNode;
            processor.parameters={{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{.2,.1,0}},
                {kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,.7}};
            in=kMotionParticlesIn;out=kMotionParticlesOut;
        }
        g.edges.back().destination_node=processor.id;g.edges.back().destination_port=in;
        g.edges.push_back({EdgeId{uid(14)},processor.id,out,NodeId{uid(4)},kOutputParticles});g.nodes.push_back(processor);
    }
    return g;
}
struct Sampler final:TemporalGraphSampler {
    const Graph& graph;bool certify{};unsigned dynamic{};unsigned node_calls{},proof_calls{};bool bad_identity{};
    Sampler(const Graph& g,bool proof,unsigned animated=0):graph(g),certify(proof),dynamic(animated){}
    GraphNode sample(NodeId id,double time) {
        const auto it=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==id;});
        if(it==graph.nodes.end())throw std::runtime_error("missing fixture node");
        auto n=*it;
        if(id==NodeId{uid(static_cast<unsigned char>(dynamic))}) {
            if(n.type_key==kEmitterNode)set(n,kEmitterOrigin,Vec3{time*.03,0,0});
            if(n.type_key==kParticleNode)set(n,kSizeStart,5.+time);
            if(n.type_key==kForceNode)set(n,kGravity,Vec3{.2,-.3+time*.1,.1});
            if(n.type_key==kTransformNode)set(n,kTransformPosition,Vec3{time*.02,.3,.1});
            if(n.type_key==kTurbulenceNode)set(n,kTurbulenceAmplitude,.2+time*.01);
            if(n.type_key==kMotionNode)set(n,kMotionAngularRate,.7+time*.02);
        }
        return n;
    }
    Result<GraphNode> node(NodeId id,double time)override{++node_calls;return Result<GraphNode>::success(sample(id,time));}
    std::optional<GraphNode> constant_node(NodeId id)override {
        ++proof_calls;if(!certify || id==NodeId{uid(static_cast<unsigned char>(dynamic))})return {};
        auto n=sample(id,0);if(bad_identity)n.id=NodeId{uid(99)};return n;
    }
    Result<double> rate(NodeId id,double)override{for(const auto& p:sample(id,0).parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));throw std::runtime_error("missing rate");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override{EmissionRateProfile p;p.constant=get(rate(id,0));return Result<std::optional<EmissionRateProfile>>::success(p);}
    Result<double> lifetime(NodeId,double)override{return Result<double>::success(4);}
    std::optional<double> lifetime_upper_bound(NodeId)override{return 4.;}
};
const NeverCancelled never;
EvaluatedGraph run(const Graph& g,RationalTime time,Sampler& sampler){return get(evaluate_temporal_particle_graph(g,time,never,{1080,1.5},sampler));}
void matrix() {
    for(unsigned mode:{0u,1u,2u,3u,4u,5u})for(unsigned hz:{30u,60u,120u})for(unsigned dynamic:{0u,1u,2u,3u,5u}) {
        auto g=fixture(mode);for(auto& n:g.nodes)if(n.type_key==kOutputNode)set(n,kTimeSamplingHz,hz);
        Sampler reference(g,false,dynamic),cached(g,true,dynamic);
        // Reusing the caller at different/subframe/jumped times must not retain
        // compiled values or particles from a previous evaluation request.
        for(int tick:{-1,0,1,17,48,51,2400,49}) {
            RationalTime t{tick,24};const auto a=run(g,t,reference),b=run(g,t,cached);
            check(get(encode_evaluated_particles(a,t))==get(encode_evaluated_particles(b,t)),"compiled history differs from original algorithm");
        }
    }
    auto g=fixture(2);Sampler a(g,false),b(g,true);run(g,{2,1},a);run(g,{2,1},b);
    check(a.node_calls>100 && b.node_calls==0,"constant compilation must eliminate per-birth sampling");
    check(b.proof_calls<=g.nodes.size(),"constant capability queried at most once per node/request");
    Sampler wrong(g,true);wrong.bad_identity=true;
    check(!evaluate_temporal_particle_graph(g,{2,1},never,{},wrong).has_value(),"reject wrong certified identity");
    struct Cancelled final:Cancellation{bool is_cancelled()const noexcept override{return true;}} cancelled;
    check(!evaluate_temporal_particle_graph(g,{2,1},cancelled,{},b).has_value(),"cached evaluation still cancels");
    // Same sampler object, changed authored values: cache lifetime is a request.
    const auto first=run(g,{2,1},b);set(g.nodes[0],kEmitterOrigin,Vec3{10,0,0});
    const auto next=run(g,{2,1},b);
    check(get(encode_evaluated_particles(first,{2,1}))!=get(encode_evaluated_particles(next,{2,1})),"next request must use new authored values");
}
void benchmark() {
    std::puts("mode,particles,proof,median_ms,node_samples");
    for(unsigned mode:{1u,2u,3u,4u,5u})for(unsigned count:{30000u,50000u})for(bool proof:{false,true}) {
        auto g=fixture(mode,count);double ms[3]{};unsigned calls=0;
        for(int i=0;i<3;++i){Sampler sampler(g,proof);const auto start=std::chrono::steady_clock::now();auto result=run(g,{48+i,24},sampler);
            check(result.particles.size()>=count-1,"benchmark population");calls=sampler.node_calls;
            ms[i]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();}
        std::sort(ms,ms+3);std::printf("%u,%u,%u,%.4f,%u\n",mode,count,unsigned(proof),ms[1],calls);
    }
}
}
int main()try{matrix();if(std::getenv("STARFIELD_TEST_PREP_BENCHMARK"))benchmark();std::printf("Temporal preparation checks: %u\n",checks);return 0;}
catch(const std::exception& e){std::fprintf(stderr,"%s\n",e.what());return 1;}
