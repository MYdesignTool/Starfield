#include "starfield/core/MotionLightPathResource.hpp"
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>

namespace starfield::core {
namespace {
constexpr std::uint32_t kMagic=0x4c504653; // SFPL, little-endian
constexpr std::uint16_t kVersion=1;
constexpr double kMaxSourceSeconds=1e9;
constexpr double kMaxProbeError=1e9;

std::uint64_t read_u64(std::span<const std::byte> bytes,std::size_t at,unsigned count) noexcept {
    std::uint64_t value=0;
    for(unsigned i=0;i<count;++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[at+i]))<<(8*i);
    return value;
}
void write_u64(std::vector<std::byte>& bytes,std::size_t at,std::uint64_t value,unsigned count) noexcept {
    for(unsigned i=0;i<count;++i)bytes[at+i]=std::byte((value>>(8*i))&255);
}
bool bounded(Vec3 p) noexcept {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&
        std::abs(p.x)<=kMaxMotionCoordinate&&std::abs(p.y)<=kMaxMotionCoordinate&&
        std::abs(p.z)<=kMaxMotionCoordinate;
}
bool valid_track(const MotionLightPathSourceTrack& track,const Cancellation& cancel) noexcept {
    if(!track.source_layer_id||track.samples.size()<2||track.samples.size()>kMaxMotionLightPathResourceSamples||
       !std::isfinite(track.maximum_probe_error)||track.maximum_probe_error<0||track.maximum_probe_error>kMaxProbeError)
        return false;
    for(std::size_t i=0;i<track.samples.size();++i) {
        if(cancel.is_cancelled())return false;
        const auto& sample=track.samples[i];
        const auto normalized=make_rational(sample.source_comp_time.value,sample.source_comp_time.scale);
        if(!normalized||!std::isfinite(to_seconds(*normalized))||
           std::abs(to_seconds(*normalized))>kMaxSourceSeconds||!bounded(sample.position))return false;
        if(i&&compare(track.samples[i-1].source_comp_time,*normalized)>=0)return false;
    }
    return true;
}
Vec3 subtract(Vec3 a,Vec3 b) noexcept {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b) noexcept {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 multiply(Vec3 a,double b) noexcept {return {a.x*b,a.y*b,a.z*b};}
Vec3 unit(Vec3 a) noexcept {
    const auto length=std::hypot(a.x,a.y,a.z);
    return length>0&&std::isfinite(length)?multiply(a,1/length):Vec3{};
}
bool valid_settings(const MotionLightPathSettings& settings) noexcept {
    const auto& f=settings.forward;
    return std::isfinite(settings.max_delay_seconds)&&settings.max_delay_seconds>=0&&settings.max_delay_seconds<=1e6&&
        std::isfinite(settings.delay_random_percent)&&settings.delay_random_percent>=0&&settings.delay_random_percent<=100&&
        std::isfinite(f.x)&&std::isfinite(f.y)&&std::isfinite(f.z)&&
        std::abs(f.x)<=kMaxMotionCoordinate&&std::abs(f.y)<=kMaxMotionCoordinate&&std::abs(f.z)<=kMaxMotionCoordinate&&
        (!settings.orient_to_path||f.x!=0||f.y!=0||f.z!=0);
}
}

Result<std::vector<std::byte>> encode_motion_light_path_resource(
    const MotionLightPathResource& resource,const Cancellation& cancel) noexcept try {
    using R=Result<std::vector<std::byte>>;
    if(resource.tracks.size()>kMaxMotionLightPathSources||
       resource.sample_count>kMaxMotionLightPathResourceSamples||
       (resource.tracks.empty()!=(resource.sample_count==0)))
        return R::failure(ErrorCode::invalid_request,"invalid Light Path resource size");
    std::size_t actual_samples=0;
    for(std::size_t i=0;i<resource.tracks.size();++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path resource encoding cancelled");
        const auto& track=resource.tracks[i];
        if(!valid_track(track,cancel))return R::failure(cancel.is_cancelled()?ErrorCode::cancelled:ErrorCode::invalid_request,
            "invalid Light Path source track");
        for(std::size_t j=0;j<i;++j)if(resource.tracks[j].source_layer_id==track.source_layer_id)
            return R::failure(ErrorCode::invalid_request,"duplicate Light Path source ID");
        if(track.samples.size()>kMaxMotionLightPathResourceSamples-actual_samples)
            return R::failure(ErrorCode::work_limit_exceeded,"Light Path aggregate sample cap exceeded");
        actual_samples+=track.samples.size();
    }
    if(actual_samples!=resource.sample_count)return R::failure(ErrorCode::invalid_request,"Light Path sample count mismatch");
    const auto total=kMotionLightPathResourceHeaderBytes+
        resource.tracks.size()*kMotionLightPathTrackHeaderBytes+actual_samples*kMotionLightPathSampleBytes;
    std::vector<std::byte> bytes(total);
    write_u64(bytes,0,kMagic,4);write_u64(bytes,4,kVersion,2);
    write_u64(bytes,6,kMotionLightPathResourceHeaderBytes,2);
    write_u64(bytes,8,kMotionLightPathTrackHeaderBytes,2);
    write_u64(bytes,10,kMotionLightPathSampleBytes,2);
    write_u64(bytes,12,resource.tracks.size(),4);write_u64(bytes,16,actual_samples,4);write_u64(bytes,20,0,4);
    std::size_t at=kMotionLightPathResourceHeaderBytes;
    for(const auto& track:resource.tracks) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path resource encoding cancelled");
        write_u64(bytes,at,track.source_layer_id,4);write_u64(bytes,at+4,track.samples.size(),4);
        write_u64(bytes,at+8,std::bit_cast<std::uint64_t>(track.maximum_probe_error),8);at+=kMotionLightPathTrackHeaderBytes;
        for(const auto& sample:track.samples) {
            const auto time=*make_rational(sample.source_comp_time.value,sample.source_comp_time.scale);
            write_u64(bytes,at,std::bit_cast<std::uint64_t>(time.value),8);
            write_u64(bytes,at+8,std::bit_cast<std::uint64_t>(time.scale),8);
            write_u64(bytes,at+16,std::bit_cast<std::uint64_t>(sample.position.x),8);
            write_u64(bytes,at+24,std::bit_cast<std::uint64_t>(sample.position.y),8);
            write_u64(bytes,at+32,std::bit_cast<std::uint64_t>(sample.position.z),8);at+=kMotionLightPathSampleBytes;
        }
    }
    return R::success(std::move(bytes));
} catch(const std::bad_alloc&) {return Result<std::vector<std::byte>>::failure(ErrorCode::allocation_failed,"Light Path resource allocation failed");}
  catch(...) {return Result<std::vector<std::byte>>::failure(ErrorCode::internal_failure,"Light Path resource encoding failed");}

