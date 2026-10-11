#pragma once
namespace starfield::adapter::native_nodes::material_layout {
inline constexpr int color_source=1,color=2,roughness=3,metallic=4,opacity=5,lights=6,last=6;
inline constexpr int disk_first=1601;
constexpr int disk_id(int index) noexcept{return index>=1&&index<=last?disk_first+index-1:0;}
constexpr bool animated(int index) noexcept{return index>=1&&index<=last;}
}
