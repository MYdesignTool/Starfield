#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"

#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Random.hpp"
#include "SpriteGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace starfield::core {
namespace {

// Host 16-bpc worlds carry channels in [0, 32768] (AE's PF_MAX_CHAN16), not 65535.
constexpr float kMaxChannel16 = 32768.0f;
constexpr std::uint64_t kCancellationParticleInterval = 64;

// Core buffers are byte-order explicit (r, g, b, a) with native byte order for
// multi-byte channels; see ADR 0003.
std::uint16_t load_u16(const std::byte* source) noexcept {
    std::uint16_t value = 0;
    std::memcpy(&value, source, sizeof(value));
    return value;
}

void store_u16(std::byte* destination, std::uint16_t value) noexcept {
    std::memcpy(destination, &value, sizeof(value));
}

float load_f32(const std::byte* source) noexcept {
    float value = 0.0f;
    std::memcpy(&value, source, sizeof(value));
    return value;
}

void store_f32(std::byte* destination, float value) noexcept {
    std::memcpy(destination, &value, sizeof(value));
}

std::uint8_t encode8(float value) noexcept {
    const float bounded = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint8_t>(bounded * 255.0f + 0.5f);
}

std::uint16_t encode16(float value) noexcept {
    const float bounded = std::clamp(value, 0.0f, 1.0f);
    return static_cast<std::uint16_t>(bounded * kMaxChannel16 + 0.5f);
}

using namespace sprite_geometry;

bool encode_region(const std::vector<float>& accumulation, std::uint32_t roi_width, std::uint32_t roi_height,
                   PixelFormat format, AlphaMode alpha_mode, std::uint32_t row_bytes,
                   std::vector<std::byte>& destination, const Cancellation& cancellation) noexcept {
    const std::uint32_t pixel_bytes = bytes_per_pixel(format);
    for (std::uint32_t y = 0; y < roi_height; ++y) {
        if (cancellation.is_cancelled()) return false;
        const float* row = accumulation.data() + static_cast<std::size_t>(y) * roi_width * 4;
        std::byte* out_row = destination.data() + static_cast<std::size_t>(y) * row_bytes;
        for (std::uint32_t x = 0; x < roi_width; ++x) {
            const float* pixel = row + static_cast<std::size_t>(x) * 4;
            std::byte* out = out_row + static_cast<std::size_t>(x) * pixel_bytes;
            float red = pixel[0];
            float green = pixel[1];
            float blue = pixel[2];
            if (alpha_mode == AlphaMode::straight) {
                if (pixel[3] > 0.0f) {
                    red /= pixel[3];
                    green /= pixel[3];
                    blue /= pixel[3];
                } else {
                    red = green = blue = 0.0f;
                }
            }
            switch (format) {
                case PixelFormat::rgba8:
                    out[0] = static_cast<std::byte>(encode8(red));
                    out[1] = static_cast<std::byte>(encode8(green));
                    out[2] = static_cast<std::byte>(encode8(blue));
                    out[3] = static_cast<std::byte>(encode8(pixel[3]));
                    break;
                case PixelFormat::rgba16:
                    store_u16(out + 0, encode16(red));
                    store_u16(out + 2, encode16(green));
                    store_u16(out + 4, encode16(blue));
                    store_u16(out + 6, encode16(pixel[3]));
                    break;
                case PixelFormat::rgba32f:
                    store_f32(out + 0, red);
                    store_f32(out + 4, green);
                    store_f32(out + 8, blue);
                    store_f32(out + 12, pixel[3]);
                    break;
            }
        }
    }
    return true;
}

} // namespace

