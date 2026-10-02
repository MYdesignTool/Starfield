#include "starfield/core/EmitterHistory.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <new>

namespace starfield::core {
namespace {
bool before(const EmitterOriginSample& a, const EmitterOriginSample& b) {
    return a.emitter != b.emitter ? a.emitter < b.emitter : a.birth_seconds < b.birth_seconds;
}
bool valid(const EmitterOriginSample& value) {
    return std::isfinite(value.birth_seconds) && value.birth_seconds >= 0 &&
        std::isfinite(value.origin.x) && std::isfinite(value.origin.y) && std::isfinite(value.origin.z);
}
bool history_tag(const OpaqueBytes& bytes) {
    return bytes.size()>=2 && bytes[0]==std::byte{3} && bytes[1]==std::byte{0x80};
}
}
Result<Vec3> EmitterOriginHistory::sample(NodeId emitter, double birth_seconds) {
    const EmitterOriginSample key{emitter,birth_seconds,{}};
    const auto found=std::lower_bound(samples.begin(),samples.end(),key,before);
    if(found==samples.end() || found->emitter!=emitter || found->birth_seconds!=birth_seconds)
        return Result<Vec3>::failure(ErrorCode::invalid_request,"emitter birth position is missing from render history");
    return Result<Vec3>::success(found->origin);
}
Result<OpaqueBytes> encode_emitter_origin_history(std::vector<EmitterOriginSample> samples) {
    using R=Result<OpaqueBytes>;
    try {
        if(samples.size()>kMaxEmitterOriginSamples) return R::failure(ErrorCode::work_limit_exceeded,"emitter origin history sample limit exceeded");
        if(!std::is_sorted(samples.begin(),samples.end(),before)) std::sort(samples.begin(),samples.end(),before);
        for(std::size_t i=0;i<samples.size();++i)
            if(!valid(samples[i]) || (i && !before(samples[i-1],samples[i])))
                return R::failure(ErrorCode::invalid_request,"invalid or duplicate emitter origin sample");
        OpaqueBytes bytes; bytes.reserve(12+48*samples.size());
        const auto append=[&](std::uint64_t value,unsigned count) {
            for(unsigned i=0;i<count;++i) bytes.push_back(static_cast<std::byte>((value>>(8*i))&255));
        };
        append(0x8003,2);append(1,2);append(12+48*samples.size(),4);append(samples.size(),4);
        for(const auto& sample:samples) {
            for(auto b:sample.emitter.value.bytes) bytes.push_back(static_cast<std::byte>(b));
            for(auto value:{sample.birth_seconds,sample.origin.x,sample.origin.y,sample.origin.z}) append(std::bit_cast<std::uint64_t>(value),8);
        }
        return R::success(std::move(bytes));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"emitter origin history allocation failed");}
}
Result<EmitterOriginHistory> decode_emitter_origin_history(const Graph& graph) {
    using R=Result<EmitterOriginHistory>;
    try {
        EmitterOriginHistory result;
        for(const auto& bytes:graph.optional_records) {
            if(!history_tag(bytes)) continue;
            if(result.present || bytes.size()<12) return R::failure(ErrorCode::invalid_request,"invalid emitter origin history record");
            result.present=true;std::size_t at=0;
            const auto read=[&](unsigned count) {
                std::uint64_t value=0;for(unsigned i=0;i<count;++i) value|=std::uint64_t(std::to_integer<unsigned char>(bytes[at++]))<<(8*i);
                return value;
            };
            (void)read(2); const auto version=read(2),length=read(4),count=read(4);
            if(version!=1 || length!=bytes.size() || count>kMaxEmitterOriginSamples || length!=12+48*count)
                return R::failure(ErrorCode::invalid_request,"invalid emitter origin history size/version");
            result.samples.reserve(static_cast<std::size_t>(count));
            for(std::uint64_t i=0;i<count;++i) {
                EmitterOriginSample sample;
                for(auto& b:sample.emitter.value.bytes) b=static_cast<std::uint8_t>(read(1));
                sample.birth_seconds=std::bit_cast<double>(read(8));
                sample.origin={std::bit_cast<double>(read(8)),std::bit_cast<double>(read(8)),std::bit_cast<double>(read(8))};
                if(!valid(sample) || (!result.samples.empty() && !before(result.samples.back(),sample)))
                    return R::failure(ErrorCode::invalid_request,"invalid emitter origin history ordering/value");
                result.samples.push_back(sample);
            }
        }
        return R::success(std::move(result));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"emitter origin history allocation failed");}
}
} // namespace starfield::core

