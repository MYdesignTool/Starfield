#include "Diagnostics.hpp"

#include "Parameters.hpp"
#include "WorldBridge.hpp"

#include "starfield/core/Geometry.hpp"
#include "starfield/core/ParticleSimulation.hpp"

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

} // namespace

PF_Err report_diagnostics(PF_InData* in_data, PF_OutData* out_data) noexcept {
    if (in_data == nullptr || out_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    MessageWriter writer(out_data);
    writer.line("Starfield 0.1.0 graph readout\n");

    A_long control_source = -1;
    try {
        std::shared_ptr<const core::Graph> graph;
        const auto graph_err = checkout_render_graph(in_data, out_data, graph, &control_source);
        if (graph_err != PF_Err_NONE) return graph_err;
        const core::NeverCancelled never;
        const auto evaluated = core::evaluate_particle_graph(*graph,
            core::RationalTime{in_data->current_time, in_data->time_scale}, never);
        if (!evaluated.has_value()) {
            writer.line("graph evaluation failed: %s\n", evaluated.error().detail);
            return PF_Err_NONE;
        }
        writer.line("graph nodes %llu edges %llu live %llu\n",
            static_cast<unsigned long long>(graph->nodes.size()), static_cast<unsigned long long>(graph->edges.size()),
            static_cast<unsigned long long>(evaluated.value().particles.size()));
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
    writer.line(control_source == kLegacyControlSource
        ? "AE Controls (driving render)\n" : "AE Controls (inactive in Node Graph mode)\n");

    // Raw host fields. The render grid is derived in the render phase from the world
    // the host hands over (ADR 0005); the downsample factor is reported for context
    // only, because the SDK documents it inconsistently.
    writer.line("layer %ldx%ld ds %ld/%lu par %ld/%lu\n",
                static_cast<long>(in_data->width), static_cast<long>(in_data->height),
                static_cast<long>(in_data->downsample_x.num), static_cast<unsigned long>(in_data->downsample_x.den),
                static_cast<long>(in_data->pixel_aspect_ratio.num),
                static_cast<unsigned long>(in_data->pixel_aspect_ratio.den));

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

    const double scale = static_cast<double>(in_data->time_scale);
    const double seconds = scale > 0.0 ? static_cast<double>(in_data->current_time) / scale : 0.0;

    const core::NeverCancelled never;
    const auto particles = core::simulate_particles(validated, seconds, never);
    if (particles.has_value()) {
        writer.line("t %.3fs live %llu\n", seconds, static_cast<unsigned long long>(particles.value().size()));
    } else {
        writer.line("t %.3fs simulate failed: %s\n", seconds, core::describe(particles.error().code));
    }

    writer.line("cnt %u rate %.2f seed %u life %.3f\n", settings.particle_count, settings.birth_rate, settings.seed,
                settings.particle_lifetime_seconds);
    writer.line("shape %u esize %.3f vspread %.2f\n", static_cast<unsigned>(settings.emitter_shape),
                settings.emitter_size, settings.velocity_spread);
    writer.line("size %.2f notices %llu\n", settings.particle_size,
                static_cast<unsigned long long>(validated.notices.size()));

    // Shows the raw host point values next to the layer pixels they were interpreted
    // as, which is what identifies a unit mismatch in the host delivery.
    const double layer_width = static_cast<double>(in_data->width > 0 ? in_data->width : 1);
    const double layer_height = static_cast<double>(in_data->height > 0 ? in_data->height : 1);
    writer.line("origin host %.0f,%.0f,%.0f px %.0f,%.0f,%.0f\n", raw_origin.x, raw_origin.y, raw_origin.z,
                core::host_point_component_to_layer_pixels(raw_origin.x, layer_width),
                core::host_point_component_to_layer_pixels(raw_origin.y, layer_height),
                core::host_point_component_to_layer_pixels(raw_origin.z, layer_height));
    writer.line("origin world %.3f,%.3f,%.3f vel %.2f,%.2f,%.2f\n", settings.emitter_origin.x,
                settings.emitter_origin.y, settings.emitter_origin.z, settings.velocity.x, settings.velocity.y,
                settings.velocity.z);
    return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
