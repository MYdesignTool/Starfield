#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 6 implements SmartFX pass-through for the internal 32-bpc node effects.
#define STARFIELD_VERSION_BUILD 6
#define STARFIELD_VERSION_PACKED 32774 /* 0x8006 */
