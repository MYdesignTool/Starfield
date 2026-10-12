#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build74 candidate: guarded node retirement before physical deletion.
// Full ten-module/Core ABI9 pairing with CEP78; ADR0040.
#define STARFIELD_VERSION_BUILD 74
#define STARFIELD_VERSION_PACKED 32842 /* 0x804A */
