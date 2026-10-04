#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
namespace starfield::adapter {
PF_Err main_presets_event(PF_InData*,PF_OutData*,PF_EventExtra*) noexcept;
void clear_main_presets_ui() noexcept;
}
