#pragma once
#include "AE_Effect.h"
#include "starfield/core/Graph.hpp"
#include "starfield/core/Render.hpp"
namespace starfield::adapter {
[[nodiscard]] PF_Err capture_emitter_origin_history(PF_InData*, PF_OutData*, core::Graph&,
    A_long width, A_long height, const core::Cancellation&) noexcept;
}
