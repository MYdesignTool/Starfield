#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"
#include "entry.h"

extern "C" {
DllExport PF_Err EffectMain(PF_Cmd, PF_InData*, PF_OutData*, PF_ParamDef*[], PF_LayerDef*, void*);
DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr, PF_PluginDataCB2, SPBasicSuite*,
                                          const char*, const char*);
}
