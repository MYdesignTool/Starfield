#pragma once

#include <cstddef>
#include <cstdint>

namespace starfield::adapter::node_sync {

inline constexpr char kRequestPrefix[] = "/*SFLDNODE1:";
inline constexpr char kRequestSuffix[] = "*/0";
inline constexpr std::size_t kMaxRequestBytes = 256;
inline constexpr std::uint32_t kMaxNonce = 1000000;

inline constexpr int kGraphRequestStreamIndex = 42;
inline constexpr int kGraphCommitStreamIndex = 43;
inline constexpr int kGraphReceiptStreamIndex = 44;

enum class ValueKind : char { float64 = 'd', uint32 = 'u', vector3 = 'v' };

} // namespace starfield::adapter::node_sync
