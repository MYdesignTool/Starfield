#pragma once

#include "ParticleLayout.hpp"
#include <cstdint>

namespace starfield::adapter::native_nodes::turbulence_layout {
inline constexpr int mode=1,axis=2,affect=3,affected_properties=4,randomize=5;
inline constexpr int origin_xy=6,origin_z=7,position_offset=8,scale_offset=9,color_offset=10,
    opacity_offset=11,rotation_offset=12;
inline constexpr int curve=13,curve_last=curve+particle_layout::curve_span-1;
inline constexpr int noise_scale=curve_last+1,noise_levels=noise_scale+1,noise_amplitude=noise_scale+2,
    noise_frequency=noise_scale+3,fractal_speed=noise_scale+4,fractal_evolution=noise_scale+5,
    fractal_offset=noise_scale+6,sphere_size=noise_scale+9,sphere_feather=noise_scale+10,last=sphere_feather;

constexpr int disk_id(int index) noexcept {
    if(index>=mode && index<=rotation_offset)return 1800+index;
    if(index==curve)return 1813;
    if(index==curve_last)return 1814;
    if(index>curve && index<curve_last)
        return (index-curve-1)%2==0?4300+(index-curve-1)/2:4400+(index-curve-2)/2;
    if(index>=noise_scale && index<=last)return 1802+index;
    return 0;
}

constexpr bool animated(int index) noexcept {
    return (index>=randomize && index<=rotation_offset) ||
        (index>=noise_scale && index<=last);
}
constexpr int curve_base(int index) noexcept{return index>=curve&&index<=curve_last?curve:0;}
static_assert(curve_last==142 && noise_scale==143 && last==153);
}
