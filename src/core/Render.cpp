#include "starfield/core/Render.hpp"

#include <cmath>
#include <limits>

namespace starfield::core {
namespace {

// Bounds for host-supplied geometry. They keep every downstream allocation and
// loop provably bounded; see ADR 0005.
constexpr std::uint32_t kMaxFrameDimension = 32768;
constexpr std::uint64_t kMaxStagingBytes = 1ull << 31; // 2 GiB

} // namespace

std::optional<std::uint32_t> checked_row_bytes(std::uint32_t width, PixelFormat format) noexcept {
    const std::uint32_t pixel_bytes = bytes_per_pixel(format);
    if (pixel_bytes == 0 || width == 0) {
        return std::nullopt;
    }
    const std::uint64_t total = static_cast<std::uint64_t>(width) * pixel_bytes;
    if (total == 0 || total > kMaxStagingBytes || total > std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(total);
}

std::optional<std::size_t> checked_buffer_bytes(std::uint32_t row_bytes, std::uint32_t height) noexcept {
    if (row_bytes == 0 || height == 0) {
        return std::nullopt;
    }
    const std::uint64_t total = static_cast<std::uint64_t>(row_bytes) * height;
    if (total == 0 || total > kMaxStagingBytes) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(total);
}

Result<FrameSpec> validate_frame(FrameSpec frame) noexcept {
    if (frame.layer_width == 0 || frame.layer_height == 0 || frame.frame_width == 0 || frame.frame_height == 0) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_request, "zero layer or frame dimension");
    }
    if (frame.layer_width > kMaxFrameDimension || frame.layer_height > kMaxFrameDimension ||
        frame.frame_width > kMaxFrameDimension || frame.frame_height > kMaxFrameDimension) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_request, "frame dimension above the supported maximum");
    }

    const RectI frame_grid{0, 0, static_cast<std::int32_t>(frame.frame_width),
                           static_cast<std::int32_t>(frame.frame_height)};
    if (!contains(frame_grid, frame.region_of_interest)) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_request, "region of interest outside the frame grid");
    }

    if (!std::isfinite(frame.pixel_aspect_ratio) || frame.pixel_aspect_ratio <= 0.0) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_request, "invalid pixel aspect ratio");
    }
    if (bytes_per_pixel(frame.format) == 0) {
        return Result<FrameSpec>::failure(ErrorCode::unsupported_format, "unknown pixel format");
    }

    const auto time = make_rational(frame.time.value, frame.time.scale);
    if (!time.has_value()) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_time, "frame time is not a valid rational");
    }
    const auto duration = make_rational(frame.frame_duration.value, frame.frame_duration.scale);
    if (!duration.has_value() || duration->value <= 0) {
        return Result<FrameSpec>::failure(ErrorCode::invalid_time, "frame duration must be a positive rational");
    }

    frame.time = *time;
    frame.frame_duration = *duration;
    return Result<FrameSpec>::success(frame);
}

} // namespace starfield::core