Result<RenderOutput> CpuParticleRenderer::render(const RenderRequest& request,
                                                const Cancellation& cancellation) const {
    using OutputResult = Result<RenderOutput>;

    const auto validated = validate_frame(request.frame);
    if (!validated.has_value()) {
        return OutputResult::failure(validated.error());
    }
    const FrameSpec& frame = validated.value();
    if (!valid_camera(request.camera)) return OutputResult::failure(ErrorCode::invalid_request, "invalid camera projection");
    const RectI roi = frame.region_of_interest;

    RenderOutput output;
    output.region = roi;
    output.format = frame.format;
    output.color_space = frame.color_space;
    output.alpha_mode = frame.alpha_mode;

    if (roi.empty()) {
        // Nothing to draw: an empty region is a legal request in AE.
        return OutputResult::success(std::move(output));
    }

    if (cancellation.is_cancelled()) {
        return OutputResult::failure(ErrorCode::cancelled, "render cancelled before staging allocation");
    }

    const auto roi_width = static_cast<std::uint32_t>(roi.width());
    const auto roi_height = static_cast<std::uint32_t>(roi.height());
    const auto row_bytes = checked_row_bytes(roi_width, frame.format);
    if (!row_bytes.has_value()) {
        return OutputResult::failure(ErrorCode::unsupported_format, "output row size is not supported");
    }
    const auto total_bytes = checked_buffer_bytes(*row_bytes, roi_height);
    if (!total_bytes.has_value()) {
        return OutputResult::failure(ErrorCode::invalid_request, "output buffer size overflows");
    }

    const std::uint64_t accumulation_values =
        static_cast<std::uint64_t>(roi_width) * static_cast<std::uint64_t>(roi_height) * 4u;
    std::vector<float> accumulation;
    try {
        accumulation.assign(static_cast<std::size_t>(accumulation_values), 0.0f);
    } catch (const std::bad_alloc&) {
        return OutputResult::failure(ErrorCode::allocation_failed, "accumulation buffer allocation failed");
    }

    const auto particles = [&]() -> Result<std::vector<ParticleInstance>> {
        const EmitterDimensionContext dimension_context{
            static_cast<double>(frame.layer_height), frame.pixel_aspect_ratio};
        if (request.graph) {
            auto evaluated = evaluate_particle_graph(*request.graph, frame.time, cancellation,
                                                     dimension_context);
            if (!evaluated.has_value()) return Result<std::vector<ParticleInstance>>::failure(evaluated.error());
            return Result<std::vector<ParticleInstance>>::success(std::move(evaluated.take_value().particles));
        }
        return simulate_particles(request.settings, to_seconds(frame.time), cancellation,
                                  dimension_context);
    }();
    if (!particles.has_value()) {
        return OutputResult::failure(particles.error());
    }

    const PixelGrid grid = make_grid(frame);
    std::uint64_t sprite_pixels = 0;

    double preview_chance = 100;
    if (request.graph) for (const auto& node : request.graph->nodes) if (node.type_key == graph_keys::kOutputNode) {
        bool enabled = false;
        double chance = 100;
        for (const auto& p : node.parameters) {
            if (p.key == graph_keys::kPreviewEnabled) enabled = std::get<std::uint32_t>(p.value) != 0;
            if (p.key == graph_keys::kPreviewChance) chance = std::get<double>(p.value);
        }
        if (enabled) preview_chance = chance;
    }
    std::vector<Sprite> sprites;
    try {
        sprites.reserve(particles.value().size());
        for (std::size_t i = 0; i < particles.value().size(); ++i) {
            if (i % kCancellationParticleInterval == 0 && cancellation.is_cancelled())
                return OutputResult::failure(ErrorCode::cancelled, "cancelled during camera projection");
            if (preview_chance < 100) {
                const auto& particle = particles.value()[i];
                std::uint64_t identity = particle.id;
                for (auto byte : particle.emitter_id.value.bytes) identity = mix64(identity ^ byte);
                if (unit_value(0, identity, RandomPurpose::preview_chance) * 100 >= preview_chance) continue;
            }
            Sprite sprite{};
            if (project_sprite(particles.value()[i], request, grid, sprite)) sprites.push_back(sprite);
        }
        if (request.camera.enabled) std::stable_sort(sprites.begin(), sprites.end(), [](const Sprite& a, const Sprite& b) {
            return a.depth > b.depth;
        });
    } catch (const std::bad_alloc&) { return OutputResult::failure(ErrorCode::allocation_failed, "camera sprite allocation failed"); }

    for (std::size_t index = 0; index < sprites.size(); ++index) {
        if ((index % kCancellationParticleInterval) == 0 && cancellation.is_cancelled()) {
            return OutputResult::failure(ErrorCode::cancelled, "cancelled during sprite rasterization");
        }

        const auto& sprite = sprites[index];
        const ParticleInstance& particle = *sprite.particle;
        if (!(particle.opacity > 0.0)) continue;
        const double radius = 0.5 * particle.size_pixels;
        if (!(radius > 0.0)) {
            continue; // a zero-size particle is invisible by contract
        }

        const double pixel_x = sprite.x - roi.left, pixel_y = sprite.y - roi.top;
        const double radius_x = particle.shape==1?std::abs(sprite.ax)+std::abs(sprite.bx):std::hypot(sprite.ax,sprite.bx);
        const double radius_y = particle.shape==1?std::abs(sprite.ay)+std::abs(sprite.by):std::hypot(sprite.ay,sprite.by);
        if (!(radius_x > 0.0) || !(radius_y > 0.0)) {
            continue;
        }

        const auto left = static_cast<std::int64_t>(std::clamp(std::floor(pixel_x - radius_x - 1.0), 0.0, double(roi_width)));
        const auto top = static_cast<std::int64_t>(std::clamp(std::floor(pixel_y - radius_y - 1.0), 0.0, double(roi_height)));
        const auto right = static_cast<std::int64_t>(std::clamp(std::ceil(pixel_x + radius_x + 1.0), 0.0, double(roi_width)));
        const auto bottom = static_cast<std::int64_t>(std::clamp(std::ceil(pixel_y + radius_y + 1.0), 0.0, double(roi_height)));
        if (right <= left || bottom <= top) {
            continue; // fully outside the region of interest
        }

        const auto box_pixels = static_cast<std::uint64_t>(right - left) * static_cast<std::uint64_t>(bottom - top);
        if (limits_.max_sprite_pixel_ops != 0 &&
            box_pixels > limits_.max_sprite_pixel_ops - sprite_pixels) {
            return OutputResult::failure(ErrorCode::work_limit_exceeded,
                                         "sprite coverage exceeds the bounded work budget");
        }
        sprite_pixels += box_pixels;

        // A one-pixel-wide analytic coverage ramp on the rim keeps edges stable
        // without any neighborhood or random sampling.
        const double determinant = sprite.ax * sprite.by - sprite.ay * sprite.bx;
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12) continue;
        const double edge_scale = std::min(radius_x, radius_y);

        for (std::int64_t y = top; y < bottom; ++y) {
            if (cancellation.is_cancelled()) {
                return OutputResult::failure(ErrorCode::cancelled, "cancelled during sprite scan");
            }
            const double dy = static_cast<double>(y) + 0.5 - pixel_y;
            float* row = accumulation.data() + static_cast<std::size_t>(y) * roi_width * 4;
            for (std::int64_t x = left; x < right; ++x) {
                const double dx = static_cast<double>(x) + 0.5 - pixel_x;
                const double delta_x = (dx * sprite.by - dy * sprite.bx) / determinant;
                const double delta_y = (dy * sprite.ax - dx * sprite.ay) / determinant;
                const double coverage=sprite_coverage(particle,delta_x,delta_y,edge_scale);
                if (coverage <= 0.0) {
                    continue;
                }

                const auto alpha = static_cast<float>(coverage * particle.opacity);
                if (!(alpha > 0.0f)) {
                    continue;
                }
                float* pixel = row + static_cast<std::size_t>(x) * 4;
                const float remaining = 1.0f - alpha;
                pixel[0] = static_cast<float>(particle.color.x) * alpha + pixel[0] * remaining;
                pixel[1] = static_cast<float>(particle.color.y) * alpha + pixel[1] * remaining;
                pixel[2] = static_cast<float>(particle.color.z) * alpha + pixel[2] * remaining;
                pixel[3] = alpha + pixel[3] * remaining;
            }
        }
    }

    try {
        output.row_bytes = *row_bytes;
        output.pixels.assign(*total_bytes, std::byte{0});
    } catch (const std::bad_alloc&) {
        return OutputResult::failure(ErrorCode::allocation_failed, "output buffer allocation failed");
    }
    if (!encode_region(accumulation, roi_width, roi_height, frame.format, frame.alpha_mode, *row_bytes,
                       output.pixels, cancellation)) {
        return OutputResult::failure(ErrorCode::cancelled, "render cancelled while encoding output pixels");
    }

    return OutputResult::success(std::move(output));
}

} // namespace starfield::core
