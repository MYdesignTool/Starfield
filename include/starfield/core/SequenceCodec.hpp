#pragma once

#include "starfield/core/Graph.hpp"

#include <cstddef>
#include <span>
#include <utility>
#include <variant>

namespace starfield::core {

inline constexpr std::uint16_t kSequenceFormatVersion = 1;
inline constexpr std::uint16_t kSequenceHeaderSize = 32;

enum class SequenceErrorCode : std::uint8_t {
    none,
    invalid_header,
    unsupported_format_version,
    unsupported_flags,
    length_mismatch,
    size_limit_exceeded,
    checksum_mismatch,
    malformed_record,
    unsupported_record,
    unsupported_record_version,
    unsupported_value_type,
    invalid_value,
    invalid_graph,
    allocation_failed,
    internal_failure,
};

struct SequenceError {
    SequenceErrorCode code{SequenceErrorCode::none};
    const char* detail{"no sequence codec error"};
    GraphError graph_error{};
};

template <typename T>
class SequenceResult {
public:
    static SequenceResult success(T value) {
        return SequenceResult(Storage{std::in_place_type<T>, std::move(value)});
    }
    static SequenceResult failure(SequenceError error) {
        return SequenceResult(Storage{std::in_place_type<SequenceError>, error});
    }

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] T&& take_value() { return std::move(std::get<T>(storage_)); }
    [[nodiscard]] const SequenceError& error() const { return std::get<SequenceError>(storage_); }

private:
    using Storage = std::variant<T, SequenceError>;
    explicit SequenceResult(Storage storage) : storage_(std::move(storage)) {}
    Storage storage_;
};

[[nodiscard]] const char* describe(SequenceErrorCode code) noexcept;

// Serialization validates first and emits canonical byte order: nodes by NodeId,
// edges by EdgeId, and parameters by ParameterKey. Deserialization checks all
// lengths/counts before allocation, verifies CRC-32, then validates the graph
// against the supplied immutable node registry.
[[nodiscard]] SequenceResult<OpaqueBytes> serialize_graph(const Graph& graph, const NodeRegistry& registry) noexcept;
[[nodiscard]] SequenceResult<Graph> deserialize_graph(std::span<const std::byte> bytes,
                                                      const NodeRegistry& registry) noexcept;

} // namespace starfield::core
