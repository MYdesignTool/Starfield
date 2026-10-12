#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/Time.hpp"
#include "starfield/core/MotionCurveClock.hpp"
#include <utility>

namespace starfield::core {

// Exactly the two end choices requested for Light Path. The continuation output
// is elapsed source-clock time beyond the last key; the caller applies the
// captured terminal direction to that value.
enum class MotionPathEndBehavior : std::uint8_t { stop=0, continue_direction=1 };

struct MotionLightPathClockSettings {
    // Percent of the source light's original keyframe time. 100 means that one
    // second of particle travel advances the source by one second.
    double speed_percent{100};
    double speed_random_percent{};
    AgeCurve over_life{};
    MotionPathEndBehavior end_behavior{MotionPathEndBehavior::stop};
};

struct MotionLightPathClockSample {
    // Time to query this particular light in its original AE animation.
    double source_time_seconds{};
    // Source-time seconds per particle second while following the keyed motion.
    double source_time_rate{};
    // For Continue, time beyond the last keyframe; multiply by the terminal
    // source velocity to continue in the terminal direction.
    double continuation_seconds{};
    double continuation_rate{};
    bool delayed{};
    bool ended{};
};

// One immutable clock per matching light. Keeping each source range independent
// preserves the owner's per-light timing rule when their keyframe spans differ.
class CompiledMotionLightPathClock {
public:
    [[nodiscard]] double first_keyframe_seconds() const noexcept { return first_seconds_; }
    [[nodiscard]] double last_keyframe_seconds() const noexcept { return last_seconds_; }
    [[nodiscard]] double duration_seconds() const noexcept { return duration_seconds_; }
    [[nodiscard]] Result<MotionLightPathClockSample> sample(
        double age_seconds,double lifetime_seconds,double effective_delay_seconds,
        double speed_random_sample=0) const noexcept;
private:
    CompiledMotionLightPathClock(double first,double last,MotionLightPathClockSettings settings,
        CompiledMotionCurveClock over_life) noexcept:
        first_seconds_(first),last_seconds_(last),duration_seconds_(last-first),
        settings_(settings),over_life_(std::move(over_life)){}
    double first_seconds_{},last_seconds_{},duration_seconds_{};
    MotionLightPathClockSettings settings_{};
    CompiledMotionCurveClock over_life_;
    friend Result<CompiledMotionLightPathClock> compile_motion_light_path_clock(
        RationalTime,RationalTime,const MotionLightPathClockSettings&) noexcept;
};

[[nodiscard]] Result<CompiledMotionLightPathClock> compile_motion_light_path_clock(
    RationalTime first_keyframe,RationalTime last_keyframe,
    const MotionLightPathClockSettings&) noexcept;

} // namespace starfield::core
