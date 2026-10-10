#pragma once
#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/AgeCurve.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace starfield::core {
class CompiledMotionCurveClock {
public:
    [[nodiscard]] Result<MotionCurveClockSample> clock(double age,double lifetime) const noexcept;
    [[nodiscard]] Result<double> integral_between(double start,double end,double lifetime) const noexcept;
    [[nodiscard]] std::size_t segment_count() const noexcept {return count_;}
private:
    CompiledMotionCurveClock()=default;
    struct Segment {double start{},width{},prefix{};std::array<double,4> coefficients{};};
    AgeCurve curve_{};
    std::array<Segment,kMaxAgeCurvePoints-1> segments_{};
    std::size_t count_{};
    [[nodiscard]] std::size_t segment_at(double fraction) const noexcept {
        std::size_t low=0,high=count_-1;
        while(low<high){const auto mid=(low+high+1)/2;if(segments_[mid].start<=fraction)low=mid;else high=mid-1;}
        return low;
    }
    friend Result<CompiledMotionCurveClock> compile_motion_curve_clock(const AgeCurve&) noexcept;
    friend class CompiledMotionCircle;
};
namespace motion_curve_clock_detail {
inline double primitive(const std::array<double,4>& c,double t) noexcept {
    return t*(c[0]+t*(c[1]/2+t*(c[2]/3+t*c[3]/4)));
}
inline bool valid_clock(double age,double lifetime) noexcept {
    return std::isfinite(age)&&std::isfinite(lifetime)&&age>=0&&lifetime>0&&lifetime<=1e6&&age<=lifetime;
}
}
inline Result<CompiledMotionCurveClock> compile_motion_curve_clock(const AgeCurve& curve) noexcept {
    using R=Result<CompiledMotionCurveClock>;
    if(static_cast<unsigned>(curve.interpolation)>3||(curve.count&&!valid_age_curve(curve,0,1000)))
        return R::failure(ErrorCode::invalid_request,"invalid Motion curve clock");
    CompiledMotionCurveClock out;out.curve_=curve;
    if(!curve.count){out.count_=1;out.segments_[0]={0,1,0,{1,0,0,0}};}
    else {
        out.count_=curve.count-1;double prefix=0;
        for(std::size_t i=0;i<out.count_;++i){const auto& left=curve.points[i];const auto& right=curve.points[i+1];
            auto& s=out.segments_[i];s.start=left.age;s.width=right.age-left.age;s.prefix=prefix;
            const auto a=left.value/100,b=right.value/100;s.coefficients={a,0,0,0};
            if(curve.interpolation==CurveInterpolation::bezier){
                const auto p=s.width*age_curve_bezier_slope(curve,i)/100,q=s.width*age_curve_bezier_slope(curve,i+1)/100;
                s.coefficients={a,p,3*(b-a)-2*p-q,2*(a-b)+p+q};
            }else if(curve.interpolation!=CurveInterpolation::hold)s.coefficients[1]=b-a;
            for(auto coefficient:s.coefficients)if(!std::isfinite(coefficient))
                return R::failure(ErrorCode::invalid_request,"Motion curve coefficients are not finite");
            prefix+=s.width*motion_curve_clock_detail::primitive(s.coefficients,1);
            if(!std::isfinite(prefix))return R::failure(ErrorCode::invalid_request,"Motion curve integral is not finite");
        }
    }
    return R::success(std::move(out));
}
inline Result<MotionCurveClockSample> CompiledMotionCurveClock::clock(double age,double lifetime) const noexcept {
    using R=Result<MotionCurveClockSample>;
    if(!motion_curve_clock_detail::valid_clock(age,lifetime))return R::failure(ErrorCode::invalid_request,"invalid Motion particle clock");
    const auto fraction=age/lifetime;const auto& s=segments_[segment_at(fraction)];
    const auto t=std::clamp((fraction-s.start)/s.width,0.,1.);
    return R::success({(s.prefix+s.width*motion_curve_clock_detail::primitive(s.coefficients,t))*lifetime,
        evaluate_age_curve(curve_,fraction,100,100)/100});
}
inline Result<double> CompiledMotionCurveClock::integral_between(double start,double end,double lifetime) const noexcept {
    using R=Result<double>;
    if(!motion_curve_clock_detail::valid_clock(start,lifetime)||!motion_curve_clock_detail::valid_clock(end,lifetime)||start>end)
        return R::failure(ErrorCode::invalid_request,"invalid delayed Motion clock interval");
    if(start==end)return R::success(0);
    if(start==0){auto value=clock(end,lifetime);return value.has_value()?R::success(value.value().integral_seconds):R::failure(value.error());}
    // Integrate each crossed segment locally. Subtracting two large accumulated
    // clocks would lose a short travel interval near the end of a long lifetime.
    double integral=0;
    for(auto i=segment_at(start/lifetime);i<count_;++i){const auto& s=segments_[i];
        const auto segment_start=s.start*lifetime;
        const auto segment_end=i+1<count_?segments_[i+1].start*lifetime:lifetime;
        if(segment_start>=end)break;
        const auto first=std::max(start,segment_start),last=std::min(end,segment_end);if(last<=first)continue;
        const auto seconds=last-first,t=std::clamp((first-segment_start)/(s.width*lifetime),0.,1.),h=seconds/(s.width*lifetime);
        const auto& c=s.coefficients;
        integral+=seconds*(c[0]+t*(c[1]+t*(c[2]+t*c[3]))+
            h*(.5*(c[1]+t*(2*c[2]+3*t*c[3]))+h*((c[2]+3*t*c[3])/3+h*c[3]/4)));
    }
    return R::success(integral);
}
} // namespace starfield::core
