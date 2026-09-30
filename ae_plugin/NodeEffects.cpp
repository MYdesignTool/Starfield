#include "AEConfig.h"
#include "NodeEffects.hpp"

#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "NodeEffectFlags.h"
#include "PluginVersion.h"

#include <cstdio>
#include <utility>

namespace {

static_assert(STARFIELD_NODE_OUT_FLAGS == (PF_OutFlag_I_AM_OBSOLETE |
                                          PF_OutFlag_DEEP_COLOR_AWARE |
                                          PF_OutFlag_PIX_INDEPENDENT));
static_assert(STARFIELD_NODE_OUT_FLAGS2 == PF_OutFlag2_FLOAT_COLOR_AWARE);

enum class NodeEffectKind { emitter, particle, appearance, force };

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
                  PF_ParamFlags flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY |
                                        PF_ParamFlag_CANNOT_INTERP,
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
    def.flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP;
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
    def.flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP;
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
    def.flags = PF_ParamFlag_SUPERVISE | PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP;
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
                                        PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP,
                                        PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return add_slider(in_data, "Panel Sync Guard", fourcc('g', 's', 'y', 'n'),
                      0.0, 2147483647.0, 0.0, PF_Precision_INTEGER,
                      PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP,
                      PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
}

PF_Err add_curve_bank(PF_InData* in_data, const char* label, char prefix) noexcept {
    char name[48]{};
    PF_Err error = PF_Err_NONE;
    std::snprintf(name, sizeof(name), "%s Curve Count", label);
    error = add_slider(in_data, name, fourcc(prefix, 'c', 'n', 't'), 0.0, 8.0, 0.0,
                       PF_Precision_INTEGER, PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP,
                       PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
    if (error != PF_Err_NONE) return error;
    for (A_long point = 0; point < 8; ++point) {
        std::snprintf(name, sizeof(name), "%s Curve %ld Age", label, static_cast<long>(point));
        error = add_slider(in_data, name, fourcc(prefix, 'a', 'g', static_cast<char>('0' + point)),
                           0.0, 1.0, static_cast<PF_FpLong>(point) / 7.0, PF_Precision_THOUSANDTHS,
                           PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
        std::snprintf(name, sizeof(name), "%s Curve %ld Value", label, static_cast<long>(point));
        error = add_slider(in_data, name, fourcc(prefix, 'v', 'a', static_cast<char>('0' + point)),
                           0.0, 100.0, 100.0, PF_Precision_TENTHS,
                           PF_ParamFlag_CANNOT_TIME_VARY | PF_ParamFlag_CANNOT_INTERP,
                           PF_PUI_NO_ECW_UI | PF_PUI_INVISIBLE);
        if (error != PF_Err_NONE) return error;
    }
    return PF_Err_NONE;
}

PF_Err add_particle_parameters(PF_InData* in_data, bool include_lifetime) noexcept {
    constexpr A_long group_id = fourcc('p', 'a', 'r', 't');
    PF_Err error = add_group(in_data, "Particle", group_id, false);
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
    return add_group(in_data, "Particle", group_id, true);
}

PF_Err setup_emitter(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_group(in_data, "Emitter", fourcc('e', 'm', 'i', 't'), false);
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
    error = add_group(in_data, "Emitter", fourcc('e', 'm', 'i', 't'), true);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = 33; // 32 registered parameters (including group markers) plus AE's input layer.
    return PF_Err_NONE;
}

PF_Err setup_particle(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_particle_parameters(in_data, true);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = 55; // 54 registered controls plus AE's input layer.
    return PF_Err_NONE;
}

PF_Err setup_appearance(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_particle_parameters(in_data, false);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = 54; // 53 registered controls plus AE's input layer.
    return PF_Err_NONE;
}

PF_Err setup_force(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_Err error = add_group(in_data, "Force", fourcc('f', 'o', 'r', 'c'), false);
    if (error != PF_Err_NONE) return error;
    error = add_point3d(in_data, "Gravity", fourcc('g', 'r', 'a', 'v'), 0.0, 0.0, 0.0);
    if (error != PF_Err_NONE) return error;
    error = add_slider(in_data, "Linear Drag", fourcc('d', 'r', 'a', 'g'), 0.0, 100.0, 0.0,
                       PF_Precision_THOUSANDTHS);
    if (error != PF_Err_NONE) return error;
    error = add_group(in_data, "Force", fourcc('f', 'o', 'r', 'c'), true);
    if (error != PF_Err_NONE) return error;
    error = add_node_identity(in_data);
    if (error != PF_Err_NONE) return error;
    out_data->num_params = 14; // Thirteen registered controls plus AE's input layer.
    return PF_Err_NONE;
}

PF_Err render_passthrough(PF_InData* in_data, PF_ParamDef* params[], PF_LayerDef* output) noexcept {
    if (!in_data || !in_data->utils || !in_data->utils->copy || !params || !params[0] || !output) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_COPY(&params[0]->u.ld, output, nullptr, nullptr);
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
