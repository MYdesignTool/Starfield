#pragma once

// Parameter bridge for the stable manifest in schema/parameters.json. The AE
// adapter owns the host controls and the conversion into core settings; the core
// never sees PF_ParamDef, PF_ParamDefUnion, or host value scales.

#include "AEConfig.h"
#include "AE_Effect.h"

#include "starfield/core/Settings.hpp"
#include "GraphParameter.hpp"

#include <cstddef>

namespace starfield::adapter {

// Settings bindings and their registration-order indices. The ECW is grouped into
// Emitter / Particle / Physics / Render topics so the control layout follows the
// reference product's structure; group markers are parameters too, which is why the
// indices below are not contiguous. AE's implicit input layer occupies parameter index
// 0, so the effect registers one more parameter than kTotalEffectParameterCount.
// Manifest revision 6 renumbered everything for the grouping; revisions 7–9 append
// layout, graph carriers, and over-life curves; revision 10 appends emitter dimensions.
inline constexpr std::size_t kCurveParameterCount = 34; // two counts and 32 age/value sliders
inline constexpr std::size_t kEmitterSizeParameterCount = 3;
inline constexpr std::size_t kEffectParameterCount = 21 + kCurveParameterCount + kEmitterSizeParameterCount;
inline constexpr std::size_t kTotalEffectParameterCount = 84; // controls + topics + project metadata

inline constexpr A_long kTypeId = 2;
inline constexpr A_long kParticlesPerSecondId = 3;
inline constexpr A_long kOriginId = 4;
inline constexpr A_long kEmitterSizeId = 5;
inline constexpr A_long kSpeedXId = 6;
inline constexpr A_long kSpeedYId = 7;
inline constexpr A_long kSpeedZId = 8;
inline constexpr A_long kSpeedRandomId = 9;
inline constexpr A_long kLifetimeId = 12;
inline constexpr A_long kSizeId = 13;
inline constexpr A_long kParticleSizeEndId = 14;
inline constexpr A_long kOpacityId = 15;
inline constexpr A_long kOpacityEndId = 16;
inline constexpr A_long kColorStartId = 17;
inline constexpr A_long kColorEndId = 18;
inline constexpr A_long kGravityXId = 21;
inline constexpr A_long kGravityYId = 22;
inline constexpr A_long kGravityZId = 23;
inline constexpr A_long kLinearDragId = 24;
inline constexpr A_long kMaxParticlesId = 27;
inline constexpr A_long kSeedId = 28;
// 29/30/31 are the control source, capture action and hidden graph parameter. Index 32
// closes the Render topic; 33..40 are hidden project-saved layout coordinates and
// 41..44 are the expression snapshot, edit request, commit trigger and receipt.
inline constexpr A_long kFirstEffectParameterId = 1;
inline constexpr A_long kLastEffectParameterId = 30; // capture action (29 is control source)
inline constexpr A_long kLayoutEmitterXId = 33;
inline constexpr A_long kLayoutEmitterYId = 34;
inline constexpr A_long kLayoutForceXId = 35;
inline constexpr A_long kLayoutForceYId = 36;
inline constexpr A_long kLayoutAppearanceXId = 37;
inline constexpr A_long kLayoutAppearanceYId = 38;
inline constexpr A_long kLayoutOutputXId = 39;
inline constexpr A_long kLayoutOutputYId = 40;
inline constexpr A_long kGraphSnapshotId = 41;
inline constexpr A_long kGraphEditRequestId = 42;
inline constexpr A_long kGraphEditCommitId = 43;
inline constexpr A_long kGraphEditReceiptId = 44;
inline constexpr A_long kSizeCurveCountId = 45;
inline constexpr A_long kSizeCurveFirstPointId = 46; // alternating age/value sliders
inline constexpr A_long kOpacityCurveCountId = 62;
inline constexpr A_long kOpacityCurveFirstPointId = 63; // alternating age/value sliders
inline constexpr A_long kCurveEditCommitId = 79;
inline constexpr A_long kEmitterSizeXId = 81;
inline constexpr A_long kEmitterSizeYId = 82;
inline constexpr A_long kEmitterSizeZId = 83;

// Pre-render records dependencies by checking out the selected parameter source.
// The returned immutable graph owns no AE handles or parameter pointers.
// `reference_width`/`reference_height` are the full-resolution layer size observed in the
// pre-render input checkout (PF_CheckoutResult::ref_width/ref_height). AE 2023.5 Build 52
// delivers point-control components scaled with preview resolution (verified Full vs
// Quarter); point_control_to_full_resolution_pixels() reverses that scale before converting to world.
// Zero means "no render context, use in_data".
[[nodiscard]] PF_Err checkout_render_graph(PF_InData* in_data, PF_OutData* out_data,
                                          std::shared_ptr<const core::Graph>& graph,
                                          A_long* control_source = nullptr,
                                          A_long reference_width = 0, A_long reference_height = 0) noexcept;
[[nodiscard]] PF_Err capture_controls(PF_InData* in_data, PF_OutData* out_data,
                                      PF_ParamDef* params[], PF_UserChangedParamExtra* extra) noexcept;

// Converts a raw PF_Point3DDef value into full-resolution layer pixels. AE 2023.5 Build 52
// scales point values by the preview factor; X and Y/Z use their corresponding horizontal
// and vertical rational factors. Invalid/unreported factors are treated as 1:1.
[[nodiscard]] starfield::core::Vec3 point_control_to_full_resolution_pixels(
    const starfield::core::Vec3& raw, const PF_InData& in_data) noexcept;

// PF_Cmd_USER_CHANGED_PARAM entry point. The capture button replaces the stored
// graph; a change to any bound control while Node Graph is selected rewrites the
// canonical graph from the delivered control values in the same undo step. This is
// the supervised edit surface the dockable panel drives (ADR 0009). AE Controls
// mode never touches the stored graph.
[[nodiscard]] PF_Err user_changed_param(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                        PF_UserChangedParamExtra* extra) noexcept;

// Registers every manifest row with stable IDs, labels, ranges, and defaults, and
// reports the resulting parameter count. Returns the host error unchanged.
[[nodiscard]] PF_Err setup_parameters(PF_InData* in_data, PF_OutData* out_data) noexcept;

// Diagnostic only: true when STARFIELD_FLAT_RENDER=1 (non-zero) is set in the host
// environment, which makes the render sample the flat AE controls instead of the stored
// graph. Used to bisect a host-side surprise between the graph and flat render paths;
// it never persists and the Options readout reports it.
[[nodiscard]] bool flat_render_override_active() noexcept;

// Diagnostic only: true when STARFIELD_NO_GRAPH_PARAM=1. Registration then swaps the
// arbitrary-data parameter for a hidden float slider with the same index and count, and
// rendering stays on the flat path, so the host can be tested without arbitrary data.
[[nodiscard]] bool graph_parameter_disabled() noexcept;

// Host-scoped parameter snapshot. Values are checked out with PF_CHECKOUT_PARAM,
// converted into core units, and never outlive checkin(). The snapshot is
// copy-disabled so a checkout cannot be duplicated without a matching checkin.
class ParameterSnapshot {
public:
    ParameterSnapshot() = default;
    ParameterSnapshot(const ParameterSnapshot&) = delete;
    ParameterSnapshot& operator=(const ParameterSnapshot&) = delete;
    ParameterSnapshot(ParameterSnapshot&&) = delete;
    ParameterSnapshot& operator=(ParameterSnapshot&&) = delete;
    ~ParameterSnapshot() = default;

