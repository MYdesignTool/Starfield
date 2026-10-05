#pragma once
#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <map>

namespace starfield::core {
// Endpoint interpolation is an approximation; exact subframes use the temporal
// evaluator. Birth/death visibility remains half-open even on this fast path.
inline EvaluatedGraph interpolate_motion_particles(const EvaluatedGraph& first,
    const EvaluatedGraph& last,double first_clock,double last_clock,double amount,
    std::uint32_t limit,const Cancellation& cancel) {
    using Identity=std::pair<NodeId,std::uint64_t>;
    std::map<Identity,std::pair<const ParticleInstance*,const ParticleInstance*>> pairs;
    std::size_t visited=0;
    for(const auto& p:first.particles) {
        if((visited++&4095)==0 && cancel.is_cancelled())return {};
        pairs[{p.emitter_id,p.id}].first=&p;
    }
    for(const auto& p:last.particles) {
        if((visited++&4095)==0 && cancel.is_cancelled())return {};
        pairs[{p.emitter_id,p.id}].second=&p;
    }
    EvaluatedGraph output;output.evaluated_nodes=first.evaluated_nodes;
    output.particles.reserve(std::min<std::size_t>(pairs.size(),limit));
    const double clock=first_clock+(last_clock-first_clock)*amount;
    const auto lerp=[&](double a,double b){return a+(b-a)*amount;};
    const auto vector=[&](Vec3 a,Vec3 b){return Vec3{lerp(a.x,b.x),lerp(a.y,b.y),lerp(a.z,b.z)};};
    std::size_t index=0;
    for(const auto& [identity,pair]:pairs) {
        if((index++&63)==0 && cancel.is_cancelled())return {};
        const auto* a=pair.first;const auto* b=pair.second;
        auto p=*(a?a:b);
        if(a && b) {
            p.position=vector(a->position,b->position);p.velocity=vector(a->velocity,b->velocity);
            p.age_seconds=lerp(a->age_seconds,b->age_seconds);
            p.lifetime_seconds=lerp(a->lifetime_seconds,b->lifetime_seconds);
            p.size_pixels=lerp(a->size_pixels,b->size_pixels);p.size_y_pixels=lerp(a->size_y_pixels,b->size_y_pixels);
            p.opacity=lerp(a->opacity,b->opacity);p.color=vector(a->color,b->color);
            p.rotation_degrees=vector(a->rotation_degrees,b->rotation_degrees);
            p.feather_percent=lerp(a->feather_percent,b->feather_percent);
            p.anchor_x_percent=lerp(a->anchor_x_percent,b->anchor_x_percent);
            p.anchor_y_percent=lerp(a->anchor_y_percent,b->anchor_y_percent);
        } else {
            const auto dt=clock-(a?first_clock:last_clock);
            p.age_seconds+=dt;
            p.position.x+=p.velocity.x*dt;p.position.y+=p.velocity.y*dt;p.position.z+=p.velocity.z*dt;
        }
        if(p.age_seconds>=0 && p.age_seconds<p.lifetime_seconds)output.particles.push_back(p);
    }
    if(output.particles.size()>limit) {
        std::stable_sort(output.particles.begin(),output.particles.end(),[](const auto& a,const auto& b){return a.age_seconds<b.age_seconds;});
        output.particles.resize(limit);
    }
    return output;
}
}
