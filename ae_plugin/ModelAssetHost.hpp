#pragma once
#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"

namespace starfield::adapter {
// Session-resident, UI-idle-only transport. A command queues work; it never
// executes a script or calls an effect while CEP's evalScript is on the stack.
inline constexpr char model_asset_command_name[]="Starfield Prepare Model Asset Export";
void initialize_model_asset_host(SPBasicSuite*,AEGP_PluginID) noexcept;
void queue_model_asset_export() noexcept;
// true asks the existing idle hook for another short wake. No normal polling.
bool step_model_asset_host() noexcept;
void stop_model_asset_host() noexcept;
}
