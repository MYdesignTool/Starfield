#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "../src/core/SpriteGeometry.hpp"

#include <bit>
#include <cstring>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace starfield::core;
namespace {
unsigned checks{};
const NeverCancelled never;
constexpr ParticleSpriteBasis identity{1,0,0,0,1,0,0,0,1};
constexpr ParticleSpriteBasis quarter_turn{0,-1,0,1,0,0,0,0,1};
constexpr ParticleSpriteBasis shear{1,.5,0,0,1,0,0,0,1};
void check(bool condition,const char* message) {
    ++checks;if(!condition)throw std::runtime_error(message);
}
void near(double a,double b,const char* message) {
    check(std::isfinite(a) && std::abs(a-b)<1e-9*std::max(1.,std::abs(b)),message);
}
struct CancelAfter final:Cancellation {
    mutable unsigned calls{};unsigned threshold;
    explicit CancelAfter(unsigned value):threshold(value){}
    bool is_cancelled()const noexcept override{return ++calls>=threshold;}
};
ParticleInstance particle() {
    ParticleInstance p;p.id=17;p.age_seconds=1;p.lifetime_seconds=4;
    p.size_pixels=20;p.size_y_pixels=6;p.opacity=.5;p.shape=1;
    p.position={.1,.05,0};p.color={1,.25,.75};return p;
}
void put(OpaqueBytes& bytes,std::size_t at,std::uint64_t value,unsigned count) {
    for(unsigned i=0;i<count;++i)bytes[at+i]=static_cast<std::byte>((value>>(8*i))&255);
}
OpaqueBytes legacy_fixture(const EvaluatedGraph& graph) {
    // Independently construct the established snapshot3 layout, including its
    // reserved field, so identity compatibility is checked byte for byte.
    OpaqueBytes bytes(32+16+200);put(bytes,0,0x8004,2);put(bytes,2,3,2);
    put(bytes,4,bytes.size(),4);put(bytes,8,1,8);put(bytes,16,1,8);
    put(bytes,24,1,4);put(bytes,28,1,4);
    std::size_t at=32;
    for(auto b:graph.evaluated_nodes[0].value.bytes)bytes[at++]=static_cast<std::byte>(b);
    const auto& p=graph.particles[0];put(bytes,at,p.id,8);at+=8;
    for(auto b:p.emitter_id.value.bytes)bytes[at++]=static_cast<std::byte>(b);
    for(double value:{p.age_seconds,p.lifetime_seconds,p.size_pixels,p.opacity,
        p.color.x,p.color.y,p.color.z,p.position.x,p.position.y,p.position.z,
        p.velocity.x,p.velocity.y,p.velocity.z,p.size_y_pixels,p.rotation_degrees.x,
        p.rotation_degrees.y,p.rotation_degrees.z,p.feather_percent,p.anchor_x_percent,p.anchor_y_percent}) {
        put(bytes,at,std::bit_cast<std::uint64_t>(value),8);at+=8;
    }
    put(bytes,at,p.shape,4);put(bytes,at+4,p.up_axis,4);put(bytes,at+8,p.limit_to_2d?1:0,4);
    return bytes;
}
void codec() {
    EvaluatedGraph graph;graph.evaluated_nodes.resize(1);graph.evaluated_nodes[0].value.bytes[0]=7;
    graph.particles={particle()};graph.particles[0].emitter_id.value.bytes[0]=5;
    const auto old=encode_evaluated_particles(graph,{1,1});check(old.has_value(),"legacy encode");
    check(old.value()==legacy_fixture(graph),"snapshot3 bytes changed");
    const auto restored_old=decode_evaluated_particles(old.value(),{2,2});
    check(restored_old.has_value() && restored_old.value().sprite_bases.empty() &&
        restored_old.value().particles[0].sprite_basis_index==0,"legacy decode default basis");
    graph.sprite_bases={quarter_turn,shear};graph.particles[0].sprite_basis_index=2;
    const auto encoded=encode_evaluated_particles(graph,{1,1});check(encoded.has_value(),"Transform encode");
    const auto& bytes=encoded.value();check(bytes[2]==std::byte{4} && bytes.size()==40+16+144+200,"snapshot4 stride/table");
    const auto restored=decode_evaluated_particles(bytes,{1,1});check(restored.has_value(),"Transform decode");
    check(restored.value().sprite_bases==graph.sprite_bases && restored.value().particles[0].sprite_basis_index==2,"basis roundtrip");
    check(restored.value().particles[0].position.x==graph.particles[0].position.x,"particle doubles roundtrip");
    for(std::size_t n=0;n<bytes.size();++n) {
        auto short_bytes=bytes;short_bytes.resize(n);
        check(!decode_evaluated_particles(short_bytes,{1,1}).has_value(),"truncation accepted");
    }
    const auto reject=[&](std::size_t at,std::uint64_t value,unsigned count) {
        auto bad=bytes;put(bad,at,value,count);check(!decode_evaluated_particles(bad,{1,1}).has_value(),"malformed table/header accepted");
    };
    reject(2,5,2);reject(4,bytes.size()-1,4);reject(24,kMaxGraphNodes+1,4);
    reject(28,kMaxParticleCount+1,4);reject(32,kMaxParticleSpriteBases+1,4);reject(36,1,4);
    reject(bytes.size()-4,3,4);reject(56,std::bit_cast<std::uint64_t>(1e13),8);
    reject(56,std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN()),8);
    check(!decode_evaluated_particles(bytes,{2,1}).has_value(),"wrong clock accepted");
    check(!decode_evaluated_particles(bytes,{1,0}).has_value(),"zero clock scale accepted");
    auto reserved=old.value();put(reserved,reserved.size()-4,1,4);
    check(!decode_evaluated_particles(reserved,{1,1}).has_value(),"snapshot3 reserved index accepted");
    graph.particles[0].sprite_basis_index=3;check(!encode_evaluated_particles(graph,{1,1}).has_value(),"missing encode basis accepted");
    graph.particles[0].sprite_basis_index=1;graph.sprite_bases[0][3]=std::numeric_limits<double>::infinity();
    check(!encode_evaluated_particles(graph,{1,1}).has_value(),"infinite basis encoded");
    graph.sprite_bases.assign(kMaxParticleSpriteBases+1,identity);
    check(!encode_evaluated_particles(graph,{1,1}).has_value(),"oversized table encoded");
    graph.sprite_bases={shear};graph.particles.assign(10000,particle());
    for(auto& p:graph.particles)p.sprite_basis_index=1;
    const auto shared=encode_evaluated_particles(graph,{1,1});check(shared.has_value(),"shared encode");
    check(shared.value().size()==40+16+72+200*10000,"matrix was stored per particle");
    CancelAfter encode_cancel(3);const auto interrupted=encode_evaluated_particles(graph,{1,1},&encode_cancel);
    check(!interrupted.has_value() && interrupted.error().code==ErrorCode::cancelled,"encode cancellation");
    CancelAfter decode_cancel(3);const auto stopped=decode_evaluated_particles(shared.value(),{1,1},&decode_cancel);
    check(!stopped.has_value() && stopped.error().code==ErrorCode::cancelled,"decode cancellation");
}
RenderRequest request() {
    RenderRequest r;r.settings=validate_settings(Settings{});
    r.frame={100,100,100,100,{0,0,100,100},{1,1},{1,24},PixelFormat::rgba32f,
        ColorSpace::ae_working_space,AlphaMode::premultiplied,1,Quality::full};return r;
}
void freeze(RenderRequest& request,const EvaluatedGraph& evaluated) {
    auto bytes=encode_evaluated_particles(evaluated,request.frame.time);check(bytes.has_value(),"render freeze");
    const auto uuid=[](unsigned char value){Uuid128 id;id.bytes[0]=value;return id;};
    auto made=make_emitter_particle_output_graph(request.settings.value,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(3)},EdgeId{uuid(4)},EdgeId{uuid(5)});
    check(made.has_value(),"freeze valid graph");auto graph=made.take_value();
    graph.optional_records.push_back(bytes.take_value());request.graph=std::make_shared<const Graph>(std::move(graph));
}
void camera(RenderRequest& request) {
    auto& c=request.camera;c.enabled=true;c.layer_to_view={1,0,0,0,0,1,0,0,0,0,1,0,-50,-50,100,1};
    c.image_to_layer=identity;c.focal_x=c.focal_y=100;c.center_x=c.center_y=50;
}
void projection() {
    auto r=request();auto p=particle();p.sprite_basis_index=1;
    sprite_geometry::Sprite sprite;const auto grid=sprite_geometry::make_grid(r.frame);
    const std::array<ParticleSpriteBasis,1> table{quarter_turn};
    check(sprite_geometry::project_sprite(p,r,grid,sprite,table),"project rotated sprite");
    near(sprite.x,60,"world centre x");near(sprite.y,45,"world centre y");
    near(sprite.ax,0,"rotated width x");near(sprite.ay,-10,"world/display rotation sign");
    near(sprite.bx,3,"rotated height x");near(sprite.by,0,"rotated height y");
    const std::array<ParticleSpriteBasis,1> shears{shear};check(sprite_geometry::project_sprite(p,r,grid,sprite,shears),"project shear");
    near(sprite.ax,10,"shear width");near(sprite.bx,-1.5,"world/display shear sign");near(sprite.by,3,"shear height");
    p.anchor_x_percent=0;p.anchor_y_percent=0;
    check(sprite_geometry::project_sprite(p,r,grid,sprite,table),"project anchor");
    near(sprite.x,57,"transformed anchor x");near(sprite.y,35,"transformed anchor y");
    p.anchor_x_percent=p.anchor_y_percent=50;p.sprite_basis_index=2;
    check(!sprite_geometry::project_sprite(p,r,grid,sprite,table),"projection out-of-range index");
    p.sprite_basis_index=1;camera(r);r.frame.pixel_aspect_ratio=2;
    check(sprite_geometry::project_sprite(p,r,sprite_geometry::make_grid(r.frame),sprite,table),"camera/PAR projection");
    near(sprite.x,55,"camera/PAR centre");near(sprite.ay,10,"camera rotation sign");near(sprite.bx,1.5,"camera/PAR axis");
    r.frame.frame_width=r.frame.frame_height=50;r.frame.region_of_interest={0,0,50,50};
    check(sprite_geometry::project_sprite(p,r,sprite_geometry::make_grid(r.frame),sprite,table),"downsample projection");
    near(sprite.x,27.5,"downsample centre");near(sprite.ay,5,"downsample rotated axis");
    r=request();p=particle();p.shape=0;p.sprite_basis_index=1;
    ParticleTransformSettings tilted;tilted.rotation_degrees.x=60;
    const auto compiled=compile_particle_transform(tilted);check(compiled.has_value(),"compile tilted plane");
    const std::array<ParticleSpriteBasis,1> tilt_table{compiled.value().particle_basis()};
    check(sprite_geometry::project_sprite(p,r,sprite_geometry::make_grid(r.frame),sprite,tilt_table),"Transform tilts billboard plane");
    near(sprite.ax,10,"tilted circle width");near(sprite.by,5,"tilted circle plane height");
}
double gpu_coverage(const SfGpuSprite& s,double u,double v) {
    const auto circle=[&](double d,double scale) {
        return std::clamp(.5+(1-d)*scale,0.,1.)*(s.feather>0?std::clamp((1-d)/s.feather,0.,1.):1);
    };
    if(s.shape==0)return circle(std::hypot(u,v),s.edge_scale);
    if(s.shape==1)return circle(std::max(std::abs(u),std::abs(v)),s.edge_scale);
    double remaining=1;
    for(const auto xy:{std::pair{0.,0.},std::pair{-.35,-.2},std::pair{.35,-.2},std::pair{-.2,.35},std::pair{.2,.35}})
        remaining*=1-circle(std::hypot(u-xy.first,v-xy.second)/.6,s.edge_scale*.6);
    return 1-remaining;
}
void scene_parity(RenderRequest r,const EvaluatedGraph& graph) {
    freeze(r,graph);const auto cpu=CpuParticleRenderer{}.render(r,never);const auto gpu=prepare_sprite_scene(r,never);
    check(cpu.has_value() && gpu.has_value(),"CPU and portable GPU scene prepare");
    double error=0;
    for(unsigned y=0;y<cpu.value().height();++y)for(unsigned x=0;x<cpu.value().width();++x) {
        const auto tile=(y/16)*gpu.value().tiles_x+x/16;double rgba[4]{};
        for(auto at=gpu.value().offsets[tile];at<gpu.value().offsets[tile+1];++at) {
            const auto& s=gpu.value().sprites[gpu.value().indices[at]];
            if(static_cast<int>(x)<s.left || static_cast<int>(x)>=s.right || static_cast<int>(y)<s.top || static_cast<int>(y)>=s.bottom)continue;
            const double dx=x+.5-s.x,dy=y+.5-s.y;
            const double alpha=gpu_coverage(s,s.inverse_ax*dx+s.inverse_ay*dy,s.inverse_bx*dx+s.inverse_by*dy)*s.opacity;
            const double color[]{s.red,s.green,s.blue,1};
            for(unsigned c=0;c<4;++c)rgba[c]=color[c]*alpha+rgba[c]*(1-alpha);
        }
        float actual[4];std::memcpy(actual,cpu.value().pixels.data()+y*cpu.value().row_bytes+x*16,16);
        for(unsigned c=0;c<4;++c)error=std::max(error,std::abs(actual[c]-rgba[c]));
    }
    check(error<.0002,"CPU / GPU packed affine coverage differ");
}
void rendering() {
    EvaluatedGraph g;g.sprite_bases={shear,quarter_turn};g.particles={particle(),particle()};
    g.particles[0].sprite_basis_index=1;g.particles[0].feather_percent=30;
    g.particles[1].sprite_basis_index=2;g.particles[1].id=18;g.particles[1].color={.2,1.4,.6};
    for(unsigned shape=0;shape<3;++shape) {
        for(auto& p:g.particles)p.shape=shape;
        auto r=request();scene_parity(r,g);r.frame.region_of_interest={48,38,71,54};scene_parity(r,g);
        camera(r);r.frame.pixel_aspect_ratio=1.5;scene_parity(r,g);
    }
    g.particles={particle()};g.particles[0].sprite_basis_index=1;g.sprite_bases={identity};
    auto r=request();camera(r);freeze(r,g);const auto with=CpuParticleRenderer{}.render(r,never);
    g.particles[0].sprite_basis_index=0;g.sprite_bases.clear();freeze(r,g);const auto without=CpuParticleRenderer{}.render(r,never);
    check(with.has_value() && without.has_value() && with.value().pixels==without.value().pixels,"rectangle identity camera parity");
    g.particles[0].sprite_basis_index=1;g.sprite_bases={{1,0,0,0,0,0,0,0,1}};freeze(r,g);
    const auto collapsed=CpuParticleRenderer{}.render(r,never);const auto empty=prepare_sprite_scene(r,never);
    check(collapsed.has_value() && std::all_of(collapsed.value().pixels.begin(),collapsed.value().pixels.end(),[](auto b){return b==std::byte{0};}),"singular sprite coverage");
    check(empty.has_value() && empty.value().sprites.empty(),"singular GPU sprite coverage");
    g.sprite_bases={shear};g.particles[0].shape=1;
    for(const auto format:{PixelFormat::rgba8,PixelFormat::rgba16,PixelFormat::rgba32f}) {
        r=request();r.frame.format=format;freeze(r,g);const auto full=CpuParticleRenderer{}.render(r,never);
        r.frame.region_of_interest={48,38,71,54};const auto crop=CpuParticleRenderer{}.render(r,never);
        check(full.has_value() && crop.has_value(),"transformed bit-depth render");
        bool same=true;const auto pixel_bytes=bytes_per_pixel(format);
        for(unsigned y=0;y<crop.value().height();++y)same&=std::memcmp(crop.value().pixels.data()+y*crop.value().row_bytes,
            full.value().pixels.data()+(y+38)*full.value().row_bytes+48*pixel_bytes,crop.value().row_bytes)==0;
        check(same,"transformed ROI matches full frame across depths");
    }
    CancelAfter cancel(1);check(!CpuParticleRenderer{}.render(r,cancel).has_value(),"CPU cancellation");
    CancelAfter cancel_scene(1);check(!prepare_sprite_scene(r,cancel_scene).has_value(),"scene cancellation");
}
void motion() {
    EvaluatedGraph first,last;first.sprite_bases={identity,quarter_turn};last.sprite_bases={shear,identity};
    for(unsigned i=0;i<1000;++i) {
        auto a=particle();a.id=i;a.sprite_basis_index=2;
        auto b=a;b.sprite_basis_index=1;b.age_seconds=2;b.position.x=.2;
        first.particles.push_back(a);last.particles.push_back(b);
    }
    auto only_first=particle();only_first.id=5000;only_first.sprite_basis_index=2;
    first.particles.push_back(only_first);
    auto only_last=particle();only_last.id=5001;only_last.sprite_basis_index=1;
    last.particles.push_back(only_last);
    const auto middle=interpolate_motion_particles(first,last,1,2,.5,kMaxParticleCount,never);
    check(middle.has_value() && middle.value().particles.size()==1002,"motion birth/death interpolation");
    check(middle.value().sprite_bases.size()==3,"motion matrices not shared per endpoint pair");
    for(unsigned i=0;i<1000;++i) {
        const auto& p=middle.value().particles[i];check(p.sprite_basis_index==1,"pair remap inconsistent");
        near(p.position.x,.15,"motion position");near(p.age_seconds,1.5,"motion age");
    }
    for(unsigned i=0;i<9;++i)near(middle.value().sprite_bases[0][i],(quarter_turn[i]+shear[i])/2,"basis interpolation");
    check(middle.value().sprite_bases[1]==quarter_turn && middle.value().sprite_bases[2]==shear,"one-endpoint basis lost");
    const auto reversed=interpolate_motion_particles(first,last,2,1,.5,kMaxParticleCount,never);
    check(reversed.has_value() && reversed.value().particles.size()==1002,"reverse remapped shutter clocks rejected");
    near(reversed.value().particles[0].position.x,.15,"reverse remapped interpolation");
    const auto frozen=encode_evaluated_particles(middle.value(),{3,2});check(frozen.has_value(),"interpolated basis encodes");
    CancelAfter cancelled(3);const auto stopped=interpolate_motion_particles(first,last,1,2,.5,kMaxParticleCount,cancelled);
    check(!stopped.has_value() && stopped.error().code==ErrorCode::cancelled,"typed motion cancellation");
    last.particles[0].sprite_basis_index=3;
    check(!interpolate_motion_particles(first,last,1,2,.5,kMaxParticleCount,never).has_value(),"invalid endpoint index accepted");
    last.particles[0].sprite_basis_index=1;
    check(!interpolate_motion_particles(first,last,1,2,1.1,kMaxParticleCount,never).has_value(),"invalid interpolation amount");
    first={};last={};first.particles={particle()};last=first;
    const auto baseline=interpolate_motion_particles(first,last,1,2,.5,10,never);
    check(baseline.has_value() && baseline.value().sprite_bases.empty() && baseline.value().particles[0].sprite_basis_index==0,"identity motion path changed");
    //4097 distinct endpoint pairs exceed the bounded shared table, even though
    // each endpoint itself is valid. This must report an error, not drop a basis.
    first.sprite_bases.assign(kMaxParticleSpriteBases,identity);last.sprite_bases={shear};
    first.particles.clear();last.particles.clear();
    for(unsigned i=0;i<=kMaxParticleSpriteBases;++i) {
        auto a=particle();a.id=i;a.sprite_basis_index=i;
        auto b=a;b.sprite_basis_index=1;first.particles.push_back(a);last.particles.push_back(b);
    }
    const auto bounded=interpolate_motion_particles(first,last,1,2,.5,kMaxParticleCount,never);
    check(!bounded.has_value() && bounded.error().code==ErrorCode::work_limit_exceeded,"motion basis table unbounded");
}
void abi() {
    SfCoreApi api{};check(StarfieldCore_GetApi(3,sizeof(api),&api)==0,"ABI3 must not accept Transform transport");
    check(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1 && api.abi_version==4,"paired ABI4 unavailable");
    auto r=request();EvaluatedGraph graph;graph.sprite_bases={shear};
    graph.particles={particle()};graph.particles[0].sprite_basis_index=1;freeze(r,graph);
    const auto bytes=serialize_graph(*r.graph,particle_node_registry());check(bytes.has_value(),"ABI graph serialization");
    SfCoreRenderRequest input{};input.struct_size=sizeof(input);
    input.frame={100,100,100,100,{0,0,100,100},1,1,1,24,
        static_cast<std::uint32_t>(PixelFormat::rgba32f),static_cast<std::uint32_t>(ColorSpace::ae_working_space),
        static_cast<std::uint32_t>(AlphaMode::premultiplied),static_cast<std::uint32_t>(Quality::full),1};
    input.graph_bytes=bytes.value().data();input.graph_byte_count=bytes.value().size();
    SfCoreRenderResult output{};output.struct_size=sizeof(output);
    check(api.render(&input,&output)==SF_CORE_OK && output.opaque_handle && output.pixels,"snapshot4 crosses C render ABI");
    const auto expected=CpuParticleRenderer{}.render(r,never);check(expected.has_value(),"ABI reference render");
    check(output.pixel_byte_count==expected.value().pixels.size() &&
        std::memcmp(output.pixels,expected.value().pixels.data(),expected.value().pixels.size())==0,"ABI CPU pixels preserved");
    api.release_render_result(&output);check(!output.opaque_handle && !output.pixels,"ABI render ownership released");
    SfCoreGpuSceneResult scene{};scene.struct_size=sizeof(scene);
    check(api.prepare_gpu_scene(&input,&scene)==SF_CORE_OK && scene.sprite_count==1 && scene.opaque_handle,"snapshot4 crosses C GPU scene ABI");
    near(scene.sprites[0].inverse_ay,1./20,"ABI packed shear retained");
    api.release_gpu_scene(&scene);check(!scene.opaque_handle && !scene.sprites,"ABI scene ownership released");
}
}
int main() {
    try {
        codec();projection();rendering();motion();abi();
        std::cout<<"Transform transport: "<<checks<<" checks passed\n";return 0;
    }catch(const std::exception& error) {
        std::cerr<<"Transform transport failed after "<<checks<<" checks: "<<error.what()<<'\n';return 1;
    }
}