namespace starfield::core {
Result<OpaqueBytes> encode_evaluated_particles(const EvaluatedGraph& graph,RationalTime time) {
    using R=Result<OpaqueBytes>;
    try {
        if(!time.scale || graph.particles.size()>kMaxParticleCount || graph.evaluated_nodes.size()>kMaxGraphNodes)
            return R::failure(ErrorCode::invalid_request,"invalid temporal particle snapshot");
        OpaqueBytes bytes;bytes.reserve(32+16*graph.evaluated_nodes.size()+128*graph.particles.size());
        const auto append=[&](std::uint64_t value,unsigned count) {
            for(unsigned i=0;i<count;++i) bytes.push_back(static_cast<std::byte>((value>>(8*i))&255));
        };
        append(0x8004,2);append(1,2);append(32+16*graph.evaluated_nodes.size()+128*graph.particles.size(),4);
        append(std::bit_cast<std::uint64_t>(time.value),8);append(time.scale,8);
        append(graph.evaluated_nodes.size(),4);append(graph.particles.size(),4);
        for(const auto& node:graph.evaluated_nodes) for(auto b:node.value.bytes) append(b,1);
        for(const auto& p:graph.particles) {
            append(p.id,8);for(auto b:p.emitter_id.value.bytes) append(b,1);
            for(double value:{p.age_seconds,p.lifetime_seconds,p.size_pixels,p.opacity,
                p.color.x,p.color.y,p.color.z,p.position.x,p.position.y,p.position.z,p.velocity.x,p.velocity.y,p.velocity.z}) {
                if(!std::isfinite(value)) return R::failure(ErrorCode::invalid_request,"nonfinite temporal particle");
                append(std::bit_cast<std::uint64_t>(value),8);
            }
        }
        return R::success(std::move(bytes));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"temporal snapshot allocation failed");}
}
Result<EvaluatedGraph> decode_evaluated_particles(const OpaqueBytes& bytes,RationalTime time) {
    using R=Result<EvaluatedGraph>;
    try {
        if(bytes.size()<32) return R::failure(ErrorCode::invalid_request,"short temporal snapshot");
        std::size_t at=0;const auto read=[&](unsigned count) {
            std::uint64_t value=0;for(unsigned i=0;i<count;++i) value|=std::uint64_t(std::to_integer<unsigned char>(bytes[at++]))<<(8*i);return value;
        };
        const auto tag=read(2),version=read(2),length=read(4);
        const auto clock=std::bit_cast<std::int64_t>(read(8));const auto scale=read(8);
        const auto nodes=read(4),particles=read(4);
        if(tag!=0x8004 || version!=1 || length!=bytes.size() || !scale || !time.scale ||
            static_cast<long double>(clock)*time.scale!=static_cast<long double>(time.value)*scale ||
            nodes>kMaxGraphNodes || particles>kMaxParticleCount || bytes.size()!=32+16*nodes+128*particles)
            return R::failure(ErrorCode::invalid_request,"invalid temporal snapshot header/time");
        EvaluatedGraph result;result.evaluated_nodes.resize(static_cast<std::size_t>(nodes));
        for(auto& node:result.evaluated_nodes) for(auto& b:node.value.bytes) b=static_cast<std::uint8_t>(read(1));
        result.particles.resize(static_cast<std::size_t>(particles));
        for(auto& p:result.particles) {
            p.id=read(8);for(auto& b:p.emitter_id.value.bytes) b=static_cast<std::uint8_t>(read(1));
            for(double* value:{&p.age_seconds,&p.lifetime_seconds,&p.size_pixels,&p.opacity,&p.color.x,&p.color.y,&p.color.z,
                &p.position.x,&p.position.y,&p.position.z,&p.velocity.x,&p.velocity.y,&p.velocity.z}) {
                *value=std::bit_cast<double>(read(8));if(!std::isfinite(*value)) return R::failure(ErrorCode::invalid_request,"nonfinite temporal particle");
            }
            if(p.age_seconds<0 || p.lifetime_seconds<=0 || p.age_seconds>=p.lifetime_seconds ||
                p.size_pixels<0 || p.opacity<0 || p.opacity>1 ||
                p.color.x<0 || p.color.y<0 || p.color.z<0 || p.color.x>kMaxParticleColor || p.color.y>kMaxParticleColor || p.color.z>kMaxParticleColor)
                return R::failure(ErrorCode::invalid_request,"invalid temporal particle values");
        }
        return R::success(std::move(result));
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"temporal snapshot allocation failed");}
}
} // namespace starfield::core
