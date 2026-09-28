#include "WorldBridge.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

// Host 16-bpc channels are scaled to PF_MAX_CHAN16 (32768) and 8-bpc to 255;
// 32-bpc worlds carry normalized floats. Core buffers use the same scales, so
// these conversions are byte-order translations only. See ADR 0003.
namespace starfield::adapter {
namespace {

namespace core = starfield::core;

} // namespace

double rational_scale_value(const PF_RationalScale& scale, double fallback) noexcept {
    if (scale.den == 0) {
        return fallback;
    }
    const double value = static_cast<double>(scale.num) / static_cast<double>(scale.den);
    if (!std::isfinite(value) || value <= 0.0) {
        return fallback;
    }
    return value;
}

double host_pixel_aspect_ratio(const PF_InData& in_data) noexcept {
    return rational_scale_value(in_data.pixel_aspect_ratio, 1.0);
}

bool classify_bit_depth(short bit_depth, HostBitDepth& out) noexcept {
    switch (bit_depth) {
        case 8:
            out = HostBitDepth::bpc8;
            return true;
        case 16:
            out = HostBitDepth::bpc16;
            return true;
        case 32:
            out = HostBitDepth::bpc32;
            return true;
        default:
            return false;
    }
}

core::PixelFormat pixel_format_for(HostBitDepth depth) noexcept {
    switch (depth) {
        case HostBitDepth::bpc8:
            return core::PixelFormat::rgba8;
        case HostBitDepth::bpc16:
            return core::PixelFormat::rgba16;
        case HostBitDepth::bpc32:
            return core::PixelFormat::rgba32f;
    }
    return core::PixelFormat::rgba8;
}

bool describe_world(const PF_EffectWorld& world, WorldLayout& out) noexcept {
    if (world.data == nullptr || world.width <= 0 || world.height <= 0 || world.rowbytes <= 0) {
        return false;
    }
    const auto width = static_cast<std::uint32_t>(world.width);
    const auto height = static_cast<std::uint32_t>(world.height);
    const auto row_bytes = static_cast<std::uint32_t>(world.rowbytes);
    if (row_bytes < width) {
        return false; // one byte per pixel is the absolute minimum any depth needs
    }

    out.origin_x = static_cast<std::int32_t>(world.origin_x);
    out.origin_y = static_cast<std::int32_t>(world.origin_y);
    out.width = width;
    out.height = height;
    out.row_bytes = row_bytes;
    return true;
}

core::Result<core::PixelBuffer> read_world(const PF_EffectWorld& world, HostBitDepth depth,
                                           const core::Cancellation& cancellation) noexcept {
    using BufferResult = core::Result<core::PixelBuffer>;

    WorldLayout layout{};
    if (!describe_world(world, layout)) {
        return BufferResult::failure(core::ErrorCode::invalid_request, "host world layout is invalid");
    }

    const core::PixelFormat format = pixel_format_for(depth);
    const std::uint32_t pixel_bytes = core::bytes_per_pixel(format);
    const auto row_bytes = core::checked_row_bytes(layout.width, format);
    if (!row_bytes.has_value()) {
        return BufferResult::failure(core::ErrorCode::unsupported_format, "host world width is not supported");
    }
    if (layout.row_bytes < *row_bytes) {
        return BufferResult::failure(core::ErrorCode::unsupported_format,
                                     "host world rows are narrower than one pixel row");
    }
    if (cancellation.is_cancelled()) {
        return BufferResult::failure(core::ErrorCode::cancelled, "render cancelled before copying the source world");
    }
    const auto total_bytes = core::checked_buffer_bytes(*row_bytes, layout.height);
    if (!total_bytes.has_value()) {
        return BufferResult::failure(core::ErrorCode::invalid_request, "host world size overflows");
    }

    core::PixelBuffer buffer;
    buffer.width = layout.width;
    buffer.height = layout.height;
    buffer.row_bytes = *row_bytes;
    buffer.format = format;
    buffer.color_space = core::ColorSpace::ae_working_space;
    buffer.alpha_mode = core::AlphaMode::premultiplied;
    buffer.origin_x = layout.origin_x;
    buffer.origin_y = layout.origin_y;
    try {
        buffer.pixels.assign(*total_bytes, std::byte{0});
    } catch (const std::bad_alloc&) {
        return BufferResult::failure(core::ErrorCode::allocation_failed, "source buffer allocation failed");
    }

    const auto* source = reinterpret_cast<const std::byte*>(world.data);
    for (std::uint32_t y = 0; y < layout.height; ++y) {
        if (cancellation.is_cancelled()) {
            return BufferResult::failure(core::ErrorCode::cancelled, "render cancelled while copying the source world");
        }
        const std::byte* source_row = source + static_cast<std::size_t>(y) * layout.row_bytes;
        std::byte* destination_row = buffer.pixels.data() + static_cast<std::size_t>(y) * *row_bytes;
        for (std::uint32_t x = 0; x < layout.width; ++x) {
            const std::byte* source_pixel = source_row + static_cast<std::size_t>(x) * pixel_bytes;
            std::byte* destination_pixel = destination_row + static_cast<std::size_t>(x) * pixel_bytes;
            switch (depth) {
                case HostBitDepth::bpc8: {
                    PF_Pixel pixel{};
                    std::memcpy(&pixel, source_pixel, sizeof(pixel));
                    destination_pixel[0] = static_cast<std::byte>(pixel.red);
                    destination_pixel[1] = static_cast<std::byte>(pixel.green);
                    destination_pixel[2] = static_cast<std::byte>(pixel.blue);
                    destination_pixel[3] = static_cast<std::byte>(pixel.alpha);
                    break;
                }
                case HostBitDepth::bpc16: {
                    PF_Pixel16 pixel{};
                    std::memcpy(&pixel, source_pixel, sizeof(pixel));
                    // Both sides already use the 0..32768 channel scale.
                    std::memcpy(destination_pixel + 0, &pixel.red, sizeof(pixel.red));
                    std::memcpy(destination_pixel + 2, &pixel.green, sizeof(pixel.green));
                    std::memcpy(destination_pixel + 4, &pixel.blue, sizeof(pixel.blue));
                    std::memcpy(destination_pixel + 6, &pixel.alpha, sizeof(pixel.alpha));
                    break;
                }
                case HostBitDepth::bpc32: {
                    PF_PixelFloat pixel{};
                    std::memcpy(&pixel, source_pixel, sizeof(pixel));
                    std::memcpy(destination_pixel + 0, &pixel.red, sizeof(pixel.red));
                    std::memcpy(destination_pixel + 4, &pixel.green, sizeof(pixel.green));
                    std::memcpy(destination_pixel + 8, &pixel.blue, sizeof(pixel.blue));
                    std::memcpy(destination_pixel + 12, &pixel.alpha, sizeof(pixel.alpha));
                    break;
                }
            }
        }
    }

    return BufferResult::success(std::move(buffer));
}

bool write_output(OutputView output, const WorldLayout& destination, PF_EffectWorld& world,
                  HostBitDepth depth, const core::Cancellation& cancellation) noexcept {
    const core::PixelFormat format = pixel_format_for(depth);
    if (output.format != format || world.data == nullptr || world.width <= 0 || world.height <= 0 ||
        world.rowbytes <= 0 ||
        destination.width != static_cast<std::uint32_t>(world.width) ||
        destination.height != static_cast<std::uint32_t>(world.height) ||
        destination.row_bytes != static_cast<std::uint32_t>(world.rowbytes)) {
        return false;
    }
    const std::uint32_t pixel_bytes = core::bytes_per_pixel(format);
    const auto destination_bytes = core::checked_row_bytes(destination.width, format);
    if (!destination_bytes.has_value() || destination.row_bytes < *destination_bytes) {
        return false;
    }

    std::uint32_t output_min_row_bytes = 0;
    if (output.region.empty()) {
        if (!output.pixels.empty()) return false;
    } else {
        if (output.pixels.empty()) return false;
        const auto rows = core::checked_row_bytes(output.width(), format);
        if (!rows.has_value() || output.row_bytes < *rows) {
            return false;
        }
        const auto storage_bytes = core::checked_buffer_bytes(output.row_bytes, output.height());
        if (!storage_bytes.has_value() || output.pixels.size() < *storage_bytes) {
            return false;
        }
        output_min_row_bytes = static_cast<std::uint32_t>(*rows);
        if (destination.width < output.width() || destination.row_bytes < output_min_row_bytes) {
            return false;
        }
    }

    // AE may hand the effect an output world already seeded with the input layer.
    // A particle pass owns the whole result: clear it first so pixels outside the
    // sparse sprite staging buffer are transparent instead of retaining a solid.
    auto* destination_pixels = reinterpret_cast<std::byte*>(world.data);
    for (std::uint32_t y = 0; y < destination.height; ++y) {
        if (cancellation.is_cancelled()) return false;
        std::memset(destination_pixels + static_cast<std::size_t>(y) * destination.row_bytes, 0,
                    static_cast<std::size_t>(*destination_bytes));
    }

    if (output.region.empty()) {
        return true; // an empty render is transparent black
    }

    for (std::uint32_t y = 0; y < output.height(); ++y) {
        if (cancellation.is_cancelled()) return false;
        const auto frame_y = static_cast<std::int64_t>(output.region.top) + y;
        const auto local_y = frame_y - destination.origin_y;
        if (local_y < 0 || local_y >= static_cast<std::int64_t>(destination.height)) {
            continue;
        }
        std::byte* destination_row =
            destination_pixels + static_cast<std::size_t>(local_y) * destination.row_bytes;
        const std::byte* source_row = output.pixels.data() + static_cast<std::size_t>(y) * output.row_bytes;

        for (std::uint32_t x = 0; x < output.width(); ++x) {
            const auto frame_x = static_cast<std::int64_t>(output.region.left) + x;
            const auto local_x = frame_x - destination.origin_x;
            if (local_x < 0 || local_x >= static_cast<std::int64_t>(destination.width)) {
                continue;
            }
            const std::byte* source_pixel = source_row + static_cast<std::size_t>(x) * pixel_bytes;
            std::byte* destination_pixel = destination_row + static_cast<std::size_t>(local_x) * pixel_bytes;

            switch (depth) {
                case HostBitDepth::bpc8: {
                    PF_Pixel pixel{};
                    pixel.red = static_cast<A_u_char>(source_pixel[0]);
                    pixel.green = static_cast<A_u_char>(source_pixel[1]);
                    pixel.blue = static_cast<A_u_char>(source_pixel[2]);
                    pixel.alpha = static_cast<A_u_char>(source_pixel[3]);
                    std::memcpy(destination_pixel, &pixel, sizeof(pixel));
                    break;
                }
                case HostBitDepth::bpc16: {
                    PF_Pixel16 pixel{};
                    std::memcpy(&pixel.red, source_pixel + 0, sizeof(pixel.red));
                    std::memcpy(&pixel.green, source_pixel + 2, sizeof(pixel.green));
                    std::memcpy(&pixel.blue, source_pixel + 4, sizeof(pixel.blue));
                    std::memcpy(&pixel.alpha, source_pixel + 6, sizeof(pixel.alpha));
                    std::memcpy(destination_pixel, &pixel, sizeof(pixel));
                    break;
                }
                case HostBitDepth::bpc32: {
                    PF_PixelFloat pixel{};
                    std::memcpy(&pixel.red, source_pixel + 0, sizeof(pixel.red));
                    std::memcpy(&pixel.green, source_pixel + 4, sizeof(pixel.green));
                    std::memcpy(&pixel.blue, source_pixel + 8, sizeof(pixel.blue));
                    std::memcpy(&pixel.alpha, source_pixel + 12, sizeof(pixel.alpha));
                    std::memcpy(destination_pixel, &pixel, sizeof(pixel));
                    break;
                }
            }
        }
    }
    return true;
}

bool write_output(const core::RenderOutput& output, const WorldLayout& destination, PF_EffectWorld& world,
                  HostBitDepth depth, const core::Cancellation& cancellation) noexcept {
    return write_output(OutputView{output.region, output.row_bytes, output.format,
                                   std::span<const std::byte>(output.pixels.data(), output.pixels.size())},
                        destination, world, depth, cancellation);
}

} // namespace starfield::adapter
