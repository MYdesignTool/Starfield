#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "ModelAssetMessage.hpp"
namespace starfield::adapter {
// UI-only, synchronous author staging. Caller owns undo and the full graph
// commit/rollback. No host handles or STL objects cross the private message.
PF_Err write_model_asset(PF_InData*,ModelAssetWriteRequest&) noexcept;
}
