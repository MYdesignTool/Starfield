#include "starfield/core/ParticleCloud.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "../src/core/SpriteGeometry.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

// Execute the actual shared CUDA/OpenCL kernel as scalar C++ for numeric parity.
// This is not hardware/AE qualification.
namespace shader {
using sf_uint=std::uint32_t;
sf_uint thread_x{},thread_y{};
inline sf_uint get_global_id(unsigned axis){return axis?thread_y:thread_x;}
#define __global
#define __kernel inline
#include "../ae_plugin/gpu/SpriteKernel.h"
#undef SF_INLINE
#undef SF_KERNEL
#undef SF_GLOBAL
#undef SF_X
#undef SF_Y
#undef __global
#undef __kernel
}
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
unsigned checks{};
NeverCancelled never;
struct Cancelled:Cancellation {bool is_cancelled() const noexcept override{return true;}} cancelled;
void check(bool v,const char* message){++checks;if(!v)throw std::runtime_error(message);}
void near(double v,double expected,const char* message){check(std::abs(v-expected)<1.e-6,message);}
template<class T>T take(Result<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
template<class T>T take(SequenceResult<T> r){if(!r.has_value())throw std::runtime_error(r.error().detail);return r.take_value();}
void set(GraphNode& n,ParameterKey key,ParameterValue v){for(auto& p:n.parameters)if(p.key==key){p.value=std::move(v);return;}n.parameters.push_back({key,std::move(v)});}
Graph base_graph(){NodeId e{},p{},o{};EdgeId ep{},po{};e.value.bytes[0]=1;p.value.bytes[0]=2;o.value.bytes[0]=3;ep.value.bytes[0]=4;po.value.bytes[0]=5;
    Settings s;s.particle_count=8;s.birth_rate=4;s.particle_lifetime_seconds=3;s.velocity={};s.velocity_spread=0;
    return take(make_emitter_particle_output_graph(s,e,p,o,ep,po));}
GraphNode& particle_node(Graph& g){for(auto& n:g.nodes)if(n.type_key==kParticleNode)return n;throw std::runtime_error("no Particle");}
ParticleInstance particle(std::uint64_t id=1){ParticleInstance p;p.id=id;p.emitter_id.value.bytes[0]=1;p.age_seconds=.25;p.lifetime_seconds=3;
    p.size_pixels=12;p.size_y_pixels=30;p.opacity=.4;p.shape=2;p.cloud_style_index=1;p.cloud_random_key=0x812345;return p;}
EvaluatedGraph evaluated(ParticleCloudStyle s={}){EvaluatedGraph g;g.cloud_styles.push_back(s);g.particles.push_back(particle());return g;}
RenderRequest request(EvaluatedGraph value,RationalTime t={1,1}){RenderRequest r;r.settings=validate_settings(Settings{});
    r.frame.layer_width=r.frame.layer_height=r.frame.frame_width=r.frame.frame_height=64;r.frame.region_of_interest={0,0,64,64};
    r.frame.time=t;r.frame.frame_duration={1,30};r.frame.format=PixelFormat::rgba32f;
    auto g=base_graph();g.optional_records.push_back(take(encode_evaluated_particles(value,t)));r.graph=std::make_shared<const Graph>(std::move(g));return r;}
std::array<float,4> pixel(const RenderOutput& out,unsigned x,unsigned y){std::array<float,4> v{};
    std::memcpy(v.data(),out.pixels.data()+std::size_t(y)*out.row_bytes+x*16,16);return v;}
void generation(){
    check(valid_cloud_style({10,150,1000}),"owner Density maximum accepted");
    for(auto s:{ParticleCloudStyle{0,150,66},{1001,150,66},{10,0,66},{10,1001,66},{10,150,-1},{10,150,1001}})
        check(!valid_cloud_style(s),"Cloud authored bounds");
    check(!valid_cloud_style({10,std::numeric_limits<double>::quiet_NaN(),66}),"Cloud NaN rejected");
    std::array<CloudCircle,1000> many{};make_cloud_circles({1000,150,1000},0xabc123,many);
    near(many[0].radius,1,"central full-radius member");
    for(unsigned i=1;i<many.size();++i){check(std::hypot(many[i].x/1.5,many[i].y)<=10+1.e-9,"centers bounded by Density");
        check(many[i].radius>=.35 && many[i].radius<1,"stable member radii");}
    auto a=cloud_circle({10,150,66},123,4),b=cloud_circle({1000,150,1000},123,4);
    near(b.x/a.x,1000./66,"Density scales member X");near(b.y/a.y,1000./66,"Density scales member Y");near(a.radius,b.radius,"Density preserves radius");
    auto aspect=cloud_circle({10,300,66},123,4);near(aspect.x,2*a.x,"Aspect changes horizontal centers");near(aspect.y,a.y,"Aspect preserves vertical centers");
    auto changed=cloud_circle({10,150,66},124,4);check(changed.x!=a.x || changed.y!=a.y,"different random key changes arrangement");
    auto zero=cloud_circle({1000,150,0},123,4);near(zero.x,0,"zero-density X");near(zero.y,0,"zero-density Y");
    std::array<CloudCircle,10> prefix{};make_cloud_circles({10,150,1000},0xabc123,prefix);
    for(unsigned i=0;i<prefix.size();++i){near(prefix[i].x,many[i].x,"count preserves member prefix");near(prefix[i].radius,many[i].radius,"count preserves radii prefix");}
}
void graph_and_history(){
    auto g=base_graph();set(particle_node(g),kParticleShape,std::uint32_t{2});
    auto first=take(evaluate_particle_graph(g,{1,1},never));check(first.particles.size()==5 && first.cloud_styles.size()==1,"logical population unchanged, shared style");
    check(first.cloud_styles[0].circles==10 && first.cloud_styles[0].aspect==150 && first.cloud_styles[0].density==66,"reference defaults");
    auto next=take(evaluate_particle_graph(g,{3,2},never));auto repeat=take(evaluate_particle_graph(g,{1,1},never));
    for(const auto& p:first.particles){check(p.cloud_style_index==1 && p.cloud_random_key<=0xffffff,"compact Cloud style/key");
        auto found=std::find_if(next.particles.begin(),next.particles.end(),[&](const auto& x){return x.id==p.id;});
        check(found!=next.particles.end() && found->cloud_random_key==p.cloud_random_key,"time preserves arrangement");}
    check(repeat.particles[0].cloud_random_key==first.particles[0].cloud_random_key,"reverse/repeat deterministic");
    auto sequence=take(serialize_graph(g,particle_node_registry()));auto restored=take(deserialize_graph(sequence,particle_node_registry()));
    check(static_cast<bool>(validate_graph(restored,particle_node_registry())),"optional Cloud keys roundtrip");
    set(particle_node(g),kCloudDensity,1001.);check(!evaluate_particle_graph(g,{1,1},never).has_value(),"graph Density rejected");
    set(particle_node(g),kCloudDensity,1000.);set(particle_node(g),kCloudCircles,std::uint32_t{0});
    check(!evaluate_particle_graph(g,{1,1},never).has_value(),"zero Circles rejected");
    auto& params=particle_node(g).parameters;std::erase_if(params,[](const auto& p){return p.key==kCloudCircles || p.key==kCloudAspect || p.key==kCloudDensity;});
    auto legacy=take(evaluate_particle_graph(g,{1,1},never));check(legacy.cloud_styles.empty() && legacy.particles[0].cloud_style_index==0,"old graph keeps fixed five-circle");
    struct History:TemporalGraphSampler {Graph g;explicit History(Graph value):g(std::move(value)){}
        Result<GraphNode> node(NodeId id,double t)override{for(const auto& n:g.nodes)if(n.id==id){auto copy=n;if(n.type_key==kParticleNode)set(copy,kCloudDensity,t*100);return Result<GraphNode>::success(copy);}return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing node");}
        Result<double> rate(NodeId,double)override{return Result<double>::success(4);}
        Result<std::optional<EmissionRateProfile>> rate_profile(NodeId)override{return Result<std::optional<EmissionRateProfile>>::success(EmissionRateProfile{4,{}});}
        std::optional<double> lifetime_upper_bound(NodeId)override{return 3;}
    } sampler(restored);
    auto history=take(evaluate_temporal_particle_graph(restored,{1,1},never,{64,1},sampler));
    check(history.particles.size()==5 && history.cloud_styles.size()==5,"historical Cloud styles retained at birth");
    for(const auto& p:history.particles)near(history.cloud_styles[p.cloud_style_index-1].density,(1-p.age_seconds)*100,"birth-style identity");
    check(evaluate_particle_graph(restored,{1,1},cancelled).error().code==ErrorCode::cancelled,"graph cancellation");
}
void wire(){
    auto g=evaluated();auto bytes=take(encode_evaluated_particles(g,{1,1}));check(std::to_integer<unsigned>(bytes[2])==7,"snapshot7 emitted");
    check(bytes.size()==56+24+200,"Cloud table and unchanged200 stride");
    auto decoded=take(decode_evaluated_particles(bytes,{1,1}));check(decoded.particles[0].cloud_style_index==1 && decoded.particles[0].cloud_random_key==0x812345,"Cloud packed words roundtrip");
    near(decoded.cloud_styles[0].density,66,"Cloud shared table roundtrip");
    for(std::size_t i=0;i<bytes.size();++i){auto short_bytes=bytes;short_bytes.resize(i);check(!decode_evaluated_particles(short_bytes,{1,1}).has_value(),"all truncated snapshots rejected");}
    for(auto at:{52u,60u}){auto bad=bytes;bad[at]=std::byte{1};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"Cloud reserved word checked");}
    auto bad=bytes;bad[48]=std::byte{2};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"Cloud table count checked");
    bad=bytes;bad[56]=std::byte{0};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"Cloud circle count checked");
    bad=bytes;bad[56+24+184+2]=std::byte{2};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"Cloud index checked");
    bad=bytes;bad[56+24+184]=std::byte{0};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"non-Cloud packed style rejected");
    g.particles[0].cloud_random_key=0x1000000;check(!encode_evaluated_particles(g,{1,1}).has_value(),"random key width checked");
    g=evaluated();g.particles[0].cloud_style_index=0;check(!encode_evaluated_particles(g,{1,1}).has_value(),"legacy Cloud cannot carry random key");
    g.cloud_styles.clear();g.particles[0].cloud_random_key=0;auto legacy=take(encode_evaluated_particles(g,{1,1}));
    check(std::to_integer<unsigned>(legacy[2])==3,"legacy snapshot3 retained");check(take(decode_evaluated_particles(legacy,{1,1})).cloud_styles.empty(),"snapshot3 remains readable");
    g.particles[0].transfer_mode=ParticleTransferMode::add;legacy=take(encode_evaluated_particles(g,{1,1}));check(std::to_integer<unsigned>(legacy[2])==5,"legacy transfer snapshot5 retained");
    g=evaluated();g.texture_styles.push_back({7,0});auto texture=particle(2);texture.shape=3;texture.cloud_style_index=texture.cloud_random_key=0;texture.texture_style_index=1;texture.texture_random_key=42;
    g.particles.push_back(texture);auto mixed=take(decode_evaluated_particles(take(encode_evaluated_particles(g,{1,1})),{1,1}));
    check(mixed.cloud_styles.size()==1 && mixed.texture_styles.size()==1 && mixed.particles[1].texture_random_key==42,"mixed Texture/Cloud tables and keys");
    check(decode_evaluated_particles(bytes,{2,1}).error().code==ErrorCode::invalid_request,"snapshot clock checked");
    check(encode_evaluated_particles(g,{1,1},&cancelled).error().code==ErrorCode::cancelled,"wire cancellation");
}
void motion(){
    auto first=evaluated({10,150,0}),last=first;last.cloud_styles.insert(last.cloud_styles.begin(),{6,100,200});last.cloud_styles[1].density=100;
    last.particles[0].cloud_style_index=2;last.particles[0].age_seconds+=1;
    auto born=particle(2);born.age_seconds=.1;born.cloud_style_index=1;last.particles.push_back(born);
    auto output=take(interpolate_motion_particles(first,last,1,2,.95,8,never));check(output.particles.size()==2 && output.cloud_styles.size()==2,"endpoint table remapping");
    for(const auto& p:output.particles){const auto& s=output.cloud_styles[p.cloud_style_index-1];near(s.density,p.id==1?95:200,"motion Density interpolation/source table");}
    last.cloud_styles[0].density=1001;check(!interpolate_motion_particles(first,last,1,2,.5,8,never).has_value(),"motion malformed style rejected");
    last=first;last.particles[0].cloud_style_index=2;check(!interpolate_motion_particles(first,last,1,2,.5,8,never).has_value(),"motion missing style rejected");
}
std::vector<float> kernel(const SpriteScene& scene){
    static_assert(sizeof(shader::SfSprite)==sizeof(SfGpuSprite));static_assert(sizeof(shader::SfCloudCircle)==sizeof(SfGpuCloudCircle));
    std::vector<shader::SfSprite> sprites(scene.sprites.size());std::vector<shader::SfCloudCircle> circles(scene.cloud_circles.size());
    if(!sprites.empty())std::memcpy(sprites.data(),scene.sprites.data(),sprites.size()*sizeof(SfGpuSprite));
    if(!circles.empty())std::memcpy(circles.data(),scene.cloud_circles.data(),circles.size()*sizeof(SfGpuCloudCircle));
    unsigned width=static_cast<unsigned>(scene.region.width()),height=static_cast<unsigned>(scene.region.height());std::vector<float> output(std::size_t(width)*height*4);
    for(unsigned y=0;y<height;++y)for(unsigned x=0;x<width;++x){shader::thread_x=x;shader::thread_y=y;
        shader::starfield_sprite_render(output.data(),sprites.data(),scene.offsets.data(),scene.indices.data(),circles.data(),width,height,width*4,scene.tiles_x,0,0,0,width,height,0,1,1);}
    return output;
}
void render(){
    auto zero=evaluated({1000,150,0});auto r=request(zero);auto output=take(CpuParticleRenderer{}.render(r,never));
    near(pixel(output,32,32)[3],.4,"collapsed Cloud applies group opacity once");
    auto circle=zero;circle.particles[0].shape=0;circle.particles[0].cloud_style_index=circle.particles[0].cloud_random_key=0;circle.cloud_styles.clear();
    auto circular=take(CpuParticleRenderer{}.render(request(circle),never));check(circular.pixels==output.pixels,"zero Density equals round Circle despite SizeY/Aspect");
    for(double density:{0.,66.,200.,1000.})for(auto transfer:{ParticleTransferMode::normal,ParticleTransferMode::add,ParticleTransferMode::screen,ParticleTransferMode::stencil}){
        auto g=evaluated({10,150,density});auto back=g.particles[0];back.id=2;back.color={1,.5,.25};back.cloud_random_key=456;
        g.particles.insert(g.particles.begin(),back);g.particles[1].transfer_mode=transfer;g.particles[1].rotation_degrees.z=23;
        g.sprite_bases.push_back({1,.2,0,.1,1,0,0,0,1});for(auto& p:g.particles)p.sprite_basis_index=1;
        r=request(g);r.frame.region_of_interest={7,9,60,58};auto cpu=take(CpuParticleRenderer{}.render(r,never));auto scene=take(prepare_sprite_scene(r,never));
        check(scene.sprites.size()==2 && scene.cloud_circles.size()==(density==0?2u:20u),"GPU precomputed members and collapsed optimization");
        for(const auto& s:scene.sprites)check(s.reserved[2] && s.reserved[1]+s.reserved[2]<=scene.cloud_circles.size(),"GPU circle range valid");
        auto gpu=kernel(scene);double maximum=0;for(unsigned y=0;y<cpu.height();++y)for(unsigned x=0;x<cpu.width();++x){auto c=pixel(cpu,x,y);auto* p=&gpu[(std::size_t(y)*cpu.width()+x)*4];
            maximum=std::max({maximum,double(std::abs(c[0]-p[2])),double(std::abs(c[1]-p[1])),double(std::abs(c[2]-p[0])),double(std::abs(c[3]-p[3]))});}
        check(maximum<2.e-5,"CPU/shared GPU kernel parity, crop, shear and transfer");
    }
    auto full=request(evaluated({10,150,200}));auto frame=take(CpuParticleRenderer{}.render(full,never));full.frame.region_of_interest={16,16,48,48};
    auto crop=take(CpuParticleRenderer{}.render(full,never));bool same=true;for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)same&=pixel(crop,x,y)==pixel(frame,x+16,y+16);check(same,"ROI independent Cloud pixels");
    RenderLimits limits;limits.max_sprite_pixel_ops=1;check(CpuParticleRenderer{limits}.render(full,never).error().code==ErrorCode::work_limit_exceeded,"member-weighted CPU work budget");
    check(prepare_sprite_scene(full,cancelled).error().code==ErrorCode::cancelled,"GPU scene cancellation");
    auto legacy=evaluated();legacy.cloud_styles.clear();legacy.particles[0].cloud_style_index=legacy.particles[0].cloud_random_key=0;
    auto scene=take(prepare_sprite_scene(request(legacy),never));check(scene.cloud_circles.empty() && scene.sprites[0].reserved[2]==0,"legacy GPU five-circle path retained");
    auto crowded=request(evaluated({1000,150,66}));auto crowded_scene=take(prepare_sprite_scene(crowded,never));
    auto crowded_cpu=take(CpuParticleRenderer{}.render(crowded,never));auto crowded_gpu=kernel(crowded_scene);
    check(crowded_scene.cloud_circles.size()==1000 && crowded_scene.sprites[0].reserved[2]==1000,"maximum member count kept without truncation");
    double worst=0;for(unsigned y=0;y<64;++y)for(unsigned x=0;x<64;++x)worst=std::max(worst,double(std::abs(pixel(crowded_cpu,x,y)[3]-crowded_gpu[(y*64+x)*4+3])));
    check(worst<2.e-5,"maximum-count kernel alpha parity");
    auto oversized=evaluated({1000,150,1000});oversized.particles.resize(1001,particle());
    for(unsigned i=0;i<oversized.particles.size();++i)oversized.particles[i].id=i;
    check(prepare_sprite_scene(request(oversized),never).error().code==ErrorCode::unsupported_format,"GPU member-weighted work budget rejects whole scene");
    oversized.particles.resize(2001,particle());for(unsigned i=0;i<oversized.particles.size();++i)oversized.particles[i].id=i;
    auto tiny=request(oversized);tiny.frame.layer_width=tiny.frame.layer_height=tiny.frame.frame_width=tiny.frame.frame_height=1;tiny.frame.region_of_interest={0,0,1,1};
    check(prepare_sprite_scene(tiny,never).error().code==ErrorCode::unsupported_format,"GPU2M member budget checked independently of pixel work");
}
void abi(){
    SfCoreApi api{};check(!StarfieldCore_GetApi(6,sizeof(api),&api),"old ABI rejected");check(StarfieldCore_GetApi(7,sizeof(api),&api)==1,"Cloud ABI7 available");
    static_assert(sizeof(SfGpuSprite)==80);static_assert(sizeof(SfGpuCloudCircle)==16);
    auto r=request(evaluated());auto bytes=take(serialize_graph(*r.graph,particle_node_registry()));
    SfCoreRenderRequest q{};q.struct_size=sizeof(q);q.frame={64,64,64,64,{0,0,64,64},1,1,1,30,2,0,1,1,1};
    q.graph_bytes=bytes.data();q.graph_byte_count=bytes.size();
    SfCoreGpuSceneResult result{};result.struct_size=sizeof(result);check(api.prepare_gpu_scene(&q,&result)==SF_CORE_OK,"ABI scene preparation");
    check(result.cloud_circle_count==10 && result.cloud_circles && result.sprites[0].reserved[2]==10,"ABI array belongs to scene lease");
    api.release_gpu_scene(&result);check(!result.opaque_handle && !result.cloud_circles && result.cloud_circle_count==0,"ABI releases added array");
}
}
int main(){try{generation();graph_and_history();wire();motion();render();abi();std::printf("Particle Cloud: %u checks passed (numeric contract only).\n",checks);return 0;}
    catch(const std::exception& e){std::printf("FAIL after %u checks: %s\n",checks,e.what());return 1;}}
