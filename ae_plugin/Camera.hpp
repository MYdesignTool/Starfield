#pragma once
#include "AE_Effect.h"
#include "starfield/core/PluginApi.h"
namespace starfield::adapter {
[[nodiscard]] PF_Err capture_camera(PF_InData*, SfCoreRenderRequest&) noexcept;
}