    // Returns the first host error and checks in whatever was already checked out.
    // reference_width/height are the full-resolution layer size when a render context
    // provides one (see checkout_render_graph); zero falls back to in_data.
    [[nodiscard]] PF_Err checkout(PF_InData* in_data, A_long reference_width = 0,
                                  A_long reference_height = 0) noexcept;
    void checkin(PF_InData* in_data) noexcept;

    [[nodiscard]] bool valid() const noexcept { return valid_; }

    // Values in core units. Only meaningful while valid() is true.
    [[nodiscard]] const starfield::core::Settings& settings() const noexcept { return settings_; }

    // Emitter origin exactly as the host delivered it, before preview-scale normalization.
    // The diagnostics readout uses it to show the raw control value.
    [[nodiscard]] const starfield::core::Vec3& raw_origin() const noexcept { return raw_origin_; }

private:
    PF_ParamDef defs_[kEffectParameterCount]{};
    bool checked_out_[kEffectParameterCount]{};
    bool valid_{false};
    starfield::core::Settings settings_{};
    starfield::core::Vec3 raw_origin_{};
};

// Checks parameters in on every exit path, including early returns. The render
// path and the diagnostics readout share it so neither can leak a checkout.
class ScopedParameterCheckin {
public:
    explicit ScopedParameterCheckin(PF_InData* in_data) noexcept : in_data_(in_data) {}
    ScopedParameterCheckin(const ScopedParameterCheckin&) = delete;
    ScopedParameterCheckin& operator=(const ScopedParameterCheckin&) = delete;
    ~ScopedParameterCheckin() { snapshot_.checkin(in_data_); }

    [[nodiscard]] ParameterSnapshot& snapshot() noexcept { return snapshot_; }

private:
    PF_InData* in_data_{nullptr};
    ParameterSnapshot snapshot_;
};

} // namespace starfield::adapter
