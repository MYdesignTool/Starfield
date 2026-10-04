#include "AEConfig.h"
#include "NodeEffects.hpp"
#include "GpuRender.hpp"

#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "NodeEffectFlags.h"
#include "NodeRecord.hpp"
#include "ParticleGradientUI.hpp"
#include "PluginVersion.h"
#include "SPBasic.h"

#include <cstdio>
#include <utility>

namespace {

static_assert(STARFIELD_NODE_OUT_FLAGS == (PF_OutFlag_I_AM_OBSOLETE |
                                          PF_OutFlag_DEEP_COLOR_AWARE |
                                          PF_OutFlag_PIX_INDEPENDENT));
static_assert(STARFIELD_NODE_OUT_FLAGS2 == (PF_OutFlag2_SUPPORTS_SMART_RENDER |
                                           PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_GPU_RENDER_F32));
static_assert(STARFIELD_PARTICLE_OUT_FLAGS==(STARFIELD_NODE_OUT_FLAGS|PF_OutFlag_CUSTOM_UI));
static_assert(STARFIELD_PARTICLE_OUT_FLAGS2==(STARFIELD_NODE_OUT_FLAGS2|PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG));

enum class NodeEffectKind { emitter, particle, force };

// Authored controls animate; topology/identity/curve banks stay constant (ADR 0023).
// Do not add CANNOT_INTERP: AE chooses interpolation appropriate to each type.
constexpr PF_ParamFlags kNodeConstantFlags = PF_ParamFlag_CANNOT_TIME_VARY;
constexpr PF_ParamFlags kNodeEditableFlags = PF_ParamFlag_SUPERVISE;

#if defined(STARFIELD_NODE_KIND_EMITTER)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::emitter;
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::particle;
#elif defined(STARFIELD_NODE_KIND_FORCE)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::force;
#else
#error Define exactly one STARFIELD_NODE_KIND_* for each node module.
#endif

using namespace starfield::adapter::native_nodes;
using namespace starfield::adapter::native_nodes::disk_ids;

PF_Err add_checked_parameter(PF_InData* in_data, PF_ParamDef& def) noexcept {
    if (!valid_disk_id(def.uu.id)) return PF_Err_BAD_CALLBACK_PARAM;
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err add_slider(PF_InData* in_data, const char* name, A_long id,
                  PF_FpLong minimum, PF_FpLong maximum, PF_FpLong initial,
                  A_short precision = PF_Precision_HUNDREDTHS,
                  PF_ParamFlags flags = kNodeEditableFlags,
                  A_long ui_flags = PF_PUI_NONE, A_short height=0) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    def.flags = flags;
    def.ui_flags = ui_flags;def.ui_height=height;def.ui_width=300;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.fs_d.valid_min = static_cast<PF_FpShort>(minimum);
    def.u.fs_d.valid_max = static_cast<PF_FpShort>(maximum);
    def.u.fs_d.slider_min = static_cast<PF_FpShort>(minimum);
    def.u.fs_d.slider_max = static_cast<PF_FpShort>(maximum);
    // Valid typed ranges and normal drag ranges serve different purposes.
    // Large supported values must not make a one-pixel drag jump thousands.
    if (ui_flags == PF_PUI_NONE) {
        switch (id) {
            case kLifetimeId: def.u.fs_d.slider_max = 10.0f; break;
            case kSizeId: case kSizeYId: case kEmitterParticleSizeId: case kBirthRateId:
            case kSeedId: case kEmitterSizeXId: case kEmitterSizeYId: case kEmitterSizeZId:
            case kEmissionSpeedId: def.u.fs_d.slider_max = 100.0f; break;
            case kForceGravityId: case kWindXId: case kWindYId: case kWindZId:
                def.u.fs_d.slider_min = -100.0f; def.u.fs_d.slider_max = 100.0f; break;
            case kSpinId: def.u.fs_d.slider_max = 100.0f; break;
            case kSpinDelayId: def.u.fs_d.slider_max = 10.0f; break;
            case kOriginZId: def.u.fs_d.slider_min = -50.0f; def.u.fs_d.slider_max = 50.0f; break;
            case kSpinFrequencyId: case kAirDensityId: case kDiscSizeId: case kDragId: def.u.fs_d.slider_max = 1.0f; break;
            case kEmissionAngleXId: case kEmissionAngleYId: case kEmissionAngleZId:
                def.u.fs_d.slider_min = -180.0f; def.u.fs_d.slider_max = 180.0f; break;
        }
    }
    def.u.fs_d.value = initial;
    def.u.fs_d.dephault = static_cast<PF_FpShort>(initial);
    def.u.fs_d.precision = precision;
    def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
    if (id == kLifeRandomId || id == kFeatherId || id == kParticleAngleRandomId || id == kRotationSpeedRandomId || id == kGravityRandomId || id == kSpinResistId || id == kOpacityId || id == kSizeRandomId || id == kOpacityRandomId ||
        id == kEmissionSpeedRandomId || id == kSizeOverLifeId || id == kOpacityOverLifeId ||
        id == kEmitChanceId || id == kEmitLifeStartId || id == kEmitLifeEndId ||
        id == kInheritVelocityId || id == kInheritSizeId || id == kInheritOpacityId || id == kInheritColorId) {
        def.u.fs_d.display_flags = PF_ValueDisplayFlag_PERCENT;
    }
    def.u.fs_d.curve_tolerance = AEFX_AUDIO_DEFAULT_CURVE_TOLERANCE;
    return add_checked_parameter(in_data, def);
}

PF_Err add_popup(PF_InData* in_data, const char* name, A_long id,
                 A_short choice_count, A_short initial, const char* choices,
                 PF_ParamFlags flags=kNodeEditableFlags) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = flags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.pd.num_choices = choice_count;
    def.u.pd.value = def.u.pd.dephault = initial;
    def.u.pd.u.namesptr = choices;
    return add_checked_parameter(in_data, def);
}

