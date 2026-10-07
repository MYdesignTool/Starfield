#pragma once

namespace starfield::adapter::native_nodes::transform_layout {
inline constexpr int inherit=1,anchor_xy=2,anchor_z=3,position=4,rotation=7,scale=10;
inline constexpr int particles_scale=13,particles_opacity=14,last=14;
// Synthetic binding fields, not native parameters. Twelve pixel affine entries
// are read through main-effect numeric aliases on the render-safe playback path.
inline constexpr int matrix_first=15,matrix_last=26;
inline constexpr bool animated(int index) noexcept {return index>=2 && index<=matrix_last;}
inline constexpr bool matrix_field(int index) noexcept {return index>=matrix_first && index<=matrix_last;}
}
