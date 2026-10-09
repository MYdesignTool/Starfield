#include "starfield/core/ModelResources.hpp"
#include "starfield/core/ModelScene.hpp"
#include "../src/core/SpriteGeometry.hpp"
#include <bit>
#include <cstdio>
#include <cstdlib>
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
int checks{},failures{};NeverCancelled never;
void check(bool ok,const char* label){++checks;if(!ok){++failures;std::printf("FAILED: %s\n",label);}}
bool near(double a,double b){return std::abs(a-b)<1e-8;}
void rejected(const auto& result,ErrorCode code,const char* label){check(!result.has_value()&&result.error().code==code,label);}
struct Cancel:Cancellation {mutable unsigned polls{};unsigned stop;explicit Cancel(unsigned n):stop(n){}
    bool is_cancelled() const noexcept override{return ++polls>=stop;}};
void put(std::vector<std::byte>& b,std::size_t at,std::uint64_t value,unsigned count){for(unsigned i=0;i<count;++i)b[at+i]=static_cast<std::byte>((value>>(8*i))&255);}
void fix_crc(std::vector<std::byte>& b) {
    std::uint32_t crc=0xffffffffu;for(std::size_t i=32;i<b.size();++i){crc^=std::to_integer<std::uint8_t>(b[i]);for(unsigned n=0;n<8;++n)crc=crc&1?(crc>>1)^0xedb88320u:crc>>1;}
    put(b,12,crc^0xffffffffu,4);
}
FrameSpec frame(){FrameSpec f;f.layer_width=f.layer_height=f.frame_width=f.frame_height=32;
    f.region_of_interest={0,0,32,32};f.time={0,1};f.frame_duration={1,60};return f;}
