#include "MotionLightTiming.hpp"
#include "MotionLightSources.hpp"
#include "SPBasic.h"
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <array>
#include <algorithm>
#include <cmath>
#include <new>
#include <utility>

namespace starfield::adapter {
namespace {
struct Suites {
    SPBasicSuite* basic{};
    std::array<const char*,6> names{};
    std::array<A_long,6> versions{};
    std::size_t count{};
    template<class T>A_Err get(const char* name,A_long version,const T*& suite) noexcept {
        const auto error=basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite));
        if(error)return error;
        names[count]=name;versions[count++]=version;
        return suite?0:PF_Err_BAD_CALLBACK_PARAM;
    }
    A_Err close() noexcept {
        A_Err first{};
        while(count){--count;const auto error=basic->ReleaseSuite(names[count],versions[count]);if(!first)first=error;}
        return first;
    }
    ~Suites(){(void)close();}
};
struct Streams {
    const AEGP_StreamSuite6* suite{};
    std::array<AEGP_StreamRefH,4> refs{};
    A_Err close() noexcept {
        A_Err first{};
        for(auto it=refs.rbegin();it!=refs.rend();++it)if(*it){
            const auto ref=*it;*it=nullptr;const auto error=suite->AEGP_DisposeStream(ref);if(!first)first=error;
        }
        return first;
    }
    ~Streams(){(void)close();}
};
std::optional<core::RationalTime> interpolate_time(core::RationalTime a,core::RationalTime b,
    std::int64_t numerator,std::int64_t denominator) noexcept {
    const auto delta=core::subtract(b,a);if(!delta)return {};
    const auto part=core::multiply(*delta,{numerator,denominator});if(!part)return {};
    const auto value=core::add(a,*part);if(!value)return {};
    return core::make_rational(value->value,value->scale);
}
double point_distance(core::Vec3 a,core::Vec3 b) noexcept {
    return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);
}
struct TrackSegment {MotionTimedLayerPoint left,right;};
}
PF_Err capture_motion_light_timing(PF_InData* data,AEGP_PluginID plugin,std::uint32_t id,
    const core::Cancellation& cancel,MotionLightTiming& output,bool include_keyframe_times) noexcept try {
    if(!data||!data->effect_ref||plugin<=0||!id||id>0x7fffffffu||!data->pica_basicP||
       !data->pica_basicP->AcquireSuite||!data->pica_basicP->ReleaseSuite)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    Suites suites{data->pica_basicP};
    const AEGP_PFInterfaceSuite1* pf{};const AEGP_LayerSuite9* layer{};const AEGP_LightSuite2* light{};
    const AEGP_StreamSuite6* stream{};const AEGP_DynamicStreamSuite4* dynamic{};const AEGP_KeyframeSuite5* keys{};
    auto error=suites.get(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,pf);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPLayerSuite,kAEGPLayerSuiteVersion9,layer);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPLightSuite,kAEGPLightSuiteVersion2,light);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,stream);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4,dynamic);if(error)return static_cast<PF_Err>(error);
    error=suites.get(kAEGPKeyframeSuite,kAEGPKeyframeSuiteVersion5,keys);if(error)return static_cast<PF_Err>(error);
    if(!pf->AEGP_GetEffectLayer||!layer->AEGP_GetLayerParentComp||!layer->AEGP_GetLayerFromLayerID||
       !layer->AEGP_GetLayerID||!layer->AEGP_GetLayerObjectType||!light->AEGP_GetLightType||
       !stream->AEGP_GetNewLayerStream||!stream->AEGP_DisposeStream||!stream->AEGP_GetStreamType||
       !dynamic->AEGP_IsSeparationLeader||!dynamic->AEGP_AreDimensionsSeparated||!dynamic->AEGP_GetSeparationFollower||
       !keys->AEGP_GetStreamNumKFs||!keys->AEGP_GetKeyframeTime)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    AEGP_LayerH owner{},source{};AEGP_CompH comp{},source_comp{};AEGP_LayerIDVal actual{};
    error=pf->AEGP_GetEffectLayer(data->effect_ref,&owner);if(error)return static_cast<PF_Err>(error);
    if(!owner)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerParentComp(owner,&comp);if(error)return static_cast<PF_Err>(error);
    if(!comp)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerFromLayerID(comp,static_cast<AEGP_LayerIDVal>(id),&source);if(error)return static_cast<PF_Err>(error);
    if(!source)return PF_Err_BAD_CALLBACK_PARAM;
    error=layer->AEGP_GetLayerParentComp(source,&source_comp);if(error)return static_cast<PF_Err>(error);
    error=layer->AEGP_GetLayerID(source,&actual);if(error)return static_cast<PF_Err>(error);
    if(source_comp!=comp||actual!=static_cast<AEGP_LayerIDVal>(id))return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_ObjectType type{};AEGP_LightType light_type{};
    error=layer->AEGP_GetLayerObjectType(source,&type);if(error)return static_cast<PF_Err>(error);
    if(type!=AEGP_ObjectType_LIGHT)return PF_Err_BAD_CALLBACK_PARAM;
    error=light->AEGP_GetLightType(source,&light_type);if(error)return static_cast<PF_Err>(error);
    if(light_type!=AEGP_LightType_POINT)return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    Streams owned{stream};
    error=stream->AEGP_GetNewLayerStream(plugin,source,AEGP_LayerStream_POSITION,&owned.refs[0]);if(error)return static_cast<PF_Err>(error);
    if(!owned.refs[0])return PF_Err_BAD_CALLBACK_PARAM;
    AEGP_StreamType stream_type{};
    error=stream->AEGP_GetStreamType(owned.refs[0],&stream_type);if(error)return static_cast<PF_Err>(error);
    if(stream_type!=AEGP_StreamType_ThreeD&&stream_type!=AEGP_StreamType_ThreeD_SPATIAL)return PF_Err_BAD_CALLBACK_PARAM;
    A_Boolean leader{},separated{};
    error=dynamic->AEGP_IsSeparationLeader(owned.refs[0],&leader);if(error)return static_cast<PF_Err>(error);
    if(leader){error=dynamic->AEGP_AreDimensionsSeparated(owned.refs[0],&separated);if(error)return static_cast<PF_Err>(error);}
    MotionLightTiming result;result.layer_id=id;result.separated=separated!=0;
    const auto scan=[&](AEGP_StreamRefH ref)->A_Err {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        A_long count{};auto e=keys->AEGP_GetStreamNumKFs(ref,&count);if(e)return e;
        if(count<0)return PF_Err_BAD_CALLBACK_PARAM;
        if(!count)return 0;
        A_Time first{},last{};
        if(include_keyframe_times) {
            // Match the path geometry's existing 256-control-point bound. Fail
            // explicitly instead of silently dropping authored keyframe times.
            if(count>256)return PF_Err_BAD_CALLBACK_PARAM;
            for(A_long index=0;index<count;++index) {
                if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
                A_Time key{};e=keys->AEGP_GetKeyframeTime(ref,index,AEGP_LTimeMode_CompTime,&key);if(e)return e;
                auto instant=core::make_rational(key.value,key.scale);if(!instant)return PF_Err_BAD_CALLBACK_PARAM;
                const auto position=std::lower_bound(result.keyframe_times.begin(),result.keyframe_times.end(),*instant,
                    [](core::RationalTime a,core::RationalTime b){return core::compare(a,b)<0;});
                if(position==result.keyframe_times.end()||core::compare(*position,*instant)!=0) {
                    if(result.keyframe_times.size()==256)return PF_Err_BAD_CALLBACK_PARAM;
                    result.keyframe_times.insert(position,*instant);
                }
            }
            if(result.keyframe_times.empty())return PF_Err_BAD_CALLBACK_PARAM;
            first={static_cast<A_long>(result.keyframe_times.front().value),static_cast<A_u_long>(result.keyframe_times.front().scale)};
            last={static_cast<A_long>(result.keyframe_times.back().value),static_cast<A_u_long>(result.keyframe_times.back().scale)};
        } else {
            e=keys->AEGP_GetKeyframeTime(ref,0,AEGP_LTimeMode_CompTime,&first);if(e)return e;
            if(count==1)last=first;
            else{e=keys->AEGP_GetKeyframeTime(ref,count-1,AEGP_LTimeMode_CompTime,&last);if(e)return e;}
        }
        auto a=core::make_rational(first.value,first.scale),b=core::make_rational(last.value,last.scale);
        if(!a||!b||(count>1&&core::compare(*a,*b)==0))return PF_Err_BAD_CALLBACK_PARAM;
        if(core::compare(*a,*b)>0)std::swap(a,b); // composition-time range, including reversed time mapping
        if(!result.position_keyframes||core::compare(*a,result.first_keyframe)<0)result.first_keyframe=*a;
        if(!result.position_keyframes||core::compare(*b,result.last_keyframe)>0)result.last_keyframe=*b;
        result.position_keyframes+=static_cast<std::uint64_t>(count);return 0;
    };
    if(separated)for(A_short dimension=0;dimension<3;++dimension){
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        error=dynamic->AEGP_GetSeparationFollower(owned.refs[0],dimension,&owned.refs[dimension+1]);if(error)return static_cast<PF_Err>(error);
        if(!owned.refs[dimension+1])return PF_Err_BAD_CALLBACK_PARAM;
        AEGP_StreamType follower_type{};
        error=stream->AEGP_GetStreamType(owned.refs[dimension+1],&follower_type);if(error)return static_cast<PF_Err>(error);
        if(follower_type!=AEGP_StreamType_OneD)return PF_Err_BAD_CALLBACK_PARAM;
        error=scan(owned.refs[dimension+1]);if(error)return static_cast<PF_Err>(error);
    }else{error=scan(owned.refs[0]);if(error)return static_cast<PF_Err>(error);}
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    error=owned.close();if(error)return static_cast<PF_Err>(error);
    error=suites.close();if(error)return static_cast<PF_Err>(error);
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    output=std::move(result);return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err capture_motion_light_keyframes(PF_InData* data,AEGP_PluginID plugin,std::uint32_t id,
    core::LayerUnits units,const core::Cancellation& cancel,MotionLightKeyframes& output) noexcept try {
    MotionLightKeyframes result;
    auto error=capture_motion_light_timing(data,plugin,id,cancel,result.timing,true);
    if(error)return error;
    // A point light without a distinct position-keyframe interval does not
    // define a Light Path. Preserve that fact as an empty/singleton snapshot so
    // the multi-source collector can skip it instead of failing the whole
    // pre-render because Starting With matched a static light too.
    if(result.timing.keyframe_times.empty()) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        output=std::move(result);
        return PF_Err_NONE;
    }
    std::vector<MotionTimedLayerPointRequest> requests;
    requests.reserve(result.timing.keyframe_times.size());
    for(const auto time:result.timing.keyframe_times) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        requests.push_back({id,{},time});
    }
    error=capture_motion_timed_layer_points(data,units,requests,cancel,result.positions);
    if(error)return error;
    if(result.positions.size()!=result.timing.keyframe_times.size())return PF_Err_BAD_CALLBACK_PARAM;
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    output=std::move(result);
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err capture_motion_light_path_track(PF_InData* data,AEGP_PluginID plugin,std::uint32_t id,
    core::LayerUnits units,double max_chord_error,const core::Cancellation& cancel,
    MotionLightPathTrack& output) noexcept try {
    if(!std::isfinite(max_chord_error)||max_chord_error<=0||max_chord_error>1e6)
        return PF_Err_BAD_CALLBACK_PARAM;
    MotionLightKeyframes keys;
    auto error=capture_motion_light_keyframes(data,plugin,id,units,cancel,keys);
    if(error)return error;
    if(keys.positions.size()>core::kMaxMotionPathSamples)
        return PF_Err_BAD_CALLBACK_PARAM;

    MotionLightPathTrack result;result.timing=std::move(keys.timing);result.samples=std::move(keys.positions);
    if(result.samples.size()<2) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        output=std::move(result);
        return PF_Err_NONE;
    }
    std::vector<TrackSegment> active;active.reserve(result.samples.size()-1);
    for(std::size_t i=1;i<result.samples.size();++i)
        active.push_back({result.samples[i-1],result.samples[i]});
    constexpr std::array<std::int64_t,3> probe_numerators{1,2,3};
    while(!active.empty()) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        if(active.size()>core::kMaxMotionPathSamples)return PF_Err_BAD_CALLBACK_PARAM;
        std::vector<MotionTimedLayerPointRequest> requests;requests.reserve(active.size()*3);
        for(const auto& segment:active)for(const auto numerator:probe_numerators) {
            if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
            const auto instant=interpolate_time(segment.left.source_comp_time,segment.right.source_comp_time,numerator,4);
            if(!instant||core::compare(segment.left.source_comp_time,*instant)>=0||
               core::compare(*instant,segment.right.source_comp_time)>=0)return PF_Err_BAD_CALLBACK_PARAM;
            requests.push_back({id,{},*instant});
        }
        std::vector<MotionTimedLayerPoint> probes;probes.reserve(requests.size());
        for(std::size_t offset=0;offset<requests.size();offset+=kMaxMotionPointRequests) {
            if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
            const auto count=std::min(kMaxMotionPointRequests,requests.size()-offset);
            std::vector<MotionTimedLayerPoint> batch;
            error=capture_motion_timed_layer_points(data,units,
                std::span<const MotionTimedLayerPointRequest>(requests).subspan(offset,count),cancel,batch);
            if(error)return error;
            if(batch.size()!=count)return PF_Err_BAD_CALLBACK_PARAM;
            probes.insert(probes.end(),batch.begin(),batch.end());
        }

        std::vector<TrackSegment> refined;refined.reserve(active.size()*2);
        std::vector<MotionTimedLayerPoint> inserted;inserted.reserve(active.size());
        for(std::size_t i=0;i<active.size();++i) {
            if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
            const auto& segment=active[i];double maximum_error=0;
            for(std::size_t probe=0;probe<3;++probe) {
                const auto fraction=double(probe+1)/4.;
                const auto& point=probes[i*3+probe];
                const core::Vec3 linear{
                    segment.left.position.x+(segment.right.position.x-segment.left.position.x)*fraction,
                    segment.left.position.y+(segment.right.position.y-segment.left.position.y)*fraction,
                    segment.left.position.z+(segment.right.position.z-segment.left.position.z)*fraction};
                const auto deviation=point_distance(point.position,linear);
                if(!std::isfinite(deviation))return PF_Err_BAD_CALLBACK_PARAM;
                maximum_error=std::max(maximum_error,deviation);
            }
            if(maximum_error<=max_chord_error) {
                result.maximum_probe_error=std::max(result.maximum_probe_error,maximum_error);
                continue;
            }
            if(result.samples.size()+inserted.size()+1>core::kMaxMotionPathSamples)
                return PF_Err_BAD_CALLBACK_PARAM;
            const auto midpoint=probes[i*3+1];
            inserted.push_back(midpoint);
            refined.push_back({segment.left,midpoint});refined.push_back({midpoint,segment.right});
        }
        result.samples.insert(result.samples.end(),inserted.begin(),inserted.end());
        active=std::move(refined);
    }
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    std::sort(result.samples.begin(),result.samples.end(),[](const auto& a,const auto& b){
        return core::compare(a.source_comp_time,b.source_comp_time)<0;
    });
    for(std::size_t i=1;i<result.samples.size();++i)
        if(result.samples[i].layer_id!=id||core::compare(result.samples[i-1].source_comp_time,result.samples[i].source_comp_time)>=0)
            return PF_Err_BAD_CALLBACK_PARAM;
    if(result.samples.front().layer_id!=id||result.maximum_probe_error>max_chord_error)
        return PF_Err_BAD_CALLBACK_PARAM;
    output=std::move(result);
    return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err capture_motion_light_path_tracks(PF_InData* data,AEGP_PluginID plugin,
    std::span<const std::uint32_t> ids,core::LayerUnits units,double max_chord_error,
    const core::Cancellation& cancel,MotionLightPathTrackSet& output) noexcept try {
    if(ids.size()>kMaxMotionLightSources||!std::isfinite(max_chord_error)||
       max_chord_error<=0||max_chord_error>1e6)return PF_Err_BAD_CALLBACK_PARAM;
    MotionLightPathTrackSet result;result.tracks.reserve(ids.size());
    for(std::size_t i=0;i<ids.size();++i) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        if(ids[i]==0)return PF_Err_BAD_CALLBACK_PARAM;
        for(std::size_t previous=0;previous<i;++previous)
            if(ids[previous]==ids[i])return PF_Err_BAD_CALLBACK_PARAM;
        MotionLightPathTrack track;
        const auto error=capture_motion_light_path_track(data,plugin,ids[i],units,max_chord_error,cancel,track);
        if(error)return error;
        if(track.timing.layer_id!=ids[i])return PF_Err_BAD_CALLBACK_PARAM;
        if(track.samples.size()<2)continue;
        if(track.samples.size()>core::kMaxMotionPathSamples-result.sample_count)
            return PF_Err_BAD_CALLBACK_PARAM;
        result.sample_count+=track.samples.size();result.tracks.push_back(std::move(track));
    }
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    output=std::move(result);return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err encode_motion_light_path_track_set(const MotionLightPathTrackSet& tracks,
    const core::Cancellation& cancel,std::vector<std::byte>& output) noexcept try {
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    if(tracks.tracks.size()>kMaxMotionLightSources||
        tracks.sample_count>core::kMaxMotionLightPathResourceSamples||
        (tracks.tracks.empty()!=(tracks.sample_count==0)))
        return PF_Err_BAD_CALLBACK_PARAM;
    core::MotionLightPathResource resource;resource.tracks.reserve(tracks.tracks.size());
    resource.sample_count=tracks.sample_count;std::size_t samples=0;
    for(const auto& source:tracks.tracks) {
        if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
        if(!source.timing.layer_id||source.samples.size()<2||
           source.samples.size()>core::kMaxMotionLightPathResourceSamples-samples||
           source.timing.layer_id!=source.samples.front().layer_id)return PF_Err_BAD_CALLBACK_PARAM;
        core::MotionLightPathSourceTrack track;track.source_layer_id=source.timing.layer_id;
        track.maximum_probe_error=source.maximum_probe_error;track.samples.reserve(source.samples.size());
        for(const auto& point:source.samples) {
            if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
            if(point.layer_id!=track.source_layer_id)return PF_Err_BAD_CALLBACK_PARAM;
            track.samples.push_back({point.source_comp_time,point.position});
        }
        samples+=track.samples.size();resource.tracks.push_back(std::move(track));
    }
    if(samples!=tracks.sample_count)return PF_Err_BAD_CALLBACK_PARAM;
    auto encoded=core::encode_motion_light_path_resource(resource,cancel);
    if(!encoded.has_value()) {
        switch(encoded.error().code) {
            case core::ErrorCode::cancelled:return PF_Interrupt_CANCEL;
            case core::ErrorCode::allocation_failed:return PF_Err_OUT_OF_MEMORY;
            default:return PF_Err_BAD_CALLBACK_PARAM;
        }
    }
    if(cancel.is_cancelled())return PF_Interrupt_CANCEL;
    output=encoded.take_value();return PF_Err_NONE;
} catch(const std::bad_alloc&) {return PF_Err_OUT_OF_MEMORY;}
  catch(...) {return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
