#pragma once
#include "EffectGraphBackup.hpp"
#include "ModelAssetMessage.hpp"
#include <array>
#include <cstdint>
#include <span>

namespace starfield::adapter {
using ModelTransactionId=std::array<std::uint8_t,16>;
struct ModelAssetRestore {
    ModelTransactionId node_id{};
    std::uint32_t source{1},revision{};
    std::span<const std::uint8_t> mesh;
    std::array<double,6> bounds{-.5,-.5,-.5,.5,.5,.5};
};
enum class ModelTransactionStage : std::uint32_t { preflight,undo,backup,prepare,assets,commit,restore,complete };
struct ModelGraphTransactionPlan {
    SPBasicSuite* basic{};AEGP_PluginID plugin{};AEGP_LayerH layer{};A_Time time{0,1};
    ModelTransactionId id{};
    std::span<const ModelTransactionId> desired_ids;
    std::span<const ModelAssetRestore> assets;
    void* context{};
    // UI route validates the pinned target before each callback. Success from
    // commit means its native graph receipt and resulting snapshot were checked.
    A_Err (*prepare)(void*){};
    A_Err (*commit)(void*){};
    std::int32_t (*is_cancelled)(void*) noexcept {};
};
struct ModelGraphTransactionResult {
    bool committed{};
    ModelTransactionStage stage{ModelTransactionStage::preflight};
    A_Err error{},rollback_error{},asset_rollback_error{},cleanup_error{},undo_error{};
    ModelAssetError asset_error{ModelAssetError::none};
    std::int32_t asset_index{-1};
};
// Admitted UI callback only, never a render or CEP synchronous command stack.
// All plan spans/context remain alive and immutable until this function returns.
[[nodiscard]] ModelGraphTransactionResult apply_model_graph_transaction(const ModelGraphTransactionPlan&) noexcept;
}
