#include "starfield/core/Time.hpp"

#include <limits>

namespace starfield::core {
namespace {

constexpr std::uint64_t magnitude(std::int64_t value) noexcept {
    return value < 0 ? static_cast<std::uint64_t>(-(value + 1)) + 1u : static_cast<std::uint64_t>(value);
}

std::uint64_t greatest_common_divisor(std::uint64_t a, std::uint64_t b) noexcept {
    while (b != 0) {
        const std::uint64_t remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

// Overflow-checked 64x64 -> 64 multiply. All products are pre-checked so the
// multiplication itself is never performed on out-of-range operands.
bool multiply_checked(std::int64_t a, std::int64_t b, std::int64_t& out) noexcept {
    constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
    constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

    if (a == 0 || b == 0) {
        out = 0;
        return true;
    }

    bool overflow = false;
    if (a > 0) {
        overflow = b > 0 ? a > kMax / b : b < kMin / a;
    } else {
        overflow = b > 0 ? a < kMin / b : a < kMax / b;
    }
    if (overflow) {
        return false;
    }
    out = a * b;
    return true;
}

bool add_checked(std::int64_t a, std::int64_t b, std::int64_t& out) noexcept {
    constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
    constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

    if ((b > 0 && a > kMax - b) || (b < 0 && a < kMin - b)) {
        return false;
    }
    out = a + b;
    return true;
}

bool subtract_checked(std::int64_t a, std::int64_t b, std::int64_t& out) noexcept {
    constexpr std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
    constexpr std::int64_t kMax = std::numeric_limits<std::int64_t>::max();

    if ((b < 0 && a > kMax + b) || (b > 0 && a < kMin + b)) {
        return false;
    }
    out = a - b;
    return true;
}

// a/b + op * c/d with a single reduction at the end.
std::optional<RationalTime> combine(RationalTime a, RationalTime b, bool subtract_operand) noexcept {
    std::int64_t left = 0;
    std::int64_t right = 0;
    std::int64_t numerator = 0;
    std::int64_t denominator = 0;

    if (!multiply_checked(a.value, b.scale, left) || !multiply_checked(b.value, a.scale, right)) {
        return std::nullopt;
    }
    const bool combined = subtract_operand ? subtract_checked(left, right, numerator)
                                          : add_checked(left, right, numerator);
    if (!combined || !multiply_checked(a.scale, b.scale, denominator)) {
        return std::nullopt;
    }
    return make_rational(numerator, denominator);
}

} // namespace

std::optional<RationalTime> make_rational(std::int64_t value, std::int64_t scale) noexcept {
    if (scale <= 0) {
        return std::nullopt;
    }
    if (value == 0) {
        return RationalTime{0, 1};
    }

    const std::uint64_t divisor = greatest_common_divisor(magnitude(value), static_cast<std::uint64_t>(scale));
    if (divisor > 1) {
        const auto reduction = static_cast<std::int64_t>(divisor);
        value /= reduction;
        scale /= reduction;
    }
    return RationalTime{value, scale};
}

std::optional<RationalTime> add(RationalTime a, RationalTime b) noexcept {
    return combine(a, b, false);
}

std::optional<RationalTime> subtract(RationalTime a, RationalTime b) noexcept {
    return combine(a, b, true);
}

std::optional<RationalTime> multiply(RationalTime a, RationalTime b) noexcept {
    std::int64_t numerator = 0;
    std::int64_t denominator = 0;
    if (!multiply_checked(a.value, b.value, numerator) || !multiply_checked(a.scale, b.scale, denominator)) {
        return std::nullopt;
    }
    return make_rational(numerator, denominator);
}

int compare(RationalTime a, RationalTime b) noexcept {
    std::int64_t left = 0;
    std::int64_t right = 0;
    if (multiply_checked(a.value, b.scale, left) && multiply_checked(b.value, a.scale, right)) {
        if (left < right) {
            return -1;
        }
        return left > right ? 1 : 0;
    }

    // Only reachable for host times whose cross products exceed 64 bits.
    const long double lhs = static_cast<long double>(a.value) / static_cast<long double>(a.scale);
    const long double rhs = static_cast<long double>(b.value) / static_cast<long double>(b.scale);
    if (lhs < rhs) {
        return -1;
    }
    return lhs > rhs ? 1 : 0;
}

double to_seconds(RationalTime value) noexcept {
    return static_cast<double>(value.value) / static_cast<double>(value.scale);
}

} // namespace starfield::core
