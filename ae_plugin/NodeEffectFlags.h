#ifndef STARFIELD_NODE_EFFECT_FLAGS_H
#define STARFIELD_NODE_EFFECT_FLAGS_H

/* Internal node modules keep per-node AE parameter streams, but should not be
 * offered as standalone effects in AE's Effects menu. CEP creates them by their
 * stable match names. Keep PiPL and PF_Cmd_GLOBAL_SETUP declarations identical;
 * SmartFX is required for the 32-bpc input pass-through. Threaded
 * rendering remains disabled. */
#define STARFIELD_NODE_OUT_FLAGS 0x02200400L /* I_AM_OBSOLETE | DEEP_COLOR_AWARE | PIX_INDEPENDENT */
#define STARFIELD_NODE_OUT_FLAGS2 0x02001400L /* SMART_RENDER | FLOAT_COLOR_AWARE | native GPU F32 pass-through */

#define STARFIELD_PARTICLE_OUT_FLAGS 0x02208400L /* base | CUSTOM_UI */
#define STARFIELD_TRANSFORM_OUT_FLAGS 0x02208400L /* base | Create Null custom control */
#define STARFIELD_PARTICLE_OUT_FLAGS2 0x02001408L /* base | PARAM_GROUP_START_COLLAPSED */

#endif /* STARFIELD_NODE_EFFECT_FLAGS_H */
