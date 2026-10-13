#include "starfield/core/TurbulenceNoise.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace starfield::core;

namespace {
int checks{};
int failures{};
void check(bool condition, const char* label) {
    ++checks;
    if (!condition) {
        ++failures;
        std::fprintf(stderr, "FAIL: %s\n", label);
    }
}
bool finite(Vec3 value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}
bool bounded(Vec3 value) {
    return finite(value) && std::abs(value.x) <= 1 && std::abs(value.y) <= 1 && std::abs(value.z) <= 1;
}
}

int main() {
    Vec3 first{}, repeat{}, different_seed{};
    check(sample_turbulence_noise3({.25, -1.5, 7.0}, 7, 3, 1.0, first), "valid sample accepted");
    check(sample_turbulence_noise3({.25, -1.5, 7.0}, 7, 3, 1.0, repeat), "repeat sample accepted");
    check(first.x == repeat.x && first.y == repeat.y && first.z == repeat.z, "same inputs are bitwise deterministic");
    check(sample_turbulence_noise3({.25, -1.5, 7.0}, 8, 3, 1.0, different_seed), "different seed accepted");
    check(first.x != different_seed.x || first.y != different_seed.y || first.z != different_seed.z,
        "seed selects a different field");
    check(std::abs(first.x - first.y) > 1e-6 && std::abs(first.y - first.z) > 1e-6,
        "vector channels have independent hash streams");

    for (std::uint32_t levels = kMinTurbulenceNoiseLevels; levels <= kMaxTurbulenceNoiseLevels; ++levels) {
        for (int z = -5; z <= 5; ++z) for (int y = -5; y <= 5; ++y) for (int x = -5; x <= 5; ++x) {
            Vec3 value{};
            check(sample_turbulence_noise3({x * .173, y * .219, z * .137},
                static_cast<std::uint32_t>(x * x + y * y + z * z), levels, .75, value), "grid sample accepted");
            check(bounded(value), "each octave stack stays finite and bounded");
        }
    }

    Vec3 left{}, right{}, near_sample{};
    check(sample_turbulence_noise3({.99999, .314, -.271}, 22, 4, 1.0, left), "left boundary sample accepted");
    check(sample_turbulence_noise3({1.00001, .314, -.271}, 22, 4, 1.0, right), "right boundary sample accepted");
    check(std::abs(left.x - right.x) < .001 && std::abs(left.y - right.y) < .001 &&
        std::abs(left.z - right.z) < .001, "field is continuous across a lattice boundary");
    check(sample_turbulence_noise3({.25001, -1.5, 7.0}, 7, 3, 1.0, near_sample), "near sample accepted");
    check(std::abs(first.x - near_sample.x) < .001 && std::abs(first.y - near_sample.y) < .001 &&
        std::abs(first.z - near_sample.z) < .001, "small coordinate change remains smooth");

    Vec3 unchanged{9, 8, 7};
    check(!sample_turbulence_noise3({std::numeric_limits<double>::quiet_NaN(), 0, 0}, 1, 1, 1, unchanged),
        "NaN coordinate rejected");
    check(unchanged.x == 9 && unchanged.y == 8 && unchanged.z == 7, "failure preserves output");
    check(!sample_turbulence_noise3({std::numeric_limits<double>::infinity(), 0, 0}, 1, 1, 1, unchanged),
        "infinite coordinate rejected");
    check(!sample_turbulence_noise3({kMaxTurbulenceNoiseCoordinate * 2, 0, 0}, 1, 1, 1, unchanged),
        "out-of-domain coordinate rejected");
    check(!sample_turbulence_noise3({}, 1, 0, 1, unchanged), "zero octaves rejected");
    check(!sample_turbulence_noise3({}, 1, kMaxTurbulenceNoiseLevels + 1, 1, unchanged), "excessive octaves rejected");
    check(!sample_turbulence_noise3({}, 1, 1, 0, unchanged), "zero frequency rejected");
    check(!sample_turbulence_noise3({}, 1, 1, std::numeric_limits<double>::infinity(), unchanged),
        "infinite frequency rejected");

    std::array<Vec3, 32> forward{}, reverse{};
    for (std::size_t i = 0; i < forward.size(); ++i) {
        const Vec3 position{static_cast<double>(i) * .37, static_cast<double>(i % 7) * -.23,
            static_cast<double>(i % 5) * .41};
        check(sample_turbulence_noise3(position, 91, 5, .4, forward[i]), "forward-order field sample accepted");
    }
    for (std::size_t i = reverse.size(); i-- > 0;) {
        const Vec3 position{static_cast<double>(i) * .37, static_cast<double>(i % 7) * -.23,
            static_cast<double>(i % 5) * .41};
        check(sample_turbulence_noise3(position, 91, 5, .4, reverse[i]), "reverse-order field sample accepted");
    }
    for (std::size_t i = 0; i < forward.size(); ++i)
        check(forward[i].x == reverse[i].x && forward[i].y == reverse[i].y && forward[i].z == reverse[i].z,
            "evaluation order does not change results");

    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
