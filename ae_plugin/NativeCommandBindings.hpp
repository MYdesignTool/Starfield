#pragma once
#include "AE_GeneralPlug.h"
#include <array>
#include <string>

namespace starfield::adapter {
// Session IDs allocated by AE, never persisted or inferred from menu positions.
// The resident AEGP initializes this before its first idle callback. Only native
// code writes it; each panel script receives a fresh copy in the AE engine.
struct NativeCommandBinding { const char* name; AEGP_Command command; };
inline std::array<NativeCommandBinding,6> native_command_bindings{};
inline AEGP_Command register_hidden_native_command(const AEGP_CommandSuite1* commands,
    const AEGP_RegisterSuite5* registrations,AEGP_PluginID plugin,const char* name,AEGP_CommandHook hook,bool register_hook=true) {
    AEGP_Command command{};
    if(commands && registrations && !commands->AEGP_GetUniqueCommand(&command) && command>0 &&
       (!register_hook || !registrations->AEGP_RegisterCommandHook(plugin,AEGP_HP_BeforeAE,command,hook,nullptr)) &&
       !commands->AEGP_InsertMenuCommand(command,name,AEGP_Menu_NONE,AEGP_MENU_INSERT_SORTED) &&
       !commands->AEGP_EnableCommand(command))return command;
    return 0;
}
inline std::string native_command_script() {
    std::string script="$.global.__SFLD_nativeCommandsV1={version:1,commands:{";
    bool comma=false;
    for(const auto& binding:native_command_bindings) {
        if(!binding.name)continue;
        if(comma)script+=',';
        comma=true;
        // All names are fixed ASCII literals owned by StarfieldHost.
        script+='"';script+=binding.name;script+="\":";
        script+=std::to_string(binding.command>0?binding.command:0);
    }
    return script+"}};\n";
}
}
