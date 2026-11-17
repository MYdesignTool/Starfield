#pragma once
#include "AE_GeneralPlug.h"
#include "SPBasic.h"

namespace starfield::adapter {
// Session-resident General AEGP only. Web callbacks enqueue owned bytes; AE is
// accessed exclusively by step_web_panel_host on the registered idle thread.
#if defined(STARFIELD_WITH_WEBVIEW2)
void initialize_web_panel_host(SPBasicSuite*, AEGP_PluginID) noexcept;
bool step_web_panel_host() noexcept;
void stop_web_panel_host() noexcept;
A_Err dispatch_web_panel_command(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command,AEGP_HookPriority,A_Boolean,A_Boolean*) noexcept;
void set_web_panel_command_registration(A_Err) noexcept;
#else
// Existing SDK fixtures and effect DLLs need no browser dependency.
inline void initialize_web_panel_host(SPBasicSuite*, AEGP_PluginID) noexcept {}
inline bool step_web_panel_host() noexcept { return false; }
inline void stop_web_panel_host() noexcept {}
inline A_Err dispatch_web_panel_command(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command,AEGP_HookPriority,A_Boolean,A_Boolean*) noexcept {return A_Err_NONE;}
inline void set_web_panel_command_registration(A_Err) noexcept {}
#endif
}
