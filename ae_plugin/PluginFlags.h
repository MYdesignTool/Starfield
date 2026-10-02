#ifndef STARFIELD_PLUGIN_FLAGS_H
#define STARFIELD_PLUGIN_FLAGS_H

/* Global out-flags shared by the PiPL resource (StarfieldPiPL.r) and the runtime
 * declaration in PF_Cmd_GLOBAL_SETUP. Both sides must agree exactly; EffectMain.cpp
 * statically asserts the decoded flag names against these values (see ADR 0004 for
 * the same pattern applied to the version).
 *
 * Decoded from the Adobe SDK:
 *   PF_OutFlag_DEEP_COLOR_AWARE       = 1 << 25  (0x02000000)
 *   PF_OutFlag_PIX_INDEPENDENT        = 1 << 10  (0x00000400)
 *   PF_OutFlag_USE_OUTPUT_EXTENT      = 1 << 6   (0x00000040)
 *   PF_OutFlag_I_DO_DIALOG            = 1 << 5   (0x00000020)  diagnostic readout
 *   PF_OutFlag_NON_PARAM_VARY         = 1 << 2   (0x00000004)  absolute-time particles
 *   PF_OutFlag2_SUPPORTS_SMART_RENDER = 1 << 10  (0x00000400)
 *   PF_OutFlag2_REVEALS_ZERO_ALPHA    = 1 << 7   (0x00000080)
 *   PF_OutFlag2_FLOAT_COLOR_AWARE     = 1 << 12  (0x00001000)
 *   PF_OutFlag2_PARAM_GROUP_START_COLLAPSED_FLAG = 1 << 3 (0x00000008)
 *   PF_OutFlag2_I_MIX_GUID_DEPENDENCIES = 1 << 21 (0x00200000)
 *
 * The group-collapsed flag is required for the topic layout: without it AE collapses
 * every parameter group, and with it each topic honours PF_ParamFlag_START_COLLAPSED,
 * so Physics and Render can start folded while Emitter and Particle stay open.
 * MFR (PF_OutFlag2_SUPPORTS_THREADED_RENDERING), GPU, and Compute Cache flags stay
 * unset until their milestones are implemented and qualified.
 */
/* Birth-position sampling checks out historical parameters. SmartFX tracks
 * those dependencies automatically; no history cache survives a frame. */
#define STARFIELD_OUT_FLAGS 0x02000466L  /* prior flags plus WIDE_TIME_INPUT */
#define STARFIELD_OUT_FLAGS2 0x0022148AL /* prior flags plus AUTOMATIC_WIDE_TIME_INPUT */

#endif /* STARFIELD_PLUGIN_FLAGS_H */
