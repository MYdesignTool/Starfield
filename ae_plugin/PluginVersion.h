#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build75 candidate: finite alias handoff and post-publication retirement.
// Full ten-module/Core ABI9 pairing with CEP79; ADR0040.
#define STARFIELD_VERSION_BUILD 75
#define STARFIELD_VERSION_PACKED 32843 /* 0x804B */
