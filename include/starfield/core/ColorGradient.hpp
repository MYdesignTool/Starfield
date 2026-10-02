#pragma once
#include "starfield/core/Graph.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>

namespace starfield::core {
struct ColorStop { double position{};Vec3 color{1,1,1}; };
struct ColorGradient { std::array<ColorStop,8> stops{};std::uint8_t count{2}; };
inline ColorGradient white_gradient() {
    ColorGradient result;result.stops[0]={0,{1,1,1}};result.stops[1]={1,{1,1,1}};return result;
}
inline bool valid_color_gradient(const ColorGradient& gradient) {
    if(gradient.count<2 || gradient.count>8) return false;
    for(std::size_t i=0;i<gradient.count;++i) {
        const auto& stop=gradient.stops[i];
        if(!std::isfinite(stop.position) || stop.position<0 || stop.position>1 ||
            (i && stop.position<=gradient.stops[i-1].position)) return false;
        for(double value:{stop.color.x,stop.color.y,stop.color.z})
            if(!std::isfinite(value) || value<0 || value>kMaxParticleColor) return false;
    }
    return gradient.stops[0].position==0 && gradient.stops[gradient.count-1].position==1;
}
inline OpaqueBytes encode_color_gradient(const ColorGradient& gradient) {
    if(!valid_color_gradient(gradient)) return {};
    OpaqueBytes bytes{std::byte{1},static_cast<std::byte>(gradient.count),std::byte{0},std::byte{0}};
    for(std::size_t i=0;i<gradient.count;++i) for(double value:{gradient.stops[i].position,
        gradient.stops[i].color.x,gradient.stops[i].color.y,gradient.stops[i].color.z}) {
        const auto bits=std::bit_cast<std::uint64_t>(value);
        for(unsigned b=0;b<8;++b) bytes.push_back(static_cast<std::byte>((bits>>(8*b))&255));
    }
    return bytes;
}
inline bool decode_color_gradient(const OpaqueBytes& bytes,ColorGradient& gradient) {
    if(bytes.size()<4 || bytes[0]!=std::byte{1} || bytes[2]!=std::byte{0} || bytes[3]!=std::byte{0}) return false;
    gradient.count=std::to_integer<std::uint8_t>(bytes[1]);
    if(gradient.count<2 || gradient.count>8 || bytes.size()!=4+32*gradient.count) return false;
    std::size_t at=4;
    for(std::size_t i=0;i<gradient.count;++i) for(double* value:{&gradient.stops[i].position,
        &gradient.stops[i].color.x,&gradient.stops[i].color.y,&gradient.stops[i].color.z}) {
        std::uint64_t bits=0;for(unsigned b=0;b<8;++b) bits|=std::uint64_t(std::to_integer<unsigned char>(bytes[at++]))<<(8*b);
        *value=std::bit_cast<double>(bits);
    }
    return valid_color_gradient(gradient);
}
inline Vec3 evaluate_color_gradient(const ColorGradient& gradient,double position) {
    position=std::clamp(position,0.0,1.0);
    for(std::size_t i=1;i<gradient.count;++i) if(position<=gradient.stops[i].position) {
        const auto& a=gradient.stops[i-1];const auto& b=gradient.stops[i];
        const double f=(position-a.position)/(b.position-a.position);
        return {a.color.x+(b.color.x-a.color.x)*f,a.color.y+(b.color.y-a.color.y)*f,a.color.z+(b.color.z-a.color.z)*f};
    }
    return gradient.stops[gradient.count-1].color;
}
} // namespace starfield::core
