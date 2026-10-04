#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
namespace starfield::adapter {
PF_Err particle_gradient_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
PF_Err particle_gradient_param_ui(PF_InData*,PF_ParamDef*[]) noexcept;
void clear_particle_gradient_ui() noexcept;
}
