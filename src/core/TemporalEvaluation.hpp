// Private implementation included by GraphEvaluation.cpp so the ordinary and
// temporal paths share the same parameter validation and appearance functions.

namespace {
constexpr double kDefaultTemporalHz = 30.0;
constexpr std::uint64_t kTemporalWorkLimit = 20'000'000;

Vec3 temporal_spin_velocity(const ForceMotion& values,double age,double lifetime) {
    const double spin_age=age-values.spin_delay;
    if(values.spin_radius<=0 || spin_age<=0) return {};
    const double omega=2*3.14159265358979323846*(values.spin_frequency>0?values.spin_frequency:1);
    const double envelope=evaluate_age_curve(values.wind_spin_curve,age/lifetime,100,100)/100;
    const double radius=values.spin_radius*std::exp(-values.spin_resist/100*spin_age)*envelope;
    const double c=std::cos(omega*spin_age),s=std::sin(omega*spin_age);
    const double rate=-values.spin_resist/100*radius;
    return {rate*(c-1)-radius*omega*s,rate*s+radius*omega*c,0};
}

struct EmissionClock {
    EmissionTimeline timeline;
    std::shared_ptr<EmissionTimeline> cached;
    EmissionTimeline& value() {return cached?*cached:timeline;}
    bool configured{};
    Result<bool> extend(NodeId id,double now,TemporalGraphSampler& sampler,
                         const Cancellation& cancel,std::uint64_t& work,unsigned hz) {
        using R=Result<bool>;
        if(!configured) {
            cached=sampler.emission_timeline(id,hz);
            if(cached && cached->frequency()==hz && cached->initialized())configured=true;
        }
        if(!configured) {
            auto profile=sampler.rate_profile(id);
            if(!profile.has_value()) return R::failure(profile.error());
            auto ready=value().configure(hz,profile.value()?&*profile.value():nullptr);
            if(!ready.has_value()) return ready;
            configured=true;
        }
        return value().extend(now,[&](double time){return sampler.rate(id,time);},cancel,work);
    }
};

class TemporalEvaluator {
    TemporalGraphSampler& sampler;
    const Cancellation& cancel;
    EmitterDimensionContext dimensions;
    std::map<NodeId,EmissionClock> clocks;
    std::uint64_t work{};
    double hz{kDefaultTemporalHz};
    std::map<std::pair<NodeId,double>,ForceValues> force_samples;
    std::map<std::pair<NodeId,double>,CompiledParticleTransform> transform_samples;
    TransformPlanningBudget transform_budget;
    using R=Result<EvaluatedGraph>;
    struct Branch {
        const GraphNode* emitter{};
        const GraphNode* particle{};
        std::vector<const GraphNode*> forces;
        std::uint64_t partition{}, stride{};
        std::shared_ptr<const TransformBranchPlan> transform;
        std::vector<const CompiledParticleTransform*> force_spaces;
    };
    Result<GraphNode> at(const GraphNode& node,double time) {
        if(cancel.is_cancelled()) return Result<GraphNode>::failure(ErrorCode::cancelled,"temporal sampling cancelled");
        if(++work>kTemporalWorkLimit) return Result<GraphNode>::failure(ErrorCode::work_limit_exceeded,"temporal sampling work limit");
        auto result=sampler.node(node.id,time);
        if(result.has_value() && (result.value().id!=node.id || result.value().type_key!=node.type_key || result.value().schema_version!=node.schema_version))
            return Result<GraphNode>::failure(ErrorCode::invalid_request,"historical node identity changed");
        return result;
    }
    Result<ForceValues> force_at(const GraphNode& node,double time) {
        const auto key=std::make_pair(node.id,time);
        if(auto found=force_samples.find(key);found!=force_samples.end())
            return Result<ForceValues>::success(found->second);
        auto sampled=at(node,time);
        if(!sampled.has_value()) return Result<ForceValues>::failure(sampled.error());
        auto values=read_force(sampled.value());
        if(values.has_value()) {
            if(force_samples.size()>=65536) force_samples.clear();
            force_samples.emplace(key,values.value());
        }
        return values;
    }
    Result<CompiledParticleTransform> transform_at(const GraphNode& node,double time) {
        const auto key=std::make_pair(node.id,time);
        if(const auto found=transform_samples.find(key);found!=transform_samples.end())
            return Result<CompiledParticleTransform>::success(found->second);
        auto sampled=at(node,time);if(!sampled.has_value())return Result<CompiledParticleTransform>::failure(sampled.error());
        auto values=read_transform(sampled.value());
        if(values.has_value()) {
            if(transform_samples.size()>=65536)transform_samples.clear();
            transform_samples.emplace(key,values.value());
        }
        return values;
    }
    Result<bool> motion(ParticleInstance& particle,const Branch& branch,
                        double birth,double now,std::uint32_t seed) {
        using M=Result<bool>;
        // Each interval solves constant acceleration/drag exactly. Force and
        // age-envelope values are evaluated at its midpoint. The lattice is
        // anchored at simulation zero, including partial birth/final intervals.
        double t=birth;
        while(t<now && !branch.forces.empty()) {
            if(cancel.is_cancelled()) return M::failure(ErrorCode::cancelled,"force history cancelled");
            if(++work>kTemporalWorkLimit) return M::failure(ErrorCode::work_limit_exceeded,"force history work limit");
            const double end=std::min(now,(std::floor(t*hz+1e-8)+1)/hz);
            if(!(end>t)) return M::failure(ErrorCode::invalid_time,"force integration did not advance");
            const double dt=end-t, midpoint=(t+end)/2, age=midpoint-birth;
            Vec3 acceleration{}, spin_velocity{};
            double drag=0;
            for(std::size_t force_index=0;force_index<branch.forces.size();++force_index) {
                const auto* force_node=branch.forces[force_index];
                auto force=force_at(*force_node,midpoint);if(!force.has_value()) return M::failure(force.error());
                const auto values=motion_for_emitter(force.value().motion,branch.emitter->id);
                const double attenuation=unit_value(seed^values.random_salt,particle.id,RandomPurpose::force_gravity)*values.gravity_random_percent/100;
                const double envelope=evaluate_age_curve(values.wind_spin_curve,
                    age/particle.lifetime_seconds,100,100)/100;
                Vec3 contribution{force.value().gravity.x*(1-attenuation)+values.wind.x*envelope,
                    force.value().gravity.y*(1-attenuation)+values.wind.y*envelope,
                    force.value().gravity.z*(1-attenuation)+values.wind.z*envelope};
                drag+=force.value().linear_drag;
                // Spin is a displacement field. Integrate its local velocity,
                // so animated radius/frequency does not teleport old particles.
                auto spin=temporal_spin_velocity(values,age,particle.lifetime_seconds);
                if(!branch.force_spaces.empty())if(const auto* space=branch.force_spaces[force_index]) {
                    contribution=space->velocity(contribution);spin=space->velocity(spin);
                }
                acceleration.x+=contribution.x;acceleration.y+=contribution.y;acceleration.z+=contribution.z;
                spin_velocity.x+=spin.x;spin_velocity.y+=spin.y;spin_velocity.z+=spin.z;
            }
            if(drag>kMaxLinearDrag) return M::failure(ErrorCode::invalid_request,"combined animated drag outside bounds");
            const double decay=std::exp(-drag*dt);
            const double a=drag>1e-10 ? -std::expm1(-drag*dt)/drag : dt;
            const double z=drag*dt;
            const double b=std::abs(z)<1e-4 ? dt*dt*(0.5-z/6+z*z/24-z*z*z/120) : (dt-a)/drag;
            particle.position.x+=particle.velocity.x*a+acceleration.x*b+spin_velocity.x*dt;
            particle.position.y+=particle.velocity.y*a+acceleration.y*b+spin_velocity.y*dt;
            particle.position.z+=particle.velocity.z*a+acceleration.z*b;
            if(spin_velocity.z!=0)particle.position.z+=spin_velocity.z*dt;
            particle.velocity={particle.velocity.x*decay+acceleration.x*a,
                               particle.velocity.y*decay+acceleration.y*a,
                               particle.velocity.z*decay+acceleration.z*a};
            t=end;
        }
        if(branch.forces.empty()) {
            const double age=now-birth;
            particle.position.x+=particle.velocity.x*age;
            particle.position.y+=particle.velocity.y*age;
            particle.position.z+=particle.velocity.z*age;
        } else {
            // Report the instantaneous field velocity for Auxiliary inheritance;
            // do not feed it back into physical velocity during integration.
            for(std::size_t force_index=0;force_index<branch.forces.size();++force_index) {
                const auto* force_node=branch.forces[force_index];
                auto force=force_at(*force_node,now);if(!force.has_value()) return M::failure(force.error());
                auto spin=temporal_spin_velocity(force.value().motion,now-birth,particle.lifetime_seconds);
                if(!branch.force_spaces.empty())if(const auto* space=branch.force_spaces[force_index])spin=space->velocity(spin);
                particle.velocity.x+=spin.x;particle.velocity.y+=spin.y;
                if(spin.z!=0)particle.velocity.z+=spin.z;
            }
        }
        return M::success(true);
    }
public:
    TemporalEvaluator(TemporalGraphSampler& s,const Cancellation& c,EmitterDimensionContext d,unsigned frequency)
        :sampler(s),cancel(c),dimensions(d),hz(frequency) {}
    R evaluate(const Graph& graph,double now,unsigned depth=0) {
        if(depth>16) return R::failure(ErrorCode::work_limit_exceeded,"auxiliary history depth limit");
        const auto validation=validate_graph(graph,particle_node_registry());
        if(!validation) return R::failure(ErrorCode::invalid_request,"invalid temporal graph");
        if(cancel.is_cancelled()) return R::failure(ErrorCode::cancelled,"temporal graph cancelled");
        std::map<NodeId,const GraphNode*> nodes;
        std::map<NodeId,std::vector<NodeId>> incoming,outgoing;
        const GraphNode* output=nullptr;
        for(const auto& node:graph.nodes) {
            nodes[node.id]=&node;
            if(node.type_key==kOutputNode) {if(output) return R::failure(ErrorCode::invalid_request,"multiple Outputs");output=&node;}
        }
        if(!output) return R::failure(ErrorCode::invalid_request,"missing Output");
        for(const auto& edge:graph.edges) {
            if(!is_particle_graph_edge(*nodes.at(edge.source_node),*nodes.at(edge.destination_node)))
                return R::failure(ErrorCode::invalid_request,"unsupported temporal edge");
            incoming[edge.destination_node].push_back(edge.source_node);
            outgoing[edge.source_node].push_back(edge.destination_node);
        }
        std::map<NodeId,bool> active;
        std::vector<NodeId> stack{output->id};
        while(!stack.empty()) {const auto id=stack.back();stack.pop_back();if(active[id]) continue;active[id]=true;
            for(auto source:incoming[id]) stack.push_back(source);}
        const auto* cap_value=find_value(*output,kParticleCount);
        const std::uint32_t cap=cap_value?std::get<std::uint32_t>(*cap_value):0;
        if(cap>kMaxParticleCount) return R::failure(ErrorCode::invalid_request,"Output cap outside bounds");
        for(auto key:{kTimeRemapEnabled,kPreviewEnabled})
            if(const auto* flag=find_value(*output,key);flag && std::get<std::uint32_t>(*flag)>1)
                return R::failure(ErrorCode::invalid_request,"invalid Output switch");
        if(const auto* chance=find_value(*output,kPreviewChance);chance &&
           (!std::isfinite(std::get<double>(*chance)) || std::get<double>(*chance)<0 || std::get<double>(*chance)>100))
            return R::failure(ErrorCode::invalid_request,"Particle chance outside 0..100");
        EvaluatedGraph result;
        for(const auto& [id,node]:nodes) if(active[id]) result.evaluated_nodes.push_back(id);
        const bool has_transforms=std::any_of(graph.nodes.begin(),graph.nodes.end(),[](const auto& n){return n.type_key==kTransformNode;});
        std::vector<const GraphNode*> plan_nodes;
        std::map<NodeId,std::size_t> plan_indices;
        std::vector<std::vector<std::size_t>> plan_incoming,plan_outgoing;
        std::vector<bool> plan_active;
        std::vector<std::size_t> plan_order;
        std::vector<std::optional<CompiledParticleTransform>> plan_transforms;
        if(has_transforms) {
            for(const auto& [id,n]:nodes){plan_indices.emplace(id,plan_nodes.size());plan_nodes.push_back(n);}
            const auto count=plan_nodes.size();plan_incoming.resize(count);plan_outgoing.resize(count);
            plan_active.resize(count);plan_transforms.resize(count);std::vector<std::size_t> degrees(count);
            std::priority_queue<std::size_t,std::vector<std::size_t>,std::greater<>> ready;
            for(std::size_t i=0;i<count;++i) {
                plan_active[i]=active[plan_nodes[i]->id];
                for(auto source:incoming[plan_nodes[i]->id])plan_incoming[i].push_back(plan_indices.at(source));
                for(auto dest:outgoing[plan_nodes[i]->id])plan_outgoing[i].push_back(plan_indices.at(dest));
                if(plan_nodes[i]->type_key==kTransformNode) {
                    auto authored=read_transform(*plan_nodes[i]);if(!authored.has_value())return R::failure(authored.error());
                    if(plan_active[i]) {
                        auto sampled=transform_at(*plan_nodes[i],now);if(!sampled.has_value())return R::failure(sampled.error());
                        plan_transforms[i]=sampled.take_value();
                    }
                }
                if(plan_active[i]){degrees[i]=plan_incoming[i].size();if(!degrees[i])ready.push(i);}
            }
            while(!ready.empty()) {
                if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform temporal topology cancelled");
                auto index=ready.top();ready.pop();plan_order.push_back(index);
                for(auto dest:plan_outgoing[index])if(plan_active[dest] && --degrees[dest]==0)ready.push(dest);
            }
        }
        if(now<0 || !cap) return R::success(std::move(result));
        std::vector<Branch> branches;
        std::map<NodeId,std::uint64_t> partitions;
        for(const auto& [id,node]:nodes) if(active[id] && node->type_key==kParticleNode) {
            Branch base; base.particle=node;bool terminal=false;
            std::map<NodeId,bool> visited;stack={id};
            while(!stack.empty()) {
                auto next=stack.back();stack.pop_back();if(visited[next]) continue;visited[next]=true;
                const auto* downstream=nodes.at(next);
                if(downstream->type_key==kEmitterNode) continue;
                if(downstream->type_key==kOutputNode) terminal=true;
                if(downstream->type_key==kForceNode) base.forces.push_back(downstream);
                for(auto dest:outgoing[next]) if(active[dest]) stack.push_back(dest);
            }
            if(!terminal) continue;
            std::sort(base.forces.begin(),base.forces.end(),[](auto a,auto b){return a->id<b->id;});
            if(has_transforms) {
                auto planned=plan_transform_branch(plan_nodes,plan_incoming,plan_outgoing,plan_active,plan_order,
                    plan_indices.at(id),plan_indices.at(output->id),plan_transforms,cancel,transform_budget);
                if(!planned.has_value())return R::failure(planned.error());auto value=planned.take_value();
                auto basis=retain_transform_basis(result,value);if(!basis.has_value())return R::failure(basis.error());
                value.sprite_basis_index=basis.value();base.transform=std::make_shared<const TransformBranchPlan>(std::move(value));
                base.forces.erase(std::remove_if(base.forces.begin(),base.forces.end(),[&](const auto* force) {
                    return std::find(base.transform->force_nodes.begin(),base.transform->force_nodes.end(),force->id)==base.transform->force_nodes.end();
                }),base.forces.end());
                for(const auto* force:base.forces) {
                    const auto found=base.transform->force_spaces.find(force->id);
                    base.force_spaces.push_back(found==base.transform->force_spaces.end()?nullptr:&found->second);
                }
            }
            auto sources=incoming[id];std::sort(sources.begin(),sources.end());
            for(auto source:sources) {
                const auto* emitter=nodes.at(source);
                if(emitter->type_key!=kEmitterNode) return R::failure(ErrorCode::invalid_request,"Particle input requires Emitter");
                auto branch=base;branch.emitter=emitter;branch.partition=partitions[source]++;
                branches.push_back(std::move(branch));
            }
        }
        for(auto& branch:branches) branch.stride=partitions[branch.emitter->id];
        struct Candidate {double birth;ParticleInstance particle;};
        const auto older=[](const Candidate& a,const Candidate& b) {
            if(a.birth!=b.birth) return a.birth>b.birth;
            if(a.particle.emitter_id!=b.particle.emitter_id) return a.particle.emitter_id>b.particle.emitter_id;
            return a.particle.id>b.particle.id;
        };
        std::vector<Candidate> storage;storage.reserve(std::min<std::uint32_t>(cap,4096));
        std::priority_queue<Candidate,std::vector<Candidate>,decltype(older)> kept(older,std::move(storage));
        for(const auto& branch:branches) {
            auto& clock=clocks[branch.emitter->id];
            const auto* timing=find_value(*branch.emitter,kEmittingMode);
            if(timing && std::get<std::uint32_t>(*timing)>3)
                return R::failure(ErrorCode::invalid_request,"invalid Emitting mode");
            const bool once=timing && std::get<std::uint32_t>(*timing)==1;
            auto extended=clock.extend(branch.emitter->id,once?0:now,sampler,cancel,work,static_cast<unsigned>(hz));
            if(!extended.has_value()) return R::failure(extended.error());
            // t=0 has a birth iff the emitter is enabled at that instant.
            if(!once && now==0 && !clock.value().initialized()) {
                auto e=clock.extend(branch.emitter->id,1/hz,sampler,cancel,work,static_cast<unsigned>(hz));
                if(!e.has_value()) return R::failure(e.error());
            }
            double total=clock.value().integral(now);
            std::uint64_t last{};
            if(once) {
                auto initial=clock.value().analytic()?Result<double>::success(clock.value().initial_rate()):sampler.rate(branch.emitter->id,0);
                if(!initial.has_value()) return R::failure(initial.error());
                if(!std::isfinite(initial.value()) || initial.value()<0 || initial.value()>kMaxBirthRate)
                    return R::failure(ErrorCode::invalid_request,"Once batch count outside bounds");
                const auto count=static_cast<std::uint64_t>(std::floor(initial.value()));
                if(!count) continue;
                last=count-1;
            } else {
                if(clock.value().first_positive()<0 || clock.value().first_positive()>now) continue;
                if(!std::isfinite(total) || total>9'007'199'254'740'991.0)
                    return R::failure(ErrorCode::invalid_time,"emission ordinal exceeds exact range");
                last=static_cast<std::uint64_t>(std::floor(total+1e-10));
            }
            if(last<branch.partition) continue;
            std::uint64_t slot=last-(last-branch.partition)%branch.stride;
            const auto life_bound=sampler.lifetime_upper_bound(branch.particle->id);
            if(life_bound && (!std::isfinite(*life_bound) || *life_bound<0 || *life_bound>kMaxLifetimeSeconds))
                return R::failure(ErrorCode::invalid_request,"invalid certified Life bound");
            for(;;) {
                if(cancel.is_cancelled()) return R::failure(ErrorCode::cancelled,"birth history cancelled");
                if(++work>kTemporalWorkLimit) return R::failure(ErrorCode::work_limit_exceeded,"birth history work limit");
                const double birth=once?0:clock.value().birth(slot);
                if(birth<0 || birth>now+1e-9) return R::failure(ErrorCode::invalid_time,"birth inversion outside frame");
                if(now-birth>=life_bound.value_or(kMaxLifetimeSeconds) || (kept.size()==cap && birth<kept.top().birth)) break;
                // Expired births need one Life query, not every birth property.
                auto lifetime=sampler.lifetime(branch.particle->id,birth);
                if(!lifetime.has_value()) return R::failure(lifetime.error());
                if(!std::isfinite(lifetime.value()) || lifetime.value()<0 || lifetime.value()>kMaxLifetimeSeconds)
                    return R::failure(ErrorCode::invalid_request,"historical Life outside bounds");
                if(now-birth>=lifetime.value()) {
                    if(slot<branch.stride) break;
                    slot-=branch.stride;continue;
                }
                auto emitter_node=at(*branch.emitter,birth);if(!emitter_node.has_value()) return R::failure(emitter_node.error());
                if(const auto* mode=find_value(emitter_node.value(),kEmittingMode);mode && std::get<std::uint32_t>(*mode)>3)
                    return R::failure(ErrorCode::invalid_request,"invalid Emitting mode");
                for(auto key:{kEmitChance,kEmitLifeStart,kEmitLifeEnd,kInheritVelocity,kInheritSize,kInheritOpacity,kInheritColor})
                    if(const auto* value=find_value(emitter_node.value(),key);value &&
                       (!std::isfinite(std::get<double>(*value)) || std::get<double>(*value)<0 || std::get<double>(*value)>100))
                        return R::failure(ErrorCode::invalid_request,"Auxiliary percentage outside 0..100");
                auto emitter=read_emitter(emitter_node.value());if(!emitter.has_value()) return R::failure(emitter.error());
                auto particle_node=at(*branch.particle,birth);if(!particle_node.has_value()) return R::failure(particle_node.error());
                auto particle_values=read_particle(particle_node.value());if(!particle_values.has_value()) return R::failure(particle_values.error());
                const double life=particle_values.value().lifetime_seconds,age=std::max(0.0,now-birth);
                if(age<life) {
                    auto appearance=particle_values;
                    Settings settings=emitter.value().value;settings.particle_lifetime_seconds=life;
                    const auto percent=[&](ParameterKey key,double fallback) {const auto* v=find_value(emitter_node.value(),key);return v?std::get<double>(*v)/100:fallback;};
                    const auto* mode=find_value(emitter_node.value(),kAuxiliarySource);
                    const bool auxiliary=mode && std::get<std::uint32_t>(*mode)==1;
                    std::vector<ParticleInstance> parents;
                    if(auxiliary && !incoming[branch.emitter->id].empty()) {
                        Graph prefix;std::map<NodeId,bool> ancestors;stack=incoming[branch.emitter->id];
                        while(!stack.empty()) {auto next=stack.back();stack.pop_back();if(ancestors[next]) continue;ancestors[next]=true;
                            for(auto source:incoming[next]) stack.push_back(source);}
                        for(const auto& [id,n]:nodes) if(ancestors[id] || id==output->id) prefix.nodes.push_back(*n);
                        for(const auto& edge:graph.edges) {
                            if(ancestors[edge.source_node] && ancestors[edge.destination_node]) prefix.edges.push_back(edge);
                            else if(edge.destination_node==branch.emitter->id) prefix.edges.push_back({edge.id,edge.source_node,edge.source_port,output->id,kOutputParticles});
                        }
                        auto evaluated=evaluate(prefix,birth,depth+1);if(!evaluated.has_value()) return R::failure(evaluated.error());
                        parents=std::move(evaluated.value().particles);
                    } else if(!auxiliary && !incoming[branch.emitter->id].empty())
                        return R::failure(ErrorCode::invalid_request,"parent input requires Auxiliary");
                    if(!auxiliary) parents.push_back({});
                    for(const auto& parent:parents) {
                        if(++work>kTemporalWorkLimit) return R::failure(ErrorCode::work_limit_exceeded,"auxiliary history work limit");
                        auto own=settings;auto looks=appearance.value();std::uint64_t identity=slot;
                        if(auxiliary) {
                            const double fraction=parent.age_seconds/parent.lifetime_seconds;
                            const double start=percent(kEmitLifeStart,0),end=percent(kEmitLifeEnd,1);
                            if(start>end) return R::failure(ErrorCode::invalid_request,"invalid Auxiliary life interval");
                            if(fraction<start || fraction>=end) continue;
                            auto parent_key=mix64(parent.id);for(auto byte:parent.emitter_id.value.bytes) parent_key=mix64(parent_key^byte);
                            if(unit_value(settings.seed,parent_key,RandomPurpose::auxiliary_chance)>=percent(kEmitChance,1)) continue;
                            identity=mix64(parent_key^mix64(slot));
                            own.seed=static_cast<std::uint32_t>(mix64(identity^own.seed));
                            const auto blend=[](double a,double b,double f){return a+(b-a)*f;};
                            looks.size_start=blend(looks.size_start,parent.size_pixels,percent(kInheritSize,0));
                            looks.opacity_start=blend(looks.opacity_start,parent.opacity,percent(kInheritOpacity,0));
                            const double inherit=percent(kInheritColor,0);
                            looks.color_start={blend(looks.color_start.x,parent.color.x,inherit),blend(looks.color_start.y,parent.color.y,inherit),blend(looks.color_start.z,parent.color.z,inherit)};
                            looks.color_end={blend(looks.color_end.x,parent.color.x,inherit),blend(looks.color_end.y,parent.color.y,inherit),blend(looks.color_end.z,parent.color.z,inherit)};
                            for(std::size_t stop=0;stop<looks.gradient.count;++stop) {
                                auto& color=looks.gradient.stops[stop].color;
                                color={blend(color.x,parent.color.x,inherit),blend(color.y,parent.color.y,inherit),blend(color.z,parent.color.z,inherit)};
                            }
                        }
                        const double actual_life=birth_lifetime(particle_values.value(),own.seed,identity);
                        if(age>=actual_life) continue;
                        own.particle_lifetime_seconds=actual_life;
                        own.gravity={};own.linear_drag=0;own.forces.clear();
                        auto instance=simulate_particle_at_age(own,0,identity,dimensions);
                        instance.id=identity;instance.emitter_id=branch.emitter->id;instance.age_seconds=age;
                        if(auxiliary) {
                            instance.position.x+=parent.position.x;instance.position.y+=parent.position.y;instance.position.z+=parent.position.z;
                            const double inherit=percent(kInheritVelocity,0);
                            instance.velocity.x+=parent.velocity.x*inherit;instance.velocity.y+=parent.velocity.y*inherit;instance.velocity.z+=parent.velocity.z*inherit;
                        }
                        const auto* tf=branch.transform && branch.transform->combined?&*branch.transform->combined:nullptr;
                        if(tf){instance.position=tf->position(instance.position);instance.velocity=tf->velocity(instance.velocity);}
                        const Vec3 birth_position=instance.position;
                        auto moved=motion(instance,branch,birth,now,own.seed);if(!moved.has_value()) return R::failure(moved.error());
                        apply_particle_style(instance,looks,own.seed);
                        apply_particle_properties(instance,particle_values.value(),own.seed,birth_position,tf);
                        if(branch.transform)apply_transform_style(instance,*branch.transform);
                        Candidate candidate{birth,std::move(instance)};
                        if(kept.size()<cap) kept.push(std::move(candidate));
                        else if(older(candidate,kept.top())) {kept.pop();kept.push(std::move(candidate));}
                    }
                }
                if(slot<branch.stride) break;
                slot-=branch.stride;
            }
        }
        result.particles.resize(kept.size());
        // Heap pops oldest first; composition order is stable across frames.
        for(std::size_t i=0;!kept.empty();++i) {result.particles[i]=kept.top().particle;kept.pop();}
        return R::success(std::move(result));
    }
};
} // namespace

Result<EvaluatedGraph> evaluate_temporal_particle_graph(const Graph& graph,RationalTime time,
    const Cancellation& cancellation,EmitterDimensionContext dimensions,TemporalGraphSampler& sampler) {
    using R=Result<EvaluatedGraph>;
    try {
        if(!time.scale || !std::isfinite(dimensions.layer_height_pixels) || dimensions.layer_height_pixels<=0 ||
            !std::isfinite(dimensions.pixel_aspect_ratio) || dimensions.pixel_aspect_ratio<=0)
            return R::failure(ErrorCode::invalid_request,"invalid temporal evaluation context");
        double now=double(time.value)/time.scale;
        unsigned frequency=30;
        for(const auto& node:graph.nodes) if(node.type_key==kOutputNode) {
            if(const auto* value=find_value(node,kTimeSamplingHz)) frequency=std::get<std::uint32_t>(*value);
            const auto* flag=find_value(node,kTimeRemapEnabled);const auto* clock=find_value(node,kTimeRemapSeconds);
            if(flag && clock && std::get<std::uint32_t>(*flag)==1) now=std::get<double>(*clock);
        }
        if(!std::isfinite(now)) return R::failure(ErrorCode::invalid_time,"invalid temporal time");
        if(frequency!=30 && frequency!=60 && frequency!=120)
            return R::failure(ErrorCode::invalid_request,"time sampling must be 30, 60 or 120 Hz");
        TemporalEvaluator evaluator(sampler,cancellation,dimensions,frequency);
        return evaluator.evaluate(graph,now);
    } catch(const std::bad_alloc&) {return R::failure(ErrorCode::allocation_failed,"temporal allocation failed");}
      catch(...) {return R::failure(ErrorCode::internal_failure,"temporal evaluation failed");}
}