Result<MotionLightPathResource> decode_motion_light_path_resource(
    std::span<const std::byte> bytes,const Cancellation& cancel) noexcept try {
    using R=Result<MotionLightPathResource>;
    if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path resource decoding cancelled");
    if(bytes.size()<kMotionLightPathResourceHeaderBytes||read_u64(bytes,0,4)!=kMagic||
       read_u64(bytes,4,2)!=kVersion||read_u64(bytes,6,2)!=kMotionLightPathResourceHeaderBytes||
       read_u64(bytes,8,2)!=kMotionLightPathTrackHeaderBytes||read_u64(bytes,10,2)!=kMotionLightPathSampleBytes||
       read_u64(bytes,20,4)!=0)return R::failure(ErrorCode::invalid_request,"invalid Light Path resource header");
    const auto track_count=static_cast<std::size_t>(read_u64(bytes,12,4));
    const auto sample_count=static_cast<std::size_t>(read_u64(bytes,16,4));
    if(track_count>kMaxMotionLightPathSources||sample_count>kMaxMotionLightPathResourceSamples||
       (track_count==0)!=(sample_count==0))return R::failure(ErrorCode::work_limit_exceeded,"Light Path resource cap exceeded");
    const auto minimum=kMotionLightPathResourceHeaderBytes+track_count*kMotionLightPathTrackHeaderBytes;
    if(bytes.size()<minimum)return R::failure(ErrorCode::invalid_request,"truncated Light Path track headers");
    MotionLightPathResource result;result.tracks.reserve(track_count);result.sample_count=sample_count;
    std::size_t at=kMotionLightPathResourceHeaderBytes,seen_samples=0;
    for(std::size_t i=0;i<track_count;++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path resource decoding cancelled");
        const auto id=static_cast<std::uint32_t>(read_u64(bytes,at,4));
        const auto count=static_cast<std::size_t>(read_u64(bytes,at+4,4));
        const auto maximum_error=std::bit_cast<double>(read_u64(bytes,at+8,8));at+=kMotionLightPathTrackHeaderBytes;
        if(!id||count<2||count>sample_count-seen_samples||!std::isfinite(maximum_error)||
           maximum_error<0||maximum_error>kMaxProbeError)
            return R::failure(ErrorCode::invalid_request,"invalid Light Path track header");
        if(count>(bytes.size()-at)/kMotionLightPathSampleBytes)
            return R::failure(ErrorCode::invalid_request,"truncated Light Path samples");
        MotionLightPathSourceTrack track;track.source_layer_id=id;track.maximum_probe_error=maximum_error;track.samples.reserve(count);
        for(std::size_t j=0;j<count;++j) {
            if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path resource decoding cancelled");
            MotionLightPathResourceSample sample;
            sample.source_comp_time={std::bit_cast<std::int64_t>(read_u64(bytes,at,8)),
                std::bit_cast<std::int64_t>(read_u64(bytes,at+8,8))};
            sample.position={std::bit_cast<double>(read_u64(bytes,at+16,8)),
                std::bit_cast<double>(read_u64(bytes,at+24,8)),std::bit_cast<double>(read_u64(bytes,at+32,8))};
            at+=kMotionLightPathSampleBytes;track.samples.push_back(sample);
        }
        if(!valid_track(track,cancel))return R::failure(cancel.is_cancelled()?ErrorCode::cancelled:ErrorCode::invalid_request,
            "invalid Light Path source track");
        for(const auto& prior:result.tracks)if(prior.source_layer_id==id)
            return R::failure(ErrorCode::invalid_request,"duplicate Light Path source ID");
        seen_samples+=count;result.tracks.push_back(std::move(track));
    }
    if(at!=bytes.size()||seen_samples!=sample_count)return R::failure(ErrorCode::invalid_request,"Light Path resource length/count mismatch");
    return R::success(std::move(result));
} catch(const std::bad_alloc&) {return Result<MotionLightPathResource>::failure(ErrorCode::allocation_failed,"Light Path resource allocation failed");}
  catch(...) {return Result<MotionLightPathResource>::failure(ErrorCode::internal_failure,"Light Path resource decoding failed");}

