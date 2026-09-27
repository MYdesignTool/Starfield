#pragma once

#include "starfield/core/Settings.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <variant>
#include <vector>

namespace starfield::core {

struct RationalTime {
    std::int64_t value{};
    std::int64_t scale{1};
};

enum class PixelFormat : std::uint8_t {
    rgba8,
    rgba16,
    rgba32f,
};

enum class ColorSpace : std::uint8_t {
    ae_working_space,
    linear_srgb,
    srgb,
};

enum class AlphaMode : std::uint8_t {
    straight,
    premultiplied,
};

struct RectI {
    std::int32_t left{};
    std::int32_t top{};
    std::int32_t right{};
    std::int32_t bottom{};
};

struct FrameSpec {
    std::uint32_t width{};
    std::uint32_t height{};
    RationalTime time;
    RationalTime frame_duration;
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    RectI region_of_interest{};
    double pixel_aspect_ratio{1.0};
    double downsample_x{1.0};
    double downsample_y{1.0};
    std::uint32_t quality{};
};

struct PixelBuffer {
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t row_bytes{};
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    std::vector<std::byte> pixels;
};

// Immutable input assembled by the AE adapter during pre-render. It owns no AE
// handles, suites, PF_ParamDef pointers, or host pixel pointers.
struct RenderRequest {
    ValidatedSettings settings;
    FrameSpec frame;
    std::uint64_t graph_revision{};
    // Optional immutable, adapter-owned checkout. The AE world itself is never
    // retained past the selector that supplied it.
    std::shared_ptr<const PixelBuffer> source;
};

struct RenderError {
    enum class Code : std::uint8_t {
        invalid_frame,
        allocation_failed,
        unsupported_format,
        backend_failure,
        cancelled,
    } code{};
};

template <typename T>
class Result {
public:
    static Result success(T value) { return Result(Storage{std::in_place_type<T>, std::move(value)}); }
    static Result failure(RenderError error) { return Result(Storage{std::in_place_type<RenderError>, error}); }

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] T&& take_value() { return std::move(std::get<T>(storage_)); }
    [[nodiscard]] const RenderError& error() const { return std::get<RenderError>(storage_); }

private:
    using Storage = std::variant<T, RenderError>;
    explicit Result(Storage storage) : storage_(std::move(storage)) {}

    Storage storage_;
};

struct RenderOutput {
    // The core owns its staging buffer. The AE adapter copies/composites this
    // into the checked-out output world while that host buffer is valid.
    std::uint32_t width{};
    std::uint32_t height{};
    std::uint32_t row_bytes{};
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    std::vector<std::byte> pixels;
};

class Renderer {
public:
    virtual ~Renderer() = default;
    [[nodiscard]] virtual Result<RenderOutput> render(const RenderRequest& request) const = 0;
};

} // namespace starfield::core
