#include "SmartRender.hpp"

#include "AE_Macros.h"
#include "CoreLoader.hpp"
#include "Camera.hpp"
#include "GpuRender.hpp"
#include "Diagnostics.hpp"
#include "Parameters.hpp"
#include "EmitterHistory.hpp"
#include "MotionBlur.hpp"
#include "WorldBridge.hpp"
#include "TextureResources.hpp"

#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <mutex>
#include <new>
#include <limits>

// Geometry contract (ADR 0005). AE_Effect.h documents in_data->width/height as the
// full-resolution layer size and says every layer size is "automatically adjusted
// to compensate" for downsampling, but it does not pin down whether the stored
// PF_RationalScale factor is a divisor (2 at half resolution) or a scale (1/2): the
// header describes a range of "1 to 999+" while the SDK's own samples multiply by
// the rational. Rather than trusting either reading, all render-space geometry here
// is derived from what the host actually hands over:
//   * the output world's dimensions vs the pre-render result rect -> pixels per
//     host rect unit,
//   * PF_CheckoutResult::max_result_rect -> the layer extent in host rect units,
//   * PF_CheckoutResult::ref_width/ref_height -> the full-resolution reference used
//     for the canonical world space.
// The downsample fields are reported by the diagnostics readout only.

namespace starfield::adapter {
namespace {

namespace core = starfield::core;
std::mutex timing_mutex;
PreRenderTimings preparation_timings;
SmartRenderTiming execution_timing;
struct PreparationTimer {
    using Clock=std::chrono::steady_clock;
    Clock::time_point start{Clock::now()};
    PreRenderTimings value;
    explicit PreparationTimer(const PF_InData* data) {
        value.valid=true;
        if(data && data->time_scale)value.seconds=double(data->current_time)/data->time_scale;
    }
    static double elapsed(Clock::time_point from) {
        return std::chrono::duration<double,std::milli>(Clock::now()-from).count();
    }
    ~PreparationTimer() {
        value.total_ms=elapsed(start);
        std::lock_guard lock(timing_mutex);preparation_timings=value;
    }
};
struct ExecutionTimer {
    PreparationTimer::Clock::time_point start{PreparationTimer::Clock::now()};
    SmartRenderTiming value;
    explicit ExecutionTimer(const PF_InData* data) {
        value.valid=true;
        if(data && data->time_scale)value.seconds=double(data->current_time)/data->time_scale;
    }
    ~ExecutionTimer() {
        value.total_ms=PreparationTimer::elapsed(start);
        std::lock_guard lock(timing_mutex);execution_timing=value;
    }
};

// Pre-render checks out empty input metadata to obtain AE's layer bounds and
// full-resolution reference geometry. Smart Render still pairs the checkout with an
// empty pixel checkout to satisfy AE's checkout_output ordering, but never reads it.
constexpr A_long kInputCheckoutId = 1;
constexpr PF_ParamIndex kInputParameterIndex = 0;

PF_LRect intersect_lrect(const PF_LRect& a, const PF_LRect& b) noexcept {
    PF_LRect result{};
    result.left = a.left > b.left ? a.left : b.left;
    result.top = a.top > b.top ? a.top : b.top;
    result.right = a.right < b.right ? a.right : b.right;
    result.bottom = a.bottom < b.bottom ? a.bottom : b.bottom;
    if (result.right < result.left) {
        result.right = result.left;
    }
    if (result.bottom < result.top) {
        result.bottom = result.top;
    }
    return result;
}

core::RectI to_core_rect(const PF_LRect& rect) noexcept {
    return core::RectI{rect.left, rect.top, rect.right, rect.bottom};
}

// Maps core errors onto stable host errors; the AE-facing contract lives here.
PF_Err host_error_for(const core::CoreError& error, PF_Err abort_error) noexcept {
    switch (error.code) {
        case core::ErrorCode::allocation_failed:
            return PF_Err_OUT_OF_MEMORY;
        case core::ErrorCode::cancelled:
            return abort_error != PF_Err_NONE ? abort_error : PF_Interrupt_CANCEL;
        case core::ErrorCode::invalid_request:
        case core::ErrorCode::invalid_time:
        case core::ErrorCode::unsupported_format:
            return PF_Err_BAD_CALLBACK_PARAM;
        case core::ErrorCode::work_limit_exceeded:
        case core::ErrorCode::internal_failure:
            break;
    }
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
}

void report_failure(PF_OutData* out_data, const core::CoreError& error) noexcept {
    if (out_data == nullptr) {
        return;
    }
    if (error.code == core::ErrorCode::cancelled) {
        out_data->return_msg[0] = '\0';
        return;
    }
    // std::snprintf instead of the host formatter: this path must not depend on a
    // callback that may be unavailable when a render fails.
    std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Starfield Particle: %s (%s)",
                  core::describe(error.code), error.detail);
    // A nonempty return_msg opens a dialog even without DISPLAY_ERROR_MESSAGE
    // (AE_Effect.h). Keep diagnostics for real failures, never normal interrupts.
}

PF_Err host_error_for_status(SfCoreStatus status, PF_Err abort_error) noexcept {
    switch (status) {
        case SF_CORE_OK: return PF_Err_NONE;
        case SF_CORE_ALLOCATION_FAILED: return PF_Err_OUT_OF_MEMORY;
        case SF_CORE_CANCELLED: return abort_error != PF_Err_NONE ? abort_error : PF_Interrupt_CANCEL;
        case SF_CORE_INVALID_REQUEST:
        case SF_CORE_INVALID_TIME:
        case SF_CORE_UNSUPPORTED_FORMAT: return PF_Err_BAD_CALLBACK_PARAM;
        case SF_CORE_WORK_LIMIT:
        case SF_CORE_INTERNAL_FAILURE: return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
}

void report_core_failure(PF_OutData* out_data, const char* detail) noexcept {
    if (out_data != nullptr)
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                      "Starfield Particle core: %s", detail != nullptr ? detail : "unknown error");
}

// Host abort callback exposed as the core cancellation contract.
class HostCancellation final : public core::Cancellation {
public:
    explicit HostCancellation(PF_InData* in_data) noexcept : in_data_(in_data) {}

