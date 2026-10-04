#pragma once
#include "starfield/core/ColorGradient.hpp"
#include <algorithm>
#include <cmath>

namespace starfield::adapter::gradient_editor {
using core::ColorGradient;
constexpr double gap=0.001;
inline bool valid(const ColorGradient& value) noexcept {
    if(value.count<2 || value.count>8 || value.stops[0].position!=0 ||
        value.stops[value.count-1].position!=1)return false;
    for(unsigned i=0;i<value.count;++i) {
        const auto& stop=value.stops[i];
        if(!std::isfinite(stop.position) || (i && stop.position<=value.stops[i-1].position))return false;
        for(double channel:{stop.color.x,stop.color.y,stop.color.z})
            if(!std::isfinite(channel) || channel<0 || channel>1)return false;
    }
    return true;
}
inline int insert(ColorGradient& value,double position) noexcept {
    if(!valid(value) || !std::isfinite(position) || value.count==8)return -1;
    position=std::clamp(position,gap,1-gap);
    for(unsigned i=1;i<value.count;++i)if(position<value.stops[i].position) {
        if(position-value.stops[i-1].position<gap || value.stops[i].position-position<gap)return -1;
        const auto color=core::evaluate_color_gradient(value,position);
        for(unsigned j=value.count;j>i;--j)value.stops[j]=value.stops[j-1];
        value.stops[i]={position,color};++value.count;return static_cast<int>(i);
    }
    return -1;
}
inline bool move(ColorGradient& value,int index,double position) noexcept {
    if(!valid(value) || !std::isfinite(position) || index<=0 || index>=value.count-1)return false;
    const double lo=value.stops[index-1].position+gap,hi=value.stops[index+1].position-gap;
    if(lo>hi)return false;
    value.stops[index].position=std::clamp(position,lo,hi);return true;
}
inline bool erase(ColorGradient& value,int index) noexcept {
    if(!valid(value) || value.count<=2 || index<=0 || index>=value.count-1)return false;
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
