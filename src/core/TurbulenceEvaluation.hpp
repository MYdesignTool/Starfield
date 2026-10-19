#pragma once

// Included after GraphEvaluation.cpp's typed graph helpers, inside its
// translation-unit-local namespace.

inline constexpr double kMaxTurbulenceAmplitude = 100'000.0;
inline constexpr double kMaxTurbulenceEvolutionSpeed = 100'000.0;
inline constexpr std::uint64_t kMaxTurbulenceNoiseWork = 24'000'000;

struct TurbulenceValues {
    Vec3 origin{};
    double amplitude{};
    double frequency{1.0};
    std::uint32_t levels{1};
    TurbulenceMode mode{TurbulenceMode::normal};
    std::uint32_t axis{2};
    TurbulenceAffect affect{TurbulenceAffect::all_particles};
    double sphere_size{1.0},sphere_feather_percent{100.0};
    double randomize_percent{};
    double evolution_speed{};
    Vec3 offset{};
    AgeCurve over_life{};
    double position_offset{100.0},scale_offset{},color_offset{},opacity_offset{},rotation_offset{};
    double noise_scale{100.0},evolution{};
    TurbulenceProperties properties{TurbulenceProperties::all};
};

inline bool bounded_turbulence_vector(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z) &&
        std::abs(value.x) <= kMaxTurbulenceNoiseCoordinate &&
        std::abs(value.y) <= kMaxTurbulenceNoiseCoordinate &&
        std::abs(value.z) <= kMaxTurbulenceNoiseCoordinate;
}

inline Result<TurbulenceValues> read_turbulence(const GraphNode& node) {
    using R = Result<TurbulenceValues>;
    TurbulenceValues result;
    const auto* raw_origin = find_value(node, graph_keys::kTurbulenceOrigin);
    const auto* origin = raw_origin ? std::get_if<Vec3>(raw_origin) : nullptr;
    const auto* raw_amplitude = find_value(node, graph_keys::kTurbulenceAmplitude);
    const auto* amplitude = raw_amplitude ? std::get_if<double>(raw_amplitude) : nullptr;
    const auto* raw_frequency = find_value(node, graph_keys::kTurbulenceFrequency);
    const auto* frequency = raw_frequency ? std::get_if<double>(raw_frequency) : nullptr;
    const auto* raw_levels = find_value(node, graph_keys::kTurbulenceLevels);
    const auto* levels = raw_levels ? std::get_if<std::uint32_t>(raw_levels) : nullptr;
    if (!origin || !amplitude || !frequency || !levels)
        return R::failure(ErrorCode::invalid_request, "Turbulence is missing a required Core value");
    result.origin = *origin;
    result.amplitude = *amplitude;
    result.frequency = *frequency;
    result.levels = *levels;
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceMode)) {
        const auto* value=std::get_if<std::uint32_t>(raw);
        if(!value || *value>static_cast<std::uint32_t>(TurbulenceMode::one_side_axis))
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence mode");
        result.mode=static_cast<TurbulenceMode>(*value);
        if(result.mode==TurbulenceMode::spherical || result.mode==TurbulenceMode::spherical2)
            return R::failure(ErrorCode::invalid_request,"Spherical Turbulence modes are not implemented yet");
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceAxis)) {
        const auto* value=std::get_if<std::uint32_t>(raw);
        if(!value || *value>3)return R::failure(ErrorCode::invalid_request,"invalid Turbulence axis");
        result.axis=*value;
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceProperties)) {
        const auto* value=std::get_if<std::uint32_t>(raw);
        if(!value || *value>1)return R::failure(ErrorCode::invalid_request,"invalid Turbulence affected properties");
        result.properties=static_cast<TurbulenceProperties>(*value);
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceAffect)) {
        const auto* value=std::get_if<std::uint32_t>(raw);
        if(!value || *value>static_cast<std::uint32_t>(TurbulenceAffect::feathered_sphere))
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence affect mode");
        result.affect=static_cast<TurbulenceAffect>(*value);
    }
    if (!bounded_turbulence_vector(result.origin) || !std::isfinite(result.amplitude) ||
        result.amplitude < 0.0 || result.amplitude > kMaxTurbulenceAmplitude ||
        !std::isfinite(result.frequency) || result.frequency <= 0.0 ||
        result.frequency > kMaxTurbulenceNoiseFrequency ||
        result.levels < kMinTurbulenceNoiseLevels || result.levels > kMaxTurbulenceNoiseLevels)
        return R::failure(ErrorCode::invalid_request, "Turbulence value is outside Core bounds");
    if (const auto* raw = find_value(node, graph_keys::kTurbulenceEvolutionSpeed)) {
        const auto* value = std::get_if<double>(raw);
        if (!value || !std::isfinite(*value) || std::abs(*value) > kMaxTurbulenceEvolutionSpeed)
            return R::failure(ErrorCode::invalid_request, "invalid Turbulence evolution speed");
        result.evolution_speed = *value;
    }
    if (const auto* raw = find_value(node, graph_keys::kTurbulenceOffset)) {
        const auto* value = std::get_if<Vec3>(raw);
        if (!value || !bounded_turbulence_vector(*value))
            return R::failure(ErrorCode::invalid_request, "invalid Turbulence offset");
        result.offset = *value;
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceSphereSize)) {
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || *value<=0.0 || *value>kMaxTurbulenceNoiseCoordinate)
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence sphere size");
        result.sphere_size=*value;
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceSphereFeather)) {
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || *value<0.0 || *value>100.0)
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence sphere feather");
        result.sphere_feather_percent=*value;
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceRandomize)) {
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || *value<0.0 || *value>100.0)
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence randomize amount");
        result.randomize_percent=*value;
    }
    const auto read_offset=[&](ParameterKey key,double& target) {
        const auto* raw=find_value(node,key);if(!raw)return true;
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || *value < -100.0 || *value > 100.0)return false;
        target=*value;return true;
    };
    if(!read_offset(graph_keys::kTurbulencePositionOffset,result.position_offset) ||
       !read_offset(graph_keys::kTurbulenceScaleOffset,result.scale_offset) ||
       !read_offset(graph_keys::kTurbulenceColorOffset,result.color_offset) ||
       !read_offset(graph_keys::kTurbulenceOpacityOffset,result.opacity_offset) ||
       !read_offset(graph_keys::kTurbulenceRotationOffset,result.rotation_offset))
        return R::failure(ErrorCode::invalid_request,"invalid Turbulence affected-property offset");
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceNoiseScale)) {
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || *value<=0.0 || *value>100000.0)
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence noise scale");
        result.noise_scale=*value;
    }
    if(const auto* raw=find_value(node,graph_keys::kTurbulenceEvolution)) {
        const auto* value=std::get_if<double>(raw);
        if(!value || !std::isfinite(*value) || std::abs(*value)>100000.0)
            return R::failure(ErrorCode::invalid_request,"invalid Turbulence evolution");
        result.evolution=*value;
    }
    if (const auto* raw = find_value(node, graph_keys::kTurbulenceOverLife)) {
        const auto* bytes = std::get_if<OpaqueBytes>(raw);
        if (!bytes || !decode_age_curve(*bytes, result.over_life, 0.0, 100.0))
            return R::failure(ErrorCode::invalid_request, "invalid Turbulence Over Life curve");
    }
    return R::success(result);
}

