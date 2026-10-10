// Private implementation included inside GraphEvaluation.cpp's anonymous
// namespace, after its typed parameter/style readers. No host dependencies.
struct TransformPlanningBudget {std::uint64_t work{},matrices{};};
struct TransformBranchPlan {
    bool terminal{};
    std::optional<CompiledParticleTransform> combined;
    std::map<NodeId,CompiledParticleTransform> force_spaces;
    std::vector<NodeId> force_nodes;
    std::uint32_t sprite_basis_index{};
};
Result<CompiledParticleTransform> read_transform(const GraphNode& node) {
    using R=Result<CompiledParticleTransform>;
    ParticleTransformSettings settings;
    if(const auto* raw=find_value(node,kTransformInheritLayer)) {
        const auto* resource=std::get_if<std::uint32_t>(raw);
        if(!resource || *resource>0x7fffffffu || (*resource && !find_value(node,kTransformInheritedMatrix)))
            return R::failure(ErrorCode::invalid_request,"unresolved Transform layer resource");
    }
    for(const auto item:{std::pair{kTransformAnchor,&settings.anchor},std::pair{kTransformPosition,&settings.position},
        std::pair{kTransformRotation,&settings.rotation_degrees},std::pair{kTransformSystemScale,&settings.scale_percent}}) {
        const auto* raw=find_value(node,item.first);const auto* value=raw?std::get_if<Vec3>(raw):nullptr;
        if(!value)return R::failure(ErrorCode::invalid_request,"missing or invalid Transform vector");
        *item.second=*value;
    }
    for(const auto item:{std::pair{kTransformParticleScale,&settings.particles_scale_percent},
        std::pair{kTransformParticleOpacity,&settings.particles_opacity_percent}}) {
        const auto* raw=find_value(node,item.first);const auto* value=raw?std::get_if<double>(raw):nullptr;
        if(!value)return R::failure(ErrorCode::invalid_request,"missing or invalid Transform percentage");
        *item.second=*value;
    }
    if(const auto* raw=find_value(node,kTransformInheritedMatrix)) {
        const auto* bytes=std::get_if<OpaqueBytes>(raw);
        if(!bytes || bytes->size()!=132 || (*bytes)[0]!=std::byte{1} || (*bytes)[1]!=std::byte{0} ||
            (*bytes)[2]!=std::byte{0} || (*bytes)[3]!=std::byte{0})
            return R::failure(ErrorCode::invalid_request,"invalid sampled Transform matrix header");
        for(unsigned i=0;i<16;++i) {
            std::uint64_t bits=0;for(unsigned b=0;b<8;++b)bits|=std::uint64_t(std::to_integer<unsigned char>((*bytes)[4+i*8+b]))<<(8*b);
            settings.inherited_motion[i]=std::bit_cast<double>(bits);
        }
    }
    return compile_particle_transform(settings);
}
Result<TransformBranchPlan> plan_transform_branch(const std::vector<const GraphNode*>& nodes,
    const std::vector<std::vector<std::size_t>>& incoming,const std::vector<std::vector<std::size_t>>& outgoing,
    const std::vector<bool>& active,const std::vector<std::size_t>& order,std::size_t root,std::size_t output,
    const std::vector<std::optional<CompiledParticleTransform>>& transforms,
    const Cancellation& cancel,TransformPlanningBudget& budget,std::span<const NodeId> deferred={}) {
    using R=Result<TransformBranchPlan>;
    const auto count=nodes.size();std::vector<bool> reachable(count,false),terminal_path(count,false),ordered(count,false);
    // Both private callers provide UUID-sorted nodes. Deferred stages are
    // applied to the per-particle frame after the common simulated prefix.
    for(auto id:deferred){
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform ordered planning cancelled");
        if(++budget.work>kMaxBranchTraversalWork)return R::failure(ErrorCode::work_limit_exceeded,"Transform ordered planning work limit");
        const auto found=std::lower_bound(nodes.begin(),nodes.end(),id,[](auto* n,NodeId key){return n->id<key;});
        if(found==nodes.end()||(*found)->id!=id)return R::failure(ErrorCode::invalid_request,"missing ordered Transform node");
        ordered[static_cast<std::size_t>(found-nodes.begin())]=true;
    }
    std::vector<std::size_t> stack{root};
    while(!stack.empty()) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform branch planning cancelled");
        auto current=stack.back();stack.pop_back();if(reachable[current])continue;reachable[current]=true;
        for(auto next:outgoing[current]) {
            if(++budget.work>kMaxBranchTraversalWork)return R::failure(ErrorCode::work_limit_exceeded,"Transform branch planning exceeds work limit");
            if(active[next] && nodes[next]->type_key!=kEmitterNode)stack.push_back(next);
        }
    }
    TransformBranchPlan result;if(!reachable[output])return R::success(std::move(result));
    result.terminal=true;stack={output};
    while(!stack.empty()) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform terminal planning cancelled");
        auto current=stack.back();stack.pop_back();if(terminal_path[current])continue;terminal_path[current]=true;
        for(auto previous:incoming[current]) {
            if(++budget.work>kMaxBranchTraversalWork)return R::failure(ErrorCode::work_limit_exceeded,"Transform terminal planning exceeds work limit");
            if(reachable[previous])stack.push_back(previous);
        }
    }
    struct Context {std::size_t parent,node;};
    std::vector<Context> contexts{{0,count}};
    std::vector<std::size_t> context_at(count,count);context_at[root]=0;
    std::map<NodeId,std::size_t> force_contexts;
    for(auto current:order) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform context planning cancelled");
        if(!terminal_path[current] || context_at[current]==count)continue;
        auto context=context_at[current];
        if(nodes[current]->type_key==kForceNode) {
            force_contexts.emplace(nodes[current]->id,context);result.force_nodes.push_back(nodes[current]->id);
        }
        if(transforms[current]&&!ordered[current]){contexts.push_back({context,current});context=contexts.size()-1;}
        if(current==output)context_at[output]=context;
        for(auto next:outgoing[current])if(terminal_path[next]) {
            if(++budget.work>kMaxBranchTraversalWork)return R::failure(ErrorCode::work_limit_exceeded,"Transform context planning exceeds work limit");
            if(context_at[next]!=count && context_at[next]!=context)
                return R::failure(ErrorCode::invalid_request,"cannot merge different Transform chains of one Particle stream");
            context_at[next]=context;
        }
    }
    std::map<std::size_t,std::optional<CompiledParticleTransform>> suffixes;
    auto context=context_at[output];suffixes.emplace(context,std::nullopt);
    while(context!=0) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform composition cancelled");
        if(++budget.matrices>65536)return R::failure(ErrorCode::work_limit_exceeded,"Transform derived matrix limit exceeded");
        const auto& token=contexts[context];auto combined=*transforms[token.node];
        if(suffixes.at(context)) {
            auto composed=compose_particle_transforms(combined,*suffixes.at(context));
            if(!composed.has_value())return R::failure(composed.error());combined=composed.take_value();
        }
        context=token.parent;suffixes.emplace(context,std::move(combined));
    }
    result.combined=suffixes.at(0);
    for(const auto& [id,at]:force_contexts) {
        if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"Transform Force planning cancelled");
        const auto found=suffixes.find(at);
        if(found==suffixes.end())return R::failure(ErrorCode::invalid_request,"Force has an ambiguous Transform frame");
        if(found->second) {
            if(++budget.matrices>65536)return R::failure(ErrorCode::work_limit_exceeded,"Transform derived matrix limit exceeded");
            result.force_spaces.emplace(id,*found->second);
        }
    }
    return R::success(std::move(result));
}
Result<std::uint32_t> retain_transform_basis(EvaluatedGraph& graph,const TransformBranchPlan& plan) {
    if(!plan.combined || plan.combined->particle_basis()==ParticleSpriteBasis{1,0,0,0,1,0,0,0,1})
        return Result<std::uint32_t>::success(0);
    if(graph.sprite_bases.size()==kMaxParticleSpriteBases)
        return Result<std::uint32_t>::failure(ErrorCode::work_limit_exceeded,"Transform sprite table limit exceeded");
    graph.sprite_bases.push_back(plan.combined->particle_basis());
    return Result<std::uint32_t>::success(static_cast<std::uint32_t>(graph.sprite_bases.size()));
}
void map_transform_forces(Settings& settings,const TransformBranchPlan& plan,
    const std::vector<NodeId>& ids,Vec3 original_gravity={}) {
    settings.gravity=plan.combined?plan.combined->velocity(original_gravity):original_gravity;
    for(std::size_t i=0;i<ids.size();++i) {
        auto& force=settings.forces[i];
        if(const auto found=plan.force_spaces.find(ids[i]);found!=plan.force_spaces.end()) {
            force.gravity=found->second.velocity(force.gravity);force.wind=found->second.velocity(force.wind);
            force.spin_axis_x=found->second.velocity(force.spin_axis_x);force.spin_axis_y=found->second.velocity(force.spin_axis_y);
        }
        settings.gravity.x+=force.gravity.x;settings.gravity.y+=force.gravity.y;settings.gravity.z+=force.gravity.z;
    }
}
void apply_transform_style(ParticleInstance& instance,const TransformBranchPlan& plan) noexcept {
    if(plan.combined) {
        instance.size_pixels*=plan.combined->particle_scale();instance.size_y_pixels*=plan.combined->particle_scale();
        instance.opacity*=plan.combined->particle_opacity();
    }
    instance.sprite_basis_index=plan.sprite_basis_index;
}
