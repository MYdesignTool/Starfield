#ifndef STARFIELD_NODE_EFFECT_FLAGS_H
#define STARFIELD_NODE_EFFECT_FLAGS_H

/* Internal node modules keep per-node AE parameter streams, but should not be
 * offered as standalone effects in AE's Effects menu. CEP creates them by their
 * stable match names. Keep PiPL and PF_Cmd_GLOBAL_SETUP declarations identical;
 * SmartFX is required for the 32-bpc input pass-through. Threaded
 * rendering remains disabled. */
#define STARFIELD_NODE_OUT_FLAGS 0x02200400L /* I_AM_OBSOLETE | DEEP_COLOR_AWARE | PIX_INDEPENDENT */
#define STARFIELD_NODE_OUT_FLAGS2 0x00001400L /* SUPPORTS_SMART_RENDER | FLOAT_COLOR_AWARE */

#endif /* STARFIELD_NODE_EFFECT_FLAGS_H */
