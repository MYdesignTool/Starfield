#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 71: downstream Material geometry and stable alias bindings;
// full nine-module/Core ABI9 pairing with CEP75. See ADR 0041 migration.
// Numeric Motion work has no public author entry in this test release.
#define STARFIELD_VERSION_BUILD 71
#define STARFIELD_VERSION_PACKED 32839 /* 0x8047 */