    [[nodiscard]] bool is_cancelled() const noexcept override {
        const PF_Err err = PF_ABORT(in_data_);
        if (err != PF_Err_NONE && abort_error_ == PF_Err_NONE) {
            abort_error_ = err;
        }
        return abort_error_ != PF_Err_NONE;
    }

    [[nodiscard]] PF_Err abort_error() const noexcept { return abort_error_; }

private:
    PF_InData* in_data_{nullptr};
    mutable PF_Err abort_error_{PF_Err_NONE};
};

// Pre-render facts handed to the render phase. AE takes ownership after pre-render
// returns and frees the block through the callback below.
struct PreRenderState {
    A_long time{},step{};A_u_long scale{};
    core::OpaqueBytes graph_bytes;
    SfCoreGpuSceneResult gpu_scene{};
    MotionExposure motion;MotionGpuStorage motion_gpu;
    TexturePreparation textures;
    A_long gpu_world_width{},gpu_world_height{};
    ~PreRenderState() { if(generation && gpu_scene.struct_size==sizeof(gpu_scene)) generation->api().release_gpu_scene(&gpu_scene); }
    std::shared_ptr<const CoreGeneration> generation;
    PF_LRect result_rect{};
    PF_LRect max_result_rect{};
    A_long ref_width{0};
    A_long ref_height{0};
    PF_RationalScale par{};
};

void delete_pre_render_state(void* data) noexcept {
    delete static_cast<PreRenderState*>(data);
}

struct HostGeometry {
    double pixel_per_rect_x{1.0};
    double pixel_per_rect_y{1.0};
};

double observed_ratio(std::uint32_t pixel_extent, std::int64_t rect_extent) noexcept {
    if (pixel_extent > 0 && rect_extent > 0) {
        return static_cast<double>(pixel_extent) / static_cast<double>(rect_extent);
    }
    return 1.0;
}

double scaled_extent(std::int64_t rect_extent, double pixel_per_rect) noexcept {
    return static_cast<double>(rect_extent) * pixel_per_rect;
}

std::uint32_t grid_dimension(double extent, A_long fallback) noexcept {
    if (!(extent > 0.0)) {
        return static_cast<std::uint32_t>(std::max<A_long>(1, fallback));
    }
    const double rounded = std::round(extent);
    if (rounded >= 32768.0) {
        return 32768;
    }
    return static_cast<std::uint32_t>(std::max(1.0, rounded));
}

std::int32_t scaled_origin(A_long rect_origin, double pixel_per_rect) noexcept {
    const double scaled = static_cast<double>(rect_origin) * pixel_per_rect;
    if (scaled > 32767.0) {
        return 32767;
    }
    if (scaled < -32768.0) {
        return -32768;
    }
    return static_cast<std::int32_t>(std::lround(scaled));
}

PF_Err make_request(PF_InData* in_data,PF_OutData* out_data,HostBitDepth depth,const PreRenderState& state,
    const WorldLayout& output_layout,HostCancellation& cancellation,SfCoreRenderRequest& request) noexcept {
    HostGeometry geometry;
    geometry.pixel_per_rect_x = observed_ratio(output_layout.width, state.result_rect.right - state.result_rect.left);
    geometry.pixel_per_rect_y =
        observed_ratio(output_layout.height, state.result_rect.bottom - state.result_rect.top);

    const std::uint32_t grid_width = grid_dimension(
        scaled_extent(state.max_result_rect.right - state.max_result_rect.left, geometry.pixel_per_rect_x),
        state.ref_width);
    const std::uint32_t grid_height = grid_dimension(
        scaled_extent(state.max_result_rect.bottom - state.max_result_rect.top, geometry.pixel_per_rect_y),
        state.ref_height);

    // Hand the geometry of this frame to the Options readout. The readout cannot call
    // checkout_layer itself, so without this it cannot say which sizes the point-control
    // conversion actually used at a reduced preview resolution.
    record_render_geometry(in_data->width, in_data->height, state.ref_width, state.ref_height,
                           static_cast<A_long>(grid_width), static_cast<A_long>(grid_height), state.par.num,
                           state.par.den);

    // The world origin is reported in host rect units, so it takes the same scale as
    // the extents to land in the frame grid.
    const std::int32_t roi_left = scaled_origin(output_layout.origin_x, geometry.pixel_per_rect_x);
    const std::int32_t roi_top = scaled_origin(output_layout.origin_y, geometry.pixel_per_rect_y);
    const core::RectI world_rect{roi_left, roi_top, roi_left + static_cast<std::int32_t>(output_layout.width),
                                 roi_top + static_cast<std::int32_t>(output_layout.height)};
    const core::RectI grid_rect{0, 0, static_cast<std::int32_t>(grid_width),
                                static_cast<std::int32_t>(grid_height)};

    core::FrameSpec frame;
    frame.layer_width = static_cast<std::uint32_t>(std::max<A_long>(1, state.ref_width));
    frame.layer_height = static_cast<std::uint32_t>(std::max<A_long>(1, state.ref_height));
    frame.frame_width = grid_width;
    frame.frame_height = grid_height;
    frame.region_of_interest = core::intersect(world_rect, grid_rect);
    frame.time = core::RationalTime{static_cast<std::int64_t>(in_data->current_time),
                                    static_cast<std::int64_t>(in_data->time_scale)};
    frame.frame_duration = core::RationalTime{static_cast<std::int64_t>(in_data->time_step),
                                              static_cast<std::int64_t>(in_data->time_scale)};
    frame.format = pixel_format_for(depth);
    frame.color_space = core::ColorSpace::ae_working_space;
    // M3-06 alpha candidate: request straight output based on the measured dark
    // composite from the prior build. The core still accumulates premultiplied;
    // this host-boundary change needs AE 2023 confirmation.
    frame.alpha_mode = core::AlphaMode::straight;
    frame.pixel_aspect_ratio = rational_scale_value(state.par, host_pixel_aspect_ratio(*in_data));
    frame.quality = in_data->quality == PF_Quality_HI ? core::Quality::full : core::Quality::draft;


    request.struct_size = sizeof(request);
    request.frame = SfCoreFrame{
        frame.layer_width, frame.layer_height, frame.frame_width, frame.frame_height,
        {frame.region_of_interest.left, frame.region_of_interest.top,
         frame.region_of_interest.right, frame.region_of_interest.bottom},
        frame.time.value, frame.time.scale, frame.frame_duration.value, frame.frame_duration.scale,
        static_cast<std::uint32_t>(frame.format), static_cast<std::uint32_t>(frame.color_space),
        static_cast<std::uint32_t>(frame.alpha_mode), static_cast<std::uint32_t>(frame.quality),
        frame.pixel_aspect_ratio};
    request.graph_bytes = state.graph_bytes.data();
    request.graph_byte_count = state.graph_bytes.size();
    request.texture_source_count=static_cast<std::uint32_t>(state.textures.abi_sources.size());
    request.texture_sources=state.textures.abi_sources.data();
    const PF_Err camera_error = capture_camera(in_data, request);
    if (camera_error) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Starfield camera geometry is unavailable or singular.");
        return camera_error;
    }
    request.is_cancelled = [](void* context) -> std::int32_t {
        return static_cast<HostCancellation*>(context)->is_cancelled() ? 1 : 0;
    };
    request.cancel_context = &cancellation;
    return PF_Err_NONE;
}

