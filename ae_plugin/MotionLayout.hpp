#pragma once
#include "ParticleLayout.hpp"
#include <cstdint>
namespace starfield::adapter::native_nodes::motion_layout {
inline constexpr int mode=1,origin_type=2,origin_xy=3,origin_z=4,angle=5,offset=8,speed=11,speed_random=12;
inline constexpr std::uint32_t core_circle_mode=1,core_light_path_mode=0,core_look_at_mode=2;
constexpr bool core_mode_from_popup(int popup,std::uint32_t& mode_value) noexcept {
    if(popup==1)mode_value=core_circle_mode;
    else if(popup==2)mode_value=core_light_path_mode;
    else if(popup==3)mode_value=core_look_at_mode;
    else return false;
    return true;
}
inline constexpr int curve=13,curve_last=curve+particle_layout::curve_span-1;
inline constexpr int path_prefix_length=curve_last+1,path_prefix_data=path_prefix_length+1,path_prefix_bytes=128;
inline constexpr int path_prefix_chunks=(path_prefix_bytes+2)/3,path_max_delay=path_prefix_data+path_prefix_chunks;
inline constexpr int path_delay_random=path_max_delay+1,path_orient=path_max_delay+2,path_end_behavior=path_max_delay+3,last=path_end_behavior;
constexpr int disk_id(int index) noexcept {
    if(index>=1&&index<=12)return 1700+index;
    if(index==curve)return 1713;
    if(index==curve_last)return 1714;
    if(index>curve&&index<curve_last)return (index-curve-1)%2==0?3900+(index-curve-1)/2:4100+(index-curve-2)/2;
    if(index==path_prefix_length)return 1715;
    if(index>=path_prefix_data&&index<path_prefix_data+path_prefix_chunks)return 1716+index-path_prefix_data;
    if(index>=path_max_delay&&index<=path_end_behavior)return 1760+index-path_max_delay;
    return 0;
}
constexpr bool animated(int index) noexcept{return (index>=2&&index<=12)||index==path_max_delay||index==path_delay_random||index==path_orient;}
constexpr int curve_base(int index) noexcept{return index>=curve&&index<=curve_last?curve:0;}
static_assert(curve_last==142&&path_prefix_chunks==43&&path_max_delay==187&&last==190);
}
