#include "Parameters.hpp"

#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "GraphCarrier.hpp"
#include "NativeNodeGraph.hpp"
#include "Param_Utils.h"
#include "WorldBridge.hpp"

#include "starfield/core/Geometry.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <utility>

#define STARFIELD_ADD_BOOTSTRAP_SLOT(NAME, ID) \
    do { \
        AEFX_CLR_STRUCT(def); \
        def.flags = PF_ParamFlag_CANNOT_TIME_VARY; \
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE; \
        PF_ADD_FLOAT_SLIDER(NAME, 0.0f, 1.0f, 0.0f, 1.0f, 0, 0.0f, PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE, 0, ID); \
    } while (0)

#define STARFIELD_ADD_HIDDEN_FLOAT(NAME, VALID_MIN, VALID_MAX, SLIDER_MIN, SLIDER_MAX, DFLT, PREC, DISP, FLAGS, ID) \
    do { \
        AEFX_CLR_STRUCT(def); \
        def.flags = (FLAGS); \
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE; \
        PF_ADD_FLOAT_SLIDER(NAME, VALID_MIN, VALID_MAX, SLIDER_MIN, SLIDER_MAX, 0, DFLT, PREC, DISP, 0, ID); \
    } while (0)

namespace starfield::adapter {
namespace {

namespace core = starfield::core;

// Parameter IDs come from schema/parameters.json and are project-file
// compatibility keys. Disk IDs only have to be unique inside this effect.
constexpr A_long kMaxParticlesDiskId = 'pcnt';
constexpr A_long kParticlesPerSecondDiskId = 'brth';
constexpr A_long kSeedDiskId = 'seed';
constexpr A_long kLifetimeDiskId = 'life';
constexpr A_long kEmitterTypeDiskId = 'esha';
constexpr A_long kOriginDiskId = 'epos';
constexpr A_long kSpeedXDiskId = 'velx';
constexpr A_long kSpeedYDiskId = 'vely';
constexpr A_long kSpeedZDiskId = 'velz';
constexpr A_long kSizeDiskId = 'size';
constexpr A_long kOpacityDiskId = 'opac';
constexpr A_long kEmitterSizeDiskId = 'esiz';
constexpr A_long kSpeedRandomDiskId = 'vspd';
constexpr A_long kGravityXDiskId = 'grvx';
constexpr A_long kGravityYDiskId = 'grvy';
constexpr A_long kGravityZDiskId = 'grvz';
constexpr A_long kLinearDragDiskId = 'drag';
constexpr A_long kColorStartDiskId = 'clrs';
constexpr A_long kColorEndDiskId = 'clre';
constexpr A_long kParticleSizeEndDiskId = 'szen';
constexpr A_long kOpacityEndDiskId = 'open';
// Topic markers are parameters too; their ids only have to be unique.
constexpr A_long kEmitterTopicDiskId = 'topE';
constexpr A_long kParticleTopicDiskId = 'topP';
constexpr A_long kPhysicsTopicDiskId = 'topH';
constexpr A_long kLayoutEmitterXDiskId = 'lEx0';
constexpr A_long kLayoutEmitterYDiskId = 'lEy0';
constexpr A_long kLayoutForceXDiskId = 'lFx0';
constexpr A_long kLayoutForceYDiskId = 'lFy0';
constexpr A_long kLayoutParticleXDiskId = 'lAx0';
constexpr A_long kLayoutParticleYDiskId = 'lAy0';
constexpr A_long kLayoutOutputXDiskId = 'lOx0';
constexpr A_long kLayoutOutputYDiskId = 'lOy0';
constexpr A_long kSizeCurveCountDiskId = 'szct';
constexpr A_long kOpacityCurveCountDiskId = 'opct';
constexpr A_long kCurveEditCommitDiskId = 'cvcm';
constexpr A_long kEmitterSizeTopicDiskId = 'topX';
constexpr A_long kEmitterSizeXDiskId = 'eszx';
constexpr A_long kEmitterSizeYDiskId = 'eszy';
constexpr A_long kEmitterSizeZDiskId = 'eszz';
constexpr A_long kParticleVariationTopicDiskId = 'topV';
constexpr A_long kParticleSizeRandomDiskId = 'szrd';
constexpr A_long kOpacityRandomDiskId = 'oprd';

constexpr A_long curve_point_disk_id(bool opacity, std::size_t point, bool value) noexcept {
    const auto a = static_cast<std::uint32_t>(opacity ? 'o' : 's');
    const auto b = static_cast<std::uint32_t>('0' + point);
    const auto c = static_cast<std::uint32_t>(value ? 'v' : 'a');
    const auto d = static_cast<std::uint32_t>(value ? 'l' : 'g');
    return static_cast<A_long>((a << 24u) | (b << 16u) | (c << 8u) | d);
}

// Parameter types and host indices in binding order: slots 0..20 map to render
// controls and slots 21..54 map to the hidden curve banks, 55..57 to emitter
// dimensions, and 58..59 to Particle random variation. Types are stamped into the checked-out PF_ParamDef before
// PF_CHECKOUT_PARAM so the host cannot be confused about how to fill the value
// union. Hosts that fill by index are unaffected.
constexpr A_long kParameterIndices[kEffectParameterCount] = {
    kMaxParticlesId,        // Max Particles
    kParticlesPerSecondId,  // Particles Per Second
    kSeedId,                // Random Seed
    kLifetimeId,            // Lifetime
    kTypeId,                // Type (popup)
    kOriginId,              // Origin (3D point)
    kSpeedXId,              // Speed X
    kSpeedYId,              // Speed Y
    kSpeedZId,              // Speed Z
    kSizeId,                // Size
    kOpacityId,             // Opacity
    kEmitterSizeId,         // Emitter Size
    kSpeedRandomId,         // Speed Random
    kGravityXId,            // Gravity X
    kGravityYId,            // Gravity Y
    kGravityZId,            // Gravity Z
    kLinearDragId,          // Linear Drag
    kColorStartId,          // Color Start
    kColorEndId,            // Color End
    kParticleSizeEndId,     // Size Over Life
    kOpacityEndId,          // Opacity Over Life
    kSizeCurveCountId,
    kSizeCurveFirstPointId + 0, kSizeCurveFirstPointId + 1,
    kSizeCurveFirstPointId + 2, kSizeCurveFirstPointId + 3,
    kSizeCurveFirstPointId + 4, kSizeCurveFirstPointId + 5,
    kSizeCurveFirstPointId + 6, kSizeCurveFirstPointId + 7,
    kSizeCurveFirstPointId + 8, kSizeCurveFirstPointId + 9,
    kSizeCurveFirstPointId + 10, kSizeCurveFirstPointId + 11,
    kSizeCurveFirstPointId + 12, kSizeCurveFirstPointId + 13,
    kSizeCurveFirstPointId + 14, kSizeCurveFirstPointId + 15,
    kOpacityCurveCountId,
    kOpacityCurveFirstPointId + 0, kOpacityCurveFirstPointId + 1,
    kOpacityCurveFirstPointId + 2, kOpacityCurveFirstPointId + 3,
    kOpacityCurveFirstPointId + 4, kOpacityCurveFirstPointId + 5,
    kOpacityCurveFirstPointId + 6, kOpacityCurveFirstPointId + 7,
    kOpacityCurveFirstPointId + 8, kOpacityCurveFirstPointId + 9,
    kOpacityCurveFirstPointId + 10, kOpacityCurveFirstPointId + 11,
    kOpacityCurveFirstPointId + 12, kOpacityCurveFirstPointId + 13,
    kOpacityCurveFirstPointId + 14, kOpacityCurveFirstPointId + 15,
    kEmitterSizeXId, kEmitterSizeYId, kEmitterSizeZId,
    kParticleSizeRandomId, kOpacityRandomId,
};
constexpr A_long kParameterTypes[kEffectParameterCount] = {
    PF_Param_FLOAT_SLIDER, /* Max Particles       */
    PF_Param_FLOAT_SLIDER, /* Particles Per Second*/
    PF_Param_FLOAT_SLIDER, /* Random Seed         */
    PF_Param_FLOAT_SLIDER, /* Lifetime            */
    PF_Param_POPUP,        /* Type                */
    PF_Param_POINT_3D,     /* Origin              */
    PF_Param_FLOAT_SLIDER, /* Speed X             */
    PF_Param_FLOAT_SLIDER, /* Speed Y             */
    PF_Param_FLOAT_SLIDER, /* Speed Z             */
    PF_Param_FLOAT_SLIDER, /* Size                */
    PF_Param_FLOAT_SLIDER, /* Opacity             */
    PF_Param_FLOAT_SLIDER, /* Emitter Size        */
    PF_Param_FLOAT_SLIDER, /* Speed Random        */
    PF_Param_FLOAT_SLIDER, /* Gravity X           */
    PF_Param_FLOAT_SLIDER, /* Gravity Y           */
    PF_Param_FLOAT_SLIDER, /* Gravity Z           */
    PF_Param_FLOAT_SLIDER, /* Linear Drag         */
    PF_Param_COLOR,        /* Color Start         */
    PF_Param_COLOR,        /* Color End           */
    PF_Param_FLOAT_SLIDER, /* Size Over Life      */
    PF_Param_FLOAT_SLIDER, /* Opacity Over Life   */
    PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
    PF_Param_FLOAT_SLIDER, PF_Param_FLOAT_SLIDER,
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

std::uint32_t to_curve_point_count(const PF_ParamDef& def) noexcept {
    const double value = static_cast<double>(def.u.fs_d.value);
    if (!std::isfinite(value) || value <= 0.0) return 0;
    if (value >= static_cast<double>(core::kMaxAgeCurvePoints)) {
        return static_cast<std::uint32_t>(core::kMaxAgeCurvePoints);
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
// control feeds the canonical graph in Node Graph mode. Looked up in the binding
// table so regrouping the ECW cannot silently invalidate the check.
bool is_bound_control(A_long index) noexcept {
    for (const A_long bound : kParameterIndices) {
        if (bound == index) return true;
    }
    return false;
}

// Maps one delivered value per manifest control (binding order, see
// kParameterIndices) into core settings. The render checkout and the supervised
// panel edit share this path so both see identical conversions. Values are not
// clamped here: validate_settings owns every bound.
core::Settings settings_from_controls(const PF_ParamDef* const* defs, PF_InData& in_data,
                                      core::Vec3* raw_origin, A_long reference_width = 0,
                                      A_long reference_height = 0) noexcept {
    core::Settings settings;

    settings.particle_count = to_particle_count(*defs[0]);
    settings.birth_rate = to_double(*defs[1]);
    settings.seed = to_seed(*defs[2]);
    settings.particle_lifetime_seconds = to_double(*defs[3]);
    settings.emitter_shape = core::emitter_shape_from_index(popup_index(*defs[4]));

    // Emitter Origin is an AE point control. AE delivers absolute layer-pixel positions
    // with the origin at the layer's top-left, x growing right and y growing down. In AE
    // 2023.5 Build 52 the delivered values also follow the preview downsample (observed
    // at Full and Quarter), so first restore full-resolution pixels, then convert into
    // the core's centered world space (ADR 0003, docs/parameter-mapping.md).
    const PF_Point3DDef& origin = defs[5]->u.point3d_d;
    const core::Vec3 raw{static_cast<double>(origin.x_value), static_cast<double>(origin.y_value),
                         static_cast<double>(origin.z_value)};
    if (raw_origin) *raw_origin = raw;

    const A_long effective_width = reference_width > 0 ? reference_width : in_data.width;
    const A_long effective_height = reference_height > 0 ? reference_height : in_data.height;
    const double layer_width = static_cast<double>(effective_width > 0 ? effective_width : 1);
    const double layer_height = static_cast<double>(effective_height > 0 ? effective_height : 1);
    core::LayerUnits units;
    units.layer_width = layer_width;
    units.layer_height = layer_height;
    units.pixel_aspect_ratio = host_pixel_aspect_ratio(in_data);
    const core::Vec3 pixels = point_control_to_full_resolution_pixels(raw, in_data);
    settings.emitter_origin = core::layer_point_to_world(pixels.x, pixels.y, pixels.z, units);

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
    // AE Controls always expose the full Particle appearance controls. The
    // compatibility constructor defaults appearance off for old core callers,
    // so mark the adapter snapshot explicitly before projecting it into a graph.
    settings.appearance_enabled = true;
    const std::uint32_t size_count = to_curve_point_count(*defs[21]);
    settings.size_over_life.count = static_cast<std::uint8_t>(size_count);
    for (std::size_t point = 0; point < core::kMaxAgeCurvePoints; ++point) {
        settings.size_over_life.points[point].age = to_double(*defs[22 + point * 2]);
        settings.size_over_life.points[point].value = to_double(*defs[23 + point * 2]);
    }
    const std::size_t opacity_count_slot = 38;
    const std::uint32_t opacity_count = to_curve_point_count(*defs[opacity_count_slot]);
    settings.opacity_over_life.count = static_cast<std::uint8_t>(opacity_count);
    for (std::size_t point = 0; point < core::kMaxAgeCurvePoints; ++point) {
        settings.opacity_over_life.points[point].age = to_double(*defs[opacity_count_slot + 1 + point * 2]);
        settings.opacity_over_life.points[point].value = to_double(*defs[opacity_count_slot + 2 + point * 2]);
    }
    // Base Size/Opacity are independent from the normalized curve ordinates.
    // The visible endpoint controls mirror only the curve's final percentage;
    // point zero and all interior knots remain project curve-bank values.
    if (size_count >= 2) {
        settings.size_over_life.points[size_count - 1].value = settings.particle_size_end;
    }
    if (opacity_count >= 2) {
        settings.opacity_over_life.points[opacity_count - 1].value = settings.opacity_end;
    }
    settings.emitter_size_pixels = core::Vec3{
        to_double(*defs[55]), to_double(*defs[56]), to_double(*defs[57])};
    settings.particle_size_random_percent = to_double(*defs[58]);
    settings.opacity_random_percent = to_double(*defs[59]);
    return settings;
}

PF_Err add_hidden_layout_coordinate(PF_InData* in_data, PF_ParamDef& def, const char* name,
                                    A_long disk_id, PF_FpLong default_value) noexcept {
    constexpr PF_FpLong kCoordinateLimit = 1000000000.0;
    if (in_data == nullptr || name == nullptr) return PF_Err_BAD_CALLBACK_PARAM;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = disk_id;
    def.u.fs_d.value = def.u.fs_d.dephault = static_cast<PF_FpShort>(default_value);
    def.u.fs_d.valid_min = def.u.fs_d.slider_min = static_cast<PF_FpShort>(-kCoordinateLimit);
    def.u.fs_d.valid_max = def.u.fs_d.slider_max = static_cast<PF_FpShort>(kCoordinateLimit);
    def.u.fs_d.precision = static_cast<A_short>(PF_Precision_THOUSANDTHS);
    def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err add_hidden_curve_slider(PF_InData* in_data, PF_ParamDef& def, const char* name,
                               A_long disk_id, PF_FpLong minimum, PF_FpLong maximum,
                               PF_FpLong default_value, A_long precision,
                               bool supervise = false) noexcept {
    if (!in_data || !name) return PF_Err_BAD_CALLBACK_PARAM;
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY |
        (supervise ? PF_ParamFlag_SUPERVISE : PF_ParamFlag_NONE);
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = disk_id;
    def.u.fs_d.value = def.u.fs_d.dephault = static_cast<PF_FpShort>(default_value);
    def.u.fs_d.valid_min = def.u.fs_d.slider_min = static_cast<PF_FpShort>(minimum);
    def.u.fs_d.valid_max = def.u.fs_d.slider_max = static_cast<PF_FpShort>(maximum);
    def.u.fs_d.precision = static_cast<A_short>(precision);
    def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
    return PF_ADD_PARAM(in_data, -1, &def);
}

} // namespace

core::Vec3 point_control_to_full_resolution_pixels(const core::Vec3& raw,
                                                   const PF_InData& in_data) noexcept {
    const auto full_resolution_factor = [](const PF_RationalScale& downsample) noexcept {
        if (downsample.num <= 0 || downsample.den == 0) return 1.0;
        // AE's observed 1/4 preview delivers one-quarter-sized point coordinates. The
        // reciprocal restores those coordinates to full-resolution layer pixels.
        return static_cast<double>(downsample.den) / static_cast<double>(downsample.num);
    };
    const double horizontal_factor = full_resolution_factor(in_data.downsample_x);
    const double vertical_factor = full_resolution_factor(in_data.downsample_y);
    return core::Vec3{
        core::host_point_component_to_layer_pixels(raw.x * horizontal_factor),
        core::host_point_component_to_layer_pixels(raw.y * vertical_factor),
        core::host_point_component_to_layer_pixels(raw.z * vertical_factor),
    };
}

PF_Err setup_parameters(PF_InData* in_data, PF_OutData* out_data) noexcept {
    if (in_data == nullptr || out_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_ParamDef def;
    PF_Err err = PF_Err_NONE;

    // Labels, ranges, precision, and defaults mirror schema/parameters.json exactly.
    // Float literals match PF_FpShort so no narrowing warning is emitted at /W4.
    // These original flat controls remain registered for the host-independent
    // bootstrap and current development schema, but node values are owned by the
    // separate hidden node effects. Keep the old controls out of Effect Controls.
    // Hidden group starts with visible group ends leave a malformed ECW tree.
    // Bootstrap data needs no hierarchy: use ordinary hidden scalar slots here.
    AEFX_CLR_STRUCT(def);
    // A picture-only control has no scalar value or collapsible Presets title.
    def.param_type=PF_Param_NO_DATA;def.flags=PF_ParamFlag_CANNOT_TIME_VARY;
    def.ui_flags=PF_PUI_CONTROL;def.ui_width=304;def.ui_height=104;def.uu.id=1631;
    err=PF_ADD_PARAM(in_data,-1,&def);if(err)return err;
    // Type and Origin are registered by hand instead of through PF_ADD_POPUP or
    // PF_ADD_POINT_3D: those macros call PF_ADD_PARAM themselves and never set
    // def.flags, and both controls must be supervised for the panel path.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = PF_ParamFlag_SUPERVISE;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "Type");
    def.uu.id = kEmitterTypeDiskId;
    def.u.pd.num_choices = 4;
    def.u.pd.dephault = 1; // AE popup values are one-based: 1 is Point
    def.u.pd.value = def.u.pd.dephault;
    def.u.pd.u.namesptr = "Point|Box|Sphere|Disc";
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Particles Per Second", 0.0f, 1000000.0f, 0.0f, 1000000.0f, 100.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kParticlesPerSecondDiskId);

    // Position control: AE owns the on-screen picking behavior for point params.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POINT_3D;
    def.flags = PF_ParamFlag_SUPERVISE;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "Origin");
    def.uu.id = kOriginDiskId;
    def.u.point3d_d.x_value = def.u.point3d_d.x_dephault = 50.0;
    def.u.point3d_d.y_value = def.u.point3d_d.y_dephault = 50.0;
    def.u.point3d_d.z_value = def.u.point3d_d.z_dephault = 50.0;
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Disc Size", 0.0f, 10.0f, 0.0f, 1.0f, 0.05f, PF_Precision_THOUSANDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kEmitterSizeDiskId);

    // Velocity, not Speed: the reference emitter has a single scalar Speed plus a direction
    // model (Direction/Angle/Direction Span), and its "Speed X/Y/Z" are per-particle rotation
    // speeds in the Particle module. Naming our axes Speed would claim a meaning they do not
    // have. docs/reference-parameter-map.md records the model difference and the planned fix.
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Velocity X", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSpeedXDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Velocity Y", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.3f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSpeedYDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Velocity Z", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSpeedZDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Speed Random", 0.0f, 100.0f, 0.0f, 1.0f, 0.15f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSpeedRandomDiskId);
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 10", 'endE');

    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 11", kParticleTopicDiskId);
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Life (Seconds)", 0.0f, 10000.0f, 0.0f, 10.0f, 2.0f, PF_Precision_TENTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kLifetimeDiskId);

    // Default 10 px matches the reference's observed "Size (Pixels): 10".
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Size", 0.0f, 100000.0f, 0.0f, 100000.0f, 10.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSizeDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Size Over Life", 0.0f, 100.0f, 0.0f, 100.0f, 100.0f, PF_Precision_TENTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kParticleSizeEndDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Opacity", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, PF_Precision_THOUSANDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kOpacityDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Opacity Over Life", 0.0f, 100.0f, 0.0f, 100.0f, 100.0f, PF_Precision_TENTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kOpacityEndDiskId);

    // PF_ADD_COLOR does not clear the struct or touch flags; set them explicitly.
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    PF_ADD_COLOR("Color Start", 255, 255, 255, kColorStartDiskId);
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_SUPERVISE;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    PF_ADD_COLOR("Color End", 255, 255, 255, kColorEndDiskId);
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 19", 'endP');

    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 20", kPhysicsTopicDiskId);
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Gravity X", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityXDiskId);
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Gravity Y", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityYDiskId);
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Gravity Z", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kGravityZDiskId);
    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Linear Drag", 0.0f, 100.0f, 0.0f, 10.0f, 0.0f, PF_Precision_THOUSANDTHS,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kLinearDragDiskId);
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 25", 'endH');

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Max Particles", 0.0f, 2000000.0f, 0.0f, 100.0f, 1000000.0f, PF_Precision_INTEGER,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kMaxParticlesDiskId);

    AEFX_CLR_STRUCT(def);
    STARFIELD_ADD_HIDDEN_FLOAT("Random Seed", 0.0f, 2147483647.0f, 0.0f, 2147483647.0f, 1.0f, PF_Precision_INTEGER,
                              PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, kSeedDiskId);

    // Control Source defaults to AE Controls (manifest revision 6): a freshly applied
    // effect must drive the visible controls, not sit in Node Graph mode where the
    // controls look inert. Node Graph is opt-in through capture or the panel.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "Control Source");
    def.uu.id = kControlSourceDiskId;
    def.u.pd.num_choices = 2;
    def.u.pd.dephault = kLegacyControlSource;
    def.u.pd.value = kLegacyControlSource;
    def.u.pd.u.namesptr = "AE Controls|Node Graph";
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    PF_ADD_BUTTON("Capture Current Controls", "Capture at Current Time", PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE,
                  PF_ParamFlag_SUPERVISE, kCaptureControlsDiskId);

    // The graph's default handle becomes host-owned only after successful ADD_PARAM.
    if (graph_parameter_disabled()) {
        // Diagnostic build probe: same index, same count, no arbitrary data at all.
        AEFX_CLR_STRUCT(def);
        def.param_type = PF_Param_FLOAT_SLIDER;
        def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
        std::snprintf(def.name, sizeof(def.name), "Node Graph Data");
        def.uu.id = kGraphParameterDiskId; // same project identity, different parameter kind
        def.u.fs_d.value = def.u.fs_d.dephault = 0.0;
        err = PF_ADD_PARAM(in_data, -1, &def);
        if (err != PF_Err_NONE) return err;
    } else {
        PF_ArbitraryH default_graph = nullptr;
        PF_ArbParamsExtra create{};
        create.id = kGraphParameterDiskId;
        create.which_function = PF_Arbitrary_NEW_FUNC;
        create.u.new_func_params.arbPH = &default_graph;
        err = graph_arbitrary_callback(in_data, &create);
        if (err != PF_Err_NONE) return err;
        AEFX_CLR_STRUCT(def);
        def.param_type = PF_Param_ARBITRARY_DATA;
        // Arbitrary data cannot be animated, and the SDK's own PF_ADD_ARBITRARY2 passes
        // no PF_ParamFlags at all; the two flags this used to carry were never part of
        // the documented arbitrary-data contract.
        def.flags = PF_ParamFlag_NONE;
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
        std::snprintf(def.name, sizeof(def.name), "Node Graph Data");
        def.uu.id = def.u.arb_d.id = kGraphParameterDiskId;
        def.u.arb_d.dephault = default_graph;
        err = PF_ADD_PARAM(in_data, -1, &def);
        if (err != PF_Err_NONE) { in_data->utils->host_dispose_handle(default_graph); return err; }
    }

    // Node positions are non-rendering UI state, but they belong to this effect
    // instance so AE can save, duplicate, and undo the layout with the project.
    // Standard hidden sliders remain script-readable/writable, unlike graph_data's
    // CUSTOM_VALUE stream. They follow the compiled graph in the flat layout.
    const struct LayoutCoordinate {
        const char* name;
        A_long disk_id;
        PF_FpLong default_value;
    } layout_coordinates[] = {
        {"Layout Emitter X", kLayoutEmitterXDiskId, 180.0},
        {"Layout Emitter Y", kLayoutEmitterYDiskId, 22.0},
        {"Layout Force X", kLayoutForceXDiskId, 180.0},
        {"Layout Force Y", kLayoutForceYDiskId, 190.0},
        {"Layout Particle X", kLayoutParticleXDiskId, 180.0},
        {"Layout Particle Y", kLayoutParticleYDiskId, 358.0},
        {"Layout Output X", kLayoutOutputXDiskId, 180.0},
        {"Layout Output Y", kLayoutOutputYDiskId, 526.0},
    };
    for (const auto& coordinate : layout_coordinates) {
        err = add_hidden_layout_coordinate(in_data, def, coordinate.name, coordinate.disk_id,
                                           coordinate.default_value);
        if (err != PF_Err_NONE) return err;
    }

    // Revision and checksum receipts are ordinary numeric streams. The panel edits separate node
    // effects and changes the numeric commit stream to compile those records into
    // the renderer's arbitrary-data graph. Index 40 guards batched Output writes.
    const struct GraphCarrierParameter {
        const char* name;
        A_long disk_id;
        PF_FpLong default_value;
        bool supervised;
    } graph_carrier_parameters[] = {
        {"Graph Revision", kGraphRevisionDiskId, 0.0, false},
        {"Panel Graph Sync Guard", kGraphSyncGuardDiskId, 0.0, false},
        {"Commit Graph Edit", kGraphEditCommitDiskId, 0.0, true},
        {"Graph Edit Receipt", kGraphEditReceiptDiskId, 0.0, false},
    };
    constexpr PF_FpLong kCarrierControlLimit = 1000000.0;
    for (const auto& carrier : graph_carrier_parameters) {
        AEFX_CLR_STRUCT(def);
        def.param_type = PF_Param_FLOAT_SLIDER;
        def.flags = PF_ParamFlag_CANNOT_TIME_VARY |
                    (carrier.supervised ? PF_ParamFlag_SUPERVISE : PF_ParamFlag_NONE);
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
        std::snprintf(def.name, sizeof(def.name), "%s", carrier.name);
        def.uu.id = carrier.disk_id;
        def.u.fs_d.value = def.u.fs_d.dephault = static_cast<PF_FpShort>(carrier.default_value);
        def.u.fs_d.valid_min = def.u.fs_d.slider_min = -kCarrierControlLimit;
        def.u.fs_d.valid_max = def.u.fs_d.slider_max = kCarrierControlLimit;
        if (carrier.disk_id == kGraphRevisionDiskId) {
            def.u.fs_d.valid_min = def.u.fs_d.slider_min = 0.0;
            def.u.fs_d.valid_max = def.u.fs_d.slider_max = 16777215.0;
        }
        def.u.fs_d.precision = PF_Precision_INTEGER;
        def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
        err = PF_ADD_PARAM(in_data, -1, &def);
        if (err != PF_Err_NONE) return err;
    }

    // Project-owned normalized-age curve banks. The CEP panel edits these
    // script-visible scalar streams as one undo group, then changes the commit
    // nonce so Node Graph mode rebuilds the canonical arbitrary-data graph once.
    const struct CurveBank {
        const char* label;
        const char* short_name;
        bool opacity;
        A_long count_id;
        A_long first_point_id;
        A_long count_disk_id;
        PF_FpLong value_max;
        A_long value_precision;
    } curve_banks[] = {
        {"Size", "Size", false, kSizeCurveCountId, kSizeCurveFirstPointId,
         kSizeCurveCountDiskId, 100.0, PF_Precision_TENTHS},
        {"Opacity", "Opacity", true, kOpacityCurveCountId, kOpacityCurveFirstPointId,
         kOpacityCurveCountDiskId, 100.0, PF_Precision_TENTHS},
    };
    for (const auto& bank : curve_banks) {
        char name[sizeof(def.name)]{};
        std::snprintf(name, sizeof(name), "%s Curve Count", bank.label);
        err = add_hidden_curve_slider(in_data, def, name, bank.count_disk_id, 0.0,
                                      static_cast<PF_FpLong>(core::kMaxAgeCurvePoints), 0.0,
                                      PF_Precision_INTEGER);
        if (err != PF_Err_NONE) return err;
        for (std::size_t point = 0; point < core::kMaxAgeCurvePoints; ++point) {
            std::snprintf(name, sizeof(name), "%s Curve Point %u Age", bank.short_name,
                          static_cast<unsigned int>(point));
            err = add_hidden_curve_slider(in_data, def, name,
                curve_point_disk_id(bank.opacity, point, false), 0.0, 1.0, 0.0,
                PF_Precision_THOUSANDTHS);
            if (err != PF_Err_NONE) return err;
            std::snprintf(name, sizeof(name), "%s Curve Point %u Value", bank.short_name,
                          static_cast<unsigned int>(point));
            err = add_hidden_curve_slider(in_data, def, name,
                curve_point_disk_id(bank.opacity, point, true), 0.0, bank.value_max,
                0.0, bank.value_precision);
            if (err != PF_Err_NONE) return err;
        }
    }
    err = add_hidden_curve_slider(in_data, def, "Curve Edit Commit", kCurveEditCommitDiskId,
                                  -1000000.0, 1000000.0, 0.0,
                                  PF_Precision_INTEGER, true);
    if (err != PF_Err_NONE) return err;

    // Direct full-resolution pixel dimensions are appended as hidden bootstrap controls
    // so all earlier AE indices remain stable. Box/Sphere read their X/Y/Z values.
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 80", kEmitterSizeTopicDiskId);
    STARFIELD_ADD_HIDDEN_FLOAT("Size X", 0.0f, 100000.0f, 0.0f, 100000.0f, 100.0f,
                              PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE,
                              PF_ParamFlag_SUPERVISE, kEmitterSizeXDiskId);
    STARFIELD_ADD_HIDDEN_FLOAT("Size Y", 0.0f, 100000.0f, 0.0f, 100000.0f, 100.0f,
                              PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE,
                              PF_ParamFlag_SUPERVISE, kEmitterSizeYDiskId);
    STARFIELD_ADD_HIDDEN_FLOAT("Size Z", 0.0f, 100000.0f, 0.0f, 100000.0f, 100.0f,
                              PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE,
                              PF_ParamFlag_SUPERVISE, kEmitterSizeZDiskId);
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 84", 'endX');

    // Revision 11 appends variation controls without shifting any released index.
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 85", kParticleVariationTopicDiskId);
    STARFIELD_ADD_HIDDEN_FLOAT("Size Random", 0.0f, 100.0f, 0.0f, 100.0f, 0.0f,
                              PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE,
                              PF_ParamFlag_SUPERVISE, kParticleSizeRandomDiskId);
    STARFIELD_ADD_HIDDEN_FLOAT("Opacity Random", 0.0f, 100.0f, 0.0f, 100.0f, 0.0f,
                              PF_Precision_INTEGER, PF_ValueDisplayFlag_NONE,
                              PF_ParamFlag_SUPERVISE, kOpacityRandomDiskId);
    STARFIELD_ADD_BOOTSTRAP_SLOT("Bootstrap Slot 88", 'endV');

    // This project-owned marker distinguishes first-time node materialization
    // from a node effect removed later in AE's Effect Parade. It is deliberately
    // unsupervised: the CEP updates it inside the same undo group as node creation.
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
    def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
    std::snprintf(def.name, sizeof(def.name), "Node Effects Ready");
    def.uu.id = kNodeEffectsReadyDiskId;
    def.u.fs_d.value = def.u.fs_d.dephault = 0.0;
    def.u.fs_d.valid_min = def.u.fs_d.slider_min = 0.0;
    def.u.fs_d.valid_max = def.u.fs_d.slider_max = 1.0;
    def.u.fs_d.precision = PF_Precision_INTEGER;
    def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
    err = PF_ADD_PARAM(in_data, -1, &def);
    if (err != PF_Err_NONE) return err;

    for (const auto& checksum : {std::pair{"Graph Checksum High", kGraphChecksumHighDiskId},
                                 std::pair{"Graph Checksum Low", kGraphChecksumLowDiskId}}) {
        AEFX_CLR_STRUCT(def);
        def.param_type = PF_Param_FLOAT_SLIDER;
        def.flags = PF_ParamFlag_CANNOT_TIME_VARY;
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
        std::snprintf(def.name, sizeof(def.name), "%s", checksum.first);
        def.uu.id = checksum.second;
        def.u.fs_d.valid_max = def.u.fs_d.slider_max = 65535.0;
        def.u.fs_d.precision = PF_Precision_INTEGER;
        err = PF_ADD_PARAM(in_data, -1, &def);
        if (err != PF_Err_NONE) return err;
    }
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Time Remapping", 920);
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("Time Remapping On / Off", "", FALSE, PF_ParamFlag_SUPERVISE, 921);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Time (Seconds)", -1000000, 1000000, 0, 10, 0, PF_Precision_HUNDREDTHS,
                        PF_ValueDisplayFlag_NONE, PF_ParamFlag_SUPERVISE, 922);
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(923);
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Render Settings", 924);
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("Preview", "", FALSE, PF_ParamFlag_SUPERVISE, 925);
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle chance", 0, 100, 0, 100, 100, PF_Precision_TENTHS,
                        PF_ValueDisplayFlag_PERCENT, PF_ParamFlag_SUPERVISE, 926);
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(927);
    for (A_long slot = 0; slot < kNativeBindingCapacity; ++slot) {
        AEFX_CLR_STRUCT(def);
        def.param_type = PF_Param_FLOAT_SLIDER;
        def.ui_flags = PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE;
        def.uu.id = kNativeBindingFirstDiskId + slot;
        std::snprintf(def.name, sizeof(def.name), "Native Render Input %ld", static_cast<long>(slot + 1));
        def.u.fs_d.valid_min = def.u.fs_d.slider_min = -1.0e15f;
        def.u.fs_d.valid_max = def.u.fs_d.slider_max = 1.0e15f;
        def.u.fs_d.precision = PF_Precision_THOUSANDTHS;
        const auto binding_error = PF_ADD_PARAM(in_data, -1, &def);
        if (binding_error) return binding_error;
    }
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Simulation Settings", 1600);
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP("Time Sampling", 3, 1, "30 Hz|60 Hz|120 Hz", 1601);
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(1602);
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("GPU Rendering", 1610);
    AEFX_CLR_STRUCT(def);
    def.flags = PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_SUPERVISE;
    PF_ADD_POPUP("Acceleration", 2, 1, "GPU|CPU", 1611);
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(1612);
    out_data->num_params = static_cast<A_long>(kTotalEffectParameterCount) + 1;
    return PF_Err_NONE;
}

PF_Err ParameterSnapshot::checkout(PF_InData* in_data, A_long reference_width,
                                   A_long reference_height) noexcept {
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

    // One conversion path for rendering and supervised panel edits; preview-scaled
    // point values are normalized in the shared helper.
    const PF_ParamDef* controls[kEffectParameterCount]{};
    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        controls[i] = &defs_[i];
    }
    settings_ = settings_from_controls(controls, *in_data, &raw_origin_, reference_width, reference_height);

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

bool graph_parameter_disabled() noexcept {
    // Diagnostic escape hatch for host triage. With STARFIELD_NO_GRAPH_PARAM=1 the
    // arbitrary-data parameter is replaced at registration time by a hidden float slider
    // that keeps the same index and count, and rendering always samples the flat
    // controls. It answers exactly one question: does the arbitrary-data parameter
    // itself break the host? Normal runs are unaffected and the value is never persisted.
    const char* value = std::getenv("STARFIELD_NO_GRAPH_PARAM");
    return value != nullptr && value[0] != '\0' && value[0] != '0';
}

bool flat_render_override_active() noexcept {
    // Diagnostic escape hatch for host triage. With STARFIELD_FLAT_RENDER=1 the render
    // samples the flat AE controls instead of the stored graph, so a host-side surprise
    // can be bisected between the graph path and the flat path without a rebuild.
    // Normal runs are unaffected, the value is never persisted, and the Options readout
    // prints whether the override is active.
    const char* value = std::getenv("STARFIELD_FLAT_RENDER");
    if (value != nullptr && value[0] != '\0' && value[0] != '0') return true;
    return graph_parameter_disabled();
}

PF_Err checkout_render_graph(PF_InData* in_data, PF_OutData* out_data,
                             std::shared_ptr<const core::Graph>& graph,
                             A_long* control_source, A_long reference_width, A_long reference_height) noexcept {
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
            err = legacy.snapshot().checkout(in_data, reference_width, reference_height);
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
            core::Graph sampled = decoded.take_value();
            A_long failed_binding = -1;
            const char* failed_stage = "unknown";
            err = sample_native_node_animation(in_data, sampled, reference_width, reference_height,
                                               &failed_binding, &failed_stage);
            if (err) {
                if (out_data) std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                    "Starfield animation: %s failed (stream %ld, error %ld, delivered params %ld).",
                    failed_stage, static_cast<long>(failed_binding), static_cast<long>(err),
                    static_cast<long>(in_data->num_params));
                return err;
            }
            graph = std::make_shared<const core::Graph>(std::move(sampled));
        } else return PF_Err_BAD_CALLBACK_PARAM;
        {
            // These registered controls are read through callbacks in SmartFX,
            // independently of the count of parameters delivered in params[].
            core::Graph snapshot = *graph;
            constexpr std::array<A_long, 6> indices{kTimeRemapEnabledId, kTimeRemapSecondsId, kPreviewEnabledId, kPreviewChanceId,kTimeSamplingHzId,kAccelerationId};
            constexpr std::array<core::ParameterKey, 6> keys{core::graph_keys::kTimeRemapEnabled, core::graph_keys::kTimeRemapSeconds,
                core::graph_keys::kPreviewEnabled, core::graph_keys::kPreviewChance,core::graph_keys::kTimeSamplingHz,core::graph_keys::kAcceleration};
            for (std::size_t i = 0; i < indices.size(); ++i) {
                CheckedParameter global(in_data);
                err = global.checkout(indices[i]);
                if (err != PF_Err_NONE) { graph.reset(); return err; }
                core::ParameterValue value;
                if (i == 0 || i == 2) {
                    if (global.value.param_type != PF_Param_CHECKBOX) { graph.reset(); return PF_Err_BAD_CALLBACK_PARAM; }
                    value = std::uint32_t(global.value.u.bd.value != 0);
                } else if(i==4) {
                    if(global.value.param_type!=PF_Param_POPUP || global.value.u.pd.value<1 || global.value.u.pd.value>3) {
                        graph.reset();return PF_Err_BAD_CALLBACK_PARAM;
                    }
                    value=std::uint32_t(30u<<(global.value.u.pd.value-1));
                } else if(i==5) {
                    if(global.value.param_type!=PF_Param_POPUP || global.value.u.pd.value<1 || global.value.u.pd.value>2) {
                        graph.reset();return PF_Err_BAD_CALLBACK_PARAM;
                    }
                    value=std::uint32_t(global.value.u.pd.value-1);
                } else {
                    if (global.value.param_type != PF_Param_FLOAT_SLIDER) { graph.reset(); return PF_Err_BAD_CALLBACK_PARAM; }
                    value = double(global.value.u.fs_d.value);
                }
                for (auto& output : snapshot.nodes) if (output.type_key == core::graph_keys::kOutputNode) {
                    output.schema_version = 4;
                    auto found = std::find_if(output.parameters.begin(), output.parameters.end(), [&](const auto& p) {return p.key == keys[i];});
                    if (found == output.parameters.end()) output.parameters.push_back({keys[i], value});
                    else found->value = value;
                }
            }
            graph = std::make_shared<const core::Graph>(std::move(snapshot));
        }
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
        const auto mirrored = write_graph_snapshot(in_data, params, graph.value());
        if (mirrored != PF_Err_NONE) {
            in_data->utils->host_dispose_handle(replacement);
            return mirrored;
        }
        // All fallible work precedes mutation. The editable parameter value is a
        // host-provided copy; its old handle is replaced, never a render checkout.
        auto& target = *params[kGraphParameterId];
        target.u.arb_d.value = replacement;
        target.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[kControlSourceId]->u.pd.value = kNodeControlSource;
        params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        // The replaced handle is NOT disposed here: parameter state belongs to the host,
        // which disposes the value it replaced once the change is committed. Disposing it
        // ourselves frees a host-owned handle, corrupts the handle table and makes AE
        // abort later, on a thread that no longer has any of our frames.
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
//
// Like capture, this path has no input checkout. The shared point conversion uses the
// observed downsample rationals to restore AE's preview-scaled control values before
// baking the world-space origin into the stored graph.
PF_Err sync_graph_from_controls(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]) noexcept {
    if (!params[kGraphParameterId] || !params[kControlSourceId]) return PF_Err_NONE;
    if (params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
        params[kControlSourceId]->param_type != PF_Param_POPUP) return PF_Err_NONE;
    if (params[kControlSourceId]->u.pd.value != kNodeControlSource) return PF_Err_NONE; // AE Controls mode
    if (params[kGraphParameterId]->u.arb_d.value == nullptr) return PF_Err_NONE; // not created yet

    // The delivered array is only trusted as far as the registered parameter count and
    // each entry's declared type agree. A partially built array (apply, undo, panic
    // restore) must be skipped, not dereferenced: reading a stale pointer here is what
    // turns a host-side callback into an access violation inside the plug-in.
    const A_long registered = static_cast<A_long>(kTotalEffectParameterCount) + 1;
    if (in_data->num_params > 0 && in_data->num_params < registered) return PF_Err_NONE;

    const PF_ParamDef* defs[kEffectParameterCount]{};
    for (std::size_t i = 0; i < kEffectParameterCount; ++i) {
        const PF_ParamDef* def = params[kParameterIndices[i]];
        if (!def || def->param_type != kParameterTypes[i]) return PF_Err_NONE;
        defs[i] = def;
    }

    const auto settings = core::validate_settings(settings_from_controls(defs, *in_data, nullptr));
    auto graph = graph_from_controls(settings.value);
    if (!graph.has_value()) return graph_error(out_data, graph.error());
    PF_ArbitraryH replacement = nullptr;
    const auto created = create_graph_parameter(in_data, graph.value(), &replacement);
    if (created != PF_Err_NONE) return created;
    const auto mirrored = write_graph_snapshot(in_data, params, graph.value());
    if (mirrored != PF_Err_NONE) {
        in_data->utils->host_dispose_handle(replacement);
        return mirrored;
    }
    // Same ownership rule as capture_controls: the host disposes the value we replaced.
    params[kGraphParameterId]->u.arb_d.value = replacement;
    params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    return PF_Err_NONE;
}

