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
    // UI-owned alias value, sampled between matching dependency states only
    // after the source has been proved constant. Never a disk-record value.
    std::optional<double> constant_value;
};
struct NativeMetadataTrace {std::size_t inputs{},evaluated{},proofs{};PF_Err error{};};
void record_native_metadata_trace(const NativeMetadataTrace&) noexcept;
[[nodiscard]] NativeMetadataTrace last_native_metadata_trace() noexcept;
// Process-local publication identity for UI scheduling only. It owns no host
// reference and expires when the bounded proof registry removes that entry.
struct NativeProofPublication {std::uint64_t id{};std::size_t count{};};
[[nodiscard]] NativeProofPublication native_control_proof_publication(PF_InData*) noexcept;
[[nodiscard]] bool native_control_proof_publication_available(std::uint64_t) noexcept;
// UI proof publication; no host handles are retained. Render reads use only
// the caller's own numeric aliases and ParamUtils, never AEGP streams.
void remember_native_control_proofs(PF_InData*,std::vector<NativeControlProof>) noexcept;
// Only after matching CHECK_ALL/all-time states bracket the complete UI capture
// following alias warm-up. Individual source/value certification is unchanged.
void remember_native_control_proofs(PF_InData*,std::vector<NativeControlProof>,const PF_State& whole_state) noexcept;
// Main UI lifecycle / USER_CHANGED_PARAM / DO_DIALOG / EVENT::DRAW only, after
// owned expressions are installed. Evaluate them before capturing dependency
// states; never PF parameter checkout in sequence callbacks. Never render or
// UPDATE_PARAMS_UI. Failed/disabled aliases cannot publish optimization proofs.
void capture_native_temporal_metadata(PF_InData*,const core::Graph&,AEGP_PluginID) noexcept;
// Main UI lifecycle reader. AEGP graph stream/value ownership is callback-local;
// no PF parameter checkout/checkin and no access to sequence params[].
[[nodiscard]] PF_Err capture_current_native_temporal_metadata(PF_InData*,AEGP_PluginID) noexcept;
// UI/render callback references are not persistent instance identities. Match
// node UUID + alias index and require AE to compare the entire dependency state.
[[nodiscard]] std::vector<NativeControlProof> validated_native_control_proofs(
    PF_InData*,std::span<const core::NodeId> nodes = {}) noexcept;
// Request-owned copy; no cache mutex survives the return or spans host callbacks.
// Publication is generation-checked on release; concurrent frames remain isolated.
[[nodiscard]] std::shared_ptr<core::EmissionTimeline> native_emission_timeline(
    PF_InData*,core::NodeId,A_long stream,unsigned hz) noexcept;
}
