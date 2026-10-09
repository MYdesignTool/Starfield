#include "starfield/core/ModelScene.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <new>

namespace starfield::core {
namespace {
constexpr ModelSceneLimits hard_limits{};
constexpr double max_matrix=1e12;
constexpr double minimum_w=1e-200;
bool finite(Vec3 a) noexcept {return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
bool limits_valid(ModelSceneLimits l) noexcept {
    return l.projected_triangles<=hard_limits.projected_triangles &&
        l.surface_pixels<=hard_limits.surface_pixels && l.sample_visits<=hard_limits.sample_visits;
}
template<std::size_t N> bool bounded_matrix(const std::array<double,N>& m) noexcept {
    return std::all_of(m.begin(),m.end(),[](double x){return std::isfinite(x)&&std::abs(x)<=max_matrix;});
}
bool affine(const std::array<double,16>& m) noexcept {
    return bounded_matrix(m)&&m[3]==0&&m[7]==0&&m[11]==0&&m[15]==1;
}
Vec3 transform(Vec3 p,const std::array<double,16>& m) noexcept {
    return {p.x*m[0]+p.y*m[4]+p.z*m[8]+m[12],
        p.x*m[1]+p.y*m[5]+p.z*m[9]+m[13],p.x*m[2]+p.y*m[6]+p.z*m[10]+m[14]};
}
Vec3 interpolate(Vec3 a,Vec3 b,double t) noexcept {
    return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};
}
struct ClipVertex {Vec3 screen;double depth{};Vec3 uv;};
bool finite(const ClipVertex& v) noexcept {return finite(v.screen)&&std::isfinite(v.depth)&&finite(v.uv);}
double distance(const ClipVertex& v,unsigned plane,const RectI& roi,double near_clip) noexcept {
    switch(plane) {
        case 0:return v.depth-near_clip;
        case 1:return v.screen.z-minimum_w;
        case 2:return v.screen.x-roi.left*v.screen.z;
        case 3:return roi.right*v.screen.z-v.screen.x;
        case 4:return v.screen.y-roi.top*v.screen.z;
        default:return roi.bottom*v.screen.z-v.screen.y;
    }
}
// Make shared-edge intersections independent of the input winding.
ClipVertex intersection(ClipVertex a,ClipVertex b,double da,double db) noexcept {
    if(a.screen.x>b.screen.x || (a.screen.x==b.screen.x && (a.screen.y>b.screen.y ||
        (a.screen.y==b.screen.y && a.screen.z>b.screen.z)))) {std::swap(a,b);std::swap(da,db);}
    const double t=std::clamp(da/(da-db),0.,1.);
    return {interpolate(a.screen,b.screen,t),a.depth+(b.depth-a.depth)*t,interpolate(a.uv,b.uv,t)};
}
bool rectangle_valid(RectI r) noexcept {
    return r.left>=0&&r.top>=0&&r.right>=r.left&&r.bottom>=r.top&&r.right<=32768&&r.bottom<=32768;
}
double edge(const ModelProjectedVertex& a,const ModelProjectedVertex& b,double x,double y) noexcept {
    return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);
}
bool inclusive(const ModelProjectedVertex& a,const ModelProjectedVertex& b) noexcept {
    const auto dy=b.y-a.y,dx=b.x-a.x;return dy<0 || (dy==0&&dx>0);
}
bool inside(double value,bool include) noexcept {return value>0 || (value==0&&include);}
RectI triangle_bounds(const ModelProjectedTriangle& t,RectI region) noexcept {
    double x0=t.vertices[0].x,x1=x0,y0=t.vertices[0].y,y1=y0;
    for(const auto& v:t.vertices) {x0=std::min(x0,v.x);x1=std::max(x1,v.x);y0=std::min(y0,v.y);y1=std::max(y1,v.y);}
    return intersect(region,{static_cast<std::int32_t>(std::floor(x0)),static_cast<std::int32_t>(std::floor(y0)),
        static_cast<std::int32_t>(std::ceil(x1)),static_cast<std::int32_t>(std::ceil(y1))});
}
}

