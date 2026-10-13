#include "starfield/core/TurbulenceNoise.hpp"

#include "starfield/core/Random.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>

namespace starfield::core {
namespace {

constexpr std::array<Vec3, 12> kGradients{{
    { .7071067811865475,  .7071067811865475, 0},
    {-.7071067811865475,  .7071067811865475, 0},
    { .7071067811865475, -.7071067811865475, 0},
    {-.7071067811865475, -.7071067811865475, 0},
    { .7071067811865475, 0,  .7071067811865475},
    {-.7071067811865475, 0,  .7071067811865475},
    { .7071067811865475, 0, -.7071067811865475},
    {-.7071067811865475, 0, -.7071067811865475},
    {0,  .7071067811865475,  .7071067811865475},
    {0, -.7071067811865475,  .7071067811865475},
    {0,  .7071067811865475, -.7071067811865475},
    {0, -.7071067811865475, -.7071067811865475},
}};

std::uint64_t lattice_hash(std::int64_t x, std::int64_t y, std::int64_t z,
    std::uint32_t seed, std::uint64_t channel) noexcept {
    auto value = mix64(static_cast<std::uint64_t>(seed) ^ channel);
    value = mix64(value ^ std::bit_cast<std::uint64_t>(x));
    value = mix64(value ^ std::rotl(std::bit_cast<std::uint64_t>(y), 21));
    return mix64(value ^ std::rotl(std::bit_cast<std::uint64_t>(z), 42));
}

double fade(double value) noexcept {
    // Quintic interpolation has zero first and second derivatives at lattice
    // boundaries, avoiding visible seams while a particle crosses a cell.
    return value * value * value * (value * (value * 6.0 - 15.0) + 10.0);
}

double gradient_dot(std::int64_t x, std::int64_t y, std::int64_t z,
    double dx, double dy, double dz, std::uint32_t seed, std::uint64_t channel) noexcept {
    const auto index = static_cast<std::size_t>(lattice_hash(x, y, z, seed, channel) % kGradients.size());
    const auto& gradient = kGradients[index];
    return gradient.x * dx + gradient.y * dy + gradient.z * dz;
}

double noise3(Vec3 coordinate, std::uint32_t seed, std::uint64_t channel) noexcept {
    const auto ix = static_cast<std::int64_t>(std::floor(coordinate.x));
    const auto iy = static_cast<std::int64_t>(std::floor(coordinate.y));
    const auto iz = static_cast<std::int64_t>(std::floor(coordinate.z));
    const auto fx = coordinate.x - static_cast<double>(ix);
    const auto fy = coordinate.y - static_cast<double>(iy);
    const auto fz = coordinate.z - static_cast<double>(iz);
    const auto u = fade(fx), v = fade(fy), w = fade(fz);

    const auto x00 = std::lerp(gradient_dot(ix, iy, iz, fx, fy, fz, seed, channel),
        gradient_dot(ix + 1, iy, iz, fx - 1, fy, fz, seed, channel), u);
    const auto x10 = std::lerp(gradient_dot(ix, iy + 1, iz, fx, fy - 1, fz, seed, channel),
        gradient_dot(ix + 1, iy + 1, iz, fx - 1, fy - 1, fz, seed, channel), u);
    const auto x01 = std::lerp(gradient_dot(ix, iy, iz + 1, fx, fy, fz - 1, seed, channel),
        gradient_dot(ix + 1, iy, iz + 1, fx - 1, fy, fz - 1, seed, channel), u);
    const auto x11 = std::lerp(gradient_dot(ix, iy + 1, iz + 1, fx, fy - 1, fz - 1, seed, channel),
        gradient_dot(ix + 1, iy + 1, iz + 1, fx - 1, fy - 1, fz - 1, seed, channel), u);
    const auto y0 = std::lerp(x00, x10, v);
    const auto y1 = std::lerp(x01, x11, v);
    return std::clamp(std::lerp(y0, y1, w) * 1.6, -1.0, 1.0);
}

double fractal_noise(Vec3 coordinate, std::uint32_t seed, std::uint32_t levels,
    double frequency, std::uint64_t channel) noexcept {
    double amplitude = 1.0;
    double amplitude_sum = 0.0;
    double value = 0.0;
    for (std::uint32_t octave = 0; octave < levels; ++octave) {
        const double octave_frequency = frequency * static_cast<double>(std::uint64_t{1} << octave);
        const Vec3 sample{coordinate.x * octave_frequency, coordinate.y * octave_frequency,
            coordinate.z * octave_frequency};
        value += amplitude * noise3(sample, seed, channel);
        amplitude_sum += amplitude;
        amplitude *= 0.5;
    }
    return amplitude_sum > 0.0 ? std::clamp(value / amplitude_sum, -1.0, 1.0) : 0.0;
}

bool valid_coordinate(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
        std::abs(value.x) <= kMaxTurbulenceNoiseCoordinate &&
        std::abs(value.y) <= kMaxTurbulenceNoiseCoordinate &&
        std::abs(value.z) <= kMaxTurbulenceNoiseCoordinate;
}

} // namespace

bool sample_turbulence_noise3(Vec3 coordinate, std::uint32_t seed, std::uint32_t levels,
    double frequency, Vec3& output) noexcept {
    if (!valid_coordinate(coordinate) || levels < kMinTurbulenceNoiseLevels ||
        levels > kMaxTurbulenceNoiseLevels || !std::isfinite(frequency) ||
        frequency <= 0.0 || frequency > kMaxTurbulenceNoiseFrequency ||
        frequency * static_cast<double>(std::uint64_t{1} << (levels - 1)) >
            kMaxTurbulenceNoiseFrequency * static_cast<double>(std::uint64_t{1} << (kMaxTurbulenceNoiseLevels - 1))) {
        return false;
    }

    constexpr std::uint64_t kChannelX = 0x54555242554c5831ull;
    constexpr std::uint64_t kChannelY = 0x54555242554c5932ull;
    constexpr std::uint64_t kChannelZ = 0x54555242554c5a33ull;
    const Vec3 candidate{
        fractal_noise(coordinate, seed, levels, frequency, kChannelX),
        fractal_noise(coordinate, seed, levels, frequency, kChannelY),
        fractal_noise(coordinate, seed, levels, frequency, kChannelZ)};
    if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) return false;
    output = candidate;
    return true;
}

} // namespace starfield::core
