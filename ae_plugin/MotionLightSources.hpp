#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <span>
#include <vector>

namespace starfield::core { class Cancellation; }
namespace starfield::adapter {
inline constexpr std::size_t kMaxMotionLightSources=256;
inline constexpr std::size_t kMaxMotionLightNameUnits=4096;
struct MotionLightSource {
    std::uint32_t layer_id{};
    std::uint32_t comp_index{}; // zero-based composition order, not a persistent identity
    std::uint32_t light_type{}; // AEGP_LightType; only point lights drive Light Path
    std::u16string name;
    bool video_active{};
};
struct MotionLightSourceInventoryStats {
    std::uint32_t composition_layers{};
    std::uint32_t av_layers{};
    std::uint32_t light_layers{};
    std::uint32_t point_light_layers{};
    std::uint32_t camera_layers{};
    std::uint32_t other_layers{};
};
// Read-only composition inventory at the caller's PF time. Names, identities
// and AE light type are owned values. The caller decides Starting With filtering,
// inclusion of inactive lights, order and target/path construction. No host handle persists.
// Failure/cancellation, including cleanup failure, preserves output and stats.
[[nodiscard]] PF_Err capture_motion_light_sources(PF_InData*,AEGP_PluginID,
    const core::Cancellation&,std::vector<MotionLightSource>& output,
    MotionLightSourceInventoryStats* stats=nullptr) noexcept;
// Starting With is a name-prefix filter (official Staff reference). The
// independent matcher compares UTF-16 units exactly; empty prefix selects all.
// Keep inventory order and include inactive sources in this selection seam;
// the caller owns the separate activity/target/path policy. No host calls.
[[nodiscard]] PF_Err select_motion_light_source_ids(std::span<const MotionLightSource>,
    std::u16string_view prefix,std::vector<std::uint32_t>& output) noexcept;
// Light Path accepts only point lights after applying the exact UTF-16 prefix
// match. Other Motion source policies continue to use the generic selector.
[[nodiscard]] PF_Err select_motion_point_light_source_ids(std::span<const MotionLightSource>,
    std::u16string_view prefix,std::vector<std::uint32_t>& output) noexcept;
// Strict UTF-8 graph values cross into the AE adapter's UTF-16 layer-name
// matcher. Invalid UTF-8, embedded NUL and overlong prefixes leave output intact.
[[nodiscard]] PF_Err motion_light_prefix_to_utf16(std::string_view,
    std::u16string& output) noexcept;
}
