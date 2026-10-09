#pragma once

namespace starfield::adapter::native_nodes::model_layout {
// ADR0038: reserved for the new, unpublished Model effect. Input occupies0.
inline constexpr int source=1,import_obj=2,mesh=3,revision=4;
inline constexpr int origin=5,rotation=8,scale=11;
inline constexpr int flip_x=14,flip_y=15,flip_z=16,center=17,normalize=18,last=18;
inline constexpr int disk_first=1501;
// Numeric constants in binding7 only; not native parameter indices.
inline constexpr int bounds_first=19,bounds_last=24;
// Appended derived author values, after unchanged metadata/guard94.
inline constexpr int author_bounds_first=95,author_bounds_last=100,author_bounds_disk_first=1519;
[[nodiscard]] constexpr int author_bounds_disk_id(int index) noexcept {
    return index>=author_bounds_first&&index<=author_bounds_last?author_bounds_disk_first+index-author_bounds_first:0;
}
[[nodiscard]] constexpr bool binding_field(int index) noexcept {
    return index==source || index==revision || (index>=origin && index<=bounds_last);
}
[[nodiscard]] constexpr int disk_id(int index) noexcept {
    return index>=source && index<=last ? disk_first+index-1 : 0;
}
[[nodiscard]] constexpr bool animated(int index) noexcept {return index>=origin && index<=last;}
static_assert(disk_id(source)==1501 && disk_id(mesh)==1503 && disk_id(last)==1518);
}
