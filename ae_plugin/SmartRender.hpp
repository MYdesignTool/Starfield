#pragma once

// SmartFX transport for the particle renderer. Pre-render declares the pixels this
// effect will produce and checks out exactly one input; render re-fetches those
// pixels, converts them into owned core buffers, runs the host-independent core,
// and copies the staging buffer into the host output world.

#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter {

[[nodiscard]] PF_Err pre_render(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) noexcept;
[[nodiscard]] PF_Err smart_render(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra) noexcept;

} // namespace starfield::adapter
