#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "starfield/core/Settings.hpp"

namespace starfield::adapter::native_nodes::particle_layout {
inline constexpr A_long curve_points=static_cast<A_long>(core::kMaxAgeCurvePoints);
inline constexpr A_long curve_span=2*curve_points+2;
constexpr A_long curve_interpolation(A_long count) noexcept {return count+curve_span-1;}
inline constexpr A_long shape=1, life=2, life_random=3, properties=4,
    size=5, size_y=6, size_random=7, opacity=8, opacity_random=9,
    color_mode=10, color=11, gradient=12, gradient_first=13,
    gradient_interpolation=29, feather=30, up_axis=31, properties_end=32, over_life=33,
    size_over_life=34, opacity_over_life=35, size_curve=36, opacity_curve=size_curve+curve_span,
    over_life_end=opacity_curve+curve_span, rotation=over_life_end+1, orient=rotation+1,
    angle=orient+1, angle_random=angle+3, random_limit=angle_random+1, limit_angle=random_limit+1,
    speed=limit_angle+1, speed_random=speed+3, rotation_curve=speed_random+1,
    anchor_x=rotation_curve+curve_span, anchor_y=anchor_x+1, limit_2d=anchor_y+1, rotation_end=limit_2d+1;
inline constexpr A_long last=rotation_end;
// Appended after legacy metadata (443..518); never shift UUID/connection streams.
inline constexpr A_long transfer=519;
constexpr A_long curve_base(A_long index) noexcept {
    for(auto base:{size_curve,opacity_curve,rotation_curve})
        if(index>=base && index<base+curve_span)return base;
    return 0;
}
constexpr bool animated(A_long index) noexcept {
    return index==transfer || (index>=shape && index<=life_random) || (index>=size && index<=color) ||
        (index>=feather && index<=up_axis) || (index>=size_over_life && index<=opacity_over_life) ||
        (index>=orient && index<=speed_random) || (index>=anchor_x && index<=limit_2d);
}
static_assert(last==442);
}
