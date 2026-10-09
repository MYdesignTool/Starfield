#pragma once

#include "AE_Effect.h"
#include <memory>
#include <optional>
#include "AE_GeneralPlug.h"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

namespace node_sync { struct NativeEdit; }

struct NativeOriginBinding { core::NodeId emitter; A_long x{}, y{}, z{}, rate{}; };
[[nodiscard]] PF_Err read_native_origin_bindings(const core::Graph&, std::vector<NativeOriginBinding>&) noexcept;
struct NativeLifetimeBinding { core::NodeId particle; A_long stream{}; };
[[nodiscard]] PF_Err read_native_lifetime_bindings(const core::Graph&,std::vector<NativeLifetimeBinding>&) noexcept;

// Decode once per frame; historical reads copy only the requested raw node.
class NativeAnimationPlan {
public:
    NativeAnimationPlan(const core::Graph&, A_long width, A_long height, double aspect);
    ~NativeAnimationPlan();
    NativeAnimationPlan(const NativeAnimationPlan&) = delete;
    NativeAnimationPlan& operator=(const NativeAnimationPlan&) = delete;
    [[nodiscard]] bool valid() const noexcept;
    void prepare_constants(PF_InData*, bool allow_static_bypass = false) noexcept;
    [[nodiscard]] const core::GraphNode* constant_node(core::NodeId) const noexcept;
    [[nodiscard]] std::optional<double> constant_birth_chance(core::NodeId) const noexcept;
    [[nodiscard]] bool fully_constant() const noexcept;
    [[nodiscard]] std::size_t input_count() const noexcept;
    [[nodiscard]] std::size_t constant_count() const noexcept;
    [[nodiscard]] std::uint64_t checkout_count() const noexcept;
    [[nodiscard]] const std::vector<struct NativeControlProof>& proofs() const noexcept;
    [[nodiscard]] PF_Err sample(PF_InData*, core::NodeId, core::GraphNode&,
                              A_long* failed_stream = nullptr) const noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// UI-only transaction. Restores changed dependency expressions unless accepted.
class NativeBindingTransaction {
public:
    // NativeEdit's local PF context has no effect_ref. Borrow its validated UI
    // owner layer; main-effect callers can resolve their real PF effect instead.
    NativeBindingTransaction(PF_InData*, AEGP_PluginID, AEGP_EffectRefH = nullptr);
    NativeBindingTransaction(PF_InData*, AEGP_PluginID, AEGP_EffectRefH,
                             AEGP_LayerH owner_layer);
    ~NativeBindingTransaction();
    NativeBindingTransaction(const NativeBindingTransaction&) = delete;
    NativeBindingTransaction& operator=(const NativeBindingTransaction&) = delete;
    PF_Err install(const core::Graph&, A_long* failed_stream = nullptr,
                   const char** failed_stage = nullptr, A_long* failed_parameter = nullptr) noexcept;
    void accept() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Render-safe: checks out owned numeric inputs; never calls an AEGP suite.
[[nodiscard]] PF_Err sample_native_node_animation(PF_InData*, core::Graph&,
                                                  A_long width, A_long height,
                                                  A_long* failed_stream = nullptr,
                                                  const char** failed_stage = nullptr,
                                                  const core::NodeId* node_filter = nullptr) noexcept;

// Reads per-node records from sibling hidden node effects on the supervised
// edit path. It must never be called from SmartFX pre-render or render.
[[nodiscard]] PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                                core::Graph& graph, bool& found_node_effects, AEGP_PluginID plugin_id,
                                                const node_sync::NativeEdit* edit = nullptr) noexcept;

} // namespace starfield::adapter
