#include "starfield/core/ParticleTexture.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <new>
#include <set>

namespace starfield::core {
bool valid_texture_style(const ParticleTextureStyle& style) noexcept {
    return static_cast<std::uint32_t>(style.time_mode) <= 7 &&
        static_cast<std::uint32_t>(style.color_use) <= 2 && style.use_ratio <= 1 && style.ignore_perspective <= 1;
}

Result<std::uint32_t> texture_frame_count(const TextureSource& source) noexcept {
    using R = Result<std::uint32_t>;
    if (!source.resource_id || !std::isfinite(source.start_seconds) || !std::isfinite(source.end_seconds) ||
        !std::isfinite(source.frame_seconds) || !(source.frame_seconds > 0) ||
        !(source.end_seconds > source.start_seconds) || !source.width || !source.height ||
        source.width > 32768 || source.height > 32768 || !std::isfinite(source.pixel_aspect_ratio) ||
        !(source.pixel_aspect_ratio > 0) || source.pixel_aspect_ratio > 100)
        return R::failure(ErrorCode::invalid_request, "invalid texture source metadata");
    const long double span = (static_cast<long double>(source.end_seconds) - source.start_seconds) / source.frame_seconds;
    // Round only arithmetic noise at an integral clip boundary, retaining real
    // partial frames in a half-open clip interval.
    const long double noise = 16 * std::numeric_limits<double>::epsilon() * std::max(1.L, span);
    const long double nearest = std::round(span);
    const long double count = nearest >= 1 && std::abs(span-nearest) <= noise ? nearest : std::ceil(span);
    if (!(count >= 1) || count > 0x1000000u)
        return R::failure(ErrorCode::work_limit_exceeded, "texture clip exceeds frame grid budget");
    return R::success(static_cast<std::uint32_t>(count));
}

Result<std::uint32_t> texture_frame_index(const TextureSource& source, TextureTimeMode mode,
    double render, double age, double life, std::uint32_t random) noexcept {
    using R = Result<std::uint32_t>;
    auto count = texture_frame_count(source);
    if (!count.has_value()) return R::failure(count.error());
    if (static_cast<std::uint32_t>(mode) > 7 || !std::isfinite(render) || !std::isfinite(age) ||
        !std::isfinite(life) || age < 0 || !(life > 0) || random > 0xffffffu)
        return R::failure(ErrorCode::invalid_request, "invalid texture sampling clock or mode");
    const auto frames = count.value();
    const auto random_frame = static_cast<std::uint32_t>((std::uint64_t(random) * frames) >> 24);
    long double position = 0;
    bool wrap = false;
    switch (mode) {
        case TextureTimeMode::current_time: position = (static_cast<long double>(render) - source.start_seconds) / source.frame_seconds; break;
        case TextureTimeMode::play_once: position = static_cast<long double>(age) / source.frame_seconds; break;
        case TextureTimeMode::loop: position = static_cast<long double>(age) / source.frame_seconds; wrap = true; break;
        case TextureTimeMode::stretch: position = std::clamp(static_cast<long double>(age) / life, 0.L, 1.L) * frames; break;
        case TextureTimeMode::random_still: return R::success(random_frame);
        case TextureTimeMode::random_once: position = random_frame + static_cast<long double>(age) / source.frame_seconds; break;
        case TextureTimeMode::random_loop: position = random_frame + static_cast<long double>(age) / source.frame_seconds; wrap = true; break;
        case TextureTimeMode::freeze_frame: position = (static_cast<long double>(render) - age - source.start_seconds) / source.frame_seconds; break;
    }
    if (!std::isfinite(position)) return R::failure(ErrorCode::invalid_time, "texture sampling clock overflow");
    // Limit tolerance below a pixel of the frame grid even for very large times.
    if (wrap) {
        const long double clip_frames = (static_cast<long double>(source.end_seconds)-source.start_seconds)/source.frame_seconds;
        position = std::fmod(position, clip_frames);
        if (position < 0) position += clip_frames;
    }
    position = std::floor(position + std::min(1.e-7L, 16 * std::numeric_limits<double>::epsilon() * std::max(1.L, std::abs(position))));
    position = std::clamp(position, 0.L, static_cast<long double>(frames - 1));
    return R::success(static_cast<std::uint32_t>(position));
}

Result<std::vector<TextureFrameRequest>> plan_texture_frames(const EvaluatedGraph& graph,
    std::span<const TextureSource> sources, double time, const Cancellation& cancel) try {
    using R = Result<std::vector<TextureFrameRequest>>;
    if (sources.size() > kMaxTextureSources || graph.texture_styles.size() > kMaxTextureStyles)
        return R::failure(ErrorCode::work_limit_exceeded, "texture source/style budget exceeded");
    std::map<std::uint32_t, const TextureSource*> source_map;
    for (const auto& source : sources) {
        auto count = texture_frame_count(source);
        if (!count.has_value()) return R::failure(count.error());
        if (!source_map.emplace(source.resource_id, &source).second)
            return R::failure(ErrorCode::invalid_request, "duplicate texture source");
    }
    for (const auto& style : graph.texture_styles) if (!valid_texture_style(style))
        return R::failure(ErrorCode::invalid_request, "invalid texture style");
    std::map<std::pair<std::uint32_t, std::uint32_t>, double> frames;
    std::size_t visited = 0;
    for (const auto& particle : graph.particles) {
        if ((visited++ & 63) == 0 && cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "texture planning cancelled");
        if (particle.shape != 3) continue;
        if (!particle.texture_style_index || particle.texture_style_index > graph.texture_styles.size())
            return R::failure(ErrorCode::invalid_request, "texture particle has no valid style");
        const auto& style = graph.texture_styles[particle.texture_style_index - 1];
        for (auto id : {style.front, style.back ? style.back : style.front}) {
            if (!id) continue;
            const auto found = source_map.find(id);
            if (found == source_map.end()) return R::failure(ErrorCode::invalid_request, "texture source is missing");
            const auto& source = *found->second;
            auto index = texture_frame_index(source, style.time_mode, time, particle.age_seconds,
                particle.lifetime_seconds, particle.texture_random_key);
            if (!index.has_value()) return R::failure(index.error());
            frames.emplace(std::pair{id, index.value()}, source.start_seconds + index.value() * source.frame_seconds);
            if (frames.size() > kMaxTextureFrames) return R::failure(ErrorCode::work_limit_exceeded, "texture frame request budget exceeded");
        }
    }
    if (cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "texture planning cancelled");
    std::vector<TextureFrameRequest> result;
    result.reserve(frames.size());
    for (const auto& [key, seconds] : frames) result.push_back({key.first, key.second, seconds});
    return R::success(std::move(result));
} catch (const std::bad_alloc&) {
    return Result<std::vector<TextureFrameRequest>>::failure(ErrorCode::allocation_failed, "texture plan allocation failed");
}

Result<bool> validate_texture_resources(std::span<const TextureSource> sources,
    std::span<const TextureFrameView> frames, const Cancellation& cancel) try {
    using R = Result<bool>;
    if (sources.size() > kMaxTextureSources || frames.size() > kMaxTextureFrames)
        return R::failure(ErrorCode::work_limit_exceeded, "texture resource count exceeded");
    std::map<std::uint32_t, std::uint32_t> counts;
    for (const auto& source : sources) {
        auto count = texture_frame_count(source);
        if (!count.has_value()) return R::failure(count.error());
        if (!counts.emplace(source.resource_id, count.value()).second)
            return R::failure(ErrorCode::invalid_request, "duplicate texture source");
    }
    std::set<std::pair<std::uint32_t, std::uint32_t>> keys;
    std::uint64_t bytes = 0;
    for (const auto& frame : frames) {
        if (cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "texture validation cancelled");
        const auto source = counts.find(frame.resource_id);
        if (source == counts.end() || frame.frame_index >= source->second || !frame.width || !frame.height ||
            frame.width > 32768 || frame.height > 32768 || frame.row_floats < std::uint64_t(frame.width) * 4 ||
            !keys.emplace(frame.resource_id, frame.frame_index).second)
            return R::failure(ErrorCode::invalid_request, "invalid or duplicate texture frame");
        const auto values = std::uint64_t(frame.row_floats) * frame.height;
        if (values > (kMaxTextureBytes - bytes) / sizeof(float))
            return R::failure(ErrorCode::work_limit_exceeded, "texture staging byte budget exceeded");
        bytes += values * sizeof(float);
        if (values != frame.pixels.size() || frame.pixels.data() == nullptr)
            return R::failure(ErrorCode::invalid_request, "texture staging layout mismatch");
        for (std::uint32_t y = 0; y < frame.height; ++y) {
            if (cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "texture pixel validation cancelled");
            const auto* row = frame.pixels.data() + std::size_t(y) * frame.row_floats;
            for (std::uint32_t x = 0; x < frame.width; ++x) {
                const auto* pixel = row + std::size_t(x) * 4;
                for (unsigned c = 0; c < 4; ++c) if (!std::isfinite(pixel[c]))
                    return R::failure(ErrorCode::invalid_request, "nonfinite texture pixel");
                if (pixel[3] < 0 || pixel[3] > 1) return R::failure(ErrorCode::invalid_request, "texture alpha outside range");
            }
        }
    }
    if (cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "texture validation cancelled");
    return R::success(true);
} catch (const std::bad_alloc&) { return Result<bool>::failure(ErrorCode::allocation_failed, "texture validation allocation failed"); }

std::array<float, 4> sample_texture(const TextureFrameView& frame, double u, double v) noexcept {
    if (!std::isfinite(u) || !std::isfinite(v) || !frame.width || !frame.height) return {};
    const double x = std::clamp(u * frame.width - .5, 0., double(frame.width - 1));
    const double y = std::clamp(v * frame.height - .5, 0., double(frame.height - 1));
    const auto left = static_cast<std::uint32_t>(x), top = static_cast<std::uint32_t>(y);
    const auto right = std::min(left + 1, frame.width - 1), bottom = std::min(top + 1, frame.height - 1);
    const double fx = x - left, fy = y - top;
    std::array<float, 4> result{};
    for (unsigned c = 0; c < 4; ++c) {
        const auto at = [&](std::uint32_t px, std::uint32_t py) { return frame.pixels[std::size_t(py) * frame.row_floats + std::size_t(px) * 4 + c]; };
        result[c] = static_cast<float>((1 - fy) * ((1 - fx) * at(left, top) + fx * at(right, top)) +
            fy * ((1 - fx) * at(left, bottom) + fx * at(right, bottom)));
    }
    return result;
}

std::array<float, 4> color_texture(std::array<float, 4> pixel, TextureColorUse use,
    std::array<double, 3> color, double opacity) noexcept {
    if (use != TextureColorUse::source) {
        double alpha = pixel[3];
        if (use == TextureColorUse::lightness) alpha *= alpha > 0 ?
            std::clamp((.2126 * pixel[0] + .7152 * pixel[1] + .0722 * pixel[2]) / alpha, 0., 1.) : 0;
        for (unsigned c = 0; c < 3; ++c) pixel[c] = static_cast<float>(color[c] * alpha);
        pixel[3] = static_cast<float>(alpha);
    }
    for (auto& channel : pixel) channel = static_cast<float>(channel * opacity);
    return pixel;
}
} // namespace starfield::core
