// Private graph evaluator implementation. No SDK type or retained host lifetime.
struct OrderedForceResponse {
    Vec3 displacement{},velocity{};
    double damping_weight{1},damping_rate{};
};
Result<Settings> ordered_force_settings(const ParticleInstance& p,std::uint32_t seed,
    std::span<const ForceValues> fields) {
    Settings settings;settings.velocity={};settings.velocity_spread=0;
    settings.emitter_origin={};settings.emitter_shape=EmitterShape::point;
    settings.seed=seed;settings.particle_lifetime_seconds=p.lifetime_seconds;
    settings.forces.reserve(fields.size());
    for(const auto& field:fields){
        settings.gravity.x+=field.gravity.x;settings.gravity.y+=field.gravity.y;settings.gravity.z+=field.gravity.z;
        settings.linear_drag+=field.linear_drag;
        settings.forces.push_back(motion_for_emitter(field.motion,p.emitter_id));
    }
    auto checked=validate_settings(settings);
    if(!checked.notices.empty())return Result<Settings>::failure(ErrorCode::invalid_request,"ordered Force values exceed bounds");
    return Result<Settings>::success(std::move(checked.value));
}
bool charge_ordered_force(const Settings& settings,std::uint64_t& work,unsigned queries=1) noexcept {
    work+=queries;
    for(const auto& field:settings.forces)work+=queries*std::max<unsigned>(1,field.wind_spin_curve.count);
    return work<=20'000'000;
}
OrderedForceResponse static_ordered_force_response(const Settings& settings,const ParticleInstance& particle,
    EmitterDimensionContext dimensions) noexcept {
    if(particle.age_seconds<=0)return {};
    const auto response=simulate_particle_at_age(settings,particle.age_seconds,particle.id,dimensions);
    const auto age=particle.age_seconds,drag=settings.linear_drag;
    const auto z=drag*age;
    const auto weight=std::abs(z)<1e-4?1-z/2+z*z/6-z*z*z/24+z*z*z*z/120:-std::expm1(-z)/z;
    const auto rate=std::abs(z)<1e-4?drag*(-.5+z/3-z*z/8+z*z*z/30):(std::exp(-z)-weight)/age;
    return {response.position,response.velocity,weight,rate};
}
Result<bool> apply_ordered_force_response(ParticleInstance& particle,Vec3 birth,const OrderedForceResponse& response) noexcept {
    using R=Result<bool>;
    if(!motion_circle_detail::bounded(birth)||!motion_circle_detail::bounded(response.displacement)||
       !motion_circle_detail::bounded(response.velocity)||!std::isfinite(response.damping_weight)||
       !std::isfinite(response.damping_rate))return R::failure(ErrorCode::invalid_request,"ordered Force response exceeds bounds");
    const Vec3 delta{particle.position.x-birth.x,particle.position.y-birth.y,particle.position.z-birth.z};
    auto next=particle;const auto f=response.damping_weight,d=response.damping_rate;
    if(f!=1)next.position={birth.x+f*delta.x,birth.y+f*delta.y,birth.z+f*delta.z};
    if(response.displacement.x!=0)next.position.x+=response.displacement.x;
    if(response.displacement.y!=0)next.position.y+=response.displacement.y;
    if(response.displacement.z!=0)next.position.z+=response.displacement.z;
    if(f!=1||d!=0)next.velocity={f*particle.velocity.x+d*delta.x,f*particle.velocity.y+d*delta.y,f*particle.velocity.z+d*delta.z};
    if(response.velocity.x!=0)next.velocity.x+=response.velocity.x;
    if(response.velocity.y!=0)next.velocity.y+=response.velocity.y;
    if(response.velocity.z!=0)next.velocity.z+=response.velocity.z;
    if(!motion_circle_detail::bounded(next.position)||!motion_circle_detail::bounded(next.velocity))
        return R::failure(ErrorCode::invalid_request,"ordered Force state exceeds bounds");
    particle=next;return R::success(true);
}
