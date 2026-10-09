#pragma once
#include "ModelControls.hpp"
#include <string_view>

namespace starfield::adapter {
// UI callback only. No filename or host handle is stored in the Core graph.
[[nodiscard]] PF_Err import_model_obj(PF_InData*,PF_OutData*,PF_ParamDef*[]) noexcept;
// The bounded file reader and tests share the same author transaction.
[[nodiscard]] PF_Err import_model_obj_text(PF_InData*,PF_OutData*,PF_ParamDef*[],
    std::string_view,const core::Cancellation&) noexcept;
}
