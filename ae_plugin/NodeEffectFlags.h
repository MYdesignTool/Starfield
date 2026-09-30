#ifndef STARFIELD_NODE_EFFECT_FLAGS_H
#define STARFIELD_NODE_EFFECT_FLAGS_H

/* Node effects are image pass-throughs. Keep their PiPL declarations identical
 * to PF_Cmd_GLOBAL_SETUP and do not advertise SmartFX or threaded rendering. */
#define STARFIELD_NODE_OUT_FLAGS 0x02000400L /* DEEP_COLOR_AWARE | PIX_INDEPENDENT */
#define STARFIELD_NODE_OUT_FLAGS2 0x00001000L /* FLOAT_COLOR_AWARE */

#endif /* STARFIELD_NODE_EFFECT_FLAGS_H */
