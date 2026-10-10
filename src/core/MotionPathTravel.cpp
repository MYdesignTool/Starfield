#include "starfield/core/MotionPathTravel.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
#include "starfield/core/Render.hpp"
#include <cmath>
#include <new>

namespace starfield::core {
namespace {
bool bounded(Vec3 v) noexcept {return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
    std::abs(v.x)<=kMaxMotionCoordinate&&std::abs(v.y)<=kMaxMotionCoordinate&&std::abs(v.z)<=kMaxMotionCoordinate;}
}
Result<CompiledMotionPathTravel> compile_motion_path_travel(std::span<const Vec3> points,
    const MotionPathTravelSettings& settings,const Cancellation& cancellation) noexcept {
    using R=Result<CompiledMotionPathTravel>;
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion path travel compilation cancelled");
    if(!std::isfinite(settings.units_per_second)||std::abs(settings.units_per_second)>1e6||
        !std::isfinite(settings.delay_seconds)||settings.delay_seconds<0||settings.delay_seconds>1e6||
        !std::isfinite(settings.speed_random_percent)||settings.speed_random_percent<0||settings.speed_random_percent>100||
        !bounded(settings.forward)||(settings.orient_to_path&&settings.forward.x==0&&settings.forward.y==0&&settings.forward.z==0))
        return R::failure(ErrorCode::invalid_request,"invalid Motion path travel settings");
    auto clock=compile_motion_curve_clock(settings.over_life);if(!clock.has_value())return R::failure(clock.error());
    auto path=compile_motion_path(points,cancellation);if(!path.has_value())return R::failure(path.error());
    auto start=path.value().position_at_distance(0);if(!start.has_value())return R::failure(start.error());
    if(cancellation.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion path travel compilation cancelled");
    return R::success(CompiledMotionPathTravel{path.take_value(),clock.take_value(),settings,start.value()});
}
Result<MotionPathTravelSample> CompiledMotionPathTravel::travel(double age,double lifetime,double random_sample) const noexcept {
    using R=Result<MotionPathTravelSample>;
    auto current=clock_.clock(age,lifetime);if(!current.has_value())return R::failure(current.error());
    if(!std::isfinite(random_sample)||random_sample<0||random_sample>1)
        return R::failure(ErrorCode::invalid_request,"invalid Motion path speed random sample");
    if(age<settings_.delay_seconds||settings_.delay_seconds>=lifetime)return travel_at_distance(0,0);
    auto elapsed=clock_.integral_between(settings_.delay_seconds,age,lifetime);if(!elapsed.has_value())return R::failure(elapsed.error());
    const auto speed=settings_.units_per_second*(1-settings_.speed_random_percent/100*random_sample);
    return travel_at_distance(elapsed.value()*speed,current.value().weight*speed);
}
Result<MotionPathTravelSample> CompiledMotionPathTravel::travel_at_distance(double distance,double rate) const noexcept {
    using R=Result<MotionPathTravelSample>;
    if(!std::isfinite(distance)||std::abs(distance)>1e12||!std::isfinite(rate)||std::abs(rate)>1e7)
        return R::failure(ErrorCode::invalid_request,"invalid Motion path distance/rate");
    auto position=path_.position_at_distance(distance);if(!position.has_value())return R::failure(position.error());
    auto tangent=path_.tangent_at_distance(distance);if(!tangent.has_value())return R::failure(tangent.error());
    // Right-hand derivative of the current clamped numeric query, not a claim
    // about reference loop/extrapolation behavior at the end of a Light Path.
    const auto length=path_.length();const bool moving=(distance>0&&distance<length)||
        (distance==0&&rate>0&&length>0)||(distance==length&&rate<0&&length>0);
    const auto speed=moving?rate:0;
    const auto direction=tangent.value();
    return R::success({{position.value().x-start_.x,position.value().y-start_.y,position.value().z-start_.z},
        {direction.x*speed,direction.y*speed,direction.z*speed},
        rate<0?Vec3{-direction.x,-direction.y,-direction.z}:direction,distance,speed});
}
Result<bool> CompiledMotionPathTravel::apply_position_sample(ParticleInstance& particle,const MotionPathTravelSample& sample) const noexcept {
    using R=Result<bool>;
    if(!bounded(particle.position)||!bounded(particle.velocity)||!valid_particle_motion_pose(particle.motion_pose))
        return R::failure(ErrorCode::invalid_request,"invalid Motion path particle state");
    auto next=particle;
    // Keep exact zero additions and the existing orientation at zero motion.
    if(sample.displacement.x!=0||sample.displacement.y!=0||sample.displacement.z!=0){next.position={particle.position.x+sample.displacement.x,
        particle.position.y+sample.displacement.y,particle.position.z+sample.displacement.z};}
    if(sample.units_per_second!=0){next.velocity={particle.velocity.x+sample.velocity.x,
        particle.velocity.y+sample.velocity.y,particle.velocity.z+sample.velocity.z};}
    if(!bounded(next.position)||!bounded(next.velocity))return R::failure(ErrorCode::invalid_request,"Motion path state exceeds bounds");
    particle=next;return R::success(true);
}
Result<bool> CompiledMotionPathTravel::orient_sample(ParticleInstance& particle,const MotionPathTravelSample& sample,
    std::span<const ParticleSpriteBasis> bases) const noexcept {
    using R=Result<bool>;auto next=particle;
    if(!valid_particle_motion_pose(next.motion_pose))return R::failure(ErrorCode::invalid_request,"invalid Motion path pose");
    if(settings_.orient_to_path&&(sample.distance!=0||sample.units_per_second!=0)){
        auto forward=particle_motion_forward(next,settings_.forward,bases);if(!forward.has_value())return R::failure(forward.error());
        auto f=forward.value(),t=sample.tangent;if(next.limit_to_2d){f.z=0;t.z=0;}
        auto pose=turn_motion_pose(next.motion_pose,f,t,1);if(!pose.has_value())return R::failure(pose.error());next.motion_pose=pose.take_value();
    }
    particle=next;return R::success(true);
}
Result<bool> CompiledMotionPathTravel::apply_sample(ParticleInstance& particle,const MotionPathTravelSample& sample,
    std::span<const ParticleSpriteBasis> bases) const noexcept {
    auto next=particle;auto moved=apply_position_sample(next,sample);if(!moved.has_value())return moved;
    auto turned=orient_sample(next,sample,bases);if(!turned.has_value())return turned;
    particle=next;return Result<bool>::success(true);
}
Result<bool> CompiledMotionPathTravel::apply(ParticleInstance& particle,double random_sample,std::span<const ParticleSpriteBasis> bases) const noexcept {
    auto sample=travel(particle.age_seconds,particle.lifetime_seconds,random_sample);
    return sample.has_value()?apply_sample(particle,sample.value(),bases):Result<bool>::failure(sample.error());
}
Result<bool> CompiledMotionPathTravel::apply_at_distance(ParticleInstance& particle,double distance,double rate,std::span<const ParticleSpriteBasis> bases) const noexcept {
    auto sample=travel_at_distance(distance,rate);return sample.has_value()?apply_sample(particle,sample.value(),bases):Result<bool>::failure(sample.error());
}
Result<bool> CompiledMotionPathTravel::apply_position_at_distance(ParticleInstance& particle,double distance,double rate) const noexcept {
    auto sample=travel_at_distance(distance,rate);return sample.has_value()?apply_position_sample(particle,sample.value()):Result<bool>::failure(sample.error());
}
Result<bool> CompiledMotionPathTravel::orient_at_distance(ParticleInstance& particle,double distance,double rate,std::span<const ParticleSpriteBasis> bases) const noexcept {
    auto sample=travel_at_distance(distance,rate);return sample.has_value()?orient_sample(particle,sample.value(),bases):Result<bool>::failure(sample.error());
}
} // namespace starfield::core
