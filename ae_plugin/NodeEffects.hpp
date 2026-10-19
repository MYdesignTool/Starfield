#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"
#include "entry.h"

#include "AE_EffectCB.h"
#include "AE_GeneralPlug.h"
#include "MotionPrefix.hpp"

extern "C" {
DllExport PF_Err EffectMain(PF_Cmd, PF_InData*, PF_OutData*, PF_ParamDef*[], PF_LayerDef*, void*);
DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr, PF_PluginDataCB2, SPBasicSuite*,
                                          const char*, const char*);
}

[[nodiscard]] PF_Err register_node_graph_sync(PF_InData* in_data) noexcept;
[[nodiscard]] AEGP_PluginID node_graph_sync_plugin_id() noexcept;
[[nodiscard]] PF_Err update_native_particle_visibility(PF_InData* in_data,PF_ParamDef* params[]) noexcept;
[[nodiscard]] PF_Err sync_node_graph_parameter(PF_InData* in_data, PF_OutData* out_data,
                                               PF_ParamDef* params[],
                                               const PF_UserChangedParamExtra* extra,
                                               bool particle_gradient=false) noexcept;

namespace starfield::adapter { class EffectUiExclusion; }
// These owned author transactions may publish inside their existing modal scope.
[[nodiscard]] PF_Err sync_model_import_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef*[],
    const PF_UserChangedParamExtra*,const starfield::adapter::EffectUiExclusion&) noexcept;
[[nodiscard]] PF_Err sync_motion_prefix_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef*[],
    const starfield::adapter::motion_prefix::Bank&,const starfield::adapter::EffectUiExclusion&) noexcept;
