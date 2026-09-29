#pragma once

#include "starfield/core/Graph.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace starfield::core {

// Nested opaque graph payload: version:u8, point_count:u8, reserved:u16,
// followed by point_count little-endian IEEE-754 binary64 age/value pairs.
// The outer sequence codec supplies graph-level bounds and checksums.
inline constexpr std::uint8_t kAgeCurvePayloadVersion = 1;
inline constexpr std::size_t kAgeCurveHeaderBytes = 4;
inline constexpr std::size_t kAgeCurvePointBytes = 16;

[[nodiscard]] inline bool valid_age_curve(const AgeCurve& curve, double value_min,
                                          double value_max) noexcept {
    if (curve.count < 2 || curve.count > kMaxAgeCurvePoints) return false;
    if (curve.points[0].age != 0.0 || curve.points[curve.count - 1].age != 1.0) return false;
    double previous_age = -1.0;
    for (std::size_t i = 0; i < curve.count; ++i) {
        const auto& point = curve.points[i];
        if (!std::isfinite(point.age) || !std::isfinite(point.value) ||
            point.age < 0.0 || point.age > 1.0 || point.age <= previous_age ||
            point.value < value_min || point.value > value_max) return false;
        previous_age = point.age;
    }
    return true;
}

[[nodiscard]] inline double evaluate_age_curve(const AgeCurve& curve, double age,
                                               double linear_start, double linear_end) noexcept {
    age = std::clamp(age, 0.0, 1.0);
    if (curve.count < 2 || curve.count > kMaxAgeCurvePoints) {
        return linear_start + (linear_end - linear_start) * age;
    }
    if (age <= curve.points[0].age) return curve.points[0].value;
    for (std::size_t i = 1; i < curve.count; ++i) {
        const auto& right = curve.points[i];
        if (age <= right.age) {
            const auto& left = curve.points[i - 1];
            const double amount = (age - left.age) / (right.age - left.age);
            return left.value + (right.value - left.value) * amount;
        }
    }
    return curve.points[curve.count - 1].value;
}

[[nodiscard]] inline OpaqueBytes encode_age_curve(const AgeCurve& curve) {
    OpaqueBytes bytes;
    if (curve.count < 2 || curve.count > kMaxAgeCurvePoints) return bytes;
    bytes.reserve(kAgeCurveHeaderBytes + curve.count * kAgeCurvePointBytes);
    bytes.push_back(static_cast<std::byte>(kAgeCurvePayloadVersion));
    bytes.push_back(static_cast<std::byte>(curve.count));
    bytes.push_back(std::byte{0});
    bytes.push_back(std::byte{0});
    const auto append_u64 = [&bytes](std::uint64_t value) {
        for (unsigned int shift = 0; shift < 64; shift += 8) {
            bytes.push_back(static_cast<std::byte>((value >> shift) & 0xffu));
        }
    };
    for (std::size_t i = 0; i < curve.count; ++i) {
        append_u64(std::bit_cast<std::uint64_t>(curve.points[i].age));
        append_u64(std::bit_cast<std::uint64_t>(curve.points[i].value));
    }
    return bytes;
}

[[nodiscard]] inline bool decode_age_curve(const OpaqueBytes& bytes, AgeCurve& curve,
                                           double value_min, double value_max) noexcept {
    curve = {};
    if (bytes.size() < kAgeCurveHeaderBytes || bytes.size() >
        kAgeCurveHeaderBytes + kMaxAgeCurvePoints * kAgeCurvePointBytes) return false;
    if (std::to_integer<std::uint8_t>(bytes[0]) != kAgeCurvePayloadVersion ||
        std::to_integer<std::uint8_t>(bytes[2]) != 0 ||
        std::to_integer<std::uint8_t>(bytes[3]) != 0) return false;
    const auto count = std::to_integer<std::uint8_t>(bytes[1]);
    if (count < 2 || count > kMaxAgeCurvePoints ||
        bytes.size() != kAgeCurveHeaderBytes + count * kAgeCurvePointBytes) return false;
    const auto read_u64 = [&bytes](std::size_t offset) {
        std::uint64_t value = 0;
        for (unsigned int i = 0; i < 8; ++i) {
            value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes[offset + i])) << (i * 8);
        }
        return value;
    };
    curve.count = count;
    std::size_t offset = kAgeCurveHeaderBytes;
    for (std::size_t i = 0; i < count; ++i) {
        curve.points[i].age = std::bit_cast<double>(read_u64(offset));
        curve.points[i].value = std::bit_cast<double>(read_u64(offset + 8));
        offset += kAgeCurvePointBytes;
    }
    if (!valid_age_curve(curve, value_min, value_max)) {
        curve = {};
        return false;
    }
    return true;
}

} // namespace starfield::core
