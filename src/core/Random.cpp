#include "starfield/core/Random.hpp"

namespace starfield::core {
namespace {

// 1 / 2^53: the number of distinct values a double represents below 1.0 when the
// exponent is fixed, which keeps the mapping uniform and platform independent.
constexpr double kInverseTwoTo53 = 1.0 / 9007199254740992.0;

} // namespace

std::uint64_t stream_bits(std::uint32_t seed, std::uint64_t particle_id, RandomPurpose purpose) noexcept {
    std::uint64_t bits = mix64(static_cast<std::uint64_t>(seed) ^ 0x2545F4914F6CDD1Dull);
    bits ^= mix64(particle_id + 0x9E3779B97F4A7C15ull);
    bits ^= mix64(static_cast<std::uint64_t>(purpose) + 0xBF58476D1CE4E5B9ull);
    return mix64(bits);
}

double unit_from_bits(std::uint64_t bits) noexcept {
    return static_cast<double>(bits >> 11) * kInverseTwoTo53;
}

double symmetric_from_bits(std::uint64_t bits) noexcept {
    return unit_from_bits(bits) * 2.0 - 1.0;
}

double unit_value(std::uint32_t seed, std::uint64_t particle_id, RandomPurpose purpose) noexcept {
    return unit_from_bits(stream_bits(seed, particle_id, purpose));
}

double symmetric_value(std::uint32_t seed, std::uint64_t particle_id, RandomPurpose purpose) noexcept {
    return symmetric_from_bits(stream_bits(seed, particle_id, purpose));
}

} // namespace starfield::core
