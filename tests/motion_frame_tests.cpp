#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ParticleMotionFrame.hpp"
#include "starfield/core/MotionPathPoints.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/ModelResources.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "../src/core/SpriteGeometry.hpp"
#include <atomic>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <functional>
#include <map>
#include <new>
#include <numbers>
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
void near(double a,double b,const char* label,double tol=1e-9){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tol,label);}
void near(Vec3 a,Vec3 b,const char* label,double tol=1e-9){near(a.x,b.x,label,tol);near(a.y,b.y,label,tol);near(a.z,b.z,label,tol);}
template<class T>T take(Result<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
template<class T>T take(SequenceResult<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 unit(Vec3 v){const auto n=std::hypot(v.x,v.y,v.z);return {v.x/n,v.y/n,v.z/n};}
Vec3 map(const ParticleMotionAffine& m,Vec3 v){return {m[0]*v.x+m[1]*v.y+m[2]*v.z,m[3]*v.x+m[4]*v.y+m[5]*v.z,m[6]*v.x+m[7]*v.y+m[8]*v.z};}
ParticleMotionAffine mul(ParticleMotionAffine a,ParticleMotionAffine b){ParticleMotionAffine r{};for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)for(unsigned k=0;k<3;++k)r[i*3+j]+=a[i*3+k]*b[k*3+j];return r;}
Vec3 frame(const ParticleInstance& p,Vec3 v){return motion_pose_axis(p.motion_pose,map(p.motion_affine,v));}
constexpr ParticleMotionAffine shear{-2,.3,.1,0,1.2,.2,0,0,.75};
ParticleTransformSettings settings(){ParticleTransformSettings s;s.position={.1,.2,.3};s.scale_percent={200,50,100};s.particles_scale_percent=150;s.particles_opacity_percent=50;
    s.inherited_motion={-2,.3,.1,.2,0,1.2,.2,-.1,0,0,.75,.3,0,0,0,1};return s;}
NodeId nid(unsigned n){NodeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
EdgeId eid(unsigned n){EdgeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
GraphNode& node(Graph& g,unsigned id){for(auto& n:g.nodes)if(n.id==nid(id))return n;throw std::runtime_error("missing node");}
void set(GraphNode& n,ParameterKey key,ParameterValue v){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(v);return;}n.parameters.push_back({key,std::move(v)});}
OpaqueBytes matrix(const std::array<double,16>& m){OpaqueBytes bytes(132);bytes[0]=std::byte{1};for(unsigned i=0;i<16;++i){const auto bits=std::bit_cast<std::uint64_t>(m[i]);for(unsigned b=0;b<8;++b)bytes[4+i*8+b]=std::byte((bits>>(8*b))&255);}return bytes;}
GraphNode transform(unsigned id,const ParticleTransformSettings& s){return {nid(id),kTransformNode,1,{{kTransformAnchor,s.anchor},{kTransformPosition,s.position},
    {kTransformRotation,s.rotation_degrees},{kTransformSystemScale,s.scale_percent},{kTransformParticleScale,s.particles_scale_percent},{kTransformParticleOpacity,s.particles_opacity_percent},
    {kTransformInheritedMatrix,matrix(s.inherited_motion)}}};}
GraphNode look(unsigned id,Vec3 goal={0,2,0}){return {nid(id),kMotionNode,1,{{kMotionMode,std::uint32_t{2}},{kMotionGoal,goal},{kMotionForward,Vec3{1,0,0}}}};}
GraphNode path(unsigned id){auto bytes=take(encode_motion_path_points(std::array<Vec3,2>{{{0,0,0},{0,10,0}}},never));
    return {nid(id),kMotionNode,1,{{kMotionMode,std::uint32_t{0}},{kMotionPathPoints,std::move(bytes)},{kMotionPathSpeed,.4}}};}
GraphNode circle(unsigned id){return {nid(id),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{}},{kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,std::numbers::pi/2}}};}
Graph base(){Settings s;s.birth_rate=2;s.particle_count=100;s.particle_lifetime_seconds=3;s.emission_speed=0;s.velocity={.1,.2,.05};s.velocity_spread=0;
    s.emitter_origin={.25,.1,-.1};s.particle_size=6;s.opacity=.8;auto g=take(make_emitter_particle_output_graph(s,nid(1),nid(2),nid(255),eid(1),eid(2)));
    set(node(g,2),kParticleShape,std::uint32_t{1});set(node(g,2),kSizeY,3.);return g;}
