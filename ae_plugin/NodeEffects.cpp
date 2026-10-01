#include "AEConfig.h"
#include "NodeEffects.hpp"

#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "NodeEffectFlags.h"
#include "NodeRecord.hpp"
#include "PluginVersion.h"
#include "SPBasic.h"

#include <cstdio>
#include <utility>

namespace {

static_assert(STARFIELD_NODE_OUT_FLAGS == (PF_OutFlag_I_AM_OBSOLETE |
                                          PF_OutFlag_DEEP_COLOR_AWARE |
                                          PF_OutFlag_PIX_INDEPENDENT));
static_assert(STARFIELD_NODE_OUT_FLAGS2 == (PF_OutFlag2_SUPPORTS_SMART_RENDER |
                                           PF_OutFlag2_FLOAT_COLOR_AWARE));

enum class NodeEffectKind { emitter, particle, appearance, force };

// Node records are constants. CANNOT_TIME_VARY is sufficient: CANNOT_INTERP
// additionally asks AE to configure interpolation when creating the stream,
// including non-spatial color controls. Do not request interpolation setup for
// a control that cannot have keyframes in the first place.
constexpr PF_ParamFlags kNodeConstantFlags = PF_ParamFlag_CANNOT_TIME_VARY;
constexpr PF_ParamFlags kNodeEditableFlags = PF_ParamFlag_SUPERVISE | kNodeConstantFlags;

#if defined(STARFIELD_NODE_KIND_EMITTER)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::emitter;
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::particle;
#elif defined(STARFIELD_NODE_KIND_APPEARANCE)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::appearance;
#elif defined(STARFIELD_NODE_KIND_FORCE)
constexpr NodeEffectKind kNodeEffectKind = NodeEffectKind::force;
#else
#error Define exactly one STARFIELD_NODE_KIND_* for each node module.
#endif

constexpr A_long fourcc(char a, char b, char c, char d) noexcept {
    return (static_cast<A_long>(static_cast<unsigned char>(a)) << 24) |
           (static_cast<A_long>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<A_long>(static_cast<unsigned char>(c)) << 8) |
           static_cast<A_long>(static_cast<unsigned char>(d));
}

// Both topic boundaries are registered parameters. Each needs a distinct disk
// ID; reusing the start ID for GROUP_END can create duplicate AE match names.
constexpr A_long kEmitterGroupStartId = fourcc('e', 'm', 'i', 't');
constexpr A_long kEmitterGroupEndId = fourcc('e', 'n', 'd', 'E');
constexpr A_long kParticleGroupStartId = fourcc('p', 'a', 'r', 't');
constexpr A_long kParticleGroupEndId = fourcc('e', 'n', 'd', 'P');
constexpr A_long kForceGroupStartId = fourcc('f', 'o', 'r', 'c');
constexpr A_long kForceGroupEndId = fourcc('e', 'n', 'd', 'F');
static_assert(kEmitterGroupStartId != kEmitterGroupEndId &&
              kParticleGroupStartId != kParticleGroupEndId &&
              kForceGroupStartId != kForceGroupEndId);

PF_Err add_group(PF_InData* in_data, const char* name, A_long id, bool end) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = end ? PF_Param_GROUP_END : PF_Param_GROUP_START;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err add_slider(PF_InData* in_data, const char* name, A_long id,
                  PF_FpLong minimum, PF_FpLong maximum, PF_FpLong initial,
                  A_short precision = PF_Precision_HUNDREDTHS,
                  PF_ParamFlags flags = kNodeEditableFlags,
                  A_long ui_flags = PF_PUI_NONE) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_FLOAT_SLIDER;
    def.flags = flags;
    def.ui_flags = ui_flags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.fs_d.valid_min = static_cast<PF_FpShort>(minimum);
    def.u.fs_d.valid_max = static_cast<PF_FpShort>(maximum);
    def.u.fs_d.slider_min = static_cast<PF_FpShort>(minimum);
    def.u.fs_d.slider_max = static_cast<PF_FpShort>(maximum);
    def.u.fs_d.value = initial;
    def.u.fs_d.dephault = static_cast<PF_FpShort>(initial);
    def.u.fs_d.precision = precision;
    def.u.fs_d.display_flags = PF_ValueDisplayFlag_NONE;
    def.u.fs_d.curve_tolerance = AEFX_AUDIO_DEFAULT_CURVE_TOLERANCE;
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err add_popup(PF_InData* in_data, const char* name, A_long id,
                 A_short choice_count, A_short initial, const char* choices) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_POPUP;
    def.flags = kNodeEditableFlags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.pd.num_choices = choice_count;
    def.u.pd.value = def.u.pd.dephault = initial;
    def.u.pd.u.namesptr = choices;
    return PF_ADD_PARAM(in_data, -1, &def);
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
    return PF_ADD_PARAM(in_data, -1, &def);
}

