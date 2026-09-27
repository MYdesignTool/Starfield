#pragma once

#include <cstdint>
#include <optional>

namespace starfield::core {

// Signed rational host time: value / scale, in host time units (AE: time_step /
// time_scale). `scale` is always positive and the fraction is reduced, so equal
// instants produced by different hosts compare equal. See ADR 0002.
struct RationalTime {
    std::int64_t value{0};
    std::int64_t scale{1};
};

// Normalizes value/scale. Returns nullopt for a non-positive scale or when
// reduction itself would overflow the signed 64-bit range.
[[nodiscard]] std::optional<RationalTime> make_rational(std::int64_t value, std::int64_t scale) noexcept;

// Checked rational arithmetic. Each function returns nullopt instead of wrapping
// or trapping; callers map that onto ErrorCode::invalid_time.
[[nodiscard]] std::optional<RationalTime> add(RationalTime a, RationalTime b) noexcept;
[[nodiscard]] std::optional<RationalTime> subtract(RationalTime a, RationalTime b) noexcept;
[[nodiscard]] std::optional<RationalTime> multiply(RationalTime a, RationalTime b) noexcept;

// Returns -1, 0 or 1. Exact unless an intermediate product overflows, in which
// case the comparison falls back to extended-precision floating point.
[[nodiscard]] int compare(RationalTime a, RationalTime b) noexcept;

[[nodiscard]] constexpr bool is_negative(RationalTime value) noexcept { return value.value < 0; }
[[nodiscard]] constexpr bool is_zero(RationalTime value) noexcept { return value.value == 0; }

// Lossy conversion used exactly once, at the simulation boundary (ADR 0002).
// Never used to sample parameters or to compare host times.
[[nodiscard]] double to_seconds(RationalTime value) noexcept;

} // namespace starfield::core
