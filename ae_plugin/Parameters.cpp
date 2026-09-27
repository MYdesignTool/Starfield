#include "Parameters.hpp"

#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "WorldBridge.hpp"

#include "starfield/core/Geometry.hpp"

#include <cmath>
#include <cstdint>

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

// Parameter types in ID order. They are stamped into the checked-out PF_ParamDef
// before PF_CHECKOUT_PARAM so the host cannot be confused about how to fill the
// value union. Hosts that fill by index are unaffected.
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

} // namespace

PF_Err setup_parameters(PF_InData* in_data, PF_OutData* out_data) noexcept {
    if (in_data == nullptr || out_data == nullptr) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }

    PF_ParamDef def;

    // Labels, ranges, precision, and defaults mirror schema/parameters.json exactly.
    // Float literals match PF_FpShort so no narrowing warning is emitted at /W4.
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Count", 0.0f, 2000000.0f, 0.0f, 2000000.0f, 1000.0f, PF_Precision_INTEGER,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kParticleCountDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Birth Rate", 0.0f, 1000000.0f, 0.0f, 1000000.0f, 30.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kBirthRateDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Random Seed", 0.0f, 2147483647.0f, 0.0f, 2147483647.0f, 1.0f, PF_Precision_INTEGER,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kSeedDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Lifetime", 0.0f, 1000000.0f, 0.0f, 1000000.0f, 2.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kLifetimeDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Emitter Shape", 4, 1, "Point|Box|Sphere|Disc", kEmitterShapeDiskId);

    // Position control: AE owns the on-screen picking behavior for point params.
    AEFX_CLR_STRUCT(def);
    PF_ADD_POINT_3D("Emitter Origin", 50.0, 50.0, 50.0, kEmitterOriginDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity X", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kVelocityXDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Y", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.3f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kVelocityYDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Z", -1000.0f, 1000.0f, -20.0f, 20.0f, 0.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kVelocityZDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Particle Size", 0.0f, 100000.0f, 0.0f, 100000.0f, 8.0f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kSizeDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Opacity", 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kOpacityDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Emitter Size", 0.0f, 10.0f, 0.0f, 1.0f, 0.05f, PF_Precision_THOUSANDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kEmitterSizeDiskId);

    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Velocity Spread", 0.0f, 100.0f, 0.0f, 1.0f, 0.15f, PF_Precision_HUNDREDTHS,
                         PF_ValueDisplayFlag_NONE, PF_ParamFlag_NONE, kVelocitySpreadDiskId);

    out_data->num_params = static_cast<A_long>(kEffectParameterCount) + 1;
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
        const A_long index = static_cast<A_long>(i) + 1;
        const PF_Err err = PF_CHECKOUT_PARAM(in_data, index, in_data->current_time, in_data->time_step,
                                            in_data->time_scale, &defs_[i]);
        if (err != PF_Err_NONE) {
            checkin(in_data);
            return err;
        }
        checked_out_[i] = true;
    }

    const double layer_width = static_cast<double>(in_data->width > 0 ? in_data->width : 1);
    const double layer_height = static_cast<double>(in_data->height > 0 ? in_data->height : 1);

    settings_.particle_count = to_particle_count(defs_[0]);
    settings_.birth_rate = to_double(defs_[1]);
    settings_.seed = to_seed(defs_[2]);
    settings_.particle_lifetime_seconds = to_double(defs_[3]);
    settings_.emitter_shape = core::emitter_shape_from_index(popup_index(defs_[4]));

    // Emitter Origin is an AE point control. AE delivers *absolute layer pixels* with
    // the origin at the layer's top-left, x growing right and y growing down; the
    // control's default is a percentage where 50 means "halfway" (SDK header note for
    // PF_Point3DDef). The core wants layer heights with the origin at the layer centre
    // and +Y up, so the conversion happens in testable core helpers (ADR 0003,
    // docs/parameter-mapping.md).
    const PF_Point3DDef& origin = defs_[5].u.point3d_d;
    raw_origin_ = core::Vec3{static_cast<double>(origin.x_value), static_cast<double>(origin.y_value),
                             static_cast<double>(origin.z_value)};

    core::LayerUnits units;
    units.layer_width = layer_width;
    units.layer_height = layer_height;
    units.pixel_aspect_ratio = host_pixel_aspect_ratio(*in_data);
    settings_.emitter_origin = core::layer_point_to_world(
        core::host_point_component_to_layer_pixels(raw_origin_.x, layer_width),
        core::host_point_component_to_layer_pixels(raw_origin_.y, layer_height),
        core::host_point_component_to_layer_pixels(raw_origin_.z, layer_height), units);

    // The velocity sliders are already in layer heights per second, which is the
    // core's world unit, so they need no conversion.
    settings_.velocity.x = to_double(defs_[6]);
    settings_.velocity.y = to_double(defs_[7]);
    settings_.velocity.z = to_double(defs_[8]);

    settings_.particle_size = to_double(defs_[9]);
    settings_.opacity = to_double(defs_[10]);

    // Emitter extent (cube edge for Box, diameter for Sphere/Disc) and the per-axis
    // velocity jitter are already in core units: layer heights and layer heights per
    // second.
    settings_.emitter_size = to_double(defs_[11]);
    settings_.velocity_spread = to_double(defs_[12]);

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

} // namespace starfield::adapter
