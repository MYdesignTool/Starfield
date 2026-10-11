#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 70: direct Model output, Material controls and AE lighting capture;
// full nine-module/Core ABI9 pairing with CEP73. See ADR 0041 migration.
// Numeric Motion work has no public author entry in this test release.
#define STARFIELD_VERSION_BUILD 70
#define STARFIELD_VERSION_PACKED 32838 /* 0x8046 */
