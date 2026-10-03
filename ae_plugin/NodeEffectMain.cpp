#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "entry.h"
#include "NodeEffects.hpp"
#include "PluginVersion.h"

#if defined(STARFIELD_NODE_KIND_EMITTER)
#define STARFIELD_NODE_NAME "Starfield Emitter"
#define STARFIELD_NODE_MATCH "org.starfieldfx.node.emitter"
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
#define STARFIELD_NODE_NAME "Starfield Particle Node"
#define STARFIELD_NODE_MATCH "org.starfieldfx.node.particle"
#elif defined(STARFIELD_NODE_KIND_FORCE)
#define STARFIELD_NODE_NAME "Starfield Force"
#define STARFIELD_NODE_MATCH "org.starfieldfx.node.force"
#else
#error Define exactly one STARFIELD_NODE_KIND_* for each node module.
#endif

// The effect match name plus '-' and a four-digit parameter ID must fit,
// including the terminator, in the SDK's stream match-name buffer.
static_assert(sizeof(STARFIELD_NODE_MATCH) + 1 + 4 <= AEGP_MAX_STREAM_MATCH_NAME_SIZE,
              "Node effect match name leaves no room for a valid parameter disk ID");

static_assert(STARFIELD_VERSION_STAGE == PF_Stage_DEVELOP);
static_assert(PF_VERSION(STARFIELD_VERSION_MAJOR, STARFIELD_VERSION_MINOR, STARFIELD_VERSION_BUG,
                         STARFIELD_VERSION_STAGE, STARFIELD_VERSION_BUILD) == STARFIELD_VERSION_PACKED,
              "Node PiPL and runtime effect versions must match");

extern "C" DllExport PF_Err PluginDataEntryFunction2(
    PF_PluginDataPtr in_ptr,
    PF_PluginDataCB2 callback,
    SPBasicSuite* basic_suite,
    const char* host_name,
    const char* host_version) {
    (void)basic_suite;
    (void)host_name;
    (void)host_version;
    PF_Err result = PF_Err_INVALID_CALLBACK;
    PF_REGISTER_EFFECT_EXT2(in_ptr, callback, STARFIELD_NODE_NAME, STARFIELD_NODE_MATCH,
                            "Starfield FX", AE_RESERVED_INFO, "EffectMain", "");
    return result;
}
