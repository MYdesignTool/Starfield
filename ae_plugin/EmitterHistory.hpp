#pragma once
#include "AE_Effect.h"
#include "starfield/core/Graph.hpp"
#include "starfield/core/Render.hpp"
namespace starfield::adapter {
enum class NativeHistoryPath { unavailable, static_graph, temporal, failed };
struct NativeHistoryTrace {
    NativeHistoryPath path{NativeHistoryPath::unavailable};
    double seconds{},milliseconds{};
    std::size_t certified{},inputs{},constants{},particles{};
    std::uint64_t checkouts{},rate_queries{},life_queries{},node_queries{};
    std::uint64_t node_samples{},constant_node_hits{};
};
// Process-global last preparation, not UI metadata or an instance-specific
// promise. Read/write coherent snapshots without holding locks across SDK calls.
[[nodiscard]] NativeHistoryTrace last_native_history_trace() noexcept;
[[nodiscard]] PF_Err capture_emitter_origin_history(PF_InData*, PF_OutData*, core::Graph&,
    A_long width, A_long height, const core::Cancellation&) noexcept;
}
