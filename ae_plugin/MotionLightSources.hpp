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
    std::u16string name;
    bool video_active{};
};
// Read-only composition inventory at the caller's PF time. Names and identities
// are owned copies. The caller decides Starting With filtering, inclusion of
// inactive lights, order and target/path construction. No host handle persists.
// Failure/cancellation, including cleanup failure, preserves output.
[[nodiscard]] PF_Err capture_motion_light_sources(PF_InData*,AEGP_PluginID,
    const core::Cancellation&,std::vector<MotionLightSource>& output) noexcept;
// Starting With is a name-prefix filter (official Staff reference). The
// independent matcher compares UTF-16 units exactly; empty prefix selects all.
// Keep inventory order and include inactive sources in this selection seam;
// the caller owns the separate activity/target/path policy. No host calls.
[[nodiscard]] PF_Err select_motion_light_source_ids(std::span<const MotionLightSource>,
    std::u16string_view prefix,std::vector<std::uint32_t>& output) noexcept;
}
