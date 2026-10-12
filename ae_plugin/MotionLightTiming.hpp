#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "starfield/core/Time.hpp"
#include <cstdint>

namespace starfield::core { class Cancellation; }
namespace starfield::adapter {
struct MotionLightTiming {
    std::uint32_t layer_id{};
    std::uint64_t position_keyframes{}; // sum across separated followers
    core::RationalTime first_keyframe{},last_keyframe{}; // composition time
    bool separated{};
};
// Read-only metadata for one explicit point light, not a path author or render
// ABI. Zero keys leaves the default times; equal endpoints do not define a
// duration. All SDK ownership is callback-local. Failure preserves output.
[[nodiscard]] PF_Err capture_motion_light_timing(PF_InData*,AEGP_PluginID,
    std::uint32_t source_layer_id,const core::Cancellation&,MotionLightTiming& output) noexcept;
}
