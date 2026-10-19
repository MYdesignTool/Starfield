#pragma once

#include "EmitterHistory.hpp"
#include "AE_GeneralPlug.h"
#include "starfield/core/Geometry.hpp"
#include "starfield/core/Time.hpp"

namespace starfield::adapter {

// Capture Look At targets at one explicit shutter time. Source layer transforms
// and inventory lookup use the same PF sample time. Outputs change atomically.
[[nodiscard]] PF_Err capture_motion_look_at_sample_resources(PF_InData*,AEGP_PluginID,
    core::LayerUnits,const core::Cancellation&,const core::Graph&,core::RationalTime,
    MotionLookAtTargetResources&,core::OpaqueBytes& cache_key) noexcept;

}
