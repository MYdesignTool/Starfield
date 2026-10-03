#include "Diagnostics.hpp"
#include "CoreLoader.hpp"
#include "GpuRender.hpp"
#include "NativeTemporalCache.hpp"
#include "NativeTemporalUI.hpp"
#include "NativeNodeGraph.hpp"
#include "GraphCarrier.hpp"
#include "EmitterHistory.hpp"
#include "SmartRender.hpp"

#include "Parameters.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "WorldBridge.hpp"

#include "starfield/core/Geometry.hpp"
#include "starfield/core/ParticleSimulation.hpp"

#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace starfield::adapter {
namespace {

namespace core = starfield::core;

// PF_OutData::return_msg is PF_MAX_EFFECT_MSG_LEN + 1 bytes, so the readout has to
// stay compact. The writer truncates instead of overflowing.
class MessageWriter {
public:
    explicit MessageWriter(PF_OutData* out_data) noexcept
        : buffer_(out_data->return_msg), capacity_(sizeof(out_data->return_msg)) {
        buffer_[0] = '\0';
    }

    void line(const char* format, ...) noexcept {
        if (used_ + 1 >= capacity_) {
            return;
        }
        va_list arguments;
        va_start(arguments, format);
        const int written = std::vsnprintf(buffer_ + used_, capacity_ - used_, format, arguments);
        va_end(arguments);
        if (written > 0) {
            const auto advance = static_cast<std::size_t>(written);
            used_ += (advance < capacity_ - used_) ? advance : (capacity_ - 1 - used_);
        }
    }

private:
    char* buffer_{nullptr};
    std::size_t capacity_{0};
    std::size_t used_{0};
};

// Atomics rather than plain fields: AE can render effect instances on different threads,
// and a diagnostic that tears would be worse than no diagnostic.
struct RenderGeometryStore {
    std::atomic<long> layer_width{0};
    std::atomic<long> layer_height{0};
    std::atomic<long> ref_width{0};
    std::atomic<long> ref_height{0};
    std::atomic<long> grid_width{0};
    std::atomic<long> grid_height{0};
    std::atomic<long> par_num{0};
    std::atomic<long> par_den{0};
    std::atomic<bool> valid{false};
};

RenderGeometryStore g_last_render;

} // namespace

void record_render_geometry(A_long layer_width, A_long layer_height, A_long ref_width, A_long ref_height,
                            A_long grid_width, A_long grid_height, A_long par_num, A_long par_den) noexcept {
    g_last_render.layer_width.store(layer_width, std::memory_order_relaxed);
    g_last_render.layer_height.store(layer_height, std::memory_order_relaxed);
    g_last_render.ref_width.store(ref_width, std::memory_order_relaxed);
    g_last_render.ref_height.store(ref_height, std::memory_order_relaxed);
    g_last_render.grid_width.store(grid_width, std::memory_order_relaxed);
    g_last_render.grid_height.store(grid_height, std::memory_order_relaxed);
    g_last_render.par_num.store(par_num, std::memory_order_relaxed);
    g_last_render.par_den.store(par_den, std::memory_order_relaxed);
    g_last_render.valid.store(true, std::memory_order_release);
}

RenderGeometry last_render_geometry() noexcept {
    RenderGeometry geometry;
    geometry.valid = g_last_render.valid.load(std::memory_order_acquire);
    geometry.layer_width = g_last_render.layer_width.load(std::memory_order_relaxed);
    geometry.layer_height = g_last_render.layer_height.load(std::memory_order_relaxed);
    geometry.ref_width = g_last_render.ref_width.load(std::memory_order_relaxed);
    geometry.ref_height = g_last_render.ref_height.load(std::memory_order_relaxed);
    geometry.grid_width = g_last_render.grid_width.load(std::memory_order_relaxed);
    geometry.grid_height = g_last_render.grid_height.load(std::memory_order_relaxed);
    geometry.par_num = g_last_render.par_num.load(std::memory_order_relaxed);
    geometry.par_den = g_last_render.par_den.load(std::memory_order_relaxed);
    return geometry;
}

