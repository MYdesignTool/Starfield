// Adobe After Effects SDK adapter. This translation unit is compiled only
// against the locally supplied official SDK; no SDK headers are vendored here.

#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "entry.h"

#include "Diagnostics.hpp"
#include "GraphCarrier.hpp"
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "NodeGraphSync.hpp"
#include "PluginFlags.h"
#include "PluginVersion.h"
#include "SmartRender.hpp"

static_assert(STARFIELD_VERSION_STAGE == PF_Stage_DEVELOP);
static_assert(PF_VERSION(STARFIELD_VERSION_MAJOR,
                         STARFIELD_VERSION_MINOR,
                         STARFIELD_VERSION_BUG,
                         STARFIELD_VERSION_STAGE,
                         STARFIELD_VERSION_BUILD) == STARFIELD_VERSION_PACKED,
              "PiPL and runtime plug-in version fields must match");

// PluginFlags.h is the single source for the global out-flags; these assertions
// are what keeps the PiPL resource from drifting away from the runtime values.
static_assert(STARFIELD_OUT_FLAGS == (PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_PIX_INDEPENDENT |
                                      PF_OutFlag_USE_OUTPUT_EXTENT | PF_OutFlag_I_DO_DIALOG | PF_OutFlag_NON_PARAM_VARY),
              "PiPL AE_Effect_Global_OutFlags must match the runtime declaration");
static_assert(STARFIELD_OUT_FLAGS2 == (PF_OutFlag2_REVEALS_ZERO_ALPHA | PF_OutFlag2_SUPPORTS_SMART_RENDER |
                                       PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG |
                                       PF_OutFlag2_I_MIX_GUID_DEPENDENCIES | PF_OutFlag2_I_USE_3D_CAMERA),
              "PiPL AE_Effect_Global_OutFlags_2 must match the runtime declaration");

namespace {

PF_Err about(PF_InData* in_data, PF_OutData* out_data) noexcept {
    (void)in_data;
    PF_SPRINTF(out_data->return_msg,
               "Starfield Particle 0.1.0\rDeterministic CPU particles with Point, Box, Sphere, and Disc emitters. "
               "\rClick Options for a parameter/geometry readout.");
    return PF_Err_NONE;
}

PF_Err global_setup(PF_InData* in_data, PF_OutData* out_data) noexcept {
    out_data->my_version = PF_VERSION(STARFIELD_VERSION_MAJOR,
                                      STARFIELD_VERSION_MINOR,
                                      STARFIELD_VERSION_BUG,
                                      STARFIELD_VERSION_STAGE,
                                      STARFIELD_VERSION_BUILD);
    out_data->out_flags = STARFIELD_OUT_FLAGS;
    out_data->out_flags2 = STARFIELD_OUT_FLAGS2;
    // AEGP access is reserved for the supervised graph carrier. A missing suite
    // must not prevent rendering through the ordinary graph path.
    if (starfield::adapter::register_graph_carrier(in_data) != PF_Err_NONE) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                      "Starfield graph editing carrier is unavailable in this host.");
    }
    return PF_Err_NONE;
}

// Fallback for hosts that never issue the SmartFX selectors. AE itself uses
// PF_Cmd_SMART_PRE_RENDER/PF_Cmd_SMART_RENDER once SUPPORTS_SMART_RENDER is set,
// so this path is a safety net, not the M2 render path.
PF_Err render_passthrough(PF_InData* in_data, PF_ParamDef* params[], PF_LayerDef* output) noexcept {
    if (in_data == nullptr || in_data->utils == nullptr || in_data->utils->copy == nullptr ||
        params == nullptr || params[0] == nullptr || output == nullptr) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return PF_COPY(&params[0]->u.ld, output, nullptr, nullptr);
}

PF_Err dispatch(PF_Cmd cmd,
                PF_InData* in_data,
                PF_OutData* out_data,
                PF_ParamDef* params[],
                PF_LayerDef* output,
                void* extra) {
    switch (cmd) {
        case PF_Cmd_ABOUT:
            return about(in_data, out_data);
        case PF_Cmd_GLOBAL_SETUP:
            return global_setup(in_data, out_data);
        case PF_Cmd_PARAMS_SETUP:
            return starfield::adapter::setup_parameters(in_data, out_data);
        case PF_Cmd_SEQUENCE_SETUP:
            // Persistent graph state is an AE arbitrary parameter (ADR 0008).
            out_data->sequence_data = nullptr;
            return PF_Err_NONE;
        case PF_Cmd_SEQUENCE_RESETUP:
        case PF_Cmd_SEQUENCE_FLATTEN:
        case PF_Cmd_SEQUENCE_SETDOWN:
        case PF_Cmd_GLOBAL_SETDOWN:
            return PF_Err_NONE;
        case PF_Cmd_SMART_PRE_RENDER:
            return starfield::adapter::pre_render(in_data, out_data, static_cast<PF_PreRenderExtra*>(extra));
        case PF_Cmd_SMART_RENDER:
            return starfield::adapter::smart_render(in_data, out_data, static_cast<PF_SmartRenderExtra*>(extra));
        case PF_Cmd_ARBITRARY_CALLBACK:
            return starfield::adapter::graph_arbitrary_callback(in_data, static_cast<PF_ArbParamsExtra*>(extra));
        case PF_Cmd_USER_CHANGED_PARAM:
            if (const PF_Err carrier_error = starfield::adapter::commit_graph_request(
                    in_data, out_data, params, static_cast<PF_UserChangedParamExtra*>(extra));
                carrier_error != PF_Err_NONE) return carrier_error;
            return starfield::adapter::user_changed_param(in_data, out_data, params,
                                                          static_cast<PF_UserChangedParamExtra*>(extra));
        case PF_Cmd_COMPLETELY_GENERAL:
            return starfield::adapter::commit_native_graph_edit(in_data, out_data, params,
                static_cast<starfield::adapter::node_sync::NativeEdit*>(extra));
        case PF_Cmd_DO_DIALOG:
            // Options explicitly reloads the selected core and reports diagnostics.
            return starfield::adapter::report_diagnostics(in_data, out_data);
        case PF_Cmd_RENDER:
            return render_passthrough(in_data, params, output);
        default:
            return PF_Err_NONE;
    }
}

} // namespace

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,
                            PF_InData* in_data,
                            PF_OutData* out_data,
                            PF_ParamDef* params[],
                            PF_LayerDef* output,
                            void* extra) {
    try {
        return dispatch(cmd, in_data, out_data, params, output, extra);
    } catch (PF_Err error) {
        return error;
    } catch (...) {
        return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}

extern "C" DllExport PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr in_ptr,
    PF_PluginDataCB2 callback,
    SPBasicSuite* basic_suite,
    const char* host_name,
    const char* host_version) {
    (void)basic_suite;
    (void)host_name;
    (void)host_version;
    PF_Err result = PF_Err_INVALID_CALLBACK;
    PF_REGISTER_EFFECT_EXT2(in_ptr,
                            callback,
                            "Starfield Particle",
                            "org.starfieldfx.particle",
                            "Starfield FX",
                            AE_RESERVED_INFO,
                            "EffectMain",
                            "");
    return result;
}