Result<CompiledMotionLightPathTrack> compile_motion_light_path_track(
    const MotionLightPathSourceTrack& source,const Cancellation& cancel) noexcept try {
    using R=Result<CompiledMotionLightPathTrack>;
    if(!valid_track(source,cancel))return R::failure(cancel.is_cancelled()?ErrorCode::cancelled:ErrorCode::invalid_request,
        "invalid Light Path source track");
    CompiledMotionLightPathTrack result;result.source_layer_id_=source.source_layer_id;
    result.times_.reserve(source.samples.size());result.positions_.reserve(source.samples.size());
    for(const auto& sample:source.samples) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path track compilation cancelled");
        result.times_.push_back(to_seconds(*make_rational(sample.source_comp_time.value,sample.source_comp_time.scale)));
        result.positions_.push_back(sample.position);
    }
    for(std::size_t i=1;i<result.times_.size();++i) {
        const auto delta=result.times_[i]-result.times_[i-1];
        const auto velocity=subtract(result.positions_[i],result.positions_[i-1]);
        const auto rate=Vec3{velocity.x/delta,velocity.y/delta,velocity.z/delta};
        if(!(delta>0)||!bounded(rate))return R::failure(ErrorCode::invalid_time,"Light Path segment time/velocity outside bounds");
    }
    return R::success(std::move(result));
} catch(const std::bad_alloc&) {return Result<CompiledMotionLightPathTrack>::failure(ErrorCode::allocation_failed,"Light Path track allocation failed");}
  catch(...) {return Result<CompiledMotionLightPathTrack>::failure(ErrorCode::internal_failure,"Light Path track compilation failed");}