Result<ModelScene> project_model_scene(const ModelGeometry& mesh,const ModelProjection& input,
    const Cancellation& cancellation,ModelSceneLimits limits) noexcept {
    using R=Result<ModelScene>;
    if(!limits_valid(limits))return R::failure(ErrorCode::invalid_request,"Model limits exceed hard caps");
    const auto frame_result=validate_frame(input.frame);if(!frame_result.has_value())return R::failure(frame_result.error());
    const auto valid=validate_model_geometry(mesh,cancellation);if(!valid.has_value())return R::failure(valid.error());
    if(!affine(input.model_to_layer))return R::failure(ErrorCode::invalid_request,"Model transform must be bounded affine");
    const auto& frame=frame_result.value();const auto& camera=input.camera;
    std::array<double,9> homography{};
    if(camera.enabled) {
        const double scalars[]{camera.focal_x,camera.focal_y,camera.center_x,camera.center_y,camera.near_clip};
        for(double v:scalars)if(!std::isfinite(v)||std::abs(v)>max_matrix)
            return R::failure(ErrorCode::invalid_request,"invalid Model camera scalar");
        if(!affine(camera.layer_to_view)||!bounded_matrix(camera.image_to_layer)||camera.near_clip<=0 ||
            camera.focal_x<=0||camera.focal_y<=0)return R::failure(ErrorCode::invalid_request,"invalid Model camera");
        double scale=0;for(double v:camera.image_to_layer)scale=std::max(scale,std::abs(v));
        if(!(scale>0))return R::failure(ErrorCode::invalid_request,"zero Model image homography");
        for(unsigned i=0;i<9;++i)homography[i]=camera.image_to_layer[i]/scale;
        const double principal=homography[6]*camera.center_x+homography[7]*camera.center_y+homography[8];
        if(!std::isfinite(principal)||std::abs(principal)<=1e-12)
            return R::failure(ErrorCode::invalid_request,"Model image horizon crosses principal point");
        for(double& v:homography)v/=principal;
    }
    ModelScene scene;scene.region=frame.region_of_interest;
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model projection cancelled");
    if(scene.region.empty())return R::success(std::move(scene));
    const double sx=double(frame.frame_width)/frame.layer_width,sy=double(frame.frame_height)/frame.layer_height;
    try {
        for(std::size_t index=0;index<mesh.triangles.size();++index) {
            if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model projection cancelled");
            std::array<ClipVertex,12> a{},b{};std::size_t count=3;
            for(unsigned corner=0;corner<3;++corner) {
                const auto& c=mesh.triangles[index].corners[corner];
                auto point=transform(mesh.positions[c.position].value,input.model_to_layer);
                if(camera.enabled)point=transform(point,camera.layer_to_view);
                const auto uv=c.texture==kMissingModelAttribute?Vec3{}:mesh.texture_coordinates[c.texture];
                if(camera.enabled) {
                    const double u=camera.center_x*point.z+camera.focal_x*point.x,
                        v=camera.center_y*point.z+camera.focal_y*point.y;
                    a[corner]={{sx*(homography[0]*u+homography[1]*v+homography[2]*point.z),
                        sy*(homography[3]*u+homography[4]*v+homography[5]*point.z),
                        homography[6]*u+homography[7]*v+homography[8]*point.z},point.z,uv};
                } else a[corner]={{sx*point.x,sy*point.y,1},point.z,uv};
                if(!finite(a[corner]))return R::failure(ErrorCode::invalid_request,"nonfinite Model projection");
            }
            for(unsigned plane=camera.enabled?0:1;plane<6&&count;++plane) {
                std::size_t out=0;
                auto previous=a[count-1];double dp=distance(previous,plane,scene.region,camera.near_clip);
                for(std::size_t i=0;i<count;++i) {
                    const auto current=a[i];const double dc=distance(current,plane,scene.region,camera.near_clip);
                    if((dp>=0)!=(dc>=0)) {
                        auto v=intersection(previous,current,dp,dc);
                        // Snap the active clip coordinate to the plane. A tiny
                        // positive W can otherwise round back to zero at a horizon.
                        if(plane==0)v.depth=camera.near_clip;
                        else if(plane==1)v.screen.z=minimum_w;
                        else if(plane==2)v.screen.x=scene.region.left*v.screen.z;
                        else if(plane==3)v.screen.x=scene.region.right*v.screen.z;
                        else if(plane==4)v.screen.y=scene.region.top*v.screen.z;
                        else v.screen.y=scene.region.bottom*v.screen.z;
                        b[out++]=v;
                    }
                    if(dc>=0)b[out++]=current;
                    previous=current;dp=dc;
                }
                count=out;a=b;
            }
            for(std::size_t i=1;i+1<count;++i) {
                if(scene.triangles.size()>=limits.projected_triangles)
                    return R::failure(ErrorCode::work_limit_exceeded,"Model projected triangle cap exceeded");
                ModelProjectedTriangle triangle;triangle.source_triangle=static_cast<std::uint32_t>(index);
                const std::size_t corners[]{0,i,i+1};
                for(unsigned c=0;c<3;++c) {
                    const auto& v=a[corners[c]];
                    if(!finite(v)||!(v.screen.z>0))return R::failure(ErrorCode::invalid_request,"invalid clipped Model vertex");
                    const auto iw=1/v.screen.z;
                    triangle.vertices[c]={std::clamp(v.screen.x*iw,double(scene.region.left),double(scene.region.right)),
                        std::clamp(v.screen.y*iw,double(scene.region.top),double(scene.region.bottom)),iw,v.depth*iw,
                        {v.uv.x*iw,v.uv.y*iw,v.uv.z*iw}};
                    const auto& p=triangle.vertices[c];
                    if(!std::isfinite(p.reciprocal_w)||!std::isfinite(p.depth_over_w)||!finite(p.texture_over_w))
                        return R::failure(ErrorCode::invalid_request,"nonfinite Model perspective payload");
                }
                if(edge(triangle.vertices[0],triangle.vertices[1],triangle.vertices[2].x,triangle.vertices[2].y)!=0)
                    scene.triangles.push_back(triangle);
            }
        }
        return R::success(std::move(scene));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"Model projection allocation failed");}
    catch(...) {return R::failure(ErrorCode::internal_failure,"Model projection failed");}
}

