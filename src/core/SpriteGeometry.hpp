#pragma once
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Render.hpp"
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
inline bool project_sprite(const ParticleInstance& particle, const RenderRequest& request, const PixelGrid& grid, Sprite& sprite) noexcept {
    sprite.particle=&particle;
    const double rx=particle.size_pixels*.5;
    const double ry=particle.shape==0?rx:particle.size_y_pixels*.5;
    if(!(rx>0) || !(ry>0) || !(particle.opacity>0)) return false;
    const bool billboard=particle.shape!=1 || particle.limit_to_2d;
    Vec3 a{rx,0,0},b{0,ry,0};
    Vec3 angles=particle.rotation_degrees;
    if(billboard) angles.x=angles.y=0;
    else if(particle.up_axis==0) {a={0,0,rx};b={0,ry,0};}
    else if(particle.up_axis==1) {a={rx,0,0};b={0,0,ry};}
    a=rotate_axis(a,angles);b=rotate_axis(b,angles);
    if(!request.camera.enabled) {
        sprite.x=(.5+particle.position.x/grid.aspect)*grid.frame_width;
        sprite.y=(.5-particle.position.y)*grid.frame_height;
        sprite.ax=a.x*grid.scale_x;sprite.ay=a.y*grid.scale_y;
        sprite.bx=b.x*grid.scale_x;sprite.by=b.y*grid.scale_y;
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
        const auto project_axis=[&](Vec3 axis,double& dx,double& dy) {
            double axis_view[3]{axis.x,-axis.y,0};
            if(!billboard) {
                const double local_axis[3]{axis.x/frame.pixel_aspect_ratio,-axis.y,axis.z};
                for(int col=0;col<3;++col) {
                    axis_view[col]=0;
                    for(int row=0;row<3;++row) axis_view[col]+=local_axis[row]*camera.layer_to_view[row*4+col];
                }
            }
            const double du=camera.focal_x*(axis_view[0]*view[2]-view[0]*axis_view[2])/(view[2]*view[2]);
            const double dv=camera.focal_y*(axis_view[1]*view[2]-view[1]*axis_view[2])/(view[2]*view[2]);
            dx=((m[0]-x*m[6])*du+(m[1]-x*m[7])*dv)/w*grid.scale_x;
            dy=((m[3]-y*m[6])*du+(m[4]-y*m[7])*dv)/w*grid.scale_y;
        };
        project_axis(a,sprite.ax,sprite.ay);project_axis(b,sprite.bx,sprite.by);
    }
    const double anchor_x=1-particle.anchor_x_percent/50,anchor_y=particle.anchor_y_percent/50-1;
    sprite.x+=sprite.ax*anchor_x+sprite.bx*anchor_y;
    sprite.y+=sprite.ay*anchor_x+sprite.by*anchor_y;
    return std::isfinite(sprite.x) && std::isfinite(sprite.y) && std::isfinite(sprite.ax) &&
        std::isfinite(sprite.ay) && std::isfinite(sprite.bx) && std::isfinite(sprite.by);
}

inline double sprite_coverage(const ParticleInstance& particle,double x,double y,double edge_scale) noexcept {
    const double feather=particle.feather_percent/100;
    const auto circle=[&](double distance,double scale) {
        const double aa=std::clamp(.5+(1-distance)*scale,0.0,1.0);
        return aa*(feather>0?std::clamp((1-distance)/feather,0.0,1.0):1);
    };
    if(particle.shape==1) return circle(std::max(std::abs(x),std::abs(y)),edge_scale);
    if(particle.shape==0) return circle(std::hypot(x,y),edge_scale);
    // A deterministic five-circle cluster. All lobes remain inside the sprite
    // bounds, so ROI/work accounting and future GPU parity use the same extent.
    double remaining=1;
    for(const auto center:{std::pair{0.0,0.0},std::pair{-.35,-.2},std::pair{.35,-.2},std::pair{-.2,.35},std::pair{.2,.35}})
        remaining*=1-circle(std::hypot(x-center.first,y-center.second)/.6,edge_scale*.6);
    return 1-remaining;
}

}
