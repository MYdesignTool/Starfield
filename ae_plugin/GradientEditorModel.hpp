#pragma once
#include "starfield/core/ColorGradient.hpp"
#include <algorithm>
#include <cmath>
#include <span>

namespace starfield::adapter::gradient_editor {
using core::ColorGradient;
constexpr double gap=0.001;
inline bool valid(const ColorGradient& value) noexcept {
    if(value.count<2 || value.count>8 || static_cast<unsigned>(value.interpolation)>1)return false;
    for(unsigned i=0;i<value.count;++i) {
        const auto& stop=value.stops[i];
        if(!std::isfinite(stop.position) || stop.position<0 || stop.position>1 ||
           (i && stop.position<=value.stops[i-1].position))return false;
        for(double channel:{stop.color.x,stop.color.y,stop.color.z})
            if(!std::isfinite(channel) || channel<0 || channel>1)return false;
    }
    return true;
}
enum class PixelOrder {bgra,argb};
inline bool rasterize_opaque32(const ColorGradient& value,unsigned width,unsigned height,
                              PixelOrder order,std::span<std::uint8_t> pixels,unsigned row_stride=0) noexcept {
    if(!valid(value) || width<2 || width>220 || !height || height>62)return false;
    if(!row_stride)row_stride=width*4;
    if(row_stride<width*4 || row_stride>880 || pixels.size()!=std::size_t(row_stride)*height)return false;
    std::fill_n(pixels.data(),row_stride,std::uint8_t{0});
    for(unsigned x=0;x<width;++x) {
        const auto rgb=core::evaluate_color_gradient(value,double(x)/(width-1));
        const std::array<std::uint8_t,3> color{static_cast<std::uint8_t>(std::lround(rgb.x*255)),
            static_cast<std::uint8_t>(std::lround(rgb.y*255)),static_cast<std::uint8_t>(std::lround(rgb.z*255))};
        if(order==PixelOrder::bgra) {
            pixels[x*4]=color[2];pixels[x*4+1]=color[1];pixels[x*4+2]=color[0];pixels[x*4+3]=255;
        } else {
            pixels[x*4]=255;pixels[x*4+1]=color[0];pixels[x*4+2]=color[1];pixels[x*4+3]=color[2];
        }
    }
    for(unsigned y=1;y<height;++y)std::copy_n(pixels.data(),row_stride,pixels.data()+y*row_stride);
    return true;
}
inline int insert(ColorGradient& value,double position) noexcept {
    if(!valid(value) || !std::isfinite(position) || value.count==8)return -1;
    position=std::clamp(position,0.0,1.0);
    unsigned index=0;
    for(unsigned i=0;i<value.count;++i) {
        if(std::abs(position-value.stops[i].position)<gap-1e-12)return -1;
        if(value.stops[i].position<position)++index;
    }
    const auto color=core::evaluate_color_gradient(value,position);
    for(unsigned j=value.count;j>index;--j)value.stops[j]=value.stops[j-1];
    value.stops[index]={position,color};++value.count;return static_cast<int>(index);
}
inline int move(ColorGradient& value,int index,double position) noexcept {
    if(!valid(value) || !std::isfinite(position) || index<0 || index>=value.count)return -1;
    const auto stop=value.stops[index];auto remaining=value;
    for(unsigned i=static_cast<unsigned>(index);i+1<remaining.count;++i)remaining.stops[i]=remaining.stops[i+1];
    --remaining.count;position=std::clamp(position,0.0,1.0);
    double closest=2,chosen=stop.position;
    const auto consider=[&](double candidate) {
        if(candidate<0 || candidate>1)return;
        for(unsigned i=0;i<remaining.count;++i)
            if(std::abs(candidate-remaining.stops[i].position)<gap-1e-12)return;
        if(const auto distance=std::abs(candidate-position);distance<closest){closest=distance;chosen=candidate;}
    };
    consider(position);consider(0);consider(1);
    for(unsigned i=0;i<remaining.count;++i){consider(remaining.stops[i].position-gap);consider(remaining.stops[i].position+gap);}
    if(closest==2)return -1;
    unsigned next=0;while(next<remaining.count && remaining.stops[next].position<chosen)++next;
    for(unsigned i=remaining.count;i>next;--i)remaining.stops[i]=remaining.stops[i-1];
    remaining.stops[next]={chosen,stop.color};++remaining.count;value=remaining;return static_cast<int>(next);
}
inline bool erase(ColorGradient& value,int index) noexcept {
    if(!valid(value) || value.count<=2 || index<0 || index>=value.count)return false;
    for(unsigned j=static_cast<unsigned>(index);j+1<value.count;++j)value.stops[j]=value.stops[j+1];
    --value.count;return true;
}
inline void flip(ColorGradient& value) noexcept {
    if(!valid(value))return;
    std::reverse(value.stops.begin(),value.stops.begin()+value.count);
    for(unsigned i=0;i<value.count;++i)value.stops[i].position=1-value.stops[i].position;
}
inline ColorGradient preset(unsigned index) noexcept {
    ColorGradient value;
    if(index==0) {value.count=2;value.stops[0]={0,{1,1,1}};value.stops[1]={1,{1,1,1}};}
    else if(index==1) {value.count=4;value.stops[0]={0,{1,0.12,0.02}};value.stops[1]={0.35,{1,0.6,0.05}};
        value.stops[2]={0.7,{1,0.9,0.3}};value.stops[3]={1,{1,1,0.9}};}
    else {value.count=5;value.stops[0]={0,{1,0.4,0.08}};value.stops[1]={0.25,{0.08,0.6,0.4}};
        value.stops[2]={0.5,{0.5,1,0.7}};value.stops[3]={0.75,{0.92,1,0.62}};value.stops[4]={1,{0.78,0.62,0.35}};}
    return value;
}
}
