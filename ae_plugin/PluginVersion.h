#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 28: UI project-load metadata initialization with a flat lifecycle marker.
#define STARFIELD_VERSION_BUILD 28
#define STARFIELD_VERSION_PACKED 32796 /* 0x801C */
