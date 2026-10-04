#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"
#include "entry.h"

#include "AE_EffectCB.h"

extern "C" {
DllExport PF_Err EffectMain(PF_Cmd, PF_InData*, PF_OutData*, PF_ParamDef*[], PF_LayerDef*, void*);
DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr, PF_PluginDataCB2, SPBasicSuite*,
                                          const char*, const char*);
}

[[nodiscard]] PF_Err register_node_graph_sync(PF_InData* in_data) noexcept;
[[nodiscard]] PF_Err update_native_particle_visibility(PF_InData* in_data,PF_ParamDef* params[]) noexcept;
[[nodiscard]] PF_Err sync_node_graph_parameter(PF_InData* in_data, PF_OutData* out_data,
                                               PF_ParamDef* params[],
                                               const PF_UserChangedParamExtra* extra,
                                               bool particle_gradient=false) noexcept;
