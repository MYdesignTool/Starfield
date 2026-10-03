#pragma once
#include <cstdint>
#include <type_traits>

namespace starfield::adapter {
// Synchronous, private read-only message. No handles, PF callbacks, graph data or
// project writes cross this boundary. Unavailable contexts acknowledge a miss.
struct NativeBootstrapRequest {
    std::uint32_t magic{0x53464931};
    std::uint32_t bytes{sizeof(NativeBootstrapRequest)};
    std::uint32_t version{1};
    std::uint32_t acknowledged{};
    std::uint32_t refreshed{};
    std::uint32_t proofs{};
    std::int32_t error{};
};
static_assert(std::is_standard_layout_v<NativeBootstrapRequest> &&
    std::is_trivially_copyable_v<NativeBootstrapRequest>);
}
