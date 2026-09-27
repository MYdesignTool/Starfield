// Adobe After Effects SDK adapter. This translation unit is compiled only
// against the locally supplied official SDK; no SDK headers are vendored here.

#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "entry.h"

#include "Diagnostics.hpp"
#include "Parameters.hpp"
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
                                      PF_OutFlag_USE_OUTPUT_EXTENT | PF_OutFlag_I_DO_DIALOG),
              "PiPL AE_Effect_Global_OutFlags must match the runtime declaration");
static_assert(STARFIELD_OUT_FLAGS2 == (PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE),
              "PiPL AE_Effect_Global_OutFlags_2 must match the runtime declaration");

namespace {

PF_Err about(PF_InData* in_data, PF_OutData* out_data) noexcept {
    (void)in_data;
    PF_SPRINTF(out_data->return_msg,
               "Starfield Particle 0.1.0\rM2 vertical slice: deterministic CPU point emitter "
               "composited over the input.\rClick Options for a parameter/geometry readout.");
    return PF_Err_NONE;
}

PF_Err global_setup(PF_OutData* out_data) noexcept {
    out_data->my_version = PF_VERSION(STARFIELD_VERSION_MAJOR,
                                      STARFIELD_VERSION_MINOR,
                                      STARFIELD_VERSION_BUG,
                                      STARFIELD_VERSION_STAGE,
                                      STARFIELD_VERSION_BUILD);
    out_data->out_flags = STARFIELD_OUT_FLAGS;
    out_data->out_flags2 = STARFIELD_OUT_FLAGS2;
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
                void* extra) noexcept {
    switch (cmd) {
        case PF_Cmd_ABOUT:
            return about(in_data, out_data);
        case PF_Cmd_GLOBAL_SETUP:
            return global_setup(out_data);
        case PF_Cmd_PARAMS_SETUP:
            return starfield::adapter::setup_parameters(in_data, out_data);
        case PF_Cmd_SEQUENCE_SETUP:
            // M2 owns no sequence state. Bounded graph parsing arrives in M4.
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
        case PF_Cmd_DO_DIALOG:
            // Diagnostic readout behind the effect's Options button. Read-only.
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
