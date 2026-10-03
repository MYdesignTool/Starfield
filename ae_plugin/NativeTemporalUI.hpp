#pragma once
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "AE_GeneralPlug.h"
#include <cstdint>

namespace starfield::adapter {
struct NativeUITiming {std::uint64_t calls{},refreshes{},sequence_refreshes{};double last_ms{},max_ms{};};
[[nodiscard]] NativeUITiming last_native_ui_timing() noexcept;
// GLOBAL_SETUP runs on AE's main thread. No source handles or PF references are
// passed from worker callbacks into this UI reader.
void initialize_native_temporal_ui() noexcept;
[[nodiscard]] PF_Err register_native_temporal_ui(PF_InData*) noexcept;
void refresh_native_temporal_ui(PF_InData*,PF_ParamDef*[],const PF_EventExtra*,AEGP_PluginID) noexcept;
// Flat schema-1 lifecycle marker contains no refs/tokens/graph values. Legacy
// null sequence data is accepted. UI setup/resetup recaptures optional metadata;
// render-only/worker resetup performs no AEGP reads.
[[nodiscard]] PF_Err setup_native_temporal_sequence(PF_InData*,PF_OutData*,PF_ParamDef*[],AEGP_PluginID,bool resetup) noexcept;
[[nodiscard]] PF_Err flatten_native_temporal_sequence(PF_InData*,PF_OutData*) noexcept;
void setdown_native_temporal_sequence(PF_InData*) noexcept;
}
