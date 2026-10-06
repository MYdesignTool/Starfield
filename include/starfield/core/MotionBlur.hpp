#pragma once
#include "starfield/core/GraphEvaluation.hpp"
#include <algorithm>
#include <map>
#include <cmath>
#include <limits>
#include <new>

namespace starfield::core {
// Endpoint interpolation is an approximation; exact subframes use the temporal
// evaluator. Birth/death visibility remains half-open even on this fast path.
inline Result<EvaluatedGraph> interpolate_motion_particles(const EvaluatedGraph& first,
    const EvaluatedGraph& last,double first_clock,double last_clock,double amount,
    std::uint32_t limit,const Cancellation& cancel) try {
    using R=Result<EvaluatedGraph>;
    if(!std::isfinite(first_clock) || !std::isfinite(last_clock) ||
       !std::isfinite(amount) || amount<0 || amount>1 || limit>kMaxParticleCount ||
       first.particles.size()>kMaxParticleCount || last.particles.size()>kMaxParticleCount ||
       first.sprite_bases.size()>kMaxParticleSpriteBases || last.sprite_bases.size()>kMaxParticleSpriteBases)
        return R::failure(ErrorCode::invalid_request,"invalid motion interpolation input");
    for(const auto* endpoint:{&first,&last})for(const auto& basis:endpoint->sprite_bases)
        if(!valid_particle_sprite_basis(basis))return R::failure(ErrorCode::invalid_request,"invalid motion sprite basis");
    using Identity=std::pair<NodeId,std::uint64_t>;
    std::map<Identity,std::pair<const ParticleInstance*,const ParticleInstance*>> pairs;
    std::size_t visited=0;
    for(const auto& p:first.particles) {
        if((visited++&4095)==0 && cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
        if(p.sprite_basis_index>first.sprite_bases.size())return R::failure(ErrorCode::invalid_request,"missing first motion sprite basis");
        pairs[{p.emitter_id,p.id}].first=&p;
    }
    for(const auto& p:last.particles) {
        if((visited++&4095)==0 && cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
        if(p.sprite_basis_index>last.sprite_bases.size())return R::failure(ErrorCode::invalid_request,"missing last motion sprite basis");
        pairs[{p.emitter_id,p.id}].second=&p;
    }
    EvaluatedGraph output;output.evaluated_nodes=first.evaluated_nodes;
    output.particles.reserve(std::min<std::size_t>(pairs.size(),limit));
    // Time remapping may run backwards. std::lerp also avoids overflowing the
    // endpoint difference for otherwise finite clocks.
    const double clock=std::lerp(first_clock,last_clock,amount);
    const auto lerp=[&](double a,double b){return a+(b-a)*amount;};
    const auto vector=[&](Vec3 a,Vec3 b){return Vec3{lerp(a.x,b.x),lerp(a.y,b.y),lerp(a.z,b.z)};};
    // An index is meaningful only with its endpoint table. Remap each pair once,
    // including one-endpoint births/deaths, instead of copying matrices per slot.
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t> basis_pairs;
    constexpr auto absent=std::numeric_limits<std::uint32_t>::max();
    constexpr ParticleSpriteBasis identity_basis{1,0,0,0,1,0,0,0,1};
    const auto basis_at=[&](const EvaluatedGraph& graph,std::uint32_t at)->const ParticleSpriteBasis& {
        return at?graph.sprite_bases[at-1]:identity_basis;
    };
    std::size_t index=0;
    for(const auto& [identity,pair]:pairs) {
        if((index++&63)==0 && cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
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
        if(p.age_seconds>=0 && p.age_seconds<p.lifetime_seconds) {
            if((a && a->sprite_basis_index) || (b && b->sprite_basis_index)) {
                const auto key=std::pair{a?a->sprite_basis_index:absent,b?b->sprite_basis_index:absent};
                auto found=basis_pairs.find(key);
                if(found==basis_pairs.end()) {
                    if(output.sprite_bases.size()==kMaxParticleSpriteBases)
                        return R::failure(ErrorCode::work_limit_exceeded,"motion sprite basis table exceeds limit");
                    auto basis=a?basis_at(first,a->sprite_basis_index):basis_at(last,b->sprite_basis_index);
                    if(a && b)for(std::size_t j=0;j<basis.size();++j)basis[j]=lerp(basis[j],basis_at(last,b->sprite_basis_index)[j]);
                    if(!valid_particle_sprite_basis(basis))return R::failure(ErrorCode::invalid_request,"invalid interpolated sprite basis");
                    output.sprite_bases.push_back(basis);
                    found=basis_pairs.emplace(key,static_cast<std::uint32_t>(output.sprite_bases.size())).first;
                }
                p.sprite_basis_index=found->second;
            } else p.sprite_basis_index=0;
            output.particles.push_back(p);
        }
    }
    if(output.particles.size()>limit) {
        std::stable_sort(output.particles.begin(),output.particles.end(),[](const auto& a,const auto& b){return a.age_seconds<b.age_seconds;});
        output.particles.resize(limit);
    }
    if(cancel.is_cancelled())return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
    return R::success(std::move(output));
} catch(const std::bad_alloc&) {return Result<EvaluatedGraph>::failure(ErrorCode::allocation_failed,"motion interpolation allocation failed");}
}
