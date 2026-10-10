#pragma once
#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/MotionCurveClock.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>

namespace starfield::core {
using MotionCircleClock=MotionCurveClockSample;
struct MotionCircleState { Vec3 position{}, velocity{}; };
class CompiledMotionCircle {
public:
    CompiledMotionCircle(const CompiledMotionCircle&)=default;
    CompiledMotionCircle(CompiledMotionCircle&&)=default;
    CompiledMotionCircle& operator=(const CompiledMotionCircle&)=default;
    CompiledMotionCircle& operator=(CompiledMotionCircle&&)=default;
    [[nodiscard]] Result<MotionCircleClock> clock(double age_seconds,double lifetime_seconds) const noexcept;
    [[nodiscard]] Result<MotionCircleState> apply(Vec3 position,Vec3 velocity,double angle_radians,
        double angular_velocity) const noexcept;
    [[nodiscard]] const MotionCircleSettings& settings() const noexcept { return settings_; }
private:
    CompiledMotionCircle()=default;
    MotionCircleSettings settings_{};
    Vec3 unit_axis_{};
    CompiledMotionCurveClock curve_clock_{};
    friend Result<CompiledMotionCircle> compile_motion_circle(const MotionCircleSettings&) noexcept;
};
[[nodiscard]] Result<CompiledMotionCircle> compile_motion_circle(const MotionCircleSettings&) noexcept;
} // namespace starfield::core

// Fixed-size numeric evaluator. Queries allocate no storage; immutable temporal
// leases share compiled coefficients rather than copying them per particle.

namespace starfield::core {
namespace motion_circle_detail {
inline bool bounded(Vec3 v) noexcept { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&
    std::abs(v.x)<=kMaxMotionCoordinate&&std::abs(v.y)<=kMaxMotionCoordinate&&std::abs(v.z)<=kMaxMotionCoordinate; }
}
inline Result<CompiledMotionCircle> compile_motion_circle(const MotionCircleSettings& settings) noexcept {
    using R=Result<CompiledMotionCircle>;
    if(!motion_circle_detail::bounded(settings.origin)||!motion_circle_detail::bounded(settings.axis)||
        !std::isfinite(settings.radians_per_second)||std::abs(settings.radians_per_second)>1e6||
        !std::isfinite(settings.speed_random_percent)||settings.speed_random_percent<0||settings.speed_random_percent>100)
        return R::failure(ErrorCode::invalid_request,"invalid Motion Circle settings");
    const auto largest=std::max({std::abs(settings.axis.x),std::abs(settings.axis.y),std::abs(settings.axis.z)});
    if(largest==0)return R::failure(ErrorCode::invalid_request,"Motion Circle requires a nonzero axis");
    CompiledMotionCircle out;out.settings_=settings;
    auto axis=Vec3{settings.axis.x/largest,settings.axis.y/largest,settings.axis.z/largest};
    const auto length=std::hypot(axis.x,axis.y,axis.z);out.unit_axis_={axis.x/length,axis.y/length,axis.z/length};
    auto curve=compile_motion_curve_clock(settings.over_life);if(!curve.has_value())return R::failure(curve.error());
    out.curve_clock_=curve.take_value();
    return R::success(std::move(out));
}
inline Result<MotionCircleClock> CompiledMotionCircle::clock(double age,double lifetime) const noexcept {
    return curve_clock_.clock(age,lifetime);
}
inline Result<MotionCircleState> CompiledMotionCircle::apply(Vec3 position,Vec3 velocity,double angle,double omega) const noexcept {
    using R=Result<MotionCircleState>;
    if(!motion_circle_detail::bounded(position)||!motion_circle_detail::bounded(velocity)||!std::isfinite(angle)||std::abs(angle)>1e12||
        !std::isfinite(omega)||std::abs(omega)>1e7)
        return R::failure(ErrorCode::invalid_request,"invalid Motion Circle state");
    if(angle==0&&omega==0)return R::success({position,velocity});
    const auto radians=std::remainder(angle,2*std::numbers::pi),cs=std::cos(radians),sn=std::sin(radians),d=1-cs;
    const auto a=unit_axis_;
    const auto rotate=[&](Vec3 v) {
        if(radians==0)return v;
        return Vec3{(cs+a.x*a.x*d)*v.x+(a.x*a.y*d-a.z*sn)*v.y+(a.x*a.z*d+a.y*sn)*v.z,
            (a.y*a.x*d+a.z*sn)*v.x+(cs+a.y*a.y*d)*v.y+(a.y*a.z*d-a.x*sn)*v.z,
            (a.z*a.x*d-a.y*sn)*v.x+(a.z*a.y*d+a.x*sn)*v.y+(cs+a.z*a.z*d)*v.z};
    };
    const auto radius=rotate({position.x-settings_.origin.x,position.y-settings_.origin.y,position.z-settings_.origin.z});
    const auto p=radians==0?position:Vec3{settings_.origin.x+radius.x,settings_.origin.y+radius.y,settings_.origin.z+radius.z};
    if(!motion_circle_detail::bounded(p))return R::failure(ErrorCode::invalid_request,"Motion Circle position exceeds bound");
    const auto r=Vec3{p.x-settings_.origin.x,p.y-settings_.origin.y,p.z-settings_.origin.z};
    auto result=rotate(velocity);
    result.x+=(a.y*r.z-a.z*r.y)*omega;result.y+=(a.z*r.x-a.x*r.z)*omega;result.z+=(a.x*r.y-a.y*r.x)*omega;
    if(!motion_circle_detail::bounded(result))return R::failure(ErrorCode::invalid_request,"Motion Circle velocity exceeds bound");
    return R::success({p,result});
}
} // namespace starfield::core