PF_Err report_diagnostics(PF_InData* in_data, PF_OutData* out_data) noexcept {
    if (in_data == nullptr || out_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    MessageWriter writer(out_data);
    const auto loaded = reload_core();
    if (loaded.changed) out_data->out_flags |= PF_OutFlag_FORCE_RERENDER;
    if (!loaded.error.empty()) writer.line("Core reload: %s\n", loaded.error.c_str());
    else writer.line("Core: %s\n", loaded.changed ? "new DLL loaded" : "current DLL");
    if (!loaded) {
        out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
        return PF_Err_NONE;
    }

    // PF_Cmd_DO_DIALOG can arrive without a render context: the SDK documents it as
    // "after SEQUENCE_SETUP", and PF_OutFlag_SEND_DO_DIALOG can request it once when
    // the effect is applied. Parameter checkouts and the graph are only meaningful
    // while a frame is being rendered, so a weak context reports the always-valid
    // host fields and never touches parameter handles.
    const bool render_context = in_data->width > 0 && in_data->height > 0 && in_data->time_scale > 0 &&
                                in_data->inter.checkout_param != nullptr && in_data->inter.checkin_param != nullptr;
    if (!render_context) {
        writer.line("Starfield 0.1.0: no render context yet (apply/sequence setup); open a comp and render a frame, "
                    "then press Options\n");
        return PF_Err_NONE;
    }

    out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    // PF_OutData::return_msg holds 255 characters. Keep the stable render identity,
    // shape and preview geometry ahead of optional controls; avoid printing two
    // identical world triples on the common path.
    const auto gpu=last_gpu_execution();
    if(gpu.rendered) writer.line("Last frame: %s device%lu\n",gpu.framework==PF_GPU_Framework_CUDA?"CUDA":gpu.framework==PF_GPU_Framework_OPENCL?"OpenCL":"CPU",static_cast<unsigned long>(gpu.device_index));
    else writer.line("Last frame: not rendered\n");
    const auto execution=last_smart_render_timing();
    if(execution.valid)writer.line("Render t%.3f/p%.3f %.1fms%s\n",execution.seconds,execution.prepared_seconds,execution.total_ms,execution.complete?"":" failed");
    // Snapshot real render preparation BEFORE UI proof capture. A UI-side proof
    // count alone cannot establish that the renderer matched or used any proof.
    const auto timing=last_pre_render_timings();
    const auto history=last_native_history_trace();
    if(timing.valid) {
        writer.line("Last prep t%.3f %.1fms%s\n",timing.seconds,timing.total_ms,timing.complete?"":" failed");
        writer.line("C/H/S %.1f/%.1f/%.1fms\n",timing.controls_ms,timing.history_ms,timing.scene_ms);
    }
    if(history.path!=NativeHistoryPath::unavailable) {
        writer.line("%s %zu/%zu PF%llu R%llu L%llu N%llu/%llu\n",
            history.path==NativeHistoryPath::static_graph?"Static":history.path==NativeHistoryPath::temporal?"Temporal":"Failed",
            history.constants,history.inputs,static_cast<unsigned long long>(history.checkouts),
            static_cast<unsigned long long>(history.rate_queries),static_cast<unsigned long long>(history.life_queries),
            static_cast<unsigned long long>(history.node_queries),static_cast<unsigned long long>(history.node_samples));
    }
    const auto setup=last_gpu_setup_timing();const auto ui=last_native_ui_timing();
    writer.line("Init max%.1fms/%llu UI max%.1fms/%llu\n",setup.max_ms,static_cast<unsigned long long>(setup.calls),
        ui.max_ms,static_cast<unsigned long long>(ui.refreshes));
    A_long control_source = -1;
    try {
        std::shared_ptr<const core::Graph> graph;
        const auto graph_err = checkout_render_graph(in_data, out_data, graph, &control_source);
        if (graph_err != PF_Err_NONE) return graph_err;
        // Existing development projects retain their generated expressions.
        // Explicit Options refresh upgrades only our owned numeric bindings;
        // native keyframes/expressions and authored graph values are preserved.
        NativeBindingTransaction bindings(in_data,graph_carrier_plugin_id());
        const auto binding_error=bindings.install(*graph);
        if(binding_error)return binding_error;
        bindings.accept();
        capture_native_temporal_metadata(in_data,*graph,graph_carrier_plugin_id());
        std::vector<core::NodeId> ids;for(const auto& node:graph->nodes)ids.push_back(node.id);
        const auto certified=validated_native_control_proofs(in_data,ids).size();
        writer.line("Bindings v27; proofs %zu\n",certified);
        if(certified)out_data->out_flags|=PF_OutFlag_FORCE_RERENDER;
        const auto encoded = core::serialize_graph(*graph, core::particle_node_registry());
        if (!encoded.has_value()) {
            writer.line("SF 0.1.0 graph serialization failed: %s\n", encoded.error().detail);
            return PF_Err_NONE;
        }
        SfCoreInspectRequest inspect_request{};
        inspect_request.struct_size = sizeof(inspect_request);
        inspect_request.graph_bytes = encoded.value().data();
        inspect_request.graph_byte_count = encoded.value().size();
        inspect_request.time_value = in_data->current_time;
        inspect_request.time_scale = in_data->time_scale;
        SfCoreInspectResult evaluated{};
        evaluated.struct_size = sizeof(evaluated);
        if (loaded.generation->api().inspect(&inspect_request, &evaluated) != SF_CORE_OK) {
            writer.line("SF 0.1.0 graph evaluation failed: %s\n", evaluated.detail);
            return PF_Err_NONE;
        }
        writer.line("SF %s g%llu/%llu live%llu\n",
                    control_source == kLegacyControlSource ? "AE" : "NG",
                    static_cast<unsigned long long>(evaluated.node_count),
                    static_cast<unsigned long long>(evaluated.edge_count),
                    static_cast<unsigned long long>(evaluated.live_particle_count));
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
    if (graph_parameter_disabled()) {
        writer.line("SF probe: STARFIELD_NO_GRAPH_PARAM, no arbitrary data\n");
    } else if (flat_render_override_active()) {
        writer.line("SF override: STARFIELD_FLAT_RENDER, flat controls\n");
    }

    // `layer` and `ds` are what the host reports for this call. `ref` and `grid` are what
    // the last rendered frame actually used. AE 2023.5 Build 52 scales point controls by
    // the preview factor, so `px` below shows their full-resolution interpretation after
    // reversing that factor; the core maps world space onto `grid`.
    const RenderGeometry last = last_render_geometry();
    writer.line("L%ldx%ld ds%ld/%lu,%ld/%lu ref%ldx%ld grid%ldx%ld\n",
                static_cast<long>(in_data->width), static_cast<long>(in_data->height),
                static_cast<long>(in_data->downsample_x.num), static_cast<unsigned long>(in_data->downsample_x.den),
                static_cast<long>(in_data->downsample_y.num), static_cast<unsigned long>(in_data->downsample_y.den),
                static_cast<long>(last.valid ? last.ref_width : 0),
                static_cast<long>(last.valid ? last.ref_height : 0),
                static_cast<long>(last.valid ? last.grid_width : 0),
                static_cast<long>(last.valid ? last.grid_height : 0));

    ScopedParameterCheckin parameters(in_data);
    const PF_Err checkout_error = parameters.snapshot().checkout(in_data);
    if (checkout_error != PF_Err_NONE) {
        writer.line("parameter checkout failed: %ld\n", static_cast<long>(checkout_error));
        return PF_Err_NONE;
    }

    const core::Settings& settings = parameters.snapshot().settings();
    // Keep allocations inside the ABI error boundary even in this diagnostic path.
    try {
    const core::ValidatedSettings validated = core::validate_settings(settings);
    const core::Vec3& raw_origin = parameters.snapshot().raw_origin();
    writer.line("shape%u esz%.3f vspr%.2f sz%.2f not%llu\n",
                static_cast<unsigned>(settings.emitter_shape), settings.emitter_size,
                settings.velocity_spread, settings.particle_size,
                static_cast<unsigned long long>(validated.notices.size()));

    // The raw host point next to the layer pixels it was read as, which is what
    // identifies a unit mismatch in the host delivery. The first world triple is the one
    // this call computed from the sizes above (the fallback path); the second is what the
    // last rendered frame computed from its full-resolution reference.
    const core::Vec3 layer_pixels = point_control_to_full_resolution_pixels(raw_origin, *in_data);
    writer.line("org%.0f,%.0f,%.0f px%.0f,%.0f,%.0f\n", raw_origin.x, raw_origin.y, raw_origin.z,
                layer_pixels.x, layer_pixels.y, layer_pixels.z);
    if (last.valid && last.ref_width > 0 && last.ref_height > 0) {
        core::LayerUnits ref_units;
        ref_units.layer_width = static_cast<double>(last.ref_width);
        ref_units.layer_height = static_cast<double>(last.ref_height);
        ref_units.pixel_aspect_ratio = (last.par_num > 0 && last.par_den > 0)
            ? static_cast<double>(last.par_num) / static_cast<double>(last.par_den) : 1.0;
        const core::Vec3 ref_pixels = point_control_to_full_resolution_pixels(raw_origin, *in_data);
        const core::Vec3 ref_world = core::layer_point_to_world(ref_pixels.x, ref_pixels.y, ref_pixels.z, ref_units);
        const bool same_world = std::fabs(settings.emitter_origin.x - ref_world.x) < 0.0005 &&
                                std::fabs(settings.emitter_origin.y - ref_world.y) < 0.0005 &&
                                std::fabs(settings.emitter_origin.z - ref_world.z) < 0.0005;
        if (same_world) {
            writer.line("world %.3f,%.3f,%.3f\n", ref_world.x, ref_world.y, ref_world.z);
        } else {
            writer.line("world l%.3f,%.3f,%.3f r%.3f,%.3f,%.3f\n", settings.emitter_origin.x,
                        settings.emitter_origin.y, settings.emitter_origin.z,
                        ref_world.x, ref_world.y, ref_world.z);
        }
    } else {
        writer.line("world %.3f,%.3f,%.3f ref:none\n", settings.emitter_origin.x, settings.emitter_origin.y,
                    settings.emitter_origin.z);
    }

    // Printed only when they differ from the defaults, so the geometry above keeps its
    // place in the 255-character budget while the controls are still the default.
    if (settings.gravity.x != 0.0 || settings.gravity.y != 0.0 || settings.gravity.z != 0.0 ||
        settings.linear_drag != 0.0) {
        writer.line("grav %.2f,%.2f,%.2f drag %.3f\n", settings.gravity.x, settings.gravity.y, settings.gravity.z,
                    settings.linear_drag);
    }
    if (settings.color_start.x != settings.color_end.x || settings.color_start.y != settings.color_end.y ||
        settings.color_start.z != settings.color_end.z || settings.particle_size_end != 100.0 ||
        settings.opacity_end != 100.0) {
        writer.line("col %.2f,%.2f,%.2f>%.2f,%.2f,%.2f size %.2fpx curve %.1f%% op %.3f curve %.1f%%\n",
                    settings.color_start.x, settings.color_start.y, settings.color_start.z,
                    settings.color_end.x, settings.color_end.y, settings.color_end.z,
                    settings.particle_size, settings.particle_size_end, settings.opacity, settings.opacity_end);
    }

    const double scale = static_cast<double>(in_data->time_scale);
    const double seconds = scale > 0.0 ? static_cast<double>(in_data->current_time) / scale : 0.0;
    writer.line("t%.3f vel%.2f,%.2f,%.2f\n", seconds, settings.velocity.x, settings.velocity.y,
                settings.velocity.z);
    writer.line("cnt%u rate%.2f seed%u life%.3f\n", settings.particle_count, settings.birth_rate, settings.seed,
                settings.particle_lifetime_seconds);
    return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