PF_Err render_frame(PF_InData* in_data, PF_OutData* out_data, HostBitDepth depth, const PreRenderState& state,
                    PF_EffectWorld* output_world,const TextureStaging& textures) noexcept {
    WorldLayout output_layout{};
    if (!describe_world(*output_world, output_layout)) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    try {
        HostCancellation cancellation(in_data);
        SfCoreRenderRequest request{};
        const auto request_error=make_request(in_data,out_data,depth,state,output_layout,cancellation,request);
        if(request_error) return request_error;
        request.texture_frame_count=static_cast<std::uint32_t>(textures.frames.size());
        request.texture_frames=textures.frames.data();
        if(state.motion.enabled)return render_motion_cpu(in_data,out_data,state.generation->api(),request,state.motion,output_layout,output_world,depth,cancellation);
        SfCoreRenderResult rendered{};
        rendered.struct_size = sizeof(rendered);
        const auto& api = state.generation->api();
        const SfCoreStatus status = api.render(&request, &rendered);
        struct ResultRelease {
            const SfCoreApi& api;
            SfCoreRenderResult& result;
            ~ResultRelease() { api.release_render_result(&result); }
        } release{api, rendered};
        if (status != SF_CORE_OK || rendered.status != SF_CORE_OK) {
            const auto failure = status != SF_CORE_OK ? status : rendered.status;
            // AE aborts superseded preview frames during ordinary editing. A
            // return_msg converts that normal interrupt into an error dialog.
            if (failure != SF_CORE_CANCELLED) report_core_failure(out_data, rendered.detail);
            else if (out_data != nullptr) out_data->return_msg[0] = '\0';
            return host_error_for_status(failure, cancellation.abort_error());
        }
        if (rendered.pixel_byte_count > std::numeric_limits<std::size_t>::max() ||
            (rendered.pixel_byte_count > 0 && rendered.pixels == nullptr) ||
            rendered.pixel_format != request.frame.pixel_format) {
            report_core_failure(out_data, "core returned an invalid pixel buffer");
            return PF_Err_INTERNAL_STRUCT_DAMAGED;
        }
        const OutputView pixels{
            {rendered.region.left, rendered.region.top, rendered.region.right, rendered.region.bottom},
            rendered.row_bytes, static_cast<core::PixelFormat>(rendered.pixel_format),
            std::span<const std::byte>(static_cast<const std::byte*>(rendered.pixels),
                                       static_cast<std::size_t>(rendered.pixel_byte_count))};
        if (!write_output(pixels, output_layout, *output_world, depth, cancellation)) {
            if (cancellation.is_cancelled()) {
                return host_error_for(core::make_error(core::ErrorCode::cancelled,
                                                       "render cancelled while copying output pixels"),
                                      cancellation.abort_error());
            }
            return PF_Err_INTERNAL_STRUCT_DAMAGED;
        }
    } catch (const std::bad_alloc&) {
        return PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    return PF_Err_NONE;
}

} // namespace

