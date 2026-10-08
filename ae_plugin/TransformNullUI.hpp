#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
namespace starfield::adapter {
PF_Err create_transform_null(PF_InData*,PF_OutData*,PF_ParamDef*[]) noexcept;
PF_Err transform_null_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
}
