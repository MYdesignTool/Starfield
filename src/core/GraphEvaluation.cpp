#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/ColorGradient.hpp"
#include "starfield/core/Random.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <functional>
#include <iterator>
#include <map>
#include <new>
#include <optional>
#include <queue>
#include <utility>

namespace starfield::core {
namespace {

using namespace graph_keys;

struct ForceValues {
    Vec3 gravity{};
    double linear_drag{0.0};
    ForceMotion motion{};
};

ForceMotion motion_for_emitter(const ForceMotion& force, NodeId emitter) noexcept {
    ForceMotion result = force;
    std::uint64_t salt = result.random_salt;
    for (auto byte : emitter.value.bytes) salt = mix64(salt ^ byte);
    result.random_salt = static_cast<std::uint32_t>(salt);
    return result;
}

struct ParticleValues {
    Vec3 color_start{1.0, 1.0, 1.0};
    Vec3 color_end{1.0, 1.0, 1.0};
    double size_start{8.0};
    double size_end{100.0};
    double opacity_start{1.0};
    double opacity_end{100.0};
    double lifetime_seconds{2.0};
    double size_random_percent{0.0};
    double opacity_random_percent{0.0};
    AgeCurve size_curve{};
    AgeCurve opacity_curve{};
    std::uint32_t color_mode{1};
    ColorGradient gradient{white_gradient()};
    double life_random_percent{}, size_y{10}, feather_percent{}, angle_random_percent{}, speed_random_percent{};
    std::uint32_t shape{}, orient_to{}, up_axis{2};
    bool limit_to_2d{false};
    Vec3 angles{}, rotation_speed{};
    std::uint32_t random_limit{};
    double limit_angle{},anchor_x{50},anchor_y{50};
    AgeCurve rotation_curve{};
};

constexpr std::uint64_t kMaxBranchTraversalWork = 16'777'216;

const ParameterValue* find_value(const GraphNode& node, ParameterKey key) noexcept {
    const auto found = std::find_if(node.parameters.begin(), node.parameters.end(),
        [key](const NodeParameter& parameter) { return parameter.key == key; });
    return found == node.parameters.end() ? nullptr : &found->value;
}

Result<ValidatedSettings> read_emitter(const GraphNode& node) {
    // validate_graph has already checked required keys and variant alternatives.
    Settings settings;
    for (const NodeParameter& parameter : node.parameters) {
        switch (parameter.key.value) {
            case kBirthRate.value: settings.birth_rate = std::get<double>(parameter.value); break;
            case kSeed.value: settings.seed = std::get<std::uint32_t>(parameter.value); break;
            case kEmitterShape.value: {
                const auto shape = std::get<std::uint32_t>(parameter.value);
                if (shape >= kEmitterShapeCount) {
                    return Result<ValidatedSettings>::failure(ErrorCode::invalid_request, "invalid graph emitter shape");
                }
                settings.emitter_shape = static_cast<EmitterShape>(shape);
                break;
            }
            case kEmitterOrigin.value: settings.emitter_origin = std::get<Vec3>(parameter.value); break;
            case kVelocity.value: settings.velocity = std::get<Vec3>(parameter.value); break;
            case kEmissionSpeed.value: settings.emission_speed = std::get<double>(parameter.value); break;
            case kEmissionSpeedRandom.value:
                settings.emission_speed_random = std::get<double>(parameter.value);
                break;
            case kEmissionAngleX.value:
                settings.emitter_shape_angles_degrees.x = std::get<double>(parameter.value);
                break;
            case kEmissionAngleY.value:
                settings.emitter_shape_angles_degrees.y = std::get<double>(parameter.value);
                break;
            case kEmissionAngleZ.value:
                settings.emitter_shape_angles_degrees.z = std::get<double>(parameter.value);
                break;
            case kEmitterOrient.value: settings.emission_angles_degrees=std::get<Vec3>(parameter.value);break;
            case kDirectionMode.value: {
                const auto mode = std::get<std::uint32_t>(parameter.value);
                if (mode > static_cast<std::uint32_t>(DirectionMode::uniform)) {
                    return Result<ValidatedSettings>::failure(ErrorCode::invalid_request,
                                                              "invalid graph direction mode");
                }
                settings.direction_mode = static_cast<DirectionMode>(mode);
                break;
            }
            case kDirectionSpan.value: settings.direction_span_degrees = std::get<double>(parameter.value); break;
            case kEmitterSizeX.value: settings.emitter_size_pixels.x = std::get<double>(parameter.value); break;
            case kEmitterSizeY.value: settings.emitter_size_pixels.y = std::get<double>(parameter.value); break;
            case kEmitterSizeZ.value: settings.emitter_size_pixels.z = std::get<double>(parameter.value); break;
            case kParticleSize.value:
                settings.particle_size = std::get<double>(parameter.value);
                break;
            case kOpacity.value:
                settings.opacity = std::get<double>(parameter.value);
                break;
            case kEmitterSize.value: settings.emitter_size = std::get<double>(parameter.value); break;
            case kVelocitySpread.value: settings.velocity_spread = std::get<double>(parameter.value); break;
        }
    }
    if (const auto* random_percent = find_value(node, kEmissionSpeedRandomPercent)) {
        const double percent = std::get<double>(*random_percent);
        if (!std::isfinite(percent) || percent < 0.0 || percent > 100.0) {
            return Result<ValidatedSettings>::failure(ErrorCode::invalid_request,
                                                      "Speed Random percentage is outside 0..100");
        }
        settings.emission_speed_random = settings.emission_speed * percent / 100.0;
    }
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<ValidatedSettings>::failure(ErrorCode::invalid_request, "graph emitter value is outside supported bounds");
    }
    return Result<ValidatedSettings>::success(std::move(validated));
}

Result<ForceValues> read_force(const GraphNode& node) {
    const auto* gravity = find_value(node, kGravity);
    const auto* drag = find_value(node, kLinearDrag);
    if (!gravity || !drag) return Result<ForceValues>::failure(ErrorCode::invalid_request, "force node is missing values");
    Settings settings;
    settings.gravity = std::get<Vec3>(*gravity);
    settings.linear_drag = std::get<double>(*drag);
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<ForceValues>::failure(ErrorCode::invalid_request, "force node value is outside supported bounds");
    }
    ForceValues result{validated.value.gravity, validated.value.linear_drag};
    result.motion.gravity = result.gravity;
    auto scalar = [&](ParameterKey key, double maximum, double& destination) {
        if (const auto* value = find_value(node, key)) destination = std::get<double>(*value);
        return std::isfinite(destination) && destination >= 0 && destination <= maximum;
    };
    if (!scalar(kGravityRandom, 100, result.motion.gravity_random_percent) ||
        !scalar(kSpin, 100000, result.motion.spin_radius) ||
        !scalar(kSpinFrequency, 1000, result.motion.spin_frequency) ||
        !scalar(kSpinResist, 100, result.motion.spin_resist) ||
        !scalar(kSpinDelay, 10000, result.motion.spin_delay))
        return Result<ForceValues>::failure(ErrorCode::invalid_request, "Force scalar is outside supported bounds");
    if (const auto* wind = find_value(node, kWind)) result.motion.wind = std::get<Vec3>(*wind);
    for (double value : {result.motion.wind.x, result.motion.wind.y, result.motion.wind.z})
        if (!std::isfinite(value) || std::abs(value) > 100000)
            return Result<ForceValues>::failure(ErrorCode::invalid_request, "Wind is outside supported bounds");
    if (const auto* curve = find_value(node, kWindSpinCurve)) {
        if (!decode_age_curve(std::get<OpaqueBytes>(*curve), result.motion.wind_spin_curve, 0, 100))
            return Result<ForceValues>::failure(ErrorCode::invalid_request, "invalid Wind and Spin Over Life curve");
    }
    std::uint64_t salt = 0;
    for (auto byte : node.id.value.bytes) salt = mix64(salt ^ byte);
    result.motion.random_salt = static_cast<std::uint32_t>(salt);
    return Result<ForceValues>::success(std::move(result));
}

