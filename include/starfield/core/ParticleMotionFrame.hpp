#pragma once
#include "starfield/core/ParticleMotionPose.hpp"
#include "starfield/core/ParticleTransform.hpp"
#include <cmath>

namespace starfield::core {
// Downstream affine transform of Q*A*shared-basis. No decomposition, allocation
// or per-particle shared-table slot; reflection and collapsed axes remain exact.
inline Result<bool> apply_ordered_particle_transform(ParticleInstance& p,const CompiledParticleTransform& transform)noexcept {
    using R=Result<bool>;
    const auto bounded=[](Vec3 v){return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
        std::abs(v.x)<=1e9&&std::abs(v.y)<=1e9&&std::abs(v.z)<=1e9;};
    if(!bounded(p.position)||!bounded(p.velocity)||!valid_particle_motion_pose(p.motion_pose)||
        !valid_particle_motion_affine(p.motion_affine)||!std::isfinite(p.size_pixels)||p.size_pixels<0||
        !std::isfinite(p.size_y_pixels)||p.size_y_pixels<0||!std::isfinite(p.opacity)||p.opacity<0||p.opacity>1)
        return R::failure(ErrorCode::invalid_request,"invalid ordered Transform particle");
    auto next=p;next.position=transform.position(p.position);next.velocity=transform.velocity(p.velocity);
    next.size_pixels*=transform.particle_scale();next.size_y_pixels*=transform.particle_scale();next.opacity*=transform.particle_opacity();
    if(!bounded(next.position)||!bounded(next.velocity)||!std::isfinite(next.size_pixels)||!std::isfinite(next.size_y_pixels)||
        !std::isfinite(next.opacity)||next.opacity<0||next.opacity>1)
        return R::failure(ErrorCode::work_limit_exceeded,"ordered Transform particle exceeds bounds");
    const auto& linear=transform.particle_basis();
    if(linear!=kIdentityMotionAffine) {
        ParticleMotionAffine affine{};
        for(unsigned column=0;column<3;++column){
            Vec3 axis{p.motion_affine[column],p.motion_affine[3+column],p.motion_affine[6+column]};
            axis=motion_affine_axis(linear,motion_pose_axis(p.motion_pose,axis));
            affine[column]=axis.x;affine[3+column]=axis.y;affine[6+column]=axis.z;
        }
        if(!valid_particle_motion_affine(affine))return R::failure(ErrorCode::work_limit_exceeded,"ordered sprite affine exceeds bounds");
        next.motion_affine=affine;next.motion_pose=kIdentityMotionPose;
    }
    p=next;return R::success(true);
}
} // namespace starfield::core
