#pragma once
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include <cstdint>
namespace starfield::adapter {
// Caller owns UI exclusion. The platform menu returns only a plain numeric
// choice; its Utility suite is released before TrackPopupMenu begins.
bool choose_particle_shape(PF_InData*,std::uint32_t current,std::uint32_t& selected) noexcept;
PF_Err particle_shape_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
}