Vec3 transform(Vec3 p,const std::array<double,16>& m){return {p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};}
}
int main() {
    using namespace starfield::core;
    const auto cube_result=make_unit_cube();check(cube_result.has_value(),"cube fixture exists");const auto cube=cube_result.value();
    const auto encoded=encode_model_geometry(cube,never);check(encoded.has_value()&&encoded.value().size()==960,"cube has exact explicit SFMG size");
    auto decoded=decode_model_geometry(encoded.value(),never);check(decoded.has_value(),"resource decodes");
    const auto again=encode_model_geometry(decoded.value(),never);check(again.has_value()&&again.value()==encoded.value(),"resource bytes are canonical and repeatable");
    check(near(decoded.value().bounds.minimum.x,-.5)&&near(decoded.value().bounds.maximum.z,.5),"bounds recompute from referenced geometry");
    auto forged=cube;forged.bounds={{99,99,99},{100,100,100}};
    check(encode_model_geometry(forged,never).value()==encoded.value(),"serialized resource does not trust cached bounds");
    for(std::size_t n=0;n<encoded.value().size();++n)
        rejected(decode_model_geometry(std::span<const std::byte>(encoded.value()).first(n),never),ErrorCode::invalid_request,"every truncated cube resource rejects");
    for(std::size_t n=32;n<encoded.value().size();++n) {
        auto corrupt=encoded.value();corrupt[n]^=std::byte{1};rejected(decode_model_geometry(corrupt,never),ErrorCode::invalid_request,"payload corruption rejects by checksum");
    }
    auto bad=encoded.value();bad.push_back(std::byte{});rejected(decode_model_geometry(bad,never),ErrorCode::invalid_request,"trailing bytes reject");
    for(std::size_t at:{0u,4u,6u,8u}){bad=encoded.value();bad[at]^=std::byte{1};rejected(decode_model_geometry(bad,never),ErrorCode::invalid_request,"unknown magic/version/header/length rejects");}
    for(std::size_t at:{16u,20u,24u,28u}){bad=encoded.value();put(bad,at,65537,4);rejected(decode_model_geometry(bad,never),ErrorCode::work_limit_exceeded,"wire count preflight rejects hard-cap excess");}
    bad=encoded.value();put(bad,32,std::bit_cast<std::uint64_t>(std::numeric_limits<double>::quiet_NaN()),8);fix_crc(bad);
    rejected(decode_model_geometry(bad,never),ErrorCode::invalid_request,"valid checksum cannot admit NaN position");
    bad=encoded.value();put(bad,32+32*8+24*4+24*6,65536,4);fix_crc(bad);
    rejected(decode_model_geometry(bad,never),ErrorCode::invalid_request,"valid checksum cannot admit invalid corner index");
    bad=encoded.value();put(bad,32+32*8+24*4,0,8);put(bad,32+32*8+24*4+8,0,8);put(bad,32+32*8+24*4+16,0,8);fix_crc(bad);
    rejected(decode_model_geometry(bad,never),ErrorCode::invalid_request,"valid checksum cannot admit zero normal");
    auto special=cube;special.positions[0].weight=-0.;special.texture_coordinates[0].z=1e-309;
    auto special_bytes=encode_model_geometry(special,never);check(special_bytes.has_value(),"finite optional weights and UVW encode");
    auto special_copy=decode_model_geometry(special_bytes.value(),never);check(special_copy.has_value()&&std::signbit(special_copy.value().positions[0].weight)&&special_copy.value().texture_coordinates[0].z==1e-309,"codec preserves signed zero and subnormal metadata");
    Cancel immediate(1),in_crc(4);rejected(decode_model_geometry(encoded.value(),immediate),ErrorCode::cancelled,"resource decode cancellation before work");
    rejected(encode_model_geometry(cube,in_crc),ErrorCode::cancelled,"resource encoding cancellation inside validation/records");
    fail_allocation=true;auto allocation_encode=encode_model_geometry(cube,never);auto allocation_decode=decode_model_geometry(encoded.value(),never);fail_allocation=false;
    rejected(allocation_encode,ErrorCode::allocation_failed,"resource encode catches allocation failure");rejected(allocation_decode,ErrorCode::allocation_failed,"resource decode catches allocation failure");
    ModelResource resource;resource.id[0]=1;resource.geometry=cube;
    std::vector<ModelResource> sources{resource};check(validate_model_resources(sources,never).has_value(),"nonzero independent resource validates");
    sources.push_back(resource);rejected(validate_model_resources(sources,never),ErrorCode::invalid_request,"duplicate resource IDs reject");
    sources.resize(1);sources[0].id={};rejected(validate_model_resources(sources,never),ErrorCode::invalid_request,"zero imported resource ID rejects");
    sources[0]=resource;sources[0].geometry.triangles[0].corners[0].position=999;
    rejected(validate_model_resources(sources,never),ErrorCode::invalid_request,"source geometry validates independently of an encoded checksum");
    sources.assign(257,resource);rejected(validate_model_resources(sources,never),ErrorCode::work_limit_exceeded,"source count is bounded");
    sources.clear();ModelGeometry large;
    large.positions.resize(65536);large.positions[0].value={0,0,0};large.positions[1].value={1,0,0};large.positions[2].value={0,1,0};
    large.texture_coordinates.resize(65536);large.normals.assign(65536,{0,0,1});large.triangles.resize(65536);
    for(auto& t:large.triangles){t.corners[0].position=0;t.corners[1].position=1;t.corners[2].position=2;}
    for(unsigned i=0;i<9;++i){ModelResource r;r.id[0]=static_cast<std::uint8_t>(i+1);r.geometry=large;sources.push_back(std::move(r));}
    rejected(validate_model_resources(sources,never),ErrorCode::work_limit_exceeded,"aggregate source bytes preflight prevents oversized request");
    sources.clear();sources.push_back(resource);Cancel source_cancel(1);rejected(validate_model_resources(sources,source_cancel),ErrorCode::cancelled,"source validation cancellation propagates");
    ParticleInstance p;p.size_pixels=8;p.size_y_pixels=8;p.opacity=1;p.shape=3;p.up_axis=2;
    auto f=frame();auto matrix=model_particle_matrix(p,f);check(matrix.has_value(),"default particle pose compiles");
    check(matrix.value()[0]==8&&matrix.value()[5]==-8&&matrix.value()[10]==8&&matrix.value()[12]==16&&matrix.value()[13]==16,"pose performs one final upward-to-downward Y conversion");
    auto par=f;par.pixel_aspect_ratio=2;check(model_particle_matrix(p,par).value()[0]==4,"uniform physical cube compensates output PAR on X");
    ModelProjection projection;projection.frame=f;projection.model_to_layer=matrix.value();auto scene=project_model_scene(cube,projection,never);
    check(scene.has_value(),"particle matrix reaches real triangle projection");auto surface=rasterize_model_scene(scene.value(),never);
    check(surface.has_value()&&surface.value().pixels.size()==64,"particle cube renders exact8x8 coverage");
    const ParticleSpriteBasis reflected_shear{-1,.25,0, 0,1,.125, 0,0,2};const std::array<ParticleSpriteBasis,1> bases{reflected_shear};
    RenderRequest request;request.frame=f;const auto grid=sprite_geometry::make_grid(f);
    for(unsigned up=0;up<3;++up)for(unsigned a=0;a<4;++a)for(bool limit:{false,true})for(bool inherited:{false,true}) {
        p.up_axis=up;p.rotation_degrees={a*23.,a*31.,a*47.};p.limit_to_2d=limit;p.sprite_basis_index=inherited?1:0;
        auto pose=model_particle_matrix(p,f,bases);check(pose.has_value(),"rotated/up-axis/Transform model pose compiles");
        sprite_geometry::Sprite sprite;check(sprite_geometry::project_sprite(p,request,grid,sprite,bases),"matching texture plane projects");
        const auto centre=transform({},pose.value()),ax=transform({.5,0,0},pose.value()),by=transform({0,-.5,0},pose.value());
        check(near(ax.x-centre.x,sprite.ax)&&near(ax.y-centre.y,sprite.ay)&&near(by.x-centre.x,sprite.bx)&&near(by.y-centre.y,sprite.by),"Model XY plane follows existing rotation/up-axis/limit/Transform convention");
    }
    p={};p.size_pixels=8;p.up_axis=2;p.anchor_x_percent=0;p.anchor_y_percent=100;
    auto anchored=model_particle_matrix(p,f);check(anchored.has_value()&&anchored.value()[12]==20&&anchored.value()[13]==20,"particle anchor offsets unit-size Model frame");
    ParticleModelInstance local;local.model_to_particle[12]=2;
    check(model_particle_matrix(p,f,{},local).value()[12]==36,"source-local origin preserves its affine translation before particle scale");
    p.anchor_x_percent=p.anchor_y_percent=50;p.size_pixels=0;
    auto collapsed=model_particle_matrix(p,f);check(collapsed.has_value()&&collapsed.value()[0]==0,"zero particle size yields legal collapsed pose");
    p.size_pixels=100000;p.sprite_basis_index=1;const std::array<ParticleSpriteBasis,1> huge{{{1e12,0,0,0,1e12,0,0,0,1e12}}};
    check(model_particle_matrix(p,f,huge).has_value(),"largest shared basis composes beyond old1e12 layer-matrix cap");
    p.sprite_basis_index=2;rejected(model_particle_matrix(p,f,bases),ErrorCode::invalid_request,"missing shared basis rejects");
    p.sprite_basis_index=0;p.up_axis=3;rejected(model_particle_matrix(p,f),ErrorCode::invalid_request,"bad Up Axis rejects");
    p.up_axis=2;p.rotation_degrees.x=std::numeric_limits<double>::infinity();rejected(model_particle_matrix(p,f),ErrorCode::invalid_request,"nonfinite particle rotation rejects");
    p.rotation_degrees={};local.model_to_particle[3]=1;rejected(model_particle_matrix(p,f,{},local),ErrorCode::invalid_request,"source-local matrix must be affine");
    std::printf("Model resources: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
