#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 63 test candidate: read diagnostic strings and claim Model requests
// only after their identity arrives; full native/Core ABI8 pairing with CEP65.
// Numeric Motion work has no public author entry in this test release.
#define STARFIELD_VERSION_BUILD 63
#define STARFIELD_VERSION_PACKED 32831 /* 0x803f */
