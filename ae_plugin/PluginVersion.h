#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build73 candidate: ordered Force after Motion and percentage units.
// Full ten-module/Core ABI9 pairing with CEP77; ADR0039.
#define STARFIELD_VERSION_BUILD 73
#define STARFIELD_VERSION_PACKED 32841 /* 0x8049 */
