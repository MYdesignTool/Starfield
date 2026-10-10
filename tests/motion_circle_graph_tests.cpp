#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/MotionCircle.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <cmath>
#include <atomic>
#include <cstdlib>
#include <new>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <numbers>
#include <stdexcept>
#include <thread>
namespace { std::atomic<std::size_t> allocations{},fail_allocation{}; }
void* operator new(std::size_t size){const auto count=++allocations;if(fail_allocation==count)throw std::bad_alloc{};
    if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};NeverCancelled never;constexpr auto pi=std::numbers::pi;
void check(bool value,const char* label){++checks;if(!value)throw std::runtime_error(label);}
void near(double a,double b,const char* label,double tolerance=1e-9){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance,label);}
void near(Vec3 a,Vec3 b,const char* label,double tolerance=1e-9){near(a.x,b.x,label,tolerance);near(a.y,b.y,label,tolerance);near(a.z,b.z,label,tolerance);}
template<class T>T take(Result<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
template<class T>T take(SequenceResult<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
NodeId nid(unsigned n){NodeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
EdgeId eid(unsigned n){EdgeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
GraphNode& node(Graph& g,unsigned id){for(auto& n:g.nodes)if(n.id==nid(id))return n;throw std::runtime_error("node missing");}
void set(GraphNode& n,ParameterKey key,ParameterValue value){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(value);return;}n.parameters.push_back({key,std::move(value)});}
Graph base(){Settings s;s.birth_rate=2;s.particle_count=100;s.particle_lifetime_seconds=2;s.emission_speed=0;
    s.velocity={};s.velocity_spread=0;s.emitter_origin={.25,0,0};s.particle_size=6;s.opacity=1;
    return take(make_emitter_particle_output_graph(s,nid(1),nid(2),nid(255),eid(1),eid(2)));}
GraphNode circle(unsigned id,double rate=pi/2,Vec3 origin={}){return {nid(id),kMotionNode,1,
    {{kMotionMode,std::uint32_t{1}},{kMotionOrigin,origin},{kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,rate}}};}
void attach(Graph& g,unsigned id=3,double rate=pi/2,Vec3 origin={}) {
    for(auto& e:g.edges)if(e.destination_node==nid(255)){e.destination_node=nid(id);e.destination_port=kMotionParticlesIn;}
    g.nodes.push_back(circle(id,rate,origin));g.edges.push_back({eid(id+10),nid(id),kMotionParticlesOut,nid(255),kOutputParticles});
}
GraphNode transform(unsigned id,Vec3 position={},Vec3 scale={100,100,100}){return {nid(id),kTransformNode,1,
    {{kTransformAnchor,Vec3{}},{kTransformPosition,position},{kTransformRotation,Vec3{}},{kTransformSystemScale,scale},
     {kTransformParticleScale,100.},{kTransformParticleOpacity,100.}}};}
EvaluatedGraph evaluate(const Graph& g,RationalTime t={1,1}){return take(evaluate_particle_graph(g,t,never));}
struct Sampler:TemporalGraphSampler {
    Graph graph;std::function<void(GraphNode&,double)> change;bool proof{};bool fail_circle{},bad_identity{};
    std::map<std::pair<NodeId,double>,unsigned> calls;
    explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double t)override {
        for(const auto& n:graph.nodes)if(n.id==id){auto copy=n;
            if(n.type_key==kMotionNode){++calls[{id,t}];if(fail_circle)return Result<GraphNode>::failure(ErrorCode::internal_failure,"fixture history error");if(bad_identity)copy.id=nid(400);}
            if(change)change(copy,t);return Result<GraphNode>::success(std::move(copy));}
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"fixture missing node");
    }
    Result<double> rate(NodeId id,double t)override {auto n=node(id,t);if(!n.has_value())return Result<double>::failure(n.error());
        for(auto& p:n.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));
        return Result<double>::failure(ErrorCode::invalid_request,"fixture missing rate");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override {auto r=rate(id,0);if(!r.has_value())return Result<std::optional<EmissionRateProfile>>::failure(r.error());
        EmissionRateProfile p;p.constant=r.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(p));}
    std::optional<MotionCircleSettings> constant_motion_circle(NodeId id)override {
        if(!proof)return {};for(auto& n:graph.nodes)if(n.id==id){MotionCircleSettings s;
            for(auto& p:n.parameters){if(p.key==kMotionOrigin)s.origin=std::get<Vec3>(p.value);if(p.key==kMotionAxis)s.axis=std::get<Vec3>(p.value);
                if(p.key==kMotionAngularRate)s.radians_per_second=std::get<double>(p.value);if(p.key==kMotionSpeedRandom)s.speed_random_percent=std::get<double>(p.value);
                if(p.key==kMotionOverLife)check(decode_age_curve(std::get<OpaqueBytes>(p.value),s.over_life,0,1000),"metadata curve");}
            return s;}return {};
    }
};
EvaluatedGraph temporal(Sampler& s,RationalTime t={1,1}){return take(evaluate_temporal_particle_graph(s.graph,t,never,{},s));}
void same(const EvaluatedGraph& a,const EvaluatedGraph& b,double tolerance=1e-9) {
    check(a.particles.size()==b.particles.size(),"population equal");check(a.sprite_bases==b.sprite_bases,"sprite basis preserved");
    for(std::size_t i=0;i<a.particles.size();++i){const auto& p=a.particles[i];const auto& q=b.particles[i];
        check(p.id==q.id&&p.emitter_id==q.emitter_id,"identities equal");near(p.position,q.position,"positions equal",tolerance);
        near(p.velocity,q.velocity,"velocities equal",tolerance);near(p.rotation_degrees,q.rotation_degrees,"authored orientation equal");
        check(p.size_pixels==q.size_pixels&&p.opacity==q.opacity&&p.shape==q.shape&&p.transfer_mode==q.transfer_mode,"style unchanged");}
}
void curve_math() {
    MotionCircleSettings s;s.radians_per_second=pi/2;auto c=take(compile_motion_circle(s));
    for(double age:{0.,.01,.5,1.,2.}){auto t=take(c.clock(age,2));near(t.integral_seconds,age,"default clock integral");near(t.weight,1,"default clock weight");}
    s.over_life.count=2;s.over_life.points[0]={0,0};s.over_life.points[1]={1,100};
    for(auto mode:{CurveInterpolation::linear,CurveInterpolation::draw,CurveInterpolation::bezier}) {
        s.over_life.interpolation=mode;c=take(compile_motion_circle(s));
        for(unsigned i=0;i<=100;++i){const auto age=i*.02;auto t=take(c.clock(age,2));near(t.integral_seconds,age*age/4,"ramp exact integral");near(t.weight,age/2,"ramp weight");}
    }
    s.over_life.count=3;s.over_life.points[0]={0,25};s.over_life.points[1]={.25,75};s.over_life.points[2]={1,50};s.over_life.interpolation=CurveInterpolation::hold;
    c=take(compile_motion_circle(s));near(take(c.clock(.5,2)).integral_seconds,.125,"Hold left integral");near(take(c.clock(.5,2)).weight,.75,"Hold exact point convention");
    near(take(c.clock(2,2)).integral_seconds,1.25,"Hold exact complete integral");near(take(c.clock(2,2)).weight,.5,"Hold final weight");
    s.over_life.interpolation=CurveInterpolation::bezier;c=take(compile_motion_circle(s));
    const auto numerical=[&](double end){double sum=0;constexpr unsigned slices=20000;
        for(unsigned i=0;i<slices;++i)sum+=evaluate_age_curve(s.over_life,(i+.5)*end/(2*slices),100,100)/100;
        return sum*end/slices;};
    for(double age:{.1,.5,.9,1.7,2.})near(take(c.clock(age,2)).integral_seconds,numerical(age),"Bezier independent quadrature",2e-9);
    const auto h=1e-5;auto p=take(c.apply({.25,0,0},{0,.1,0},pi/2,2));near(p.position,Vec3{0,.25,0},"quarter circle");near(p.velocity,Vec3{-.6,0,0},"orbit plus rotated velocity");
    const Vec3 at{.25,0,0},v{0,.1,0};
    auto before=take(c.apply({at.x-v.x*h,at.y-v.y*h,at.z-v.z*h},v,pi/2-2*h,2));
    auto after=take(c.apply({at.x+v.x*h,at.y+v.y*h,at.z+v.z*h},v,pi/2+2*h,2));
    near({(after.position.x-before.position.x)/(2*h),(after.position.y-before.position.y)/(2*h),(after.position.z-before.position.z)/(2*h)},p.velocity,"velocity derivative",1e-9);
    check(!c.clock(-1,2).has_value()&&!c.clock(1,0).has_value()&&!c.clock(3,2).has_value(),"clock rejects invalid age/lifetime");
    s.axis={};check(!compile_motion_circle(s).has_value(),"zero axis rejected");s.axis={1e-320,1e-320,1e-320};check(compile_motion_circle(s).has_value(),"subnormal axis accepted");
    s.radians_per_second=std::numeric_limits<double>::infinity();check(!compile_motion_circle(s).has_value(),"infinite rate rejected");
    check(!c.apply(at,v,1e13,0).has_value()&&!c.apply(at,v,0,1e8).has_value(),"large clocks rejected");
}
void graph_and_history() {
    auto original=base();auto g=original;attach(g);const auto bytes=take(serialize_graph(g,particle_node_registry()));
    auto result=evaluate(g);check(result.particles.size()==3,"normal population");
    near(result.particles[0].position,Vec3{0,.25,0},"oldest quarter orbit");near(result.particles[0].velocity,Vec3{-pi/8,0,0},"oldest instantaneous velocity");
    near(result.particles[2].position,Vec3{.25,0,0},"newborn unchanged position");near(result.particles[2].velocity,Vec3{0,pi/8,0},"newborn instantaneous orbit velocity");
    check(take(serialize_graph(g,particle_node_registry()))==bytes,"author graph immutable");
    same(result,evaluate(take(deserialize_graph(bytes,particle_node_registry()))));
    same(result,take(decode_evaluated_particles(take(encode_evaluated_particles(result,{1,1})),{1,1})));
    Sampler s(g);same(result,temporal(s));for(auto [key,count]:s.calls)check(count==1,"historical Circle samples reused by node/time");
    s.proof=true;s.calls.clear();same(result,temporal(s));check(s.calls.empty(),"metadata exact clock avoids history calls");
    set(node(g,3),kMotionAngularRate,0.);same(evaluate(original),evaluate(g));
    set(node(g,3),kMotionAngularRate,pi/2);set(node(g,3),kMotionSpeedRandom,80.);auto random=evaluate(g);
    same(random,evaluate(g));same(random,evaluate(g,{1,1}));
    (void)evaluate(g,{1,4});same(random,evaluate(g));
    Sampler r(g);r.proof=true;same(random,temporal(r));
    Sampler animated(original);attach(animated.graph);animated.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode)set(n,kMotionAngularRate,t);};
    auto moved=temporal(animated);near(moved.particles[0].position,Vec3{.25*std::cos(.5),.25*std::sin(.5),0},"animated rate integrated not endpoint multiplied");
    near(moved.particles[0].velocity,Vec3{-.25*std::sin(.5),.25*std::cos(.5),0},"animated endpoint velocity");
    animated.fail_circle=true;check(!evaluate_temporal_particle_graph(animated.graph,{1,1},never,{},animated).has_value(),"history error propagated");
    animated.fail_circle=false;animated.bad_identity=true;check(!evaluate_temporal_particle_graph(animated.graph,{1,1},never,{},animated).has_value(),"history identity guarded");
    for(unsigned mode:{0,2,9}){auto bad=g;set(node(bad,3),kMotionMode,std::uint32_t{mode});check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"unimplemented mode rejected");}
    auto bad=g;set(node(bad,3),kMotionAxis,Vec3{});check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"invalid axis rejected");
    set(node(g,1),kEmittingMode,std::uint32_t{1});Sampler once(g);once.proof=true;same(evaluate(g),temporal(once));
    check(evaluate(g,{-1,1}).particles.empty(),"negative exposure empty");
}
void topology_force_transform() {
    auto g=base();attach(g);g.edges[1].destination_node=nid(4);g.edges[1].destination_port=kForceParticlesIn;
    g.nodes.push_back({nid(4),kForceNode,3,{{kGravity,Vec3{0,-.2,0}},{kLinearDrag,0.}}});g.edges.push_back({eid(5),nid(4),kForceParticlesOut,nid(3),kMotionParticlesIn});
    auto forced=evaluate(g);near(forced.particles[0].position,Vec3{.1,.25,0},"Force before Circle");
    near(forced.particles[0].velocity,Vec3{.2-pi/8,pi*.05,0},"Force velocity plus orbit");Sampler fs(g);same(forced,temporal(fs));
    auto tf=base();attach(tf);tf.edges[1].destination_node=nid(4);tf.edges[1].destination_port=kTransformParticlesIn;
    tf.nodes.push_back(transform(4,{.1,0,0},{200,50,-100}));tf.edges.push_back({eid(5),nid(4),kTransformParticlesOut,nid(3),kMotionParticlesIn});
    auto transformed=evaluate(tf);near(transformed.particles[0].position,Vec3{0,.6,0},"Transform then Circle world frame");
    Sampler ts(tf);same(transformed,temporal(ts));
    auto after=base();attach(after);after.edges.back().destination_node=nid(4);after.edges.back().destination_port=kTransformParticlesIn;
    after.nodes.push_back(transform(4));after.edges.push_back({eid(5),nid(4),kTransformParticlesOut,nid(255),kOutputParticles});
    check(!evaluate_particle_graph(after,{1,1},never).has_value(),"unsupported downstream Transform rejected");Sampler as(after);check(!evaluate_temporal_particle_graph(after,{1,1},never,{},as).has_value(),"temporal downstream Transform rejected");
    auto serial=base();attach(serial);attach(serial,5,-pi/2,{.1,0,0});
    near(evaluate(serial).particles[0].position,Vec3{.35,.1,0},"serial Circle order retained");Sampler ss(serial);same(evaluate(serial),temporal(ss));
    auto ambiguous=base();attach(ambiguous);ambiguous.edges.push_back({eid(99),nid(2),kParticleParticlesOut,nid(255),kOutputParticles});
    check(!evaluate_particle_graph(ambiguous,{1,1},never).has_value(),"Motion versus plain branch merge rejected");
    Sampler am(ambiguous);check(!evaluate_temporal_particle_graph(ambiguous,{1,1},never,{},am).has_value(),"temporal ambiguous merge rejected");
}
void auxiliary_and_render() {
    auto g=base();attach(g);auto emitter=node(g,1);emitter.id=nid(4);set(emitter,kAuxiliarySource,std::uint32_t{1});
    set(emitter,kBirthRate,2.);set(emitter,kEmitterOrigin,Vec3{});set(emitter,kInheritVelocity,100.);
    auto particle=node(g,2);particle.id=nid(5);set(particle,kParticleLifetimeSeconds,3.);
    g.edges.back().destination_node=nid(4);g.edges.back().destination_port=kEmitterParents;
    g.nodes.push_back(emitter);g.nodes.push_back(particle);g.edges.push_back({eid(20),nid(4),kEmitterParticles,nid(5),kParticleParticlesIn});
    g.edges.push_back({eid(21),nid(5),kParticleParticlesOut,nid(255),kOutputParticles});
    auto children=evaluate(g);Sampler s(g);s.proof=true;same(children,temporal(s));
    auto found=std::find_if(children.particles.begin(),children.particles.end(),[](const auto& p){return std::abs(p.age_seconds-.5)<1e-9;});
    check(found!=children.particles.end(),"child born after parent orbit");
    const auto a=pi/4;near(found->position,Vec3{.25*std::cos(a)-.5*pi/8*std::sin(a),.25*std::sin(a)+.5*pi/8*std::cos(a),0},"parent at birth and velocity inherited");
    auto draw=base();set(node(draw,1),kBirthRate,1.);set(node(draw,1),kEmittingMode,std::uint32_t{1});attach(draw);
    RenderRequest request;request.graph=std::make_shared<const Graph>(draw);request.frame.layer_width=request.frame.layer_height=request.frame.frame_width=request.frame.frame_height=64;
    request.frame.region_of_interest={0,0,64,64};request.frame.time={1,1};request.frame.frame_duration={1,60};request.frame.format=PixelFormat::rgba32f;
    const auto rendered=take(CpuParticleRenderer{}.render(request,never));
    const auto alpha=[&](unsigned x,unsigned y){float value;std::memcpy(&value,rendered.pixels.data()+y*rendered.row_bytes+x*16+12,4);return value;};
    check(alpha(32,16)>.9f,"CPU draws actual orbit position");check(alpha(48,32)==0,"CPU no longer draws original position");
    auto first=evaluate(draw,{0,1}),last=evaluate(draw,{1,1});
    auto linear=take(interpolate_motion_particles(first,last,0,1,.5,100,never));check(linear.particles.size()==1,"Linear shutter identity survives Circle");
    near(linear.particles[0].position,Vec3{.125,.125,0},"Linear shutter documented chord approximation");
    near(evaluate(draw,{1,2}).particles[0].position,Vec3{.25/std::sqrt(2.),.25/std::sqrt(2.),0},"Subframe samples actual circle");
}
void failures_and_limits() {
    auto g=base();attach(g);auto original=take(serialize_graph(g,particle_node_registry()));
    struct Interrupt:Cancellation {mutable unsigned calls{};unsigned after{};
        explicit Interrupt(unsigned n):after(n){}bool is_cancelled()const noexcept override{return calls++>=after;}};
    bool finished=false;
    for(unsigned n=0;n<400;++n){Interrupt stop(n);auto result=evaluate_particle_graph(g,{1,1},stop);
        if(result.has_value()){finished=true;break;}check(result.error().code==ErrorCode::cancelled,"static cancellation typed");}
    check(finished,"static cancellation tested through success");finished=false;
    for(unsigned n=0;n<600;++n){Sampler s(g);Interrupt stop(n);auto result=evaluate_temporal_particle_graph(g,{1,1},stop,{},s);
        if(result.has_value()){finished=true;break;}check(result.error().code==ErrorCode::cancelled,"history cancellation typed");}
    check(finished,"history cancellation tested through success");
    const auto failures=[&](bool history){bool success=false;
        for(unsigned n=1;n<2000;++n){Sampler s(g);const auto before=allocations.load();fail_allocation=before+n;
            auto result=history?evaluate_temporal_particle_graph(g,{1,1},never,{},s):evaluate_particle_graph(g,{1,1},never);
            fail_allocation=0;if(result.has_value()){success=true;break;}
            if(result.error().code!=ErrorCode::allocation_failed)std::fprintf(stderr,"allocation fault %u history=%d: %s\n",n,history,result.error().detail);
            check(result.error().code==ErrorCode::allocation_failed,"each allocation failure typed");}
        check(success,"allocation scan reaches successful request");};
    failures(false);failures(true);check(take(serialize_graph(g,particle_node_registry()))==original,"failures leave graph unchanged");
    auto bad=g;set(node(bad,3),kMotionSpeedRandom,100.01);check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"random percent bounds");
    MotionCircleSettings s;s.over_life.count=3;s.over_life.interpolation=CurveInterpolation::bezier;
    s.over_life.points[0]={0,0};s.over_life.points[1]={std::numeric_limits<double>::denorm_min(),100};s.over_life.points[2]={1,100};
    check(!compile_motion_circle(s).has_value(),"nonfinite Bezier coefficients rejected");
    const auto reference=evaluate(g);std::atomic<bool> good{true};std::array<std::thread,4> workers;
    for(auto& worker:workers)worker=std::thread([&](){for(unsigned i=0;i<20;++i){auto r=evaluate_particle_graph(g,{1,1},never);
        if(!r.has_value()||r.value().particles[0].position.y!=reference.particles[0].position.y)good=false;}});
    for(auto& worker:workers)worker.join();check(good,"concurrent graph requests deterministic");
}
}
int main()try{curve_math();graph_and_history();topology_force_transform();auxiliary_and_render();failures_and_limits();
    std::printf("motion_circle_graph_tests: %u checks passed; graph/history/codec/CPU/Circle only, no AE qualification\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"motion_circle_graph_tests: after %u checks: %s\n",checks,e.what());return 1;}
