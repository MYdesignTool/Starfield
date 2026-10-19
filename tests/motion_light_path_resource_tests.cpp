#include "starfield/core/MotionLightPathResource.hpp"
#include "starfield/core/Render.hpp"
#include <cmath>
#include <cstdio>
#include <stdexcept>

using namespace starfield::core;
namespace {
unsigned checks{};
class Never final:public Cancellation {public:bool is_cancelled()const noexcept override{return false;}} never;
void check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}
void near(double actual,double expected,const char* message,double tolerance=1e-9){
    check(std::isfinite(actual)&&std::abs(actual-expected)<=tolerance,message);
}
template<class T>T take(Result<T> result){if(!result.has_value())throw std::runtime_error(result.error().detail);return result.take_value();}
MotionLightPathResource fixture(){
    MotionLightPathResource resource;
    MotionLightPathSourceTrack x;x.source_layer_id=101;x.maximum_probe_error=.001;
    x.samples={{{0,1},{0,0,0}},{{2,1},{20,0,0}}};
    MotionLightPathSourceTrack y;y.source_layer_id=202;y.maximum_probe_error=.002;
    y.samples={{{5,1},{0,0,0}},{{7,1},{0,40,0}}};
    resource.tracks={std::move(x),std::move(y)};resource.sample_count=4;return resource;
}
void resource_roundtrip_and_track_playback(){
    auto input=fixture();auto bytes=take(encode_motion_light_path_resource(input,never));
    check(bytes.size()==24+2*16+4*40,"SFPL1 resource has canonical exact length");
    auto decoded=take(decode_motion_light_path_resource(bytes,never));
    check(decoded.sample_count==4&&decoded.tracks.size()==2,"resource preserves source/sample counts");
    check(decoded.tracks[1].source_layer_id==202,"resource preserves source identity and order");
    check(decoded.tracks[1].samples[0].source_comp_time.value==5&&decoded.tracks[1].samples[0].source_comp_time.scale==1,
        "resource preserves rational CompTime");
    near(decoded.tracks[0].maximum_probe_error,.001,"resource preserves measured probe error");
    auto track=take(compile_motion_light_path_track(decoded.tracks[0],never));
    auto point=take(track.sample(1));near(point.position.x,10,"track interpolates source-space position");
    near(point.velocity_per_source_second.x,10,"track reports derivative per source second");
    point=take(track.sample(2,1,1));near(point.position.x,30,"track extends by terminal derivative");
    near(point.velocity_per_source_second.x,10,"continued track retains source derivative units");
    check(point.continued,"track marks terminal continuation");
}
void all_matching_sources_are_clocked_and_summed(){
    auto resource=fixture();MotionLightPathSettings settings;settings.max_delay_seconds=.5;
    auto path=take(compile_motion_light_path(resource,settings,never));
    check(path.source_count()==2,"compiled Light Path retains all matching sources");
    auto sample=take(path.sample(.49,10,0,never));
    near(sample.displacement.x,0,"fixed max delay holds first source");
    near(sample.displacement.y,0,"fixed max delay holds second source");
    check(sample.active_source_count==0,"delayed sources are inactive");

    sample=take(path.sample(.5,10,0,never));
    near(sample.displacement.x,0,"delay boundary starts from first source position");
    near(sample.velocity.x,10,"first source keeps its own derivative");
    near(sample.velocity.y,20,"second source keeps its independent derivative");
    sample=take(path.sample(1.5,10,0,never));
    near(sample.displacement.x,10,"first source uses its own clock range");
    near(sample.displacement.y,20,"second source uses its own clock range");
    near(sample.velocity.x,10,"combined motion sums first source velocity");
    near(sample.velocity.y,20,"combined motion sums second source velocity");
    near(sample.combined_direction.x,1/std::sqrt(5),"direction is normalized from combined velocity");
    near(sample.combined_direction.y,2/std::sqrt(5),"direction preserves velocity contribution ratios");

    settings.clock.end_behavior=MotionPathEndBehavior::stop;
    path=take(compile_motion_light_path(resource,settings,never));
    sample=take(path.sample(3,10,0,never));
    near(sample.displacement.x,20,"Stop holds each source endpoint");
    near(sample.displacement.y,40,"Stop uses each source's own endpoint");
    near(sample.velocity.x,0,"Stop removes first terminal velocity");
    near(sample.velocity.y,0,"Stop removes second terminal velocity");

    settings.clock.end_behavior=MotionPathEndBehavior::continue_direction;
    path=take(compile_motion_light_path(resource,settings,never));
    sample=take(path.sample(3.5,10,0,never));
    near(sample.displacement.x,30,"Continue extends first source along its terminal derivative");
    near(sample.displacement.y,60,"Continue extends second source independently");
    near(sample.velocity.x,10,"Continue reports first source velocity");
    near(sample.velocity.y,20,"Continue reports second source velocity");

    settings.clock.speed_random_percent=50;
    path=take(compile_motion_light_path(resource,settings,never));
    sample=take(path.sample(1.5,10,.5,never));
    near(sample.displacement.x,7.5,"stable Speed Random modifies each source clock");
    near(sample.displacement.y,15,"randomized source clocks remain independently ranged");
}
void empty_match_is_a_zero_motion_resource(){
    MotionLightPathResource empty;
    auto bytes=take(encode_motion_light_path_resource(empty,never));
    check(bytes.size()==kMotionLightPathResourceHeaderBytes,"empty source set has a canonical header");
    auto decoded=take(decode_motion_light_path_resource(bytes,never));
    check(decoded.tracks.empty()&&decoded.sample_count==0,"empty source set roundtrips");
    auto path=take(compile_motion_light_path(decoded,{},never));
    check(path.source_count()==0,"compiled empty path has no sources");
    auto sample=take(path.sample(2,5,.25,0,never));
    check(sample.source_count==0&&sample.active_source_count==0,"empty path reports no active sources");
    near(sample.displacement.x,0,"empty path contributes no displacement");
    near(sample.velocity.y,0,"empty path contributes no velocity");
    sample=take(path.sample_at_source_elapsed(2,1,MotionPathEndBehavior::continue_direction,never));
    near(sample.displacement.z,0,"empty continued path remains a no-op");
    bytes[16]=std::byte{1};
    check(!decode_motion_light_path_resource(bytes,never).has_value(),"empty track count cannot carry samples");
    bytes=take(encode_motion_light_path_resource(empty,never));bytes[12]=std::byte{1};
    check(!decode_motion_light_path_resource(bytes,never).has_value(),"nonempty track count cannot omit samples");
}
void malformed_resources_reject(){
    auto resource=fixture();auto bytes=take(encode_motion_light_path_resource(resource,never));
    for(std::size_t i=0;i<bytes.size();++i)
        check(!decode_motion_light_path_resource(std::span<const std::byte>(bytes).first(i),never).has_value(),
            "every truncated SFPL1 prefix rejects");
    bytes.push_back(std::byte{});check(!decode_motion_light_path_resource(bytes,never).has_value(),"trailing bytes reject");
    resource=fixture();resource.tracks[1].source_layer_id=101;
    check(!encode_motion_light_path_resource(resource,never).has_value(),"duplicate source IDs reject");
    resource=fixture();resource.tracks[0].samples[1].source_comp_time={-1,0};
    check(!encode_motion_light_path_resource(resource,never).has_value(),"invalid source time rejects");
    resource=fixture();resource.sample_count=3;
    check(!compile_motion_light_path(resource,{},never).has_value(),"aggregate sample count mismatch rejects");
    MotionLightPathSettings invalid;invalid.orient_to_path=true;invalid.forward={};
    check(!compile_motion_light_path(fixture(),invalid,never).has_value(),"orientation requires a nonzero forward axis");
}
}
int main()try{
    resource_roundtrip_and_track_playback();all_matching_sources_are_clocked_and_summed();
    empty_match_is_a_zero_motion_resource();malformed_resources_reject();
    std::printf("motion_light_path_resource_tests: %u checks passed; SFPL1, per-source timing and summed tracks only\n",checks);
    return 0;
}catch(const std::exception& error){
    std::fprintf(stderr,"motion_light_path_resource_tests after %u checks: %s\n",checks,error.what());return 1;
}
