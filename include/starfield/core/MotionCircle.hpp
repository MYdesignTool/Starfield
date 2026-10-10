#pragma once
#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/AgeCurve.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <array>

namespace starfield::core {
struct MotionCircleClock { double integral_seconds{}, weight{}; };
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
    struct Segment { double start{}, width{}, prefix{}; std::array<double,4> coefficients{}; };
    MotionCircleSettings settings_{};
    Vec3 unit_axis_{};
    std::array<Segment,kMaxAgeCurvePoints-1> segments_{};
    std::size_t count_{};
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
inline double primitive(const std::array<double,4>& c,double t) noexcept {
    return t*(c[0]+t*(c[1]/2+t*(c[2]/3+t*c[3]/4)));
}
}
inline Result<CompiledMotionCircle> compile_motion_circle(const MotionCircleSettings& settings) noexcept {
    using R=Result<CompiledMotionCircle>;
    if(!motion_circle_detail::bounded(settings.origin)||!motion_circle_detail::bounded(settings.axis)||
        !std::isfinite(settings.radians_per_second)||std::abs(settings.radians_per_second)>1e6||
        !std::isfinite(settings.speed_random_percent)||settings.speed_random_percent<0||settings.speed_random_percent>100||
        static_cast<unsigned>(settings.over_life.interpolation)>3||
        (settings.over_life.count && !valid_age_curve(settings.over_life,0,1000)))
        return R::failure(ErrorCode::invalid_request,"invalid Motion Circle settings");
    const auto largest=std::max({std::abs(settings.axis.x),std::abs(settings.axis.y),std::abs(settings.axis.z)});
    if(largest==0)return R::failure(ErrorCode::invalid_request,"Motion Circle requires a nonzero axis");
    CompiledMotionCircle out;out.settings_=settings;
    auto axis=Vec3{settings.axis.x/largest,settings.axis.y/largest,settings.axis.z/largest};
    const auto length=std::hypot(axis.x,axis.y,axis.z);out.unit_axis_={axis.x/length,axis.y/length,axis.z/length};
    const auto& curve=settings.over_life;
    if(!curve.count){out.count_=1;out.segments_[0]={0,1,0,{1,0,0,0}};}
    else {
        out.count_=curve.count-1;double prefix=0;
        for(std::size_t i=0;i<out.count_;++i) {
            const auto& left=curve.points[i];const auto& right=curve.points[i+1];
            auto& s=out.segments_[i];s.start=left.age;s.width=right.age-left.age;s.prefix=prefix;
            const auto a=left.value/100,b=right.value/100;s.coefficients={a,0,0,0};
            if(curve.interpolation==CurveInterpolation::bezier) {
                const auto p=s.width*age_curve_bezier_slope(curve,i)/100,q=s.width*age_curve_bezier_slope(curve,i+1)/100;
                s.coefficients={a,p,3*(b-a)-2*p-q,2*(a-b)+p+q};
            } else if(curve.interpolation!=CurveInterpolation::hold)s.coefficients[1]=b-a;
            for(auto coefficient:s.coefficients)if(!std::isfinite(coefficient))
                return R::failure(ErrorCode::invalid_request,"Motion curve coefficients are not finite");
            prefix+=s.width*motion_circle_detail::primitive(s.coefficients,1);
            if(!std::isfinite(prefix))return R::failure(ErrorCode::invalid_request,"Motion curve integral is not finite");
        }
    }
    return R::success(std::move(out));
}
inline Result<MotionCircleClock> CompiledMotionCircle::clock(double age,double lifetime) const noexcept {
    using R=Result<MotionCircleClock>;
    if(!std::isfinite(age)||!std::isfinite(lifetime)||age<0||lifetime<=0||lifetime>1e6||age>lifetime)
        return R::failure(ErrorCode::invalid_request,"invalid Motion Circle particle clock");
    const auto fraction=age/lifetime;
    std::size_t low=0,high=count_-1;
    while(low<high){const auto mid=(low+high+1)/2;if(segments_[mid].start<=fraction)low=mid;else high=mid-1;}
    const auto& s=segments_[low];const auto t=std::clamp((fraction-s.start)/s.width,0.,1.);
    const auto integral=(s.prefix+s.width*motion_circle_detail::primitive(s.coefficients,t))*lifetime;
    // Preserve Hold's endpoint convention, and the shared Bezier value clamp.
    const auto weight=evaluate_age_curve(settings_.over_life,fraction,100,100)/100;
    return R::success({integral,weight});
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
