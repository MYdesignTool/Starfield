#pragma once
#include "starfield/core/Settings.hpp"
#include "starfield/core/Random.hpp"
#include <cmath>
#include <span>

namespace starfield::core {
struct CloudCircle { double x{}, y{}, radius{1}; };
inline bool valid_cloud_style(const ParticleCloudStyle& s) noexcept {
    return s.circles>=1 && s.circles<=kMaxCloudCircles &&
        std::isfinite(s.aspect) && s.aspect>=1 && s.aspect<=1000 &&
        std::isfinite(s.density) && s.density>=0 && s.density<=1000;
}
// Radius1 makes collapsed clouds a circle. Remaining members use a stable
// prefix, independent of density/count/time, without changing logical identity.
inline CloudCircle cloud_circle(const ParticleCloudStyle& s,std::uint32_t key,std::uint32_t member) noexcept {
    if(member==0) return {};
    const auto bits=[&](unsigned purpose) {
        return mix64(std::uint64_t(key) ^ (std::uint64_t(member)<<24) ^ (std::uint64_t(purpose)<<56));
    };
    const double angle=unit_from_bits(bits(1))*6.28318530717958647692;
    const double distance=std::sqrt(unit_from_bits(bits(2)))*s.density/100;
    return {std::cos(angle)*distance*s.aspect/100,std::sin(angle)*distance,
        .35+.65*unit_from_bits(bits(3))};
}
inline void make_cloud_circles(const ParticleCloudStyle& s,std::uint32_t key,std::span<CloudCircle> destination) noexcept {
    for(std::uint32_t i=0;i<destination.size();++i)destination[i]=cloud_circle(s,key,i);
}
}
