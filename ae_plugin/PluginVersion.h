#pragma once

// Keep the PiPL's packed value in sync with PF_VERSION below. The packed
// integer is needed by PiPLTool; the compile-time assertion catches drift.
#define STARFIELD_VERSION_MAJOR 0
#define STARFIELD_VERSION_MINOR 1
#define STARFIELD_VERSION_BUG 0
#define STARFIELD_VERSION_STAGE 0 /* PF_Stage_DEVELOP */
// Build 61 test candidate: Model geometry/author/asset transport and modal guards;
// graph7, binding7, snapshot8, manifest30 and fully paired Core ABI8 / CEP62.
#define STARFIELD_VERSION_BUILD 61
#define STARFIELD_VERSION_PACKED 32829 /* 0x803d */
