#include "starfield/core/ParticleTexture.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "../src/core/SpriteGeometry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <stdexcept>

using namespace starfield::core;
namespace {
unsigned checks{};
void check(bool value, const char* message) { ++checks; if(!value) throw std::runtime_error(message); }
void near(double value, double expected, const char* message) { check(std::abs(value-expected)<1.e-6,message); }
template<class T> T take(Result<T> result) { if(!result.has_value()) throw std::runtime_error(result.error().detail); return result.take_value(); }
NeverCancelled never;
class Cancelled : public Cancellation { bool is_cancelled() const noexcept override { return true; } } cancelled;

ParticleInstance particle(std::uint64_t id=1) {
    ParticleInstance p; p.id=id;p.emitter_id.value.bytes[0]=1;p.age_seconds=.25;p.lifetime_seconds=4;
    p.size_pixels=16;p.size_y_pixels=16;p.opacity=1;p.color={1,0,0};p.shape=3;p.texture_style_index=1;
    p.texture_random_key=0x800000;return p;
}
EvaluatedGraph evaluated() {
    EvaluatedGraph g;g.texture_styles.push_back({7,8,TextureTimeMode::current_time,TextureColorUse::source,1,0});
    g.particles.push_back(particle());return g;
}
Graph base_graph() {
    NodeId e{},p{},o{};EdgeId ep{},po{};e.value.bytes[0]=1;p.value.bytes[0]=2;o.value.bytes[0]=3;
    ep.value.bytes[0]=4;po.value.bytes[0]=5;
    auto result=make_emitter_particle_output_graph(Settings{},e,p,o,ep,po);
    return take(std::move(result));
}
void set(GraphNode& node,ParameterKey key,ParameterValue value) {
    for(auto& param:node.parameters)if(param.key==key){param.value=std::move(value);return;}
    node.parameters.push_back({key,std::move(value)});
}
RenderRequest request(EvaluatedGraph particles, RationalTime time={1,1}) {
    RenderRequest r;r.settings=validate_settings(Settings{});
    r.frame.layer_width=r.frame.layer_height=r.frame.frame_width=r.frame.frame_height=32;
    r.frame.region_of_interest={0,0,32,32};r.frame.time=time;r.frame.frame_duration={1,30};r.frame.format=PixelFormat::rgba32f;
    auto g=base_graph();g.optional_records.push_back(take(encode_evaluated_particles(particles,time)));
    r.graph=std::make_shared<const Graph>(std::move(g));return r;
}
std::array<float,4> pixel(const RenderOutput& output,unsigned x=16,unsigned y=16) {
    std::array<float,4> result{};
    std::memcpy(result.data(),output.pixels.data()+std::size_t(y)*output.row_bytes+x*16,16);return result;
}
void time_modes() {
    TextureSource s{7,-2,2,.5,10,20,1};
    check(take(texture_frame_count(s))==8,"negative clip frame count");
    const unsigned expected[]{4,2,2,2,4,6,6,2};
    for(unsigned mode=0;mode<8;++mode) check(take(texture_frame_index(s,static_cast<TextureTimeMode>(mode),.25,1.25,5,0x800000))==expected[mode],"eight sampling modes");
    check(take(texture_frame_index(s,TextureTimeMode::freeze_frame,1,1,5,0))==4,"birth frame");
    check(take(texture_frame_index(s,TextureTimeMode::freeze_frame,3,3,5,0))==4,"freeze remains birth frame");
    check(take(texture_frame_index(s,TextureTimeMode::current_time,-99,0,5,0))==0,"clip lower clamp");
    check(take(texture_frame_index(s,TextureTimeMode::current_time,99,0,5,0))==7,"clip last valid clamp");
    check(take(texture_frame_index(s,TextureTimeMode::random_still,0,0,5,0xffffff))==7,"random last frame");
    check(take(texture_frame_index(s,TextureTimeMode::loop,0,4.75,5,0))==1,"positive modulo");
    check(take(texture_frame_index(s,TextureTimeMode::random_loop,0,2.75,5,0x800000))==1,"random loop wrap");
    check(take(texture_frame_index(s,TextureTimeMode::stretch,0,5,5,0))==7,"stretch endpoint");
    TextureSource decimal{1,0,1,.1,1,1,1};check(take(texture_frame_count(decimal))==10,"decimal grid excludes phantom frame");
    near(take(texture_frame_index(decimal,TextureTimeMode::current_time,.3,0,1,0)),3,"decimal frame boundary");
    decimal.end_seconds=1.01;check(take(texture_frame_count(decimal))==11,"partial last frame retained");
    check(take(texture_frame_index(decimal,TextureTimeMode::loop,0,1.02,4,0))==0,"partial clip loops at true end");
    decimal.end_seconds=1.e-20;check(take(texture_frame_count(decimal))==1,"subframe clip has one frame");
    decimal.frame_seconds=0;check(!texture_frame_count(decimal).has_value(),"zero frame duration rejected");
    check(!texture_frame_index(s,TextureTimeMode::freeze_frame,0,-1,4,0).has_value(),"negative age rejected");
    check(!texture_frame_index(s,static_cast<TextureTimeMode>(8),0,0,4,0).has_value(),"unknown time mode rejected");
    check(!texture_frame_index(s,TextureTimeMode::current_time,0,0,4,0x1000000).has_value(),"random key width checked");
}
void planning_and_validation() {
    auto g=evaluated();g.particles.push_back(particle(2));
    std::vector<TextureSource> sources{{7,0,4,.5,1,1,1},{8,0,4,.5,1,1,1}};
    auto plan=take(plan_texture_frames(g,sources,1,never));
    check(plan.size()==2,"same frames deduplicated");check(plan[0].resource_id==7 && plan[1].resource_id==8,"stable source order");
    near(plan[0].seconds,1,"source checkout time");
    g.texture_styles[0].time_mode=TextureTimeMode::freeze_frame;g.particles[1].age_seconds=.75;
    plan=take(plan_texture_frames(g,sources,1,never));check(plan.size()==4,"distinct births request distinct frames");
    std::reverse(g.particles.begin(),g.particles.end());auto again=take(plan_texture_frames(g,sources,1,never));
    check(plan.size()==again.size() && plan[0].frame_index==again[0].frame_index,"planning independent of order");
    check(!plan_texture_frames(g,{sources.data(),1},1,never).has_value(),"deleted back source rejected");
    check(plan_texture_frames(g,sources,1,cancelled).error().code==ErrorCode::cancelled,"planner cancellation");
    g.texture_styles[0].front=g.texture_styles[0].back=0;check(take(plan_texture_frames(g,{},1,never)).empty(),"None sources request no texture");
    float rgba[]{1,0,0,1};TextureFrameView frame{7,2,1,1,4,rgba};
    check(take(validate_texture_resources(sources,{&frame,1},never)),"valid texture staging");
    TextureFrameView duplicate[]{frame,frame};check(!validate_texture_resources(sources,duplicate,never).has_value(),"duplicate frames rejected");
    frame.row_floats=3;check(!validate_texture_resources(sources,{&frame,1},never).has_value(),"short rows rejected");frame.row_floats=4;
    rgba[0]=std::numeric_limits<float>::quiet_NaN();check(!validate_texture_resources(sources,{&frame,1},never).has_value(),"NaN pixel rejected");rgba[0]=1;
    rgba[3]=2;check(!validate_texture_resources(sources,{&frame,1},never).has_value(),"alpha bounds");rgba[3]=1;
    frame={7,0,1,32768,8192,{}};
    check(validate_texture_resources(sources,{&frame,1},never).error().code==ErrorCode::work_limit_exceeded,"byte budget checked before pixels");
    check(validate_texture_resources(sources,{},cancelled).error().code==ErrorCode::cancelled,"validation cancellation");
    g=evaluated();g.texture_styles[0].back=0;g.texture_styles[0].time_mode=TextureTimeMode::random_still;
    g.particles.resize(4097,particle());sources={{7,0,4097,1,1,1,1}};
    for(unsigned i=0;i<g.particles.size();++i)g.particles[i].texture_random_key=static_cast<std::uint32_t>((std::uint64_t(i)*0x1000000+4096)/4097);
    check(plan_texture_frames(g,sources,0,never).error().code==ErrorCode::work_limit_exceeded,"frame request count bounded");
}
void filtering_color() {
    float rgba[]{.5f,0,0,.5f,0,0,0,0};TextureFrameView frame{7,0,2,1,8,rgba};
    const auto filtered=sample_texture(frame,.5,.5);near(filtered[0],.25,"premultiplied bilinear RGB");near(filtered[3],.25,"premultiplied bilinear alpha");
    auto source=color_texture(filtered,TextureColorUse::source,{0,1,0},.5);near(source[0],.125,"source color preserved");near(source[3],.125,"source opacity multiplied");
    auto alpha=color_texture(filtered,TextureColorUse::alpha,{0,1,0},1);near(alpha[0],0,"alpha replaces color");near(alpha[1],.25,"alpha tint premultiplied");
    auto light=color_texture(filtered,TextureColorUse::lightness,{0,1,0},1);near(light[3],.25*.2126,"lightness uses unpremultiplied luminance");near(light[1],light[3],"lightness color and alpha agree");
    auto hdr=color_texture({10,10,10,.5f},TextureColorUse::lightness,{1,1,1},1);near(hdr[3],.5,"HDR lightness bounded");
    auto clear=color_texture({0,0,0,0},TextureColorUse::lightness,{1,1,1},1);near(clear[3],0,"transparent black avoids division");
}
void wire_graph_motion() {
    auto g=evaluated();g.particles[0].transfer_mode=ParticleTransferMode::screen;
    auto wire=take(encode_evaluated_particles(g,{1,1}));check(wire.size()==48+24+200,"snapshot six preserves particle stride");
    check(wire[2]==std::byte{6},"texture snapshot version");auto decoded=take(decode_evaluated_particles(wire,{1,1}));
    check(decoded.texture_styles==g.texture_styles,"style roundtrip");check(decoded.particles[0].texture_random_key==0x800000,"random key roundtrip");
    check(decoded.particles[0].transfer_mode==ParticleTransferMode::screen,"texture transfer roundtrip");
    auto bad=wire;bad[44]=std::byte{1};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"reserved header rejected");
    bad=wire;bad[48+24+184+1]=std::byte{4};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"unused shape bits rejected");
    bad=wire;bad[48+8]=std::byte{8};check(!decode_evaluated_particles(bad,{1,1}).has_value(),"style mode bounds decoded");
    for(unsigned version=3;version<=5;++version) {
        auto legacy=g;legacy.texture_styles.clear();auto& p=legacy.particles[0];p.shape=0;p.texture_style_index=p.texture_random_key=0;
        p.transfer_mode=version==5?ParticleTransferMode::add:ParticleTransferMode::normal;
        if(version==4) {legacy.sprite_bases.push_back({1,0,0,0,1,0,0,0,1});p.sprite_basis_index=1;}
        auto bytes=take(encode_evaluated_particles(legacy,{1,1}));check(bytes[2]==std::byte(version),"legacy encoder preserved");
        auto roundtrip=take(decode_evaluated_particles(bytes,{1,1}));check(roundtrip.texture_styles.empty(),"legacy texture defaults");
    }
    auto graph=base_graph();for(auto& n:graph.nodes)if(n.type_key==graph_keys::kParticleNode) {
        set(n,graph_keys::kParticleShape,std::uint32_t{3});set(n,graph_keys::kTextureFront,std::uint32_t{7});set(n,graph_keys::kTextureTimeMode,std::uint32_t{4});
    }
    auto one=take(evaluate_particle_graph(graph,{1,1},never));auto two=take(evaluate_particle_graph(graph,{1,1},never));
    check(!one.particles.empty() && one.texture_styles.size()==1,"graph texture style shared");
    check(one.particles[0].texture_style_index==1 && one.particles[0].texture_random_key==two.particles[0].texture_random_key,"stable graph random key");
    auto sequence=serialize_graph(graph,particle_node_registry());check(sequence.has_value(),"optional texture graph keys serialize");
    auto restored=deserialize_graph(sequence.value(),particle_node_registry());check(restored.has_value(),"optional texture graph keys decode");
    for(auto& n:graph.nodes)if(n.type_key==graph_keys::kParticleNode)set(n,graph_keys::kLifeRandom,10.);
    auto temporal=take(evaluate_particle_graph(graph,{1,1},never));check(!temporal.particles.empty() && temporal.texture_styles.size()==1,"temporal texture registration");
    auto last=g;last.particles[0].age_seconds+=1;last.texture_styles.insert(last.texture_styles.begin(),{99,0});last.particles[0].texture_style_index=2;
    auto born=particle(2);born.age_seconds=.1;born.texture_style_index=1;last.particles.push_back(born);
    auto mixed=take(interpolate_motion_particles(g,last,1,2,.95,100,never));
    check(mixed.texture_styles.size()==2 && mixed.particles.size()==2,"motion endpoint styles remapped");
    for(const auto& p:mixed.particles)check(mixed.texture_styles[p.texture_style_index-1].front==(p.id==1?7u:99u),"motion style identity");
    auto frozen=mixed;for(auto& s:frozen.texture_styles)s.time_mode=TextureTimeMode::freeze_frame;
    std::vector<TextureSource> clips{{7,0,4,.5,1,1,1},{8,0,4,.5,1,1,1},{99,0,4,.5,1,1,1}};
    check(!take(plan_texture_frames(frozen,clips,1.95,never)).empty(),"motion sample birth-frame planning");
}
void projection_render_abi() {
    auto g=evaluated();auto r=request(g);const auto grid=sprite_geometry::make_grid(r.frame);
    sprite_geometry::Sprite a{},b{};check(sprite_geometry::project_sprite(g.particles[0],r,grid,a,{},2),"texture projects");
    near(a.ax,8,"texture width");near(a.by,4,"texture aspect");check(!a.back_facing,"default front face");
    r.frame.pixel_aspect_ratio=2;
    check(sprite_geometry::project_sprite(g.particles[0],r,sprite_geometry::make_grid(r.frame),b,{},2),"anamorphic output projects");
    near(b.ax*2/b.by,2,"source physical aspect preserved on anamorphic output");
    auto rotated=g.particles[0];rotated.rotation_degrees.z=90;
    check(sprite_geometry::project_sprite(rotated,r,sprite_geometry::make_grid(r.frame),b,{},2),"anamorphic rotation projects");
    near(std::abs(b.bx)*2/std::abs(b.ay),.5,"anamorphic rotation preserves physical axes");
    r.frame.pixel_aspect_ratio=1;
    auto flipped=g.particles[0];flipped.rotation_degrees.y=180;check(sprite_geometry::project_sprite(flipped,r,grid,b),"back face projects");check(b.back_facing,"back face classification");
    r.camera.enabled=true;r.camera.focal_x=r.camera.focal_y=100;r.camera.near_clip=.01;
    r.camera.layer_to_view={1,0,0,0,0,1,0,0,0,0,1,0,-16,-16,100,1};r.camera.image_to_layer={1,0,0,0,1,0,0,0,1};r.camera.center_x=r.camera.center_y=16;
    auto far=g.particles[0];far.position.z=100./32;
    check(sprite_geometry::project_sprite(far,r,grid,a,{},2,false),"perspective texture projects");near(std::abs(a.ax),4,"perspective size");
    check(sprite_geometry::project_sprite(far,r,grid,b,{},2,true),"ignore perspective texture projects");near(std::abs(b.ax),8,"ignore perspective size");near(a.x,b.x,"perspective center retained");
    r.camera.image_to_layer={-1,0,32,0,1,0,0,0,1};check(sprite_geometry::project_sprite(far,r,grid,b,{},2),"inverse layer reflection projects");check(!b.back_facing,"screen reflection does not change face");
    far.position.z=-4;check(!sprite_geometry::project_sprite(far,r,grid,b,{},2,true),"ignore perspective still clips camera");
    float front[]{0,1,0,1},back[]{0,0,1,1};
    const auto render=[&](EvaluatedGraph scene) {
        auto req=request(std::move(scene));req.texture_sources={{7,0,4,.5,2,2,2},{8,0,4,.5,1,1,1}};
        req.texture_frames={{7,2,1,1,4,front},{8,2,1,1,4,back}};
        return take(CpuParticleRenderer{}.render(req,never));
    };
    auto output=render(g);near(pixel(output)[1],1,"front source renders");
    near(pixel(output,16,11)[3],0,"source PAR controls aspect extent");
    g.particles[0].rotation_degrees.y=180;output=render(g);near(pixel(output)[2],1,"back source renders");
    g.texture_styles[0].front=0;output=render(g);near(pixel(output)[2],1,"back-only source renders its face");
    g.particles[0].rotation_degrees.y=0;output=render(g);near(pixel(output)[3],0,"None front is transparent");
    g=evaluated();g.texture_styles[0].color_use=TextureColorUse::alpha;output=render(g);near(pixel(output)[0],1,"particle alpha tint renders");
    g=evaluated();g.particles.push_back(particle(2));g.particles.back().transfer_mode=ParticleTransferMode::add;
    output=render(g);near(pixel(output)[1],2,"texture Add preserves HDR");
    g.particles.back().transfer_mode=ParticleTransferMode::stencil;output=render(g);near(pixel(output)[3],0,"texture Stencil uses source alpha");
    auto missing=request(evaluated());check(!CpuParticleRenderer{}.render(missing,never).has_value(),"missing resources do not substitute");
    check(prepare_sprite_scene(missing,never).error().code==ErrorCode::unsupported_format,"texture gets typed CPU fallback");
    SfCoreApi api{};check(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1,"current ABI accepted");check(StarfieldCore_GetApi(6,sizeof(api),&api)==0,"old ABI rejected");
    check(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1,"ABI table restored");
    const auto wire=serialize_graph(*request(evaluated()).graph,particle_node_registry());check(wire.has_value(),"frozen texture sequence serializes");
    SfCoreRenderRequest abi{};abi.struct_size=sizeof(abi);abi.frame={32,32,32,32,{0,0,32,32},1,1,1,30,2,0,1,1,1};abi.graph_bytes=wire.value().data();abi.graph_byte_count=wire.value().size();
    SfTextureSource sources[]{{sizeof(SfTextureSource),7,0,4,.5,2,2,2},{sizeof(SfTextureSource),8,0,4,.5,1,1,1}};
    SfTextureFrame frames[]{{sizeof(SfTextureFrame),7,2,1,1,4,front,4},{sizeof(SfTextureFrame),8,2,1,1,4,back,4}};
    abi.texture_source_count=2;abi.texture_frame_count=2;abi.texture_sources=sources;abi.texture_frames=frames;
    SfCoreRenderResult result{};result.struct_size=sizeof(result);check(api.render(&abi,&result)==SF_CORE_OK,"ABI texture render");
    api.release_render_result(&result);check(result.opaque_handle==nullptr,"ABI result released");
    frames[0].struct_size=0;check(api.render(&abi,&result)==SF_CORE_INVALID_REQUEST,"ABI nested size rejected");frames[0].struct_size=sizeof(SfTextureFrame);
    frames[0].pixel_float_count=UINT64_MAX;check(api.render(&abi,&result)==SF_CORE_WORK_LIMIT,"ABI byte count bounded before span");
    abi.texture_source_count=129;check(api.render(&abi,&result)==SF_CORE_WORK_LIMIT,"ABI source count bounded");
}
}
int main() {
    try {time_modes();planning_and_validation();filtering_color();wire_graph_motion();projection_render_abi();
        std::printf("Particle texture: %u checks passed (Core numeric contract only).\n",checks);return 0;
    } catch(const std::exception& error) {std::printf("FAIL after %u checks: %s\n",checks,error.what());return 1;}
}
