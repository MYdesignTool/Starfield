#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "MotionPointCapture.hpp"
#include "starfield/core/Time.hpp"
#include <cstdint>
#include <vector>

namespace starfield::core { class Cancellation; }
namespace starfield::adapter {
struct MotionLightTiming {
    std::uint32_t layer_id{};
    std::uint64_t position_keyframes{}; // sum across separated followers
    core::RationalTime first_keyframe{},last_keyframe{}; // composition time
    bool separated{};
    // Optional union of actual CompTime keyframe instants across XYZ. Requesting
    // this list is bounded; the default metadata-only path remains allocation-free.
    std::vector<core::RationalTime> keyframe_times;
};
struct MotionLightKeyframes {
    MotionLightTiming timing;
    // AE-evaluated point-light position at each original CompTime keyframe.
    // This is keyframe data only; it does not approximate between-key interpolation.
    std::vector<MotionTimedLayerPoint> positions;
};
// Read-only metadata for one explicit point light, not a path author or render
// ABI. Zero keys leaves the default times; equal endpoints do not define a
// duration. All SDK ownership is callback-local. Failure preserves output.
[[nodiscard]] PF_Err capture_motion_light_timing(PF_InData*,AEGP_PluginID,
    std::uint32_t source_layer_id,const core::Cancellation&,MotionLightTiming& output,
    bool include_keyframe_times=false) noexcept;
// Captures one explicit point light's keyframe-time union and samples its layer
// origin at each of those original composition times. Output is atomic and owned.
[[nodiscard]] PF_Err capture_motion_light_keyframes(PF_InData*,AEGP_PluginID,
    std::uint32_t source_layer_id,core::LayerUnits,const core::Cancellation&,
    MotionLightKeyframes& output) noexcept;
}
