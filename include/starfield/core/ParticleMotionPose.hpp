#pragma once
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/ParticleTransform.hpp"
#include "starfield/core/AgeCurve.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace starfield::core {
inline bool valid_particle_motion_affine(const ParticleMotionAffine& a)noexcept{return valid_particle_sprite_basis(a);}
inline Vec3 motion_affine_axis(const ParticleMotionAffine& a,Vec3 v)noexcept {
    return {a[0]*v.x+a[1]*v.y+a[2]*v.z,a[3]*v.x+a[4]*v.y+a[5]*v.z,a[6]*v.x+a[7]*v.y+a[8]*v.z};
}
inline bool valid_particle_motion_pose(const ParticleMotionPose& q) noexcept {
    double sum=0;for(auto v:q){if(!std::isfinite(v)||std::abs(v)>1.00000001)return false;sum+=v*v;}
    return std::abs(sum-1)<=1e-8;
}
inline Vec3 motion_pose_axis(const ParticleMotionPose& q,Vec3 v) noexcept {
    const auto w=q[0],x=q[1],y=q[2],z=q[3];
    return {(1-2*(y*y+z*z))*v.x+2*(x*y-w*z)*v.y+2*(x*z+w*y)*v.z,
        2*(x*y+w*z)*v.x+(1-2*(x*x+z*z))*v.y+2*(y*z-w*x)*v.z,
        2*(x*z-w*y)*v.x+2*(y*z+w*x)*v.y+(1-2*(x*x+y*y))*v.z};
}
namespace motion_pose_detail {
inline bool finite(Vec3 v)noexcept{return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z);}
inline Vec3 normalized(Vec3 v)noexcept {
    const auto m=std::max({std::abs(v.x),std::abs(v.y),std::abs(v.z)});if(m==0)return {};
    v={v.x/m,v.y/m,v.z/m};const auto n=std::hypot(v.x,v.y,v.z);return {v.x/n,v.y/n,v.z/n};
}
inline ParticleMotionPose normalized(ParticleMotionPose q)noexcept {
    double sum=0;for(auto v:q)sum+=v*v;const auto n=std::sqrt(sum);
    for(auto& v:q)v/=n;if(q[0]<0)for(auto& v:q)v=-v;return q;
}
inline ParticleMotionPose multiply(const ParticleMotionPose& a,const ParticleMotionPose& b)noexcept {
    return {a[0]*b[0]-a[1]*b[1]-a[2]*b[2]-a[3]*b[3],
        a[0]*b[1]+a[1]*b[0]+a[2]*b[3]-a[3]*b[2],
        a[0]*b[2]-a[1]*b[3]+a[2]*b[0]+a[3]*b[1],
        a[0]*b[3]+a[1]*b[2]-a[2]*b[1]+a[3]*b[0]};
}
inline Vec3 euler(Vec3 v,Vec3 angles)noexcept {
    constexpr auto radians=std::numbers::pi/180;
    for(unsigned i=0;i<3;++i){const auto a=(i==0?-angles.x:i==1?angles.y:angles.z)*radians,c=std::cos(a),s=std::sin(a);
        if(i==0)v={v.x,c*v.y-s*v.z,s*v.y+c*v.z};
        else if(i==1)v={c*v.x+s*v.z,v.y,-s*v.x+c*v.z};else v={c*v.x-s*v.y,s*v.x+c*v.y,v.z};}
    return v;
}
}
inline Result<Vec3> particle_motion_forward(const ParticleInstance& p,Vec3 forward,
    std::span<const ParticleSpriteBasis> bases)noexcept {
    using R=Result<Vec3>;
    if(!motion_pose_detail::finite(forward)||!motion_pose_detail::finite(p.rotation_degrees)||
        !valid_particle_motion_pose(p.motion_pose)||!valid_particle_motion_affine(p.motion_affine)||p.sprite_basis_index>bases.size())
        return R::failure(ErrorCode::invalid_request,"invalid Motion particle forward/basis");
    auto angles=p.rotation_degrees;if(p.limit_to_2d||p.shape==0||p.shape==2)angles.x=angles.y=0;
    auto value=motion_pose_detail::euler(forward,angles);
    if(p.sprite_basis_index){const auto& b=bases[p.sprite_basis_index-1];
        if(!valid_particle_sprite_basis(b))return R::failure(ErrorCode::invalid_request,"invalid Motion shared basis");
        value={b[0]*value.x+b[1]*value.y+b[2]*value.z,b[3]*value.x+b[4]*value.y+b[5]*value.z,b[6]*value.x+b[7]*value.y+b[8]*value.z};}
    if(p.motion_affine!=kIdentityMotionAffine)value=motion_affine_axis(p.motion_affine,value);
    value=motion_pose_axis(p.motion_pose,value);
    if(!motion_pose_detail::finite(value))return R::failure(ErrorCode::invalid_request,"Motion forward overflow");
    return R::success(value);
}
inline Result<ParticleMotionPose> turn_motion_pose(const ParticleMotionPose& existing,Vec3 forward,Vec3 direction,double weight)noexcept {
    using R=Result<ParticleMotionPose>;
    if(!valid_particle_motion_pose(existing)||!motion_pose_detail::finite(forward)||!motion_pose_detail::finite(direction)||
        !std::isfinite(weight)||weight<0||weight>1)return R::failure(ErrorCode::invalid_request,"invalid Motion orientation input");
    if(weight==0)return R::success(existing);
    auto f=motion_pose_detail::normalized(forward),d=motion_pose_detail::normalized(direction);
    if((f.x==0&&f.y==0&&f.z==0)||(d.x==0&&d.y==0&&d.z==0))return R::success(existing);
    Vec3 axis{f.y*d.z-f.z*d.y,f.z*d.x-f.x*d.z,f.x*d.y-f.y*d.x};
    const auto sine=std::hypot(axis.x,axis.y,axis.z),cosine=std::clamp(f.x*d.x+f.y*d.y+f.z*d.z,-1.,1.);
    if(sine<=1e-12){if(cosine>=0)return R::success(existing);
        const Vec3 a=std::abs(f.x)<=std::abs(f.y)&&std::abs(f.x)<=std::abs(f.z)?Vec3{1,0,0}:std::abs(f.y)<=std::abs(f.z)?Vec3{0,1,0}:Vec3{0,0,1};
        axis={f.y*a.z-f.z*a.y,f.z*a.x-f.x*a.z,f.x*a.y-f.y*a.x};}
    axis=motion_pose_detail::normalized(axis);const auto angle=std::atan2(sine,cosine)*weight*.5,s=std::sin(angle);
    const auto value=motion_pose_detail::normalized(motion_pose_detail::multiply({std::cos(angle),axis.x*s,axis.y*s,axis.z*s},existing));
    if(!valid_particle_motion_pose(value))return R::failure(ErrorCode::invalid_request,"invalid composed Motion orientation");
    return R::success(value);
}
inline Result<ParticleMotionPose> interpolate_motion_pose(ParticleMotionPose a,ParticleMotionPose b,double t)noexcept {
    using R=Result<ParticleMotionPose>;
    if(!valid_particle_motion_pose(a)||!valid_particle_motion_pose(b)||!std::isfinite(t)||t<0||t>1)
        return R::failure(ErrorCode::invalid_request,"invalid Motion pose interpolation");
    if(t==0||a==b)return R::success(a);if(t==1)return R::success(b);
    double dot=0;for(unsigned i=0;i<4;++i)dot+=a[i]*b[i];if(dot<0){for(auto& v:b)v=-v;dot=-dot;}
    dot=std::clamp(dot,0.,1.);ParticleMotionPose q;
    if(dot>.9995){for(unsigned i=0;i<4;++i)q[i]=std::lerp(a[i],b[i],t);}
    else{const auto angle=std::acos(dot),denom=std::sin(angle),u=std::sin((1-t)*angle)/denom,v=std::sin(t*angle)/denom;
        for(unsigned i=0;i<4;++i)q[i]=u*a[i]+v*b[i];}
    q=motion_pose_detail::normalized(q);if(!valid_particle_motion_pose(q))return R::failure(ErrorCode::invalid_request,"invalid interpolated Motion orientation");
    return R::success(q);
}
// Hot path: settings were validated once by the graph reader. Forward is in the
// canonical pre-Euler frame; the caller has already resolved its Up Axis policy.
inline Result<bool> apply_motion_look_at(ParticleInstance& p,const MotionLookAtSettings& settings,
    std::span<const ParticleSpriteBasis> bases)noexcept {
    using R=Result<bool>;
    if(!motion_pose_detail::finite(p.position)||!std::isfinite(p.age_seconds)||!std::isfinite(p.lifetime_seconds)||p.lifetime_seconds<=0)
        return R::failure(ErrorCode::invalid_request,"invalid Look At particle state");
    auto forward=particle_motion_forward(p,settings.forward,bases);if(!forward.has_value())return R::failure(forward.error());
    Vec3 direction{settings.goal.x-p.position.x,settings.goal.y-p.position.y,settings.goal.z-p.position.z};
    auto value=forward.value();if(p.limit_to_2d){value.z=0;direction.z=0;}
    const auto weight=std::clamp(evaluate_age_curve(settings.over_life,p.age_seconds/p.lifetime_seconds,100,100)/100,0.,1.);
    auto q=turn_motion_pose(p.motion_pose,value,direction,weight);if(!q.has_value())return R::failure(q.error());
    p.motion_pose=q.take_value();return R::success(true);
}
} // namespace starfield::core