Result<ParticleValues> read_particle(const GraphNode& node) {
    const auto* color_start = find_value(node, kColorStart);
    const auto* color_end = find_value(node, kColorEnd);
    const auto* size_start = find_value(node, kSizeStart);
    const auto* size_end = find_value(node, kSizeEnd);
    const auto* opacity_start = find_value(node, kOpacityStart);
    const auto* opacity_end = find_value(node, kOpacityEnd);
    if (!color_start || !size_start || !size_end || !opacity_start || !opacity_end) {
        return Result<ParticleValues>::failure(ErrorCode::invalid_request, "Particle node is missing values");
    }
    Settings settings;
    settings.color_start = std::get<Vec3>(*color_start);
    settings.color_end = color_end?std::get<Vec3>(*color_end):settings.color_start;
    settings.particle_size = std::get<double>(*size_start);
    settings.particle_size_end = std::get<double>(*size_end);
    settings.opacity = std::get<double>(*opacity_start);
    settings.opacity_end = std::get<double>(*opacity_end);
    if (const auto* lifetime = find_value(node, kParticleLifetimeSeconds)) {
        settings.particle_lifetime_seconds = std::get<double>(*lifetime);
    }
    if (const auto* size_random = find_value(node, kSizeRandom)) {
        settings.particle_size_random_percent = std::get<double>(*size_random);
    }
    if (const auto* opacity_random = find_value(node, kOpacityRandom)) {
        settings.opacity_random_percent = std::get<double>(*opacity_random);
    }
    settings.appearance_enabled = true;
    const auto* size_curve = find_value(node, kSizeOverLifeCurve);
    const auto* opacity_curve = find_value(node, kOpacityOverLifeCurve);
    if (size_curve) {
        const auto* bytes = std::get_if<OpaqueBytes>(size_curve);
        if (!bytes || !decode_age_curve(*bytes, settings.size_over_life, 0.0, 100.0)) {
            return Result<ParticleValues>::failure(ErrorCode::invalid_request, "Particle size curve is invalid");
        }
    }
    if (opacity_curve) {
        const auto* bytes = std::get_if<OpaqueBytes>(opacity_curve);
        if (!bytes || !decode_age_curve(*bytes, settings.opacity_over_life, 0.0, 100.0)) {
            return Result<ParticleValues>::failure(ErrorCode::invalid_request, "Particle opacity curve is invalid");
        }
    }
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<ParticleValues>::failure(ErrorCode::invalid_request, "Particle node value is outside supported bounds");
    }
    ParticleValues result{
        validated.value.color_start, validated.value.color_end,
        validated.value.particle_size, validated.value.particle_size_end,
        validated.value.opacity, validated.value.opacity_end,
        validated.value.particle_lifetime_seconds,
        validated.value.particle_size_random_percent, validated.value.opacity_random_percent,
        validated.value.size_over_life, validated.value.opacity_over_life};
    result.gradient.stops[0]={0,result.color_start};result.gradient.stops[1]={1,result.color_end};
    if(const auto* mode=find_value(node,kParticleColorMode)) {
        result.color_mode=std::get<std::uint32_t>(*mode);
        if(result.color_mode>3) return Result<ParticleValues>::failure(ErrorCode::invalid_request,"invalid Particle Color mode");
    }
    if(const auto* gradient=find_value(node,kColorGradient))
        if(!decode_color_gradient(std::get<OpaqueBytes>(*gradient),result.gradient))
            return Result<ParticleValues>::failure(ErrorCode::invalid_request,"invalid Color Gradient");
    if(node.type_key==kParticleNode) {
        const auto scalar=[&](ParameterKey key,double maximum,double& value) {
            if(const auto* v=find_value(node,key)) value=std::get<double>(*v);
            return std::isfinite(value) && value>=0 && value<=maximum;
        };
        const auto enumeration=[&](ParameterKey key,std::uint32_t maximum,std::uint32_t& value) {
            if(const auto* v=find_value(node,key)) value=std::get<std::uint32_t>(*v);
            return value<=maximum;
        };
        std::uint32_t limit=0;
        if(!scalar(kLifeRandom,100,result.life_random_percent) || !scalar(kSizeY,100000,result.size_y) ||
           !scalar(kParticleFeather,100,result.feather_percent) || !scalar(kAngleRandom,100,result.angle_random_percent) ||
           !scalar(kRotationSpeedRandom,100,result.speed_random_percent) || !enumeration(kParticleShape,2,result.shape) ||
           !enumeration(kOrientTo,2,result.orient_to) || !enumeration(kUpAxis,2,result.up_axis) || !enumeration(kLimitTo2D,1,limit))
            return Result<ParticleValues>::failure(ErrorCode::invalid_request,"Particle property outside bounds");
        result.limit_to_2d=limit!=0;
        if(!scalar(kAnchorX,100,result.anchor_x) || !scalar(kAnchorY,100,result.anchor_y) ||
           !enumeration(kRandomLimit,4,result.random_limit))
            return Result<ParticleValues>::failure(ErrorCode::invalid_request,"Particle anchor/random limit outside bounds");
        if(const auto* v=find_value(node,kLimitAngle))result.limit_angle=std::get<double>(*v);
        if(!std::isfinite(result.limit_angle) || std::abs(result.limit_angle)>32768)
            return Result<ParticleValues>::failure(ErrorCode::invalid_request,"Limit Angle outside native Angle range");
        if(const auto* v=find_value(node,kRotationOverLife))
            if(!decode_age_curve(std::get<OpaqueBytes>(*v),result.rotation_curve,-32768,32768))
                return Result<ParticleValues>::failure(ErrorCode::invalid_request,"Invalid Rotation Over Life curve");
        for(auto [key,value]:{std::pair{kParticleAngles,&result.angles},std::pair{kRotationSpeed,&result.rotation_speed}}) {
            if(const auto* v=find_value(node,key)) *value=std::get<Vec3>(*v);
            for(double axis:{value->x,value->y,value->z}) if(!std::isfinite(axis) || axis < -32768 || axis > 32768)
                return Result<ParticleValues>::failure(ErrorCode::invalid_request,"Particle angle outside native Angle range");
        }
    }
    return Result<ParticleValues>::success(std::move(result));
}

