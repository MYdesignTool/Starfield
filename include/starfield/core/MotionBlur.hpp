#pragma once
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ModelResources.hpp"
#include "starfield/core/ParticleMotionPose.hpp"
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
       first.sprite_bases.size()>kMaxParticleSpriteBases || last.sprite_bases.size()>kMaxParticleSpriteBases ||
       first.texture_styles.size()>kMaxTextureStyles || last.texture_styles.size()>kMaxTextureStyles ||
       first.cloud_styles.size()>kMaxCloudStyles || last.cloud_styles.size()>kMaxCloudStyles ||
       first.model_styles.size()>kMaxModelStyles || last.model_styles.size()>kMaxModelStyles)
        return R::failure(ErrorCode::invalid_request,"invalid motion interpolation input");
    for(const auto* endpoint:{&first,&last})for(const auto& basis:endpoint->sprite_bases)
        if(!valid_particle_sprite_basis(basis))return R::failure(ErrorCode::invalid_request,"invalid motion sprite basis");
    std::size_t model_members=0;
    for(const auto* endpoint:{&first,&last}) {
        for(const auto& p:endpoint->particles)if(!valid_particle_motion_pose(p.motion_pose))
            return R::failure(ErrorCode::invalid_request,"invalid endpoint Motion pose");
        for(const auto& style:endpoint->model_styles) {
            if(style.instances.empty() || style.instances.size()>kMaxModelsPerStyle)
                return R::failure(ErrorCode::invalid_request,"invalid motion Model group");
            for(const auto& instance:style.instances) {
                if((model_members++&63)==0 && cancel.is_cancelled())
                    return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
                if(!valid_model_instance(instance))
                    return R::failure(ErrorCode::invalid_request,"invalid motion Model instance");
            }
        }
        for(const auto& p:endpoint->particles)
            if(p.model_style_index>endpoint->model_styles.size() || (p.shape!=4 && p.model_style_index))
                return R::failure(ErrorCode::invalid_request,"missing motion Model group");
        for(const auto& style:endpoint->cloud_styles)if(!valid_cloud_style(style))
            return R::failure(ErrorCode::invalid_request,"invalid motion Cloud style");
        for(const auto& p:endpoint->particles)if(p.cloud_style_index>endpoint->cloud_styles.size() ||
            (p.shape!=2 && p.cloud_style_index) || p.cloud_random_key>0xffffffu ||
            (!p.cloud_style_index && p.cloud_random_key))
            return R::failure(ErrorCode::invalid_request,"missing motion Cloud style");
        for(const auto& style:endpoint->texture_styles) if(!valid_texture_style(style))
            return R::failure(ErrorCode::invalid_request,"invalid motion texture style");
        for(const auto& p:endpoint->particles) if(p.texture_style_index>endpoint->texture_styles.size() ||
            (p.shape==3 ? !p.texture_style_index : p.texture_style_index!=0) || p.texture_random_key>0xffffffu)
            return R::failure(ErrorCode::invalid_request,"missing motion texture style");
    }
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
    std::map<std::pair<bool,std::uint32_t>,std::uint32_t> texture_indices;
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t> cloud_pairs;
    std::map<std::pair<std::uint32_t,std::uint32_t>,std::uint32_t> model_pairs;
    constexpr auto absent=std::numeric_limits<std::uint32_t>::max();
    constexpr ParticleSpriteBasis identity_basis{1,0,0,0,1,0,0,0,1};
    const auto basis_at=[&](const EvaluatedGraph& graph,std::uint32_t at)->const ParticleSpriteBasis& {
        return at?graph.sprite_bases[at-1]:identity_basis;
    };
    const ParticleModelInstance implicit_cube{};
    const auto models_at=[&](const EvaluatedGraph& graph,std::uint32_t at)->std::span<const ParticleModelInstance> {
        return at?std::span<const ParticleModelInstance>{graph.model_styles[at-1].instances}:
            std::span<const ParticleModelInstance>{&implicit_cube,1};
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
            auto pose=interpolate_motion_pose(a->motion_pose,b->motion_pose,amount);
            if(!pose.has_value())return R::failure(pose.error());p.motion_pose=pose.take_value();
            p.feather_percent=lerp(a->feather_percent,b->feather_percent);
            p.anchor_x_percent=lerp(a->anchor_x_percent,b->anchor_x_percent);
            p.anchor_y_percent=lerp(a->anchor_y_percent,b->anchor_y_percent);
        } else {
            const auto dt=clock-(a?first_clock:last_clock);
            p.age_seconds+=dt;
            p.position.x+=p.velocity.x*dt;p.position.y+=p.velocity.y*dt;p.position.z+=p.velocity.z*dt;
        }
        if(p.age_seconds>=0 && p.age_seconds<p.lifetime_seconds) {
            if(p.shape==4) {
                const auto key=std::pair{a&&a->shape==4?a->model_style_index:absent,
                    b&&b->shape==4?b->model_style_index:absent};
                auto found=model_pairs.find(key);
                if(found==model_pairs.end()) {
                    const auto chosen=models_at(a?first:last,p.model_style_index);
                    bool compatible=a&&b&&a->shape==4&&b->shape==4;
                    const auto other=compatible?models_at(last,b->model_style_index):std::span<const ParticleModelInstance>{};
                    compatible=compatible&&chosen.size()==other.size();
                    if(compatible)for(std::size_t j=0;j<chosen.size();++j)
                        if(chosen[j].resource!=other[j].resource){compatible=false;break;}
                    std::uint32_t retained_index=0;
                    if(p.model_style_index || (compatible && b->model_style_index)) {
                        if(output.model_styles.size()==kMaxModelStyles)
                            return R::failure(ErrorCode::work_limit_exceeded,"motion Model groups exceed budget");
                        ParticleModelStyle style;style.instances.assign(chosen.begin(),chosen.end());
                        if(compatible)for(std::size_t j=0;j<style.instances.size();++j) {
                            if((model_members++&63)==0 && cancel.is_cancelled())
                                return R::failure(ErrorCode::cancelled,"motion interpolation cancelled");
                            auto& instance=style.instances[j];
                            for(std::size_t k=0;k<instance.model_to_particle.size();++k)
                                instance.model_to_particle[k]=std::lerp(instance.model_to_particle[k],other[j].model_to_particle[k],amount);
                            if(!valid_model_instance(instance))
                                return R::failure(ErrorCode::invalid_request,"invalid interpolated Model instance");
                        }
                        output.model_styles.push_back(std::move(style));
                        retained_index=static_cast<std::uint32_t>(output.model_styles.size());
                    }
                    found=model_pairs.emplace(key,retained_index).first;
                }
                p.model_style_index=found->second;
            }
            if(p.cloud_style_index) {
                const auto key=std::pair{a?a->cloud_style_index:absent,b?b->cloud_style_index:absent};
                auto found=cloud_pairs.find(key);
                if(found==cloud_pairs.end()) {
                    if(output.cloud_styles.size()==kMaxCloudStyles)
                        return R::failure(ErrorCode::work_limit_exceeded,"motion Cloud styles exceed budget");
                    const auto& source=a?first:last;
                    auto style=source.cloud_styles[p.cloud_style_index-1];
                    if(a && b && b->cloud_style_index) {
                        const auto& other=last.cloud_styles[b->cloud_style_index-1];
                        style.aspect=lerp(style.aspect,other.aspect);style.density=lerp(style.density,other.density);
                    }
                    output.cloud_styles.push_back(style);
                    found=cloud_pairs.emplace(key,static_cast<std::uint32_t>(output.cloud_styles.size())).first;
                }
                p.cloud_style_index=found->second;
            }
            if(p.shape==3) {
                const auto key=std::pair{a!=nullptr,p.texture_style_index};
                auto found=texture_indices.find(key);
                if(found==texture_indices.end()) {
                    if(output.texture_styles.size()==kMaxTextureStyles)
                        return R::failure(ErrorCode::work_limit_exceeded,"motion texture styles exceed budget");
                    const auto& source=a?first:last;
                    output.texture_styles.push_back(source.texture_styles[p.texture_style_index-1]);
                    found=texture_indices.emplace(key,static_cast<std::uint32_t>(output.texture_styles.size())).first;
                }
                p.texture_style_index=found->second;
            }
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
