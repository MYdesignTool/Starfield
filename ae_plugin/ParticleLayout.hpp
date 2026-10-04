#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter::native_nodes::particle_layout {
inline constexpr A_long shape=1, life=2, life_random=3, properties=4,
    size=5, size_y=6, size_random=7, opacity=8, opacity_random=9,
    color_mode=10, color=11, gradient=12, gradient_first=13,
    feather=29, up_axis=30, properties_end=31, over_life=32,
    size_over_life=33, opacity_over_life=34, size_curve=35, opacity_curve=52,
    over_life_end=69, rotation=70, orient=71, angle=72, angle_random=75,
    random_limit=76, limit_angle=77, speed=78, speed_random=81,
    rotation_curve=82, anchor_x=99, anchor_y=100, limit_2d=101, rotation_end=102;
inline constexpr A_long last=rotation_end;
constexpr bool animated(A_long index) noexcept {
    return (index>=shape && index<=life_random) || (index>=size && index<=color) ||
        (index>=feather && index<=up_axis) || (index>=size_over_life && index<=opacity_over_life) ||
        (index>=orient && index<=speed_random) || (index>=anchor_x && index<=limit_2d);
}
}
