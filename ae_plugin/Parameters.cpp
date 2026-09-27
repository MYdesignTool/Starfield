#include "Parameters.hpp"

#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "WorldBridge.hpp"

#include "starfield/core/Geometry.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

namespace starfield::adapter {
namespace {

namespace core = starfield::core;

// Parameter IDs come from schema/parameters.json and are project-file
// compatibility keys. Disk IDs only have to be unique inside this effect.
constexpr A_long kParticleCountDiskId = 'pcnt';
constexpr A_long kBirthRateDiskId = 'brth';
constexpr A_long kSeedDiskId = 'seed';
constexpr A_long kLifetimeDiskId = 'life';
constexpr A_long kEmitterShapeDiskId = 'esha';
constexpr A_long kEmitterOriginDiskId = 'epos';
constexpr A_long kVelocityXDiskId = 'velx';
constexpr A_long kVelocityYDiskId = 'vely';
constexpr A_long kVelocityZDiskId = 'velz';
constexpr A_long kSizeDiskId = 'size';
constexpr A_long kOpacityDiskId = 'opac';
constexpr A_long kEmitterSizeDiskId = 'esiz';
constexpr A_long kVelocitySpreadDiskId = 'vspd';
constexpr A_long kGravityXDiskId = 'grvx';
constexpr A_long kGravityYDiskId = 'grvy';
constexpr A_long kGravityZDiskId = 'grvz';
constexpr A_long kLinearDragDiskId = 'drag';
constexpr A_long kColorStartDiskId = 'clrs';
constexpr A_long kColorEndDiskId = 'clre';
constexpr A_long kParticleSizeEndDiskId = 'szen';
constexpr A_long kOpacityEndDiskId = 'open';

// Parameter types and host indices in binding order: slot 0..20 map to the manifest
// controls. Types are stamped into the checked-out PF_ParamDef before
// PF_CHECKOUT_PARAM so the host cannot be confused about how to fill the value
// union. Hosts that fill by index are unaffected.
constexpr A_long kParameterIndices[kEffectParameterCount] = {
    1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, // Particle Count .. Velocity Spread
    17, 18, 19, 20, 21, 22, 23, 24,                     // Gravity X .. Opacity End
};
constexpr A_long kParameterTypes[kEffectParameterCount] = {
    PF_Param_FLOAT_SLIDER, /* 1  Particle Count    */
    PF_Param_FLOAT_SLIDER, /* 2  Birth Rate        */
    PF_Param_FLOAT_SLIDER, /* 3  Random Seed       */
    PF_Param_FLOAT_SLIDER, /* 4  Particle Lifetime */
    PF_Param_POPUP,        /* 5  Emitter Shape     */
    PF_Param_POINT_3D,     /* 6  Emitter Origin    */
    PF_Param_FLOAT_SLIDER, /* 7  Velocity X        */
    PF_Param_FLOAT_SLIDER, /* 8  Velocity Y        */
    PF_Param_FLOAT_SLIDER, /* 9  Velocity Z        */
    PF_Param_FLOAT_SLIDER, /* 10 Particle Size     */
    PF_Param_FLOAT_SLIDER, /* 11 Opacity           */
    PF_Param_FLOAT_SLIDER, /* 12 Emitter Size      */
    PF_Param_FLOAT_SLIDER, /* 13 Velocity Spread   */
    PF_Param_FLOAT_SLIDER, /* 17 Gravity X         */
    PF_Param_FLOAT_SLIDER, /* 18 Gravity Y         */
    PF_Param_FLOAT_SLIDER, /* 19 Gravity Z         */
    PF_Param_FLOAT_SLIDER, /* 20 Linear Drag       */
    PF_Param_COLOR,        /* 21 Color Start       */
    PF_Param_COLOR,        /* 22 Color End         */
    PF_Param_FLOAT_SLIDER, /* 23 Size End          */
    PF_Param_FLOAT_SLIDER, /* 24 Opacity End       */
};

std::uint32_t to_particle_count(const PF_ParamDef& def) noexcept {
    const double value = static_cast<double>(def.u.fs_d.value);
    if (!std::isfinite(value) || value <= 0.0) {
        return 0;
    }
    if (value >= static_cast<double>(core::kMaxParticleCount)) {
        return core::kMaxParticleCount;
    }
    return static_cast<std::uint32_t>(value + 0.5);
}

std::uint32_t to_seed(const PF_ParamDef& def) noexcept {
    const double value = static_cast<double>(def.u.fs_d.value);
    if (!std::isfinite(value) || value <= 0.0) {
        return 0;
    }
    if (value >= static_cast<double>(core::kMaxSeed)) {
        return core::kMaxSeed;
    }
    return static_cast<std::uint32_t>(value + 0.5);
}

// Float sliders are converted without clamping so that non-finite host values
// reach validate_settings, which replaces them with the documented default.
double to_double(const PF_ParamDef& def) noexcept {
    return static_cast<double>(def.u.fs_d.value);
}

// AE popup values are one-based; the core enum is zero-based (see M2-02 notes in
// docs/parameter-mapping.md).
std::uint32_t popup_index(const PF_ParamDef& def) noexcept {
    const A_long value = def.u.pd.value;
    if (value <= 1) {
        return 0;
    }
    return static_cast<std::uint32_t>(value - 1);
}

// Color controls deliver 8-bit channels (PF_ColorDef). The core stores
// working-space 0..1 channel values and M2 performs no color-space conversion
// (ADR 0005); the alpha channel is owned by the opacity controls.
core::Vec3 to_color(const PF_ParamDef& def) noexcept {
    const PF_Pixel& pixel = def.u.cd.value;
    return core::Vec3{static_cast<double>(pixel.red) / 255.0, static_cast<double>(pixel.green) / 255.0,
                      static_cast<double>(pixel.blue) / 255.0};
}

// True when the parameter index is one of the manifest controls; every bound
// control feeds the canonical graph in Node Graph mode.
constexpr bool is_bound_control(A_long index) noexcept {
    return (index >= kFirstEffectParameterId && index <= 13) ||
           (index >= kGravityXId && index <= kLastEffectParameterId);
}

// Maps one delivered value per manifest control (binding order, see
// kParameterIndices) into core settings. The render checkout and the supervised
// panel edit share this path so both see identical conversions. Values are not
// clamped here: validate_settings owns every bound.
core::Settings settings_from_controls(const PF_ParamDef* const* defs, PF_InData& in_data,
                                      core::Vec3* raw_origin) noexcept {
    core::Settings settings;

    settings.particle_count = to_particle_count(*defs[0]);
    settings.birth_rate = to_double(*defs[1]);
    settings.seed = to_seed(*defs[2]);
    settings.particle_lifetime_seconds = to_double(*defs[3]);
    settings.emitter_shape = core::emitter_shape_from_index(popup_index(*defs[4]));

    // Emitter Origin is an AE point control. AE delivers *absolute layer pixels* with
    // the origin at the layer's top-left, x growing right and y growing down; the
    // control's default is a percentage where 50 means "halfway" (SDK header note for
    // PF_Point3DDef). The core wants layer heights with the origin at the layer centre
    // and +Y up, so the conversion happens in testable core helpers (ADR 0003,
    // docs/parameter-mapping.md).
    const PF_Point3DDef& origin = defs[5]->u.point3d_d;
    const core::Vec3 raw{static_cast<double>(origin.x_value), static_cast<double>(origin.y_value),
                         static_cast<double>(origin.z_value)};
    if (raw_origin) *raw_origin = raw;

    const double layer_width = static_cast<double>(in_data.width > 0 ? in_data.width : 1);
    const double layer_height = static_cast<double>(in_data.height > 0 ? in_data.height : 1);
    core::LayerUnits units;
    units.layer_width = layer_width;
    units.layer_height = layer_height;
    units.pixel_aspect_ratio = host_pixel_aspect_ratio(in_data);
    settings.emitter_origin = core::layer_point_to_world(
        core::host_point_component_to_layer_pixels(raw.x, layer_width),
        core::host_point_component_to_layer_pixels(raw.y, layer_height),
        core::host_point_component_to_layer_pixels(raw.z, layer_height), units);

    // The velocity sliders are already in layer heights per second, which is the
    // core's world unit, so they need no conversion.
    settings.velocity.x = to_double(*defs[6]);
    settings.velocity.y = to_double(*defs[7]);
    settings.velocity.z = to_double(*defs[8]);

    settings.particle_size = to_double(*defs[9]);
    settings.opacity = to_double(*defs[10]);

    // Emitter extent (cube edge for Box, diameter for Sphere/Disc), per-axis velocity
    // jitter, gravity (layer heights per second squared), linear drag (inverse
    // seconds) and the age-curve endpoints all share the core's world units.
    settings.emitter_size = to_double(*defs[11]);
    settings.velocity_spread = to_double(*defs[12]);
    settings.gravity = core::Vec3{to_double(*defs[13]), to_double(*defs[14]), to_double(*defs[15])};
    settings.linear_drag = to_double(*defs[16]);
    settings.color_start = to_color(*defs[17]);
    settings.color_end = to_color(*defs[18]);
    settings.particle_size_end = to_double(*defs[19]);
    settings.opacity_end = to_double(*defs[20]);
    return settings;
}

} // namespace

PF_Err setup_parameters(PF_InData* in_data, PF_OutData* out_data) noexcept {
    if (in_data == nullptr || out_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_ParamDef def;
    PF_Err err = PF_Err_NONE;

    // Labels, ranges, precision, and defaults mirror schema/parameters.json exactly.
    // Float literals match PF_FpShort so no narrowing warning is emitted at /W4.
    // Every bound control is supervised: an edit in Node Graph mode rewrites the
    // canonical graph in the same user-change transaction (ADR 0009).
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Count", 0.0f, 2000000.0f, 0.0f, 2000000.0f, 1000.0f, PF_Precision_INTEGER,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kParticleCountDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Birth Rate", 0.0f, 1000000.0f, 0.0f, 1000000.0f, 30.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kBirthRateDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Random Seed", 0.0f, 2147483647.0f, 0.0f, 2147483647.0f, 1.0f, PF_Precision_INTEGER,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSeedDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Lifetime", 0.0f, 1000000.0f, 0.0f, 1000000.0f, 2.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kLifetimeDiskId);

    // Emitter Shape and Emitter Origin are registered by hand instead of through
    // PF_ADD_POPUP/PF_ADD_POINT_3D: those macros call PF_ADD_PARAM themselves and
    // never set def.flags, and both controls must be supervised for the panel path.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = PF_ParamFlag_SUPERVISE;
    std::snprintf(def.name, sizeof(def.name), "Emitter Shape");
    def.uu.id = kEmitterShapeDiskId;
    def.u.pd.num_choices = 4;
    def.u.pd.dephault = 1; // AE popup values are one-based: 1 is Point
    def.u.pd.value = def.u.pd.dephault;
    def.u.pd.u.namesptr = "Point|Box|Sphere|Disc";
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    // Position control: AE owns the on-screen picking behavior for point params.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POINT_3D;
    def.flags = PF_ParamFlag_SUPERVISE;
    std::snprintf(def.name, sizeof(def.name), "Emitter Origin");
    def.uu.id = kEmitterOriginDiskId;
    def.u.point3d_d.x_value = def.u.point3d_d.x_dephault = 50.0;
    def.u.point3d_d.y_value = def.u.point3d_d.y_dephault = 50.0;
    def.u.point3d_d.z_value = def.u.point3d_d.z_dephault = 50.0;
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity X", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kVelocityXDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Y", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.3f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kVelocityYDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Z", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kVelocityZDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Size", 0.0f, 100000.0f, 0.0f, 100000.0f, 8.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSizeDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Opacity", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kOpacityDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Emitter Size", 0.0f, 10.0f, 0.0f, 1.0f, 0.05f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kEmitterSizeDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Spread", 0.0f, 100.0f, 0.0f, 1.0f, 0.15f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kVelocitySpreadDiskId);

    // The graph's default handle becomes host-owned only after successful ADD_PARAM.
    PF_ArbitraryH default_graph = nullptr;
    PF_ArbParamsExtra create{};
    create.id = kGraphParameterId;
    create.which_function = PF_Arbitrary_NEW_FUNC;
    create.u.new_func_params.arbPH = &default_graph;
    err = graph_arbitrary_callback(in_data, &create);
    if (err != PF_Err_NONE) return err;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_ARBITRARY_DATA;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "Node Graph Data");
    def.uu.id = def.u.arb_d.id = kGraphParameterId;
    def.u.arb_d.dephault = default_graph;
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) { in_data->utils->host_dispose_handle(default_graph); return err; }

    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS;
    std::snprintf(def.name, sizeof(def.name), "Control Source");
    def.uu.id = kControlSourceId;
    def.u.pd.num_choices = 2;
    def.u.pd.dephault = kNodeControlSource;
    def.u.pd.value = kLegacyControlSource;
    def.u.pd.u.namesptr = "AE Controls|Node Graph";
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    PF_ADD_BUTTON("Capture Current Controls", "Capture at Current Time", PF_PUI_NONE,
                  PF_ParamFlag_SUPERVISE, kCaptureControlsId);

    // Force and appearance controls (IDs 17..24). They are appended after the
    // graph/system parameters because parameter IDs are registration order and
    // 14..16 are already stored in existing projects. Defaults reproduce the M2 look
    // (no gravity, no drag, white sprites, constant size/opacity), so a fresh
    // instance renders exactly as before until one of these controls is changed.
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Gravity X", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityXDiskId);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Gravity Y", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityYDiskId);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Gravity Z", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityZDiskId);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Linear Drag", 0.0f, 100.0f, 0.0f, 10.0f, 0.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kLinearDragDiskId);
    // PF_ADD_COLOR does not clear the struct or touch flags; set them explicitly.
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    PF_ADD_COLOR("Color Start", 255, 255, 255, kColorStartDiskId);
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    PF_ADD_COLOR("Color End", 255, 255, 255, kColorEndDiskId);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Size End", 0.0f, 100000.0f, 0.0f, 100000.0f, 8.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kParticleSizeEndDiskId);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Opacity End", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kOpacityEndDiskId);

    out_data->num_params = static_cast<A_long>(kTotalEffectParameterCount) + 1;
    return PF_Err_NONE;
}

