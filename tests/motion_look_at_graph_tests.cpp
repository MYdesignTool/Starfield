#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/ModelResources.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "../src/core/SpriteGeometry.hpp"
#include <atomic>
#include <bit>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <functional>
#include <map>
#include <new>
#include <stdexcept>
#include <thread>
namespace{std::atomic<std::size_t> allocations{},fail_allocation{};}
void* operator new(std::size_t size){auto n=++allocations;if(n==fail_allocation)throw std::bad_alloc{};
    if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};}
void operator delete(void* p)noexcept{std::free(p);}void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield::core;using namespace starfield::core::graph_keys;
namespace{
unsigned checks{};NeverCancelled never;
void check(bool v,const char* label){++checks;if(!v)throw std::runtime_error(label);}
void near(double a,double b,const char* label,double tol=1e-9){check(std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<tol,label);}
void near(Vec3 a,Vec3 b,const char* label){near(a.x,b.x,label);near(a.y,b.y,label);near(a.z,b.z,label);}
template<class T>T take(Result<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
template<class T>T take(SequenceResult<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
NodeId nid(unsigned n){NodeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
EdgeId eid(unsigned n){EdgeId id;id.value.bytes[14]=std::uint8_t(n>>8);id.value.bytes[15]=std::uint8_t(n);return id;}
GraphNode& node(Graph& g,unsigned id){for(auto& n:g.nodes)if(n.id==nid(id))return n;throw std::runtime_error("missing fixture node");}
void set(GraphNode& n,ParameterKey key,ParameterValue v){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(v);return;}n.parameters.push_back({key,std::move(v)});}
Graph base(){Settings s;s.birth_rate=2;s.particle_count=100;s.particle_lifetime_seconds=3;s.emission_speed=0;s.velocity={};s.velocity_spread=0;
    s.emitter_origin={};s.particle_size=20;s.opacity=1;auto g=take(make_emitter_particle_output_graph(s,nid(1),nid(2),nid(255),eid(1),eid(2)));
    set(node(g,2),kParticleShape,std::uint32_t{1});set(node(g,2),kSizeY,4.);return g;}
GraphNode look(unsigned id=3,Vec3 goal={0,1,0}){return {nid(id),kMotionNode,1,{{kMotionMode,std::uint32_t{2}},{kMotionGoal,goal},{kMotionForward,Vec3{1,0,0}}}};}
void attach(Graph& g,unsigned id=3,Vec3 goal={0,1,0}){for(auto& e:g.edges)if(e.destination_node==nid(255)){e.destination_node=nid(id);e.destination_port=kMotionParticlesIn;}
    g.nodes.push_back(look(id,goal));g.edges.push_back({eid(id+10),nid(id),kMotionParticlesOut,nid(255),kOutputParticles});}
EvaluatedGraph evaluate(const Graph& g,RationalTime t={1,1}){return take(evaluate_particle_graph(g,t,never));}
void same_centres(const EvaluatedGraph& a,const EvaluatedGraph& b){check(a.particles.size()==b.particles.size(),"population unchanged");check(a.sprite_bases==b.sprite_bases,"affine basis unchanged");
    for(unsigned i=0;i<a.particles.size();++i){const auto& p=a.particles[i];const auto& q=b.particles[i];near(p.position,q.position,"center unchanged");near(p.velocity,q.velocity,"velocity unchanged");
        near(p.rotation_degrees,q.rotation_degrees,"authored Euler unchanged");check(p.id==q.id&&p.emitter_id==q.emitter_id&&p.shape==q.shape&&p.size_pixels==q.size_pixels&&p.opacity==q.opacity,"identity/style unchanged");}}
struct Sampler:TemporalGraphSampler{
    Graph graph;std::function<void(GraphNode&,double)> change;std::map<std::pair<NodeId,double>,unsigned> calls;
    explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double t)override{for(const auto& n:graph.nodes)if(n.id==id){auto copy=n;if(n.type_key==kMotionNode)++calls[{id,t}];
        if(change)change(copy,t);return Result<GraphNode>::success(std::move(copy));}return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing fixture node");}
    Result<double> rate(NodeId id,double t)override{auto n=node(id,t);if(!n.has_value())return Result<double>::failure(n.error());for(auto& p:n.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));return Result<double>::failure(ErrorCode::invalid_request,"missing rate");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override{auto r=rate(id,0);if(!r.has_value())return Result<std::optional<EmissionRateProfile>>::failure(r.error());EmissionRateProfile p;p.constant=r.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(p));}
};
EvaluatedGraph temporal(Sampler& s){return take(evaluate_temporal_particle_graph(s.graph,{1,1},never,{},s));}
void math(){
    auto q=take(turn_motion_pose(kIdentityMotionPose,{1,0,0},{0,1,0},1));near(motion_pose_axis(q,{1,0,0}),Vec3{0,1,0},"shortest arc");
    near(motion_pose_axis(q,{0,1,0}),Vec3{-1,0,0},"proper rotation handedness");check(valid_particle_motion_pose(q),"unit quaternion");
    auto half=take(turn_motion_pose(kIdentityMotionPose,{1,0,0},{0,1,0},.5));near(motion_pose_axis(half,{1,0,0}),Vec3{std::sqrt(.5),std::sqrt(.5),0},"half weight");
    check(take(turn_motion_pose(q,{1,0,0},{0,1,0},0))==q,"zero weight exact prior pose");check(take(turn_motion_pose(q,{},{0,1,0},1))==q,"collapsed forward exact prior pose");
    check(take(turn_motion_pose(q,{1,0,0},{},1))==q,"coincident goal exact prior pose");
    near(motion_pose_axis(take(turn_motion_pose(kIdentityMotionPose,{1e-320,1e-320,0},{-1,-1,0},1)),Vec3{std::sqrt(.5),std::sqrt(.5),0}),Vec3{-std::sqrt(.5),-std::sqrt(.5),0},"subnormal antiparallel");
    auto antipodal=q;for(auto& v:antipodal)v=-v;near(motion_pose_axis(take(interpolate_motion_pose(q,antipodal,.5)),{1,0,0}),Vec3{0,1,0},"equivalent quaternions don't spin");
    near(motion_pose_axis(take(interpolate_motion_pose(kIdentityMotionPose,q,.5)),{1,0,0}),Vec3{std::sqrt(.5),std::sqrt(.5),0},"SLERP midpoint");
    check(!turn_motion_pose({0,0,0,0},{1,0,0},{0,1,0},1).has_value(),"invalid pose rejected");
    ParticleInstance particle;particle.shape=1;particle.age_seconds=1;particle.lifetime_seconds=3;
    particle.rotation_degrees={45,30,75};particle.position={.2,-.4,.1};particle.sprite_basis_index=1;
    const ParticleSpriteBasis basis{-2,.3,.1,0,1,.2,0,0,1};MotionLookAtSettings target;target.goal={-1,4,3};
    take(apply_motion_look_at(particle,target,{&basis,1}));
    auto mapped=take(particle_motion_forward(particle,target.forward,{&basis,1}));const auto n=std::hypot(mapped.x,mapped.y,mapped.z);
    const Vec3 delta{target.goal.x-particle.position.x,target.goal.y-particle.position.y,target.goal.z-particle.position.z};const auto dn=std::hypot(delta.x,delta.y,delta.z);
    near({mapped.x/n,mapped.y/n,mapped.z/n},Vec3{delta.x/dn,delta.y/dn,delta.z/dn},"Look At accounts for authored Euler/shear/reflection");
    near(particle.rotation_degrees,Vec3{45,30,75},"authored angles retained");
}
void graph(){auto plain=base(),g=plain;attach(g);auto p=evaluate(plain),r=evaluate(g);same_centres(p,r);
    for(auto& particle:r.particles)near(take(particle_motion_forward(particle,{1,0,0},r.sprite_bases)),Vec3{0,1,0},"graph actual forward");
    Sampler s(g);auto t=temporal(s);same_centres(r,t);for(unsigned i=0;i<t.particles.size();++i)check(t.particles[i].motion_pose==r.particles[i].motion_pose,"temporal same pose");
    for(auto [key,count]:s.calls)check(count==1,"goal capture once per node/time");
    auto bytes=take(serialize_graph(g,particle_node_registry()));same_centres(r,evaluate(take(deserialize_graph(bytes,particle_node_registry()))));
    AgeCurve curve;curve.count=2;curve.points[0]={0,0};curve.points[1]={1,100};set(node(g,3),kMotionOverLife,encode_age_curve(curve));
    auto age=evaluate(g);near(motion_pose_axis(age.particles[0].motion_pose,{1,0,0}),Vec3{std::cos(std::numbers::pi/6),.5,0},"age weight");
    check(age.particles.back().motion_pose==kIdentityMotionPose,"newborn zero curve preserves pose");
    set(node(g,3),kMotionGoal,Vec3{});auto coincident=evaluate(g);for(auto& particle:coincident.particles)check(particle.motion_pose==kIdentityMotionPose,"coincident goal preserves original");
    auto animated=base();attach(animated);Sampler a(animated);a.change=[](GraphNode& n,double seconds){if(n.type_key==kMotionNode)set(n,kMotionGoal,Vec3{seconds,1,0});};
    auto moved=temporal(a);near(take(particle_motion_forward(moved.particles[0],{1,0,0},moved.sprite_bases)),Vec3{std::sqrt(.5),std::sqrt(.5),0},"animated goal request time");
    set(node(animated,2),kLimitTo2D,std::uint32_t{1});set(node(animated,3),kMotionGoal,Vec3{0,1,100});
    auto flat=evaluate(animated);near(take(particle_motion_forward(flat.particles[0],{1,0,0},flat.sprite_bases)),Vec3{0,1,0},"Limit To2D");
    auto many=base();set(node(many,1),kBirthRate,5000.);set(node(many,255),kParticleCount,std::uint32_t{6000});set(node(many,1),kVelocity,Vec3{.4,-.2,0});attach(many,3,{1,1,0});
    auto population=evaluate(many);check(population.particles.size()==5001&&population.sprite_bases.empty(),"per-particle pose exceeds4096 without shared entries");
    check(population.particles.front().motion_pose!=population.particles.back().motion_pose,"individual orientations");
    auto malformed=many;set(node(malformed,3),kMotionForward,Vec3{});check(!evaluate_particle_graph(malformed,{1,1},never).has_value(),"zero authored forward rejected");
    set(node(malformed,3),kMotionForward,Vec3{1,0,0});set(node(malformed,3),kMotionGoal,Vec3{std::numeric_limits<double>::infinity(),0,0});check(!evaluate_particle_graph(malformed,{1,1},never).has_value(),"nonfinite goal rejected");
}
void snapshots(){auto g=base();attach(g);auto r=evaluate(g);auto bytes=take(encode_evaluated_particles(r,{1,1}));
    check(std::to_integer<unsigned>(bytes[2])==9,"pose uses snapshot9");check(bytes.size()==72+16*r.evaluated_nodes.size()+232*r.particles.size(),"snapshot9 explicit stride");
    auto restored=take(decode_evaluated_particles(bytes,{1,1}));same_centres(r,restored);for(unsigned i=0;i<r.particles.size();++i)check(r.particles[i].motion_pose==restored.particles[i].motion_pose,"quaternion roundtrip");
    auto legacy=take(encode_evaluated_particles(evaluate(base()),{1,1}));check(std::to_integer<unsigned>(legacy[2])==3,"identity retains old snapshot version");
    auto old=take(decode_evaluated_particles(legacy,{1,1}));for(auto& p:old.particles)check(p.motion_pose==kIdentityMotionPose,"old snapshot initializes identity");
    for(std::size_t n=0;n<bytes.size();++n){auto short_record=bytes;short_record.resize(n);check(!decode_evaluated_particles(short_record,{1,1}).has_value(),"every truncated record rejected");}
    for(unsigned offset:{64u,68u}){auto bad=bytes;bad[offset]^=std::byte{1};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"bad stride/reserved rejected");}
    auto bad=bytes;const auto pose_offset=72+16*r.evaluated_nodes.size()+200;for(unsigned i=0;i<32;++i)bad[pose_offset+i]=std::byte{};
    check(!decode_evaluated_particles(bad,{1,1}).has_value(),"zero quaternion rejected");
    auto a=r,b=r;for(auto& p:a.particles)p.motion_pose=kIdentityMotionPose;
    auto middle=take(interpolate_motion_particles(a,b,1,1,.5,100,never));near(motion_pose_axis(middle.particles[0].motion_pose,{1,0,0}),Vec3{std::sqrt(.5),std::sqrt(.5),0},"Linear shutter SLERP");
    b.particles[0].motion_pose={};check(!interpolate_motion_particles(a,b,1,1,.5,100,never).has_value(),"invalid shutter pose rejected");

    // Independently reconstruct every old header/table selection. Identity pose
    // adds neither a header word nor any bytes to the released record stride.
    for(unsigned version=3;version<=8;++version){auto legacy_graph=evaluate(base());legacy_graph.particles.resize(1);auto& p=legacy_graph.particles[0];
        if(version==4){legacy_graph.sprite_bases.push_back({-1,0,0,0,1,0,0,0,1});p.sprite_basis_index=1;}
        if(version==5)p.transfer_mode=ParticleTransferMode::add;
        if(version==6){legacy_graph.texture_styles.push_back({});p.shape=3;p.texture_style_index=1;}
        if(version==7){legacy_graph.cloud_styles.push_back({});p.shape=2;p.cloud_style_index=1;}
        if(version==8){legacy_graph.model_styles.push_back({{ParticleModelInstance{}}});p.shape=4;p.model_style_index=1;}
        auto saved=take(encode_evaluated_particles(legacy_graph,{1,1}));check(std::to_integer<unsigned>(saved[2])==version,"old version selection stable");
        auto loaded=take(decode_evaluated_particles(saved,{1,1}));check(loaded.particles[0].motion_pose==kIdentityMotionPose,"all old versions load identity");
        check(take(encode_evaluated_particles(loaded,{1,1}))==saved,"old bytes roundtrip exactly");
        p.motion_pose=take(turn_motion_pose(kIdentityMotionPose,{1,0,0},{0,1,0},1));
        auto migrated=take(encode_evaluated_particles(legacy_graph,{1,1}));check(std::to_integer<unsigned>(migrated[2])==9,"all old tables migrate to9");
        auto current=take(decode_evaluated_particles(migrated,{1,1}));check(current.particles[0].motion_pose==p.motion_pose,"all table pose roundtrip");
        check(current.particles[0].shape==p.shape&&current.particles[0].sprite_basis_index==p.sprite_basis_index&&current.particles[0].model_style_index==p.model_style_index,"table indices intact");
    }
}
void mixed_chains(){auto g=base();attach(g);attach(g,4,{-1,0,0});auto r=evaluate(g);
    near(take(particle_motion_forward(r.particles[0],{1,0,0},r.sprite_bases)),Vec3{-1,0,0},"serial Look At composes");
    Sampler s(g);auto t=temporal(s);check(t.particles[0].motion_pose==r.particles[0].motion_pose,"serial temporal Look At");
    auto before=base();set(node(before,1),kEmitterOrigin,Vec3{.25,0,0});attach(before);
    before.edges[1].destination_node=nid(4);before.nodes.push_back({nid(4),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{}},{kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,std::numbers::pi/2}}});
    before.edges.push_back({eid(5),nid(4),kMotionParticlesOut,nid(3),kMotionParticlesIn});r=evaluate(before);
    near(r.particles[0].position,Vec3{0,.25,0},"Circle then Look At position");near(take(particle_motion_forward(r.particles[0],{1,0,0},r.sprite_bases)),Vec3{0,1,0},"Look At uses resulting center");
    auto after=base();attach(after);after.edges.back().destination_node=nid(4);after.edges.back().destination_port=kMotionParticlesIn;
    after.nodes.push_back({nid(4),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,Vec3{}},{kMotionAxis,Vec3{0,0,1}},{kMotionAngularRate,0.}}});
    after.edges.push_back({eid(5),nid(4),kMotionParticlesOut,nid(255),kOutputParticles});check(!evaluate_particle_graph(after,{1,1},never).has_value(),"unimplemented Look At/Circle order rejects even zero rate");
}
void rendering(){auto g=base();set(node(g,1),kBirthRate,1.);set(node(g,1),kEmittingMode,std::uint32_t{1});attach(g);
    RenderRequest request;request.graph=std::make_shared<const Graph>(g);request.frame.layer_width=request.frame.layer_height=request.frame.frame_width=request.frame.frame_height=64;
    request.frame.region_of_interest={0,0,64,64};request.frame.time={1,1};request.frame.frame_duration={1,60};request.frame.format=PixelFormat::rgba32f;
    auto image=take(CpuParticleRenderer{}.render(request,never));const auto alpha=[&](unsigned x,unsigned y){float v;std::memcpy(&v,image.pixels.data()+y*image.row_bytes+x*16+12,4);return v;};
    check(alpha(32,25)>.9f&&alpha(39,32)==0,"CPU rectangle orientation applied");auto scene=take(prepare_sprite_scene(request,never));check(scene.sprites.size()==1,"GPU scene gets oriented sprite");
    near(scene.sprites[0].inverse_ax,0,"GPU rotated inverse X",1e-6);near(scene.sprites[0].inverse_by,0,"GPU rotated inverse Y",1e-6);
    near(std::abs(scene.sprites[0].inverse_ay),.1,"GPU long axis orientation",1e-6);near(std::abs(scene.sprites[0].inverse_bx),.5,"GPU short axis orientation",1e-6);
    auto r=evaluate(g);auto p=r.particles[0];p.shape=4;p.size_pixels=1;ParticleSpriteBasis shear{-2,.3,.1,0,1,.2,0,0,1};p.sprite_basis_index=1;
    auto posed=take(model_particle_matrix(p,request.frame,{&shear,1}));p.motion_pose=kIdentityMotionPose;auto original=take(model_particle_matrix(p,request.frame,{&shear,1}));
    for(unsigned a=0;a<3;++a)for(unsigned b=0;b<3;++b){double x=0,y=0;for(unsigned i=0;i<3;++i){x+=posed[a*4+i]*posed[b*4+i];y+=original[a*4+i]*original[b*4+i];}near(x,y,"Model affine Gram preserved");}
    check(posed[12]==original[12]&&posed[13]==original[13]&&posed[14]==original[14],"Model center preserved");

    // Circle/Cloud normally billboard. A nonidentity proper pose makes their
    // full world axes significant even without a shared Transform table.
    const auto c=std::sqrt(.5);request.camera.enabled=true;request.camera.focal_x=request.camera.focal_y=100;
    request.camera.center_x=request.camera.center_y=32;request.camera.image_to_layer={1,0,0,0,1,0,0,0,1};
    request.camera.layer_to_view={c,0,-c,0,0,1,0,0,c,0,c,0,-32*c,-32,100+32*c,1};
    const auto grid=sprite_geometry::make_grid(request.frame);
    ParticleInstance facing;facing.shape=1;facing.size_pixels=facing.size_y_pixels=20;facing.opacity=1;
    facing.rotation_degrees={0,-90,0};sprite_geometry::Sprite expected;
    check(sprite_geometry::project_sprite(facing,request,grid,expected),"reference 3D plane projects");
    for(unsigned shape:{0u,2u}){auto oriented=facing;oriented.shape=shape;oriented.rotation_degrees={};
        oriented.motion_pose=take(turn_motion_pose(kIdentityMotionPose,{1,0,0},{0,0,1},1));sprite_geometry::Sprite actual;
        check(sprite_geometry::project_sprite(oriented,request,grid,actual),"oriented billboard projects");
        near(actual.ax,expected.ax,"camera keeps posed billboard world X");near(actual.ay,expected.ay,"camera keeps posed billboard world Y");
        near(actual.bx,expected.bx,"camera keeps posed billboard transverse X");near(actual.by,expected.by,"camera keeps posed billboard transverse Y");}
}
void failures(){auto g=base();attach(g);bool completed=false;
    struct Stop:Cancellation{mutable unsigned n{};unsigned bound;explicit Stop(unsigned b):bound(b){}bool is_cancelled()const noexcept override{return n++>=bound;}};
    for(unsigned i=0;i<300;++i){Sampler s(g);Stop cancel(i);auto r=evaluate_temporal_particle_graph(g,{1,1},cancel,{},s);if(r.has_value()){completed=true;break;}check(r.error().code==ErrorCode::cancelled,"cancellation typed");}check(completed,"cancellation reaches success");
    for(bool temporal_scope:{false,true}){completed=false;for(unsigned n=1;n<2000;++n){Sampler s(g);fail_allocation=allocations.load()+n;
        auto r=temporal_scope?evaluate_temporal_particle_graph(g,{1,1},never,{},s):evaluate_particle_graph(g,{1,1},never);fail_allocation=0;
        if(r.has_value()){completed=true;break;}check(r.error().code==ErrorCode::allocation_failed,"all allocation failures typed");}check(completed,"fault scan reaches success");}
}
}
int main()try{math();graph();snapshots();mixed_chains();rendering();failures();std::printf("motion_look_at_graph_tests: %u checks passed; numeric goal/pose, CPU/GPU preparation and snapshot9; AE remains open\n",checks);return 0;
}catch(const std::exception& e){std::fprintf(stderr,"Look At fixture after %u checks: %s\n",checks,e.what());return 1;}
