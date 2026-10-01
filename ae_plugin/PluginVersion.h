#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Revision 18 replaces expression snapshots with numeric revision/checksum streams.
#define STARFIELD_VERSION_BUILD 5
#define STARFIELD_VERSION_PACKED 32773 /* 0x8005 */
