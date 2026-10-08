#pragma once
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Render.hpp"
#include "starfield/core/ParticleTransform.hpp"
#include "starfield/core/ParticleCloud.hpp"
#include <algorithm>
#include <cmath>
namespace starfield::core::sprite_geometry {
struct PixelGrid {
    double aspect{1.0};
    double frame_width{1.0};
    double frame_height{1.0};
    double scale_x{1.0};
    double scale_y{1.0};
};

inline PixelGrid make_grid(const FrameSpec& frame) noexcept {
    PixelGrid grid;
    grid.aspect = (static_cast<double>(frame.layer_width) * frame.pixel_aspect_ratio) /
                  static_cast<double>(frame.layer_height);
    grid.frame_width = static_cast<double>(frame.frame_width);
    grid.frame_height = static_cast<double>(frame.frame_height);
    grid.scale_x = grid.frame_width / static_cast<double>(frame.layer_width);
    grid.scale_y = grid.frame_height / static_cast<double>(frame.layer_height);
    return grid;
}

struct Sprite {
    const ParticleInstance* particle{};
    double x{}, y{}, depth{}, ax{}, ay{}, bx{}, by{};
    bool back_facing{};
    const TextureFrameView* texture_frame{};
    const ParticleTextureStyle* texture_style{};
    const ParticleCloudStyle* cloud_style{};
};
inline bool valid_camera(const RenderRequest::Camera& camera) noexcept {
    if (!camera.enabled) return true;
    return std::all_of(camera.layer_to_view.begin(), camera.layer_to_view.end(), [](double v) { return std::isfinite(v); }) &&
        std::all_of(camera.image_to_layer.begin(), camera.image_to_layer.end(), [](double v) { return std::isfinite(v); }) &&
        std::isfinite(camera.focal_x) && camera.focal_x > 0 && std::isfinite(camera.focal_y) && camera.focal_y > 0 &&
        std::isfinite(camera.center_x) && std::isfinite(camera.center_y) && std::isfinite(camera.near_clip) && camera.near_clip > 0;
}
inline Vec3 rotate_axis(Vec3 value,Vec3 angles) noexcept {
    constexpr double radians=3.14159265358979323846/180;
    for(int axis=0;axis<3;++axis) {
        const double angle=(axis==0?angles.x:axis==1?angles.y:-angles.z)*radians;
        const double c=std::cos(angle),s=std::sin(angle);
        if(axis==0) value={value.x,c*value.y-s*value.z,s*value.y+c*value.z};
        if(axis==1) value={c*value.x+s*value.z,value.y,-s*value.x+c*value.z};
        if(axis==2) value={c*value.x-s*value.y,s*value.x+c*value.y,value.z};
    }
    return value;
}
inline bool project_sprite(const ParticleInstance& particle, const RenderRequest& request, const PixelGrid& grid, Sprite& sprite,
    std::span<const ParticleSpriteBasis> bases = {}, double texture_ratio = 0, bool ignore_perspective = false,
    std::span<const ParticleCloudStyle> clouds = {}) noexcept {
    sprite.particle=&particle;
    if(particle.cloud_style_index) {
        if(particle.shape!=2 || particle.cloud_style_index>clouds.size())return false;
        sprite.cloud_style=&clouds[particle.cloud_style_index-1];
    }
    const double rx=particle.size_pixels*.5;
    const bool texture=particle.shape==3;
    const double texture_par=texture?request.frame.pixel_aspect_ratio:1;
    const double physical_rx=rx*texture_par;
    const double ry=texture && texture_ratio>0 ? physical_rx/texture_ratio : (particle.shape==0 || sprite.cloud_style)?rx:particle.size_y_pixels*.5;
    if(!(rx>0) || !(ry>0) || !(particle.opacity>0)) return false;
    const bool billboard=(particle.shape!=1 && particle.shape!=3) || particle.limit_to_2d;
    // Texture axes rotate in physical pixel units. Convert X back to layer
    // pixels at projection, so source PAR is preserved on anamorphic outputs.
    Vec3 a{physical_rx,0,0},b{0,ry,0};
    Vec3 angles=particle.rotation_degrees;
    if(billboard) angles.x=angles.y=0;
    else if(particle.up_axis==0) {a={0,0,physical_rx};b={0,ry,0};}
    else if(particle.up_axis==1) {a={physical_rx,0,0};b={0,0,ry};}
    a=rotate_axis(a,angles);b=rotate_axis(b,angles);
    const bool transformed=particle.sprite_basis_index!=0;
    if(transformed) {
        if(particle.sprite_basis_index>bases.size())return false;
        const auto& basis=bases[particle.sprite_basis_index-1];
        // Base axes are in the historic display convention. The Transform basis
        // acts in canonical world coordinates, where positive Y points upward.
        const auto transform_axis=[&](Vec3 axis) {
            const Vec3 world{axis.x,-axis.y,axis.z};
            return Vec3{basis[0]*world.x+basis[1]*world.y+basis[2]*world.z,
                -(basis[3]*world.x+basis[4]*world.y+basis[5]*world.z),
                basis[6]*world.x+basis[7]*world.y+basis[8]*world.z};
        };
        a=transform_axis(a);b=transform_axis(b);
    }
    if(!request.camera.enabled) {
        sprite.x=(.5+particle.position.x/grid.aspect)*grid.frame_width;
        sprite.y=(.5-particle.position.y)*grid.frame_height;
        sprite.ax=a.x/texture_par*grid.scale_x;sprite.ay=a.y*grid.scale_y;
        sprite.bx=b.x/texture_par*grid.scale_x;sprite.by=b.y*grid.scale_y;
        sprite.back_facing = a.x*b.y-a.y*b.x < 0;
    } else {
        const auto& camera=request.camera;const auto& frame=request.frame;
        const double local[4]{frame.layer_width*.5+particle.position.x*frame.layer_height/frame.pixel_aspect_ratio,
            (.5-particle.position.y)*frame.layer_height,particle.position.z*frame.layer_height,1};
        double view[3]{};
        for(int col=0;col<3;++col) for(int row=0;row<4;++row) view[col]+=local[row]*camera.layer_to_view[row*4+col];
        sprite.depth=view[2];if(!(view[2]>=camera.near_clip)) return false;
        const double u=camera.center_x+camera.focal_x*view[0]/view[2],v=camera.center_y+camera.focal_y*view[1]/view[2];
        const auto& m=camera.image_to_layer;const double w=m[6]*u+m[7]*v+m[8];
        if(!std::isfinite(w) || std::abs(w)<1e-12) return false;
        const double x=(m[0]*u+m[1]*v+m[2])/w,y=(m[3]*u+m[4]*v+m[5])/w;
        sprite.x=x*grid.scale_x;sprite.y=y*grid.scale_y;
        const auto view_axis=[&](Vec3 axis) {
            Vec3 axis_view{axis.x,-axis.y,0};
            if(!billboard || transformed) {
                const double local_axis[3]{axis.x/frame.pixel_aspect_ratio,-axis.y,axis.z};
                axis_view={};
                for(int row=0;row<3;++row) {
                    axis_view.x+=local_axis[row]*camera.layer_to_view[row*4];
                    axis_view.y+=local_axis[row]*camera.layer_to_view[row*4+1];
                    axis_view.z+=local_axis[row]*camera.layer_to_view[row*4+2];
                }
            }
            return axis_view;
        };
        const auto av=view_axis(a), bv=view_axis(b);
        const Vec3 normal{av.y*bv.z-av.z*bv.y,av.z*bv.x-av.x*bv.z,av.x*bv.y-av.y*bv.x};
        sprite.back_facing=normal.x*view[0]+normal.y*view[1]+normal.z*view[2] > 0;
        const auto project_axis=[&](Vec3 axis_view,double& dx,double& dy) {
            const double du=ignore_perspective?axis_view.x:camera.focal_x*(axis_view.x*view[2]-view[0]*axis_view.z)/(view[2]*view[2]);
            const double dv=ignore_perspective?axis_view.y:camera.focal_y*(axis_view.y*view[2]-view[1]*axis_view.z)/(view[2]*view[2]);
            dx=((m[0]-x*m[6])*du+(m[1]-x*m[7])*dv)/w*grid.scale_x;
            dy=((m[3]-y*m[6])*du+(m[4]-y*m[7])*dv)/w*grid.scale_y;
        };
        project_axis(av,sprite.ax,sprite.ay);project_axis(bv,sprite.bx,sprite.by);
    }
    const double anchor_x=1-particle.anchor_x_percent/50,anchor_y=particle.anchor_y_percent/50-1;
    sprite.x+=sprite.ax*anchor_x+sprite.bx*anchor_y;
    sprite.y+=sprite.ay*anchor_x+sprite.by*anchor_y;
    return std::isfinite(sprite.x) && std::isfinite(sprite.y) && std::isfinite(sprite.ax) &&
        std::isfinite(sprite.ay) && std::isfinite(sprite.bx) && std::isfinite(sprite.by);
}

inline std::pair<double,double> sprite_bounds(const Sprite& s) noexcept {
    if(s.cloud_style) {
        const auto& c=*s.cloud_style;
        const double spread=c.circles==1?0:c.density/100;
        const double hx=1+spread*c.aspect/100,hy=1+spread;
        return {hx*std::abs(s.ax)+hy*std::abs(s.bx),hx*std::abs(s.ay)+hy*std::abs(s.by)};
    }
    const bool rectangle=s.particle->shape==1 || s.particle->shape==3;
    return {rectangle?std::abs(s.ax)+std::abs(s.bx):std::hypot(s.ax,s.bx),
        rectangle?std::abs(s.ay)+std::abs(s.by):std::hypot(s.ay,s.by)};
}

inline double sprite_coverage(const ParticleInstance& particle,double x,double y,double edge_scale,
    std::span<const CloudCircle> members = {}) noexcept {
    const double feather=particle.feather_percent/100;
    const auto circle=[&](double distance,double scale) {
        const double aa=std::clamp(.5+(1-distance)*scale,0.0,1.0);
        return aa*(feather>0?std::clamp((1-distance)/feather,0.0,1.0):1);
    };
    if(particle.shape==1 || particle.shape==3) return circle(std::max(std::abs(x),std::abs(y)),edge_scale);
    if(particle.shape==0) return circle(std::hypot(x,y),edge_scale);
    if(!members.empty()) {
        double remaining=1;
        for(const auto& member:members) {
            const double dx=x-member.x,dy=y-member.y;
            // Cheap rejection avoids a sqrt for the sparse high-Density clouds.
            const double margin=member.radius+.5/edge_scale;
            if(std::abs(dx)>margin || std::abs(dy)>margin)continue;
            remaining*=1-circle(std::hypot(dx,dy)/member.radius,edge_scale*member.radius);
            if(remaining<=0)break;
        }
        return 1-remaining;
    }
    // A deterministic five-circle cluster. All lobes remain inside the sprite
    // bounds, so ROI/work accounting and future GPU parity use the same extent.
    double remaining=1;
    for(const auto center:{std::pair{0.0,0.0},std::pair{-.35,-.2},std::pair{.35,-.2},std::pair{-.2,.35},std::pair{.2,.35}})
        remaining*=1-circle(std::hypot(x-center.first,y-center.second)/.6,edge_scale*.6);
    return 1-remaining;
}

}
