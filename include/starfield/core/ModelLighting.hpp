#pragma once
#include "starfield/core/Settings.hpp"
#include <algorithm>
#include <cmath>
#include <span>

namespace starfield::core {
enum class ModelLightType : std::uint32_t { ambient,parallel,point,spot };
inline constexpr std::size_t kMaxModelLights=64;
struct ModelLight {
    ModelLightType type{ModelLightType::ambient};
    Vec3 color{1,1,1},position{},direction{0,0,1};
    double intensity{1},cone_degrees{90},cone_feather{.5};
    std::uint32_t falloff{0}; // none, smooth radius, inverse square
    double radius{500},falloff_distance{500};
};
struct ModelLighting {
    std::array<double,16> layer_to_world{kIdentityModelMatrix};
    Vec3 camera_position{};
    bool camera_enabled{false};
    std::vector<ModelLight> lights;
};
inline bool valid_model_material(const ModelMaterial& m) noexcept {
    const double v[]{m.color.x,m.color.y,m.color.z,m.roughness,m.metallic,m.opacity};
    for(double x:v)if(!std::isfinite(x)||x<0)return false;
    return m.color_source<=1 && m.color.x<=kMaxParticleColor && m.color.y<=kMaxParticleColor &&
        m.color.z<=kMaxParticleColor && m.roughness<=1 && m.metallic<=1 && m.opacity<=1;
}
inline bool valid_model_lighting(const ModelLighting& input) noexcept {
    if(input.lights.size()>kMaxModelLights)return false;
    for(double x:input.layer_to_world)if(!std::isfinite(x)||std::abs(x)>1e12)return false;
    const auto& m=input.layer_to_world;
    if(m[3]!=0||m[7]!=0||m[11]!=0||m[15]!=1)return false;
    for(double x:{input.camera_position.x,input.camera_position.y,input.camera_position.z})
        if(!std::isfinite(x)||std::abs(x)>1e12)return false;
    for(const auto& l:input.lights) {
        for(double x:{l.color.x,l.color.y,l.color.z,l.position.x,l.position.y,l.position.z,
            l.direction.x,l.direction.y,l.direction.z,l.intensity,l.cone_degrees,l.cone_feather,l.radius,l.falloff_distance})
            if(!std::isfinite(x)||std::abs(x)>1e12)return false;
        if(static_cast<unsigned>(l.type)>3 || l.color.x<0||l.color.y<0||l.color.z<0||l.intensity<0||
            l.cone_degrees<0||l.cone_degrees>360||l.cone_feather<0||l.cone_feather>1||l.falloff>2||l.radius<0||l.falloff_distance<0)return false;
    }
    return true;
}
namespace model_shading {
inline Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 scale(Vec3 a,double s){return {a.x*s,a.y*s,a.z*s};}
inline Vec3 mul(Vec3 a,Vec3 b){return {a.x*b.x,a.y*b.y,a.z*b.z};}
inline double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3 unit(Vec3 a){const double n=std::hypot(a.x,a.y,a.z);return n>1e-30?scale(a,1/n):Vec3{};}
inline Vec3 world(Vec3 a,const std::array<double,16>& m){return {a.x*m[0]+a.y*m[4]+a.z*m[8]+m[12],a.x*m[1]+a.y*m[5]+a.z*m[9]+m[13],a.x*m[2]+a.y*m[6]+a.z*m[10]+m[14]};}
// Cofactor/inverse transpose; determinant sign preserves authored outward normals.
inline Vec3 normal(Vec3 n,const std::array<double,16>& m){
    const Vec3 a{m[0],m[1],m[2]},b{m[4],m[5],m[6]},c{m[8],m[9],m[10]};
    const auto x=cross(b,c),y=cross(c,a),z=cross(a,b);const auto det=dot(a,x);
    if(std::abs(det)<1e-30)return {};
    return unit(scale(add(add(scale(x,n.x),scale(y,n.y)),scale(z,n.z)),det<0?-1:1));
}
inline Vec3 shade(const ModelMaterial& material,Vec3 inherited,Vec3 position,Vec3 normal,Vec3 view,
    std::span<const ModelLight> lights) noexcept {
    const auto base=material.color_source?material.color:inherited;
    if(!material.enabled||!material.ae_lights||lights.empty())return base;
    const auto n=unit(normal),v=unit(view);const double nv=std::max(1e-6,dot(n,v));
    constexpr double pi=3.14159265358979323846;
    const double a=std::max(.04,material.roughness*material.roughness),a2=a*a;
    const auto f0=add(scale(Vec3{.04,.04,.04},1-material.metallic),scale(base,material.metallic));
    Vec3 result{};
    for(const auto& light:lights) {
        const auto radiance=scale(light.color,light.intensity);
        if(light.type==ModelLightType::ambient){result=add(result,mul(base,radiance));continue;}
        auto delta=light.type==ModelLightType::parallel?scale(light.direction,-1):sub(light.position,position);
        const auto distance=std::hypot(delta.x,delta.y,delta.z);const auto l=unit(delta);
        const double nl=std::max(0.,dot(n,l));if(nl==0)continue;
        double attenuation=1;
        if(light.type!=ModelLightType::parallel && light.falloff) {
            if(light.falloff==1){const double t=std::clamp((distance-light.radius)/std::max(1e-6,light.falloff_distance),0.,1.);attenuation=1-t*t*(3-2*t);}
            else {const double ratio=std::max(distance,std::max(1.,light.radius))/std::max(1.,light.radius);attenuation=1/(ratio*ratio);}
        }
        if(light.type==ModelLightType::spot){
            const double outer=std::cos(light.cone_degrees*pi/360),inner=std::cos(light.cone_degrees*(1-light.cone_feather)*pi/360);
            const double d=dot(unit(light.direction),scale(l,-1));
            const double t=inner>outer?std::clamp((d-outer)/(inner-outer),0.,1.):(d>=outer?1.:0.);
            attenuation*=t*t*(3-2*t);
        }
        const auto h=unit(add(v,l));const double nh=std::max(0.,dot(n,h)),vh=std::clamp(dot(v,h),0.,1.);
        const double denominator=nh*nh*(a2-1)+1,D=a2/(pi*denominator*denominator);
        const auto g=[&](double c){return 2*c/(c+std::sqrt(a2+(1-a2)*c*c));};
        const auto f=add(f0,scale(sub(Vec3{1,1,1},f0),std::pow(1-vh,5)));
        const auto diffuse=scale(mul(sub(Vec3{1,1,1},f),base),(1-material.metallic)/pi);
        const auto specular=scale(f,D*g(nv)*g(nl)/(4*nv*std::max(nl,1e-6)));
        result=add(result,scale(mul(add(diffuse,specular),radiance),nl*attenuation));
    }
    return {std::clamp(result.x,0.,kMaxParticleColor),std::clamp(result.y,0.,kMaxParticleColor),std::clamp(result.z,0.,kMaxParticleColor)};
}
}
}
