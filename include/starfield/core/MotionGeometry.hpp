#pragma once

#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace starfield::core {
class Cancellation;
inline constexpr std::size_t kMaxMotionPathControlPoints=256;
inline constexpr std::size_t kMaxMotionPathSamples=16385;
inline constexpr std::size_t kMaxMotionPathEvaluations=262144;
inline constexpr double kMaxMotionCoordinate=1e9;

// Geometry only: the public Motion Speed/radius/target policies are not encoded
// in these helpers. Radians and a weighted shortest-arc rotation are explicit.
[[nodiscard]] Result<Vec3> rotate_motion_circle(Vec3 position,Vec3 origin,Vec3 axis,double radians) noexcept;
[[nodiscard]] Result<std::array<double,9>> motion_look_at_rotation(
    Vec3 forward,Vec3 origin,Vec3 goal,double weight) noexcept;

class CompiledMotionPath {
public:
    [[nodiscard]] double length() const noexcept { return distances_.empty()?0:distances_.back(); }
    [[nodiscard]] std::size_t sample_count() const noexcept { return parameters_.size(); }
    [[nodiscard]] std::size_t storage_bytes() const noexcept {
        return sizeof(*this)+(controls_.capacity()+derivative_controls_.capacity())*sizeof(Vec3)+
            (knots_.capacity()+derivative_knots_.capacity()+parameters_.capacity()+distances_.capacity())*sizeof(double);
    }
    [[nodiscard]] Result<Vec3> position_at_distance(double distance) const noexcept;
    // An exact zero derivative yields a zero tangent: callers preserve their
    // existing orientation instead of inventing a direction at a stationary point.
    [[nodiscard]] Result<Vec3> tangent_at_distance(double distance) const noexcept;
    CompiledMotionPath(const CompiledMotionPath&)=default;
    CompiledMotionPath(CompiledMotionPath&&) noexcept=default;
    CompiledMotionPath& operator=(const CompiledMotionPath&);
    CompiledMotionPath& operator=(CompiledMotionPath&&) noexcept=default;
    ~CompiledMotionPath()=default;
private:
    CompiledMotionPath()=default;
    [[nodiscard]] double parameter_at_distance(double distance) const noexcept;
    std::vector<Vec3> controls_,derivative_controls_;
    std::vector<double> knots_,derivative_knots_,parameters_,distances_;
    Vec3 origin_{},end_{};
    unsigned degree_{};
    friend Result<CompiledMotionPath> compile_motion_path(std::span<const Vec3>,const Cancellation&) noexcept;
};

[[nodiscard]] Result<CompiledMotionPath> compile_motion_path(
    std::span<const Vec3> control_points,const Cancellation&) noexcept;
} // namespace starfield::core
