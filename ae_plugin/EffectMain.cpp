// Adobe After Effects SDK adapter. This translation unit is compiled only
// against the locally supplied official SDK; no SDK headers are vendored here.

#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "entry.h"
#include "PluginVersion.h"

static_assert(STARFIELD_VERSION_STAGE == PF_Stage_DEVELOP);
static_assert(PF_VERSION(STARFIELD_VERSION_MAJOR,
                         STARFIELD_VERSION_MINOR,
                         STARFIELD_VERSION_BUG,
                         STARFIELD_VERSION_STAGE,
                         STARFIELD_VERSION_BUILD) == STARFIELD_VERSION_PACKED,
              "PiPL and runtime plug-in version fields must match");

namespace {

PF_Err about(PF_InData* in_data, PF_OutData* out_data) noexcept {
    PF_SPRINTF(out_data->return_msg,
               "Starfield Particle 0.1.0\rM1 lifecycle shell. Input is passed through.");
    return PF_Err_NONE;
}

PF_Err global_setup(PF_OutData* out_data) noexcept {
    out_data->my_version = PF_VERSION(STARFIELD_VERSION_MAJOR,
                                      STARFIELD_VERSION_MINOR,
                                      STARFIELD_VERSION_BUG,
                                      STARFIELD_VERSION_STAGE,
                                      STARFIELD_VERSION_BUILD);
    out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE;
    out_data->out_flags2 = PF_OutFlag2_NONE;
    return PF_Err_NONE;
}

PF_Err parameter_setup(PF_OutData* out_data) noexcept {
    // Parameter zero is AE's implicit input layer. Stable user controls are
    // added from schema/parameters.json in the next rendering milestone.
    out_data->num_params = 1;
    return PF_Err_NONE;
}

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
                PF_LayerDef* output) noexcept {
    switch (cmd) {
        case PF_Cmd_ABOUT:
            return about(in_data, out_data);
        case PF_Cmd_GLOBAL_SETUP:
            return global_setup(out_data);
        case PF_Cmd_PARAMS_SETUP:
            return parameter_setup(out_data);
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
        default:
            (void)in_data;
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
    (void)extra;
    try {
        return dispatch(cmd, in_data, out_data, params, output);
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