PF_Err add_angle(PF_InData* in_data,const char* name,A_long id) noexcept {
    PF_ParamDef def{};
    def.param_type=PF_Param_ANGLE;def.flags=kNodeEditableFlags;def.uu.id=id;
    std::snprintf(def.name,sizeof(def.name),"%s",name);
    def.u.ad.value=def.u.ad.dephault=0; // Native AE turns + degrees and dial.
    return add_checked_parameter(in_data,def);
}

PF_Err add_point3d(PF_InData* in_data, const char* name, A_long id,
                   PF_FpLong x, PF_FpLong y, PF_FpLong z) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POINT_3D;
    def.flags = kNodeEditableFlags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.point3d_d.x_value = def.u.point3d_d.x_dephault = x;
    def.u.point3d_d.y_value = def.u.point3d_d.y_dephault = y;
    def.u.point3d_d.z_value = def.u.point3d_d.z_dephault = z;
    return add_checked_parameter(in_data, def);
}

PF_Err add_point2d(PF_InData* in_data, const char* name, A_long id) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POINT;
    def.flags = kNodeEditableFlags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.td.x_value = def.u.td.x_dephault = 50 << 16;
    def.u.td.y_value = def.u.td.y_dephault = 50 << 16;
    def.u.td.restrict_bounds = FALSE;
    return add_checked_parameter(in_data, def);
}

PF_Err add_color(PF_InData* in_data, const char* name, A_long id,
    PF_ParamFlags flags=kNodeEditableFlags,PF_ParamUIFlags ui=PF_PUI_NONE) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_COLOR;
    def.flags = flags;def.ui_flags=ui;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.cd.value = PF_Pixel{255, 255, 255, 255};
    def.u.cd.dephault = def.u.cd.value;
    return add_checked_parameter(in_data, def);
}

