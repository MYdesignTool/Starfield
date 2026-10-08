#include "starfield/core/PluginApi.h"

#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SpriteScene.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"

#include <cstdio>
#include <limits>
#include <memory>
#include <new>
#include <span>

namespace {
namespace core = starfield::core;

SfCoreStatus status_for(core::ErrorCode code) noexcept {
    switch (code) {
        case core::ErrorCode::invalid_request: return SF_CORE_INVALID_REQUEST;
        case core::ErrorCode::invalid_time: return SF_CORE_INVALID_TIME;
        case core::ErrorCode::unsupported_format: return SF_CORE_UNSUPPORTED_FORMAT;
        case core::ErrorCode::allocation_failed: return SF_CORE_ALLOCATION_FAILED;
        case core::ErrorCode::work_limit_exceeded: return SF_CORE_WORK_LIMIT;
        case core::ErrorCode::cancelled: return SF_CORE_CANCELLED;
        case core::ErrorCode::internal_failure: return SF_CORE_INTERNAL_FAILURE;
    }
    return SF_CORE_INTERNAL_FAILURE;
}

void detail(char (&destination)[128], const char* message) noexcept {
    std::snprintf(destination, sizeof(destination), "%s", message != nullptr ? message : "unknown core error");
}

core::Result<core::Graph> decode_graph(const void* bytes, std::uint64_t count) {
    if (bytes == nullptr || count < core::kSequenceHeaderSize || count > core::kMaxGraphPayloadBytes ||
        count > std::numeric_limits<std::size_t>::max()) {
        return core::Result<core::Graph>::failure(core::ErrorCode::invalid_request, "invalid graph byte span");
    }
    auto decoded = core::deserialize_graph(
        std::span<const std::byte>(static_cast<const std::byte*>(bytes), static_cast<std::size_t>(count)),
        core::particle_node_registry());
    if (!decoded.has_value()) {
        return core::Result<core::Graph>::failure(
            decoded.error().code == core::SequenceErrorCode::allocation_failed
                ? core::ErrorCode::allocation_failed : core::ErrorCode::invalid_request,
            decoded.error().detail);
    }
    return core::Result<core::Graph>::success(decoded.take_value());
}

class CallbackCancellation final : public core::Cancellation {
public:
    explicit CallbackCancellation(const SfCoreRenderRequest& request) noexcept : request_(request) {}
    [[nodiscard]] bool is_cancelled() const noexcept override {
        return request_.is_cancelled != nullptr && request_.is_cancelled(request_.cancel_context) != 0;
    }
private:
    const SfCoreRenderRequest& request_;
};

core::Result<core::RenderRequest> decode_render_request(const SfCoreRenderRequest& input) {
    auto graph = decode_graph(input.graph_bytes, input.graph_byte_count);
    if (!graph.has_value()) return core::Result<core::RenderRequest>::failure(graph.error());
    const SfCoreFrame& source = input.frame;
    core::RenderRequest request;
    auto& frame = request.frame;
    frame.layer_width = source.layer_width;
    frame.layer_height = source.layer_height;
    frame.frame_width = source.frame_width;
    frame.frame_height = source.frame_height;
    frame.region_of_interest = {source.roi.left, source.roi.top, source.roi.right, source.roi.bottom};
    frame.time = {source.time_value, source.time_scale};
    frame.frame_duration = {source.duration_value, source.duration_scale};
    if (source.pixel_format > 2 || source.color_space > 2 || source.alpha_mode > 1 || source.quality > 1) {
        return core::Result<core::RenderRequest>::failure(core::ErrorCode::unsupported_format,"unsupported frame enum value");
    }
    frame.format = static_cast<core::PixelFormat>(source.pixel_format);
    frame.color_space = static_cast<core::ColorSpace>(source.color_space);
    frame.alpha_mode = static_cast<core::AlphaMode>(source.alpha_mode);
    frame.quality = static_cast<core::Quality>(source.quality);
    frame.pixel_aspect_ratio = source.pixel_aspect_ratio;
    request.graph = std::make_shared<const core::Graph>(graph.take_value());
    request.graph_revision = input.graph_revision;
    if (input.camera_enabled > 1) {
        return core::Result<core::RenderRequest>::failure(core::ErrorCode::invalid_request,"invalid camera flag");
    }
    request.camera.enabled = input.camera_enabled != 0;
    std::copy_n(input.layer_to_view, 16, request.camera.layer_to_view.begin());
    std::copy_n(input.image_to_layer, 9, request.camera.image_to_layer.begin());
    request.camera.focal_x = input.focal_x;
    request.camera.focal_y = input.focal_y;
    request.camera.center_x = input.center_x;
    request.camera.center_y = input.center_y;
    request.camera.near_clip = input.near_clip;
    if (input.texture_source_count > core::kMaxTextureSources || input.texture_frame_count > core::kMaxTextureFrames)
        return core::Result<core::RenderRequest>::failure(core::ErrorCode::work_limit_exceeded, "texture ABI count exceeded");
    if ((input.texture_source_count && !input.texture_sources) || (input.texture_frame_count && !input.texture_frames))
        return core::Result<core::RenderRequest>::failure(core::ErrorCode::invalid_request, "texture ABI array is missing");
    request.texture_sources.reserve(input.texture_source_count);
    for (std::uint32_t i = 0; i < input.texture_source_count; ++i) {
        const auto& s = input.texture_sources[i];
        if (s.struct_size != sizeof(SfTextureSource))
            return core::Result<core::RenderRequest>::failure(core::ErrorCode::invalid_request, "texture source ABI size mismatch");
        request.texture_sources.push_back({s.resource_id, s.start_seconds, s.end_seconds, s.frame_seconds,
            s.width, s.height, s.pixel_aspect_ratio});
    }
    request.texture_frames.reserve(input.texture_frame_count);
    std::uint64_t texture_bytes = 0;
    for (std::uint32_t i = 0; i < input.texture_frame_count; ++i) {
        const auto& f = input.texture_frames[i];
        if (f.struct_size != sizeof(SfTextureFrame) || !f.pixels || f.pixel_float_count > SIZE_MAX)
            return core::Result<core::RenderRequest>::failure(core::ErrorCode::invalid_request, "texture frame ABI layout mismatch");
        if (f.pixel_float_count > (core::kMaxTextureBytes - texture_bytes) / sizeof(float))
            return core::Result<core::RenderRequest>::failure(core::ErrorCode::work_limit_exceeded, "texture ABI byte budget exceeded");
        texture_bytes += f.pixel_float_count * sizeof(float);
        request.texture_frames.push_back({f.resource_id, f.frame_index, f.width, f.height, f.row_floats,
            {f.pixels, static_cast<std::size_t>(f.pixel_float_count)}});
    }
    return core::Result<core::RenderRequest>::success(std::move(request));
}

SfCoreStatus SF_CORE_CALL prepare_gpu_scene(const SfCoreRenderRequest* input, SfCoreGpuSceneResult* output) {
    if (output == nullptr || output->struct_size != sizeof(SfCoreGpuSceneResult)) return SF_CORE_INVALID_REQUEST;
    *output = SfCoreGpuSceneResult{};
    output->struct_size = sizeof(SfCoreGpuSceneResult);
    if (input == nullptr || input->struct_size != sizeof(SfCoreRenderRequest)) {
        output->status = SF_CORE_INVALID_REQUEST;
        detail(output->detail, "core render ABI request size mismatch");
        return output->status;
    }
    try {
        auto decoded = decode_render_request(*input);
        if (!decoded.has_value()) {
            output->status = status_for(decoded.error().code);
            detail(output->detail,decoded.error().detail);
            return output->status;
        }
        auto request = decoded.take_value();
        const CallbackCancellation cancellation(*input);
        auto scene = core::prepare_sprite_scene(request, cancellation);
        if (!scene.has_value()) {
            output->status = status_for(scene.error().code);
            detail(output->detail, scene.error().detail);
            return output->status;
        }
        auto owned = std::make_unique<core::SpriteScene>(scene.take_value());
        output->region = {owned->region.left,owned->region.top,owned->region.right,owned->region.bottom};
        output->tile_size = 16;
        output->tiles_x = owned->tiles_x; output->tiles_y = owned->tiles_y;
        output->sprite_count = static_cast<uint32_t>(owned->sprites.size());
        output->index_count = static_cast<uint32_t>(owned->indices.size());
        output->sprites = owned->sprites.data(); output->tile_offsets = owned->offsets.data();
        output->cloud_circle_count=static_cast<uint32_t>(owned->cloud_circles.size());
        output->cloud_circles=owned->cloud_circles.data();
        output->tile_indices = owned->indices.data(); output->opaque_handle = owned.release();
        output->status = SF_CORE_OK;
        return SF_CORE_OK;
    } catch (const std::bad_alloc&) {
        output->status = SF_CORE_ALLOCATION_FAILED;
        detail(output->detail, "GPU scene allocation failed");
    } catch (...) {
        output->status = SF_CORE_INTERNAL_FAILURE;
        detail(output->detail, "GPU scene unexpected exception");
    }
    return output->status;
}
void SF_CORE_CALL release_gpu_scene(SfCoreGpuSceneResult* result) {
    if (!result || result->struct_size != sizeof(SfCoreGpuSceneResult)) return;
    delete static_cast<core::SpriteScene*>(result->opaque_handle);
    *result = {}; result->struct_size = sizeof(SfCoreGpuSceneResult);
}

SfCoreStatus SF_CORE_CALL render(const SfCoreRenderRequest* input, SfCoreRenderResult* output) {
    if (output == nullptr || output->struct_size != sizeof(SfCoreRenderResult)) return SF_CORE_INVALID_REQUEST;
    *output = SfCoreRenderResult{};
    output->struct_size = sizeof(SfCoreRenderResult);
    if (input == nullptr || input->struct_size != sizeof(SfCoreRenderRequest)) {
        output->status = SF_CORE_INVALID_REQUEST;
        detail(output->detail, "core render ABI request size mismatch");
        return output->status;
    }
    try {
        auto decoded = decode_render_request(*input);
        if (!decoded.has_value()) {
            output->status = status_for(decoded.error().code);
            detail(output->detail,decoded.error().detail);
            return output->status;
        }
        auto request = decoded.take_value();
        const CallbackCancellation cancellation(*input);
        const core::CpuParticleRenderer renderer;
        auto rendered = renderer.render(request, cancellation);
        if (!rendered.has_value()) {
            output->status = status_for(rendered.error().code);
            detail(output->detail, rendered.error().detail);
            return output->status;
        }
        auto owned = std::make_unique<core::RenderOutput>(rendered.take_value());
        output->region = {owned->region.left, owned->region.top, owned->region.right, owned->region.bottom};
        output->row_bytes = owned->row_bytes;
        output->pixel_format = static_cast<std::uint32_t>(owned->format);
        output->pixels = owned->pixels.data();
        output->pixel_byte_count = owned->pixels.size();
        output->opaque_handle = owned.release();
        output->status = SF_CORE_OK;
        return SF_CORE_OK;
    } catch (const std::bad_alloc&) {
        output->status = SF_CORE_ALLOCATION_FAILED;
        detail(output->detail, "core render allocation failed");
    } catch (...) {
        output->status = SF_CORE_INTERNAL_FAILURE;
        detail(output->detail, "core render unexpected exception");
    }
    return output->status;
}

void SF_CORE_CALL release_render_result(SfCoreRenderResult* output) {
    if (output == nullptr || output->struct_size != sizeof(SfCoreRenderResult)) return;
    delete static_cast<core::RenderOutput*>(output->opaque_handle);
    *output = SfCoreRenderResult{};
    output->struct_size = sizeof(SfCoreRenderResult);
}

SfCoreStatus SF_CORE_CALL inspect(const SfCoreInspectRequest* input, SfCoreInspectResult* output) {
    if (output == nullptr || output->struct_size != sizeof(SfCoreInspectResult)) return SF_CORE_INVALID_REQUEST;
    *output = SfCoreInspectResult{};
    output->struct_size = sizeof(SfCoreInspectResult);
    if (input == nullptr || input->struct_size != sizeof(SfCoreInspectRequest)) {
        output->status = SF_CORE_INVALID_REQUEST;
        detail(output->detail, "core inspect ABI request size mismatch");
        return output->status;
    }
    try {
        auto graph = decode_graph(input->graph_bytes, input->graph_byte_count);
        if (!graph.has_value()) {
            output->status = status_for(graph.error().code);
            detail(output->detail, graph.error().detail);
            return output->status;
        }
        output->node_count = graph.value().nodes.size();
        output->edge_count = graph.value().edges.size();
        const core::NeverCancelled cancellation;
        const auto evaluated = core::evaluate_particle_graph(
            graph.value(), core::RationalTime{input->time_value, input->time_scale}, cancellation);
        if (!evaluated.has_value()) {
            output->status = status_for(evaluated.error().code);
            detail(output->detail, evaluated.error().detail);
            return output->status;
        }
        output->live_particle_count = evaluated.value().particles.size();
        output->status = SF_CORE_OK;
        return SF_CORE_OK;
    } catch (const std::bad_alloc&) {
        output->status = SF_CORE_ALLOCATION_FAILED;
        detail(output->detail, "core inspect allocation failed");
    } catch (...) {
        output->status = SF_CORE_INTERNAL_FAILURE;
        detail(output->detail, "core inspect unexpected exception");
    }
    return output->status;
}
} // namespace

extern "C" SF_CORE_EXPORT int32_t SF_CORE_CALL StarfieldCore_GetApi(
    uint32_t abi_version, uint32_t api_struct_size, SfCoreApi* out_api) {
    if (out_api == nullptr || abi_version != SF_CORE_ABI_VERSION || api_struct_size != sizeof(SfCoreApi))
        return 0;
    *out_api = SfCoreApi{sizeof(SfCoreApi), SF_CORE_ABI_VERSION, render, release_render_result, inspect, prepare_gpu_scene, release_gpu_scene};
    return 1;
}