double birth_lifetime(const ParticleValues& values,std::uint32_t seed,std::uint64_t identity) noexcept {
    return values.lifetime_seconds*(1-values.life_random_percent/100*unit_value(seed,identity,RandomPurpose::particle_life));
}
void apply_particle_properties(ParticleInstance& particle,const ParticleValues& values,
    std::uint32_t seed,Vec3 birth_position,const CompiledParticleTransform* transform = nullptr) noexcept {
    particle.shape=values.shape;particle.up_axis=values.up_axis;particle.limit_to_2d=values.limit_to_2d;
    particle.feather_percent=values.feather_percent;
    particle.anchor_x_percent=values.anchor_x;particle.anchor_y_percent=values.anchor_y;
    particle.size_y_pixels=values.size_start>0?particle.size_pixels*values.size_y/values.size_start:0;
    const double spin_scale=1-values.speed_random_percent/100*unit_value(seed,particle.id,RandomPurpose::particle_spin);
    const auto angle_offset=[&](unsigned axis) {
        const bool limited=values.random_limit==1 || values.random_limit==axis+2;
        const double amplitude=limited?std::min(180*values.angle_random_percent/100,std::abs(values.limit_angle)):180*values.angle_random_percent/100;
        const auto salt=axis==2?0U:(axis+1)*0x9e3779b9U;
        return (2*unit_value(seed^salt,particle.id,RandomPurpose::particle_angle)-1)*amplitude;
    };
    const double age_fraction=particle.lifetime_seconds>0?particle.age_seconds/particle.lifetime_seconds:0;
    particle.rotation_degrees={values.angles.x+values.rotation_speed.x*particle.age_seconds*spin_scale,
        values.angles.y+values.rotation_speed.y*particle.age_seconds*spin_scale,
        values.angles.z+values.rotation_speed.z*particle.age_seconds*spin_scale};
    particle.rotation_degrees.x+=angle_offset(0);particle.rotation_degrees.y+=angle_offset(1);
    particle.rotation_degrees.z+=angle_offset(2)+evaluate_age_curve(values.rotation_curve,age_fraction,0,0);
    if(values.orient_to) {
        auto direction=values.orient_to==1?particle.velocity:Vec3{birth_position.x-particle.position.x,
            birth_position.y-particle.position.y,birth_position.z-particle.position.z};
        if(transform)direction=transform->unmap_particle_axis(direction);
        const double planar=std::hypot(direction.x,direction.y);
        if(planar>1e-12 || std::abs(direction.z)>1e-12) {
            constexpr double degrees=180/3.14159265358979323846;
            particle.rotation_degrees.z+=std::atan2(-direction.y,direction.x)*degrees;
            if(!values.limit_to_2d) particle.rotation_degrees.y+=std::atan2(-direction.z,planar)*degrees;
        }
    }
}

void apply_particle_style(ParticleInstance& particle, const ParticleValues& appearance,
                      std::uint32_t seed) noexcept {
    const double age_fraction = particle.lifetime_seconds > 0.0
        ? std::clamp(particle.age_seconds / particle.lifetime_seconds, 0.0, 1.0) : 0.0;
    const double size_percent = evaluate_age_curve(appearance.size_curve, age_fraction,
                                                   100.0, appearance.size_end);
    const double opacity_percent = evaluate_age_curve(appearance.opacity_curve, age_fraction,
                                                      100.0, appearance.opacity_end);
    particle.size_pixels = appearance.size_start * (size_percent / 100.0);
    particle.opacity = appearance.opacity_start * (opacity_percent / 100.0);
    particle.size_pixels *= 1.0 - (appearance.size_random_percent / 100.0) *
        unit_value(seed, particle.id, RandomPurpose::size);
    particle.opacity *= 1.0 - (appearance.opacity_random_percent / 100.0) *
        unit_value(seed, particle.id, RandomPurpose::opacity);
    // An independent random stream keeps gradient sampling stable across frames.
    const double random=unit_value(seed,particle.id,RandomPurpose::particle_color);
    double location=age_fraction;
    if(appearance.color_mode==2) location=random;
    if(appearance.color_mode==3) location=std::fmod(random+age_fraction,1.0);
    particle.color=appearance.color_mode==0?appearance.color_start:evaluate_color_gradient(appearance.gradient,location);
}

#include "TransformEvaluation.hpp"

bool is_particle_graph_edge(const GraphNode& source, const GraphNode& destination) noexcept {
    const auto& from = source.type_key;
    const auto& to = destination.type_key;
    if (from == kEmitterNode) return to == kParticleNode;
    if (to == kEmitterNode) return from == kParticleNode || from == kForceNode || from == kTransformNode;
    if (from == kParticleNode || from == kForceNode || from == kTransformNode)
        return to == kForceNode || to == kTransformNode || to == kOutputNode;
    return false;
}

} // namespace


