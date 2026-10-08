#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"

#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Random.hpp"
#include "SpriteGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <map>

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
    const auto resources=validate_texture_resources(request.texture_sources,request.texture_frames,cancellation);
    if (!resources.has_value()) return OutputResult::failure(resources.error());
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

    const auto evaluated = [&]() -> Result<EvaluatedGraph> {
        const EmitterDimensionContext dimension_context{
            static_cast<double>(frame.layer_height), frame.pixel_aspect_ratio};
        if (request.graph) {
            return evaluate_particle_graph(*request.graph, frame.time, cancellation, dimension_context);
        }
        auto simulated=simulate_particles(request.settings, to_seconds(frame.time), cancellation,dimension_context);
        if(!simulated.has_value())return Result<EvaluatedGraph>::failure(simulated.error());
        EvaluatedGraph result;result.particles=simulated.take_value();
        return Result<EvaluatedGraph>::success(std::move(result));
    }();
    if (!evaluated.has_value()) {
        return OutputResult::failure(evaluated.error());
    }
    const auto& particles=evaluated.value().particles;

    std::map<std::uint32_t,const TextureSource*> texture_sources;
    std::map<std::pair<std::uint32_t,std::uint32_t>,const TextureFrameView*> texture_frames;
    try {
        for (const auto& source:request.texture_sources) texture_sources.emplace(source.resource_id,&source);
        for (const auto& texture:request.texture_frames) texture_frames.emplace(std::pair{texture.resource_id,texture.frame_index},&texture);
        if (!evaluated.value().texture_styles.empty()) {
            const auto planned=plan_texture_frames(evaluated.value(),request.texture_sources,to_seconds(frame.time),cancellation);
            if (!planned.has_value()) return OutputResult::failure(planned.error());
            for (const auto& wanted:planned.value()) if (!texture_frames.contains({wanted.resource_id,wanted.frame_index}))
                return OutputResult::failure(ErrorCode::invalid_request,"requested texture frame is missing");
        }
    } catch (const std::bad_alloc&) { return OutputResult::failure(ErrorCode::allocation_failed,"texture lookup allocation failed"); }

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
        sprites.reserve(particles.size());
        for (std::size_t i = 0; i < particles.size(); ++i) {
            if (i % kCancellationParticleInterval == 0 && cancellation.is_cancelled())
                return OutputResult::failure(ErrorCode::cancelled, "cancelled during camera projection");
            if (preview_chance < 100) {
                const auto& particle = particles[i];
                std::uint64_t identity = particle.id;
                for (auto byte : particle.emitter_id.value.bytes) identity = mix64(identity ^ byte);
                if (unit_value(0, identity, RandomPurpose::preview_chance) * 100 >= preview_chance) continue;
            }
            Sprite sprite{};
            const auto& particle=particles[i];
            if (particle.shape==3) {
                const auto& style=evaluated.value().texture_styles[particle.texture_style_index-1];
                if (!style.front && !style.back) continue;
                auto source=texture_sources.at(style.front?style.front:style.back);
                const auto ratio=[&]() {return style.use_ratio ? double(source->width)*source->pixel_aspect_ratio/source->height : 0.;};
                if (!project_sprite(particle,request,grid,sprite,evaluated.value().sprite_bases,ratio(),style.ignore_perspective!=0)) continue;
                if (!sprite.back_facing && !style.front) continue;
                if (sprite.back_facing && style.back && style.back!=style.front) {
                    source=texture_sources.at(style.back);
                    if (!project_sprite(particle,request,grid,sprite,evaluated.value().sprite_bases,ratio(),style.ignore_perspective!=0)) continue;
                }
                auto index=texture_frame_index(*source,style.time_mode,to_seconds(frame.time),particle.age_seconds,
                    particle.lifetime_seconds,particle.texture_random_key);
                if (!index.has_value()) return OutputResult::failure(index.error());
                sprite.texture_frame=texture_frames.at({source->resource_id,index.value()});
                sprite.texture_style=&style;
                sprites.push_back(sprite);
            } else if (project_sprite(particle,request,grid,sprite,evaluated.value().sprite_bases,0,false,evaluated.value().cloud_styles)) sprites.push_back(sprite);
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
        const auto [radius_x,radius_y]=sprite_bounds(sprite);
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

        const auto member_count=sprite.cloud_style ? (sprite.cloud_style->density==0?1u:sprite.cloud_style->circles) : 1u;
        const auto box_pixels = static_cast<std::uint64_t>(right - left) * static_cast<std::uint64_t>(bottom - top)*member_count;
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
        const double edge_scale = sprite.cloud_style ? std::min(std::hypot(sprite.ax,sprite.bx),std::hypot(sprite.ay,sprite.by)) : std::min(radius_x, radius_y);
        std::array<CloudCircle,kMaxCloudCircles> cloud_storage;
        std::span<const CloudCircle> members;
        if(sprite.cloud_style) {
            auto writable=std::span{cloud_storage}.first(member_count);
            make_cloud_circles(*sprite.cloud_style,particle.cloud_random_key,writable);
            members=writable;
        }

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
                if (!std::isfinite(delta_x) || !std::isfinite(delta_y)) continue;
                const double coverage=sprite_coverage(particle,delta_x,delta_y,edge_scale,members);
                if (coverage <= 0.0) {
                    continue;
                }

                std::array<float,4> source{};
                if (sprite.texture_frame) source=color_texture(sample_texture(*sprite.texture_frame,(delta_x+1)*.5,(delta_y+1)*.5),
                    sprite.texture_style->color_use,{particle.color.x,particle.color.y,particle.color.z},coverage*particle.opacity);
                else {
                    const auto a=static_cast<float>(coverage*particle.opacity);
                    source={static_cast<float>(particle.color.x)*a,static_cast<float>(particle.color.y)*a,static_cast<float>(particle.color.z)*a,a};
                }
                const auto alpha = source[3];
                if (!(alpha > 0.0f)) {
                    continue;
                }
                float* pixel = row + static_cast<std::size_t>(x) * 4;
                const float remaining = 1.0f - alpha;
                if (particle.transfer_mode == ParticleTransferMode::stencil) {
                    for (unsigned channel=0; channel<4; ++channel) pixel[channel] *= remaining;
                } else {
                    for (unsigned channel=0; channel<3; ++channel) {
                        if (particle.transfer_mode == ParticleTransferMode::add) pixel[channel] += source[channel];
                        else if (particle.transfer_mode == ParticleTransferMode::screen)
                            pixel[channel] = source[channel]+pixel[channel]-source[channel]*pixel[channel];
                        else pixel[channel] = source[channel]+pixel[channel]*remaining;
                    }
                    pixel[3] = alpha+pixel[3]*remaining;
                }
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
