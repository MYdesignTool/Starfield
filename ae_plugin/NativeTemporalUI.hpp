#pragma once
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "AE_GeneralPlug.h"

namespace starfield::adapter {
// GLOBAL_SETUP runs on AE's main thread. No source handles or PF references are
// passed from worker callbacks into this UI reader.
void initialize_native_temporal_ui() noexcept;
[[nodiscard]] PF_Err register_native_temporal_ui(PF_InData*) noexcept;
void refresh_native_temporal_ui(PF_InData*,PF_ParamDef*[],const PF_EventExtra*,AEGP_PluginID) noexcept;
}
