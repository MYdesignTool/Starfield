#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/Random.hpp"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iterator>
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
};

struct AppearanceValues {
    Vec3 color_start{1.0, 1.0, 1.0};
    Vec3 color_end{1.0, 1.0, 1.0};
    double size_start{8.0};
    double size_end{8.0};
    double opacity_start{1.0};
    double opacity_end{1.0};
    double lifetime_seconds{2.0};
    double size_random_percent{0.0};
    double opacity_random_percent{0.0};
    AgeCurve size_curve{};
    AgeCurve opacity_curve{};
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
            case kParticleCount.value: settings.particle_count = std::get<std::uint32_t>(parameter.value); break;
            case kBirthRate.value: settings.birth_rate = std::get<double>(parameter.value); break;
            case kSeed.value: settings.seed = std::get<std::uint32_t>(parameter.value); break;
            case kLifetimeSeconds.value: settings.particle_lifetime_seconds = std::get<double>(parameter.value); break;
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
                settings.emission_angles_degrees.x = std::get<double>(parameter.value);
                break;
            case kEmissionAngleY.value:
                settings.emission_angles_degrees.y = std::get<double>(parameter.value);
                break;
            case kEmissionAngleZ.value:
                settings.emission_angles_degrees.z = std::get<double>(parameter.value);
                break;
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
                settings.particle_size_end = settings.particle_size;
                break;
            case kOpacity.value:
                settings.opacity = std::get<double>(parameter.value);
                settings.opacity_end = settings.opacity;
                break;
            case kEmitterSize.value: settings.emitter_size = std::get<double>(parameter.value); break;
            case kVelocitySpread.value: settings.velocity_spread = std::get<double>(parameter.value); break;
        }
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
    return Result<ForceValues>::success(ForceValues{validated.value.gravity, validated.value.linear_drag});
}

Result<AppearanceValues> read_appearance(const GraphNode& node) {
    const auto* color_start = find_value(node, kColorStart);
    const auto* color_end = find_value(node, kColorEnd);
    const auto* size_start = find_value(node, kSizeStart);
    const auto* size_end = find_value(node, kSizeEnd);
    const auto* opacity_start = find_value(node, kOpacityStart);
    const auto* opacity_end = find_value(node, kOpacityEnd);
    if (!color_start || !color_end || !size_start || !size_end || !opacity_start || !opacity_end) {
        return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance node is missing values");
    }
    Settings settings;
    settings.color_start = std::get<Vec3>(*color_start);
    settings.color_end = std::get<Vec3>(*color_end);
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
        if (!bytes || !decode_age_curve(*bytes, settings.size_over_life, 0.0, kMaxParticleSize)) {
            return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance size curve is invalid");
        }
    }
    if (opacity_curve) {
        const auto* bytes = std::get_if<OpaqueBytes>(opacity_curve);
        if (!bytes || !decode_age_curve(*bytes, settings.opacity_over_life, 0.0, 1.0)) {
            return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance opacity curve is invalid");
        }
    }
    auto validated = validate_settings(settings);
    if (!validated.notices.empty()) {
        return Result<AppearanceValues>::failure(ErrorCode::invalid_request, "appearance node value is outside supported bounds");
    }
    return Result<AppearanceValues>::success(AppearanceValues{
        validated.value.color_start, validated.value.color_end,
        validated.value.particle_size, validated.value.particle_size_end,
        validated.value.opacity, validated.value.opacity_end,
        validated.value.particle_lifetime_seconds,
        validated.value.particle_size_random_percent, validated.value.opacity_random_percent,
        validated.value.size_over_life, validated.value.opacity_over_life});
}

void apply_appearance(ParticleInstance& particle, const AppearanceValues& appearance,
                      std::uint32_t seed) noexcept {
    const double age_fraction = particle.lifetime_seconds > 0.0
        ? std::clamp(particle.age_seconds / particle.lifetime_seconds, 0.0, 1.0) : 0.0;
    particle.size_pixels = evaluate_age_curve(appearance.size_curve, age_fraction,
                                              appearance.size_start, appearance.size_end);
    particle.opacity = evaluate_age_curve(appearance.opacity_curve, age_fraction,
                                          appearance.opacity_start, appearance.opacity_end);
    particle.size_pixels *= 1.0 - (appearance.size_random_percent / 100.0) *
        unit_value(seed, particle.id, RandomPurpose::size);
    particle.opacity *= 1.0 - (appearance.opacity_random_percent / 100.0) *
        unit_value(seed, particle.id, RandomPurpose::opacity);
    particle.color = Vec3{
        appearance.color_start.x + (appearance.color_end.x - appearance.color_start.x) * age_fraction,
        appearance.color_start.y + (appearance.color_end.y - appearance.color_start.y) * age_fraction,
        appearance.color_start.z + (appearance.color_end.z - appearance.color_start.z) * age_fraction};
}

