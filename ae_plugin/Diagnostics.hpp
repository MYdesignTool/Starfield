#pragma once

// Host-state readout used to find out what the effect actually receives. It is
// invoked from the effect's Options button and reports the values the code read
// and the geometry it computed, so a mismatch between the parameter UI and the
// render path can be identified without a debugger.
//
// Options also refreshes our generated native dependency bindings on existing
// development effects. Source keyframes, source expressions and graph values
// are preserved; Core reload and proof refresh can request a new render.
// The contents are a support/diagnostic aid, not part of the rendered contract.

#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter {

// Geometry of the most recently rendered frame, recorded by the render path and printed
// by the readout. The readout cannot call checkout_layer (that callback only exists in
// the pre-render phase), so without this the full-resolution reference the render used
// would be invisible exactly when it differs from the preview size, which is the case
// that decides how a point control's units have to be interpreted. Diagnostic only: the
// render path never reads it back.
struct RenderGeometry {
    A_long layer_width{0};
    A_long layer_height{0};
    A_long ref_width{0};
    A_long ref_height{0};
    A_long grid_width{0};
    A_long grid_height{0};
    A_long par_num{0};
    A_long par_den{0};
    bool valid{false};
};

// Called by the render path once the frame geometry is known. Safe to call from any
// thread; later calls overwrite earlier ones.
void record_render_geometry(A_long layer_width, A_long layer_height, A_long ref_width, A_long ref_height,
                            A_long grid_width, A_long grid_height, A_long par_num, A_long par_den) noexcept;
[[nodiscard]] RenderGeometry last_render_geometry() noexcept;

// Writes a compact summary into out_data->return_msg and asks AE to display it.
[[nodiscard]] PF_Err report_diagnostics(PF_InData* in_data, PF_OutData* out_data) noexcept;

} // namespace starfield::adapter
