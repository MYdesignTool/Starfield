#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"

#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/Random.hpp"

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

struct PixelGrid {
    double aspect{1.0};
    double frame_width{1.0};
    double frame_height{1.0};
    double scale_x{1.0};
    double scale_y{1.0};
};

PixelGrid make_grid(const FrameSpec& frame) noexcept {
    PixelGrid grid;
    grid.aspect = (static_cast<double>(frame.layer_width) * frame.pixel_aspect_ratio) /
                  static_cast<double>(frame.layer_height);
    grid.frame_width = static_cast<double>(frame.frame_width);
    grid.frame_height = static_cast<double>(frame.frame_height);
    grid.scale_x = grid.frame_width / static_cast<double>(frame.layer_width);
    grid.scale_y = grid.frame_height / static_cast<double>(frame.layer_height);
    return grid;
}

struct Sprite {
    const ParticleInstance* particle{};
    double x{}, y{}, depth{}, ax{}, ay{}, bx{}, by{};
};
bool valid_camera(const RenderRequest::Camera& camera) noexcept {
    if (!camera.enabled) return true;
    return std::all_of(camera.layer_to_view.begin(), camera.layer_to_view.end(), [](double v) { return std::isfinite(v); }) &&
        std::all_of(camera.image_to_layer.begin(), camera.image_to_layer.end(), [](double v) { return std::isfinite(v); }) &&
        std::isfinite(camera.focal_x) && camera.focal_x > 0 && std::isfinite(camera.focal_y) && camera.focal_y > 0 &&
        std::isfinite(camera.center_x) && std::isfinite(camera.center_y) && std::isfinite(camera.near_clip) && camera.near_clip > 0;
}
Vec3 rotate_axis(Vec3 value,Vec3 angles) noexcept {
    constexpr double radians=3.14159265358979323846/180;
    for(int axis=0;axis<3;++axis) {
        const double angle=(axis==0?angles.x:axis==1?angles.y:angles.z)*radians;
        const double c=std::cos(angle),s=std::sin(angle);
        if(axis==0) value={value.x,c*value.y-s*value.z,s*value.y+c*value.z};
        if(axis==1) value={c*value.x+s*value.z,value.y,-s*value.x+c*value.z};
        if(axis==2) value={c*value.x-s*value.y,s*value.x+c*value.y,value.z};
    }
    return value;
}
bool project_sprite(const ParticleInstance& particle, const RenderRequest& request, const PixelGrid& grid, Sprite& sprite) noexcept {
    sprite.particle=&particle;
    const double rx=particle.size_pixels*.5;
    const double ry=particle.shape==0?rx:particle.size_y_pixels*.5;
    if(!(rx>0) || !(ry>0) || !(particle.opacity>0)) return false;
    const bool billboard=particle.shape!=1 || particle.limit_to_2d;
    Vec3 a{rx,0,0},b{0,ry,0};
    Vec3 angles=particle.rotation_degrees;
    if(billboard) angles.x=angles.y=0;
    else if(particle.up_axis==0) {a={0,0,rx};b={0,ry,0};}
    else if(particle.up_axis==1) {a={rx,0,0};b={0,0,ry};}
    a=rotate_axis(a,angles);b=rotate_axis(b,angles);
    if(!request.camera.enabled) {
        sprite.x=(.5+particle.position.x/grid.aspect)*grid.frame_width;
        sprite.y=(.5-particle.position.y)*grid.frame_height;
        sprite.ax=a.x*grid.scale_x;sprite.ay=a.y*grid.scale_y;
        sprite.bx=b.x*grid.scale_x;sprite.by=b.y*grid.scale_y;
    } else {
        const auto& camera=request.camera;const auto& frame=request.frame;
        const double local[4]{frame.layer_width*.5+particle.position.x*frame.layer_height/frame.pixel_aspect_ratio,
            (.5-particle.position.y)*frame.layer_height,particle.position.z*frame.layer_height,1};
        double view[3]{};
        for(int col=0;col<3;++col) for(int row=0;row<4;++row) view[col]+=local[row]*camera.layer_to_view[row*4+col];
        sprite.depth=view[2];if(!(view[2]>=camera.near_clip)) return false;
        const double u=camera.center_x+camera.focal_x*view[0]/view[2],v=camera.center_y+camera.focal_y*view[1]/view[2];
        const auto& m=camera.image_to_layer;const double w=m[6]*u+m[7]*v+m[8];
        if(!std::isfinite(w) || std::abs(w)<1e-12) return false;
        const double x=(m[0]*u+m[1]*v+m[2])/w,y=(m[3]*u+m[4]*v+m[5])/w;
        sprite.x=x*grid.scale_x;sprite.y=y*grid.scale_y;
        const auto project_axis=[&](Vec3 axis,double& dx,double& dy) {
            double axis_view[3]{axis.x,axis.y,0};
            if(!billboard) {
                const double local_axis[3]{axis.x/frame.pixel_aspect_ratio,axis.y,axis.z};
                for(int col=0;col<3;++col) {
                    axis_view[col]=0;
                    for(int row=0;row<3;++row) axis_view[col]+=local_axis[row]*camera.layer_to_view[row*4+col];
                }
            }
            const double du=camera.focal_x*(axis_view[0]*view[2]-view[0]*axis_view[2])/(view[2]*view[2]);
            const double dv=camera.focal_y*(axis_view[1]*view[2]-view[1]*axis_view[2])/(view[2]*view[2]);
            dx=((m[0]-x*m[6])*du+(m[1]-x*m[7])*dv)/w*grid.scale_x;
            dy=((m[3]-y*m[6])*du+(m[4]-y*m[7])*dv)/w*grid.scale_y;
        };
        project_axis(a,sprite.ax,sprite.ay);project_axis(b,sprite.bx,sprite.by);
    }
    return std::isfinite(sprite.x) && std::isfinite(sprite.y) && std::isfinite(sprite.ax) &&
        std::isfinite(sprite.ay) && std::isfinite(sprite.bx) && std::isfinite(sprite.by);
}

double sprite_coverage(const ParticleInstance& particle,double x,double y,double edge_scale) noexcept {
    const double feather=particle.feather_percent/100;
    const auto circle=[&](double distance,double scale) {
        const double aa=std::clamp(.5+(1-distance)*scale,0.0,1.0);
        return aa*(feather>0?std::clamp((1-distance)/feather,0.0,1.0):1);
    };
    if(particle.shape==1) return circle(std::max(std::abs(x),std::abs(y)),edge_scale);
    if(particle.shape==0) return circle(std::hypot(x,y),edge_scale);
    // A deterministic five-circle cluster. All lobes remain inside the sprite
    // bounds, so ROI/work accounting and future GPU parity use the same extent.
    double remaining=1;
    for(const auto center:{std::pair{0.0,0.0},std::pair{-.35,-.2},std::pair{.35,-.2},std::pair{-.2,.35},std::pair{.2,.35}})
        remaining*=1-circle(std::hypot(x-center.first,y-center.second)/.6,edge_scale*.6);
    return 1-remaining;
}

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
