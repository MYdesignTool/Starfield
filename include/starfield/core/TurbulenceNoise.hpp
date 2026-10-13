#pragma once

#include "starfield/core/Settings.hpp"

#include <cstdint>

namespace starfield::core {

inline constexpr std::uint32_t kMinTurbulenceNoiseLevels = 1;
inline constexpr std::uint32_t kMaxTurbulenceNoiseLevels = 8;
inline constexpr double kMaxTurbulenceNoiseFrequency = 1024.0;
inline constexpr double kMaxTurbulenceNoiseCoordinate = 1.0e9;

// Deterministic, allocation-free, smooth three-channel fractal noise. `coordinate`
// is an implementation-space input; the graph/AE adapter owns the mapping from
// Stardust's Noise Scale/Frequency/Fractal Speed controls into this space.
// Each channel uses an independent hash stream. Values are finite and bounded
// to [-1, 1], regardless of octave count.
[[nodiscard]] bool sample_turbulence_noise3(Vec3 coordinate, std::uint32_t seed,
    std::uint32_t levels, double frequency, Vec3& output) noexcept;

} // namespace starfield::core
