#pragma once
#include "starfield/core/Graph.hpp"
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <bit>
#include <cmath>
#include <new>

namespace starfield::core {
namespace motion_path_points_detail {
inline std::uint64_t read(std::span<const std::byte> bytes,std::size_t at,unsigned count) noexcept {
    std::uint64_t value=0;for(unsigned i=0;i<count;++i)value|=std::uint64_t(std::to_integer<unsigned>(bytes[at+i]))<<(8*i);return value;
}
inline void write(OpaqueBytes& bytes,std::size_t at,std::uint64_t value,unsigned count) noexcept {
    for(unsigned i=0;i<count;++i)bytes[at+i]=std::byte((value>>(8*i))&255);
}
inline bool coordinate(double v) noexcept {return std::isfinite(v)&&std::abs(v)<=kMaxMotionCoordinate;}
}
// Independent numeric packet; the surrounding graph owns its checksum.
inline Result<std::size_t> validate_motion_path_points(std::span<const std::byte> bytes,
    const Cancellation& cancel) noexcept {
    using R=Result<std::size_t>;using namespace motion_path_points_detail;
    if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion points validation cancelled");
    if(bytes.size()<16||read(bytes,0,4)!=0x504d4653||read(bytes,4,2)!=1||read(bytes,6,2)!=24||read(bytes,12,4)!=0)
        return R::failure(ErrorCode::invalid_request,"invalid Motion points header");
    const auto count=read(bytes,8,4);
    if(!count||count>kMaxMotionPathControlPoints||bytes.size()!=16+count*24)
        return R::failure(ErrorCode::invalid_request,"invalid Motion points count/length");
    for(std::size_t i=0;i<count*3;++i){
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion points validation cancelled");
        if(!coordinate(std::bit_cast<double>(read(bytes,16+i*8,8))))
            return R::failure(ErrorCode::invalid_request,"invalid Motion point coordinate");
    }
    return R::success(static_cast<std::size_t>(count));
}
inline Result<std::vector<Vec3>> decode_motion_path_points(std::span<const std::byte> bytes,
    const Cancellation& cancel) noexcept {
    using R=Result<std::vector<Vec3>>;using namespace motion_path_points_detail;
    auto checked=validate_motion_path_points(bytes,cancel);if(!checked.has_value())return R::failure(checked.error());
    try {
        std::vector<Vec3> points;points.reserve(checked.value());
        for(std::size_t i=0;i<checked.value();++i){
            if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion points decoding cancelled");
            points.push_back({std::bit_cast<double>(read(bytes,16+i*24,8)),
                std::bit_cast<double>(read(bytes,24+i*24,8)),std::bit_cast<double>(read(bytes,32+i*24,8))});
        }
        return R::success(std::move(points));
    }catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Motion points allocation failed");}
}
inline Result<OpaqueBytes> encode_motion_path_points(std::span<const Vec3> points,const Cancellation& cancel) noexcept {
    using R=Result<OpaqueBytes>;using namespace motion_path_points_detail;
    if(points.empty()||points.size()>kMaxMotionPathControlPoints)return R::failure(ErrorCode::invalid_request,"invalid Motion points count");
    for(auto p:points){if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion points encoding cancelled");
        if(!coordinate(p.x)||!coordinate(p.y)||!coordinate(p.z))return R::failure(ErrorCode::invalid_request,"invalid Motion point coordinate");}
    try {
        OpaqueBytes bytes(16+points.size()*24);write(bytes,0,0x504d4653,4);write(bytes,4,1,2);write(bytes,6,24,2);write(bytes,8,points.size(),4);
        for(std::size_t i=0;i<points.size();++i){if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion points encoding cancelled");
            write(bytes,16+i*24,std::bit_cast<std::uint64_t>(points[i].x),8);write(bytes,24+i*24,std::bit_cast<std::uint64_t>(points[i].y),8);
            write(bytes,32+i*24,std::bit_cast<std::uint64_t>(points[i].z),8);}
        return R::success(std::move(bytes));
    }catch(const std::bad_alloc&){return R::failure(ErrorCode::allocation_failed,"Motion points allocation failed");}
}
} // namespace starfield::core