// Output-wide controls remain on the main effect. Once native node records have
// been materialized, changing Max Particles must recompile those records with the
// new cap; rebuilding from the now-hidden legacy controls would replace the graph.
PF_Err sync_native_graph_with_output_controls(PF_InData* in_data, PF_OutData* out_data,
                                             PF_ParamDef* params[]) noexcept {
    (void)out_data;
    if (!in_data || !params || !params[kGraphParameterId] || !params[kNodeEffectsReadyId] ||
        params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
        !params[kGraphParameterId]->u.arb_d.value ||
        params[kNodeEffectsReadyId]->param_type != PF_Param_FLOAT_SLIDER ||
        params[kNodeEffectsReadyId]->u.fs_d.value < 1.0) return PF_Err_NONE;

    core::Graph graph;
    bool found_node_effects = false;
    const PF_Err compiled = compile_native_node_graph(in_data, params, graph, found_node_effects, graph_carrier_plugin_id());
    if (compiled != PF_Err_NONE) return compiled;

    PF_ArbitraryH replacement = nullptr;
    const PF_Err created = create_graph_parameter(in_data, graph, &replacement);
    if (created != PF_Err_NONE) return created;
    const PF_Err mirrored = write_graph_snapshot(in_data, params, graph);
    if (mirrored != PF_Err_NONE) {
        in_data->utils->host_dispose_handle(replacement);
        return mirrored;
    }
    params[kGraphParameterId]->u.arb_d.value = replacement;
    params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    return PF_Err_NONE;
}

