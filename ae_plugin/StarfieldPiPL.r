#include "AEConfig.h"
#include "AE_EffectVers.h"
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
    AE_Effect_Global_OutFlags { 0x02000000 }, /* PF_OutFlag_DEEP_COLOR_AWARE */
    AE_Effect_Global_OutFlags_2 { PF_OutFlag2_NONE },
    AE_Effect_Match_Name { "org.starfieldfx.particle" },
    AE_Reserved_Info { 0 }
}
};
