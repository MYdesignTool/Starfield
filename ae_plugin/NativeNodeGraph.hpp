#pragma once

#include "AE_Effect.h"
#include <memory>
#include "AE_GeneralPlug.h"
#include "starfield/core/Graph.hpp"

namespace starfield::adapter {

namespace node_sync { struct NativeEdit; }

// UI-only transaction. Restores changed dependency expressions unless accepted.
class NativeBindingTransaction {
public:
    NativeBindingTransaction(PF_InData*, AEGP_PluginID, AEGP_EffectRefH = nullptr);
    ~NativeBindingTransaction();
    NativeBindingTransaction(const NativeBindingTransaction&) = delete;
    NativeBindingTransaction& operator=(const NativeBindingTransaction&) = delete;
    PF_Err install(const core::Graph&) noexcept;
    void accept() noexcept;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// Render-safe: checks out owned numeric inputs; never calls an AEGP suite.
[[nodiscard]] PF_Err sample_native_node_animation(PF_InData*, core::Graph&,
                                                  A_long width, A_long height) noexcept;

// Reads per-node records from sibling hidden node effects on the supervised
// edit path. It must never be called from SmartFX pre-render or render.
[[nodiscard]] PF_Err compile_native_node_graph(PF_InData* in_data, PF_ParamDef* params[],
                                                core::Graph& graph, bool& found_node_effects, AEGP_PluginID plugin_id,
                                                const node_sync::NativeEdit* edit = nullptr) noexcept;

} // namespace starfield::adapter
