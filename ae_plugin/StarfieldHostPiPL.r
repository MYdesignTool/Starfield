#include "AEConfig.h"
#include "PluginVersion.h"
resource 'PiPL' (16000) {
    {
        Kind { AEGP },
        Name { "Starfield Host" },
        Category { "General Plugin" },
        Version { STARFIELD_VERSION_PACKED },
#ifdef AE_OS_WIN
        CodeWin64X86 { "StarfieldHostEntry" },
#endif
    }
};