PF_Err add_node_identity(PF_InData* in_data) noexcept {
    // Eight exact 16-bit chunks preserve the complete graph UUID. Each chunk is
    // exactly representable in the slider's 32-bit range fields. Graph-canvas
    // duplication allocates a fresh UUID; the gateway rejects ambiguous AE-level
    // copies instead of silently binding the first matching effect.
    constexpr PF_FpLong kU16Max = 65535.0;
    for (A_long chunk = 0; chunk < 8; ++chunk) {
        char name[24]{};
        std::snprintf(name, sizeof(name), "Node UUID %ld", static_cast<long>(chunk));
        const PF_Err error = add_slider(in_data, name, uuid_id(chunk),
                                        0.0, kU16Max, 0.0, PF_Precision_INTEGER,
                                        kNodeConstantFlags,
                                        PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return add_slider(in_data, "Panel Sync Guard", kSyncGuardId,
                      0.0, 2147483647.0, 0.0, PF_Precision_INTEGER,
                      kNodeConstantFlags,
                      PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
}

PF_Err add_node_record(PF_InData* in_data, starfield::adapter::native_nodes::Kind kind) noexcept {
    using namespace starfield::adapter::native_nodes;
    PF_Err error = add_slider(in_data, "Node Layout X", layout_x_id(), -1000000000.0, 1000000000.0, 0.0,
                              PF_Precision_TENTHS, kNodeConstantFlags,
                              PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Node Layout Y", layout_y_id(), -1000000000.0, 1000000000.0, 0.0,
                       PF_Precision_TENTHS, kNodeConstantFlags,
                       PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Outgoing Connection Count", connection_count_id(), 0.0,
                       static_cast<PF_FpLong>(kMaxOutgoingEdges), 0.0, PF_Precision_INTEGER,
                       kNodeConstantFlags,
                       PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    for (A_long slot = 0; slot < kMaxOutgoingEdges; ++slot) {
        for (A_long chunk = 0; chunk < kConnectionUuidChunks; ++chunk) {
            char name[48]{};
            std::snprintf(name, sizeof(name), "Connection %ld Target UUID %ld",
                          static_cast<long>(slot), static_cast<long>(chunk));
            error = add_slider(in_data, name, connection_uuid_id(slot, chunk), 0.0, 65535.0, 0.0,
                               PF_Precision_INTEGER,
                               kNodeConstantFlags,
                               PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
            if (error != PF_Err_NONE) return error;
        }
        for (A_long chunk = 0; chunk < kConnectionUuidChunks; ++chunk) {
            char name[48]{};
            std::snprintf(name, sizeof(name), "Connection %ld Edge UUID %ld",
                          static_cast<long>(slot), static_cast<long>(chunk));
            error = add_slider(in_data, name, connection_edge_uuid_id(slot, chunk), 0.0, 65535.0, 0.0,
                               PF_Precision_INTEGER,
                               kNodeConstantFlags,
                               PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
            if (error != PF_Err_NONE) return error;
        }
    }
    (void)kind; // Kind determines the shared stream indices used by the compiler.
    return PF_Err_NONE;
}

PF_Err add_curve_bank(PF_InData* in_data, const char* label, char prefix) noexcept {
    char name[48]{};
    PF_Err error = PF_Err_NONE;
    std::snprintf(name, sizeof(name), "%s Curve Count", label);
    error = add_slider(in_data, name, curve_count_id(prefix), 0.0, 8.0, 0.0,
                       PF_Precision_INTEGER, kNodeConstantFlags,
                       PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    for (A_long point = 0; point < 8; ++point) {
        std::snprintf(name, sizeof(name), "%s Curve %ld Age", label, static_cast<long>(point));
        error = add_slider(in_data, name, curve_age_id(prefix, point),
                           0.0, 1.0, static_cast<PF_FpLong>(point) / 7.0, PF_Precision_THOUSANDTHS,
                           kNodeConstantFlags,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
        std::snprintf(name, sizeof(name), "%s Curve %ld Value", label, static_cast<long>(point));
        error = add_slider(in_data, name, curve_value_id(prefix, point),
                           0.0, 100.0, 100.0, PF_Precision_TENTHS,
                           kNodeConstantFlags,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return PF_Err_NONE;
}

PF_Err particle_group(PF_InData* data,const char* name,A_long id,bool end=false,bool collapsed=false) noexcept {
    PF_ParamDef def{};def.param_type=end?PF_Param_GROUP_END:PF_Param_GROUP_START;def.uu.id=id;
    def.flags=collapsed?PF_ParamFlag_START_COLLAPSED:0;std::snprintf(def.name,sizeof(def.name),"%s",name);
    return add_checked_parameter(data,def);
}

PF_Err add_particle_parameters(PF_InData* in_data) noexcept {
    PF_Err error = PF_Err_NONE;
    {
        error=add_popup(in_data,"Shape",kParticleShapeId,3,1,"Circle|Rectangle|Cloud");if(error)return error;
        error=add_slider(in_data,"Life (Seconds)",kLifetimeId,0,10000,2,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Life Random",kLifeRandomId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        error=particle_group(in_data,"Particle Properties",kParticlePropertiesId);if(error)return error;
        error=add_slider(in_data,"Size (Pixels)",kSizeId,0,100000,10,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Size Y (Pixels)",kSizeYId,0,100000,10,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Size Random",kSizeRandomId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Opacity",kOpacityId,0,100,100,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Opacity Random",kOpacityRandomId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        error=add_popup(in_data,"Particle Color",kParticleColorModeId,4,1,"Solid color|Color over life|Random from gradient|Loop from grad");if(error)return error;
        error=add_color(in_data,"Color",kColorStartId);if(error)return error;
        error=add_slider(in_data,"Color Gradient",kColorGradientCountId,2,8,2,PF_Precision_INTEGER,kNodeConstantFlags|PF_ParamFlag_SUPERVISE,PF_PUI_CONTROL,178);if(error)return error;
        for(A_long i=0;i<8;++i) {
            char name[48]{};
            const auto ui=PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE;
            const auto flags=PF_ParamFlag_CANNOT_TIME_VARY|PF_ParamFlag_SUPERVISE;
            std::snprintf(name,sizeof(name),"Color Gradient %ld Position",static_cast<long>(i));
            error=add_slider(in_data,name,kColorGradientPositionFirstId+i,0,100,i==1?100:double(i)*100/7,
                PF_Precision_TENTHS,flags,ui);if(error)return error;
            std::snprintf(name,sizeof(name),"Color Gradient %ld Color",static_cast<long>(i));
            error=add_color(in_data,name,kColorGradientColorFirstId+i,flags,ui);if(error)return error;
        }
        error=add_slider(in_data,"Particle Feather",kFeatherId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        error=add_popup(in_data,"Up Axis",kUpAxisId,3,3,"X|Y|Z");if(error)return error;
        error=particle_group(in_data,"",kParticlePropertiesEndId,true);if(error)return error;
        error=particle_group(in_data,"Over Life",kParticleOverLifeId,false,true);if(error)return error;
        error=add_slider(in_data,"Size Over Life",kSizeOverLifeId,0,100,100,PF_Precision_TENTHS);if(error)return error;
        error=add_slider(in_data,"Opacity Over Life",kOpacityOverLifeId,0,100,100,PF_Precision_TENTHS);if(error)return error;
        error=add_curve_bank(in_data,"Size",'s');if(error)return error;
        error=add_curve_bank(in_data,"Opacity",'o');if(error)return error;
        error=particle_group(in_data,"",kParticleOverLifeEndId,true);if(error)return error;
        error=particle_group(in_data,"Rotation",kParticleRotationId,false,true);if(error)return error;
        error=add_popup(in_data,"Orient To",kOrientToId,3,1,"None|Motion(particle)|Emitter");if(error)return error;
        for(auto axis:{std::pair{"X",kParticleAngleXId},std::pair{"Y",kParticleAngleYId},std::pair{"Z",kParticleAngleZId}}) {
            char name[24]{};std::snprintf(name,sizeof(name),"Angle %s",axis.first);
            error=add_angle(in_data,name,axis.second);if(error)return error;
        }
        error=add_slider(in_data,"Angle Random",kParticleAngleRandomId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        for(auto axis:{std::pair{"X",kRotationSpeedXId},std::pair{"Y",kRotationSpeedYId},std::pair{"Z",kRotationSpeedZId}}) {
            char name[24]{};std::snprintf(name,sizeof(name),"Speed %s",axis.first);
            error=add_angle(in_data,name,axis.second);if(error)return error;
        }
        error=add_slider(in_data,"Speed Random",kRotationSpeedRandomId,0,100,0,PF_Precision_TENTHS);if(error)return error;
        error=add_popup(in_data,"Limit to 2D",kLimitTo2DId,2,2,"Off|On");if(error)return error;
        error=particle_group(in_data,"",kParticleRotationEndId,true);if(error)return error;
    }
    return PF_Err_NONE;
}

PF_Err setup_emitter(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_popup(in_data, "Type", kEmitterTypeId, 4, 1,
                             "Point|Box|Sphere|Disc");
    if (error != PF_Err_NONE) return error;
    error = add_popup(in_data, "Emitting", kEmittingModeId, 4, 1, "Default|Once|Sequenced|Randomized",kNodeConstantFlags);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Particles Per Second", kBirthRateId,
                       0.0, 1000000.0, 100.0, PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_point2d(in_data, "Origin XY", kOriginXYId);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Origin Z", kOriginZId, -10000000.0, 10000000.0, 0.0,
                       PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;

    error = add_slider(in_data, "Speed", kEmissionSpeedId, 0.0, 10000.0, 100.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Speed Random", kEmissionSpeedRandomId, 0.0, 100.0, 0.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    for (const auto& axis : {std::pair{"X", kEmitterSizeXId}, std::pair{"Y", kEmitterSizeYId}, std::pair{"Z", kEmitterSizeZId}}) {
        char name[24]{};
        std::snprintf(name, sizeof(name), "Size %s", axis.first);
        error = add_slider(in_data, name, axis.second, 0.0, 100000.0, 100.0,
                           PF_Precision_INTEGER);
        if (error != PF_Err_NONE) return error;
    }
    error = add_slider(in_data, "Disc Size", kDiscSizeId, 0.0, 10.0, 0.05,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    for (const auto& axis : {std::pair{"X", kEmissionAngleXId}, std::pair{"Y", kEmissionAngleYId}, std::pair{"Z", kEmissionAngleZId}}) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "Angle %s", axis.first);
        error = add_angle(in_data, name, axis.second);
        if (error != PF_Err_NONE) return error;
    }
    error = add_popup(in_data, "Direction", kDirectionId, 2, 2,
                      "Directional|Uniform");
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Direction Span", kDirectionSpanId, 0.0, 180.0, 60.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    for (const auto& control : {
            std::pair{"Emit Chance", kEmitChanceId}, std::pair{"Emit Life Start", kEmitLifeStartId},
            std::pair{"Emit Life End", kEmitLifeEndId}, std::pair{"Inherit Velocity", kInheritVelocityId},
            std::pair{"Inherit Size", kInheritSizeId}, std::pair{"Inherit Opacity", kInheritOpacityId},
            std::pair{"Inherit Color", kInheritColorId}}) {
        const double initial = control.second == kEmitChanceId || control.second == kEmitLifeEndId ? 100.0 : 0.0;
        error = add_slider(in_data, control.first, control.second, 0.0, 100.0, initial, PF_Precision_TENTHS);
        if (error != PF_Err_NONE) return error;
    }
    error = add_slider(in_data, "Random Seed", kSeedId,
                       0.0, 2147483647.0, 1000.0, PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Particle Size", kEmitterParticleSizeId, 0.0, 100000.0, 10.0,
                       PF_Precision_TENTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Opacity", kOpacityId, 0.0, 100.0, 100.0,
                       PF_Precision_TENTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity X", kVelocityXId, -1000.0, 1000.0, 0.0,
                       PF_Precision_HUNDREDTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity Y", kVelocityYId, -1000.0, 1000.0, 0.0,
                       PF_Precision_HUNDREDTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity Z", kVelocityZId, -1000.0, 1000.0, 0.0,
                       PF_Precision_HUNDREDTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity Random", kSpeedRandomId, 0.0, 100.0, 0.0,
                       PF_Precision_HUNDREDTHS, kNodeEditableFlags, PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    error=add_slider(in_data,"Auxiliary Source",kAuxiliarySourceId,0,1,0,PF_Precision_INTEGER,
        kNodeConstantFlags,PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE);
    if(error) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::emitter);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::emitter);
    return PF_Err_NONE;
}

PF_Err setup_particle(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_particle_parameters(in_data);
    if (error != PF_Err_NONE) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::particle);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::particle);
    PF_CustomUIInfo ui{};ui.events=PF_CustomEFlag_EFFECT;
    return in_data->inter.register_ui?PF_REGISTER_UI(in_data,&ui):PF_Err_NONE;
}

PF_Err setup_force(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_slider(in_data, "Gravity", kForceGravityId, -100000, 100000, 0, PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Gravity random", kGravityRandomId, 0, 100, 0, PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    for (const auto& control : {std::pair{"Wind X", kWindXId}, std::pair{"Wind Y", kWindYId}, std::pair{"Wind Z", kWindZId}}) {
        error = add_slider(in_data, control.first, control.second, -100000, 100000, 0, PF_Precision_TENTHS);
        if (error != PF_Err_NONE) return error;
    }
    error = add_slider(in_data, "Spin", kSpinId, 0, 100000, 0, PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Spin Frequency", kSpinFrequencyId, 0, 1000, 0, PF_Precision_HUNDREDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Spin resist", kSpinResistId, 0, 100, 0, PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Spin Delay (Seconds)", kSpinDelayId, 0, 10000, 0, PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Air Density", kAirDensityId, 0, 100, 0, PF_Precision_HUNDREDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_curve_bank(in_data, "Wind and Spin", 'w');
    if (error != PF_Err_NONE) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::force);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::force);
    return PF_Err_NONE;
}

PF_Err render_passthrough(PF_InData* in_data, PF_ParamDef* params[], PF_LayerDef* output) noexcept {
    if (!in_data || !in_data->utils || !in_data->utils->copy || !params || !params[0] || !output) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_COPY(&params[0]->u.ld, output, nullptr, nullptr);
}

// Node effects carry controls only. Forward the exact input request and let the
// host copy its pixel world; legacy PF_COPY cannot be the 32-bpc render path.
constexpr A_long kNodeInputCheckout = 0;

PF_Err pre_render_passthrough(PF_InData* in_data, PF_PreRenderExtra* extra) noexcept {
    if (!in_data || !extra || !extra->input || !extra->output || !extra->cb ||
        !extra->cb->checkout_layer) return PF_Err_BAD_CALLBACK_PARAM;
    PF_CheckoutResult result{};
    const PF_Err error = extra->cb->checkout_layer(in_data->effect_ref, 0, kNodeInputCheckout,
        &extra->input->output_request, in_data->current_time, in_data->time_step,
        in_data->time_scale, &result);
    if (error) return error;
    extra->output->result_rect = result.result_rect;
    extra->output->max_result_rect = result.max_result_rect;
    extra->output->solid = result.solid;
    extra->output->flags = starfield::adapter::gpu_device_matches(extra->input->gpu_data,extra->input->what_gpu,extra->input->device_index)?PF_RenderOutputFlag_GPU_RENDER_POSSIBLE:0;
    extra->output->pre_render_data = nullptr;
    extra->output->delete_pre_render_data_func = nullptr;
    return PF_Err_NONE;
}

PF_Err copy_smart_world(PF_InData* in_data, PF_EffectWorld* input, PF_EffectWorld* output) noexcept {
    if (!input || !output) return PF_Err_BAD_CALLBACK_PARAM;
    if (output->width == 0 || output->height == 0) return PF_Err_NONE;
    if (!in_data->pica_basicP || !in_data->pica_basicP->AcquireSuite ||
        !in_data->pica_basicP->ReleaseSuite) return PF_Err_BAD_CALLBACK_PARAM;
    const PF_WorldTransformSuite1* suite = nullptr;
    PF_Err error = static_cast<PF_Err>(in_data->pica_basicP->AcquireSuite(kPFWorldTransformSuite,
        kPFWorldTransformSuiteVersion1, reinterpret_cast<const void**>(&suite)));
    if (error) return error;
    error = suite && suite->copy ? suite->copy(in_data->effect_ref, input, output, nullptr, nullptr) :
        PF_Err_BAD_CALLBACK_PARAM;
    const PF_Err release_error = static_cast<PF_Err>(in_data->pica_basicP->ReleaseSuite(
        kPFWorldTransformSuite, kPFWorldTransformSuiteVersion1));
    return error ? error : release_error;
}

PF_Err smart_render_passthrough(PF_InData* in_data, PF_SmartRenderExtra* extra) noexcept {
    if (!in_data || !extra || !extra->input || !extra->cb || !extra->cb->checkout_layer_pixels ||
        !extra->cb->checkout_output || !extra->cb->checkin_layer_pixels) return PF_Err_BAD_CALLBACK_PARAM;
    PF_EffectWorld* input = nullptr;
    PF_Err error = extra->cb->checkout_layer_pixels(in_data->effect_ref, kNodeInputCheckout, &input);
    if (error) return error;
    PF_EffectWorld* output = nullptr;
    error = extra->cb->checkout_output(in_data->effect_ref, &output);
    if (!error) error = extra->input->what_gpu==PF_GPU_Framework_NONE?copy_smart_world(in_data,input,output):
        starfield::adapter::copy_gpu_pixels(in_data,extra->input->gpu_data,extra->input->what_gpu,extra->input->device_index,input,output);
    // A successful pixel checkout is checked in even if output/suite/copy fails.
    const PF_Err checkin_error = extra->cb->checkin_layer_pixels(in_data->effect_ref, kNodeInputCheckout);
    return error ? error : checkin_error;
}

PF_Err dispatch(PF_Cmd command, PF_InData* in_data, PF_OutData* out_data,
                PF_ParamDef* params[], PF_LayerDef* output, void* extra) noexcept {
    if (!out_data) return PF_Err_BAD_CALLBACK_PARAM;
    switch (command) {
        case PF_Cmd_GLOBAL_SETUP:
            out_data->my_version = PF_VERSION(STARFIELD_VERSION_MAJOR, STARFIELD_VERSION_MINOR,
                                              STARFIELD_VERSION_BUG, STARFIELD_VERSION_STAGE,
                                              STARFIELD_VERSION_BUILD);
            out_data->out_flags = kNodeEffectKind==NodeEffectKind::particle?STARFIELD_PARTICLE_OUT_FLAGS:STARFIELD_NODE_OUT_FLAGS;
            out_data->out_flags2 = kNodeEffectKind==NodeEffectKind::particle?STARFIELD_PARTICLE_OUT_FLAGS2:STARFIELD_NODE_OUT_FLAGS2;
            if (register_node_graph_sync(in_data) != PF_Err_NONE) {
                std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                              "Starfield node parameter synchronization is unavailable.");
            }
            return PF_Err_NONE;
        case PF_Cmd_PARAMS_SETUP:
            if (!in_data) return PF_Err_BAD_CALLBACK_PARAM;
            if constexpr (kNodeEffectKind == NodeEffectKind::emitter) {
                return setup_emitter(in_data, out_data);
            } else if constexpr (kNodeEffectKind == NodeEffectKind::particle) {
                return setup_particle(in_data, out_data);

            } else {
                return setup_force(in_data, out_data);
            }
        case PF_Cmd_SEQUENCE_SETUP:
            out_data->sequence_data = nullptr;
            return PF_Err_NONE;
        case PF_Cmd_SEQUENCE_RESETUP:
        case PF_Cmd_SEQUENCE_FLATTEN:
        case PF_Cmd_SEQUENCE_SETDOWN:
            return PF_Err_NONE;
        case PF_Cmd_GLOBAL_SETDOWN:
            if constexpr(kNodeEffectKind==NodeEffectKind::particle)starfield::adapter::clear_particle_gradient_ui();
            return PF_Err_NONE;
        case PF_Cmd_EVENT:
            if constexpr(kNodeEffectKind==NodeEffectKind::particle) {
                return starfield::adapter::particle_gradient_event(in_data,out_data,params,static_cast<PF_EventExtra*>(extra));
            } else return PF_Err_NONE;
        case PF_Cmd_RENDER:
            return render_passthrough(in_data, params, output);
        case PF_Cmd_SMART_PRE_RENDER:
            return pre_render_passthrough(in_data, static_cast<PF_PreRenderExtra*>(extra));
        case PF_Cmd_GPU_DEVICE_SETUP:
            return starfield::adapter::gpu_device_setup(in_data,out_data,static_cast<PF_GPUDeviceSetupExtra*>(extra));
        case PF_Cmd_GPU_DEVICE_SETDOWN:
            return starfield::adapter::gpu_device_setdown(in_data,static_cast<PF_GPUDeviceSetdownExtra*>(extra));
        case PF_Cmd_SMART_RENDER_GPU:
        case PF_Cmd_SMART_RENDER:
            return smart_render_passthrough(in_data, static_cast<PF_SmartRenderExtra*>(extra));
        case PF_Cmd_USER_CHANGED_PARAM:
            if (!in_data) return PF_Err_BAD_CALLBACK_PARAM;
            return sync_node_graph_parameter(in_data, out_data, params,
                static_cast<const PF_UserChangedParamExtra*>(extra));
        case PF_Cmd_UPDATE_PARAMS_UI:
            if constexpr(kNodeEffectKind==NodeEffectKind::particle) {
                return starfield::adapter::particle_gradient_param_ui(in_data,params);
            } else return PF_Err_NONE;
        default:
            return PF_Err_NONE;
    }
}

PF_Err guarded(PF_Cmd command, PF_InData* in_data, PF_OutData* out_data,
               PF_ParamDef* params[], PF_LayerDef* output, void* extra) noexcept {
    try { return dispatch(command, in_data, out_data, params, output, extra); }
    catch (PF_Err error) { return error; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace

extern "C" DllExport PF_Err EffectMain(PF_Cmd command, PF_InData* in_data, PF_OutData* out_data,
                                        PF_ParamDef* params[], PF_LayerDef* output, void* extra) {
    return guarded(command, in_data, out_data, params, output, extra);
}
