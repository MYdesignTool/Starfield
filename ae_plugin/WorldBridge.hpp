#pragma once

// Conversion boundary between host pixel worlds and owned core buffers. Host
// pointers are only touched here, while the corresponding checkout is valid.

#include "AEConfig.h"
#include "AE_Effect.h"

#include "starfield/core/Render.hpp"

#include <cstdint>
#include <span>

namespace starfield::adapter {

// Bit depth the host drives this render in (PF_SmartRenderInput::bitdepth):
// 8, 16 or 32 bits per channel. Anything else is rejected, never guessed.
enum class HostBitDepth : std::uint8_t {
    bpc8,
    bpc16,
    bpc32,
};

[[nodiscard]] bool classify_bit_depth(short bit_depth, HostBitDepth& out) noexcept;
[[nodiscard]] starfield::core::PixelFormat pixel_format_for(HostBitDepth depth) noexcept;

// Geometry of a checked-out host world in render-resolution layer coordinates.
// `origin_x`/`origin_y` are the world's own origin fields, which AE documents as
// valid for smart-effect checkouts.
struct WorldLayout {
    std::int32_t origin_x{0};
    std::int32_t origin_y{0};
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t row_bytes{0};
};

[[nodiscard]] bool describe_world(const PF_EffectWorld& world, WorldLayout& out) noexcept;

// Host scalar helpers shared by the render path and the diagnostics readout.
// - `rational_scale_value` converts a PF_RationalScale (num/den) with a fallback.
// - `host_pixel_aspect_ratio` normalizes in_data->pixel_aspect_ratio.
//
// There is deliberately no helper that derives the render-resolution layer grid
// from in_data->downsample_x/y: the SDK documents the factor inconsistently (see
// ADR 0005), so render geometry comes from observed checked-out worlds instead.
[[nodiscard]] double rational_scale_value(const PF_RationalScale& scale, double fallback) noexcept;
[[nodiscard]] double host_pixel_aspect_ratio(const PF_InData& in_data) noexcept;

// Copies the whole checked-out world into an owned, tightly packed core buffer
// (premultiplied RGBA in the host's channel scale). The host pointer is not
// retained. Fails with unsupported_format when the host layout cannot be
// represented and allocation_failed when the bounded allocation fails.
[[nodiscard]] starfield::core::Result<starfield::core::PixelBuffer> read_world(const PF_EffectWorld& world,
                                                                             HostBitDepth depth,
                                                                             const starfield::core::Cancellation& cancellation) noexcept;

// Clears the host output world to transparent black, then copies the core staging
// buffer into it, translating channel order and clipping to the destination extent.
// The output format must match the host bit depth; a mismatch is a bug and returns false.
struct OutputView {
    starfield::core::RectI region{};
    std::uint32_t row_bytes{0};
    starfield::core::PixelFormat format{starfield::core::PixelFormat::rgba8};
    std::span<const std::byte> pixels;
    [[nodiscard]] std::uint32_t width() const noexcept {
        return static_cast<std::uint32_t>(region.width());
    }
    [[nodiscard]] std::uint32_t height() const noexcept {
        return static_cast<std::uint32_t>(region.height());
    }
};

[[nodiscard]] bool write_output(OutputView output, const WorldLayout& destination,
                               PF_EffectWorld& world, HostBitDepth depth,
                               const starfield::core::Cancellation& cancellation) noexcept;
[[nodiscard]] bool write_output(const starfield::core::RenderOutput& output, const WorldLayout& destination,
                               PF_EffectWorld& world, HostBitDepth depth,
                               const starfield::core::Cancellation& cancellation) noexcept;

} // namespace starfield::adapter