struct EvaluationBudget { std::uint64_t work{0};TransformPlanningBudget transform; };
static Result<EvaluatedGraph> evaluate_graph_impl(const Graph& graph, RationalTime time,
                                               const Cancellation& cancellation,
                                               EmitterDimensionContext dimension_context,
                                               EvaluationBudget& budget, unsigned depth, EmitterOriginSampler* origins) {
    using R = Result<EvaluatedGraph>;
    if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph evaluation cancelled");
    budget.work += graph.nodes.size() + graph.edges.size();
    if (depth > 16 || budget.work > 20'000'000) return R::failure(ErrorCode::work_limit_exceeded, "auxiliary graph evaluation limit exceeded");
    if (!std::isfinite(dimension_context.layer_height_pixels) ||
        !(dimension_context.layer_height_pixels > 0.0) ||
        !std::isfinite(dimension_context.pixel_aspect_ratio) ||
        !(dimension_context.pixel_aspect_ratio > 0.0)) {
        return R::failure(ErrorCode::invalid_request, "invalid emitter dimension context");
    }
    const auto normalized = make_rational(time.value, time.scale);
    if (!normalized) return R::failure(ErrorCode::invalid_time, "invalid graph evaluation time");
    try {
        const auto validation = validate_graph(graph, particle_node_registry());
        if (!validation) {
            const auto code = validation.error.code == GraphErrorCode::allocation_failed ? ErrorCode::allocation_failed
                : validation.error.code == GraphErrorCode::internal_failure ? ErrorCode::internal_failure
                : ErrorCode::invalid_request;
            return R::failure(code, validation.error.detail);
        }
        if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph validation cancelled");

        std::vector<const GraphNode*> nodes;
        nodes.reserve(graph.nodes.size());
        for (const auto& node : graph.nodes) nodes.push_back(&node);
        std::sort(nodes.begin(), nodes.end(), [](const auto* a, const auto* b) { return a->id < b->id; });
        const auto index_of = [&nodes](NodeId id) {
            return static_cast<std::size_t>(std::lower_bound(nodes.begin(), nodes.end(), id,
                [](const auto* node, NodeId key) { return node->id < key; }) - nodes.begin());
        };

        const std::size_t count = nodes.size();
        std::size_t output = count;
        std::uint32_t output_particle_count = kDefaultParticleCount;
        std::vector<std::optional<ValidatedSettings>> emitters(count);
        std::vector<std::optional<ForceValues>> forces(count);
        std::vector<std::optional<ParticleValues>> particles(count);
        std::vector<std::optional<CompiledParticleTransform>> transforms(count);
        // Check semantic bounds on every node, including disconnected/parked nodes.
        for (std::size_t i = 0; i < count; ++i) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "node validation cancelled");
            const auto& node = *nodes[i];
            if (node.type_key == kOutputNode) {
                if (output != count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");
                output = i;
                const auto* particle_count = find_value(node, kParticleCount);
                if (!particle_count) {
                    return R::failure(ErrorCode::invalid_request, "output node is missing the Max Particles value");
                }
                Settings output_settings;
                output_settings.particle_count = std::get<std::uint32_t>(*particle_count);
                auto validated_output = validate_settings(output_settings);
                if (!validated_output.notices.empty()) {
                    return R::failure(ErrorCode::invalid_request, "output Max Particles is outside supported bounds");
                }
                output_particle_count = validated_output.value.particle_count;
                for (auto key : {kTimeRemapEnabled, kPreviewEnabled})
                    if (const auto* flag = find_value(node, key); flag && std::get<std::uint32_t>(*flag) > 1)
                        return R::failure(ErrorCode::invalid_request, "invalid renderer enable switch");
                if (const auto* clock = find_value(node, kTimeRemapSeconds); clock &&
                    (!std::isfinite(std::get<double>(*clock)) || std::abs(std::get<double>(*clock)) > 1000000))
                    return R::failure(ErrorCode::invalid_time, "Time Remapping is outside supported bounds");
                if (const auto* chance = find_value(node, kPreviewChance); chance &&
                    (!std::isfinite(std::get<double>(*chance)) || std::get<double>(*chance) < 0 || std::get<double>(*chance) > 100))
                    return R::failure(ErrorCode::invalid_request, "Particle chance must be 0..100 percent");
            } else if (node.type_key == kEmitterNode) {
                if (const auto* mode = find_value(node, kEmittingMode); mode && std::get<std::uint32_t>(*mode) > 3)
                    return R::failure(ErrorCode::invalid_request, "invalid Emitting mode");
                for (const auto key : {kEmitChance, kEmitLifeStart, kEmitLifeEnd, kInheritVelocity, kInheritSize, kInheritOpacity, kInheritColor}) {
                    if (const auto* value = find_value(node, key); value && (std::get<double>(*value) < 0 || std::get<double>(*value) > 100))
                        return R::failure(ErrorCode::invalid_request, "auxiliary percentage outside 0..100");
                }
                auto value = read_emitter(node);
                if (!value.has_value()) return R::failure(value.error());
                emitters[i] = value.take_value();
            } else if (node.type_key == kParticleNode) {
                auto value = read_particle(node);
                if (!value.has_value()) return R::failure(value.error());
                particles[i] = value.take_value();
            } else if (node.type_key == kForceNode) {
                auto value = read_force(node);
                if (!value.has_value()) return R::failure(value.error());
                forces[i] = value.take_value();
            } else if(node.type_key==kTransformNode) {
                auto value=read_transform(node);if(!value.has_value())return R::failure(value.error());
                transforms[i]=value.take_value();
            } else {
                return R::failure(ErrorCode::invalid_request, "node has no evaluation kernel");
            }
        }
        if (output == count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");

        std::vector<std::vector<std::size_t>> incoming(count), outgoing(count);
        for (const auto& edge : graph.edges) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph planning cancelled");
            const auto source = index_of(edge.source_node);
            const auto destination = index_of(edge.destination_node);
            incoming[destination].push_back(source);
            outgoing[source].push_back(destination);
        }
        for (auto& list : incoming) std::sort(list.begin(), list.end());
        for (auto& list : outgoing) std::sort(list.begin(), list.end());
        std::vector<bool> active(count, false);
        std::vector<std::size_t> pending{output};
        active[output] = true;
        while (!pending.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph reachability cancelled");
            const auto index = pending.back();
            pending.pop_back();
            for (const auto source : incoming[index]) {
                if (!active[source]) { active[source] = true; pending.push_back(source); }
            }
        }

        std::vector<std::size_t> degrees(count, 0);
        std::priority_queue<std::size_t, std::vector<std::size_t>, std::greater<>> ready;
        std::size_t active_count = 0;
        for (std::size_t i = 0; i < count; ++i) {
            if (!active[i]) continue;
            ++active_count;
            degrees[i] = incoming[i].size();
            if (degrees[i] == 0) ready.push(i);
        }

        std::vector<std::size_t> topological_order;
        topological_order.reserve(active_count);
        std::size_t active_emitter_count = 0;
        std::vector<std::size_t> active_particles;
        while (!ready.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph execution cancelled");
            const auto index = ready.top();
            ready.pop();
            const auto& node = *nodes[index];
            topological_order.push_back(index);
            if (node.type_key == kEmitterNode) {
                ++active_emitter_count;
            } else if (node.type_key == kParticleNode) {
                active_particles.push_back(index);
            }
            for (const auto destination : outgoing[index]) {
                if (active[destination] && --degrees[destination] == 0) ready.push(destination);
            }
        }
        if (topological_order.size() != active_count) {
            return R::failure(ErrorCode::invalid_request, "graph cannot be evaluated in dependency order");
        }
        EvaluatedGraph result;
        result.evaluated_nodes.reserve(active_count);
        for (const std::size_t index : topological_order) result.evaluated_nodes.push_back(nodes[index]->id);
        const bool has_transforms=std::any_of(transforms.begin(),transforms.end(),[](const auto& value){return value.has_value();});
        std::vector<std::shared_ptr<const TransformBranchPlan>> transform_plans(count);
        const auto transform_plan=[&](std::size_t particle)->Result<std::shared_ptr<const TransformBranchPlan>> {
            using P=Result<std::shared_ptr<const TransformBranchPlan>>;
            if(!has_transforms)return P::success({});
            if(transform_plans[particle])return P::success(transform_plans[particle]);
            auto planned=plan_transform_branch(nodes,incoming,outgoing,active,topological_order,particle,output,transforms,cancellation,budget.transform);
            if(!planned.has_value())return P::failure(planned.error());
            auto value=planned.take_value();auto basis=retain_transform_basis(result,value);
            if(!basis.has_value())return P::failure(basis.error());value.sprite_basis_index=basis.value();
            transform_plans[particle]=std::make_shared<const TransformBranchPlan>(std::move(value));
            return P::success(transform_plans[particle]);
        };

        // A topology edit can temporarily leave the Output ancestry without an
        // emitter (for example, while a user disconnects and rewires a chain).
        // Keep the graph valid and render transparent until a complete source
        // path is connected again. Structural/type errors are still rejected by
        // validate_graph above.
        if (active_emitter_count == 0) return R::success(std::move(result));

        // Apply stage rules before either primary or Auxiliary evaluation. An
        // auxiliary prefix must not make an invalid active bypass disappear.
        for (const auto& edge : graph.edges) {
            const std::size_t source = index_of(edge.source_node);
            const std::size_t destination = index_of(edge.destination_node);
            if (active[source] && active[destination] &&
                !is_particle_graph_edge(*nodes[source], *nodes[destination])) {
                return R::failure(ErrorCode::invalid_request, "invalid active Particle graph connection");
            }
        }

        const auto auxiliary = [&](std::size_t index) {
            const auto* mode = find_value(*nodes[index], kAuxiliarySource);
            return mode && std::get<std::uint32_t>(*mode) == 1;
        };
        bool has_auxiliary = false;
        for (const auto index : topological_order) if (emitters[index]) {
            if (!auxiliary(index) && !incoming[index].empty()) return R::failure(ErrorCode::invalid_request, "a parent input requires an Auxiliary source");
            has_auxiliary = has_auxiliary || auxiliary(index);
        }
        if (has_auxiliary) {
            // Parent graphs are restricted to each source's ancestors. Sampling
            // at child birth time preserves children after their parent dies.
            Graph roots = graph;
            roots.edges.erase(std::remove_if(roots.edges.begin(), roots.edges.end(), [&](const GraphEdge& edge) {
                const auto source = index_of(edge.source_node);
                return emitters[source] && auxiliary(source);
            }), roots.edges.end());
            auto primary = evaluate_graph_impl(roots, time, cancellation, dimension_context, budget, depth + 1, origins);
            // Parked Auxiliary nodes remain ancestors through incoming edges in
            // a root-only graph only if their outputs are present (removed above).
            if (!primary.has_value()) return R::failure(primary.error());
            result.sprite_bases=std::move(primary.take_value().sprite_bases);
            struct Candidate { double birth; ParticleInstance particle; };
            const auto before = [](const Candidate& a, const Candidate& b) {
                if (a.birth != b.birth) return a.birth > b.birth; // oldest at heap top
                if (a.particle.emitter_id != b.particle.emitter_id) return a.particle.emitter_id > b.particle.emitter_id;
                return a.particle.id > b.particle.id;
            };
            std::vector<Candidate> storage; storage.reserve(std::min<std::uint32_t>(output_particle_count, 4096));
            std::priority_queue<Candidate, std::vector<Candidate>, decltype(before)> kept(before, std::move(storage));
            const double now = to_seconds(*normalized);
            auto retain = [&](ParticleInstance instance, double birth) {
                if (output_particle_count == 0) return;
                Candidate candidate{birth, std::move(instance)};
                if (kept.size() < output_particle_count) kept.push(std::move(candidate));
                else if (before(candidate, kept.top())) { kept.pop(); kept.push(std::move(candidate)); }
            };
            for (auto& instance : primary.value().particles) retain(std::move(instance), now - instance.age_seconds);
            for (const auto emitter : topological_order) {
                if (!emitters[emitter] || !auxiliary(emitter) || incoming[emitter].empty() || output_particle_count == 0) continue;
                const auto percent = [&](ParameterKey key, double initial) {
                    const auto* value = find_value(*nodes[emitter], key); return value ? std::get<double>(*value) / 100.0 : initial;
                };
                const double chance = percent(kEmitChance, 1), start = percent(kEmitLifeStart, 0), end = percent(kEmitLifeEnd, 1);
                if (start > end) return R::failure(ErrorCode::invalid_request, "Emit Life Start exceeds Emit Life End");
                if (chance == 0 || start == end || emitters[emitter]->value.birth_rate == 0) continue;
                std::vector<std::size_t> children;
                for (const auto child : outgoing[emitter]) if (active[child] && particles[child]) children.push_back(child);
                std::sort(children.begin(), children.end());
                if (children.empty()) continue;
                // Reuse edge identities for terminal links, after pruning the
                // original parent->Auxiliary edges out of this prefix graph.
                std::vector<bool> ancestors(count, false);
                std::vector<std::size_t> stack = incoming[emitter];
                while (!stack.empty()) {
                    const auto current = stack.back(); stack.pop_back();
                    if (ancestors[current]) continue;
                    ancestors[current] = true;
                    for (const auto source : incoming[current]) stack.push_back(source);
                }
                Graph parent_graph;
                for (std::size_t n = 0; n < count; ++n) if (ancestors[n] || n == output) parent_graph.nodes.push_back(*nodes[n]);
                for (const auto& edge : graph.edges) {
                    const auto source = index_of(edge.source_node), destination = index_of(edge.destination_node);
                    if (ancestors[source] && ancestors[destination]) parent_graph.edges.push_back(edge);
                    else if (destination == emitter) parent_graph.edges.push_back({edge.id, edge.source_node, edge.source_port, nodes[output]->id, kOutputParticles});
                }
                for (std::size_t branch = 0; branch < children.size(); ++branch) {
                    const auto child = children[branch];
                    auto planned=transform_plan(child);if(!planned.has_value())return R::failure(planned.error());
                    const auto tf_plan=planned.value();
                    if(tf_plan && !tf_plan->terminal)continue;
                    const auto* tf=tf_plan && tf_plan->combined?&*tf_plan->combined:nullptr;
                    Settings settings = emitters[emitter]->value;
                    settings.particle_count = output_particle_count;
                    settings.particle_lifetime_seconds = particles[child]->lifetime_seconds;
                    ParticleValues appearance = *particles[child];
                    std::vector<bool> visited(count, false); stack = {child};
                    std::vector<NodeId> force_ids;
                    while (!stack.empty()) {
                        const auto current = stack.back(); stack.pop_back();
                        if (visited[current]) continue; visited[current] = true;
                        if (forces[current] && (!tf_plan || std::find(tf_plan->force_nodes.begin(),tf_plan->force_nodes.end(),nodes[current]->id)!=tf_plan->force_nodes.end())) { settings.gravity.x += forces[current]->gravity.x; settings.gravity.y += forces[current]->gravity.y; settings.gravity.z += forces[current]->gravity.z; settings.linear_drag += forces[current]->linear_drag; settings.forces.push_back(motion_for_emitter(forces[current]->motion, nodes[emitter]->id));force_ids.push_back(nodes[current]->id); }
                        for (const auto destination : outgoing[current]) if (active[destination] && !emitters[destination]) stack.push_back(destination);
                    }
                    auto validated_settings = validate_settings(settings);
                    if (!validated_settings.notices.empty()) return R::failure(ErrorCode::invalid_request, "auxiliary force values outside bounds");
                    if(tf_plan)map_transform_forces(validated_settings.value,*tf_plan,force_ids,emitters[emitter]->value.gravity);
                    if (now < 0 || settings.particle_lifetime_seconds <= 0) continue;
                    // Count clocks, not children: low chance or an empty parent
                    // interval must not prematurely truncate the candidate window.
                    const double last_tick = std::floor(now * settings.birth_rate);
                    const double first_tick = std::max(0.0, std::floor((now-settings.particle_lifetime_seconds)*settings.birth_rate)+1.0);
                    if (!std::isfinite(last_tick) || last_tick > 9e15) return R::failure(ErrorCode::invalid_time,"auxiliary clock exceeds exact slot range");
                    const auto stride = static_cast<std::uint64_t>(children.size());
                    const auto first = static_cast<std::uint64_t>(first_tick);
                    const auto last = static_cast<std::uint64_t>(last_tick);
                    const auto aligned_first = first + (branch + stride - first % stride) % stride;
                    const auto clock_count = aligned_first <= last ? (last-aligned_first)/stride+1 : 0;
                    for (std::uint64_t remaining = clock_count; remaining > 0; --remaining) {
                        if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "auxiliary emission cancelled");
                        const auto slot = aligned_first + (remaining - 1) * stride;
                        const double birth = double(slot) / settings.birth_rate;
                        if (kept.size() == output_particle_count && birth < kept.top().birth) break;
                        if (birth > 9e9) return R::failure(ErrorCode::invalid_time, "auxiliary birth exceeds time range");
                        const RationalTime birth_time{static_cast<std::int64_t>(std::llround(birth * 1e9)), 1'000'000'000};
                        auto parents = evaluate_graph_impl(parent_graph, birth_time, cancellation, dimension_context, budget, depth + 1, origins);
                        if (!parents.has_value()) return R::failure(parents.error());
                        for (const auto& parent : parents.value().particles) {
                            if (++budget.work > 20'000'000) return R::failure(ErrorCode::work_limit_exceeded, "auxiliary population evaluation limit exceeded");
                            const double age_fraction = parent.lifetime_seconds > 0 ? parent.age_seconds / parent.lifetime_seconds : 1;
                            if (age_fraction < start || age_fraction >= end) continue;
                            std::uint64_t parent_key = mix64(parent.id);
                            for (const auto byte : parent.emitter_id.value.bytes) parent_key = mix64(parent_key ^ byte);
                            if (unit_value(settings.seed, parent_key, RandomPurpose::auxiliary_chance) >= chance) continue;
                            const auto child_id = mix64(parent_key ^ mix64(slot));
                            auto child_settings = validated_settings;
                            child_settings.value.seed = static_cast<std::uint32_t>(mix64(child_id ^ settings.seed));
                            // Translate after the kernel; world positions are not
                            // clamped to the authored Origin slider's UI bounds.
                            child_settings.value.emitter_origin = {};
                            ParticleInstance instance;
                            const ParticleSlotTarget target{0, 0};
                            const auto simulated = simulate_selected_particles_into(child_settings, now - birth, {&target, 1}, {&instance, 1}, cancellation, dimension_context,tf);
                            if (!simulated.has_value()) return R::failure(simulated.error());
                            instance.id = child_id; instance.emitter_id = nodes[emitter]->id;
                            auto inherited = appearance;
                            const auto blend = [](double own, double source, double amount) { return own + (source - own) * amount; };
                            inherited.size_start = blend(inherited.size_start, parent.size_pixels, percent(kInheritSize, 0));
                            inherited.opacity_start = blend(inherited.opacity_start, parent.opacity, percent(kInheritOpacity, 0));
                            const double color = percent(kInheritColor, 0);
                            inherited.color_start = {blend(inherited.color_start.x, parent.color.x, color), blend(inherited.color_start.y, parent.color.y, color), blend(inherited.color_start.z, parent.color.z, color)};
                            inherited.color_end = {blend(inherited.color_end.x, parent.color.x, color), blend(inherited.color_end.y, parent.color.y, color), blend(inherited.color_end.z, parent.color.z, color)};
                            apply_particle_style(instance, inherited, child_settings.value.seed);

                            const double inherited_velocity = percent(kInheritVelocity, 0);
                            const double integral = settings.linear_drag > 0 ? -std::expm1(-settings.linear_drag * instance.age_seconds) / settings.linear_drag : instance.age_seconds;
                            const double decay = std::exp(-settings.linear_drag * instance.age_seconds);
                            Vec3 birth_origin=settings.emitter_origin;
                            if(origins) {
                                const auto sampled=origins->sample(nodes[emitter]->id,birth);
                                if(!sampled.has_value()) return R::failure(sampled.error());
                                birth_origin=sampled.value();
                            }
                            Vec3 origin{parent.position.x+birth_origin.x,parent.position.y+birth_origin.y,parent.position.z+birth_origin.z};
                            Vec3 velocity=parent.velocity;
                            const auto birth_position=tf?tf->position(origin):origin;
                            if(tf){origin=tf->velocity(origin);velocity=tf->velocity(velocity);}
                            instance.position.x += origin.x + velocity.x * inherited_velocity * integral;
                            instance.position.y += origin.y + velocity.y * inherited_velocity * integral;
                            instance.position.z += origin.z + velocity.z * inherited_velocity * integral;
                            instance.velocity.x += velocity.x * inherited_velocity * decay;
                            instance.velocity.y += velocity.y * inherited_velocity * decay;
                            instance.velocity.z += velocity.z * inherited_velocity * decay;
                            apply_particle_properties(instance,*particles[children[branch]],child_settings.value.seed,
                                birth_position,tf);
                            if(tf_plan)apply_transform_style(instance,*tf_plan);
                            retain(std::move(instance), birth);
                        }
                    }
                }
            }
            result.particles.resize(kept.size());
            for (std::size_t i = 0; !kept.empty(); ++i) { result.particles[i] = kept.top().particle; kept.pop(); }
            return R::success(std::move(result));
        }

        // An emitter without an active Particle branch is an incomplete graph,
        // not an implicit single-stream renderer. Disconnected Output ancestry
        // stays transparent; active force bypasses are rejected.
        if (active_particles.empty()) {
            for (const std::size_t index : topological_order) {
                const auto type = nodes[index]->type_key;
                if (type == kForceNode || type==kTransformNode) {
                    return R::failure(ErrorCode::invalid_request,
                                      "active Force paths require a connected Particle node");
                }
            }
            return R::success(std::move(result));
        }

        std::sort(active_particles.begin(), active_particles.end(), [&nodes](std::size_t left, std::size_t right) {
            return nodes[left]->id < nodes[right]->id;
        });
        std::size_t branch_count = 0;
        std::vector<std::size_t> topological_rank(count, count);
        for (std::size_t rank = 0; rank < topological_order.size(); ++rank) {
            topological_rank[topological_order[rank]] = rank;
        }

        std::vector<std::uint32_t> emitter_branch_counts(count, 0);
        for (const std::size_t particle_index : active_particles) {
            for (const std::size_t emitter_index : incoming[particle_index]) {
                if (!emitters[emitter_index]) {
                    return R::failure(ErrorCode::invalid_request,
                                      "Particle inputs must connect directly to emitters");
                }
                ++emitter_branch_counts[emitter_index];
                ++branch_count;
            }
        }

        // An active Force node may be included in Output's ancestry
        // without being reachable from any Particle stream (for example, a
        // disconnected Force wired directly to Output beside a valid branch).
        // Reject that topology instead of silently dropping its effect.
        std::vector<bool> has_particle_source(count, false);
        for (const std::size_t index : topological_order) {
            if (nodes[index]->type_key == kParticleNode) {
                has_particle_source[index] = true;
            } else {
                for (const std::size_t source : incoming[index]) {
                    has_particle_source[index] = has_particle_source[index] || has_particle_source[source];
                }
            }
            if ((nodes[index]->type_key == kForceNode || nodes[index]->type_key==kTransformNode) &&
                !has_particle_source[index]) {
                return R::failure(ErrorCode::invalid_request,
                                  "every active Force path must descend from a Particle node");
            }
        }

        struct BranchPlan {
            ValidatedSettings settings;
            ParticleSlotSequence slots;
            std::size_t emitter{0};
            std::size_t particle{0};
            std::shared_ptr<const TransformBranchPlan> transform;
        };
        const double time_seconds = to_seconds(*normalized);
        std::vector<BranchPlan> branches;
        branches.reserve(branch_count);
        std::vector<std::uint32_t> emitter_branch_indices(count, 0);
        std::vector<std::uint32_t> visited(count, 0);
        std::uint64_t traversal_work = 0;
        for (std::size_t particle_branch = 0; particle_branch < active_particles.size(); ++particle_branch) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "particle branch planning cancelled");
            const std::size_t particle_index = active_particles[particle_branch];
            if (incoming[particle_index].empty()) continue;
            const std::uint32_t visit_id = static_cast<std::uint32_t>(particle_branch + 1);
            std::vector<std::size_t> stack{particle_index};
            std::vector<std::size_t> branch_forces;
            visited[particle_index] = visit_id;
            while (!stack.empty()) {
                if (cancellation.is_cancelled()) {
                    return R::failure(ErrorCode::cancelled, "particle branch traversal cancelled");
                }
                if (traversal_work >= kMaxBranchTraversalWork) {
                    return R::failure(ErrorCode::invalid_request, "graph exceeds the particle-branch evaluation budget");
                }
                const std::size_t current = stack.back();
                stack.pop_back();
                ++traversal_work;
                const auto& node = *nodes[current];
                if (node.type_key == kForceNode) branch_forces.push_back(current);

                for (const std::size_t destination : outgoing[current]) {
                    if (traversal_work >= kMaxBranchTraversalWork) {
                        return R::failure(ErrorCode::invalid_request,
                                          "graph exceeds the particle-branch evaluation budget");
                    }
                    ++traversal_work;
                    if (active[destination] && !emitters[destination] && visited[destination] != visit_id) {
                        visited[destination] = visit_id;
                        stack.push_back(destination);
                    }
                }
            }
            const auto by_dependency = [&topological_rank](std::size_t left, std::size_t right) {
                return topological_rank[left] < topological_rank[right];
            };
            std::sort(branch_forces.begin(), branch_forces.end(), by_dependency);
            auto planned=transform_plan(particle_index);if(!planned.has_value())return R::failure(planned.error());
            const auto tf_plan=planned.value();
            if(tf_plan)branch_forces.erase(std::remove_if(branch_forces.begin(),branch_forces.end(),[&](auto force) {
                return std::find(tf_plan->force_nodes.begin(),tf_plan->force_nodes.end(),nodes[force]->id)==tf_plan->force_nodes.end();
            }),branch_forces.end());
            std::vector<NodeId> force_ids;for(auto force:branch_forces)force_ids.push_back(nodes[force]->id);
            // Plan the shared Particle's downstream topology once, then create
            // independent birth sequences for each direct emitter input.
            for (const std::size_t emitter : incoming[particle_index]) {
                Settings branch_settings = emitters[emitter]->value;
                branch_settings.particle_count = output_particle_count;
                branch_settings.particle_lifetime_seconds = particles[particle_index]->lifetime_seconds;
                for (const std::size_t force_index : branch_forces) {
                    if (cancellation.is_cancelled()) {
                        return R::failure(ErrorCode::cancelled, "force-chain evaluation cancelled");
                    }
                    const auto& force = *forces[force_index];
                    branch_settings.gravity.x += force.gravity.x;
                    branch_settings.gravity.y += force.gravity.y;
                    branch_settings.gravity.z += force.gravity.z;
                    branch_settings.linear_drag += force.linear_drag;
                    branch_settings.forces.push_back(motion_for_emitter(force.motion, nodes[emitter]->id));
                }
                auto bounded = validate_settings(branch_settings);
                if (!bounded.notices.empty()) {
                    return R::failure(ErrorCode::invalid_request,
                                      "combined force values exceed supported bounds");
                }
                if(tf_plan)map_transform_forces(bounded.value,*tf_plan,force_ids,emitters[emitter]->value.gravity);

                const auto slots = live_particle_branch_slots(bounded, time_seconds, emitter_branch_counts[emitter],
                                                              emitter_branch_indices[emitter]++);
                if (!slots.has_value()) return R::failure(slots.error());
                branches.push_back({std::move(bounded), slots.value(), emitter,
                                    particle_index,tf_plan});
            }
        }

        struct SlotCursor {
            double birth_time{0.0};
            std::size_t emitter{0}; // nodes[] is UUID-ordered
            std::size_t branch{0};
            std::uint64_t slot{0};
            std::uint64_t remaining{0};
        };
        const auto older = [](const SlotCursor& left, const SlotCursor& right) {
            if (left.birth_time != right.birth_time) return left.birth_time < right.birth_time;
            if (left.emitter != right.emitter) return left.emitter < right.emitter;
            return left.slot < right.slot;
        };
        // Merge newest live births first, so Output's cap is shared across all
        // emitters. A cursor skips expired modulo slots by construction: no
        // per-emitter population buffers or long scans through dead particles.
        std::vector<SlotCursor> cursor_storage;
        cursor_storage.reserve(branch_count);
        std::priority_queue<SlotCursor, std::vector<SlotCursor>, decltype(older)> newest(older,
                                                                                     std::move(cursor_storage));
        std::uint64_t candidate_count = 0;
        for (std::size_t branch = 0; branch < branch_count; ++branch) {
            const auto& plan = branches[branch];
            candidate_count += plan.slots.count;
            if (plan.slots.count == 0) continue;
            const auto last = plan.slots.first_slot + (plan.slots.count - 1) * plan.slots.stride;
            newest.push({static_cast<double>(last) / plan.settings.value.birth_rate,
                         plan.emitter, branch, last, plan.slots.count});
        }
        struct SelectedSlot { std::size_t branch; std::uint64_t slot; };
        std::vector<SelectedSlot> selected;
        selected.reserve(static_cast<std::size_t>(std::min<std::uint64_t>(output_particle_count, candidate_count)));
        std::vector<std::size_t> target_counts(branch_count, 0);
        while (!newest.empty() && selected.size() < output_particle_count) {
            if ((selected.size() % 4096) == 0 && cancellation.is_cancelled()) {
                return R::failure(ErrorCode::cancelled, "particle output merge cancelled");
            }
            auto cursor = newest.top();
            newest.pop();
            selected.push_back({cursor.branch, cursor.slot});
            ++target_counts[cursor.branch];
            if (--cursor.remaining > 0) {
                cursor.slot -= branches[cursor.branch].slots.stride;
                cursor.birth_time = static_cast<double>(cursor.slot) / branches[cursor.branch].settings.value.birth_rate;
                newest.push(cursor);
            }
        }

        // Reverse the merge order for stable oldest-to-newest compositing, then
        // bucket slot targets by branch without sorting or growing any buffers.
        result.particles.resize(selected.size());
        std::vector<std::size_t> offsets(branch_count + 1, 0);
        for (std::size_t branch = 0; branch < branch_count; ++branch) {
            offsets[branch + 1] = offsets[branch] + target_counts[branch];
        }
        auto next_target = offsets;
        std::vector<ParticleSlotTarget> targets(selected.size());
        for (std::size_t destination = 0; destination < selected.size(); ++destination) {
            if ((destination % 4096) == 0 && cancellation.is_cancelled()) {
                return R::failure(ErrorCode::cancelled, "particle output planning cancelled");
            }
            const auto& slot = selected[selected.size() - 1 - destination];
            targets[next_target[slot.branch]++] = {slot.slot, destination};
        }
        for (std::size_t branch = 0; branch < branch_count; ++branch) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph evaluation cancelled");
            const auto& plan = branches[branch];
            const auto branch_targets = std::span<const ParticleSlotTarget>(targets).subspan(offsets[branch],
                                                                                            target_counts[branch]);
            const auto* tf=plan.transform && plan.transform->combined?&*plan.transform->combined:nullptr;
            const auto simulated = simulate_selected_particles_into(plan.settings, time_seconds, branch_targets,
                                                                     result.particles, cancellation, dimension_context,tf);
            if (!simulated.has_value()) return R::failure(simulated.error());
            const ParticleValues& appearance = *particles[plan.particle];
            for (std::size_t i = 0; i < branch_targets.size(); ++i) {
                if ((i % 4096) == 0 && cancellation.is_cancelled()) {
                    return R::failure(ErrorCode::cancelled, "particle appearance evaluation cancelled");
                }
                auto& instance = result.particles[branch_targets[i].destination];
                instance.emitter_id = nodes[plan.emitter]->id;
                Vec3 birth_position=tf?tf->position(plan.settings.value.emitter_origin):plan.settings.value.emitter_origin;
                if(particles[plan.particle]->orient_to==2)
                    birth_position=simulate_particle_at_age(plan.settings.value,0,instance.id,dimension_context,tf).position;
                if(origins) {
                    const double birth=double(branch_targets[i].slot)/plan.settings.value.birth_rate;
                    const auto sampled=origins->sample(instance.emitter_id,birth);
                    if(!sampled.has_value()) return R::failure(sampled.error());
                    Vec3 delta{sampled.value().x-plan.settings.value.emitter_origin.x,sampled.value().y-plan.settings.value.emitter_origin.y,sampled.value().z-plan.settings.value.emitter_origin.z};
                    if(tf)delta=tf->velocity(delta);
                    instance.position.x+=delta.x;instance.position.y+=delta.y;instance.position.z+=delta.z;
                    birth_position.x+=delta.x;birth_position.y+=delta.y;birth_position.z+=delta.z;
                }
                // Particle owns all per-life style curves.
                apply_particle_style(instance, appearance, plan.settings.value.seed);
                apply_particle_properties(instance,*particles[plan.particle],plan.settings.value.seed,birth_position,tf);
                if(plan.transform)apply_transform_style(instance,*plan.transform);
            }
        }
        budget.work += result.particles.size();
        if (budget.work > 20'000'000) return R::failure(ErrorCode::work_limit_exceeded, "auxiliary evaluation limit exceeded");
        return R::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "graph evaluation allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "graph evaluation failed");
    }
}

