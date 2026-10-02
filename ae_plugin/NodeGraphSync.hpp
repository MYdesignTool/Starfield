#pragma once

#include "Parameters.hpp"

#include <cstddef>
#include <cstdint>

namespace starfield::adapter::node_sync {

inline constexpr std::uint32_t kMaxNonce = 1000000;

inline constexpr int kGraphCommitStreamIndex = kGraphEditCommitId;

} // namespace starfield::adapter::node_sync
