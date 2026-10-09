#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/ModelResources.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/SequenceCodec.hpp"
#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstring>
#include <limits>
#include <map>
#include <stdexcept>
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};NeverCancelled never;
void check(bool value,const char* label){++checks;if(!value)throw std::runtime_error(label);}
template<class T>T take(Result<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
template<class T>T take(SequenceResult<T> value){if(!value.has_value())throw std::runtime_error(value.error().detail);return value.take_value();}
NodeId nid(unsigned n){NodeId id;id.value.bytes[14]=static_cast<std::uint8_t>(n>>8);id.value.bytes[15]=static_cast<std::uint8_t>(n);return id;}
EdgeId eid(unsigned n){EdgeId id;id.value.bytes[14]=static_cast<std::uint8_t>(n>>8);id.value.bytes[15]=static_cast<std::uint8_t>(n);return id;}
GraphNode& node(Graph& g,unsigned id){for(auto& n:g.nodes)if(n.id==nid(id))return n;throw std::runtime_error("fixture node missing");}
const ParameterValue& get(const GraphNode& n,ParameterKey key){for(const auto& p:n.parameters)if(p.key==key)return p.value;throw std::runtime_error("fixture key missing");}
void set(GraphNode& n,ParameterKey key,ParameterValue value){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(value);return;}n.parameters.push_back({key,std::move(value)});}
OpaqueBytes matrix_bytes(std::array<double,16> m){OpaqueBytes bytes;for(double value:m){const auto bits=std::bit_cast<std::uint64_t>(value);for(unsigned b=0;b<8;++b)bytes.push_back(static_cast<std::byte>((bits>>(b*8))&255));}return bytes;}
OpaqueBytes resource_bytes(unsigned id){OpaqueBytes bytes(16);bytes[0]=static_cast<std::byte>(id);return bytes;}
GraphNode model(unsigned id,unsigned resource=0,double x=0){auto m=kIdentityModelMatrix;m[12]=x;return {nid(id),kModelNode,1,{{kModelResource,resource_bytes(resource)},{kModelRevision,std::uint32_t{0}},{kModelLocalMatrix,matrix_bytes(m)}}};}
Graph base(){Settings s;s.birth_rate=4;s.particle_count=64;s.particle_lifetime_seconds=2;s.emission_speed=0;s.velocity={};s.velocity_spread=0;
    auto g=take(make_emitter_particle_output_graph(s,nid(1),nid(2),nid(3),eid(1),eid(2)));
    set(node(g,2),kParticleShape,std::uint32_t{4});set(node(g,2),kSizeStart,8.);set(node(g,2),kSizeEnd,100.);
    set(node(g,2),kOpacityStart,.5);set(node(g,2),kOpacityEnd,100.);set(node(g,2),kSizeRandom,0.);set(node(g,2),kOpacityRandom,0.);return g;}
void attach(Graph& g,unsigned model_id,unsigned particle_id,unsigned edge_id,unsigned resource=0,double x=0){g.nodes.push_back(model(model_id,resource,x));g.edges.push_back({eid(edge_id),nid(model_id),kModelGeometryOut,nid(particle_id),kParticleModelsIn});}
EvaluatedGraph evaluate(const Graph& g,RationalTime time={1,1}){return take(evaluate_particle_graph(g,time,never));}
RenderRequest request(const Graph& g,RationalTime time={0,1}){RenderRequest r;r.graph=std::make_shared<const Graph>(g);r.frame.layer_width=r.frame.layer_height=r.frame.frame_width=r.frame.frame_height=32;
    r.frame.region_of_interest={0,0,32,32};r.frame.time=time;r.frame.frame_duration={1,60};r.frame.format=PixelFormat::rgba32f;return r;}
