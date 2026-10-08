#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/Random.hpp"
#include "SpriteGeometry.hpp"
#include <limits>
#include <new>

namespace starfield::core {
Result<SpriteScene> prepare_sprite_scene(const RenderRequest& request, const Cancellation& cancel) try {
    using R = Result<SpriteScene>;
    const auto valid = validate_frame(request.frame);
    if (!valid.has_value()) return R::failure(valid.error());
    if (!sprite_geometry::valid_camera(request.camera))
        return R::failure(ErrorCode::invalid_request, "invalid GPU scene camera");
    const auto& frame = valid.value();
    SpriteScene scene;
    scene.region = frame.region_of_interest;
    if (scene.region.empty()) return R::success(std::move(scene));
    if (cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "scene preparation cancelled");
    const auto width = static_cast<std::uint32_t>(scene.region.width());
    const auto height = static_cast<std::uint32_t>(scene.region.height());
    scene.tiles_x = (width + 15) / 16; scene.tiles_y = (height + 15) / 16;
    const std::size_t tile_count = std::size_t(scene.tiles_x) * scene.tiles_y;
    // Budget accounts for all tile-list visits, not just visible sprite pixels.
    constexpr std::size_t max_indices = 32u * 1024u * 1024u;
    constexpr std::uint64_t max_pixel_visits = 512ull * 1024ull * 1024ull;
    const EmitterDimensionContext dimension{double(frame.layer_height), frame.pixel_aspect_ratio};
    auto evaluated = [&]() -> Result<EvaluatedGraph> {
        if (request.graph) {
            return evaluate_particle_graph(*request.graph, frame.time, cancel, dimension);
        }
        auto simulated=simulate_particles(request.settings, to_seconds(frame.time), cancel, dimension);
        if(!simulated.has_value())return Result<EvaluatedGraph>::failure(simulated.error());
        EvaluatedGraph result;result.particles=simulated.take_value();
        return Result<EvaluatedGraph>::success(std::move(result));
    }();
    if (!evaluated.has_value()) return R::failure(evaluated.error());
    const auto& particles=evaluated.value().particles;
    if (std::any_of(particles.begin(), particles.end(), [](const auto& p) { return p.shape==3; }))
        return R::failure(ErrorCode::unsupported_format, "texture particles require the CPU backend");
    const auto grid = sprite_geometry::make_grid(frame);
    double preview = 100;
    if (request.graph) for (const auto& node : request.graph->nodes) if (node.type_key == graph_keys::kOutputNode) {
        bool enabled = false; double chance = 100;
        for (const auto& p : node.parameters) {
            if (p.key == graph_keys::kPreviewEnabled) enabled = std::get<std::uint32_t>(p.value) != 0;
            if (p.key == graph_keys::kPreviewChance) chance = std::get<double>(p.value);
        }
        if (enabled) preview = chance;
    }
    std::vector<sprite_geometry::Sprite> projected;
    projected.reserve(particles.size());
    for (std::size_t i = 0; i < particles.size(); ++i) {
        if ((i & 63) == 0 && cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "scene projection cancelled");
        const auto& p = particles[i];
        if (preview < 100) {
            auto identity = p.id;
            for (auto byte : p.emitter_id.value.bytes) identity = mix64(identity ^ byte);
            if (unit_value(0, identity, RandomPurpose::preview_chance) * 100 >= preview) continue;
        }
        sprite_geometry::Sprite sprite;
        if (sprite_geometry::project_sprite(p, request, grid, sprite,evaluated.value().sprite_bases)) projected.push_back(sprite);
    }
    if (request.camera.enabled) std::stable_sort(projected.begin(), projected.end(),
        [](const auto& a, const auto& b) { return a.depth > b.depth; });
    scene.offsets.assign(tile_count + 1, 0);
    std::uint64_t visits = 0;
    scene.sprites.reserve(projected.size());
    for (std::size_t i = 0; i < projected.size(); ++i) {
        if ((i & 63) == 0 && cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "scene binning cancelled");
        const auto& s = projected[i]; const auto& p = *s.particle;
        const double determinant = s.ax * s.by - s.ay * s.bx;
        if (!std::isfinite(determinant) || std::abs(determinant) < 1e-12) continue;
        const double rx = p.shape == 1 ? std::abs(s.ax) + std::abs(s.bx) : std::hypot(s.ax, s.bx);
        const double ry = p.shape == 1 ? std::abs(s.ay) + std::abs(s.by) : std::hypot(s.ay, s.by);
        const double x = s.x - scene.region.left, y = s.y - scene.region.top;
        const auto left = std::int32_t(std::clamp(std::floor(x-rx-1), 0., double(width)));
        const auto top = std::int32_t(std::clamp(std::floor(y-ry-1), 0., double(height)));
        const auto right = std::int32_t(std::clamp(std::ceil(x+rx+1), 0., double(width)));
        const auto bottom = std::int32_t(std::clamp(std::ceil(y+ry+1), 0., double(height)));
        if (right <= left || bottom <= top) continue;
        SfGpuSprite sprite{float(x), float(y), float(s.by/determinant), float(-s.bx/determinant),
            float(-s.ay/determinant), float(s.ax/determinant), float(std::min(rx,ry)), float(p.feather_percent/100),
            float(p.color.x), float(p.color.y), float(p.color.z), float(p.opacity), left, top, right, bottom, p.shape,
            {static_cast<std::uint32_t>(p.transfer_mode),0,0}};
        // Extreme projection is a per-frame CPU fallback, never a truncated GPU scene.
        const float numeric[]{sprite.x,sprite.y,sprite.inverse_ax,sprite.inverse_ay,sprite.inverse_bx,sprite.inverse_by,
            sprite.edge_scale,sprite.feather,sprite.red,sprite.green,sprite.blue,sprite.opacity};
        for (int f = 0; f < 12; ++f) if (!std::isfinite(numeric[f]))
            return R::failure(ErrorCode::unsupported_format, "GPU scene projection exceeds float range");
        scene.sprites.push_back(sprite);
        for (auto ty = top/16; ty <= (bottom-1)/16; ++ty) for (auto tx = left/16; tx <= (right-1)/16; ++tx) {
            const auto tile = std::size_t(ty)*scene.tiles_x+tx;
            ++scene.offsets[tile+1];
            visits += std::uint64_t(std::min(16u,width-std::uint32_t(tx)*16)) * std::min(16u,height-std::uint32_t(ty)*16);
            if (visits > max_pixel_visits) return R::failure(ErrorCode::unsupported_format, "GPU scene exceeds tile work budget");
        }
    }
    for (std::size_t t = 1; t <= tile_count; ++t) {
        const std::uint64_t sum = std::uint64_t(scene.offsets[t]) + scene.offsets[t-1];
        if (sum > max_indices) return R::failure(ErrorCode::unsupported_format, "GPU scene exceeds index budget");
        scene.offsets[t] = std::uint32_t(sum);
    }
    scene.indices.resize(scene.offsets.back());
    auto cursors = scene.offsets;
    for (std::uint32_t i = 0; i < scene.sprites.size(); ++i) {
        if ((i & 63) == 0 && cancel.is_cancelled()) return R::failure(ErrorCode::cancelled, "scene indices cancelled");
        const auto& s = scene.sprites[i];
        for (auto ty = s.top/16; ty <= (s.bottom-1)/16; ++ty) for (auto tx = s.left/16; tx <= (s.right-1)/16; ++tx)
            scene.indices[cursors[std::size_t(ty)*scene.tiles_x+tx]++] = i;
    }
    return R::success(std::move(scene));
} catch (const std::bad_alloc&) { return Result<SpriteScene>::failure(ErrorCode::allocation_failed, "scene allocation failed"); }
}
