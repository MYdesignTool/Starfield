#include "SmartRender.hpp"

#include "AE_Macros.h"
#include "CoreLoader.hpp"
#include "Diagnostics.hpp"
#include "Parameters.hpp"
#include "WorldBridge.hpp"

#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <memory>
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
    // std::snprintf instead of the host formatter: this path must not depend on a
    // callback that may be unavailable when a render fails.
    std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Starfield Particle: %s (%s)",
                  core::describe(error.code), error.detail);
    // Deliberately not PF_OutFlag_DISPLAY_ERROR_MESSAGE: a failing preview frame
    // should land in AE's error list instead of interrupting every render.
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
    core::OpaqueBytes graph_bytes;
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

PF_Err render_frame(PF_InData* in_data, PF_OutData* out_data, HostBitDepth depth, const PreRenderState& state,
                    PF_EffectWorld* output_world) noexcept {
    WorldLayout output_layout{};
    if (!describe_world(*output_world, output_layout)) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }

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

    try {
        HostCancellation cancellation(in_data);
        SfCoreRenderRequest request{};
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
        request.is_cancelled = [](void* context) -> std::int32_t {
            return static_cast<HostCancellation*>(context)->is_cancelled() ? 1 : 0;
        };
        request.cancel_context = &cancellation;
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
            report_core_failure(out_data, rendered.detail);
            return host_error_for_status(status != SF_CORE_OK ? status : rendered.status,
                                         cancellation.abort_error());
        }
        if (rendered.pixel_byte_count > std::numeric_limits<std::size_t>::max() ||
            (rendered.pixel_byte_count > 0 && rendered.pixels == nullptr) ||
            rendered.pixel_format != static_cast<std::uint32_t>(frame.format)) {
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

PF_Err pre_render(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) noexcept try {
    if (in_data == nullptr || extra == nullptr || extra->input == nullptr || extra->output == nullptr ||
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
    const auto graph_err = checkout_render_graph(in_data, out_data, graph, nullptr,
                                                 input_result.ref_width, input_result.ref_height);
    if (graph_err != PF_Err_NONE) {
        // Failing here is safe resource-wise: pre-render checkouts belong to the frame,
        // only the smart-render phase checks them in, and AE tears the frame down when
        // pre-render reports an error. (PF_PreRenderCallbacks has no checkin callback.)
        return graph_err;
    }
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
    state->graph_bytes = encoded.take_value();
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

    extra->output->result_rect = result;
    extra->output->max_result_rect = layer_rect;
    extra->output->solid = PF_Boolean{0}; // the composite always carries alpha
    extra->output->flags = 0;
    extra->output->pre_render_data = state.release();
    extra->output->delete_pre_render_data_func = delete_pre_render_state;
    return PF_Err_NONE;
} catch (const std::bad_alloc&) {
    return PF_Err_OUT_OF_MEMORY;
} catch (...) {
    return PF_Err_INTERNAL_STRUCT_DAMAGED;
}

PF_Err smart_render(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra) noexcept {
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

    PF_EffectWorld* output_world = nullptr;
    const PF_Err output_err = extra->cb->checkout_output(in_data->effect_ref, &output_world);
    if (output_err != PF_Err_NONE) {
        return output_err;
    }
    if (output_world == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    const PF_Err err = render_frame(in_data, out_data, depth, *state, output_world);

    return err;
}

} // namespace starfield::adapter
