#pragma once
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include <string>
namespace starfield::adapter {
PF_Err edit_motion_prefix(PF_InData*,PF_OutData*) noexcept;
PF_Err motion_prefix_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
bool motion_prefix_preview(PF_InData*,PF_ParamDef*[],std::wstring&,bool& enabled) noexcept;
}
