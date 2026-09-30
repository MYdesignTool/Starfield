#include "AEConfig.h"
#include "AE_EffectVers.h"
#include "NodeEffectFlags.h"
#include "PluginVersion.h"

resource 'PiPL' (16013) {
{
    Kind { AEEffect },
    Name { "Starfield Appearance" },
    Category { "Starfield FX" },
    CodeWin64X86 { "EffectMain" },
    AE_PiPL_Version { 2, 0 },
    AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
    AE_Effect_Version { STARFIELD_VERSION_PACKED },
    AE_Effect_Info_Flags { 0 },
    AE_Effect_Global_OutFlags { STARFIELD_NODE_OUT_FLAGS },
    AE_Effect_Global_OutFlags_2 { STARFIELD_NODE_OUT_FLAGS2 },
    AE_Effect_Match_Name { "org.starfieldfx.node.appearance" },
    AE_Reserved_Info { 0 }
}
};