void attach(Graph& g,GraphNode n){auto id=n.id;for(auto& e:g.edges)if(e.destination_node==nid(255)){e.destination_node=id;e.destination_port=kMotionParticlesIn;}
    g.nodes.push_back(std::move(n));g.edges.push_back({eid(unsigned(g.edges.size()+10)),id,kMotionParticlesOut,nid(255),kOutputParticles});}
EvaluatedGraph evaluate(const Graph& g,RationalTime t={1,1}){return take(evaluate_particle_graph(g,t,never));}
struct Sampler:TemporalGraphSampler {Graph graph;std::function<void(GraphNode&,double)> change;std::map<std::pair<NodeId,double>,unsigned> calls;
    explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double t)override{for(const auto& n:graph.nodes)if(n.id==id){auto copy=n;if(n.type_key==kMotionNode||n.type_key==kTransformNode)++calls[{id,t}];if(change)change(copy,t);return Result<GraphNode>::success(std::move(copy));}return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing node");}
    Result<double> rate(NodeId id,double t)override{auto r=node(id,t);if(!r.has_value())return Result<double>::failure(r.error());for(auto& p:r.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));return Result<double>::failure(ErrorCode::invalid_request,"missing rate");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override{auto r=rate(id,0);if(!r.has_value())return Result<std::optional<EmissionRateProfile>>::failure(r.error());EmissionRateProfile p;p.constant=r.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(p));}
};
EvaluatedGraph temporal(Sampler& s,RationalTime t={1,1}){return take(evaluate_temporal_particle_graph(s.graph,t,never,{},s));}
void same(const ParticleInstance& p,const ParticleInstance& q){near(p.position,q.position,"center");near(p.velocity,q.velocity,"velocity");near(p.rotation_degrees,q.rotation_degrees,"Euler");near(p.size_pixels,q.size_pixels,"size");near(p.size_y_pixels,q.size_y_pixels,"sizeY");near(p.opacity,q.opacity,"opacity");
    check(p.id==q.id&&p.emitter_id==q.emitter_id&&p.shape==q.shape&&p.sprite_basis_index==q.sprite_basis_index,"identity");
    for(unsigned j=0;j<4;++j)near(p.motion_pose[j],q.motion_pose[j],"quaternion");for(unsigned j=0;j<9;++j)near(p.motion_affine[j],q.motion_affine[j],"affine");}
