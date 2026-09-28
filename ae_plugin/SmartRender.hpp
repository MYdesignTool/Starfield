#pragma once

// SmartFX transport for the particle renderer. Pre-render checks empty input metadata
// for bounds/reference geometry; render satisfies AE's input/output checkout ordering
// but does not read input pixels, then copies transparent particle output to AE's world.

#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter {

[[nodiscard]] PF_Err pre_render(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) noexcept;
[[nodiscard]] PF_Err smart_render(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra) noexcept;

} // namespace starfield::adapter
