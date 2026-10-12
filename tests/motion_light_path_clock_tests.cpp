#include "starfield/core/MotionLightPathClock.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace starfield::core;
namespace {
unsigned checks{};
void check(bool ok,const char* message){++checks;if(!ok)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message,double tolerance=1e-10){
    check(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,message);
}
template<class T>T take(Result<T> result){if(!result.has_value())throw std::runtime_error(result.error().detail);return result.take_value();}
void basic_time_mapping(){
    MotionLightPathClockSettings settings;settings.speed_random_percent=100;
    auto clock=take(compile_motion_light_path_clock({10,1},{12,1},settings));
    near(clock.first_keyframe_seconds(),10,"keeps this source's first key time");
    near(clock.last_keyframe_seconds(),12,"keeps this source's last key time");
    near(clock.duration_seconds(),2,"keeps source duration");
    auto sample=take(clock.sample(.25,10,0));
    near(sample.source_time_seconds,10.25,"Speed 100 maps particle seconds 1:1 to this light");
    near(sample.source_time_rate,1,"Speed 100 has unit source-time rate");
    check(!sample.delayed&&!sample.ended,"active path state");

    sample=take(clock.sample(2,10,0,.5));
    near(sample.source_time_seconds,11,"different speed random samples slow source time");
    near(sample.source_time_rate,.5,"reports instantaneous source-time rate");
}
void delay_and_end_modes(){
    MotionLightPathClockSettings settings;auto stop=take(compile_motion_light_path_clock({3,2},{7,2},settings));
    auto sample=take(stop.sample(.49,5,.5));
    near(sample.source_time_seconds,1.5,"delay holds at source start");check(sample.delayed,"delay is reported");
    sample=take(stop.sample(.5,5,.5));near(sample.source_time_seconds,1.5,"source starts at delay boundary");
    near(sample.source_time_rate,1,"source has right derivative at delay boundary");
    sample=take(stop.sample(3.5,5,.5));near(sample.source_time_seconds,3.5,"Stop holds last key after original duration");
    check(sample.ended,"Stop reports endpoint");near(sample.source_time_rate,0,"Stop velocity is zero after endpoint");

    settings.end_behavior=MotionPathEndBehavior::continue_direction;
    auto continued=take(compile_motion_light_path_clock({3,2},{7,2},settings));
    sample=take(continued.sample(3.5,5,.5));near(sample.source_time_seconds,3.5,"Continue clamps the AE query to the last key");
    near(sample.continuation_seconds,1,"Continue reports time beyond final key");
    near(sample.continuation_rate,1,"Continue keeps the end clock rate");
    sample=take(continued.sample(2,5,.5));near(sample.continuation_seconds,0,"Continue boundary has no overshoot");
}
void curve_clock_and_independent_ranges(){
    MotionLightPathClockSettings settings;settings.over_life.count=2;
    settings.over_life.points[0]={0,0};settings.over_life.points[1]={1,100};
    auto first=take(compile_motion_light_path_clock({0,1},{2,1},settings));
    auto second=take(compile_motion_light_path_clock({5,1},{9,1},settings));
    auto a=take(first.sample(1,2,0));auto b=take(second.sample(1,2,0));
    near(a.source_time_seconds,.25,"over-life integrates the first source clock");
    near(b.source_time_seconds,5.25,"over-life integrates the second source independently");
    near(a.source_time_rate,.5,"over-life supplies current clock weight");
    near(b.source_time_rate,.5,"different source retains same normalized clock");
}
void invalid_inputs(){
    MotionLightPathClockSettings settings;
    check(!compile_motion_light_path_clock({1,1},{1,1},settings).has_value(),"zero duration rejects");
    check(!compile_motion_light_path_clock({2,1},{1,1},settings).has_value(),"reversed source range rejects");
    check(!compile_motion_light_path_clock({0,0},{1,1},settings).has_value(),"invalid rational time rejects");
    settings.speed_percent=-1;check(!compile_motion_light_path_clock({0,1},{1,1},settings).has_value(),"negative percentage rejects");
    settings.speed_percent=100;settings.speed_random_percent=101;
    check(!compile_motion_light_path_clock({0,1},{1,1},settings).has_value(),"invalid speed random rejects");
    settings.speed_random_percent=0;settings.end_behavior=static_cast<MotionPathEndBehavior>(2);
    check(!compile_motion_light_path_clock({0,1},{1,1},settings).has_value(),"only the two requested end choices are accepted");
    settings.end_behavior=MotionPathEndBehavior::stop;auto clock=take(compile_motion_light_path_clock({0,1},{1,1},settings));
    check(!clock.sample(.5,1,-.01).has_value(),"negative delay rejects");
    check(!clock.sample(.5,1,0,1.01).has_value(),"out-of-range stable random sample rejects");
    check(!clock.sample(-.1,1,0).has_value(),"negative particle age rejects");
}
}
int main()try{
    basic_time_mapping();delay_and_end_modes();curve_clock_and_independent_ranges();invalid_inputs();
    std::printf("motion_light_path_clock_tests: %u checks passed; exact key-time clock math only, interpolation/resource integration pending\n",checks);
    return 0;
}catch(const std::exception& error){
    std::fprintf(stderr,"motion_light_path_clock_tests after %u checks: %s\n",checks,error.what());return 1;
}
