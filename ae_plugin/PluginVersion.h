#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build78 candidate: graph/receipt/expression generation coherence.
// Full ten-module/Core ABI9 pairing with the accepted CEP81; ADR0040.
#define STARFIELD_VERSION_BUILD 78
#define STARFIELD_VERSION_PACKED 32846 /* 0x804E */