void sync_curve_endpoint(PF_ParamDef* params[], A_long count_id, A_long first_point_id,
                         bool end, PF_FpLong value) noexcept {
    if (!params || !params[count_id] || params[count_id]->param_type != PF_Param_FLOAT_SLIDER) return;
    const std::uint32_t count = to_curve_point_count(*params[count_id]);
    if (count < 2 || count > core::kMaxAgeCurvePoints) return;
    const A_long value_index = first_point_id + static_cast<A_long>((end ? count - 1 : 0) * 2 + 1);
    PF_ParamDef* point_value = params[value_index];
    if (!point_value || point_value->param_type != PF_Param_FLOAT_SLIDER) return;
    point_value->u.fs_d.value = value;
    point_value->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
}

} // namespace

PF_Err user_changed_param(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                          PF_UserChangedParamExtra* extra) noexcept {
    if (!in_data || !params || !extra) return PF_Err_BAD_CALLBACK_PARAM;
    try {
        if (params[kGraphSyncGuardId] && params[kGraphSyncGuardId]->param_type == PF_Param_FLOAT_SLIDER &&
            params[kGraphSyncGuardId]->u.fs_d.value != 0.0) return PF_Err_NONE;
        if (extra->param_index == kCaptureControlsId) return capture_controls(in_data, out_data, params, extra);
        const A_long registered = static_cast<A_long>(kTotalEffectParameterCount) + 1;
        if (in_data->num_params > 0 && in_data->num_params < registered) return PF_Err_NONE;
        if (extra->param_index == kCurveEditCommitId) {
            return sync_graph_from_controls(in_data, out_data, params);
        }
        const bool renderer_global = extra->param_index == kTimeRemapEnabledId || extra->param_index == kTimeRemapSecondsId ||
            extra->param_index == kPreviewEnabledId || extra->param_index == kPreviewChanceId || extra->param_index==kTimeSamplingHzId || extra->param_index==kAccelerationId;
        if (!is_bound_control(extra->param_index) && !renderer_global) return PF_Err_NONE;
        if ((extra->param_index == kMaxParticlesId || renderer_global) && params[kNodeEffectsReadyId] &&
            params[kNodeEffectsReadyId]->param_type == PF_Param_FLOAT_SLIDER &&
            params[kNodeEffectsReadyId]->u.fs_d.value >= 1.0) {
            return sync_native_graph_with_output_controls(in_data, out_data, params);
        }
        if (extra->param_index == kParticleSizeEndId && params[kParticleSizeEndId] &&
                   params[kParticleSizeEndId]->param_type == PF_Param_FLOAT_SLIDER) {
            sync_curve_endpoint(params, kSizeCurveCountId, kSizeCurveFirstPointId, true,
                                params[kParticleSizeEndId]->u.fs_d.value);
        } else if (extra->param_index == kOpacityEndId && params[kOpacityEndId] &&
                   params[kOpacityEndId]->param_type == PF_Param_FLOAT_SLIDER) {
            sync_curve_endpoint(params, kOpacityCurveCountId, kOpacityCurveFirstPointId, true,
                                params[kOpacityEndId]->u.fs_d.value);
        }
        return sync_graph_from_controls(in_data, out_data, params);
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
