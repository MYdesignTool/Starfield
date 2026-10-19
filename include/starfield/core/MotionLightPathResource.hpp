#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/MotionLightPathClock.hpp"
#include "starfield/core/Time.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace starfield::core {
class Cancellation;

inline constexpr std::size_t kMaxMotionLightPathSources=256;
inline constexpr std::size_t kMaxMotionLightPathResourceSamples=16385;
inline constexpr std::size_t kMotionLightPathResourceHeaderBytes=24;
inline constexpr std::size_t kMotionLightPathTrackHeaderBytes=16;
inline constexpr std::size_t kMotionLightPathSampleBytes=40;

struct MotionLightPathResourceSample {
    RationalTime source_comp_time{};
    Vec3 position{};
};
struct MotionLightPathSourceTrack {
    std::uint32_t source_layer_id{};
    std::vector<MotionLightPathResourceSample> samples;
    double maximum_probe_error{};
};
struct MotionLightPathResource {
    std::vector<MotionLightPathSourceTrack> tracks;
    std::size_t sample_count{};
};
struct MotionLightPathTrackSample {
    Vec3 position{};
    Vec3 velocity_per_source_second{};
    Vec3 tangent{};
    bool continued{};
};
struct MotionLightPathSettings {
    MotionLightPathClockSettings clock{};
    // With Delay Random=0, every source uses the fixed Max Delay value.
    double max_delay_seconds{0.5};
    double delay_random_percent{};
    bool orient_to_path{};
    Vec3 forward{1,0,0};
};
struct MotionLightPathEvaluation {
    // Each matching light contributes its own position(time)-position(first).
    // The per-source contributions are then summed in composition order.
    Vec3 displacement{};
    Vec3 velocity{};
    Vec3 combined_direction{};
    std::size_t source_count{};
    std::size_t active_source_count{};
};

// SFPL1 carries only owned rational times, canonical positions, and composition-
// local source IDs. The surrounding graph record owns its checksum/identity.
[[nodiscard]] Result<std::vector<std::byte>> encode_motion_light_path_resource(
    const MotionLightPathResource&,const Cancellation&) noexcept;
[[nodiscard]] Result<MotionLightPathResource> decode_motion_light_path_resource(
    std::span<const std::byte>,const Cancellation&) noexcept;

class CompiledMotionLightPathTrack {
public:
    [[nodiscard]] std::uint32_t source_layer_id() const noexcept { return source_layer_id_; }
    [[nodiscard]] double first_source_time() const noexcept { return times_.empty()?0:times_.front(); }
    [[nodiscard]] double last_source_time() const noexcept { return times_.empty()?0:times_.back(); }
    [[nodiscard]] Vec3 first_position() const noexcept { return positions_.empty()?Vec3{}:positions_.front(); }
    [[nodiscard]] std::size_t sample_count() const noexcept { return times_.size(); }
    [[nodiscard]] std::size_t storage_bytes() const noexcept {
        return sizeof(*this)+times_.capacity()*sizeof(double)+positions_.capacity()*sizeof(Vec3);
    }
    // Piecewise-linear playback interpolates the AE-evaluated adaptive snapshot.
    // Continue distance/rate are source seconds and source seconds per particle
    // second, respectively; zero values provide a stopped endpoint.
    [[nodiscard]] Result<MotionLightPathTrackSample> sample(
        double source_time_seconds,double continuation_seconds=0,
        double continuation_rate=0) const noexcept;
private:
    std::uint32_t source_layer_id_{};
    std::vector<double> times_;
    std::vector<Vec3> positions_;
    friend Result<CompiledMotionLightPathTrack> compile_motion_light_path_track(
        const MotionLightPathSourceTrack&,const Cancellation&) noexcept;
};

[[nodiscard]] Result<CompiledMotionLightPathTrack> compile_motion_light_path_track(
    const MotionLightPathSourceTrack&,const Cancellation&) noexcept;

class CompiledMotionLightPath {
public:
    [[nodiscard]] const MotionLightPathSettings& settings() const noexcept { return settings_; }
    [[nodiscard]] std::size_t source_count() const noexcept { return sources_.size(); }
    [[nodiscard]] std::size_t storage_bytes() const noexcept;
    [[nodiscard]] Result<MotionLightPathEvaluation> sample(double age_seconds,
        double lifetime_seconds,double speed_random_sample,const Cancellation&) const noexcept;
    [[nodiscard]] Result<MotionLightPathEvaluation> sample(double age_seconds,
        double lifetime_seconds,double effective_delay_seconds,double speed_random_sample,
        const Cancellation&) const noexcept;
    [[nodiscard]] Result<MotionLightPathEvaluation> sample_at_source_elapsed(
        double elapsed_source_seconds,double source_time_rate,MotionPathEndBehavior,
        const Cancellation&) const noexcept;
private:
    struct Source {
        CompiledMotionLightPathTrack track;
        CompiledMotionLightPathClock clock;
        Source(CompiledMotionLightPathTrack path,CompiledMotionLightPathClock source_clock):
            track(std::move(path)),clock(std::move(source_clock)){}
    };
    MotionLightPathSettings settings_{};
    std::vector<Source> sources_;
    friend Result<CompiledMotionLightPath> compile_motion_light_path(
        const MotionLightPathResource&,const MotionLightPathSettings&,const Cancellation&) noexcept;
};

[[nodiscard]] Result<CompiledMotionLightPath> compile_motion_light_path(
    const MotionLightPathResource&,const MotionLightPathSettings&,const Cancellation&) noexcept;

} // namespace starfield::core
