#pragma once

// Host-state readout used to find out what the effect actually receives. It is
// invoked from the effect's Options button and reports the values the code read
// and the geometry it computed, so a mismatch between the parameter UI and the
// render path can be identified without a debugger.
//
// It is read-only: it never changes output pixels, sequence data, or settings.
// The contents are a support/diagnostic aid, not part of the rendered contract.

#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter {

// Writes a compact summary into out_data->return_msg and asks AE to display it.
[[nodiscard]] PF_Err report_diagnostics(PF_InData* in_data, PF_OutData* out_data) noexcept;

} // namespace starfield::adapter
