#include "AEConfig.h"
#include "AE_EffectVers.h"
#include "PluginFlags.h"
#include "PluginVersion.h"

resource 'PiPL' (16000) {
{
    Kind { AEEffect },
    Name { "Starfield Particle" },
    Category { "Starfield FX" },
    CodeWin64X86 { "EffectMain" },
    AE_PiPL_Version { 2, 0 },
    AE_Effect_Spec_Version { PF_PLUG_IN_VERSION, PF_PLUG_IN_SUBVERS },
    AE_Effect_Version { STARFIELD_VERSION_PACKED },
    AE_Effect_Info_Flags { 0 },
    /* Values come from PluginFlags.h so PiPL and PF_Cmd_GLOBAL_SETUP agree. */
    AE_Effect_Global_OutFlags { STARFIELD_OUT_FLAGS },
    AE_Effect_Global_OutFlags_2 { STARFIELD_OUT_FLAGS2 },
    AE_Effect_Match_Name { "org.starfieldfx.particle" },
    AE_Reserved_Info { 0 }
}
};
