#pragma once
#include "AE_Effect.h"
#include "AE_EffectSuites.h"
#include "AE_GeneralPlug.h"
#include "starfield/core/GraphEvaluation.hpp"
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace starfield::adapter {
struct NativeControlProof {
    core::NodeId node;
    A_long stream{};
    PF_State state{};
    bool constant{};
    std::optional<core::EmissionRateProfile> rate;
    std::optional<double> life_bound;
};
// UI proof publication; no host handles are retained. Render reads use only
// the caller's own numeric aliases and ParamUtils, never AEGP streams.
void remember_native_control_proofs(PF_InData*,std::vector<NativeControlProof>) noexcept;
// Only call from the main USER_CHANGED_PARAM / DO_DIALOG / EVENT::DRAW UI paths, after its
// owned dependency expressions are installed. Never UPDATE_PARAMS_UI or render.
void capture_native_temporal_metadata(PF_InData*,const core::Graph&,AEGP_PluginID) noexcept;
// Main UI lifecycle reader. AEGP graph stream/value ownership is callback-local;
// no PF parameter checkout/checkin and no access to sequence params[].
[[nodiscard]] PF_Err capture_current_native_temporal_metadata(PF_InData*,AEGP_PluginID) noexcept;
// UI/render callback references are not persistent instance identities. Match
// node UUID + alias index and require AE to compare the entire dependency state.
[[nodiscard]] std::vector<NativeControlProof> validated_native_control_proofs(
    PF_InData*,std::span<const core::NodeId> nodes = {}) noexcept;
[[nodiscard]] std::shared_ptr<core::EmissionTimeline> native_emission_timeline(
    PF_InData*,core::NodeId,A_long stream,unsigned hz) noexcept;
}
