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
 *   PF_OutFlag2_FLOAT_COLOR_AWARE     = 1 << 12  (0x00001000)
 *
 * MFR (PF_OutFlag2_SUPPORTS_THREADED_RENDERING), GPU, and Compute Cache flags stay
 * unset until their milestones are implemented and qualified.
 */
#define STARFIELD_OUT_FLAGS 0x02000464L  /* DEEP_COLOR_AWARE | PIX_INDEPENDENT | USE_OUTPUT_EXTENT | I_DO_DIALOG | NON_PARAM_VARY */
#define STARFIELD_OUT_FLAGS2 0x00001400L /* SUPPORTS_SMART_RENDER | FLOAT_COLOR_AWARE */

#endif /* STARFIELD_PLUGIN_FLAGS_H */
