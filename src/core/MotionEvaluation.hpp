// Private implementation included in GraphEvaluation.cpp's anonymous namespace.
Result<CompiledMotionCircle> read_motion_circle(const GraphNode& node) {
    using R=Result<CompiledMotionCircle>;MotionCircleSettings s;
    const auto* mode=find_value(node,kMotionMode);
    if(!mode||!std::holds_alternative<std::uint32_t>(*mode)||std::get<std::uint32_t>(*mode)!=1)
        return R::failure(ErrorCode::invalid_request,"Motion mode has no evaluator in this Circle substage");
    for(auto [key,destination]:{std::pair{kMotionOrigin,&s.origin},std::pair{kMotionAxis,&s.axis}}) {
        const auto* raw=find_value(node,key);const auto* value=raw?std::get_if<Vec3>(raw):nullptr;
        if(!value)return R::failure(ErrorCode::invalid_request,"invalid Motion Circle vector");*destination=*value;
    }
    const auto* rate=find_value(node,kMotionAngularRate);const auto* value=rate?std::get_if<double>(rate):nullptr;
    if(!value)return R::failure(ErrorCode::invalid_request,"missing Motion Circle angular rate");s.radians_per_second=*value;
    if(const auto* raw=find_value(node,kMotionSpeedRandom)) {
        const auto* random=std::get_if<double>(raw);if(!random)return R::failure(ErrorCode::invalid_request,"invalid Motion speed random");
        s.speed_random_percent=*random;
    }
    if(const auto* raw=find_value(node,kMotionOverLife)) {
        const auto* bytes=std::get_if<OpaqueBytes>(raw);
        if(!bytes||!decode_age_curve(*bytes,s.over_life,0,1000))return R::failure(ErrorCode::invalid_request,"invalid Motion Over Life curve");
    }
    return compile_motion_circle(s);
}
double motion_speed_random(const ParticleInstance& p,NodeId motion,std::uint32_t seed) noexcept {
    auto identity=p.id;
    for(auto byte:p.emitter_id.value.bytes)identity=mix64(identity^byte);
    for(auto byte:motion.value.bytes)identity=mix64(identity^byte);
    return unit_value(seed,identity,RandomPurpose::motion_speed);
}
double motion_speed_factor(const ParticleInstance& p,NodeId motion,std::uint32_t seed,double percent) noexcept {
    return 1-percent/100*motion_speed_random(p,motion,seed);
}
Result<bool> apply_circle_state(ParticleInstance& p,const CompiledMotionCircle& c,double angle,double omega) {
    auto state=c.apply(p.position,p.velocity,angle,omega);
    if(!state.has_value())return Result<bool>::failure(state.error());
    p.position=state.value().position;p.velocity=state.value().velocity;return Result<bool>::success(true);
}
Result<std::vector<NodeId>> plan_motion_chain(const std::vector<const GraphNode*>& nodes,
    const std::vector<std::vector<std::size_t>>& incoming,const std::vector<std::vector<std::size_t>>& outgoing,
    const std::vector<bool>& active,const std::vector<std::size_t>& order,std::size_t root,std::size_t output,
    const Cancellation& cancel,std::uint64_t& work) {
    using R=Result<std::vector<NodeId>>;const auto count=nodes.size();
    std::vector<bool> reachable(count),terminal(count);std::vector<std::size_t> stack{root};
    const auto step=[&](){return ++work<=20'000'000;};
    while(!stack.empty()) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion planning cancelled");
        auto at=stack.back();stack.pop_back();if(reachable[at])continue;reachable[at]=true;
        for(auto next:outgoing[at]){if(!step())return R::failure(ErrorCode::work_limit_exceeded,"Motion planning work limit");
            if(active[next]&&nodes[next]->type_key!=kEmitterNode)stack.push_back(next);}
    }
    if(!reachable[output])return R::success({});stack={output};
    while(!stack.empty()) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion terminal planning cancelled");
        auto at=stack.back();stack.pop_back();if(terminal[at])continue;terminal[at]=true;
        for(auto prev:incoming[at]){if(!step())return R::failure(ErrorCode::work_limit_exceeded,"Motion planning work limit");if(reachable[prev])stack.push_back(prev);}
    }
    struct Context {std::size_t parent;NodeId id;};std::vector<Context> contexts{{0,{}}};
    std::vector<std::size_t> context_at(count,count);context_at[root]=0;
    for(auto at:order) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Motion context planning cancelled");
        if(!terminal[at]||context_at[at]==count)continue;auto context=context_at[at];
        const auto& type=nodes[at]->type_key;
        if(context && (type==kForceNode||type==kTransformNode))
            return R::failure(ErrorCode::invalid_request,"Force/Transform after Motion requires ordered-frame integration");
        if(type==kMotionNode){contexts.push_back({context,nodes[at]->id});context=contexts.size()-1;}
        if(at==output)context_at[output]=context;
        for(auto next:outgoing[at])if(terminal[next]) {
            if(!step())return R::failure(ErrorCode::work_limit_exceeded,"Motion planning work limit");
            if(context_at[next]!=count&&context_at[next]!=context)
                return R::failure(ErrorCode::invalid_request,"cannot merge different Motion chains of one Particle stream");
            context_at[next]=context;
        }
    }
    std::vector<NodeId> chain;
    for(auto context=context_at[output];context;context=contexts[context].parent)chain.push_back(contexts[context].id);
    std::reverse(chain.begin(),chain.end());return R::success(std::move(chain));
}
