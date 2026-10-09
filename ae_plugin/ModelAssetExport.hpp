#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "ModelAssetMessage.hpp"

namespace starfield::adapter {
// Acknowledged errors stay in the private message, without raising an AE alert.
[[nodiscard]] PF_Err export_model_asset(PF_InData*,ModelAssetExportRequest&) noexcept;
}