Result<MotionLightPathTrackSample> CompiledMotionLightPathTrack::sample(
    double source_time,double continuation_seconds,double continuation_rate) const noexcept {
    using R=Result<MotionLightPathTrackSample>;
    if(times_.size()<2||positions_.size()!=times_.size()||!std::isfinite(source_time)||
       std::abs(source_time)>kMaxSourceSeconds||!std::isfinite(continuation_seconds)||
       continuation_seconds<0||continuation_seconds>1e6||!std::isfinite(continuation_rate)||
       continuation_rate<0||continuation_rate>1e6)
        return R::failure(ErrorCode::invalid_request,"invalid Light Path track sample request");
    const auto first=times_.front(),last=times_.back();
    if(source_time<first)return R::success({positions_.front(),{},{},false});
    const auto terminal_delta=last-times_[times_.size()-2];
    const auto terminal=subtract(positions_.back(),positions_[positions_.size()-2]);
    const Vec3 terminal_velocity{terminal.x/terminal_delta,terminal.y/terminal_delta,terminal.z/terminal_delta};
    if(source_time>=last) {
        const auto extension=multiply(terminal_velocity,continuation_seconds);
        const auto position=add(positions_.back(),extension);
        if(!bounded(position)||!bounded(terminal_velocity))return R::failure(ErrorCode::invalid_request,"Light Path continued sample outside bounds");
        return R::success({position,terminal_velocity,unit(terminal_velocity),continuation_seconds>0||continuation_rate>0});
    }
    const auto upper=static_cast<std::size_t>(std::upper_bound(times_.begin(),times_.end(),source_time)-times_.begin());
    if(upper==0||upper>=times_.size())return R::failure(ErrorCode::internal_failure,"Light Path track lookup invariant failed");
    const auto lower=upper-1;
    const auto dt=times_[upper]-times_[lower];
    const auto fraction=(source_time-times_[lower])/dt;
    const auto delta=subtract(positions_[upper],positions_[lower]);
    const Vec3 velocity{delta.x/dt,delta.y/dt,delta.z/dt};
    const auto position=add(positions_[lower],multiply(delta,fraction));
    if(!bounded(position)||!bounded(velocity))return R::failure(ErrorCode::invalid_request,"Light Path interpolated sample outside bounds");
    return R::success({position,velocity,unit(velocity),false});
}

Result<CompiledMotionLightPath> compile_motion_light_path(const MotionLightPathResource& resource,
    const MotionLightPathSettings& settings,const Cancellation& cancel) noexcept try {
    using R=Result<CompiledMotionLightPath>;
    if(!valid_settings(settings)||resource.tracks.size()>kMaxMotionLightPathSources||
       resource.sample_count>kMaxMotionLightPathResourceSamples||
       (resource.tracks.empty()!=(resource.sample_count==0)))
        return R::failure(ErrorCode::invalid_request,"invalid Light Path compile request");
    std::size_t actual_samples=0;
    CompiledMotionLightPath result;result.settings_=settings;result.sources_.reserve(resource.tracks.size());
    for(std::size_t i=0;i<resource.tracks.size();++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path compilation cancelled");
        const auto& source=resource.tracks[i];
        if(!valid_track(source,cancel))return R::failure(cancel.is_cancelled()?ErrorCode::cancelled:ErrorCode::invalid_request,
            "invalid Light Path source track");
        for(std::size_t previous=0;previous<i;++previous)
            if(resource.tracks[previous].source_layer_id==source.source_layer_id)
                return R::failure(ErrorCode::invalid_request,"duplicate Light Path source ID");
        if(source.samples.size()>kMaxMotionLightPathResourceSamples-actual_samples)
            return R::failure(ErrorCode::work_limit_exceeded,"Light Path aggregate sample cap exceeded");
        actual_samples+=source.samples.size();
        auto track=compile_motion_light_path_track(source,cancel);if(!track.has_value())return R::failure(track.error());
        auto clock=compile_motion_light_path_clock(source.samples.front().source_comp_time,
            source.samples.back().source_comp_time,settings.clock);
        if(!clock.has_value())return R::failure(clock.error());
        result.sources_.emplace_back(track.take_value(),clock.take_value());
    }
    if(actual_samples!=resource.sample_count)return R::failure(ErrorCode::invalid_request,"Light Path aggregate sample count mismatch");
    return R::success(std::move(result));
} catch(const std::bad_alloc&) {return Result<CompiledMotionLightPath>::failure(ErrorCode::allocation_failed,"Light Path compilation allocation failed");}
  catch(...) {return Result<CompiledMotionLightPath>::failure(ErrorCode::internal_failure,"Light Path compilation failed");}