std::array<float,4> pixel(const RenderOutput& out,unsigned x=16,unsigned y=16){std::array<float,4> p;std::memcpy(p.data(),out.pixels.data()+y*out.row_bytes+x*16,16);return p;}
void same_motion(const EvaluatedGraph& a,const EvaluatedGraph& b){check(a.particles.size()==b.particles.size(),"Model input cannot change population");
    for(std::size_t i=0;i<a.particles.size();++i){const auto& p=a.particles[i];const auto& q=b.particles[i];
        check(p.id==q.id&&p.emitter_id==q.emitter_id,"Model input retains birth identity");
        check(p.age_seconds==q.age_seconds&&p.lifetime_seconds==q.lifetime_seconds,"Model input retains birth/life");
        check(p.position.x==q.position.x&&p.position.y==q.position.y&&p.position.z==q.position.z,"mesh offset does not move logical centers");
        check(p.size_pixels==q.size_pixels&&p.opacity==q.opacity,"Model input retains logical size/opacity");}}
struct Sampler:TemporalGraphSampler {
    Graph graph;bool animate_matrix{},animate_shape{},bad_kind{},bad_identity{},duplicate{},fail_model{};
    std::vector<std::pair<NodeId,double>> model_samples;
    explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double seconds)override {
        for(const auto& n:graph.nodes)if(n.id==id){auto copy=n;
            if(n.type_key==kModelNode){model_samples.emplace_back(id,seconds);
                if(fail_model)return Result<GraphNode>::failure(ErrorCode::allocation_failed,"fixture model sampling fails");
                if(animate_matrix){auto m=kIdentityModelMatrix;m[12]=seconds;set(copy,kModelLocalMatrix,matrix_bytes(m));}
                if(bad_kind)set(copy,kModelResource,std::uint32_t{1});
                if(bad_identity)copy.id=nid(900);
                if(duplicate)copy.parameters.push_back(copy.parameters.front());
            }
            if(n.type_key==kParticleNode && animate_shape)set(copy,kParticleShape,std::uint32_t{seconds<.5?4u:0u});
            return Result<GraphNode>::success(std::move(copy));
        }
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"fixture sampled node missing");
    }
    Result<double> rate(NodeId id,double)override {for(const auto& n:graph.nodes)if(n.id==id)return Result<double>::success(std::get<double>(get(n,kBirthRate)));return Result<double>::failure(ErrorCode::invalid_request,"fixture rate missing");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id)override {auto rate_value=rate(id,0);if(!rate_value.has_value())return Result<std::optional<EmissionRateProfile>>::failure(rate_value.error());EmissionRateProfile p;p.constant=rate_value.value();return Result<std::optional<EmissionRateProfile>>::success(std::move(p));}
    std::optional<double> lifetime_upper_bound(NodeId id)override {for(const auto& n:graph.nodes)if(n.id==id)return std::get<double>(get(n,kParticleLifetimeSeconds));return {};}
};
EvaluatedGraph temporal(const Graph& g,RationalTime time={1,1}){Sampler s(g);return take(evaluate_temporal_particle_graph(g,time,never,{},s));}
void rejected(const auto& r,ErrorCode code,const char* label){check(!r.has_value()&&r.error().code==code,label);}
void topology() {
    auto g=base();auto implicit=evaluate(g);check(!implicit.particles.empty()&&implicit.model_styles.empty(),"shape4 supports implicit cube in live graph");
    attach(g,100,2,3,12,2);attach(g,10,2,4,11,-2);auto explicit_models=evaluate(g);
    check(explicit_models.model_styles.size()==1&&explicit_models.model_styles[0].instances.size()==2,"multiple Model inputs make one shared group");
    const auto& group=explicit_models.model_styles[0].instances;
    check(group[0].resource[0]==11&&group[1].resource[0]==12&&group[0].model_to_particle[12]==-2,"Model group order is source UUID order");
    for(const auto& p:explicit_models.particles)check(p.shape==4&&p.model_style_index==1,"live particle references its shared model group");
    same_motion(implicit,explicit_models);auto timed=temporal(g);same_motion(explicit_models,timed);
    check(timed.model_styles[0].instances[1].resource[0]==12,"temporal grouping matches static");
    auto before=[&](const EvaluatedGraph& e,NodeId a,NodeId b){return std::find(e.evaluated_nodes.begin(),e.evaluated_nodes.end(),a)<std::find(e.evaluated_nodes.begin(),e.evaluated_nodes.end(),b);};
    check(before(explicit_models,nid(100),nid(2))&&before(timed,nid(100),nid(2)),"full dependencies execute Models before Particle");
    auto wire=take(serialize_graph(g,particle_node_registry()));auto restored=take(deserialize_graph(wire,particle_node_registry()));
    auto roundtrip=evaluate(restored);check(take(encode_evaluated_particles(roundtrip,{1,1}))==take(encode_evaluated_particles(explicit_models,{1,1})),"graph wire preserves Model metadata and snapshot8");
    check(wire.size()<16384,"graph contains metadata rather than mesh arrays");
    std::reverse(g.nodes.begin(),g.nodes.end());std::reverse(g.edges.begin(),g.edges.end());
    check(take(serialize_graph(g,particle_node_registry()))==wire,"Model graph source ordering is canonical");
    check(take(encode_evaluated_particles(evaluate(g),{1,1}))==take(encode_evaluated_particles(explicit_models,{1,1})),"Model execution does not depend on insertion order");
    auto partitions=base();auto second=node(partitions,2);second.id=nid(7);partitions.nodes.push_back(second);
    partitions.edges.push_back({eid(5),nid(1),kEmitterParticles,nid(7),kParticleParticlesIn});partitions.edges.push_back({eid(6),nid(7),kParticleParticlesOut,nid(3),kOutputParticles});
    for(auto& n:partitions.nodes)if(n.type_key==kParticleNode)n.parameters.erase(std::remove_if(n.parameters.begin(),n.parameters.end(),[](const auto& p){return p.key==kParticleSeedShift||p.key==kParticleBirthChance;}),n.parameters.end());
    auto old=evaluate(partitions);attach(partitions,100,2,10);same_motion(old,evaluate(partitions));same_motion(evaluate(partitions),temporal(partitions));
    auto fanout=base();auto sibling=node(fanout,2);sibling.id=nid(7);fanout.nodes.push_back(sibling);
    fanout.edges.push_back({eid(5),nid(1),kEmitterParticles,nid(7),kParticleParticlesIn});fanout.edges.push_back({eid(6),nid(7),kParticleParticlesOut,nid(3),kOutputParticles});attach(fanout,10,2,10);
    fanout.edges.push_back({eid(11),nid(10),kModelGeometryOut,nid(7),kParticleModelsIn});auto shared=evaluate(fanout);
    check(shared.model_styles.size()==2,"one Model can feed distinct Particle groups");same_motion(shared,temporal(fanout));
}
void temporal_sampling() {
    auto g=base();attach(g,100,2,3);g.nodes.push_back(model(200));Sampler s(g);s.animate_matrix=true;s.animate_shape=true;
    auto evaluated=take(evaluate_temporal_particle_graph(g,{1,1},never,{},s));
    check(s.model_samples.size()==1&&s.model_samples[0]==std::pair{nid(100),1.},"only active Model is sampled once at current frame");
    check(evaluated.model_styles[0].instances[0].model_to_particle[12]==1,"matrix is current-frame metadata rather than birth-time metadata");
    unsigned models=0,others=0;for(const auto& p:evaluated.particles){if(p.shape==4){++models;check(p.model_style_index==1,"birth Model shape uses captured current group");}else{++others;check(p.model_style_index==0,"birth nonModel shape has no group index");}}
    check(models&&others,"shape at birth can animate across Model and Circle");
    auto snapshot=take(encode_evaluated_particles(evaluated,{1,1}));check(take(encode_evaluated_particles(take(decode_evaluated_particles(snapshot,{1,1})),{1,1}))==snapshot,"animated shapes retain Model group snapshot");
    Sampler negative(g);negative.fail_model=true;check(take(evaluate_temporal_particle_graph(g,{-1,60},never,{},negative)).particles.empty()&&negative.model_samples.empty(),"negative shutter frame does not sample Models");
    auto zero=g;set(node(zero,3),kParticleCount,std::uint32_t{0});Sampler zero_sampler(zero);zero_sampler.fail_model=true;
    check(take(evaluate_temporal_particle_graph(zero,{1,1},never,{},zero_sampler)).particles.empty()&&zero_sampler.model_samples.empty(),"zero-cap frame does not sample Models");
    for(unsigned failure=0;failure<4;++failure){Sampler bad(g);bad.bad_kind=failure==0;bad.bad_identity=failure==1;bad.duplicate=failure==2;bad.fail_model=failure==3;
        rejected(evaluate_temporal_particle_graph(g,{1,1},never,{},bad),failure==3?ErrorCode::allocation_failed:ErrorCode::invalid_request,"sampled Model errors propagate typed");}
    set(node(g,3),kTimeRemapEnabled,std::uint32_t{1});set(node(g,3),kTimeRemapSeconds,2.);Sampler remapped(g);remapped.animate_matrix=true;
    auto result=take(evaluate_temporal_particle_graph(g,{1,1},never,{},remapped));check(remapped.model_samples[0].second==2&&result.model_styles[0].instances[0].model_to_particle[12]==2,"Model current time follows remapping");
    set(node(g,3),kTimeRemapEnabled,std::uint32_t{0});for(auto t:{RationalTime{3,4},RationalTime{5,4}}){Sampler shutter(g);shutter.animate_matrix=true;
        auto sample=take(evaluate_temporal_particle_graph(g,t,never,{},shutter));check(shutter.model_samples.size()==1&&sample.model_styles[0].instances[0].model_to_particle[12]==to_seconds(t),"each shutter frame samples Model metadata at that frame");}
}
void auxiliary() {
    auto g=base();auto emitter=node(g,1);emitter.id=nid(4);set(emitter,kAuxiliarySource,std::uint32_t{1});set(emitter,kEmitterOrigin,Vec3{});
    auto child=node(g,2);child.id=nid(5);g.nodes.push_back(emitter);g.nodes.push_back(child);
    g.edges.push_back({eid(3),nid(2),kParticleParticlesOut,nid(4),kEmitterParents});g.edges.push_back({eid(4),nid(4),kEmitterParticles,nid(5),kParticleParticlesIn});g.edges.push_back({eid(5),nid(5),kParticleParticlesOut,nid(3),kOutputParticles});
    auto original=evaluate(g);attach(g,100,2,10,13,2);attach(g,101,5,11,0,-2);auto models=evaluate(g);same_motion(original,models);same_motion(models,temporal(g));
    unsigned parents=0,children=0;for(const auto& p:models.particles){if(p.emitter_id==nid(1)){++parents;check(p.model_style_index==1,"Auxiliary root-only copy retains parent group index");}else{++children;check(p.model_style_index==2,"Auxiliary child uses own group index");}}
    check(parents&&children,"Auxiliary fixture renders primary and child particles");
    Sampler sampled(g);sampled.animate_matrix=true;auto result=take(evaluate_temporal_particle_graph(g,{1,1},never,{},sampled));
    check(sampled.model_samples.size()==2,"Auxiliary prefixes do not request Model assets at historical births");
    for(const auto& p:result.particles)check(p.model_style_index==(p.emitter_id==nid(1)?1u:2u),"temporal Auxiliary Model groups remain distinct");
    g.edges.erase(std::remove_if(g.edges.begin(),g.edges.end(),[](const auto& e){return e.source_node==nid(2)&&e.destination_node==nid(3);}),g.edges.end());
    auto r=request(g,{1,1});check(CpuParticleRenderer{}.render(r,never).has_value(),"physics-only parent Model does not require capturing its unused mesh");
}
void bounds_and_rendering() {
    auto g=base();attach(g,10,2,3);auto out=take(CpuParticleRenderer{}.render(request(g),never));check(std::abs(pixel(out)[3]-.5)<1e-6,"live Model graph renders cube pixels");
    attach(g,11,2,4);auto doubled=take(CpuParticleRenderer{}.render(request(g),never));check(doubled.pixels==out.pixels,"multiple live Models retain one logical opacity");
    auto parked=node(g,2);parked.id=nid(7);g.nodes.push_back(parked);attach(g,100,7,10,77);
    auto evaluated=evaluate(g);check(evaluated.model_styles.size()==2,"parked group participates in stable catalogue");check(CpuParticleRenderer{}.render(request(g),never).has_value(),"parked resource presence does not fail active rendering");
    auto cube_graph=base();cube_graph.nodes.push_back({nid(10),kModelNode,1,{}});cube_graph.edges.push_back({eid(3),nid(10),kModelGeometryOut,nid(2),kParticleModelsIn});
    check(take(CpuParticleRenderer{}.render(request(cube_graph),never)).pixels==out.pixels,"missing metadata chooses builtin cube defaults");
    auto invalid=cube_graph;node(invalid,10).schema_version=2;rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"unknown Model schema rejects");
    for(unsigned n:{0u,15u,17u}){invalid=cube_graph;set(node(invalid,10),kModelResource,OpaqueBytes(n));rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"invalid resource byte lengths reject");}
    for(unsigned n:{0u,127u,129u}){invalid=cube_graph;set(node(invalid,10),kModelLocalMatrix,OpaqueBytes(n));rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"invalid local matrix byte lengths reject");}
    for(unsigned variant=0;variant<4;++variant){invalid=cube_graph;auto m=kIdentityModelMatrix;if(variant==0)m[0]=std::numeric_limits<double>::quiet_NaN();if(variant==1)m[0]=1e13;if(variant==2)m[3]=1;if(variant==3)m[15]=0;set(node(invalid,10),kModelLocalMatrix,matrix_bytes(m));
        rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"invalid local matrices reject");}
    invalid=cube_graph;set(node(invalid,10),kModelRevision,1.);rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"wrong revision kind rejects");
    invalid=cube_graph;invalid.edges.back().destination_port=kParticleParticlesIn;rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"Model cannot enter particle flow port");
    invalid=cube_graph;invalid.edges.back().destination_node=nid(3);invalid.edges.back().destination_port=kOutputParticles;rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"Model cannot directly feed Output");
    invalid=cube_graph;invalid.edges.push_back({eid(4),nid(10),kModelGeometryOut,nid(2),kParticleModelsIn});rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"duplicate Model connection rejects");
    invalid=cube_graph;set(node(invalid,2),kParticleShape,std::uint32_t{5});rejected(evaluate_particle_graph(invalid,{1,1},never),ErrorCode::invalid_request,"unknown Particle shape still rejects");
    auto many=base();for(unsigned i=0;i<256;++i)attach(many,100+i,2,1000+i);check(evaluate(many).model_styles[0].instances.size()==256,"exact group cap accepted");
    check(std::abs(pixel(take(CpuParticleRenderer{}.render(request(many),never)))[3]-.5)<1e-6,"256 meshes still compose once per logical particle");
    attach(many,400,2,1400);rejected(evaluate_particle_graph(many,{1,1},never),ErrorCode::invalid_request,"one-over group connection cap rejects");
}
void transport() {
    auto g=base();attach(g,10,2,3,1);auto r=request(g);auto graph_bytes=take(serialize_graph(g,particle_node_registry()));
    SfCoreApi api{};check(StarfieldCore_GetApi(8,sizeof(api),&api)==1,"live Model ABI8 negotiates");
    SfCoreRenderRequest input{};input.struct_size=sizeof(input);input.frame={32,32,32,32,{0,0,32,32},0,1,1,60,2,0,0,1,1};input.graph_bytes=graph_bytes.data();input.graph_byte_count=graph_bytes.size();
    SfModelPosition positions[]{{-.5,-.5,0,1},{.5,-.5,0,1},{-.5,.5,0,1}};SfModelTriangle triangle{};for(unsigned i=0;i<3;++i)triangle.corners[i]={i,kMissingModelAttribute,kMissingModelAttribute};
    SfModelSource source{};source.struct_size=sizeof(source);source.resource_id[0]=1;source.position_count=3;source.positions=positions;source.triangle_count=1;source.triangles=&triangle;input.model_source_count=1;input.model_sources=&source;
    SfCoreRenderResult output{};output.struct_size=sizeof(output);check(api.render(&input,&output)==SF_CORE_OK&&output.pixels,"C API renders imported mesh from live graph");
    std::array<float,4> p{};if(output.pixels)std::memcpy(p.data(),static_cast<const std::byte*>(output.pixels)+18*output.row_bytes+13*16,16);check(std::abs(p[3]-.5)<1e-6,"C API imported triangle pixels have one-particle opacity");api.release_render_result(&output);
    input.model_source_count=0;input.model_sources=nullptr;check(api.render(&input,&output)==SF_CORE_INVALID_REQUEST&&!output.pixels,"live missing mesh returns typed without pixels");
}
void force_transform() {
    auto g=base();g.nodes.push_back({nid(7),kForceNode,3,{{kGravity,Vec3{.03,-.12,.04}},{kLinearDrag,0.}}});
    g.nodes.push_back({nid(8),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{.2,-.1,.3}},
        {kTransformRotation,Vec3{10,25,40}},{kTransformSystemScale,Vec3{120,80,150}},
        {kTransformParticleScale,75.},{kTransformParticleOpacity,65.},{kTransformInheritLayer,std::uint32_t{0}}}});
    g.edges.pop_back();g.edges.push_back({eid(2),nid(2),kParticleParticlesOut,nid(7),PortKey{1}});
    g.edges.push_back({eid(3),nid(7),PortKey{2},nid(8),PortKey{1}});g.edges.push_back({eid(4),nid(8),PortKey{2},nid(3),kOutputParticles});
    auto no_metadata=evaluate(g);attach(g,100,2,10,0,.2);auto authored=evaluate(g);same_motion(no_metadata,authored);
    auto timed=temporal(g);check(timed.particles.size()==authored.particles.size(),"Model/Force/Transform temporal population matches");
    for(std::size_t i=0;i<authored.particles.size();++i){const auto& p=authored.particles[i];const auto& q=timed.particles[i];
        check(p.model_style_index==1&&q.model_style_index==1&&p.sprite_basis_index&&q.sprite_basis_index,"Model group and Transform basis survive Force path");
        check(p.id==q.id&&p.emitter_id==q.emitter_id,"Model/Force/Transform identity matches");
        check(std::abs(p.position.x-q.position.x)<1e-8&&std::abs(p.position.y-q.position.y)<1e-8&&std::abs(p.position.z-q.position.z)<1e-8,"Model does not interfere with temporal Force integration");}
    auto r=request(g,{1,1});check(CpuParticleRenderer{}.render(r,never).has_value(),"live Model/Force/Transform chain renders");
}
void linear_shutter() {
    auto g=base();attach(g,10,2,3);auto first=evaluate(g);auto last=first;
    last.model_styles[0].instances[0].model_to_particle[12]=2;
    last.model_styles[0].instances[0].model_to_particle[0]=-1;
    const auto mix=[&](const EvaluatedGraph& a,const EvaluatedGraph& b,double t=.5){return interpolate_motion_particles(a,b,1,1,t,64,never);};
    auto mid=take(mix(first,last));check(mid.model_styles.size()==1,"Linear shutter reuses one group for all particles");
    check(mid.model_styles[0].instances[0].model_to_particle[12]==1&&mid.model_styles[0].instances[0].model_to_particle[0]==0,"Linear shutter interpolates affine including singular midpoint");
    for(const auto& p:mid.particles)check(p.model_style_index==1,"Linear shutter remaps shared Model indices");
    auto wire=take(encode_evaluated_particles(mid,{1,1}));check(take(decode_evaluated_particles(wire,{1,1})).model_styles.size()==1,"Linear Model group retains snapshot8");
    auto r=request(g,{1,1});auto frozen=g;frozen.optional_records.push_back(wire);r.graph=std::make_shared<const Graph>(frozen);
    check(CpuParticleRenderer{}.render(r,never).has_value(),"Linear Model snapshot renders");
    auto moving=mid;moving.model_styles[0].instances[0].model_to_particle[0]=1;
    frozen.optional_records={take(encode_evaluated_particles(moving,{1,1}))};r.graph=std::make_shared<const Graph>(frozen);
    auto shifted=take(CpuParticleRenderer{}.render(r,never));
    check(pixel(shifted,16,16)[3]==0&&pixel(shifted,24,16)[3]>0,"interpolated Model matrix changes actual rendered pixels");
    auto implicit=first;implicit.model_styles.clear();for(auto& p:implicit.particles)p.model_style_index=0;
    auto converted=take(mix(implicit,last));check(converted.model_styles.size()==1&&converted.model_styles[0].instances[0].model_to_particle[12]==1,"implicit cube interpolates with explicit cube");
    converted=take(mix(last,implicit));check(converted.model_styles.size()==1&&converted.model_styles[0].instances[0].model_to_particle[12]==1,"explicit cube interpolates with implicit cube");
    auto both=take(mix(implicit,implicit));check(both.model_styles.empty()&&both.particles.front().model_style_index==0,"implicit endpoints do not allocate Model groups");
    auto changed=last;changed.model_styles[0].instances[0].resource[0]=9;
    auto held=take(mix(first,changed));check(held.model_styles[0].instances[0].resource[0]==0&&held.model_styles[0].instances[0].model_to_particle[12]==0,"changed resource retains first endpoint geometry");
    held=take(mix(implicit,changed));check(held.model_styles.empty()&&held.particles.front().model_style_index==0,"changed imported resource retains implicit first cube");
    changed=last;changed.model_styles[0].instances.push_back({});held=take(mix(first,changed));
    check(held.model_styles[0].instances.size()==1&&held.model_styles[0].instances[0].model_to_particle[12]==0,"changed group member count remains discrete");
    auto empty=first;empty.particles.clear();empty.model_styles.clear();auto births=take(mix(empty,last));
    check(births.particles.size()==last.particles.size()&&births.model_styles.size()==1&&births.model_styles[0].instances[0].model_to_particle[12]==2,"one endpoint birth retains last Model table");
    auto deaths=take(mix(first,empty));check(deaths.particles.size()==first.particles.size()&&deaths.model_styles.size()==1,"one endpoint death retains first Model table");
    auto remapped=first;remapped.model_styles.insert(remapped.model_styles.begin(),ParticleModelStyle{{ParticleModelInstance{}}});for(auto& p:remapped.particles)p.model_style_index=2;
    held=take(mix(remapped,last));check(held.model_styles.size()==1&&held.particles.front().model_style_index==1,"endpoint table indices do not leak into result");
    auto circles=implicit;for(auto& p:circles.particles)p.shape=0;
    held=take(mix(first,circles));check(held.model_styles.size()==1&&held.particles.front().shape==4,"shape transition retains chosen Model geometry");
    held=take(mix(circles,last));check(held.model_styles.empty()&&held.particles.front().shape==0,"shape transition retains chosen primitive geometry");
    for(unsigned kind=0;kind<6;++kind){auto invalid=first;
        if(kind==0)invalid.particles.front().model_style_index=2;
        if(kind==1)invalid.particles.front().shape=0;
        if(kind==2)invalid.model_styles[0].instances.clear();
        if(kind==3)invalid.model_styles[0].instances.resize(kMaxModelsPerStyle+1);
        if(kind==4)invalid.model_styles[0].instances[0].model_to_particle[3]=1;
        if(kind==5)invalid.model_styles.resize(kMaxModelStyles+1,invalid.model_styles[0]);
        rejected(mix(invalid,last),ErrorCode::invalid_request,"invalid Model endpoint rejects before interpolation");
        rejected(mix(first,invalid),ErrorCode::invalid_request,"invalid last Model endpoint rejects before interpolation");}
    struct Cancelled:Cancellation{bool is_cancelled()const noexcept override{return true;}} cancelled;
    rejected(interpolate_motion_particles(first,last,1,1,.5,64,cancelled),ErrorCode::cancelled,"Model shutter respects cancellation");
    auto crowded=first;crowded.model_styles.resize(kMaxModelStyles,crowded.model_styles[0]);
    const auto seed=crowded.particles.front();crowded.particles.resize(kMaxModelStyles+1,seed);
    for(std::size_t i=0;i<crowded.particles.size();++i){crowded.particles[i].id=i;crowded.particles[i].model_style_index=static_cast<std::uint32_t>(i%kMaxModelStyles+1);}
    auto crowded_last=crowded;crowded_last.particles.back().model_style_index=2;
    rejected(interpolate_motion_particles(crowded,crowded_last,1,1,.5,8192,never),ErrorCode::work_limit_exceeded,"Linear Model group pair cap rejects before excessive staging");
}
}
int main(){try{topology();temporal_sampling();auxiliary();bounds_and_rendering();transport();force_transform();linear_shutter();std::printf("Model graph: %u checks passed\n",checks);return 0;}catch(const std::exception& e){std::printf("FAILED at check %u: %s\n",checks,e.what());return 1;}}