Result<ModelSurface> rasterize_model_scene(const ModelScene& scene,const Cancellation& cancellation,
    ModelSceneLimits limits) noexcept {
    using R=Result<ModelSurface>;
    if(!limits_valid(limits)||!rectangle_valid(scene.region))return R::failure(ErrorCode::invalid_request,"invalid Model surface geometry");
    if(scene.triangles.size()>limits.projected_triangles)return R::failure(ErrorCode::work_limit_exceeded,"Model projected triangle cap exceeded");
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model rasterization cancelled");
    ModelSurface surface;surface.region={scene.region.left,scene.region.top,scene.region.left,scene.region.top};
    std::uint64_t estimated_work=0;
    for(std::size_t i=0;i<scene.triangles.size();++i) {
        if(i%64==0&&cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model raster validation cancelled");
        const auto& t=scene.triangles[i];
        if(t.source_triangle>=kMaxModelTriangles)return R::failure(ErrorCode::invalid_request,"invalid Model source triangle");
        for(const auto& v:t.vertices)if(!std::isfinite(v.x)||!std::isfinite(v.y)||v.x<0||v.y<0||v.x>32768||v.y>32768||
            !std::isfinite(v.reciprocal_w)||v.reciprocal_w<=0||v.reciprocal_w>1e250 ||
            !std::isfinite(v.depth_over_w)||std::abs(v.depth_over_w)>1e250||!finite(v.texture_over_w))
                return R::failure(ErrorCode::invalid_request,"invalid projected Model vertex");
        if(edge(t.vertices[0],t.vertices[1],t.vertices[2].x,t.vertices[2].y)!=0) {
            const auto box=triangle_bounds(t,scene.region);
            if(!box.empty()) {
                const auto visits=static_cast<std::uint64_t>(box.width())*static_cast<std::uint64_t>(box.height())*4;
                if(visits>limits.sample_visits-estimated_work)
                    return R::failure(ErrorCode::work_limit_exceeded,"Model sample visit cap exceeded");
                estimated_work+=visits;surface.region=unite(surface.region,box);
            }
        }
    }
    if(surface.region.empty())return R::success(std::move(surface));
    const auto width=static_cast<std::size_t>(surface.region.width()),height=static_cast<std::size_t>(surface.region.height());
    if(width*height>limits.surface_pixels)return R::failure(ErrorCode::work_limit_exceeded,"Model surface pixel cap exceeded");
    try {
        surface.pixels.resize(width*height);
        std::vector<std::array<double,4>> depths(width*height);
        for(std::size_t i=0;i<depths.size();++i) {
            if(i%4096==0&&cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model depth initialization cancelled");
            depths[i].fill(std::numeric_limits<double>::infinity());
        }
        std::vector<unsigned char> masks(width*height,0);
        std::uint64_t work=0;
        constexpr double offsets[4][2]{{.25,.25},{.75,.25},{.25,.75},{.75,.75}};
        for(auto triangle:scene.triangles) {
            if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model rasterization cancelled");
            auto& v=triangle.vertices;auto area=edge(v[0],v[1],v[2].x,v[2].y);
            if(area==0)continue;if(area<0){std::swap(v[1],v[2]);area=-area;}
            const auto box=triangle_bounds(triangle,surface.region);if(box.empty())continue;
            const auto visits=static_cast<std::uint64_t>(box.width())*static_cast<std::uint64_t>(box.height())*4;
            if(visits>limits.sample_visits-work)return R::failure(ErrorCode::work_limit_exceeded,"Model sample visit cap exceeded");
            work+=visits;
            const bool e0=inclusive(v[1],v[2]),e1=inclusive(v[2],v[0]),e2=inclusive(v[0],v[1]);
            for(std::int32_t y=box.top;y<box.bottom;++y) {
                if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model rasterization cancelled");
                for(std::int32_t x=box.left;x<box.right;++x)for(unsigned s=0;s<4;++s) {
                    const double px=x+offsets[s][0],py=y+offsets[s][1];
                    const double a=edge(v[1],v[2],px,py),b=edge(v[2],v[0],px,py),c=edge(v[0],v[1],px,py);
                    if(!inside(a,e0)||!inside(b,e1)||!inside(c,e2))continue;
                    const double wa=a/area,wb=b/area,wc=c/area;
                    const double iw=wa*v[0].reciprocal_w+wb*v[1].reciprocal_w+wc*v[2].reciprocal_w;
                    const double depth=(wa*v[0].depth_over_w+wb*v[1].depth_over_w+wc*v[2].depth_over_w)/iw;
                    if(!std::isfinite(depth))return R::failure(ErrorCode::invalid_request,"invalid Model interpolated depth");
                    const auto index=static_cast<std::size_t>(y-surface.region.top)*width+static_cast<std::size_t>(x-surface.region.left);
                    if(depth<depths[index][s] || (depth==depths[index][s]&&triangle.source_triangle<surface.pixels[index].source_triangle)) {
                        depths[index][s]=depth;
                        auto& p=surface.pixels[index];
                        if(masks[index]==0||depth<p.depth || (depth==p.depth&&triangle.source_triangle<p.source_triangle)) {
                            p.depth=depth;p.source_triangle=triangle.source_triangle;
                        }
                        masks[index]|=static_cast<unsigned char>(1u<<s);
                    }
                }
            }
        }
        for(std::size_t i=0;i<masks.size();++i) {
            if(i%4096==0&&cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model raster finalization cancelled");
            const auto m=masks[i];surface.pixels[i].coverage=.25f*((m&1)+((m>>1)&1)+((m>>2)&1)+((m>>3)&1));
        }
        return R::success(std::move(surface));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"Model surface allocation failed");}
    catch(...) {return R::failure(ErrorCode::internal_failure,"Model rasterization failed");}
}

Result<std::size_t> composite_model_surface(const ModelSurface& surface,Vec3 color,double opacity,
    ParticleTransferMode mode,RectI region,std::span<float> destination,const Cancellation& cancellation) noexcept {
    using R=Result<std::size_t>;
    if(!rectangle_valid(region)||!rectangle_valid(surface.region)||!contains(region,surface.region)||!finite(color)||
        color.x<0||color.y<0||color.z<0||color.x>kMaxParticleColor||color.y>kMaxParticleColor||color.z>kMaxParticleColor ||
        !std::isfinite(opacity)||opacity<0||opacity>1||static_cast<std::uint32_t>(mode)>3)
        return R::failure(ErrorCode::invalid_request,"invalid Model composition input");
    const auto width=static_cast<std::size_t>(region.width()),sw=static_cast<std::size_t>(surface.region.width());
    const auto count=width*static_cast<std::size_t>(region.height());
    if(destination.size()!=count*4||surface.pixels.size()!=sw*static_cast<std::size_t>(surface.region.height())||
        surface.pixels.size()>hard_limits.surface_pixels)return R::failure(ErrorCode::invalid_request,"invalid Model composition storage");
    for(std::size_t i=0;i<surface.pixels.size();++i) {
        if(i%4096==0&&cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model composition validation cancelled");
        const auto& p=surface.pixels[i];if(!std::isfinite(p.coverage)||p.coverage<0||p.coverage>1||
            (p.coverage>0&&(!std::isfinite(p.depth)||p.source_triangle>=kMaxModelTriangles)))
            return R::failure(ErrorCode::invalid_request,"invalid Model source pixel");
        if(p.coverage>0&&opacity>0) {
            const auto y=static_cast<std::size_t>(surface.region.top-region.top)+i/sw,
                x=static_cast<std::size_t>(surface.region.left-region.left)+i%sw;
            const auto* d=destination.data()+4*(y*width+x);
            for(unsigned c=0;c<4;++c)if(!std::isfinite(d[c])||std::abs(d[c])>1e30 || (c==3&&(d[c]<0||d[c]>1)))
                return R::failure(ErrorCode::invalid_request,"invalid Model destination pixel");
        }
    }
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model composition cancelled");
    std::size_t changed=0;
    for(std::int32_t y=surface.region.top;y<surface.region.bottom;++y) {
        if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Model composition cancelled");
        for(std::int32_t x=surface.region.left;x<surface.region.right;++x) {
            const auto& p=surface.pixels[static_cast<std::size_t>(y-surface.region.top)*sw+x-surface.region.left];
            const float alpha=static_cast<float>(p.coverage*opacity);if(!(alpha>0))continue;
            auto* d=destination.data()+4*(static_cast<std::size_t>(y-region.top)*width+x-region.left);
            const float source[]{static_cast<float>(color.x)*alpha,static_cast<float>(color.y)*alpha,static_cast<float>(color.z)*alpha};
            const float remaining=1-alpha;
            if(mode==ParticleTransferMode::stencil)for(unsigned c=0;c<4;++c)d[c]*=remaining;
            else {
                for(unsigned c=0;c<3;++c) {
                    if(mode==ParticleTransferMode::add)d[c]+=source[c];
                    else if(mode==ParticleTransferMode::screen)d[c]=source[c]+d[c]-source[c]*d[c];
                    else d[c]=source[c]+d[c]*remaining;
                }
                d[3]=alpha+d[3]*remaining;
            }
            ++changed;
        }
    }
    return R::success(changed);
}
}
