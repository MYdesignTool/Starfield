// Private implementation included in GraphEvaluation.cpp's anonymous namespace.
Result<std::uint32_t> read_motion_mode(const GraphNode& node) {
    const auto* raw=find_value(node,kMotionMode);const auto* mode=raw?std::get_if<std::uint32_t>(raw):nullptr;
    if(!mode||*mode>2)return Result<std::uint32_t>::failure(ErrorCode::invalid_request,"Motion mode has no evaluator");
    return Result<std::uint32_t>::success(*mode);
}
constexpr std::size_t kMotionPathRequestBytes=32*1024*1024;
struct MotionPathClockValue {
    MotionPathTravelSettings settings;
    CompiledMotionCurveClock clock;
    Result<std::pair<double,double>> distance_rate(double age,double life,double random) const noexcept {
        using R=Result<std::pair<double,double>>;
        auto endpoint=clock.clock(age,life);if(!endpoint.has_value())return R::failure(endpoint.error());
        if(age<settings.delay_seconds||settings.delay_seconds>=life)return R::success({0,0});
        auto interval=clock.integral_between(settings.delay_seconds,age,life);if(!interval.has_value())return R::failure(interval.error());
        const auto speed=settings.units_per_second*(1-settings.speed_random_percent/100*random);
        return R::success({speed*interval.value(),speed*endpoint.value().weight});
    }
};
Result<MotionPathClockValue> compile_motion_path_clock(const MotionPathTravelSettings& s) {
    using R=Result<MotionPathClockValue>;
    if(!std::isfinite(s.units_per_second)||std::abs(s.units_per_second)>1e6||
        !std::isfinite(s.delay_seconds)||s.delay_seconds<0||s.delay_seconds>1e6||
        !std::isfinite(s.speed_random_percent)||s.speed_random_percent<0||s.speed_random_percent>100||
        !motion_circle_detail::bounded(s.forward)||(s.orient_to_path&&s.forward.x==0&&s.forward.y==0&&s.forward.z==0))
        return R::failure(ErrorCode::invalid_request,"invalid Light Path numeric clock");
    auto clock=compile_motion_curve_clock(s.over_life);if(!clock.has_value())return R::failure(clock.error());
    return R::success({s,clock.take_value()});
}
Result<MotionPathClockValue> read_motion_path_clock(const GraphNode& node) {
    using R=Result<MotionPathClockValue>;MotionPathTravelSettings s;
    auto mode=read_motion_mode(node);if(!mode.has_value()||mode.value()!=0)return R::failure(ErrorCode::invalid_request,"historical Light Path mode changed");
    const auto* raw=find_value(node,kMotionPathSpeed);const auto* speed=raw?std::get_if<double>(raw):nullptr;
    if(!speed)return R::failure(ErrorCode::invalid_request,"missing Light Path numeric speed");s.units_per_second=*speed;
    for(auto [key,destination]:{std::pair{kMotionPathDelay,&s.delay_seconds},std::pair{kMotionSpeedRandom,&s.speed_random_percent}})
        if(const auto* v=find_value(node,key)){const auto* number=std::get_if<double>(v);if(!number)return R::failure(ErrorCode::invalid_request,"invalid Light Path numeric value");*destination=*number;}
    if(const auto* v=find_value(node,kMotionPathOrient)){const auto* flag=std::get_if<std::uint32_t>(v);
        if(!flag||*flag>1)return R::failure(ErrorCode::invalid_request,"invalid Light Path orient flag");s.orient_to_path=*flag==1;}
    if(const auto* v=find_value(node,kMotionForward)){const auto* forward=std::get_if<Vec3>(v);
        if(!forward)return R::failure(ErrorCode::invalid_request,"invalid Light Path forward");s.forward=*forward;}
    else if(s.orient_to_path)return R::failure(ErrorCode::invalid_request,"missing Light Path forward");
    if(const auto* v=find_value(node,kMotionOverLife)){const auto* bytes=std::get_if<OpaqueBytes>(v);
        if(!bytes||!decode_age_curve(*bytes,s.over_life,0,1000))return R::failure(ErrorCode::invalid_request,"invalid Light Path Over Life");}
    return compile_motion_path_clock(s);
}
class MotionPathWorkCancellation final:public Cancellation {
    const Cancellation& source_;std::uint64_t& work_;mutable bool exceeded_{};
public:
    MotionPathWorkCancellation(const Cancellation& source,std::uint64_t& work):source_(source),work_(work){}
    bool is_cancelled()const noexcept override {
        if(source_.is_cancelled())return true;
        if(++work_>20'000'000){exceeded_=true;return true;}return false;
    }
    bool exceeded()const noexcept{return exceeded_;}
};
Result<const OpaqueBytes*> read_motion_path_points(const GraphNode& node,const Cancellation& cancel,std::uint64_t& work) {
    using R=Result<const OpaqueBytes*>;const auto* raw=find_value(node,kMotionPathPoints);const auto* bytes=raw?std::get_if<OpaqueBytes>(raw):nullptr;
    if(!bytes)return R::failure(ErrorCode::invalid_request,"missing Light Path points");
    MotionPathWorkCancellation charged(cancel,work);auto checked=validate_motion_path_points(*bytes,charged);
    if(charged.exceeded())return R::failure(ErrorCode::work_limit_exceeded,"Light Path request work limit");
    return checked.has_value()?R::success(bytes):R::failure(checked.error());
}
Result<CompiledMotionPathTravel> compile_motion_graph_path(const GraphNode& node,const MotionPathTravelSettings& settings,
    const Cancellation& cancel,std::uint64_t& work,std::size_t& bytes_used) {
    using R=Result<CompiledMotionPathTravel>;
    auto bytes=read_motion_path_points(node,cancel,work);if(!bytes.has_value())return R::failure(bytes.error());
    MotionPathWorkCancellation charged(cancel,work);auto points=decode_motion_path_points(*bytes.value(),charged);
    if(charged.exceeded())return R::failure(ErrorCode::work_limit_exceeded,"Light Path request work limit");
    if(!points.has_value())return R::failure(points.error());
    auto compiled=compile_motion_path_travel(points.value(),settings,charged);
    if(charged.exceeded())return R::failure(ErrorCode::work_limit_exceeded,"Light Path request work limit");
    if(!compiled.has_value())return compiled;
    const auto storage=compiled.value().storage_bytes();
    if(storage>kMotionPathRequestBytes-bytes_used)return R::failure(ErrorCode::work_limit_exceeded,"Light Path request storage limit");
    bytes_used+=storage;return compiled;
}
Result<MotionLookAtSettings> read_motion_look_at(const GraphNode& node) {
    using R=Result<MotionLookAtSettings>;MotionLookAtSettings s;
    for(auto [key,destination]:{std::pair{kMotionGoal,&s.goal},std::pair{kMotionForward,&s.forward}}) {
        const auto* raw=find_value(node,key);const auto* value=raw?std::get_if<Vec3>(raw):nullptr;
        if(!value||!motion_circle_detail::bounded(*value))return R::failure(ErrorCode::invalid_request,"missing/invalid Look At goal/forward");*destination=*value;
    }
    if(s.forward.x==0&&s.forward.y==0&&s.forward.z==0)return R::failure(ErrorCode::invalid_request,"Look At forward must be nonzero");
    if(const auto* raw=find_value(node,kMotionOverLife)) {
        const auto* bytes=std::get_if<OpaqueBytes>(raw);if(!bytes||!decode_age_curve(*bytes,s.over_life,0,1000))
            return R::failure(ErrorCode::invalid_request,"invalid Look At Over Life curve");
    }
    return R::success(std::move(s));
}
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
        if(context && type==kForceNode)
            return R::failure(ErrorCode::invalid_request,"Force after Motion requires ordered-force integration");
        if(type==kMotionNode||(context&&type==kTransformNode)){
            contexts.push_back({context,nodes[at]->id});context=contexts.size()-1;}
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
