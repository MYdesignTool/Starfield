#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/Render.hpp"

#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace starfield::core {

inline constexpr std::size_t kMaxMotionLookAtTargets=256;
inline constexpr std::size_t kMotionLookAtTargetsHeaderBytes=16;
inline constexpr std::size_t kMotionLookAtTargetBytes=32;

struct MotionLookAtTarget {
    std::uint32_t layer_id{};
    Vec3 position{};
};

inline bool valid_motion_look_at_target(const MotionLookAtTarget& target) noexcept {
    return target.layer_id!=0 && target.layer_id<=0x7fffffff &&
        std::isfinite(target.position.x) && std::isfinite(target.position.y) &&
        std::isfinite(target.position.z) && std::abs(target.position.x)<=1e9 &&
        std::abs(target.position.y)<=1e9 && std::abs(target.position.z)<=1e9;
}

inline Result<std::vector<std::byte>> encode_motion_look_at_targets(
    std::span<const MotionLookAtTarget> targets,const Cancellation& cancel) noexcept try {
    using R=Result<std::vector<std::byte>>;
    if(targets.size()>kMaxMotionLookAtTargets)
        return R::failure(ErrorCode::work_limit_exceeded,"Look At light target cap exceeded");
    std::vector<std::byte> bytes(kMotionLookAtTargetsHeaderBytes+targets.size()*kMotionLookAtTargetBytes);
    const auto write=[&](std::size_t at,std::uint64_t value,std::size_t count) {
        for(std::size_t i=0;i<count;++i)bytes[at+i]=static_cast<std::byte>((value>>(i*8))&0xffu);
    };
    bytes[0]=std::byte{'S'};bytes[1]=std::byte{'F'};bytes[2]=std::byte{'L'};bytes[3]=std::byte{'A'};
    write(4,1,2);write(6,kMotionLookAtTargetsHeaderBytes,2);write(8,targets.size(),4);write(12,0,4);
    std::size_t at=kMotionLookAtTargetsHeaderBytes;
    for(std::size_t i=0;i<targets.size();++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Look At target encoding cancelled");
        if(!valid_motion_look_at_target(targets[i]))
            return R::failure(ErrorCode::invalid_request,"invalid Look At light target");
        for(std::size_t j=0;j<i;++j)if(targets[j].layer_id==targets[i].layer_id)
            return R::failure(ErrorCode::invalid_request,"duplicate Look At light target ID");
        write(at,targets[i].layer_id,4);write(at+4,0,4);
        write(at+8,std::bit_cast<std::uint64_t>(targets[i].position.x),8);
        write(at+16,std::bit_cast<std::uint64_t>(targets[i].position.y),8);
        write(at+24,std::bit_cast<std::uint64_t>(targets[i].position.z),8);
        at+=kMotionLookAtTargetBytes;
    }
    if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Look At target encoding cancelled");
    return R::success(std::move(bytes));
} catch(const std::bad_alloc&) {
    return Result<std::vector<std::byte>>::failure(ErrorCode::allocation_failed,"Look At target allocation failed");
} catch(...) {
    return Result<std::vector<std::byte>>::failure(ErrorCode::internal_failure,"Look At target encoding failed");
}

inline Result<std::vector<MotionLookAtTarget>> decode_motion_look_at_targets(
    std::span<const std::byte> bytes,const Cancellation& cancel) noexcept try {
    using R=Result<std::vector<MotionLookAtTarget>>;
    const auto read=[&](std::size_t at,std::size_t count) {
        std::uint64_t value{};for(std::size_t i=0;i<count;++i)
            value|=std::uint64_t(std::to_integer<unsigned char>(bytes[at+i]))<<(i*8);return value;
    };
    if(bytes.size()<kMotionLookAtTargetsHeaderBytes || bytes[0]!=std::byte{'S'} ||
       bytes[1]!=std::byte{'F'} || bytes[2]!=std::byte{'L'} || bytes[3]!=std::byte{'A'} ||
       read(4,2)!=1 || read(6,2)!=kMotionLookAtTargetsHeaderBytes || read(12,4)!=0)
        return R::failure(ErrorCode::invalid_request,"invalid Look At target header");
    const auto count=static_cast<std::size_t>(read(8,4));
    if(count>kMaxMotionLookAtTargets || bytes.size()!=kMotionLookAtTargetsHeaderBytes+count*kMotionLookAtTargetBytes)
        return R::failure(ErrorCode::work_limit_exceeded,"invalid Look At target resource size");
    std::vector<MotionLookAtTarget> result;result.reserve(count);
    for(std::size_t i=0;i<count;++i) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Look At target decoding cancelled");
        const auto at=kMotionLookAtTargetsHeaderBytes+i*kMotionLookAtTargetBytes;
        if(read(at+4,4)!=0)return R::failure(ErrorCode::invalid_request,"invalid Look At target flags");
        MotionLookAtTarget target{static_cast<std::uint32_t>(read(at,4)),{
            std::bit_cast<double>(read(at+8,8)),std::bit_cast<double>(read(at+16,8)),
            std::bit_cast<double>(read(at+24,8))}};
        if(!valid_motion_look_at_target(target))return R::failure(ErrorCode::invalid_request,"invalid Look At target value");
        for(const auto& old:result)if(old.layer_id==target.layer_id)
            return R::failure(ErrorCode::invalid_request,"duplicate Look At target ID");
        result.push_back(target);
    }
    return R::success(std::move(result));
} catch(const std::bad_alloc&) {
    return Result<std::vector<MotionLookAtTarget>>::failure(ErrorCode::allocation_failed,"Look At target allocation failed");
} catch(...) {
    return Result<std::vector<MotionLookAtTarget>>::failure(ErrorCode::internal_failure,"Look At target decoding failed");
}

} // namespace starfield::core
