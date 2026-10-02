#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include "starfield/core/Time.hpp"

#include <cstddef>
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// Host-independent render contract (ADR 0005). A backend receives one immutable
// RenderRequest and returns an owned staging buffer or a typed CoreError. The AE
// adapter owns every host pointer and checkout; nothing in this header refers to
// AE types, handles, suites, or process-global state.

namespace starfield::core {

struct Graph;

// Byte order in memory is always r, g, b, a with 1/2/4 bytes per channel. The AE
// adapter converts to and from the host's a, r, g, b pixel structs.
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

// Host quality hint. Draft frames are allowed to differ from full-quality frames,
// but a given request must always produce identical output (ADR 0002).
enum class Quality : std::uint8_t {
    draft,
    full,
};

[[nodiscard]] constexpr std::uint32_t bytes_per_pixel(PixelFormat format) noexcept {
    switch (format) {
        case PixelFormat::rgba8:
            return 4;
        case PixelFormat::rgba16:
            return 8;
        case PixelFormat::rgba32f:
            return 16;
    }
    return 0;
}

// Half-open rectangle: [left, right) x [top, bottom). Negative origins are legal
// so the contract does not depend on layers being anchored at (0, 0).
struct RectI {
    std::int32_t left{0};
    std::int32_t top{0};
    std::int32_t right{0};
    std::int32_t bottom{0};

    [[nodiscard]] constexpr bool empty() const noexcept { return right <= left || bottom <= top; }
    [[nodiscard]] constexpr std::int64_t width() const noexcept { return static_cast<std::int64_t>(right) - left; }
    [[nodiscard]] constexpr std::int64_t height() const noexcept { return static_cast<std::int64_t>(bottom) - top; }
};

[[nodiscard]] constexpr RectI intersect(RectI a, RectI b) noexcept {
    if (a.empty()) {
        return a;
    }
    if (b.empty()) {
        return b;
    }
    RectI result{a.left > b.left ? a.left : b.left,
                 a.top > b.top ? a.top : b.top,
                 a.right < b.right ? a.right : b.right,
                 a.bottom < b.bottom ? a.bottom : b.bottom};
    if (result.right < result.left) {
        result.right = result.left;
    }
    if (result.bottom < result.top) {
        result.bottom = result.top;
    }
    return result;
}

[[nodiscard]] constexpr RectI unite(RectI a, RectI b) noexcept {
    if (a.empty()) {
        return b;
    }
    if (b.empty()) {
        return a;
    }
    return RectI{a.left < b.left ? a.left : b.left,
                 a.top < b.top ? a.top : b.top,
                 a.right > b.right ? a.right : b.right,
                 a.bottom > b.bottom ? a.bottom : b.bottom};
}

[[nodiscard]] constexpr bool contains(RectI outer, RectI inner) noexcept {
    if (inner.empty()) {
        return true;
    }
    return !outer.empty() && inner.left >= outer.left && inner.top >= outer.top && inner.right <= outer.right &&
           inner.bottom <= outer.bottom;
}

struct FrameSpec {
    // Full-resolution layer size. This is the world-space reference for canonical
    // coordinates: one world unit is one layer height (ADR 0003).
    std::uint32_t layer_width{0};
    std::uint32_t layer_height{0};
    // Full render-resolution pixel grid: the layer grid scaled by the host
    // downsample factor. World-to-pixel mapping is expressed in this grid.
    std::uint32_t frame_width{0};
    std::uint32_t frame_height{0};
    // Sub-rect of the frame grid this render must fill. May be empty, in which
    // case the renderer produces an empty output without allocating.
    RectI region_of_interest{};
    RationalTime time{};
    RationalTime frame_duration{};
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    // Requested encoding for the generated output pixels; compositing stays premultiplied internally.
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    double pixel_aspect_ratio{1.0};
    Quality quality{Quality::full};
};

// Owned pixel storage with explicit layout. Rows may carry padding; callers may
// only read row_bytes per row, and must never assume tightly packed rows.
struct PixelBuffer {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t row_bytes{0};
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    // Encoding used for the returned pixel bytes.
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    // Placement of pixel (0, 0) inside FrameSpec's frame grid.
    std::int32_t origin_x{0};
    std::int32_t origin_y{0};
    std::vector<std::byte> pixels;
};

// Immutable render input assembled by the adapter. It owns no host handles, no
// PF_ParamDef pointers, and no host pixel pointers. This effect generates particles
// over transparent black; its layer input provides geometry and controls only.
struct RenderRequest {
    ValidatedSettings settings;
    FrameSpec frame;
    // When present, the graph is validated/evaluated at frame.time and takes
    // precedence over `settings`. An absent graph preserves the flat-settings
    // compatibility path while AE sequence persistence is integrated.
    std::shared_ptr<const Graph> graph;
    // Stable caller-owned revision key reserved for derived render caches.
    std::uint64_t graph_revision{0};
    struct Camera {
        bool enabled{false};
        // Row-vector layer-pixel -> view transform; +Z is in front of camera.
        std::array<double, 16> layer_to_view{};
        // Column-vector homogeneous image-pixel -> output-layer-pixel mapping.
        // Removes AE's subsequent layer transform, including 3D perspective.
        std::array<double, 9> image_to_layer{};
        double focal_x{1.0}, focal_y{1.0};
        double center_x{0.0}, center_y{0.0};
        double near_clip{0.01};
    } camera;
};

struct RenderOutput {
    // Region of the frame grid covered by pixels, and therefore the output's
    // placement. Empty output means "nothing to draw" (empty ROI).
    RectI region{};
    std::uint32_t row_bytes{0};
    PixelFormat format{PixelFormat::rgba8};
    ColorSpace color_space{ColorSpace::ae_working_space};
    AlphaMode alpha_mode{AlphaMode::premultiplied};
    // The core owns its staging buffer. The adapter copies it into the host
    // output world while the host buffer and checkouts are valid.
    std::vector<std::byte> pixels;

    [[nodiscard]] std::uint32_t width() const noexcept { return static_cast<std::uint32_t>(region.width()); }
    [[nodiscard]] std::uint32_t height() const noexcept { return static_cast<std::uint32_t>(region.height()); }
};

// Cooperative cancellation. Core loops poll this at bounded intervals; the AE
// adapter implements it with the host's abort callback.
class Cancellation {
public:
    virtual ~Cancellation() = default;
    [[nodiscard]] virtual bool is_cancelled() const noexcept = 0;
};

class NeverCancelled final : public Cancellation {
public:
    [[nodiscard]] bool is_cancelled() const noexcept override { return false; }
};

class Renderer {
public:
    virtual ~Renderer() = default;
    [[nodiscard]] virtual Result<RenderOutput> render(const RenderRequest& request,
                                                     const Cancellation& cancellation) const = 0;
};

// Rejects structurally invalid geometry, oversized dimensions, non-positive
// rational scale, and unknown pixel formats, and returns the normalized frame.
[[nodiscard]] Result<FrameSpec> validate_frame(FrameSpec frame) noexcept;

// Overflow-checked layout arithmetic used before any allocation. Row size is
// bounded by the maximum frame dimension, so it always fits in 32 bits; whole
// buffer sizes are reported as size_t.
[[nodiscard]] std::optional<std::uint32_t> checked_row_bytes(std::uint32_t width, PixelFormat format) noexcept;
[[nodiscard]] std::optional<std::size_t> checked_buffer_bytes(std::uint32_t row_bytes, std::uint32_t height) noexcept;

} // namespace starfield::core