PreRenderTimings last_pre_render_timings() noexcept {
    std::lock_guard lock(timing_mutex);return preparation_timings;
}
SmartRenderTiming last_smart_render_timing() noexcept {
    std::lock_guard lock(timing_mutex);return execution_timing;
}
PF_Err pre_render(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) noexcept try {
    PreparationTimer timer(in_data);
    if (in_data == nullptr || out_data==nullptr || extra == nullptr || extra->input == nullptr || extra->output == nullptr ||
        extra->cb == nullptr || extra->cb->checkout_layer == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    const PF_RenderRequest request = extra->input->output_request;

    // The metadata checkout comes first because its ref_width/ref_height are the
    // full-resolution reference the parameter conversion needs. AE 2023.5 Build 52
    // scales point values with preview resolution; Parameters.cpp restores that scale
    // before dividing by this reference.
    // This emitter does not depend on source pixels. Request an empty input rectangle
    // to preserve the layer metadata while avoiding a render of the solid/background.
    PF_CheckoutResult input_result{};
    AEFX_CLR_STRUCT(input_result);
    PF_RenderRequest input_request = request;
    input_request.rect = PF_LRect{0, 0, 0, 0};
    const auto input_err = extra->cb->checkout_layer(in_data->effect_ref, kInputParameterIndex, kInputCheckoutId,
                                                     &input_request, in_data->current_time, in_data->time_step,
                                                     in_data->time_scale, &input_result);
    if (input_err != PF_Err_NONE) return input_err;

    std::shared_ptr<const core::Graph> graph;
    const auto controls_start=PreparationTimer::Clock::now();
    const auto graph_err = checkout_render_graph(in_data, out_data, graph, nullptr,
                                                 input_result.ref_width, input_result.ref_height);
    timer.value.controls_ms=PreparationTimer::elapsed(controls_start);
    if (graph_err != PF_Err_NONE) {
        // Failing here is safe resource-wise: pre-render checkouts belong to the frame,
        // only the smart-render phase checks them in, and AE tears the frame down when
        // pre-render reports an error. (PF_PreRenderCallbacks has no checkin callback.)
        return graph_err;
    }
    core::Graph frame_graph=*graph;
    HostCancellation history_cancel(in_data);
    const auto history_start=PreparationTimer::Clock::now();
    MotionExposure motion;
    auto history_error=prepare_motion_exposure(in_data,out_data,frame_graph,input_result.ref_width,input_result.ref_height,history_cancel,motion);
    if(!history_error && !motion.enabled)history_error=capture_emitter_origin_history(in_data,out_data,frame_graph,
        input_result.ref_width,input_result.ref_height,history_cancel);
    timer.value.history_ms=PreparationTimer::elapsed(history_start);
    if(history_error) return history_error;
    graph=std::make_shared<const core::Graph>(std::move(frame_graph));
    auto encoded = core::serialize_graph(*graph, core::particle_node_registry());
    if (!encoded.has_value()) {
        report_core_failure(out_data, encoded.error().detail);
        return encoded.error().code == core::SequenceErrorCode::allocation_failed
            ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
    }
    const auto loaded = acquire_core();
    if (!loaded) {
        report_core_failure(out_data, loaded.error.c_str());
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

    // Layer extent in host rect units. AE documents this as independent of the
    // request, which is what max_result_rect must be.
    PF_LRect layer_rect = input_result.max_result_rect;
    if (layer_rect.right <= layer_rect.left || layer_rect.bottom <= layer_rect.top) {
        const A_long width = in_data->width > 0 ? in_data->width : 1;
        const A_long height = in_data->height > 0 ? in_data->height : 1;
        layer_rect = PF_LRect{0, 0, width, height};
    }

    // Particles are not limited to the input's extent, so the whole requested rect can
    // be produced; it never exceeds the request (RETURNS_EXTRA_PIXELS stays unset).
    const PF_LRect result = intersect_lrect(request.rect, layer_rect);

    auto state = std::unique_ptr<PreRenderState>(new (std::nothrow) PreRenderState{});
    if (!state) {
        return PF_Err_OUT_OF_MEMORY;
    }
    state->motion=std::move(motion);
    state->graph_bytes = encoded.take_value();
    state->time=in_data->current_time;state->step=in_data->time_step;state->scale=in_data->time_scale;
    state->generation = loaded.generation;
    state->result_rect = result;
    state->max_result_rect = layer_rect;
    state->ref_width = input_result.ref_width > 0 ? input_result.ref_width : (in_data->width > 0 ? in_data->width : 1);
    state->ref_height =
        input_result.ref_height > 0 ? input_result.ref_height : (in_data->height > 0 ? in_data->height : 1);
    state->par = input_result.par;
    if (state->par.den == 0 || state->par.num <= 0) {
        state->par = in_data->pixel_aspect_ratio;
    }
    if (state->par.den == 0 || state->par.num <= 0) {
        state->par = PF_RationalScale{1, 1};
    }

    // External code generation affects pixels but is not an AE parameter stream.
    // Mix the selected immutable generation into the SmartFX cache key.
    if (extra->cb->GuidMixInPtr == nullptr) {
        report_core_failure(out_data, "AE did not provide the SmartFX cache-key callback");
        return PF_Err_BAD_CALLBACK_PARAM;
    }
    const std::uint64_t generation_identity = state->generation->cache_identity();
    const PF_Err cache_err = extra->cb->GuidMixInPtr(in_data->effect_ref,
                                                     static_cast<A_u_long>(sizeof(generation_identity)),
                                                     &generation_identity);
    if (cache_err != PF_Err_NONE) return cache_err;
    if(const auto error=prepare_texture_resources(in_data,out_data,extra,*graph,state->motion,state->ref_height,
        double(state->par.num)/state->par.den,history_cancel,state->textures);error)return error;
    const double exposure_key[2]{state->motion.enabled?1.0:0.0,state->motion.gain};
    if(const auto error=extra->cb->GuidMixInPtr(in_data->effect_ref,sizeof(exposure_key),exposure_key);error)return error;
    for(const auto& sample:state->motion.samples) {
        double key[34]{};key[0]=double(sample.time.value);key[1]=double(sample.time.scale);key[2]=sample.camera.camera_enabled;
        std::copy(std::begin(sample.camera.layer_to_view),std::end(sample.camera.layer_to_view),key+3);
        std::copy(std::begin(sample.camera.image_to_layer),std::end(sample.camera.image_to_layer),key+19);
        key[28]=sample.camera.focal_x;key[29]=sample.camera.focal_y;key[30]=sample.camera.center_x;key[31]=sample.camera.center_y;key[32]=sample.camera.near_clip;key[33]=double(state->motion.samples.size());
        if(const auto error=extra->cb->GuidMixInPtr(in_data->effect_ref,sizeof(key),key);error)return error;
    }

    bool prefer_gpu=true;
    for(const auto& node:graph->nodes) if(node.type_key==core::graph_keys::kOutputNode)
        for(const auto& p:node.parameters) if(p.key==core::graph_keys::kAcceleration)
            prefer_gpu=std::get<std::uint32_t>(p.value)==0;
    bool gpu_possible=false;
    if(prefer_gpu && gpu_device_matches(extra->input->gpu_data,extra->input->what_gpu,extra->input->device_index)) {
        const auto scene_start=PreparationTimer::Clock::now();
        // Pre-render eligibility includes actual scene construction and bounds.
        // This candidate expects GPU rectangles in render-resolution pixels; a GPU
        // checkout with different geometry is rejected, never clipped or guessed.
        const WorldLayout predicted{result.left,result.top,std::uint32_t(std::max<A_long>(0,result.right-result.left)),
            std::uint32_t(std::max<A_long>(0,result.bottom-result.top)),0};
        HostCancellation cancellation(in_data); SfCoreRenderRequest scene_request{};
        const auto request_error=make_request(in_data,out_data,HostBitDepth::bpc32,*state,predicted,cancellation,scene_request);
        if(request_error) return request_error;
        state->gpu_scene.struct_size=sizeof(state->gpu_scene);
        SfCoreStatus status=SF_CORE_OK;
        if(state->motion.enabled) {
            const auto error=prepare_motion_gpu(in_data,out_data,state->generation->api(),scene_request,state->motion,state->gpu_scene,state->motion_gpu,cancellation);
            if(error)return error;
        } else status=state->generation->api().prepare_gpu_scene(&scene_request,&state->gpu_scene);
        timer.value.scene_ms=PreparationTimer::elapsed(scene_start);
        if(status==SF_CORE_OK && state->gpu_scene.status==SF_CORE_OK) {
            state->gpu_world_width=static_cast<A_long>(predicted.width);state->gpu_world_height=static_cast<A_long>(predicted.height);
            gpu_possible=true;
        } else if(status!=SF_CORE_UNSUPPORTED_FORMAT) {
            if(status!=SF_CORE_CANCELLED) report_core_failure(out_data,state->gpu_scene.detail);
            else out_data->return_msg[0]='\0';
            return host_error_for_status(status,cancellation.abort_error());
        }
    }
    extra->output->result_rect = result;
    extra->output->max_result_rect = layer_rect;
    extra->output->solid = PF_Boolean{0}; // the composite always carries alpha
    extra->output->flags = gpu_possible?PF_RenderOutputFlag_GPU_RENDER_POSSIBLE:0;
    extra->output->pre_render_data = state.release();
    extra->output->delete_pre_render_data_func = delete_pre_render_state;
    timer.value.complete=true;
    return PF_Err_NONE;
} catch (const std::bad_alloc&) {
    return PF_Err_OUT_OF_MEMORY;
} catch (...) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
}

PF_Err smart_render(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra) noexcept {
    ExecutionTimer timer(in_data);
    if (in_data == nullptr || out_data == nullptr || extra == nullptr || extra->input == nullptr ||
        extra->cb == nullptr || extra->cb->checkout_layer_pixels == nullptr ||
        extra->cb->checkout_output == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    const auto* state = static_cast<const PreRenderState*>(extra->input->pre_render_data);
    if (state == nullptr || !state->generation || state->graph_bytes.empty()) {
        // Render without a matching pre-render cannot know the layer geometry; fail
        // loudly instead of guessing coordinates.
        report_failure(out_data, core::make_error(core::ErrorCode::internal_failure, "pre-render state missing"));
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    timer.value.prepared_seconds=state->scale?double(state->time)/state->scale:0;
    timer.value.paired=state->scale && in_data->time_scale &&
        std::int64_t(state->time)*in_data->time_scale==std::int64_t(in_data->current_time)*state->scale &&
        std::int64_t(state->step)*in_data->time_scale==std::int64_t(in_data->time_step)*state->scale;
    // GPU scenes and historical snapshots contain particles for exactly one
    // time. A mismatched callback pair must never silently reuse those pixels.
    if(!timer.value.paired) {
        report_core_failure(out_data,"SmartFX render time differs from its prepared scene");
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    HostBitDepth depth{};
    if (!classify_bit_depth(extra->input->bitdepth, depth)) {
        return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    }

    // SmartFX requires one pixel checkout for each pre-render layer checkout, and AE
    // requires an input pixel checkout before output checkout. The pre-render request
    // is empty, and this pixel world is deliberately ignored by render_frame().
    PF_EffectWorld* unused_input_world = nullptr;
    const auto input_err = extra->cb->checkout_layer_pixels(in_data->effect_ref, kInputCheckoutId,
                                                            &unused_input_world);
    if (input_err != PF_Err_NONE) return input_err;
    struct InputCheckin {
        PF_InData* in;
        PF_SmartRenderExtra* extra;
        ~InputCheckin() {
            if (extra->cb->checkin_layer_pixels)
                (void)extra->cb->checkin_layer_pixels(in->effect_ref, kInputCheckoutId);
        }
    } input_checkin{in_data, extra};

    HostCancellation texture_cancel(in_data);TextureStaging textures;
    if(const auto error=stage_texture_resources(in_data,out_data,extra,state->textures,depth,texture_cancel,textures);error)return error;

    PF_EffectWorld* output_world = nullptr;
    const PF_Err output_err = extra->cb->checkout_output(in_data->effect_ref, &output_world);
    if (output_err != PF_Err_NONE) {
        return output_err;
    }
    if (output_world == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_Err err{};
    if(extra->input->what_gpu!=PF_GPU_Framework_NONE) {
        if(state->gpu_scene.status!=SF_CORE_OK || state->gpu_scene.struct_size!=sizeof(state->gpu_scene) ||
            output_world->width!=state->gpu_world_width || output_world->height!=state->gpu_world_height)
            return PF_Err_BAD_CALLBACK_PARAM;
        // Ratio is 1 here, as checked against the pre-render world's dimensions.
        // The actual GPU buffer is validated by GPU suites, never world.data.
        err=render_gpu_scene(in_data,out_data,extra->input->gpu_data,extra->input->what_gpu,extra->input->device_index,
            state->gpu_scene,output_world,output_world->origin_x,output_world->origin_y,true,
            state->motion.enabled?static_cast<unsigned>(state->motion.samples.size()):1u,static_cast<float>(state->motion.gain));
    } else {err=render_frame(in_data,out_data,depth,*state,output_world,textures);if(!err) record_cpu_execution();}

    timer.value.complete=err==PF_Err_NONE;
    return err;
}

} // namespace starfield::adapter