std::size_t CompiledMotionLightPath::storage_bytes() const noexcept {
    std::size_t bytes=sizeof(*this)+sources_.capacity()*sizeof(Source);
    for(const auto& source:sources_)bytes+=source.track.storage_bytes();
    return bytes;
}

Result<MotionLightPathEvaluation> CompiledMotionLightPath::sample(double age,double lifetime,double random,
    const Cancellation& cancel) const noexcept {
    return sample(age,lifetime,settings_.max_delay_seconds,random,cancel);
}

Result<MotionLightPathEvaluation> CompiledMotionLightPath::sample(double age,double lifetime,double delay,
    double random,const Cancellation& cancel) const noexcept {
    using R=Result<MotionLightPathEvaluation>;
    if(!std::isfinite(delay)||delay<0||delay>1e6||!std::isfinite(random)||random<0||random>1)
        return R::failure(ErrorCode::invalid_request,"invalid Light Path sample request");
    if(sources_.empty())return R::success(MotionLightPathEvaluation{});
    MotionLightPathEvaluation result;result.source_count=sources_.size();
    for(const auto& source:sources_) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path sampling cancelled");
        const auto clock=source.clock.sample(age,lifetime,delay,random);
        if(!clock.has_value())return R::failure(clock.error());
        const auto point=source.track.sample(clock.value().source_time_seconds,
            clock.value().continuation_seconds,clock.value().continuation_rate);
        if(!point.has_value())return R::failure(point.error());
        const auto displacement=subtract(point.value().position,source.track.first_position());
        const auto rate=clock.value().ended?clock.value().continuation_rate:clock.value().source_time_rate;
        const auto velocity=multiply(point.value().velocity_per_source_second,rate);
        result.displacement=add(result.displacement,displacement);
        result.velocity=add(result.velocity,velocity);
        if(!clock.value().delayed&&rate!=0)++result.active_source_count;
        if(!bounded(result.displacement)||!bounded(result.velocity))
            return R::failure(ErrorCode::invalid_request,"combined Light Path result exceeds bounds");
    }
    result.combined_direction=unit(result.velocity);
    return R::success(result);
}

Result<MotionLightPathEvaluation> CompiledMotionLightPath::sample_at_source_elapsed(double elapsed,double rate,
    MotionPathEndBehavior end_behavior,const Cancellation& cancel) const noexcept {
    using R=Result<MotionLightPathEvaluation>;
    if(!std::isfinite(elapsed)||elapsed<0||elapsed>1e9||!std::isfinite(rate)||rate<0||rate>1e6||
       static_cast<unsigned>(end_behavior)>static_cast<unsigned>(MotionPathEndBehavior::continue_direction))
        return R::failure(ErrorCode::invalid_request,"invalid Light Path elapsed sample");
    if(sources_.empty())return R::success(MotionLightPathEvaluation{});
    MotionLightPathEvaluation result;result.source_count=sources_.size();
    for(const auto& source:sources_) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Light Path sampling cancelled");
        const auto first=source.track.first_source_time(),last=source.track.last_source_time();
        const auto duration=last-first;
        double source_time=first,continuation=0,source_rate=rate;
        if(elapsed<duration)source_time=first+elapsed;
        else if(end_behavior==MotionPathEndBehavior::stop){source_time=last;source_rate=0;}
        else {source_time=last;continuation=elapsed-duration;}
        const auto point=source.track.sample(source_time,continuation,source_rate);
        if(!point.has_value())return R::failure(point.error());
        const auto displacement=subtract(point.value().position,source.track.first_position());
        const auto velocity=multiply(point.value().velocity_per_source_second,source_rate);
        result.displacement=add(result.displacement,displacement);result.velocity=add(result.velocity,velocity);
        if(rate!=0&&elapsed<duration)++result.active_source_count;
        if(rate!=0&&elapsed>=duration&&end_behavior==MotionPathEndBehavior::continue_direction)++result.active_source_count;
        if(!bounded(result.displacement)||!bounded(result.velocity))
            return R::failure(ErrorCode::invalid_request,"combined Light Path result exceeds bounds");
    }
    result.combined_direction=unit(result.velocity);return R::success(result);
}
} // namespace starfield::core
