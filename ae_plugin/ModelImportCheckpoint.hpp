#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "starfield/core/Graph.hpp"
#include <array>
#include <cstdint>

namespace starfield::adapter {
// Current-time numeric author state and exact owned mesh bytes. No SDK handles
// or borrowed objects survive the capture or are held through the file chooser.
struct ModelImportCheckpoint {
    A_long project{},comp{},layer{},time_value{};
    std::uint32_t time_scale{},flags{};
    std::array<double,101> numbers{};
    core::OpaqueBytes mesh;
};
[[nodiscard]] PF_Err capture_model_import_checkpoint(PF_InData*,ModelImportCheckpoint&) noexcept;
[[nodiscard]] PF_Err validate_model_import_checkpoint(PF_InData*,const ModelImportCheckpoint&) noexcept;
}
