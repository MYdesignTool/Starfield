#pragma once

#include "starfield/core/Error.hpp"
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace starfield::core {
class Cancellation;
struct EvaluatedGraph;

enum class TextureTimeMode : std::uint32_t {
    current_time, play_once, loop, stretch, random_still, random_once, random_loop, freeze_frame
};
enum class TextureColorUse : std::uint32_t { source, alpha, lightness };

// Shared by particles from the same authored node. Resource zero is None.
struct ParticleTextureStyle {
    std::uint32_t front{}, back{};
    TextureTimeMode time_mode{TextureTimeMode::current_time};
    TextureColorUse color_use{TextureColorUse::source};
    std::uint32_t use_ratio{1}, ignore_perspective{};
    friend bool operator==(const ParticleTextureStyle&, const ParticleTextureStyle&) = default;
};

struct TextureSource {
    std::uint32_t resource_id{};
    double start_seconds{}, end_seconds{}, frame_seconds{};
    std::uint32_t width{}, height{};
    double pixel_aspect_ratio{1};
};

// Immutable numeric staging data, borrowed for one render call. Pixels are
// premultiplied RGBA32F in the same working space as the generated output.
struct TextureFrameView {
    std::uint32_t resource_id{}, frame_index{}, width{}, height{}, row_floats{};
    std::span<const float> pixels;
};
struct TextureFrameRequest {
    std::uint32_t resource_id{}, frame_index{};
    double seconds{};
};

inline constexpr std::uint32_t kMaxTextureSources = 128;
inline constexpr std::uint32_t kMaxTextureFrames = 4096;
inline constexpr std::uint32_t kMaxTextureStyles = 4096;
inline constexpr std::uint64_t kMaxTextureBytes = 512ull * 1024 * 1024;

[[nodiscard]] bool valid_texture_style(const ParticleTextureStyle&) noexcept;
[[nodiscard]] Result<std::uint32_t> texture_frame_count(const TextureSource&) noexcept;
[[nodiscard]] Result<std::uint32_t> texture_frame_index(const TextureSource&, TextureTimeMode,
    double render_seconds, double age_seconds, double lifetime_seconds, std::uint32_t random_key) noexcept;
[[nodiscard]] Result<std::vector<TextureFrameRequest>> plan_texture_frames(const EvaluatedGraph&,
    std::span<const TextureSource>, double render_seconds, const Cancellation&);
[[nodiscard]] Result<bool> validate_texture_resources(std::span<const TextureSource>,
    std::span<const TextureFrameView>, const Cancellation&);
// Call after validate_texture_resources has accepted the layout/storage.
[[nodiscard]] std::array<float, 4> sample_texture(const TextureFrameView&, double u, double v) noexcept;
[[nodiscard]] std::array<float, 4> color_texture(std::array<float, 4>, TextureColorUse,
    std::array<double, 3> color, double opacity) noexcept;
} // namespace starfield::core