PF_Err ParameterSnapshot::checkout(PF_InData* in_data) noexcept {
    valid_ = false;
    settings_ = core::Settings{};
    raw_origin_ = core::Vec3{};

    if (in_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        AEFX_CLR_STRUCT(defs_[i]);
        defs_[i].param_type = kParameterTypes[i];
        checked_out_[i] = false;
    }

    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        const A_long index = kParameterIndices[i];
        const PF_Err err = PF_CHECKOUT_PARAM(in_data, index, in_data->current_time, in_data->time_step,
                                            in_data->time_scale, &defs_[i]);
        if (err != PF_Err_NONE) {
            checkin(in_data);
            return err;
        }
        checked_out_[i] = true;
    }

    // One conversion path for the render checkout and the supervised panel edit;
    // the emitter-origin unit ladder lives in the shared helper.
    const PF_ParamDef* controls[kEffectParameterCount]{};
    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        controls[i] = &defs_[i];
    }
    settings_ = settings_from_controls(controls, *in_data, &raw_origin_);

    valid_ = true;
    return PF_Err_NONE;
}

void ParameterSnapshot::checkin(PF_InData* in_data) noexcept {
    if (in_data == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        if (checked_out_[i]) {
            PF_CHECKIN_PARAM(in_data, &defs_[i]);
            checked_out_[i] = false;
        }
    }
    valid_ = false;
}

