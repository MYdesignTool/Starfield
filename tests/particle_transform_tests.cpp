#include "starfield/core/ParticleTransform.hpp"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace starfield::core;

namespace {
unsigned checks{};
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
void near(double actual, double expected, const char* message) {
    check(std::isfinite(actual) && std::abs(actual-expected) <= 1e-10*std::max(1.0,std::abs(expected)),message);
}
void near(Vec3 actual, Vec3 expected, const char* message) {
    near(actual.x,expected.x,message); near(actual.y,expected.y,message); near(actual.z,expected.z,message);
}
CompiledParticleTransform compile(const ParticleTransformSettings& settings) {
    const auto result=compile_particle_transform(settings);
    check(result.has_value(),"valid transform rejected");
    return result.value();
}
void rejected(const ParticleTransformSettings& settings) {
    const auto result=compile_particle_transform(settings);
    check(!result.has_value(),"invalid transform accepted");
    check(result.error().code==ErrorCode::invalid_request,"wrong transform error");
}

void identity_and_translation() {
    ParticleTransformSettings settings;
    auto transform=compile(settings);
    near(transform.position({1,2,3}),Vec3{1,2,3},"identity centre");
    near(transform.velocity({-3,4,-5}),Vec3{-3,4,-5},"identity velocity");
    near(transform.particle_axis({-3,4,-5}),Vec3{-3,4,-5},"identity sprite axis");
    near(transform.particle_scale(),1,"identity size");
    near(transform.particle_opacity(),1,"identity opacity");
    const std::array<double,16> identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    for (std::size_t i=0;i<identity.size();++i) near(transform.world_matrix()[i],identity[i],"identity matrix");
    settings.position={4,5,6};
    transform=compile(settings);
    near(transform.position({1,2,3}),Vec3{5,7,9},"translation centre");
    near(transform.velocity({1,2,3}),Vec3{1,2,3},"translation leaked into velocity");
    near(transform.particle_axis({1,2,3}),Vec3{1,2,3},"translation leaked into sprite axis");
}

void pivot_scale_and_rotation() {
    ParticleTransformSettings settings;
    settings.anchor={1,2,3}; settings.position={2,-3,4}; settings.scale_percent={200,50,-100};
    auto transform=compile(settings);
    near(transform.position({3,6,9}),Vec3{7,1,1},"pivot scale centre");
    near(transform.position(settings.anchor),Vec3{3,-1,7},"pivot moved incorrectly");
    near(transform.velocity({1,2,3}),Vec3{2,1,-3},"scale velocity");
    near(transform.particle_axis({1,0,0}),Vec3{1,0,0},"system scale changed sprite axis");
    settings={}; settings.anchor={1,1,0}; settings.rotation_degrees.z=90;
    transform=compile(settings);
    near(transform.position({2,1,0}),Vec3{1,2,0},"rotation around pivot");
    near(transform.velocity({1,0,0}),Vec3{0,1,0},"rotated velocity");
    near(transform.particle_axis({1,0,0}),Vec3{0,1,0},"rotated sprite axis");
    settings={}; settings.rotation_degrees={90,90,0};
    transform=compile(settings);
    near(transform.particle_axis({0,1,0}),Vec3{1,0,0},"Euler X then Y order");
    settings.rotation_degrees={720,-1080,360};
    transform=compile(settings);
    near(transform.position({2,3,4}),Vec3{2,3,4},"whole turns changed centre");
    settings.scale_percent={0,0,0}; settings.anchor={7,8,9}; settings.position={1,2,3};
    settings.particles_scale_percent=250; settings.particles_opacity_percent=30;
    transform=compile(settings);
    near(transform.position({100,-100,33}),Vec3{8,10,12},"zero scale collapse");
    near(transform.velocity({100,-100,33}),Vec3{},"zero scale velocity");
    near(transform.particle_axis({1,0,0}),Vec3{1,0,0},"zero system scale collapsed sprite");
    near(transform.particle_scale(),2.5,"particle scale multiplier");
    near(transform.particle_opacity(),.3,"particle opacity multiplier");
}

void inherited_affine_and_composition() {
    ParticleTransformSettings settings;
    settings.position={1,2,3};
    settings.inherited_motion={0,-2,0,7, 3,0,0,8, 0,0,-4,9, 0,0,0,1};
    auto transform=compile(settings);
    near(transform.position({1,0,0}),Vec3{3,14,-3},"inherited affine centre");
    near(transform.velocity({1,2,3}),Vec3{-4,3,-12},"inherited affine velocity");
    near(transform.particle_axis({1,0,0}),Vec3{0,3,0},"inherited scale lost on sprite");
    near(transform.particle_axis({0,0,1}),Vec3{0,0,-4},"inherited reflection lost");
    settings={}; settings.inherited_motion={1,2,0,0, 0,1,.5,0, 0,0,1,0, 0,0,0,1};
    transform=compile(settings);
    near(transform.particle_axis({0,1,0}),Vec3{2,1,0},"inherited shear lost");
    settings.inherited_motion={0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,1};
    transform=compile(settings);
    near(transform.particle_axis({1,2,3}),Vec3{},"singular inherited basis");

    // Check full affine composition against two separately compiled transforms.
    // Sprite bases are intentionally tested separately: local system scale must
    // not be used as a parent sprite scale when chaining graph nodes.
    ParticleTransformSettings first_settings,second_settings;
    first_settings.anchor={2,3,-1}; first_settings.position={1,-4,7};
    first_settings.rotation_degrees={20,-70,30}; first_settings.scale_percent={130,70,-20};
    second_settings.anchor={-4,2,1}; second_settings.position={-1,3,6};
    second_settings.rotation_degrees={-15,40,100}; second_settings.scale_percent={60,-110,200};
    const auto first=compile(first_settings),second=compile(second_settings);
    const auto chained=compose_particle_transforms(first,second);
    check(chained.has_value(),"valid chain rejected");
    first_settings.inherited_motion=second.world_matrix();
    const auto combined=compile(first_settings);
    for (int i=-10;i<=10;++i) {
        const Vec3 point{static_cast<double>(i)*.3,static_cast<double>(i*i)*.02,static_cast<double>(i-4)*-.11};
        near(combined.position(point),second.position(first.position(point)),"affine composition centre");
        near(combined.velocity(point),second.velocity(first.velocity(point)),"affine composition velocity");
        near(chained.value().position(point),second.position(first.position(point)),"compiled chain centre");
        near(chained.value().velocity(point),second.velocity(first.velocity(point)),"compiled chain velocity");
        near(chained.value().particle_axis(point),second.particle_axis(first.particle_axis(point)),"compiled chain sprite basis");
    }
    first_settings={};second_settings={};
    first_settings.particles_scale_percent=250;first_settings.particles_opacity_percent=20;
    second_settings.particles_scale_percent=30;second_settings.particles_opacity_percent=70;
    const auto factors=compose_particle_transforms(compile(first_settings),compile(second_settings));
    check(factors.has_value(),"valid factor chain rejected");
    near(factors.value().particle_scale(),.75,"chained sprite scale");
    near(factors.value().particle_opacity(),.14,"chained opacity");
    first_settings.particles_scale_percent=10000;
    const auto maximum=compile(first_settings);
    const auto twice=compose_particle_transforms(maximum,maximum);
    const auto thrice=compose_particle_transforms(twice.value(),maximum);
    check(twice.has_value() && thrice.has_value(),"scale product boundary rejected");
    const auto excessive=compose_particle_transforms(thrice.value(),maximum);
    check(!excessive.has_value() && excessive.error().code==ErrorCode::work_limit_exceeded,"scale product overflow not bounded");
}

void bounds_and_invalid_inputs() {
    const double nan=std::numeric_limits<double>::quiet_NaN(),inf=std::numeric_limits<double>::infinity();
    ParticleTransformSettings settings;
    settings.anchor.x=nan; rejected(settings);
    settings={}; settings.position.y=inf; rejected(settings);
    settings={}; settings.rotation_degrees.z=kMaxEmissionAngleDegrees+1; rejected(settings);
    settings={}; settings.scale_percent.y=10001; rejected(settings);
    settings={}; settings.scale_percent.x=-10001; rejected(settings);
    settings={}; settings.particles_scale_percent=-1; rejected(settings);
    settings={}; settings.particles_scale_percent=inf; rejected(settings);
    settings={}; settings.particles_opacity_percent=101; rejected(settings);
    settings={}; settings.particles_opacity_percent=-.1; rejected(settings);
    settings={}; settings.inherited_motion[0]=nan; rejected(settings);
    settings={}; settings.inherited_motion[3]=1'000'001; rejected(settings);
    settings={}; settings.inherited_motion[12]=.1; rejected(settings);
    settings={}; settings.inherited_motion[13]=.1; rejected(settings);
    settings={}; settings.inherited_motion[14]=.1; rejected(settings);
    settings={}; settings.inherited_motion[15]=2; rejected(settings);
    settings={}; settings.anchor.x=1'000'001; rejected(settings);

    // Boundaries, reflections, singular scales and zero opacity are supported.
    settings={}; settings.anchor={1'000'000,-1'000'000,1'000'000};
    settings.position=settings.anchor; settings.rotation_degrees={kMaxEmissionAngleDegrees,-kMaxEmissionAngleDegrees,0};
    settings.scale_percent={10'000,-10'000,0}; settings.particles_scale_percent=10'000;
    settings.particles_opacity_percent=0;
    settings.inherited_motion={1'000'000,-1'000'000,1'000'000,1'000'000,
                              -1'000'000,1'000'000,1'000'000,-1'000'000,
                              1'000'000,1'000'000,-1'000'000,1'000'000,0,0,0,1};
    const auto transform=compile(settings);
    const auto point=transform.position({2,-3,4}),axis=transform.particle_axis({1,0,0});
    check(std::isfinite(point.x)&&std::isfinite(point.y)&&std::isfinite(point.z),"bounded centre overflow");
    check(std::isfinite(axis.x)&&std::isfinite(axis.y)&&std::isfinite(axis.z),"bounded axis overflow");
    near(transform.particle_opacity(),0,"zero opacity");
    near(transform.particle_scale(),100,"maximum particle scale");
}
} // namespace

int main() {
    try {
        identity_and_translation(); pivot_scale_and_rotation();
        inherited_affine_and_composition(); bounds_and_invalid_inputs();
        std::cout << "Particle Transform: " << checks << " checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Particle Transform failed after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