PF_Err add_color(PF_InData* in_data, const char* name, A_long id) noexcept {
    PF_ParamDef def{};
    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_COLOR;
    def.flags = kNodeEditableFlags;
    std::snprintf(def.name, sizeof(def.name), "%s", name);
    def.uu.id = id;
    def.u.cd.value = PF_Pixel{255, 255, 255, 255};
    def.u.cd.dephault = def.u.cd.value;
    return PF_ADD_PARAM(in_data, -1, &def);
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
        const PF_Err error = add_slider(in_data, name, fourcc('u', 'i', 'd', static_cast<char>('0' + chunk)),
                                        0.0, kU16Max, 0.0, PF_Precision_INTEGER,
                                        kNodeConstantFlags,
                                        PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return add_slider(in_data, "Panel Sync Guard", fourcc('g', 's', 'y', 'n'),
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
    error = add_slider(in_data, name, fourcc(prefix, 'c', 'n', 't'), 0.0, 8.0, 0.0,
                       PF_Precision_INTEGER, kNodeConstantFlags,
                       PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    for (A_long point = 0; point < 8; ++point) {
        std::snprintf(name, sizeof(name), "%s Curve %ld Age", label, static_cast<long>(point));
        error = add_slider(in_data, name, fourcc(prefix, 'a', 'g', static_cast<char>('0' + point)),
                           0.0, 1.0, static_cast<PF_FpLong>(point) / 7.0, PF_Precision_THOUSANDTHS,
                           kNodeConstantFlags,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
        std::snprintf(name, sizeof(name), "%s Curve %ld Value", label, static_cast<long>(point));
        error = add_slider(in_data, name, fourcc(prefix, 'v', 'a', static_cast<char>('0' + point)),
                           0.0, 100.0, 100.0, PF_Precision_TENTHS,
                           kNodeConstantFlags,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return PF_Err_NONE;
}

PF_Err add_particle_parameters(PF_InData* in_data, bool include_lifetime) noexcept {
    PF_Err error = add_group(in_data, "Particle", kParticleGroupStartId, false);
    if (error != PF_Err_NONE) return error;
    if (include_lifetime) {
        error = add_slider(in_data, "Lifetime", fourcc('l', 'i', 'f', 'e'), 0.0, 1000000.0, 2.0,
                           PF_Precision_THOUSANDTHS);
        if (error != PF_Err_NONE) return error;
    }
    error = add_slider(in_data, "Size", fourcc('s', 'i', 'z', 'e'), 0.0, 100000.0, 10.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Size Over Life", fourcc('s', 'z', 'e', 'n'), 0.0, 100.0, 100.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Opacity", fourcc('o', 'p', 'a', 'c'), 0.0, 1.0, 1.0,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Opacity Over Life", fourcc('o', 'p', 'e', 'n'), 0.0, 100.0, 100.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_color(in_data, "Color Start", fourcc('c', 'l', 'r', 's'));
    if (error != PF_Err_NONE) return error;
    error = add_color(in_data, "Color End", fourcc('c', 'l', 'r', 'e'));
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Size Random", fourcc('s', 'z', 'r', 'd'), 0.0, 100.0, 0.0,
                       PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Opacity Random", fourcc('o', 'p', 'r', 'd'), 0.0, 100.0, 0.0,
                       PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_curve_bank(in_data, "Size", 's');
    if (error != PF_Err_NONE) return error;
    error = add_curve_bank(in_data, "Opacity", 'o');
    if (error != PF_Err_NONE) return error;
    return add_group(in_data, "Particle", kParticleGroupEndId, true);
}

PF_Err setup_emitter(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_group(in_data, "Emitter", kEmitterGroupStartId, false);
    if (error != PF_Err_NONE) return error;
    error = add_popup(in_data, "Type", fourcc('e', 's', 'h', 'a'), 4, 1,
                             "Point|Box|Sphere|Disc");
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Particles Per Second", fourcc('b', 'r', 't', 'h'),
                       0.0, 1000000.0, 30.0, PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Random Seed", fourcc('s', 'e', 'e', 'd'),
                       0.0, 2147483647.0, 1.0, PF_Precision_INTEGER);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Particle Size", fourcc('p', 's', 'i', 'z'), 0.0, 100000.0, 10.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Opacity", fourcc('o', 'p', 'a', 'c'), 0.0, 1.0, 1.0,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_point3d(in_data, "Origin", fourcc('e', 'p', 'o', 's'), 50.0, 50.0, 50.0);
    if (error != PF_Err_NONE) return error;

    error = add_slider(in_data, "Velocity X", fourcc('v', 'e', 'l', 'x'), -1000.0, 1000.0, 0.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity Y", fourcc('v', 'e', 'l', 'y'), -1000.0, 1000.0, 0.3);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Velocity Z", fourcc('v', 'e', 'l', 'z'), -1000.0, 1000.0, 0.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Disc Size", fourcc('d', 's', 'i', 'z'), 0.0, 10.0, 0.05,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Speed Random", fourcc('v', 's', 'p', 'd'), 0.0, 100.0, 0.15);
    if (error != PF_Err_NONE) return error;
    for (const auto& axis : {std::pair{"X", 'x'}, std::pair{"Y", 'y'}, std::pair{"Z", 'z'}}) {
        char name[24]{};
        std::snprintf(name, sizeof(name), "Size %s", axis.first);
        error = add_slider(in_data, name, fourcc('e', 's', 'z', axis.second), 0.0, 100000.0, 100.0,
                           PF_Precision_INTEGER);
        if (error != PF_Err_NONE) return error;
    }
    error = add_slider(in_data, "Emission Speed", fourcc('e', 'm', 's', 'p'), 0.0, 1000.0, 0.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Emission Speed Random", fourcc('e', 'm', 'r', 'd'), 0.0, 1000.0, 0.0);
    if (error != PF_Err_NONE) return error;
    for (const auto& axis : {std::pair{"X", 'x'}, std::pair{"Y", 'y'}, std::pair{"Z", 'z'}}) {
        char name[32]{};
        std::snprintf(name, sizeof(name), "Emission Angle %s", axis.first);
        error = add_slider(in_data, name, fourcc('e', 'a', 'n', axis.second), -100000.0, 100000.0, 0.0,
                           PF_Precision_TENTHS);
        if (error != PF_Err_NONE) return error;
    }
    error = add_popup(in_data, "Direction", fourcc('d', 'i', 'r', 'm'), 2, 1,
                      "Directional|Uniform");
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Direction Span", fourcc('d', 's', 'p', 'n'), 0.0, 180.0, 60.0,
                       PF_Precision_TENTHS);
    if (error != PF_Err_NONE) return error;
    error = add_group(in_data, "Emitter", kEmitterGroupEndId, true);
    if (error != PF_Err_NONE) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::emitter);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::emitter);
    return PF_Err_NONE;
}

PF_Err setup_particle(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_particle_parameters(in_data, true);
    if (error != PF_Err_NONE) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::particle);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::particle);
    return PF_Err_NONE;
}

PF_Err setup_appearance(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_particle_parameters(in_data, false);
    if (error != PF_Err_NONE) return error;
    error = add_node_record(in_data, starfield::adapter::native_nodes::Kind::appearance);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = starfield::adapter::native_nodes::parameter_count(
        starfield::adapter::native_nodes::Kind::appearance);
    return PF_Err_NONE;
}

PF_Err setup_force(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_group(in_data, "Force", kForceGroupStartId, false);
    if (error != PF_Err_NONE) return error;
    error = add_point3d(in_data, "Gravity", fourcc('g', 'r', 'a', 'v'), 0.0, 0.0, 0.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Linear Drag", fourcc('d', 'r', 'a', 'g'), 0.0, 100.0, 0.0,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_group(in_data, "Force", kForceGroupEndId, true);
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
    extra->output->flags = 0;
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
    if (!error) error = copy_smart_world(in_data, input, output);
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
            out_data->out_flags = STARFIELD_NODE_OUT_FLAGS;
            out_data->out_flags2 = STARFIELD_NODE_OUT_FLAGS2;
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
            } else if constexpr (kNodeEffectKind == NodeEffectKind::appearance) {
                return setup_appearance(in_data, out_data);
            } else {
                return setup_force(in_data, out_data);
            }
        case PF_Cmd_SEQUENCE_SETUP:
            out_data->sequence_data = nullptr;
            return PF_Err_NONE;
        case PF_Cmd_SEQUENCE_RESETUP:
        case PF_Cmd_SEQUENCE_FLATTEN:
        case PF_Cmd_SEQUENCE_SETDOWN:
        case PF_Cmd_GLOBAL_SETDOWN:
            return PF_Err_NONE;
        case PF_Cmd_RENDER:
            return render_passthrough(in_data, params, output);
        case PF_Cmd_SMART_PRE_RENDER:
            return pre_render_passthrough(in_data, static_cast<PF_PreRenderExtra*>(extra));
        case PF_Cmd_SMART_RENDER:
            return smart_render_passthrough(in_data, static_cast<PF_SmartRenderExtra*>(extra));
        case PF_Cmd_USER_CHANGED_PARAM:
            if (!in_data) return PF_Err_BAD_CALLBACK_PARAM;
            return sync_node_graph_parameter(in_data, out_data, params,
                static_cast<const PF_UserChangedParamExtra*>(extra));
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