inline Result<bool> apply_turbulence(ParticleInstance& particle, const TurbulenceValues& values,
    NodeId node, std::uint32_t seed) noexcept {
    using R = Result<bool>;
    if (!bounded_turbulence_vector(particle.position) || !std::isfinite(particle.age_seconds) ||
        !std::isfinite(particle.lifetime_seconds) || particle.age_seconds < 0.0 ||
        particle.lifetime_seconds <= 0.0)
        return R::failure(ErrorCode::invalid_request, "invalid Turbulence particle state");
    const double life = std::clamp(particle.age_seconds / particle.lifetime_seconds, 0.0, 1.0);
    double weight = std::clamp(evaluate_age_curve(values.over_life, life, 100.0, 100.0) / 100.0, 0.0, 1.0);
    if(values.affect==TurbulenceAffect::feathered_sphere) {
        const double dx=particle.position.x-values.origin.x,dy=particle.position.y-values.origin.y,
            dz=particle.position.z-values.origin.z;
        const double distance=std::sqrt(dx*dx+dy*dy+dz*dz);
        if(!std::isfinite(distance))return R::failure(ErrorCode::invalid_request,"invalid Turbulence sphere distance");
        if(distance>=values.sphere_size)return R::success(true);
        const double feather=values.sphere_feather_percent/100.0;
        if(feather>0.0) {
            const double inner=values.sphere_size*(1.0-feather);
            if(distance>inner) {
                const double t=std::clamp((values.sphere_size-distance)/(values.sphere_size-inner),0.0,1.0);
                weight*=t*t*(3.0-2.0*t);
            }
        }
    }
    weight=std::clamp(weight,0.0,1.0);
    if (weight == 0.0) return R::success(true);

    std::uint64_t identity = mix64(static_cast<std::uint64_t>(seed) ^ particle.id);
    for (const auto byte : node.value.bytes) identity = mix64(identity ^ byte);
    if(values.randomize_percent>0.0) {
        const auto random=mix64(identity^0x74757262756c656eull);
        const double unit=static_cast<double>(random>>11)*(1.0/9007199254740992.0);
        weight*=1.0-(values.randomize_percent/100.0)*unit;
    }
    if(weight==0.0)return R::success(true);

    const double drift = values.evolution_speed * particle.age_seconds;
    const double coordinate_scale=100.0/values.noise_scale;
    const Vec3 coordinate{
        (particle.position.x - values.origin.x + values.offset.x)*coordinate_scale + drift,
        (particle.position.y - values.origin.y + values.offset.y)*coordinate_scale + drift,
        (particle.position.z - values.origin.z + values.offset.z)*coordinate_scale + drift + values.evolution/360.0};
    if (!bounded_turbulence_vector(coordinate))
        return R::failure(ErrorCode::invalid_request, "Turbulence coordinate exceeds Core bounds");
    const auto noise_seed = static_cast<std::uint32_t>(identity ^ (identity >> 32));
    Vec3 noise{};
    if (!sample_turbulence_noise3(coordinate, noise_seed, values.levels, values.frequency, noise))
        return R::failure(ErrorCode::invalid_request, "Turbulence noise sample rejected");
    Vec3 displacement=noise;
    if(values.mode==TurbulenceMode::axis || values.mode==TurbulenceMode::one_side_axis) {
        const double axis_noise=values.mode==TurbulenceMode::one_side_axis?std::max(0.0,noise.x):noise.x;
        displacement={};
        if(values.axis==0)displacement.x=axis_noise;
        else if(values.axis==1)displacement.y=axis_noise;
        else if(values.axis==2)displacement.z=axis_noise;
        else {
            const double speed=std::sqrt(particle.velocity.x*particle.velocity.x+
                particle.velocity.y*particle.velocity.y+particle.velocity.z*particle.velocity.z);
            const Vec3 direction=speed>1e-12?Vec3{particle.velocity.x/speed,particle.velocity.y/speed,particle.velocity.z/speed}:Vec3{0,0,1};
            displacement={direction.x*axis_noise,direction.y*axis_noise,direction.z*axis_noise};
        }
    } else if(values.mode!=TurbulenceMode::normal) {
        return R::failure(ErrorCode::invalid_request,"Spherical Turbulence modes are not implemented yet");
    }
    displacement.x*=weight;displacement.y*=weight;displacement.z*=weight;
    particle.turbulence_noise=displacement;
    if(values.properties==TurbulenceProperties::noise)return R::success(true);
    const double position_amount=values.amplitude*values.position_offset/100.0;
    const Vec3 position{particle.position.x+displacement.x*position_amount,
        particle.position.y+displacement.y*position_amount,
        particle.position.z+displacement.z*position_amount};
    if (!bounded_turbulence_vector(position))
        return R::failure(ErrorCode::invalid_request, "Turbulence displacement exceeds Core bounds");
    particle.position = position;
    const double scale=std::max(0.0,1.0+displacement.x*values.scale_offset/100.0);
    particle.size_pixels*=scale;particle.size_y_pixels*=scale;
    particle.color={std::clamp(particle.color.x+displacement.x*values.color_offset/100.0,0.0,1.0),
        std::clamp(particle.color.y+displacement.y*values.color_offset/100.0,0.0,1.0),
        std::clamp(particle.color.z+displacement.z*values.color_offset/100.0,0.0,1.0)};
    particle.opacity=std::clamp(particle.opacity+displacement.x*values.opacity_offset/100.0,0.0,1.0);
    particle.rotation_degrees.x+=displacement.x*values.rotation_offset;
    particle.rotation_degrees.y+=displacement.y*values.rotation_offset;
    particle.rotation_degrees.z+=displacement.z*values.rotation_offset;
    if(!std::isfinite(particle.size_pixels)||!std::isfinite(particle.size_y_pixels)||
       !std::isfinite(particle.opacity)||!bounded_turbulence_vector(particle.color)||
       !bounded_turbulence_vector(particle.rotation_degrees))
        return R::failure(ErrorCode::invalid_request,"Turbulence produced invalid particle properties");
    return R::success(true);
}

