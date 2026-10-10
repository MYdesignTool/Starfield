#pragma once
#include "starfield/core/MotionGeometry.hpp"
#include "starfield/core/MotionCurveClock.hpp"
#include "starfield/core/ParticleSimulation.hpp"
#include "starfield/core/ParticleTransform.hpp"

namespace starfield::core {
// Numeric travel over an explicitly supplied path. Reference author policies
// and resource selection are outside this helper; no host object is retained.
class CompiledMotionPathTravel {
public:
    [[nodiscard]] const MotionPathTravelSettings& settings() const noexcept {return settings_;}
    [[nodiscard]] double length() const noexcept {return path_.length();}
    [[nodiscard]] std::size_t curve_segment_count() const noexcept {return clock_.segment_count();}
    [[nodiscard]] std::size_t storage_bytes() const noexcept {return sizeof(*this)+path_.storage_bytes()-sizeof(CompiledMotionPath);}
    [[nodiscard]] Result<MotionPathTravelSample> travel(double age,double lifetime,double random_sample=0) const noexcept;
    // A caller with a historical integral supplies explicit distance/rate here.
    [[nodiscard]] Result<MotionPathTravelSample> travel_at_distance(double distance,double rate) const noexcept;
    [[nodiscard]] Result<bool> apply(ParticleInstance&,double random_sample,
        std::span<const ParticleSpriteBasis> bases={}) const noexcept;
    [[nodiscard]] Result<bool> apply_at_distance(ParticleInstance&,double distance,double rate,
        std::span<const ParticleSpriteBasis> bases={}) const noexcept;
    CompiledMotionPathTravel(const CompiledMotionPathTravel&)=default;
    CompiledMotionPathTravel(CompiledMotionPathTravel&&) noexcept=default;
    CompiledMotionPathTravel& operator=(const CompiledMotionPathTravel&)=default;
    CompiledMotionPathTravel& operator=(CompiledMotionPathTravel&&) noexcept=default;
private:
    CompiledMotionPathTravel(CompiledMotionPath path,CompiledMotionCurveClock clock,MotionPathTravelSettings settings,Vec3 start):
        path_(std::move(path)),clock_(std::move(clock)),settings_(settings),start_(start){}
    [[nodiscard]] Result<bool> apply_sample(ParticleInstance&,const MotionPathTravelSample&,
        std::span<const ParticleSpriteBasis>) const noexcept;
    CompiledMotionPath path_;
    CompiledMotionCurveClock clock_;
    MotionPathTravelSettings settings_;
    Vec3 start_;
    friend Result<CompiledMotionPathTravel> compile_motion_path_travel(
        std::span<const Vec3>,const MotionPathTravelSettings&,const Cancellation&) noexcept;
};
[[nodiscard]] Result<CompiledMotionPathTravel> compile_motion_path_travel(
    std::span<const Vec3>,const MotionPathTravelSettings&,const Cancellation&) noexcept;
} // namespace starfield::core
