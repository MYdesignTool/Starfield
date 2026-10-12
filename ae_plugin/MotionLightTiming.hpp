#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "MotionPointCapture.hpp"
#include "starfield/core/MotionLightPathResource.hpp"
#include "starfield/core/Time.hpp"
#include <cstddef>
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
struct MotionLightPathTrack {
    MotionLightTiming timing;
    // AE-evaluated source positions at the authored key times and adaptive
    // subdivision times. Adjacent samples are linearized to the caller's
    // requested probe tolerance; this remains an owned numeric snapshot.
    std::vector<MotionTimedLayerPoint> samples;
    double maximum_probe_error{};
};
struct MotionLightPathTrackSet {
    // One track per matched source, in the caller's composition order. The total
    // sample count is bounded across the whole set, not reset for each light.
    std::vector<MotionLightPathTrack> tracks;
    std::size_t sample_count{};
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
// Samples the host-evaluated position curve independently for one point light.
// Each accepted segment is checked at its 1/4, 1/2 and 3/4 times; intervals over
// max_chord_error are bisected until bounded by that tolerance or the sample cap.
[[nodiscard]] PF_Err capture_motion_light_path_track(PF_InData*,AEGP_PluginID,
    std::uint32_t source_layer_id,core::LayerUnits,double max_chord_error,
    const core::Cancellation&,MotionLightPathTrack& output) noexcept;
// Captures all explicitly selected matching lights independently. IDs are kept
// in caller order, must be unique, and share one aggregate sample bound. Failure
// or cancellation leaves output unchanged.
[[nodiscard]] PF_Err capture_motion_light_path_tracks(PF_InData*,AEGP_PluginID,
    std::span<const std::uint32_t> source_layer_ids,core::LayerUnits,double max_chord_error,
    const core::Cancellation&,MotionLightPathTrackSet& output) noexcept;
// Converts the owned AE snapshot to the host-independent SFPL1 graph resource.
// Encoded bytes are replaced only after the complete set validates and encodes.
[[nodiscard]] PF_Err encode_motion_light_path_track_set(const MotionLightPathTrackSet&,
    const core::Cancellation&,std::vector<std::byte>& output) noexcept;
}