#include "TemporalEvaluation.hpp"

Result<EvaluatedGraph> evaluate_particle_graph(const Graph& graph, RationalTime time,
                                               const Cancellation& cancellation,
                                               EmitterDimensionContext dimension_context, EmitterOriginSampler* origin_sampler) {
    const OpaqueBytes* frozen=nullptr;
    for(const auto& record:graph.optional_records) if(record.size()>=2 &&
        record[0]==std::byte{4} && record[1]==std::byte{0x80}) {
        if(frozen) return Result<EvaluatedGraph>::failure(ErrorCode::invalid_request,"duplicate temporal particle record");
        frozen=&record;
    }
    if(frozen) {
        if(cancellation.is_cancelled()) return Result<EvaluatedGraph>::failure(ErrorCode::cancelled,"temporal render cancelled");
        return decode_evaluated_particles(*frozen,time,&cancellation);
    }
    // Static graph entry points share Once timing and variable survivor selection
    // with the historical evaluator.
    // These values are constant by the graph contract, not by sampling inference.
    bool once=false;
    for(const auto& node:graph.nodes) if(node.type_key==kEmitterNode) {
        if(const auto* mode=find_value(node,kEmittingMode)) {
            const auto* value=std::get_if<std::uint32_t>(mode);
            once=once || (value && *value==1);
        }
    }
    for(const auto& node:graph.nodes) if(node.type_key==kParticleNode) {
        if(const auto* random=find_value(node,kLifeRandom)) {
            const auto* value=std::get_if<double>(random);
            once=once || (value && *value>0);
        }
    }
    if(once) {
        auto decoded=decode_emitter_origin_history(graph);
        if(!decoded.has_value()) return Result<EvaluatedGraph>::failure(decoded.error());
        auto history=decoded.take_value();
        if(!origin_sampler && history.present) origin_sampler=&history;
        class StaticSampler final:public TemporalGraphSampler {
            const Graph& graph_;
            EmitterOriginSampler* origins_;
        public:
            StaticSampler(const Graph& graph,EmitterOriginSampler* origins):graph_(graph),origins_(origins) {}
            Result<GraphNode> node(NodeId id,double seconds) override {
                for(const auto& n:graph_.nodes) if(n.id==id) {
                    auto copy=n;
                    if(origins_ && n.type_key==kEmitterNode) {
                        auto origin=origins_->sample(id,seconds);
                        if(!origin.has_value()) return Result<GraphNode>::failure(origin.error());
                        for(auto& parameter:copy.parameters) if(parameter.key==kEmitterOrigin) parameter.value=origin.value();
                    }
                    return Result<GraphNode>::success(std::move(copy));
                }
                return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing static node");
            }
            Result<double> rate(NodeId id,double) override {
                for(const auto& n:graph_.nodes) if(n.id==id) {
                    if(const auto* rate=find_value(n,kBirthRate)) if(const auto* value=std::get_if<double>(rate))
                        return Result<double>::success(*value);
                }
                return Result<double>::failure(ErrorCode::invalid_request,"missing static emission rate");
            }
            Result<std::optional<EmissionRateProfile>> rate_profile(NodeId id) override {
                auto value=rate(id,0);
                if(!value.has_value()) return Result<std::optional<EmissionRateProfile>>::failure(value.error());
                EmissionRateProfile profile;profile.constant=value.value();
                return Result<std::optional<EmissionRateProfile>>::success(std::move(profile));
            }
        } sampler(graph,origin_sampler);
        return evaluate_temporal_particle_graph(graph,time,cancellation,dimension_context,sampler);
    }
    for (const auto& node : graph.nodes) if (node.type_key == graph_keys::kOutputNode) {
        const auto* enabled = find_value(node, graph_keys::kTimeRemapEnabled);
        const auto* clock = find_value(node, graph_keys::kTimeRemapSeconds);
        const auto* flag = enabled ? std::get_if<std::uint32_t>(enabled) : nullptr;
        const auto* seconds_value = clock ? std::get_if<double>(clock) : nullptr;
        if (flag && *flag == 1 && seconds_value) {
            const double seconds = *seconds_value;
            if (!std::isfinite(seconds) || std::abs(seconds) > 1000000)
                return Result<EvaluatedGraph>::failure(ErrorCode::invalid_time, "invalid remapped time");
            time = RationalTime{static_cast<std::int64_t>(std::llround(seconds * 1000000)), 1000000};
        }
    }
    EvaluationBudget budget;
    auto decoded=decode_emitter_origin_history(graph);
    if(!decoded.has_value()) return Result<EvaluatedGraph>::failure(decoded.error());
    auto history=decoded.take_value();
    if(!origin_sampler && history.present) origin_sampler=&history;
    return evaluate_graph_impl(graph, time, cancellation, dimension_context, budget, 0, origin_sampler);
}

} // namespace starfield::core
