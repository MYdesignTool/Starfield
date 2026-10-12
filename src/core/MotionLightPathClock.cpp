#include "starfield/core/MotionLightPathClock.hpp"

#include <cmath>

namespace starfield::core {
namespace {
constexpr double kMaxSourceSeconds=1e9;
constexpr double kMaxDurationSeconds=1e6;
constexpr double kMaxSpeedPercent=1e6;
}

Result<CompiledMotionLightPathClock> compile_motion_light_path_clock(
    RationalTime first,RationalTime last,const MotionLightPathClockSettings& settings) noexcept {
    using R=Result<CompiledMotionLightPathClock>;
    const auto normalized_first=make_rational(first.value,first.scale);
    const auto normalized_last=make_rational(last.value,last.scale);
    if(!normalized_first||!normalized_last||compare(*normalized_first,*normalized_last)>=0||
       !std::isfinite(settings.speed_percent)||settings.speed_percent<0||settings.speed_percent>kMaxSpeedPercent||
       !std::isfinite(settings.speed_random_percent)||settings.speed_random_percent<0||settings.speed_random_percent>100||
       static_cast<unsigned>(settings.end_behavior)>static_cast<unsigned>(MotionPathEndBehavior::continue_direction))
        return R::failure(ErrorCode::invalid_request,"invalid Light Path source clock settings");
    const auto start=to_seconds(*normalized_first),end=to_seconds(*normalized_last),duration=end-start;
    if(!std::isfinite(start)||!std::isfinite(end)||!std::isfinite(duration)||
       std::abs(start)>kMaxSourceSeconds||std::abs(end)>kMaxSourceSeconds||
       duration<=0||duration>kMaxDurationSeconds)
        return R::failure(ErrorCode::invalid_request,"Light Path source time range is outside its bounds");
    auto curve=compile_motion_curve_clock(settings.over_life);
    if(!curve.has_value())return R::failure(curve.error());
    return R::success(CompiledMotionLightPathClock{start,end,settings,curve.take_value()});
}

Result<MotionLightPathClockSample> CompiledMotionLightPathClock::sample(
    double age,double lifetime,double delay,double random_sample) const noexcept {
    using R=Result<MotionLightPathClockSample>;
    if(!std::isfinite(delay)||delay<0||delay>kMaxDurationSeconds||
       !std::isfinite(random_sample)||random_sample<0||random_sample>1)
        return R::failure(ErrorCode::invalid_request,"invalid Light Path delay or random sample");
    auto current=over_life_.clock(age,lifetime);
    if(!current.has_value())return R::failure(current.error());
    if(age<delay||delay>=lifetime)
        return R::success({first_seconds_,0,0,0,true,false});

    auto integral=over_life_.integral_between(delay,age,lifetime);
    if(!integral.has_value())return R::failure(integral.error());
    const auto speed_factor=(settings_.speed_percent/100.)*
        (1-settings_.speed_random_percent/100.*random_sample);
    const auto elapsed=integral.value()*speed_factor;
    const auto rate=current.value().weight*speed_factor;
    if(!std::isfinite(elapsed)||elapsed<0||!std::isfinite(rate)||rate<0)
        return R::failure(ErrorCode::invalid_request,"Light Path source clock overflow");
    if(elapsed<duration_seconds_)
        return R::success({first_seconds_+elapsed,rate,0,0,false,false});

    if(settings_.end_behavior==MotionPathEndBehavior::stop)
        return R::success({last_seconds_,0,0,0,false,true});
    return R::success({last_seconds_,0,elapsed-duration_seconds_,rate,false,true});
}

} // namespace starfield::core