void same(const EvaluatedGraph& a,const EvaluatedGraph& b){check(a.particles.size()==b.particles.size()&&a.sprite_bases==b.sprite_bases,"population/basis");for(unsigned i=0;i<a.particles.size();++i)same(a.particles[i],b.particles[i]);}
void primitive(){auto s=settings();const auto tf=take(compile_particle_transform(s));ParticleInstance p;p.position={.2,-.4,.1};p.velocity={.3,.2,-.1};p.size_pixels=6;p.size_y_pixels=3;p.opacity=.8;
    const auto c=std::sqrt(.5);p.motion_pose={std::cos(std::numbers::pi/8),0,0,std::sin(std::numbers::pi/8)};p.motion_affine={1,.2,.3,0,2,.1,0,0,-1};auto old=p;
    take(apply_ordered_particle_transform(p,tf));near(p.position,add(map(shear,{2*old.position.x+.1,.5*old.position.y+.2,old.position.z+.3}),{.2,-.1,.3}),"independent affine center");
    near(p.velocity,map(shear,{2*old.velocity.x,.5*old.velocity.y,old.velocity.z}),"independent affine velocity");
    for(auto axis:{Vec3{1,0,0},Vec3{0,1,0},Vec3{0,0,1},Vec3{.3,-.4,.2}}){auto v=map(old.motion_affine,axis);const Vec3 turned{c*(v.x-v.y),c*(v.x+v.y),v.z};near(frame(p,axis),map(shear,turned),"L Q A preserves full shear/reflection");}
    check(p.motion_pose==kIdentityMotionPose,"affine bake resets Q");near(p.size_pixels,9,"separate particle scale");near(p.size_y_pixels,4.5,"separate particle scale Y");near(p.opacity,.4,"particle opacity");
    const auto identity=take(compile_particle_transform(ParticleTransformSettings{}));auto identity_p=old;take(apply_ordered_particle_transform(identity_p,identity));same(identity_p,old);
    check(identity_p.motion_pose==old.motion_pose&&identity_p.motion_affine==old.motion_affine,"identity retains exact pose/affine");
    auto collapse=s;collapse.inherited_motion={0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1};auto collapsed=old;take(apply_ordered_particle_transform(collapsed,take(compile_particle_transform(collapse))));
    near(frame(collapsed,{1,0,0}),Vec3{0,c,0},"singular matrix allowed");
    auto invalid=old;invalid.motion_affine[2]=std::numeric_limits<double>::infinity();auto bad=apply_ordered_particle_transform(invalid,tf);check(!bad.has_value()&&bad.error().code==ErrorCode::invalid_request&&std::isinf(invalid.motion_affine[2]),"invalid frame unchanged");near(invalid.position,old.position,"invalid center unchanged");
    auto overflowing=old;overflowing.position={1e9,0,0};auto before=overflowing;bad=apply_ordered_particle_transform(overflowing,tf);check(!bad.has_value()&&bad.error().code==ErrorCode::work_limit_exceeded,"overflow typed");same(overflowing,before);
    overflowing=old;overflowing.motion_affine[0]=1e12;before=overflowing;bad=apply_ordered_particle_transform(overflowing,tf);check(!bad.has_value()&&bad.error().code==ErrorCode::work_limit_exceeded,"affine coefficient overflow");same(overflowing,before);
    const auto count=allocations.load();for(unsigned i=0;i<5000;++i){auto copy=old;take(apply_ordered_particle_transform(copy,tf));}check(allocations==count,"transform helper allocates nothing");
}
Graph ordered(){auto g=base();ParticleTransformSettings prefix;prefix.position={.1,-.2,.05};prefix.rotation_degrees={0,0,30};attach(g,transform(8,prefix));attach(g,look(3));attach(g,transform(4,settings()));attach(g,path(5));attach(g,circle(6));attach(g,look(7,{2,4,1}));return g;}
void graph(){auto g=ordered();auto r=evaluate(g);auto prefix=base();ParticleTransformSettings before;before.position={.1,-.2,.05};before.rotation_degrees={0,0,30};attach(prefix,transform(8,before));auto expected=evaluate(prefix);
    const auto downstream=take(compile_particle_transform(settings()));for(unsigned i=0;i<r.particles.size();++i){auto p=expected.particles[i];MotionLookAtSettings goal;goal.goal={0,2,0};goal.forward={1,0,0};take(apply_motion_look_at(p,goal,expected.sprite_bases));
        take(apply_ordered_particle_transform(p,downstream));p.position.y+=.4*p.age_seconds;p.velocity.y+=.4;const auto angle=std::numbers::pi/2*p.age_seconds;const auto c=std::cos(angle),s=std::sin(angle);
        const auto pos=p.position,vel=p.velocity;p.position={c*pos.x-s*pos.y,s*pos.x+c*pos.y,pos.z};p.velocity={c*vel.x-s*vel.y-std::numbers::pi/2*p.position.y,s*vel.x+c*vel.y+std::numbers::pi/2*p.position.x,vel.z};
        goal.goal={2,4,1};take(apply_motion_look_at(p,goal,expected.sprite_bases));same(p,r.particles[i]);
        near(unit(take(particle_motion_forward(r.particles[i],{1,0,0},r.sprite_bases))),unit({2-p.position.x,4-p.position.y,1-p.position.z}),"final Look At uses final stage center");}
    Sampler sampler(g);same(r,temporal(sampler));for(auto [key,n]:sampler.calls)check(n==1,"one capture per stage/time");
    Sampler partial(g);same(evaluate(g,{13,20}),temporal(partial,{13,20}));
    Sampler reverse(g);auto later=temporal(reverse,{3,2});same(evaluate(g,{3,2}),later);same(r,temporal(reverse));
    auto encoded=take(serialize_graph(g,particle_node_registry()));same(r,evaluate(take(deserialize_graph(encoded,particle_node_registry()))));
    auto animated=g;set(node(animated,4),kTransformPosition,Vec3{.4,.8,.3});auto current=evaluate(animated);Sampler history(g);history.change=[](GraphNode& n,double t){if(n.id==nid(4))set(n,kTransformPosition,Vec3{.4*t,.8*t,.3});};same(current,temporal(history));
    auto basic=base();attach(basic,look(3));attach(basic,transform(4,settings()));auto reversed=base();attach(reversed,transform(4,settings()));attach(reversed,look(3));auto a=evaluate(basic),b=evaluate(reversed);
    check(a.particles[0].motion_affine!=b.particles[0].motion_affine,"reversed order differs");near(unit(take(particle_motion_forward(b.particles[0],{1,0,0},b.sprite_bases))),unit({-b.particles[0].position.x,2-b.particles[0].position.y,-b.particles[0].position.z}),"prefix Transform then Look goal");
    auto serial=base();attach(serial,look(3));attach(serial,transform(4,settings()));ParticleTransformSettings last;last.rotation_degrees={0,0,90};last.position={-.1,.1,.2};attach(serial,transform(5,last));a=evaluate(serial);
    auto previous=evaluate(basic);const auto last_tf=take(compile_particle_transform(last));for(unsigned i=0;i<a.particles.size();++i){auto p=previous.particles[i];take(apply_ordered_particle_transform(p,last_tf));same(p,a.particles[i]);}Sampler series(serial);same(a,temporal(series));
    auto aligned=basic;auto following=path(5);set(following,kMotionPathOrient,std::uint32_t{1});set(following,kMotionForward,Vec3{1,0,0});attach(aligned,std::move(following));a=evaluate(aligned);
    for(auto& p:a.particles)near(unit(take(particle_motion_forward(p,{1,0,0},a.sprite_bases))),Vec3{0,1,0},"Path alignment sees baked shear frame");
    for(unsigned hz:{30u,60u,120u}){auto sampled_graph=aligned;set(node(sampled_graph,255),kTimeSamplingHz,hz);Sampler sampled(sampled_graph);same(a,temporal(sampled));}
    auto once=basic;set(node(once,1),kBirthRate,1.);set(node(once,1),kEmittingMode,std::uint32_t{1});auto first=evaluate(once,{0,1}),end=evaluate(once);auto linear=take(interpolate_motion_particles(first,end,0,1,.5,100,never));auto exact=evaluate(once,{1,2});
    Sampler subframe(once);same(exact,temporal(subframe,{1,2}));bool differs=false;for(unsigned i=0;i<9;++i)differs=differs||std::abs(linear.particles[0].motion_affine[i]-exact.particles[0].motion_affine[i])>1e-6;check(differs,"Linear affine chord remains explicit approximation to actual subframe");
    auto many=basic;set(node(many,1),kBirthRate,5000.);set(node(many,255),kParticleCount,std::uint32_t{6000});auto population=evaluate(many);check(population.particles.size()==5001&&population.sprite_bases.empty(),"5001 frames without shared slots");check(population.particles.front().motion_affine!=population.particles.back().motion_affine,"individual affine frames");
    auto unsupported=basic;attach(unsupported,{nid(5),kForceNode,3,{{kGravity,Vec3{}},{kLinearDrag,0.}}});auto failure=evaluate_particle_graph(unsupported,{1,1},never);check(!failure.has_value()&&failure.error().code==ErrorCode::invalid_request,"ordered Force is explicit remaining gate");
    Sampler rejected(unsupported);check(!evaluate_temporal_particle_graph(unsupported,{1,1},never,{},rejected).has_value(),"temporal ordered Force gate");
}
void put(OpaqueBytes& bytes,std::size_t offset,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)bytes[offset+i]=std::byte((value>>(8*i))&255);}
void snapshots(){auto g=base();attach(g,look(3));attach(g,transform(4,settings()));auto r=evaluate(g);auto bytes=take(encode_evaluated_particles(r,{1,1}));check(std::to_integer<unsigned>(bytes[2])==10,"nonidentity affine snapshot10");
    const auto start=80+16*r.evaluated_nodes.size();check(bytes.size()==start+304*r.particles.size(),"explicit 80/304 stride");same(r,take(decode_evaluated_particles(bytes,{1,1})));check(take(encode_evaluated_particles(take(decode_evaluated_particles(bytes,{1,1})),{1,1}))==bytes,"exact snapshot10 roundtrip");
    for(std::size_t n=0;n<bytes.size();++n){auto truncated=bytes;truncated.resize(n);check(!decode_evaluated_particles(truncated,{1,1}).has_value(),"every snapshot10 truncation");}
    for(auto offset:{36u,44u,52u,60u,64u,68u,72u,76u}){auto bad=bytes;bad[offset]^=std::byte{1};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"snapshot10 header/reserved");}
    for(auto value:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),1e12+1}){auto bad=bytes;put(bad,start+232,std::bit_cast<std::uint64_t>(value),8);check(!decode_evaluated_particles(bad,{1,1}).has_value(),"invalid affine bytes");auto invalid=r;invalid.particles[0].motion_affine[0]=value;check(!encode_evaluated_particles(invalid,{1,1}).has_value(),"invalid affine cannot encode");}
    auto bad=bytes;put(bad,start+200,0,8);check(!decode_evaluated_particles(bad,{1,1}).has_value(),"snapshot10 retains quaternion validation");
    for(unsigned version=3;version<=9;++version){auto legacy=evaluate(base());legacy.particles.resize(1);auto& p=legacy.particles[0];if(version==4){legacy.sprite_bases.push_back(shear);p.sprite_basis_index=1;}if(version==5)p.transfer_mode=ParticleTransferMode::add;
        if(version==6){legacy.texture_styles.push_back({});p.shape=3;p.texture_style_index=1;}if(version==7){legacy.cloud_styles.push_back({});p.shape=2;p.cloud_style_index=1;}if(version==8){legacy.model_styles.push_back({{ParticleModelInstance{}}});p.shape=4;p.model_style_index=1;}
        if(version==9)p.motion_pose=take(turn_motion_pose(kIdentityMotionPose,{1,0,0},{0,1,0},1));auto old=take(encode_evaluated_particles(legacy,{1,1}));check(std::to_integer<unsigned>(old[2])==version,"old snapshot selection");auto decoded=take(decode_evaluated_particles(old,{1,1}));check(decoded.particles[0].motion_affine==kIdentityMotionAffine,"old frame initializes identity");check(take(encode_evaluated_particles(decoded,{1,1}))==old,"old byte roundtrip exact");
        p.motion_affine=shear;auto migrated=take(encode_evaluated_particles(legacy,{1,1}));check(std::to_integer<unsigned>(migrated[2])==10,"all table families migrate10");auto restored=take(decode_evaluated_particles(migrated,{1,1}));same(legacy,restored);check(restored.model_styles.size()==legacy.model_styles.size()&&restored.texture_styles.size()==legacy.texture_styles.size()&&restored.cloud_styles.size()==legacy.cloud_styles.size(),"all table counts retained");}
    auto all=r;all.particles.resize(1);all.sprite_bases.push_back(shear);auto& p=all.particles[0];p.sprite_basis_index=1;all.model_styles.push_back({{ParticleModelInstance{}}});p.shape=4;p.model_style_index=1;
    auto model=take(encode_evaluated_particles(all,{1,1}));same(all,take(decode_evaluated_particles(model,{1,1})));
    auto malformed=model;const auto group_start=80+16*all.evaluated_nodes.size()+72;malformed.erase(malformed.begin()+group_start,malformed.begin()+group_start+152);put(malformed,4,malformed.size(),4);
    const auto allocation_start=allocations.load();check(!decode_evaluated_particles(malformed,{1,1}).has_value(),"Model group absent rejects preflight");check(allocations==allocation_start,"Model header bounds before allocation");
    auto a=r,b=r;for(auto& particle:a.particles){particle.motion_affine=kIdentityMotionAffine;particle.motion_pose=kIdentityMotionPose;}auto middle=take(interpolate_motion_particles(a,b,1,1,.5,100,never));
    for(unsigned i=0;i<9;++i)near(middle.particles[0].motion_affine[i],.5*(a.particles[0].motion_affine[i]+b.particles[0].motion_affine[i]),"Linear shutter affine lerp");
    auto single=b;single.particles.clear();middle=take(interpolate_motion_particles(a,single,1,1,.5,100,never));check(middle.particles[0].motion_affine==a.particles[0].motion_affine,"endpoint-only affine");
    b.particles[0].motion_affine[0]=std::numeric_limits<double>::infinity();check(!interpolate_motion_particles(a,b,1,1,.5,100,never).has_value(),"invalid shutter affine");
    struct Stop:Cancellation{mutable unsigned n{};unsigned limit;explicit Stop(unsigned l):limit(l){}bool is_cancelled()const noexcept override{return n++>=limit;}};
    for(unsigned operation=0;operation<3;++operation){bool success=false;for(unsigned n=0;n<256;++n){Stop stop(n);if(operation==0){auto outcome=encode_evaluated_particles(r,{1,1},&stop);if(outcome.has_value()){success=true;break;}check(outcome.error().code==ErrorCode::cancelled,"snapshot10 encode cancellation");}
        else {auto outcome=operation==1?decode_evaluated_particles(bytes,{1,1},&stop):interpolate_motion_particles(r,r,1,1,.5,100,stop);if(outcome.has_value()){success=true;break;}check(outcome.error().code==ErrorCode::cancelled,"snapshot10 decode/shutter cancellation");}}check(success,"snapshot cancellation reaches success");
        success=false;for(unsigned n=1;n<128;++n){fail_allocation=allocations.load()+n;if(operation==0){auto outcome=encode_evaluated_particles(r,{1,1});fail_allocation=0;if(outcome.has_value()){success=true;break;}check(outcome.error().code==ErrorCode::allocation_failed,"snapshot10 encode allocation");}
        else {auto outcome=operation==1?decode_evaluated_particles(bytes,{1,1}):interpolate_motion_particles(r,r,1,1,.5,100,never);fail_allocation=0;if(outcome.has_value()){success=true;break;}check(outcome.error().code==ErrorCode::allocation_failed,"snapshot10 decode/shutter allocation");}}check(success,"snapshot allocation reaches success");}
}
void rendering(){RenderRequest request;request.frame.layer_width=request.frame.layer_height=request.frame.frame_width=request.frame.frame_height=64;request.frame.region_of_interest={0,0,64,64};request.frame.time={1,1};request.frame.frame_duration={1,60};request.frame.format=PixelFormat::rgba32f;
    auto g=base();set(node(g,1),kBirthRate,1.);set(node(g,1),kEmittingMode,std::uint32_t{1});set(node(g,1),kEmitterOrigin,Vec3{});set(node(g,1),kVelocity,Vec3{});attach(g,look(3));ParticleTransformSettings rotate;rotate.rotation_degrees={0,0,90};attach(g,transform(4,rotate));request.graph=std::make_shared<const Graph>(g);
    auto image=take(CpuParticleRenderer{}.render(request,never));auto alpha=[&](unsigned x,unsigned y){float a;std::memcpy(&a,image.pixels.data()+y*image.row_bytes+x*16+12,4);return a;};check(alpha(34,32)>.5f&&alpha(32,35)==0,"CPU uses baked affine orientation");
    auto scene=take(prepare_sprite_scene(request,never));check(scene.sprites.size()==1,"GPU prepares ordered affine sprite");near(std::abs(scene.sprites[0].inverse_ax),1./3,"GPU long inverse axis",1e-6);near(std::abs(scene.sprites[0].inverse_by),2./3,"GPU short inverse axis",1e-6);
    ParticleInstance p;p.shape=4;p.size_pixels=2;p.size_y_pixels=2;p.motion_affine=shear;p.motion_pose={std::cos(.2),0,0,std::sin(.2)};p.sprite_basis_index=1;const ParticleSpriteBasis basis{1,.1,0,0,2,0,0,0,-1};auto m=take(model_particle_matrix(p,request.frame,{&basis,1}));auto combined=mul(shear,basis);p.motion_affine=kIdentityMotionAffine;auto expected=take(model_particle_matrix(p,request.frame,{&combined,1}));for(unsigned i=0;i<16;++i)near(m[i],expected[i],"Model Q A B numeric matrix");
    request.camera.enabled=true;request.camera.focal_x=request.camera.focal_y=100;request.camera.center_x=request.camera.center_y=32;request.camera.image_to_layer={1,0,0,0,1,0,0,0,1};const auto c=std::sqrt(.5);
    request.camera.layer_to_view={c,0,-c,0,0,1,0,0,c,0,c,0,-32*c,-32,100+32*c,1};const auto grid=sprite_geometry::make_grid(request.frame);ParticleInstance facing;facing.shape=1;facing.size_pixels=facing.size_y_pixels=20;facing.opacity=1;facing.rotation_degrees={0,-90,0};sprite_geometry::Sprite reference;
    check(sprite_geometry::project_sprite(facing,request,grid,reference),"reference depth plane");for(unsigned shape:{0u,2u}){auto affine=facing;affine.shape=shape;affine.rotation_degrees={};affine.motion_affine={0,0,-1,0,1,0,1,0,0};sprite_geometry::Sprite actual;check(sprite_geometry::project_sprite(affine,request,grid,actual),"affine billboard projects");near(actual.ax,reference.ax,"camera full affine X");near(actual.ay,reference.ay,"camera full affine Y");near(actual.bx,reference.bx,"camera transverse X");near(actual.by,reference.by,"camera transverse Y");}
}
void auxiliary(){auto g=base();set(node(g,1),kEmittingMode,std::uint32_t{1});attach(g,look(3));attach(g,transform(6,settings()));auto child=node(g,1);child.id=nid(4);set(child,kEmittingMode,std::uint32_t{0});set(child,kAuxiliarySource,std::uint32_t{1});set(child,kBirthRate,2.);set(child,kInheritVelocity,100.);set(child,kEmitterOrigin,Vec3{});set(child,kVelocity,Vec3{});auto style=node(g,2);style.id=nid(5);g.nodes.push_back(child);g.nodes.push_back(style);
    g.edges.push_back({eid(30),nid(6),kTransformParticlesOut,nid(4),kEmitterParents});g.edges.push_back({eid(31),nid(4),kEmitterParticles,nid(5),kParticleParticlesIn});g.edges.push_back({eid(32),nid(5),kParticleParticlesOut,nid(255),kOutputParticles});
    auto r=evaluate(g);Sampler s(g);same(r,temporal(s));unsigned children=0;auto parent_graph=g;parent_graph.nodes.erase(parent_graph.nodes.end()-2,parent_graph.nodes.end());parent_graph.edges.resize(parent_graph.edges.size()-3);
    for(auto& p:r.particles)if(p.emitter_id==nid(4)){++children;const auto birth=1-p.age_seconds;auto parent=evaluate(parent_graph,{std::int64_t(std::round(birth*1000)),1000}).particles[0];near(p.position,{parent.position.x+parent.velocity.x*p.age_seconds,parent.position.y+parent.velocity.y*p.age_seconds,parent.position.z+parent.velocity.z*p.age_seconds},"Auxiliary inherits ordered parent center/velocity");}check(children>=2,"Auxiliary ordered prefix exercised");
}
void failures(){auto g=ordered();const auto original=take(serialize_graph(g,particle_node_registry()));bool success=false;
    struct Stop:Cancellation{mutable unsigned n{};unsigned bound;explicit Stop(unsigned b):bound(b){}bool is_cancelled()const noexcept override{return n++>=bound;}};
    for(bool temporal_scope:{false,true}){success=false;for(unsigned i=0;i<3000;++i){Sampler s(g);Stop cancel(i);auto r=temporal_scope?evaluate_temporal_particle_graph(g,{1,1},cancel,{},s):evaluate_particle_graph(g,{1,1},cancel);if(r.has_value()){success=true;break;}check(r.error().code==ErrorCode::cancelled,"whole evaluation cancellation typed");}check(success,"cancel reaches success");
        success=false;for(unsigned n=1;n<3000;++n){Sampler s(g);fail_allocation=allocations.load()+n;auto r=temporal_scope?evaluate_temporal_particle_graph(g,{1,1},never,{},s):evaluate_particle_graph(g,{1,1},never);fail_allocation=0;if(r.has_value()){success=true;break;}check(r.error().code==ErrorCode::allocation_failed,"whole frame allocation typed");}check(success,"allocation reaches success");}
    check(original==take(serialize_graph(g,particle_node_registry())),"failure leaves source graph unchanged");const auto reference=take(encode_evaluated_particles(evaluate(g),{1,1}));std::atomic<bool> okay{true};std::thread jobs[4];for(auto& job:jobs)job=std::thread([&]{for(unsigned n=0;n<10;++n){auto r=evaluate_particle_graph(g,{1,1},never);if(!r.has_value()){okay=false;return;}auto bytes=encode_evaluated_particles(r.value(),{1,1});if(!bytes.has_value()||bytes.value()!=reference)okay=false;}});for(auto& job:jobs)job.join();check(okay,"concurrent independent graph requests");
}
}
int main()try{primitive();graph();snapshots();rendering();auxiliary();failures();std::printf("motion_frame_tests: %u checks passed; ordered numeric Motion/Transform and snapshot10; AE qualification remains open\n",checks);return 0;}catch(const std::exception& e){std::fprintf(stderr,"Motion frame after %u checks: %s\n",checks,e.what());return 1;}