namespace {
class CheckedParameter {
public:
    explicit CheckedParameter(PF_InData* data) : data_(data) {}
    ~CheckedParameter() { if (checked_) PF_CHECKIN_PARAM(data_, &value); }
    CheckedParameter(const CheckedParameter&) = delete;
    CheckedParameter& operator=(const CheckedParameter&) = delete;
    PF_Err checkout(A_long index) {
        const auto err = PF_CHECKOUT_PARAM(data_, index, data_->current_time, data_->time_step, data_->time_scale, &value);
        checked_ = err == PF_Err_NONE;
        return err;
    }
    PF_ParamDef value{};
private:
    PF_InData* data_;
    bool checked_{false};
};

PF_Err graph_error(PF_OutData* out, const core::CoreError& error) noexcept {
    if (out) std::snprintf(out->return_msg, sizeof(out->return_msg), "Starfield graph: %s", error.detail);
    return error.code == core::ErrorCode::allocation_failed ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
}
} // namespace

bool flat_render_override_active() noexcept {
    // Diagnostic escape hatch for host triage. With STARFIELD_FLAT_RENDER=1 the render
    // samples the flat AE controls instead of the stored graph, so a host-side surprise
    // can be bisected between the graph path and the flat path without a rebuild.
    // Normal runs are unaffected, the value is never persisted, and the Options readout
    // prints whether the override is active.
    const char* value = std::getenv("STARFIELD_FLAT_RENDER");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

PF_Err checkout_render_graph(PF_InData* in_data, PF_OutData* out_data,
                             std::shared_ptr<const core::Graph>& graph,
                             A_long* control_source) noexcept {
    graph.reset();
    if (control_source) *control_source = -1;
    if (!in_data || !in_data->inter.checkout_param || !in_data->inter.checkin_param) return PF_Err_BAD_CALLBACK_PARAM;
    try {
        CheckedParameter source(in_data);
        PF_Err err = source.checkout(kControlSourceId);
        if (err != PF_Err_NONE) return err;
        if (source.value.param_type != PF_Param_POPUP) return PF_Err_BAD_CALLBACK_PARAM;
        if (source.value.u.pd.value == kLegacyControlSource || flat_render_override_active()) {
            ScopedParameterCheckin legacy(in_data);
            err = legacy.snapshot().checkout(in_data);
            if (err != PF_Err_NONE) return err;
            auto converted = graph_from_controls(core::validate_settings(legacy.snapshot().settings()).value);
            if (!converted.has_value()) return graph_error(out_data, converted.error());
            graph = std::make_shared<const core::Graph>(converted.take_value());
        } else if (source.value.u.pd.value == kNodeControlSource) {
            CheckedParameter stored(in_data);
            err = stored.checkout(kGraphParameterId);
            if (err != PF_Err_NONE) return err;
            if (stored.value.param_type != PF_Param_ARBITRARY_DATA) return PF_Err_BAD_CALLBACK_PARAM;
            auto decoded = read_graph_parameter(in_data, stored.value.u.arb_d.value);
            if (!decoded.has_value()) return graph_error(out_data, decoded.error());
            graph = std::make_shared<const core::Graph>(decoded.take_value());
        } else return PF_Err_BAD_CALLBACK_PARAM;
        if (control_source) *control_source = source.value.u.pd.value;
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err capture_controls(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                         PF_UserChangedParamExtra* extra) noexcept {
    if (!in_data || !params || !extra) return PF_Err_BAD_CALLBACK_PARAM;
    if (extra->param_index != kCaptureControlsId) return PF_Err_NONE;
    if (!params[kGraphParameterId] || !params[kControlSourceId] ||
        params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
        params[kControlSourceId]->param_type != PF_Param_POPUP) return PF_Err_BAD_CALLBACK_PARAM;
    try {
        ScopedParameterCheckin current(in_data);
        const auto err = current.snapshot().checkout(in_data);
        if (err != PF_Err_NONE) return err;
        auto graph = graph_from_controls(core::validate_settings(current.snapshot().settings()).value);
        if (!graph.has_value()) return graph_error(out_data, graph.error());
        PF_ArbitraryH replacement = nullptr;
        const auto created = create_graph_parameter(in_data, graph.value(), &replacement);
        if (created != PF_Err_NONE) return created;
        // All fallible work precedes mutation. The editable parameter value is a
        // host-provided copy; its old handle is replaced, never a render checkout.
        auto& target = *params[kGraphParameterId];
        const auto old = target.u.arb_d.value;
        target.u.arb_d.value = replacement;
        target.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[kControlSourceId]->u.pd.value = kNodeControlSource;
        params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        if (old) in_data->utils->host_dispose_handle(old);
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

namespace {

// Rebuilds the canonical graph from the delivered control values. Only called from
// PF_Cmd_USER_CHANGED_PARAM, where params[] carries the accepted new values: a
// PF_CHECKOUT_PARAM during a user change can still return the pre-edit value, so
// the delivered array is the authoritative source here. All fallible work happens
// before the graph parameter is replaced.
PF_Err sync_graph_from_controls(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]) noexcept {
    if (!params[kGraphParameterId] || !params[kControlSourceId]) return PF_Err_BAD_CALLBACK_PARAM;
    if (params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
        params[kControlSourceId]->param_type != PF_Param_POPUP) return PF_Err_BAD_CALLBACK_PARAM;
    if (params[kControlSourceId]->u.pd.value != kNodeControlSource) return PF_Err_NONE; // AE Controls mode

    const PF_ParamDef* defs[kEffectParameterCount]{};
    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        const PF_ParamDef* def = params[kParameterIndices[i]];
        if (!def) return PF_Err_BAD_CALLBACK_PARAM;
        defs[i] = def;
    }

    const auto settings = core::validate_settings(settings_from_controls(defs, *in_data, nullptr));
    auto graph = graph_from_controls(settings.value);
    if (!graph.has_value()) return graph_error(out_data, graph.error());
    PF_ArbitraryH replacement = nullptr;
    const auto created = create_graph_parameter(in_data, graph.value(), &replacement);
    if (created != PF_Err_NONE) return created;
    auto& target = *params[kGraphParameterId];
    const auto old = target.u.arb_d.value;
    target.u.arb_d.value = replacement;
    target.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    if (old) in_data->utils->host_dispose_handle(old);
    return PF_Err_NONE;
}

} // namespace

PF_Err user_changed_param(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                          PF_UserChangedParamExtra* extra) noexcept {
    if (!in_data || !params || !extra) return PF_Err_BAD_CALLBACK_PARAM;
    try {
        if (extra->param_index == kCaptureControlsId) return capture_controls(in_data, out_data, params, extra);
        if (!is_bound_control(extra->param_index)) return PF_Err_NONE;
        return sync_graph_from_controls(in_data, out_data, params);
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