bool is_particle_graph_edge(const GraphNode& source, const GraphNode& destination) noexcept {
    const auto& from = source.type_key;
    const auto& to = destination.type_key;
    if (from == kEmitterNode) return to == kParticleNode;
    if (from == kParticleNode) return to == kForceNode || to == kAppearanceNode || to == kOutputNode;
    if (from == kForceNode) return to == kForceNode || to == kAppearanceNode || to == kOutputNode;
    if (from == kAppearanceNode) return to == kOutputNode;
    return false;
}

} // namespace


Result<EvaluatedGraph> evaluate_particle_graph(const Graph& graph, RationalTime time,
                                               const Cancellation& cancellation,
                                               EmitterDimensionContext dimension_context) {
    using R = Result<EvaluatedGraph>;
    if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph evaluation cancelled");
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
        std::vector<std::optional<ValidatedSettings>> emitters(count);
        std::vector<std::optional<ForceValues>> forces(count);
        std::vector<std::optional<AppearanceValues>> appearances(count);
        std::vector<std::optional<AppearanceValues>> particles(count);
        // Check semantic bounds on every node, including disconnected/parked nodes.
        for (std::size_t i = 0; i < count; ++i) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "node validation cancelled");
            const auto& node = *nodes[i];
            if (node.type_key == kOutputNode) {
                if (output != count) return R::failure(ErrorCode::invalid_request, "graph requires exactly one output");
                output = i;
            } else if (node.type_key == kEmitterNode) {
                auto value = read_emitter(node);
                if (!value.has_value()) return R::failure(value.error());
                emitters[i] = value.take_value();
            } else if (node.type_key == kParticleNode) {
                auto value = read_appearance(node);
                if (!value.has_value()) return R::failure(value.error());
                particles[i] = value.take_value();
            } else if (node.type_key == kForceNode) {
                auto value = read_force(node);
                if (!value.has_value()) return R::failure(value.error());
                forces[i] = value.take_value();
            } else if (node.type_key == kAppearanceNode) {
                auto value = read_appearance(node);
                if (!value.has_value()) return R::failure(value.error());
                appearances[i] = value.take_value();
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
        std::size_t active_emitter = count;
        std::vector<std::size_t> active_particles;
        while (!ready.empty()) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "graph execution cancelled");
            const auto index = ready.top();
            ready.pop();
            const auto& node = *nodes[index];
            topological_order.push_back(index);
            if (node.type_key == kEmitterNode) {
                if (active_emitter != count) {
                    return R::failure(ErrorCode::invalid_request, "multiple active emitters are unsupported");
                }
                active_emitter = index;
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

        // A topology edit can temporarily leave the Output ancestry without an
        // emitter (for example, while a user disconnects and rewires a chain).
        // Keep the graph valid and render transparent until a complete source
        // path is connected again. Structural/type errors are still rejected by
        // validate_graph above.
        if (active_emitter == count) return R::success(std::move(result));

        // Graphs whose active output ancestry predates Particle are interpreted
        // as one legacy stream. Disconnected/parked Particle nodes stay inert.
        const bool legacy_graph = active_particles.empty();
        if (legacy_graph) {
            Settings render_settings = emitters[active_emitter]->value;
            std::uint32_t active_appearance_count = 0;
            int previous_stage = -1;
            for (const std::size_t index : topological_order) {
                if (cancellation.is_cancelled()) {
                    return R::failure(ErrorCode::cancelled, "legacy graph execution cancelled");
                }
                const auto& node = *nodes[index];
                int stage = -1;
                if (node.type_key == kEmitterNode) stage = 0;
                else if (node.type_key == kForceNode) stage = 1;
                else if (node.type_key == kAppearanceNode) stage = 2;
                else if (node.type_key == kOutputNode) stage = 3;
                if (stage < previous_stage) {
                    return R::failure(ErrorCode::invalid_request,
                                      "legacy graph stages must be emitter, force, appearance, output");
                }
                previous_stage = stage;
                if (node.type_key == kForceNode) {
                    const auto& force = *forces[index];
                    render_settings.gravity.x += force.gravity.x;
                    render_settings.gravity.y += force.gravity.y;
                    render_settings.gravity.z += force.gravity.z;
                    render_settings.linear_drag += force.linear_drag;
                } else if (node.type_key == kAppearanceNode) {
                    if (++active_appearance_count != 1) {
                        return R::failure(ErrorCode::invalid_request,
                                          "legacy graph supports one active appearance node");
                    }
                    const auto& appearance = *appearances[index];
                    render_settings.color_start = appearance.color_start;
                    render_settings.color_end = appearance.color_end;
                    render_settings.particle_size = appearance.size_start;
                    render_settings.particle_size_end = appearance.size_end;
                    render_settings.opacity = appearance.opacity_start;
                    render_settings.opacity_end = appearance.opacity_end;
                    render_settings.size_over_life = appearance.size_curve;
                    render_settings.opacity_over_life = appearance.opacity_curve;
                    render_settings.particle_size_random_percent = appearance.size_random_percent;
                    render_settings.opacity_random_percent = appearance.opacity_random_percent;
                    render_settings.appearance_enabled = true;
                }
            }
            const auto bounded = validate_settings(render_settings);
            if (!bounded.notices.empty()) {
                return R::failure(ErrorCode::invalid_request,
                                   "combined legacy force or appearance values exceed supported bounds");
            }
            auto evaluated = simulate_particles(bounded, to_seconds(*normalized), cancellation,
                                                dimension_context);
            if (!evaluated.has_value()) return R::failure(evaluated.error());
            result.particles = evaluated.take_value();
            return R::success(std::move(result));
        }

        const ValidatedSettings& validated_emitter = *emitters[active_emitter];
        const Settings& emitter_settings = validated_emitter.value;
        std::sort(active_particles.begin(), active_particles.end(), [&nodes](std::size_t left, std::size_t right) {
            return nodes[left]->id < nodes[right]->id;
        });
        const std::size_t particle_count = active_particles.size();
        std::vector<std::size_t> topological_rank(count, count);
        for (std::size_t rank = 0; rank < topological_order.size(); ++rank) {
            topological_rank[topological_order[rank]] = rank;
        }

        for (const auto& edge : graph.edges) {
            const std::size_t source = index_of(edge.source_node);
            const std::size_t destination = index_of(edge.destination_node);
            if (!active[source] || !active[destination]) continue;
            if (!is_particle_graph_edge(*nodes[source], *nodes[destination])) {
                return R::failure(ErrorCode::invalid_request, "invalid active Particle graph connection");
            }
        }
        for (const std::size_t particle_index : active_particles) {
            if (incoming[particle_index].size() != 1 || incoming[particle_index].front() != active_emitter) {
                return R::failure(ErrorCode::invalid_request,
                                  "each active Particle node must connect directly to the active emitter");
            }
        }

        // Emission slots are globally capped and stable, while each Particle branch
        // can retire its assigned slots at its own lifetime. Generate the bounded
        // candidate interval using the longest active branch, then compact expired
        // branch slots in global-ID order below.
        std::vector<double> branch_lifetimes(particle_count, 0.0);
        double candidate_lifetime = 0.0;
        for (std::size_t branch = 0; branch < particle_count; ++branch) {
            const GraphNode& particle_node = *nodes[active_particles[branch]];
            double lifetime = particles[active_particles[branch]]->lifetime_seconds;
            // Schema-1 graphs created before Particle owned Lifetime stored it on
            // Emitter. Keep that narrow fallback while all newly built graphs write
            // the branch value on Particle.
            if (!find_value(particle_node, kParticleLifetimeSeconds)) {
                lifetime = emitter_settings.particle_lifetime_seconds;
            }
            branch_lifetimes[branch] = lifetime;
            candidate_lifetime = std::max(candidate_lifetime, lifetime);
        }
        Settings slot_settings = emitter_settings;
        slot_settings.particle_lifetime_seconds = candidate_lifetime;
        const auto bounded_slots = validate_settings(slot_settings);
        if (!bounded_slots.notices.empty()) {
            return R::failure(ErrorCode::invalid_request,
                              "Particle lifetime values exceed supported bounds");
        }
        const auto live_slots_result = live_particle_slot_range(bounded_slots, to_seconds(*normalized));
        if (!live_slots_result.has_value()) return R::failure(live_slots_result.error());
        const ParticleSlotRange live_slots = live_slots_result.value();

        std::vector<std::uint32_t> visited(count, 0);
        std::vector<std::size_t> branch_appearance(particle_count, count);
        std::vector<ParticleInstance> staged_particles(static_cast<std::size_t>(live_slots.count));
        std::uint64_t traversal_work = 0;
        for (std::size_t branch = 0; branch < particle_count; ++branch) {
            if (cancellation.is_cancelled()) return R::failure(ErrorCode::cancelled, "particle branch planning cancelled");
            const std::uint32_t visit_id = static_cast<std::uint32_t>(branch + 1);
            std::vector<std::size_t> stack{active_particles[branch]};
            std::vector<std::size_t> branch_forces;
            std::vector<std::size_t> branch_appearances;
            visited[active_particles[branch]] = visit_id;
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
                else if (node.type_key == kAppearanceNode) branch_appearances.push_back(current);

                for (const std::size_t destination : outgoing[current]) {
                    if (traversal_work >= kMaxBranchTraversalWork) {
                        return R::failure(ErrorCode::invalid_request,
                                          "graph exceeds the particle-branch evaluation budget");
                    }
                    ++traversal_work;
                    if (active[destination] && visited[destination] != visit_id) {
                        visited[destination] = visit_id;
                        stack.push_back(destination);
                    }
                }
            }
            const auto by_dependency = [&topological_rank](std::size_t left, std::size_t right) {
                return topological_rank[left] < topological_rank[right];
            };
            std::sort(branch_forces.begin(), branch_forces.end(), by_dependency);
            std::sort(branch_appearances.begin(), branch_appearances.end(), by_dependency);
            if (branch_appearances.size() > 1) {
                return R::failure(ErrorCode::invalid_request,
                                  "one Particle stream cannot have multiple active Appearance overrides");
            }
            if (!branch_appearances.empty()) branch_appearance[branch] = branch_appearances.front();

            Settings branch_settings = slot_settings;
            for (const std::size_t force_index : branch_forces) {
                if (cancellation.is_cancelled()) {
                    return R::failure(ErrorCode::cancelled, "force-chain evaluation cancelled");
                }
                const auto& force = *forces[force_index];
                branch_settings.gravity.x += force.gravity.x;
                branch_settings.gravity.y += force.gravity.y;
                branch_settings.gravity.z += force.gravity.z;
                branch_settings.linear_drag += force.linear_drag;
            }
            const auto bounded = validate_settings(branch_settings);
            if (!bounded.notices.empty()) {
                return R::failure(ErrorCode::invalid_request,
                                  "combined force values exceed supported bounds");
            }

            const auto branch_result = simulate_particles_partition_into(
                bounded, to_seconds(*normalized), static_cast<std::uint32_t>(particle_count),
                static_cast<std::uint32_t>(branch), staged_particles, cancellation,
                dimension_context);
            if (!branch_result.has_value()) return R::failure(branch_result.error());
        }

        // Particle appearance is the default for its stream; a downstream Appearance
        // replaces it. Resolve that precedence once, after branch simulation.
        result.particles.reserve(staged_particles.size());
        for (auto& instance : staged_particles) {
            const std::size_t branch = static_cast<std::size_t>(instance.id % particle_count);
            if (instance.age_seconds >= branch_lifetimes[branch]) continue;
            instance.lifetime_seconds = branch_lifetimes[branch];
            const std::size_t override_index = branch_appearance[branch];
            const AppearanceValues& appearance = override_index == count
                ? *particles[active_particles[branch]] : *appearances[override_index];
            apply_appearance(instance, appearance, emitter_settings.seed);
            result.particles.push_back(std::move(instance));
        }
        return R::success(std::move(result));
    } catch (const std::bad_alloc&) {
        return R::failure(ErrorCode::allocation_failed, "graph evaluation allocation failed");
    } catch (...) {
        return R::failure(ErrorCode::internal_failure, "graph evaluation failed");
    }
}

} // namespace starfield::core
