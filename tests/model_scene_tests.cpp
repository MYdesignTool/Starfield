#include "starfield/core/ModelScene.hpp"
#include <algorithm>
#include <cmath>
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
void check(bool v,const char* label){++checks;if(!v){++failures;std::printf("FAILED: %s\n",label);}}
bool near(double a,double b,double eps=1e-8){return std::abs(a-b)<=eps;}
struct Cancel:Cancellation {mutable unsigned polls{};unsigned stop;explicit Cancel(unsigned n):stop(n){}
    bool is_cancelled() const noexcept override{return ++polls>=stop;}};
ModelProjection projection() {
    ModelProjection p;p.frame.layer_width=p.frame.layer_height=p.frame.frame_width=p.frame.frame_height=32;
    p.frame.region_of_interest={0,0,32,32};p.frame.time={0,1};p.frame.frame_duration={1,60};
    p.model_to_layer={8,0,0,0, 0,8,0,0, 0,0,8,0, 16,16,10,1};return p;
}
ModelProjection perspective() {
    auto p=projection();p.model_to_layer={1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    p.camera.enabled=true;p.camera.layer_to_view=p.model_to_layer;
    p.camera.image_to_layer={1,0,0, 0,1,0, 0,0,1};p.camera.focal_x=p.camera.focal_y=8;
    p.camera.center_x=p.camera.center_y=16;p.camera.near_clip=.5;return p;
}
ModelGeometry parse(const char* text){auto r=parse_model_obj(text,never);check(r.has_value(),"test geometry parses");return r.take_value();}
ModelSurface draw(const ModelGeometry& mesh,const ModelProjection& p) {
    auto s=project_model_scene(mesh,p,never);check(s.has_value(),"Model projection succeeds");
    if(!s.has_value())return {};
    auto r=rasterize_model_scene(s.value(),never);check(r.has_value(),"Model rasterization succeeds");
    return r.has_value()?r.take_value():ModelSurface{};
}
ModelSurfacePixel pixel(const ModelSurface& s,int x,int y) {
    if(x<s.region.left||x>=s.region.right||y<s.region.top||y>=s.region.bottom)return {};
    return s.pixels[static_cast<std::size_t>(y-s.region.top)*static_cast<std::size_t>(s.region.width())+x-s.region.left];
}
void same(const ModelSurface& a,const ModelSurface& b,const char* label) {
    for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
        const auto pa=pixel(a,x,y),pb=pixel(b,x,y);
        check(pa.coverage==pb.coverage&&(pa.coverage==0||near(pa.depth,pb.depth)),label);
    }
}
double edge(const ModelProjectedVertex& a,const ModelProjectedVertex& b,double x,double y){return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);}
void failed(const auto& r,ErrorCode code,const char* label){check(!r.has_value()&&r.error().code==code,label);}
}
int main() {
    using namespace starfield::core;
    auto cube_result=make_unit_cube();check(cube_result.has_value(),"default cube exists");const auto cube=cube_result.value();
    auto p=projection();auto scene=project_model_scene(cube,p,never);check(scene.has_value(),"orthographic cube projects");
    auto base=draw(cube,p);check(base.region.left==12&&base.region.top==12&&base.region.right==20&&base.region.bottom==20,"surface uses bounded cube rectangle");
    check(base.pixels.size()==64,"cube surface contains64 pixels");
    for(const auto& v:base.pixels){check(v.coverage==1,"cube shared edges have no holes or repeated opacity");check(near(v.depth,6),"cube resolves front surface depth");}
    auto fraction=p;fraction.model_to_layer[12]+=.5;fraction.model_to_layer[13]+=.5;
    auto fractional=draw(cube,fraction);
    check(pixel(fractional,12,12).coverage==.25f&&pixel(fractional,12,16).coverage==.5f,"quarter-pixel samples produce union coverage along mesh silhouettes");
    auto reversed=cube;for(auto& t:reversed.triangles)std::swap(t.corners[1],t.corners[2]);
    same(base,draw(reversed,p),"both windings preserve solid cube coverage/depth");
    auto duplicate=cube;duplicate.triangles.insert(duplicate.triangles.end(),cube.triangles.begin(),cube.triangles.end());
    same(base,draw(duplicate,p),"coincident triangles do not amplify opacity");
    auto reflected=p;reflected.model_to_layer[0]=-8;same(base,draw(cube,reflected),"reflection retains mesh coverage/depth");
    auto shear=p;shear.model_to_layer[4]=4;const auto sheared=draw(cube,shear);
    check(pixel(sheared,16,16).coverage==1&&sheared.region.width()==12,"affine shear retains filled centre");
    auto down=p;down.frame.layer_width=down.frame.layer_height=64;
    for(unsigned i:{0u,5u,12u,13u})down.model_to_layer[i]*=2;
    same(base,draw(cube,down),"downsample maps full-resolution layer geometry consistently");
    auto par=p;par.frame.pixel_aspect_ratio=2;par.model_to_layer[0]=4;
    auto anamorphic=draw(cube,par);check(anamorphic.region.width()==4&&anamorphic.region.height()==8,"explicit physical-to-layer X conversion preserves PAR");
    for(int y=0;y<32;y+=8)for(int x=0;x<32;x+=8) {
        auto tiled=p;tiled.frame.region_of_interest={x,y,x+8,y+8};const auto part=draw(cube,tiled);
        for(int py=y;py<y+8;++py)for(int px=x;px<x+8;++px) {
            const auto a=pixel(base,px,py),b=pixel(part,px,py);
            check(a.coverage==b.coverage&&(a.coverage==0||near(a.depth,b.depth)),"ROI clipping preserves full-frame samples");
        }
    }
    auto tri=parse("v -1 -1 1\nv 1 -1 2\nv 0 1 4\nvt 0 0\nvt 1 0\nvt 0 1\nf 1/1 2/2 3/3\n");
    auto camera=perspective();auto projected=project_model_scene(tri,camera,never);check(projected.has_value()&&projected.value().triangles.size()==1,"perspective triangle remains one triangle");
    const auto& t=projected.value().triangles.front();
    check(near(t.vertices[0].x,8)&&near(t.vertices[0].y,8)&&near(t.vertices[1].x,20)&&near(t.vertices[2].y,18),"perspective vertex coordinates match analytic projection");
    check(near(t.vertices[1].texture_over_w.x,.5)&&near(t.vertices[2].texture_over_w.y,.25),"UV payloads carry reciprocal homogeneous weight");
    const auto rendered=draw(tri,camera);const auto at=pixel(rendered,15,13);double expected=std::numeric_limits<double>::infinity();
    for(double oy:{.25,.75})for(double ox:{.25,.75}) {
        const double area=edge(t.vertices[0],t.vertices[1],t.vertices[2].x,t.vertices[2].y);
        const double a=edge(t.vertices[1],t.vertices[2],15+ox,13+oy)/area,
            b=edge(t.vertices[2],t.vertices[0],15+ox,13+oy)/area,c=1-a-b;
        if(a>=0&&b>=0&&c>=0)expected=std::min(expected,1/(a/1+b/2+c/4));
    }
    check(at.coverage==1&&near(at.depth,expected),"depth interpolation is perspective-correct at all four samples");
    auto cliptri=parse("v -1 -1 .25\nv 1 -1 2\nv 0 1 2\nvt 0 0\nvt 1 0\nvt .5 1\nf 1/1 2/2 3/3\n");
    camera.camera.near_clip=1;auto clipped=project_model_scene(cliptri,camera,never);
    check(clipped.has_value()&&clipped.value().triangles.size()==2,"one behind-near vertex clips to two triangles");
    unsigned near_vertices=0;
    for(const auto& ct:clipped.value().triangles)for(const auto& v:ct.vertices) {
        const double z=v.depth_over_w/v.reciprocal_w;
        check(z>=1-1e-12&&v.x>=0&&v.x<=32&&v.y>=0&&v.y<=32,"clipped vertices stay in near/ROI bounds");
        if(near(z,1)){++near_vertices;check(near(v.texture_over_w.x/v.reciprocal_w,3./7)||near(v.texture_over_w.x/v.reciprocal_w,3./14),"near clipping interpolates authored UVs before perspective division");}
    }
    check(near_vertices>=2,"near-plane intersections exist");
    auto behind=cliptri;for(auto& v:behind.positions)v.value.z=.1;
    auto invisible=draw(behind,camera);check(invisible.pixels.empty(),"whole mesh behind near plane is invisible");
    auto straddle=camera;straddle.model_to_layer={2,0,0,0, 0,2,0,0, 0,0,2,0, 0,0,.75,1};
    const auto crossing=draw(cube,straddle);check(!crossing.pixels.empty()&&pixel(crossing,16,16).coverage==1,"mesh crossing near plane remains visible even with centre behind it");
    auto neg=camera;for(auto& v:neg.camera.image_to_layer)v*=-7;same(draw(cliptri,camera),draw(cliptri,neg),"homography scale/sign does not change projection");
    auto warp=camera;warp.camera.image_to_layer[6]=.02;const auto warped=draw(cliptri,warp);
    check(!warped.pixels.empty(),"projective image-to-layer transform rasterizes");
    auto horizon=camera;horizon.camera.image_to_layer[6]=1;horizon.camera.image_to_layer[8]=-16;
    failed(project_model_scene(cube,horizon,never),ErrorCode::invalid_request,"principal-point horizon rejects explicitly");
    auto horizon_crossing=camera;horizon_crossing.camera.image_to_layer[6]=.1;horizon_crossing.camera.image_to_layer[8]=-1.5;
    auto wide=parse("v -1 -1 2\nv 12 -1 2\nv 0 1 2\nf 1 2 3\n");
    const auto principal_branch=draw(wide,horizon_crossing);
    check(!principal_branch.pixels.empty(),"mesh crossing homography horizon clips to visible principal branch");
    ModelSceneLimits caps;caps.projected_triangles=1;
    failed(project_model_scene(cube,p,never,caps),ErrorCode::work_limit_exceeded,"projected triangle cap is enforced");
    caps={};caps.surface_pixels=63;failed(rasterize_model_scene(scene.value(),never,caps),ErrorCode::work_limit_exceeded,"surface pixel cap is enforced");
    caps={};caps.sample_visits=0;failed(rasterize_model_scene(scene.value(),never,caps),ErrorCode::work_limit_exceeded,"sample visit cap is enforced");
    caps={};++caps.surface_pixels;failed(rasterize_model_scene(scene.value(),never,caps),ErrorCode::invalid_request,"caller cannot raise hard limits");
    auto bad=p;bad.model_to_layer[3]=1;failed(project_model_scene(cube,bad,never),ErrorCode::invalid_request,"non-affine model matrix rejects");
    bad=p;bad.model_to_layer[0]=1e13;failed(project_model_scene(cube,bad,never),ErrorCode::invalid_request,"unbounded model matrix rejects");
    bad=p;bad.frame.pixel_aspect_ratio=0;failed(project_model_scene(cube,bad,never),ErrorCode::invalid_request,"invalid frame rejects");
    auto raw=scene.value();raw.triangles[0].vertices[0].x=std::numeric_limits<double>::quiet_NaN();
    failed(rasterize_model_scene(raw,never),ErrorCode::invalid_request,"nonfinite projected vertex rejects");
    raw=scene.value();raw.triangles[0].vertices[0].reciprocal_w=0;failed(rasterize_model_scene(raw,never),ErrorCode::invalid_request,"zero perspective denominator rejects");
    raw=scene.value();raw.triangles[0].source_triangle=kMaxModelTriangles;failed(rasterize_model_scene(raw,never),ErrorCode::invalid_request,"invalid source triangle rejects");
    Cancel immediate(1),later(10);failed(project_model_scene(cube,p,immediate),ErrorCode::cancelled,"projection honours immediate cancellation");
    failed(rasterize_model_scene(scene.value(),later),ErrorCode::cancelled,"raster honours cancellation during work");
    fail_allocation=true;auto a=project_model_scene(cube,p,never);auto b=rasterize_model_scene(scene.value(),never);fail_allocation=false;
    failed(a,ErrorCode::allocation_failed,"projection catches allocation failure");failed(b,ErrorCode::allocation_failed,"raster catches allocation failure");
    caps={};caps.sample_visits=0;fail_allocation=true;auto before_allocation=rasterize_model_scene(scene.value(),never,caps);fail_allocation=false;
    failed(before_allocation,ErrorCode::work_limit_exceeded,"sample work preflight rejects before allocating depth storage");
    const RectI output{0,0,32,32};
    for(auto mode:{ParticleTransferMode::normal,ParticleTransferMode::add,ParticleTransferMode::screen,ParticleTransferMode::stencil}) {
        std::vector<float> d(32*32*4,0);
        auto one=composite_model_surface(base,{.8,.4,.2},.5,mode,output,d,never);
        check(one.has_value()&&one.value()==64,"all transfer modes compose one logical cube");
        auto two=composite_model_surface(base,{.8,.4,.2},.5,mode,output,d,never);check(two.has_value(),"second logical particle composes");
        const auto offset=(16*32+16)*4;
        const double red=mode==ParticleTransferMode::normal?.6:mode==ParticleTransferMode::add?.8:mode==ParticleTransferMode::screen?.64:0;
        check(near(d[offset],red,1e-6)&&near(d[offset+3],mode==ParticleTransferMode::stencil?0:.75),"transfer color/alpha matches existing CPU equations");
    }
    std::vector<float> hdr(32*32*4,0);check(composite_model_surface(base,{64,4,2},.5,ParticleTransferMode::screen,output,hdr,never).has_value(),"HDR screen first particle");
    check(composite_model_surface(base,{64,4,2},.5,ParticleTransferMode::screen,output,hdr,never).has_value(),"HDR screen second particle");
    check(near(hdr[(16*32+16)*4],-960)&&near(hdr[(16*32+16)*4+3],.75),"HDR screen preserves signed working-space result");
    std::vector<float> dest(32*32*4,0);auto invalid_surface=base;invalid_surface.pixels.back().coverage=2;
    const auto unchanged=dest;
    failed(composite_model_surface(invalid_surface,{1,1,1},1,ParticleTransferMode::normal,output,dest,never),ErrorCode::invalid_request,"late bad source pixel rejects before mutation");
    check(dest==unchanged,"invalid composition leaves destination unchanged");
    dest[0]=std::numeric_limits<float>::quiet_NaN();
    check(composite_model_surface(base,{1,1,1},1,ParticleTransferMode::normal,output,dest,never).has_value(),"composition does not scan untouched pixels outside the mesh surface");
    check(std::isnan(dest[0]),"composition leaves untouched staging bytes alone");
    dest=unchanged;dest[(19*32+19)*4]=std::numeric_limits<float>::infinity();const auto untouched_corner=dest[(12*32+12)*4];
    failed(composite_model_surface(base,{1,1,1},1,ParticleTransferMode::normal,output,dest,never),ErrorCode::invalid_request,"late nonfinite touched destination rejects before mutation");
    check(dest[(12*32+12)*4]==untouched_corner,"invalid touched destination leaves prior pixels unchanged");
    failed(composite_model_surface(base,{1,1,1},2,ParticleTransferMode::normal,output,dest,never),ErrorCode::invalid_request,"invalid opacity rejects");
    Cancel composition_cancel(1);failed(composite_model_surface(base,{1,1,1},1,ParticleTransferMode::normal,output,dest,composition_cancel),ErrorCode::cancelled,"composition cancellation propagates");
    dest=unchanged;Cancel after_row(4);
    failed(composite_model_surface(base,{1,1,1},1,ParticleTransferMode::normal,output,dest,after_row),ErrorCode::cancelled,"composition may cancel after a completed row");
    check(dest[(12*32+12)*4]==1&&dest[(13*32+12)*4]==0,"cancelled caller-owned staging is partial and must be discarded");
    dest=unchanged;
    for(std::size_t i=0;i<dest.size();i+=4){dest[i]=.2f;dest[i+1]=.4f;dest[i+2]=.6f;dest[i+3]=.8f;}
    check(composite_model_surface(base,{1,1,1},.5,ParticleTransferMode::stencil,output,dest,never).has_value(),"stencil applies to existing accumulation");
    check(near(dest[(16*32+16)*4],.1,1e-6)&&near(dest[(16*32+16)*4+3],.4,1e-6),"stencil removes color and alpha together");
    std::printf("Model scene: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
