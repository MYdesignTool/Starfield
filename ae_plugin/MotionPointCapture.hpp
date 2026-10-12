#pragma once
#include "AE_Effect.h"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/Time.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace starfield::core { class Cancellation; }
namespace starfield::adapter {
inline constexpr std::size_t kMaxMotionPointRequests=256;
// Stable, composition-local layer identity and an explicit point in that layer's
// pixel coordinates. The caller owns target/filter/anchor/time policies.
struct MotionLayerPointRequest {
    std::uint32_t layer_id{};
    core::Vec3 local_pixels{};
};
struct MotionLayerPoint {
    std::uint32_t layer_id{};
    core::Vec3 position{};
};
struct MotionTimedLayerPointRequest {
    std::uint32_t layer_id{};
    core::Vec3 local_pixels{};
    core::RationalTime source_comp_time{};
};
struct MotionTimedLayerPoint {
    std::uint32_t layer_id{};
    core::Vec3 position{};
    core::RationalTime source_comp_time{}; // owned, normalized requested time
};
// Read-only adapter seam, not a new author or render ABI. Output contains numbers
// only, in caller order; a failed/cancelled capture leaves it unchanged. Matrices
// are sampled once per unique source at the comp time converted from PF_InData.
[[nodiscard]] PF_Err capture_motion_layer_points(PF_InData*,core::LayerUnits,
    std::span<const MotionLayerPointRequest>,const core::Cancellation&,
    std::vector<MotionLayerPoint>& output) noexcept;
// Source world matrices use each requested composition time; the owner frame
// uses current PF→comp time. Numeric-only output, same bounded/atomic contract.
[[nodiscard]] PF_Err capture_motion_timed_layer_points(PF_InData*,core::LayerUnits,
    std::span<const MotionTimedLayerPointRequest>,const core::Cancellation&,
    std::vector<MotionTimedLayerPoint>& output) noexcept;
}
