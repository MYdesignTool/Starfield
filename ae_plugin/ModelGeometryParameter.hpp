#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "starfield/core/ModelResources.hpp"

namespace starfield::adapter {
[[nodiscard]] core::Result<core::ModelGeometry> read_model_geometry_parameter(
    PF_InData*,PF_ArbitraryH,const core::Cancellation&) noexcept;
[[nodiscard]] PF_Err create_model_geometry_parameter(PF_InData*,const core::ModelGeometry&,
    PF_ArbitraryH*,const core::Cancellation&) noexcept;
// Registration/ID allocation belongs to the Model author, not this helper.
[[nodiscard]] PF_Err model_geometry_arbitrary_callback(PF_InData*,PF_ArbParamsExtra*,
    A_long expected_disk_id) noexcept;
}
