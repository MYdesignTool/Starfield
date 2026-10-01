#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"

namespace starfield::adapter::native_nodes {

// Node-module project record v1. Keep this layout shared by the node AEX and
// renderer-side graph compiler. This native record currently supports four
// outgoing edges per node. It is a stricter authoring limit than the portable
// graph's total edge bound; a wider fan-out requires extending this record.
inline constexpr A_long kMaxOutgoingEdges = 4;
inline constexpr A_long kConnectionUuidChunks = 8;
inline constexpr A_long kConnectionRecordChunks = 16; // destination UUID + edge UUID

enum class Kind : A_long { emitter, particle, appearance, force };

[[nodiscard]] constexpr A_long base_parameter_count(Kind kind) noexcept {
    switch (kind) {
        case Kind::emitter: return 23;
        case Kind::particle: return 45;
        case Kind::appearance: return 44;
        case Kind::force: return 4;
    }
    return 0;
}

[[nodiscard]] constexpr A_long layout_x_index(Kind kind) noexcept { return base_parameter_count(kind) + 1; }
[[nodiscard]] constexpr A_long layout_y_index(Kind kind) noexcept { return base_parameter_count(kind) + 2; }
[[nodiscard]] constexpr A_long connection_count_index(Kind kind) noexcept { return base_parameter_count(kind) + 3; }
[[nodiscard]] constexpr A_long connection_first_index(Kind kind) noexcept { return base_parameter_count(kind) + 4; }
[[nodiscard]] constexpr A_long uuid_first_index(Kind kind) noexcept {
    return connection_first_index(kind) + kMaxOutgoingEdges * kConnectionRecordChunks;
}
[[nodiscard]] constexpr A_long sync_guard_index(Kind kind) noexcept { return uuid_first_index(kind) + 8; }
[[nodiscard]] constexpr A_long last_parameter_index(Kind kind) noexcept { return sync_guard_index(kind); }
// PF_OutData::num_params includes parameter 0 (the input layer), while registered
// node controls occupy indices 1..last_parameter_index.
[[nodiscard]] constexpr A_long parameter_count(Kind kind) noexcept { return last_parameter_index(kind) + 1; }

[[nodiscard]] constexpr A_long connection_uuid_index(Kind kind, A_long slot, A_long chunk) noexcept {
    return connection_first_index(kind) + slot * kConnectionRecordChunks + chunk;
}
[[nodiscard]] constexpr A_long connection_edge_uuid_index(Kind kind, A_long slot, A_long chunk) noexcept {
    return connection_uuid_index(kind, slot, kConnectionUuidChunks + chunk);
}

[[nodiscard]] constexpr A_long fourcc(char a, char b, char c, char d) noexcept {
    return (static_cast<A_long>(static_cast<unsigned char>(a)) << 24) |
           (static_cast<A_long>(static_cast<unsigned char>(b)) << 16) |
           (static_cast<A_long>(static_cast<unsigned char>(c)) << 8) |
           static_cast<A_long>(static_cast<unsigned char>(d));
}

[[nodiscard]] constexpr A_long connection_count_id() noexcept { return fourcc('n', 'c', 'n', '0'); }
[[nodiscard]] constexpr A_long layout_x_id() noexcept { return fourcc('n', 'l', 'x', '0'); }
[[nodiscard]] constexpr A_long layout_y_id() noexcept { return fourcc('n', 'l', 'y', '0'); }
[[nodiscard]] constexpr A_long connection_uuid_id(A_long slot, A_long chunk) noexcept {
    return fourcc('l', static_cast<char>('0' + slot), 'u', static_cast<char>('0' + chunk));
}
[[nodiscard]] constexpr A_long connection_edge_uuid_id(A_long slot, A_long chunk) noexcept {
    return fourcc('l', static_cast<char>('0' + slot), 'e', static_cast<char>('0' + chunk));
}

} // namespace starfield::adapter::native_nodes
