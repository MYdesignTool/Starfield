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

// User-visible controls defined by the manifest (IDs 1..13). AE's implicit input
// layer occupies parameter index 0, so the effect registers one more parameter.
// New controls are appended with new IDs; ID order is preserved even when the UI
// grouping is still pending (M3-03 adds AE parameter groups).
inline constexpr std::size_t kEffectParameterCount = 13;
inline constexpr std::size_t kTotalEffectParameterCount = 16;

// Pre-render records dependencies by checking out the selected parameter source.
// The returned immutable graph owns no AE handles or parameter pointers.
[[nodiscard]] PF_Err checkout_render_graph(PF_InData* in_data, PF_OutData* out_data,
                                          std::shared_ptr<const core::Graph>& graph,
                                          A_long* control_source = nullptr) noexcept;
[[nodiscard]] PF_Err capture_controls(PF_InData* in_data, PF_OutData* out_data,
                                      PF_ParamDef* params[], PF_UserChangedParamExtra* extra) noexcept;

// Registers every manifest row with stable IDs, labels, ranges, and defaults, and
// reports the resulting parameter count. Returns the host error unchanged.
[[nodiscard]] PF_Err setup_parameters(PF_InData* in_data, PF_OutData* out_data) noexcept;

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
    [[nodiscard]] PF_Err checkout(PF_InData* in_data) noexcept;
    void checkin(PF_InData* in_data) noexcept;

    [[nodiscard]] bool valid() const noexcept { return valid_; }

    // Values in core units. Only meaningful while valid() is true.
    [[nodiscard]] const starfield::core::Settings& settings() const noexcept { return settings_; }

    // Emitter origin exactly as the host delivered it, before the percent-to-world
    // conversion. The diagnostics readout uses it to show what the UI stored.
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
