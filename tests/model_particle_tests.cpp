#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ModelResources.hpp"
#include "starfield/core/ModelScene.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/SpriteScene.hpp"
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <new>

static bool fail_allocation=false;
void* operator new(std::size_t n){if(fail_allocation)throw std::bad_alloc();if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
void operator delete[](void* p,std::size_t) noexcept {std::free(p);}
namespace {
using namespace starfield::core;
int checks{},failures{};NeverCancelled never;constexpr RationalTime clock{1,1};
void check(bool ok,const char* label){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",label);}}
bool near(double a,double b){return std::abs(a-b)<1e-6;}
template<class T>T take(Result<T> r){if(!r.has_value()){std::printf("FIXTURE: %s\n",r.error().detail);std::exit(2);}return r.take_value();}
template<class T>T take(SequenceResult<T> r){if(!r.has_value()){std::printf("FIXTURE: sequence codec failed\n");std::exit(2);}return r.take_value();}
void rejected(const auto& r,ErrorCode code,const char* label){check(!r.has_value()&&r.error().code==code,label);}
void put(OpaqueBytes& b,std::size_t at,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)b[at+i]=static_cast<std::byte>((value>>(8*i))&255);}
unsigned version(const OpaqueBytes& b){return std::to_integer<unsigned>(b[2])|(std::to_integer<unsigned>(b[3])<<8);}
NodeId node(unsigned n){NodeId id;id.value.bytes[15]=static_cast<std::uint8_t>(n);return id;}
EdgeId edge(unsigned n){EdgeId id;id.value.bytes[15]=static_cast<std::uint8_t>(n);return id;}
ParticleInstance particle(){ParticleInstance p;p.shape=4;p.size_pixels=p.size_y_pixels=8;p.opacity=.5;p.lifetime_seconds=2;p.age_seconds=1;return p;}
EvaluatedGraph evaluated(){EvaluatedGraph g;g.particles.push_back(particle());return g;}
RenderRequest request(const EvaluatedGraph& evaluated,RationalTime time=clock) {
    RenderRequest r;r.frame.layer_width=r.frame.layer_height=r.frame.frame_width=r.frame.frame_height=32;
    r.frame.region_of_interest={0,0,32,32};r.frame.time=time;r.frame.frame_duration={1,60};r.frame.format=PixelFormat::rgba32f;
    auto graph=take(make_emitter_particle_output_graph(Settings{},node(1),node(2),node(3),edge(1),edge(2)));
    graph.optional_records.push_back(take(encode_evaluated_particles(evaluated,time)));
    r.graph=std::make_shared<const Graph>(std::move(graph));return r;
}
std::array<float,4> pixel(const RenderOutput& o,unsigned x=16,unsigned y=16) {
    std::array<float,4> p{};std::memcpy(p.data(),o.pixels.data()+std::size_t(y-o.region.top)*o.row_bytes+(x-o.region.left)*16,16);return p;
}
struct Cancel:Cancellation {mutable unsigned polls{};unsigned stop;explicit Cancel(unsigned n):stop(n){}
    bool is_cancelled() const noexcept override{return ++polls>=stop;}};
void snapshots() {
    auto g=evaluated();const auto implicit=take(encode_evaluated_particles(g,clock));
    check(version(implicit)==8&&implicit.size()==264,"implicit cube uses snapshot8 with unchanged200-byte stride");
    auto decoded=take(decode_evaluated_particles(implicit,clock));check(decoded.particles[0].shape==4&&!decoded.particles[0].model_style_index,"implicit Model roundtrips");
    g.model_styles.push_back({{ParticleModelInstance{},ParticleModelInstance{}}});g.particles[0].model_style_index=1;
    g.model_styles[0].instances[1].resource[15]=37;g.model_styles[0].instances[1].model_to_particle[12]=2;
    const auto wire=take(encode_evaluated_particles(g,clock));check(wire.size()==560,"Model group has explicit length/count/144-byte entries");
    decoded=take(decode_evaluated_particles(wire,clock));check(take(encode_evaluated_particles(decoded,clock))==wire,"Model snapshot deterministic roundtrip");
    check(decoded.model_styles[0].instances[1].resource[15]==37&&decoded.model_styles[0].instances[1].model_to_particle[12]==2,"IDs and local pose retained");
    for(std::size_t n=0;n<wire.size();++n)rejected(decode_evaluated_particles(OpaqueBytes(wire.begin(),wire.begin()+n),clock),ErrorCode::invalid_request,"every truncated Model snapshot rejects");
    for(auto offset:{36u,44u,52u,60u}){auto bad=wire;put(bad,offset,1,4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"all reserved headers reject");}
    for(auto n:{0u,1u,295u,297u,0xffffffffu}){auto bad=wire;put(bad,64,n,4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"bad group byte length rejects");}
    for(auto n:{0u,1u,3u,257u,0xffffffffu}){auto bad=wire;put(bad,68,n,4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"bad member count rejects");}
    auto bad=wire;put(bad,56,4097,4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"group count cap rejects before allocation");
    bad=wire;put(bad,88,std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN()),8);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"nonfinite local transform rejects");
    bad=wire;put(bad,88+3*8,std::bit_cast<std::uint64_t>(1.),8);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"nonaffine local transform rejects");
    bad=wire;put(bad,wire.size()-16,4|(2u<<16),4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"missing group index rejects");
    bad=wire;put(bad,wire.size()-16,5,4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"unknown shape rejects");
    bad=wire;put(bad,wire.size()-12,2|(1u<<8),4);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"Model cannot carry Texture random key");
    rejected(decode_evaluated_particles(wire,{2,1}),ErrorCode::invalid_request,"wrong clock rejects");
    bad=implicit;put(bad,2,7,2);rejected(decode_evaluated_particles(bad,clock),ErrorCode::invalid_request,"Model bytes cannot masquerade as snapshot7");
    auto invalid=g;invalid.model_styles[0].instances.clear();rejected(encode_evaluated_particles(invalid,clock),ErrorCode::invalid_request,"empty group rejects at encoding");
    invalid=g;invalid.particles[0].shape=0;rejected(encode_evaluated_particles(invalid,clock),ErrorCode::invalid_request,"nonModel cannot reference Model group");
    invalid=g;invalid.model_styles[0].instances.resize(257);rejected(encode_evaluated_particles(invalid,clock),ErrorCode::invalid_request,"oversized group rejects at encoding");
    invalid=g;invalid.model_styles.resize(4097);rejected(encode_evaluated_particles(invalid,clock),ErrorCode::invalid_request,"Model group table cap rejects at encoding");
    Cancel encode_stop(2);rejected(encode_evaluated_particles(g,clock,&encode_stop),ErrorCode::cancelled,"Model table encoding cancels");
    Cancel decode_stop(3);rejected(decode_evaluated_particles(wire,clock,&decode_stop),ErrorCode::cancelled,"Model table decoding cancels");
    fail_allocation=true;const auto failed_encode=encode_evaluated_particles(g,clock);const auto failed_decode=decode_evaluated_particles(wire,clock);fail_allocation=false;
    rejected(failed_encode,ErrorCode::allocation_failed,"snapshot encoding allocation failure is typed");rejected(failed_decode,ErrorCode::allocation_failed,"snapshot decoding allocation failure is typed");
    for(unsigned v=3;v<=7;++v) {
        EvaluatedGraph old;auto p=particle();p.shape=0;
        if(v==4)old.sprite_bases.push_back({1,0,0,0,1,0,0,0,1});
        if(v==5)p.transfer_mode=ParticleTransferMode::add;
        if(v==6){p.shape=3;p.texture_style_index=1;p.texture_random_key=53;old.texture_styles.push_back({});}
        if(v==7){p.shape=2;p.cloud_style_index=1;p.cloud_random_key=71;old.cloud_styles.push_back({});}
        old.particles.push_back(p);auto old_wire=take(encode_evaluated_particles(old,clock));check(version(old_wire)==v,"legacy encoding chooses same snapshot version");
        auto old_decoded=take(decode_evaluated_particles(old_wire,clock));check(old_decoded.model_styles.empty()&&old_decoded.particles[0].model_style_index==0,"legacy reading defaults absent Model data");
        check(take(encode_evaluated_particles(old_decoded,clock))==old_wire,"legacy snapshot bytes stay canonical");
    }
    g.particles.push_back(particle());g.particles.back().shape=2;g.particles.back().cloud_style_index=1;g.particles.back().cloud_random_key=55;g.cloud_styles.push_back({});
    g.particles.push_back(particle());g.particles.back().shape=3;g.particles.back().texture_style_index=1;g.particles.back().texture_random_key=99;g.texture_styles.push_back({});
    decoded=take(decode_evaluated_particles(take(encode_evaluated_particles(g,clock)),clock));
    check(decoded.particles[1].cloud_random_key==55&&decoded.particles[2].texture_random_key==99&&decoded.model_styles.size()==1,"mixed Model Cloud Texture fields retain distinct tables");
}
void rendering() {
    CpuParticleRenderer renderer;auto g=evaluated();auto r=request(g);auto out=take(renderer.render(r,never));
    check(near(pixel(out)[3],.5)&&near(pixel(out)[0],.5),"default cube applies logical opacity once");
    unsigned filled=0;for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)if(pixel(out,x,y)[3]>0)++filled;
    check(filled==64,"unit cube has exact8-by8 footprint");
    for(auto roi:{RectI{0,0,16,16},RectI{16,0,32,16},RectI{0,16,16,32},RectI{16,16,32,32}}) {
        auto tile_request=r;tile_request.frame.region_of_interest=roi;const auto tile=take(renderer.render(tile_request,never));
        for(int y=roi.top;y<roi.bottom;++y)for(int x=roi.left;x<roi.right;++x)check(pixel(tile,x,y)==pixel(out,x,y),"Model ROI agrees with full frame");
    }
    r.frame.pixel_aspect_ratio=2;auto par=take(renderer.render(r,never));filled=0;for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)if(pixel(par,x,y)[3]>0)++filled;
    check(filled==32,"physical Model cube width respects output PAR");
    r=request(g);r.frame.frame_width=r.frame.frame_height=16;r.frame.region_of_interest={0,0,16,16};auto down=take(renderer.render(r,never));check(near(pixel(down,8,8)[3],.5),"downsample projects from full layer pose");
    g.model_styles.push_back({{ParticleModelInstance{},ParticleModelInstance{}}});g.particles[0].model_style_index=1;r=request(g);
    auto duplicates=take(renderer.render(r,never));check(duplicates.pixels==out.pixels,"coincident Model members do not double opacity");
    g.model_styles[0].instances[1].model_to_particle[12]=2;r=request(g);auto separated=take(renderer.render(r,never));
    check(near(pixel(separated,16,16)[3],.5)&&near(pixel(separated,30,16)[3],.5),"all Model group members render from one particle");
    g=evaluated();ParticleModelInstance imported;imported.resource[15]=9;g.model_styles.push_back({{imported}});g.particles[0].model_style_index=1;r=request(g);
    rejected(renderer.render(r,never),ErrorCode::invalid_request,"missing imported resource never silently falls back");
    auto cube=take(make_unit_cube());r.model_sources.push_back({imported.resource,cube});auto imported_out=take(renderer.render(r,never));check(imported_out.pixels==out.pixels,"numeric imported cube matches implicit geometry");
    auto triangle_mesh=take(parse_model_obj("v -.5 -.5 0\nv .5 -.5 0\nv -.5 .5 0\nf 1 2 3\n",never));r.model_sources[0].geometry=triangle_mesh;
    auto triangular=take(renderer.render(r,never));check(pixel(triangular,13,18)[3]>.4&&pixel(triangular,19,13)[3]==0,"imported triangle is actual polygon geometry");
    auto invalid=r;invalid.model_sources.push_back(invalid.model_sources[0]);rejected(renderer.render(invalid,never),ErrorCode::invalid_request,"direct C++ resource IDs remain strict");
    invalid=r;invalid.model_sources[0].geometry.positions[0].value.x=std::numeric_limits<double>::quiet_NaN();rejected(renderer.render(invalid,never),ErrorCode::invalid_request,"direct C++ bad geometry rejects before rasterization");
    auto compiled=take(compile_model_resources(r.model_sources,never));check(&compiled[0].geometry()==&r.model_sources[0].geometry,"compiled resource lease references caller-owned immutable mesh");
    fail_allocation=true;const auto lease_allocation=compile_model_resources(r.model_sources,never);fail_allocation=false;
    rejected(lease_allocation,ErrorCode::allocation_failed,"Model lease table allocation failure is typed");
    Cancel lease_stop(1);rejected(compile_model_resources(r.model_sources,lease_stop),ErrorCode::cancelled,"Model lease compilation cooperatively cancels");
    g=evaluated();g.particles[0].rotation_degrees={20,40,10};g.particles[0].up_axis=1;
    g.sprite_bases.push_back({-1,0,0,.25,1,0,0,0,1});g.particles[0].sprite_basis_index=1;r=request(g);auto rotated=take(renderer.render(r,never));
    const auto matrix=take(model_particle_matrix(g.particles[0],r.frame,g.sprite_bases));auto scene=take(project_model_scene(cube,{r.frame,r.camera,matrix},never));auto surface=take(rasterize_model_scene(scene,never));
    std::vector<float> expected(32*32*4);take(composite_model_surface(surface,{1,1,1},.5,ParticleTransferMode::normal,{0,0,32,32},expected,never));
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x)check(near(pixel(rotated,x,y)[3],expected[(y*32+x)*4+3]),"renderer uses full Euler Up Axis Transform mesh pose");
    auto primitives=evaluated();primitives.particles[0].shape=1;primitives.particles[0].opacity=.5;primitives.particles[0].color={0,1,0};
    auto model=particle();model.color={1,0,0};primitives.particles.push_back(model);
    for(unsigned mode=0;mode<4;++mode) {
        primitives.particles[1].transfer_mode=static_cast<ParticleTransferMode>(mode);auto mixed=take(renderer.render(request(primitives),never));const auto p=pixel(mixed);
        const double expected_red[]{.5,.5,.5,0},expected_green[]{.25,.5,.5,.25},expected_alpha[]{.75,.75,.75,.25};
        check(near(p[0],expected_red[mode])&&near(p[1],expected_green[mode])&&near(p[3],expected_alpha[mode]),"Model transfer modes compose with primitive particles");
    }
    primitives.particles[1].transfer_mode=ParticleTransferMode::normal;primitives.particles[0].position.z=1;primitives.particles[1].position.z=2;
    r=request(primitives);r.camera.enabled=true;r.camera.layer_to_view=kIdentityModelMatrix;r.camera.layer_to_view[12]=r.camera.layer_to_view[13]=-16;r.camera.layer_to_view[14]=32;
    r.camera.image_to_layer={1,0,0,0,1,0,0,0,1};r.camera.center_x=r.camera.center_y=16;r.camera.focal_x=r.camera.focal_y=64;
    auto ordered=take(renderer.render(r,never));check(near(pixel(ordered)[0],.25)&&near(pixel(ordered)[1],.5),"Model and primitive particles share camera depth sorting");
    g=evaluated();g.particles[0].size_pixels=16;g.particles[0].position.z=-1;r=request(g);r.camera.enabled=true;r.camera.layer_to_view=kIdentityModelMatrix;
    r.camera.layer_to_view[12]=r.camera.layer_to_view[13]=-16;r.camera.layer_to_view[14]=32;r.camera.focal_x=r.camera.focal_y=4;r.camera.center_x=r.camera.center_y=16;r.camera.near_clip=1;r.camera.image_to_layer={1,0,0,0,1,0,0,0,1};
    auto crossing=take(renderer.render(r,never));check(pixel(crossing)[3]>0,"mesh crosses near plane when particle center is behind it");
    g=evaluated();r=request(g);rejected(prepare_sprite_scene(r,never),ErrorCode::unsupported_format,"Model explicitly requires CPU fallback");
    rejected(CpuParticleRenderer({0,11,512'000'000}).render(r,never),ErrorCode::work_limit_exceeded,"frame input triangle cap applies before projecting cube");
    rejected(CpuParticleRenderer({0,12,1}).render(r,never),ErrorCode::work_limit_exceeded,"frame sample visit cap rejects typed without output");
    g.particles.push_back(particle());r=request(g);rejected(CpuParticleRenderer({0,23,512'000'000}).render(r,never),ErrorCode::work_limit_exceeded,"input triangle budget is shared across particles");
    const auto default_pose=take(model_particle_matrix(g.particles[0],r.frame));
    const auto default_scene=take(project_model_scene(cube,{r.frame,r.camera,default_pose},never));
    const auto visits=take(rasterize_model_scene(default_scene,never)).sample_visits;
    check(visits>0,"Model surface reports charged sample visits");
    rejected(CpuParticleRenderer({0,24,visits}).render(r,never),ErrorCode::work_limit_exceeded,"sample budget is shared across logical particles");
    rejected(CpuParticleRenderer({0,16'000'001,512'000'000}).render(r,never),ErrorCode::invalid_request,"caller cannot enlarge Model hard frame cap");
    Cancel stop(12);rejected(renderer.render(r,stop),ErrorCode::cancelled,"Model render cooperatively cancels without pixels");
    g=evaluated();g.particles[0].size_pixels=0;auto invisible=take(renderer.render(request(g),never));check(pixel(invisible)[3]==0,"zero size Model is invisible");
    r=request(evaluated());r.frame.format=PixelFormat::rgba8;auto eight=take(renderer.render(r,never));check(std::to_integer<unsigned>(eight.pixels[16*eight.row_bytes+16*4+3])==128,"Model encodes8-bit alpha");
    r.frame.format=PixelFormat::rgba16;auto sixteen=take(renderer.render(r,never));std::uint16_t alpha;std::memcpy(&alpha,sixteen.pixels.data()+16*sixteen.row_bytes+16*8+6,2);check(alpha==16384,"Model encodes AE16-bit alpha");
}
void transport() {
    auto g=evaluated();ParticleModelInstance instance;instance.resource[0]=1;g.model_styles.push_back({{instance}});g.particles[0].model_style_index=1;
    auto r=request(g);auto graph=take(serialize_graph(*r.graph,particle_node_registry()));
    SfCoreApi api{};check(StarfieldCore_GetApi(8,sizeof(api),&api)==1,"Model pixel transport API available");
    SfCoreRenderRequest input{};input.struct_size=sizeof(input);input.frame={32,32,32,32,{0,0,32,32},1,1,1,60,2,0,0,1,1};input.graph_bytes=graph.data();input.graph_byte_count=graph.size();
    SfModelPosition positions[]{{-.5,-.5,0,1},{.5,-.5,0,1},{-.5,.5,0,1}};SfModelTriangle triangle{};
    for(unsigned i=0;i<3;++i)triangle.corners[i]={i,kMissingModelAttribute,kMissingModelAttribute};
    SfModelSource source{};source.struct_size=sizeof(source);source.resource_id[0]=1;source.positions=positions;source.position_count=3;source.triangles=&triangle;source.triangle_count=1;
    input.model_sources=&source;input.model_source_count=1;SfCoreRenderResult output{};output.struct_size=sizeof(output);
    check(api.render(&input,&output)==SF_CORE_OK&&output.pixel_byte_count==32*32*16,"ABI8 numeric Model produces rendered pixels");
    std::array<float,4> p{};if(output.pixels)std::memcpy(p.data(),static_cast<const std::byte*>(output.pixels)+18*output.row_bytes+13*16,16);
    check(near(p[3],.5),"C API uses imported triangle instead of missing fallback");api.release_render_result(&output);
    input.struct_size=SF_CORE_ABI7_RENDER_REQUEST_SIZE;check(api.render(&input,&output)==SF_CORE_INVALID_REQUEST&&!output.pixels,"ABI7 cannot read absent imported resource tail");
    input.struct_size=sizeof(input);SfCoreGpuSceneResult gpu{};gpu.struct_size=sizeof(gpu);
    check(api.prepare_gpu_scene(&input,&gpu)==SF_CORE_UNSUPPORTED_FORMAT&&!gpu.opaque_handle,"C API Model GPU preparation fails explicitly for CPU fallback");
}
}
int main(){snapshots();rendering();transport();std::printf("Model particle: %d checks, %d failures\n",checks,failures);return failures?1:0;}
