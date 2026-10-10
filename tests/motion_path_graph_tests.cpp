#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/MotionPathPoints.hpp"
#include "starfield/core/MotionPathTravel.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <functional>
#include <limits>
#include <map>
#include <new>
#include <stdexcept>
#include <thread>
namespace {std::atomic<std::size_t> allocations{},fail_allocation{};}
void* operator new(std::size_t size){const auto n=++allocations;if(n==fail_allocation)throw std::bad_alloc{};
    if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield::core;using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};NeverCancelled never;
void check(bool v,const char* label){++checks;if(!v)throw std::runtime_error(label);}
void near(double a,double b,const char* label,double tolerance=1e-9){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance,label);}
void near(Vec3 a,Vec3 b,const char* label,double tolerance=1e-9){near(a.x,b.x,label,tolerance);near(a.y,b.y,label,tolerance);near(a.z,b.z,label,tolerance);}
bool exact(Vec3 a,Vec3 b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
template<class T>T take(Result<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
template<class T>T take(SequenceResult<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
NodeId nid(unsigned n){NodeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
EdgeId eid(unsigned n){EdgeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
GraphNode& node(Graph& g,unsigned id){for(auto& n:g.nodes)if(n.id==nid(id))return n;throw std::runtime_error("node missing");}
void set(GraphNode& n,ParameterKey key,ParameterValue v){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(v);return;}n.parameters.push_back({key,std::move(v)});}
OpaqueBytes points(std::initializer_list<Vec3> values){return take(encode_motion_path_points({values.begin(),values.size()},never));}
Graph base(){Settings s;s.birth_rate=2;s.particle_count=100;s.particle_lifetime_seconds=3;s.emission_speed=0;
    s.velocity={.1,-.2,.05};s.velocity_spread=0;s.emitter_origin={.25,.1,-.1};s.particle_size=6;s.opacity=.8;
    return take(make_emitter_particle_output_graph(s,nid(1),nid(2),nid(255),eid(1),eid(2)));}
GraphNode path(unsigned id=3,double speed=.4){return {nid(id),kMotionNode,1,{{kMotionMode,std::uint32_t{0}},
    {kMotionPathPoints,points({{5,7,9},{5,17,9}})},{kMotionPathSpeed,speed}}};}
void attach(Graph& g,GraphNode n){const auto id=n.id;for(auto& e:g.edges)if(e.destination_node==nid(255)){e.destination_node=id;e.destination_port=kMotionParticlesIn;}
    g.nodes.push_back(std::move(n));g.edges.push_back({eid(unsigned(g.edges.size()+10)),id,kMotionParticlesOut,nid(255),kOutputParticles});}
EvaluatedGraph evaluate(const Graph& g,RationalTime t={1,1}){return take(evaluate_particle_graph(g,t,never));}
struct Sampler:TemporalGraphSampler {
    Graph graph;std::function<void(GraphNode&,double)> change;bool proof{},fail{},bad_id{};unsigned proof_calls{};
    std::map<std::pair<NodeId,double>,unsigned> calls;
    explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double t)override {for(const auto& n:graph.nodes)if(n.id==id){auto copy=n;
        if(n.type_key==kMotionNode){++calls[{id,t}];if(fail)return Result<GraphNode>::failure(ErrorCode::internal_failure,"history fixture error");if(bad_id)copy.id=nid(400);}
        if(change)change(copy,t);return Result<GraphNode>::success(std::move(copy));}
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"fixture node missing");}
    Result<double> rate(NodeId id,double t)override {auto n=node(id,t);if(!n.has_value())return Result<double>::failure(n.error());
        for(auto& p:n.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));
        return Result<double>::failure(ErrorCode::invalid_request,"fixture rate missing");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override {auto r=rate(id,0);if(!r.has_value())return Result<std::optional<EmissionRateProfile>>::failure(r.error());
        EmissionRateProfile p;p.constant=r.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(p));}
    std::optional<MotionPathTravelSettings> constant_motion_path_clock(NodeId id)override {
        ++proof_calls;if(!proof)return {};for(auto& n:graph.nodes)if(n.id==id){MotionPathTravelSettings s;
            for(auto& p:n.parameters){if(p.key==kMotionPathSpeed)s.units_per_second=std::get<double>(p.value);
                if(p.key==kMotionPathDelay)s.delay_seconds=std::get<double>(p.value);if(p.key==kMotionSpeedRandom)s.speed_random_percent=std::get<double>(p.value);
                if(p.key==kMotionOverLife)check(decode_age_curve(std::get<OpaqueBytes>(p.value),s.over_life,0,1000),"proof curve");}
            return s;}return {};}
};
EvaluatedGraph temporal(Sampler& s,RationalTime t={1,1}){return take(evaluate_temporal_particle_graph(s.graph,t,never,{},s));}
void same(const EvaluatedGraph& a,const EvaluatedGraph& b,double tol=1e-9){check(a.particles.size()==b.particles.size(),"population equal");
    check(a.sprite_bases==b.sprite_bases,"basis equal");for(unsigned i=0;i<a.particles.size();++i){const auto& p=a.particles[i];const auto& q=b.particles[i];
        near(p.position,q.position,"position equal",tol);near(p.velocity,q.velocity,"velocity equal",tol);near(p.rotation_degrees,q.rotation_degrees,"Euler equal");
        check(p.id==q.id&&p.emitter_id==q.emitter_id&&p.shape==q.shape&&p.size_pixels==q.size_pixels&&p.opacity==q.opacity&&p.transfer_mode==q.transfer_mode,"identity/style equal");
        for(unsigned j=0;j<4;++j)near(p.motion_pose[j],q.motion_pose[j],"pose equal",tol);}}
void packet(){std::vector<Vec3> p(256);for(unsigned i=0;i<p.size();++i)p[i]={double(i),-double(i),i*.25};
    auto bytes=take(encode_motion_path_points(p,never));check(bytes.size()==6160,"packet max exact length");auto decoded=take(decode_motion_path_points(bytes,never));
    check(decoded.size()==p.size(),"packet population");for(unsigned i=0;i<p.size();++i)check(exact(decoded[i],p[i]),"packet roundtrip");
    for(std::size_t n=0;n<bytes.size();++n)check(!validate_motion_path_points({bytes.data(),n},never).has_value(),"every truncation rejects");
    for(auto at:{0u,4u,6u,8u,9u,12u}){auto bad=bytes;bad[at]^=std::byte{255};check(!validate_motion_path_points(bad,never).has_value(),"bad header rejects");}
    auto bad=bytes;motion_path_points_detail::write(bad,16,std::bit_cast<std::uint64_t>(std::numeric_limits<double>::infinity()),8);
    const auto before=allocations.load();check(!decode_motion_path_points(bad,never).has_value(),"bad numeric packet rejects");check(allocations==before,"reject before allocation");
    fail_allocation=allocations.load()+1;auto fail_decode=decode_motion_path_points(bytes,never);fail_allocation=0;
    check(!fail_decode.has_value()&&fail_decode.error().code==ErrorCode::allocation_failed,"decoder allocation typed");
    check(!encode_motion_path_points({},never).has_value(),"empty packet rejects");p.push_back({});check(!encode_motion_path_points(p,never).has_value(),"257 points rejects");
}
void graph(){auto plain=base(),g=plain;attach(g,path());auto old=evaluate(plain),r=evaluate(g);check(r.particles.size()==old.particles.size(),"distribution population retained");
    for(unsigned i=0;i<r.particles.size();++i){const auto& p=old.particles[i];const auto& q=r.particles[i];near(q.position,Vec3{p.position.x,p.position.y+.4*p.age_seconds,p.position.z},"relative path displacement");
        near(q.velocity,Vec3{p.velocity.x,p.velocity.y+.4,p.velocity.z},"instant path velocity");check(p.id==q.id&&p.emitter_id==q.emitter_id&&p.motion_pose==q.motion_pose,"identity/default orientation retained");}
    same(r,evaluate(take(deserialize_graph(take(serialize_graph(g,particle_node_registry())),particle_node_registry()))));
    Sampler s(g);same(r,temporal(s));check(s.proof_calls==1,"metadata queried once");
    for(auto [key,count]:s.calls)check(count==1,"one numeric node sample per time");
    Sampler proved(g);proved.proof=true;same(r,temporal(proved));check(proved.calls.size()==1,"constant clock removes midpoint sampling");
    set(node(g,3),kMotionPathDelay,.5);r=evaluate(g);for(unsigned i=0;i<r.particles.size();++i){auto age=old.particles[i].age_seconds;
        near(r.particles[i].position.y,old.particles[i].position.y+.4*std::max(0.,age-.5),"delay displacement");
        near(r.particles[i].velocity.y,old.particles[i].velocity.y+(age>=.5?.4:0),"delay derivative");}
    Sampler delayed(g);same(r,temporal(delayed));Sampler fixed(g);fixed.proof=true;same(r,temporal(fixed));
    set(node(g,3),kMotionPathDelay,3.);same(old,evaluate(g));set(node(g,3),kMotionPathDelay,0.);set(node(g,3),kMotionPathSpeed,0.);same(old,evaluate(g));
    set(node(g,3),kMotionPathSpeed,100.);r=evaluate(g);for(unsigned i=0;i<r.particles.size();++i){near(r.particles[i].position.y,old.particles[i].position.y+(old.particles[i].age_seconds>0?10:0),"endpoint clamp");
        if(old.particles[i].age_seconds>0)near(r.particles[i].velocity,old.particles[i].velocity,"clipped derivative zero");}
    set(node(g,3),kMotionPathSpeed,-.4);same(old,evaluate(g));
    set(node(g,3),kMotionPathSpeed,.4);set(node(g,3),kMotionSpeedRandom,70.);r=evaluate(g);Sampler random(g);same(r,temporal(random));
    Sampler reversed(g);auto later=temporal(reversed,{3,2});same(later,evaluate(g,{3,2}));same(r,temporal(reversed,{1,1}));
    check(r.particles.front().position.y-old.particles.front().position.y>.12&&r.particles.front().position.y-old.particles.front().position.y<.4,"stable random attenuation");
    const auto record=take(encode_evaluated_particles(r,{1,1}));same(r,take(decode_evaluated_particles(record,{1,1})));
}
void history(){auto g=base();attach(g,path());Sampler s(g);s.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode)set(n,kMotionPathSpeed,.2+.3*t);};
    auto r=temporal(s),plain=evaluate(base());for(unsigned i=0;i<r.particles.size();++i){const double age=plain.particles[i].age_seconds,birth=1-age;
        near(r.particles[i].position.y,plain.particles[i].position.y+.2*age+.15*(1-birth*birth),"animated linear speed integral");
        near(r.particles[i].velocity.y,plain.particles[i].velocity.y+.5,"animated current speed");}
    set(node(g,3),kMotionPathDelay,.2);Sampler delay(g);delay.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode)set(n,kMotionPathDelay,.2+t);};
    r=temporal(delay);for(unsigned i=0;i<r.particles.size();++i){const double age=plain.particles[i].age_seconds,birth=1-age;
        near(r.particles[i].position.y,plain.particles[i].position.y+.4*std::max(0.,age-.2-birth),"delay captured at birth");}
    set(node(g,3),kMotionPathDelay,0.);Sampler geometry(g);geometry.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode)set(n,kMotionPathPoints,points({{1,2,3},{1+10*t,2,3}}));};
    r=temporal(geometry);for(unsigned i=0;i<r.particles.size();++i)near(r.particles[i].position,Vec3{plain.particles[i].position.x+.4*plain.particles[i].age_seconds,plain.particles[i].position.y,plain.particles[i].position.z},"geometry frozen at request time");
    for(auto interpolation:{CurveInterpolation::linear,CurveInterpolation::hold,CurveInterpolation::bezier,CurveInterpolation::draw}){
        AgeCurve curve;curve.count=3;curve.points[0]={0,20};curve.points[1]={.5,80};curve.points[2]={1,30};curve.interpolation=interpolation;
        set(node(g,3),kMotionOverLife,encode_age_curve(curve));Sampler proof(g);proof.proof=true;same(evaluate(g),temporal(proof));}
    Sampler identity(g);identity.bad_id=true;auto invalid=evaluate_temporal_particle_graph(g,{1,1},never,{},identity);
    check(!invalid.has_value()&&invalid.error().code==ErrorCode::invalid_request,"identity change rejects");
    Sampler error(g);error.fail=true;invalid=evaluate_temporal_particle_graph(g,{1,1},never,{},error);check(!invalid.has_value()&&invalid.error().code==ErrorCode::internal_failure,"sampler error preserved");
    Sampler mode(g);mode.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode&&t<.9)set(n,kMotionMode,std::uint32_t{2});};
    check(!evaluate_temporal_particle_graph(g,{1,1},never,{},mode).has_value(),"historical mode change rejects");
    auto evict=base();attach(evict,path(3,.002));set(node(evict,1),kBirthRate,1.);set(node(evict,1),kEmittingMode,std::uint32_t{1});set(node(evict,2),kParticleLifetimeSeconds,200.);
    Sampler bounded(evict);same(evaluate(evict,{1401,10}),temporal(bounded,{1401,10}));check(bounded.calls.size()>4096,"clock leases survive cache eviction");
    for(unsigned frequency:{30u,60u,120u}){
        auto lattice=base();attach(lattice,path());set(node(lattice,255),kTimeSamplingHz,frequency);set(node(lattice,3),kMotionPathDelay,.123);
        Sampler varying(lattice);varying.change=[](GraphNode& n,double t){if(n.type_key==kMotionNode)set(n,kMotionPathSpeed,.2+.3*t*t);};
        auto value=temporal(varying);for(unsigned i=0;i<value.particles.size();++i){const auto& p=plain.particles[i];double t=1-p.age_seconds+.123,integral=0;
            while(t<1){const auto end=std::min(1.,(std::floor(t*frequency+1e-8)+1)/frequency);const auto mid=(t+end)/2;integral+=(.2+.3*mid*mid)*(end-t);t=end;}
            near(value.particles[i].position.y,p.position.y+integral,"absolute lattice partial birth/delay interval");}
    }
}
void orientation(){auto g=base();attach(g,path());set(node(g,3),kMotionPathOrient,std::uint32_t{1});set(node(g,3),kMotionForward,Vec3{1,0,0});
    set(node(g,2),kParticleShape,std::uint32_t{1});set(node(g,2),kParticleAngles,Vec3{45,30,75});auto r=evaluate(g);
    for(auto& p:r.particles)near(take(particle_motion_forward(p,{1,0,0},r.sprite_bases)),Vec3{0,1,0},"alignment sees final Euler");
    Sampler s(g);same(r,temporal(s));auto record=take(encode_evaluated_particles(r,{1,1}));check(std::to_integer<unsigned>(record[2])==9,"path pose snapshot9");same(r,take(decode_evaluated_particles(record,{1,1})));
    auto tf=g;tf.edges[1].destination_node=nid(4);tf.edges[1].destination_port=kTransformParticlesIn;
    tf.nodes.push_back({nid(4),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.1,.2,.3}},
        {kTransformRotation,Vec3{12,24,35}},{kTransformSystemScale,Vec3{-200,50,125}},{kTransformParticleScale,120.},{kTransformParticleOpacity,50.}}});
    tf.edges.push_back({eid(20),nid(4),kTransformParticlesOut,nid(3),kMotionParticlesIn});r=evaluate(tf);
    for(auto& p:r.particles){auto f=take(particle_motion_forward(p,{1,0,0},r.sprite_bases));const auto n=std::hypot(f.x,f.y,f.z);near({f.x/n,f.y/n,f.z/n},Vec3{0,1,0},"affine/reflection alignment");}
    Sampler transformed(tf);same(r,temporal(transformed));
    auto twice=g;attach(twice,path(5,.3));set(node(twice,5),kMotionPathPoints,points({{0,0,0},{10,0,0}}));set(node(twice,5),kMotionPathOrient,std::uint32_t{1});set(node(twice,5),kMotionForward,Vec3{1,0,0});
    r=evaluate(twice);for(auto& p:r.particles)near(take(particle_motion_forward(p,{1,0,0},r.sprite_bases)),Vec3{1,0,0},"serial paths align in order");
    Sampler serial(twice);same(r,temporal(serial));attach(twice,{nid(6),kMotionNode,1,{{kMotionMode,std::uint32_t{2}},{kMotionGoal,Vec3{2,4,0}},{kMotionForward,Vec3{1,0,0}}}});
    r=evaluate(twice);for(auto& p:r.particles){auto f=take(particle_motion_forward(p,{1,0,0},r.sprite_bases));Vec3 target{2-p.position.x,4-p.position.y,-p.position.z};
        const double length=std::hypot(target.x,target.y,target.z);near(f,Vec3{target.x/length,target.y/length,target.z/length},"Look At follows path position/alignment");}
    Sampler final(twice);same(r,temporal(final));attach(twice,path(7));auto downstream=evaluate(twice);
    for(unsigned i=0;i<r.particles.size();++i){near(downstream.particles[i].position,Vec3{r.particles[i].position.x,r.particles[i].position.y+.4*r.particles[i].age_seconds,r.particles[i].position.z},"path after Look At advances its stage center");
        check(downstream.particles[i].motion_pose==r.particles[i].motion_pose,"unoriented downstream path keeps earlier Look At pose");}
    Sampler followed(twice);same(downstream,temporal(followed));
    auto after=g;attach(after,{nid(6),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{}},{kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,0.}}});
    same(evaluate(g),evaluate(after));Sampler circled(after);same(evaluate(after),temporal(circled));
    MotionPathTravelSettings settings;settings.orient_to_path=true;auto helper=take(compile_motion_path_travel(std::array<Vec3,2>{{{0,0,0},{0,10,0}}},settings,never));
    ParticleInstance p;p.age_seconds=1;p.lifetime_seconds=3;p.rotation_degrees={0,0,60};auto all=p,phased=p;
    take(helper.apply_at_distance(all,1,1));take(helper.apply_position_at_distance(phased,1,1));take(helper.orient_at_distance(phased,1,1));check(all.motion_pose==phased.motion_pose&&exact(all.position,phased.position)&&exact(all.velocity,phased.velocity),"split matches atomic API");
}
void auxiliary(){auto g=base();attach(g,path());set(node(g,1),kBirthRate,1.);set(node(g,1),kEmittingMode,std::uint32_t{1});
    auto child_emitter=node(g,1);child_emitter.id=nid(4);set(child_emitter,kEmittingMode,std::uint32_t{0});set(child_emitter,kAuxiliarySource,std::uint32_t{1});set(child_emitter,kBirthRate,2.);
    set(child_emitter,kInheritVelocity,100.);set(child_emitter,kEmitterOrigin,Vec3{});set(child_emitter,kVelocity,Vec3{});
    auto child_particle=node(g,2);child_particle.id=nid(5);g.nodes.push_back(child_emitter);g.nodes.push_back(child_particle);
    g.edges.push_back({eid(30),nid(3),kMotionParticlesOut,nid(4),kEmitterParents});g.edges.push_back({eid(31),nid(4),kEmitterParticles,nid(5),kParticleParticlesIn});g.edges.push_back({eid(32),nid(5),kParticleParticlesOut,nid(255),kOutputParticles});
    Sampler s(g);auto r=temporal(s);check(r.particles.size()>=3,"auxiliary population");unsigned children=0;
    const auto parent=evaluate(g);same(parent,r);
    for(auto& p:r.particles)if(p.emitter_id==nid(4)){++children;near(p.velocity,Vec3{.1,.2,.05},"parent path instantaneous velocity inherited");
        near(p.position,Vec3{.35,.3,-.05},"child samples parent at birth and follows inherited velocity");}
    check(children>0,"auxiliary child exists");
}
void rendering(){auto g=base();attach(g,path(3,.25));set(node(g,1),kBirthRate,1.);set(node(g,1),kEmittingMode,std::uint32_t{1});
    set(node(g,1),kEmitterOrigin,Vec3{});set(node(g,1),kVelocity,Vec3{});set(node(g,2),kParticleShape,std::uint32_t{1});set(node(g,2),kSizeStart,20.);
    set(node(g,2),kSizeY,4.);
    set(node(g,3),kMotionPathOrient,std::uint32_t{1});set(node(g,3),kMotionForward,Vec3{1,0,0});
    RenderRequest request;request.graph=std::make_shared<const Graph>(g);request.frame.layer_width=request.frame.layer_height=request.frame.frame_width=request.frame.frame_height=64;
    request.frame.region_of_interest={0,0,64,64};request.frame.time={1,1};request.frame.frame_duration={1,60};request.frame.format=PixelFormat::rgba32f;
    auto image=take(CpuParticleRenderer{}.render(request,never));const auto alpha=[&](unsigned x,unsigned y){float v;std::memcpy(&v,image.pixels.data()+y*image.row_bytes+x*16+12,4);return v;};
    check(alpha(32,16)>.7f&&alpha(39,16)==0&&alpha(32,32)==0,"actual CPU moved/aligned path sprite");
    auto scene=take(prepare_sprite_scene(request,never));check(scene.sprites.size()==1,"GPU preparation actual path sprite");near(scene.sprites[0].inverse_ax,0,"prepared rotated long axis",1e-6);
    auto first=evaluate(g,{0,1}),last=evaluate(g,{1,1});auto linear=take(interpolate_motion_particles(first,last,0,1,.5,100,never));
    near(linear.particles[0].position,Vec3{0,.125,0},"linear shutter center");auto subframe=evaluate(g,{1,2});near(subframe.particles[0].position,Vec3{0,.125,0},"actual subframe position");
}
void failures(){auto g=base();attach(g,path());const auto original=take(serialize_graph(g,particle_node_registry()));
    struct Stop:Cancellation{mutable unsigned n{};unsigned limit;explicit Stop(unsigned l):limit(l){}bool is_cancelled()const noexcept override{return n++>=limit;}};
    for(bool history:{false,true}){bool success=false;for(unsigned i=0;i<3000;++i){Sampler s(g);Stop stop(i);auto r=history?evaluate_temporal_particle_graph(g,{1,1},stop,{},s):evaluate_particle_graph(g,{1,1},stop);
        if(r.has_value()){success=true;break;}check(r.error().code==ErrorCode::cancelled,"every cancellation typed");}check(success,"cancellation reaches success");
        success=false;for(unsigned i=1;i<3000;++i){Sampler s(g);fail_allocation=allocations.load()+i;auto r=history?evaluate_temporal_particle_graph(g,{1,1},never,{},s):evaluate_particle_graph(g,{1,1},never);fail_allocation=0;
            if(r.has_value()){success=true;break;}if(r.error().code!=ErrorCode::allocation_failed)std::fprintf(stderr,"allocation %u history %d: %s\n",i,history,r.error().detail);
            check(r.error().code==ErrorCode::allocation_failed,"every allocation typed");}check(success,"allocation scan reaches success");}
    check(original==take(serialize_graph(g,particle_node_registry())),"failures preserve graph");
    for(auto [key,value]:{std::pair{kMotionPathSpeed,1e6+1},std::pair{kMotionPathDelay,-1.},std::pair{kMotionSpeedRandom,101.}}){auto bad=g;set(node(bad,3),key,value);check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"clock bounds reject");}
    auto bad=g;set(node(bad,3),kMotionPathOrient,std::uint32_t{1});check(!evaluate_particle_graph(bad,{1,1},never).has_value(),"alignment needs forward");
    auto parked=base();auto dormant=path();set(dormant,kMotionPathPoints,OpaqueBytes{std::byte{1}});parked.nodes.push_back(std::move(dormant));check(!evaluate_particle_graph(parked,{1,1},never).has_value(),"parked corrupt points reject");
    std::vector<Vec3> dense_points(256);for(unsigned i=0;i<256;++i)dense_points[i]={double(i),0,0};
    const auto dense_packet=take(encode_motion_path_points(dense_points,never));
    auto many=g;for(unsigned i=4;i<1600;++i)if(i!=255){auto p=path(i,0);set(p,kMotionPathPoints,dense_packet);attach(many,std::move(p));}
    auto limit=evaluate_particle_graph(many,{1,1},never);
    check(!limit.has_value()&&limit.error().code==ErrorCode::work_limit_exceeded,"cumulative path storage bounded");
    Sampler memory(many);limit=evaluate_temporal_particle_graph(many,{1,1},never,{},memory);
    check(!limit.has_value()&&limit.error().code==ErrorCode::work_limit_exceeded,"historical cumulative path storage bounded");
    auto proof=g;set(node(proof,1),kBirthRate,100000.);set(node(proof,255),kParticleCount,std::uint32_t{100000});
    AgeCurve curve;curve.count=64;for(unsigned i=0;i<64;++i)curve.points[i]={double(i)/63,100};set(node(proof,3),kMotionOverLife,encode_age_curve(curve));
    for(unsigned i=4;i<=7;++i){auto p=path(i,0);set(p,kMotionOverLife,encode_age_curve(curve));attach(proof,std::move(p));}
    limit=evaluate_particle_graph(proof,{1,1},never);check(!limit.has_value()&&limit.error().code==ErrorCode::work_limit_exceeded,"analytic curve work charged");
    const auto reference=evaluate(g);std::atomic<bool> good{true};std::array<std::thread,4> threads;
    for(auto& t:threads)t=std::thread([&](){for(unsigned i=0;i<20;++i){auto r=evaluate_particle_graph(g,{1,1},never);if(!r.has_value()||!exact(r.value().particles[0].position,reference.particles[0].position))good=false;}});
    for(auto& t:threads)t.join();check(good,"concurrent requests deterministic");
}
}
int main()try{packet();graph();history();orientation();auxiliary();rendering();failures();std::printf("motion_path_graph_tests: %u checks passed; explicit numeric path/history, no AE/reference qualification\n",checks);return 0;}
catch(const std::exception& e){std::fprintf(stderr,"motion_path_graph_tests after %u checks: %s\n",checks,e.what());return 1;}
