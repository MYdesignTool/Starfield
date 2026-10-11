#pragma once
#include "ParticleLayout.hpp"
namespace starfield::adapter::native_nodes::motion_layout {
inline constexpr int mode=1,origin_type=2,origin_xy=3,origin_z=4,angle=5,offset=8,speed=11,speed_random=12;
inline constexpr int curve=13,last=curve+particle_layout::curve_span-1;
constexpr int disk_id(int index) noexcept {
    if(index>=1&&index<=12)return 1700+index;
    if(index==curve)return 1713;
    if(index==last)return 1714;
    if(index>curve&&index<last)return (index-curve-1)%2==0?3900+(index-curve-1)/2:4100+(index-curve-2)/2;
    return 0;
}
constexpr bool animated(int index) noexcept{return index>=2&&index<=12;}
constexpr int curve_base(int index) noexcept{return index>=curve&&index<=last?curve:0;}
static_assert(last==142);
}
