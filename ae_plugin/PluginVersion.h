#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 51: persistent relative Null attachment and stable CEP graph targets.
#define STARFIELD_VERSION_BUILD 51
#define STARFIELD_VERSION_PACKED 32819 /* 0x8033 */
