#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Geometry.hpp"
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

} // namespace starfield::core
