#include "starfield/core/ParticleTransform.hpp"

#include <cmath>
#include <numbers>

namespace starfield::core {
namespace {
bool bounded(double value, double limit) noexcept {
    return std::isfinite(value) && std::abs(value) <= limit;
}
bool bounded(Vec3 value, double limit) noexcept {
    return bounded(value.x,limit) && bounded(value.y,limit) && bounded(value.z,limit);
}
Vec3 vector(const std::array<double,16>& matrix, Vec3 value) noexcept {
    return {matrix[0]*value.x+matrix[1]*value.y+matrix[2]*value.z,
            matrix[4]*value.x+matrix[5]*value.y+matrix[6]*value.z,
            matrix[8]*value.x+matrix[9]*value.y+matrix[10]*value.z};
}
} // namespace

bool valid_particle_sprite_basis(const ParticleSpriteBasis& basis) noexcept {
    for(double value:basis) if(!bounded(value,kMaxParticleSpriteBasisCoefficient)) return false;
    return true;
}

Vec3 CompiledParticleTransform::position(Vec3 value) const noexcept {
    const auto rotated=vector(world_,value);
    return {rotated.x+world_[3],rotated.y+world_[7],rotated.z+world_[11]};
}
Vec3 CompiledParticleTransform::velocity(Vec3 value) const noexcept {
    return vector(world_,value);
}
Vec3 CompiledParticleTransform::particle_axis(Vec3 value) const noexcept {
    return {particle_basis_[0]*value.x+particle_basis_[1]*value.y+particle_basis_[2]*value.z,
            particle_basis_[3]*value.x+particle_basis_[4]*value.y+particle_basis_[5]*value.z,
            particle_basis_[6]*value.x+particle_basis_[7]*value.y+particle_basis_[8]*value.z};
}

Result<CompiledParticleTransform> compile_particle_transform(const ParticleTransformSettings& settings) noexcept {
    using R=Result<CompiledParticleTransform>;
    if (!bounded(settings.anchor,1'000'000) || !bounded(settings.position,1'000'000) ||
        !bounded(settings.rotation_degrees,kMaxEmissionAngleDegrees) || !bounded(settings.scale_percent,10'000) ||
        !bounded(settings.particles_scale_percent,10'000) || settings.particles_scale_percent<0 ||
        !bounded(settings.particles_opacity_percent,100) || settings.particles_opacity_percent<0)
        return R::failure(ErrorCode::invalid_request,"Transform values exceed supported bounds");
    for (double value:settings.inherited_motion) if (!bounded(value,1'000'000))
        return R::failure(ErrorCode::invalid_request,"Inherited Transform matrix is invalid");
    const auto& parent=settings.inherited_motion;
    if (parent[12]!=0 || parent[13]!=0 || parent[14]!=0 || parent[15]!=1)
        return R::failure(ErrorCode::invalid_request,"Inherited Transform must be affine");

    constexpr double radians=std::numbers::pi/180.0;
    const double x=std::remainder(settings.rotation_degrees.x,360.0)*radians;
    const double y=std::remainder(settings.rotation_degrees.y,360.0)*radians;
    const double z=std::remainder(settings.rotation_degrees.z,360.0)*radians;
    const double sx=std::sin(x),cx=std::cos(x),sy=std::sin(y),cy=std::cos(y),sz=std::sin(z),cz=std::cos(z);
    // X, then Y, then Z: Rz * Ry * Rx, matching the core's particle Euler convention.
    const std::array<double,9> rotation{
        cz*cy,cz*sy*sx-sz*cx,cz*sy*cx+sz*sx,
        sz*cy,sz*sy*sx+cz*cx,sz*sy*cx-cz*sx,
        -sy,cy*sx,cy*cx};
    const double scales[]{settings.scale_percent.x/100.0,settings.scale_percent.y/100.0,settings.scale_percent.z/100.0};
    std::array<double,16> local{0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
    for (std::size_t row=0;row<3;++row) for (std::size_t column=0;column<3;++column)
        local[row*4+column]=rotation[row*3+column]*scales[column];
    const Vec3 pivot=vector(local,settings.anchor);
    local[3]=settings.position.x+settings.anchor.x-pivot.x;
    local[7]=settings.position.y+settings.anchor.y-pivot.y;
    local[11]=settings.position.z+settings.anchor.z-pivot.z;

    CompiledParticleTransform compiled;
    for (std::size_t row=0;row<4;++row) for (std::size_t column=0;column<4;++column)
        for (std::size_t k=0;k<4;++k) compiled.world_[row*4+column]+=parent[row*4+k]*local[k*4+column];
    for (std::size_t row=0;row<3;++row) for (std::size_t column=0;column<3;++column)
        for (std::size_t k=0;k<3;++k) compiled.particle_basis_[row*3+column]+=parent[row*4+k]*rotation[k*3+column];
    compiled.particle_scale_=settings.particles_scale_percent/100.0;
    compiled.particle_opacity_=settings.particles_opacity_percent/100.0;
    return R::success(std::move(compiled));
}

Result<CompiledParticleTransform> compose_particle_transforms(
    const CompiledParticleTransform& before,const CompiledParticleTransform& after) noexcept {
    using R=Result<CompiledParticleTransform>;
    CompiledParticleTransform result;
    for(std::size_t row=0;row<4;++row) for(std::size_t col=0;col<4;++col)
        for(std::size_t k=0;k<4;++k) result.world_[row*4+col]+=after.world_[row*4+k]*before.world_[k*4+col];
    for(std::size_t row=0;row<3;++row) for(std::size_t col=0;col<3;++col)
        for(std::size_t k=0;k<3;++k) result.particle_basis_[row*3+col]+=after.particle_basis_[row*3+k]*before.particle_basis_[k*3+col];
    result.particle_scale_=before.particle_scale_*after.particle_scale_;
    result.particle_opacity_=before.particle_opacity_*after.particle_opacity_;
    for(double value:result.world_) if(!bounded(value,1e18))
        return R::failure(ErrorCode::work_limit_exceeded,"composed Transform centre matrix exceeds bounds");
    if(!valid_particle_sprite_basis(result.particle_basis_) || !bounded(result.particle_scale_,1e6))
        return R::failure(ErrorCode::work_limit_exceeded,"composed Transform sprite exceeds bounds");
    return R::success(std::move(result));
}

} // namespace starfield::core
