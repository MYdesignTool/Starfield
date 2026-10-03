#include "starfield/core/EmissionTimeline.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include <cstdio>
#include <cstring>
#include <limits>

using namespace starfield::core;
namespace {
int checks{},failures{};
void check(bool value,const char* name) {++checks;if(!value){++failures;std::printf("FAILED: %s\n",name);}}
void close(double actual,double expected,const char* name,double epsilon=1e-9) {check(std::abs(actual-expected)<epsilon,name);}
NeverCancelled never;
class Cancelled final:public Cancellation {bool is_cancelled() const noexcept override {return true;}} cancelled;
void test_constant_and_keys() {
    EmissionTimeline timeline;EmissionRateProfile profile;profile.constant=100;
    check(timeline.configure(30,&profile).has_value(),"constant metadata accepted");
    std::uint64_t work=0,calls=0;
    const auto rate=[&](double){++calls;return Result<double>::success(100);};
    check(timeline.extend(1000000000,rate,never,work).has_value(),"constant billion-second clock is bounded O(1)");
    check(calls==0 && work==0 && timeline.segment_count()==0,"constant path performs no rate samples");
    close(timeline.integral(1000000000),100000000000.0,"constant integral rate times t");
    close(timeline.birth(12345),123.45,"constant birth inversion");
    close(timeline.birth(0),0,"initial constant birth");
    profile.keys={{0,0,RateInterpolation::linear},{2,100,RateInterpolation::hold},{4,0,RateInterpolation::linear},{6,20,RateInterpolation::linear}};
    check(timeline.configure(120,&profile).has_value(),"linear/hold key metadata accepted");
    close(timeline.integral(1),25,"partial linear trapezoid");
    close(timeline.integral(2),100,"complete linear trapezoid");
    close(timeline.integral(3),200,"hold integrates left value");
    close(timeline.integral(4),300,"hold boundary has no area discontinuity");
    close(timeline.integral(5),305,"second ramp integrates from zero");
    close(timeline.integral(1000000),320+20.0*(1000000-6),"constant extrapolation beyond final key");
    for(std::uint64_t ordinal:{1ULL,50ULL,100ULL,200ULL,300ULL,310ULL,400ULL})
        close(timeline.integral(timeline.birth(ordinal)),double(ordinal),"analytic threshold inversion",1e-7);
    check(timeline.segment_count()==3,"analytic memory depends only on key count");
    profile.keys={{-2,0,RateInterpolation::linear},{2,100,RateInterpolation::linear}};
    check(timeline.configure(30,&profile).has_value(),"keys before time zero are accepted");
    close(timeline.integral(2),150,"clip ramp integral to simulation zero");
    profile.keys={{0,100,RateInterpolation::hold},{1,0,RateInterpolation::hold},{5,100,RateInterpolation::linear}};
    check(timeline.configure(60,&profile).has_value(),"plateau keys accepted");
    close(timeline.birth(100),1,"plateau equality uses earliest threshold crossing");
    close(timeline.birth(101),5.01,"birth after plateau");
    profile.keys={{0,0,RateInterpolation::linear},{0,1,RateInterpolation::linear}};
    check(!timeline.configure(30,&profile).has_value(),"duplicate key time rejected");
    profile.keys={{0,-1,RateInterpolation::linear}};
    check(!timeline.configure(30,&profile).has_value(),"negative keyed rate rejected");
    profile.keys.clear();profile.constant=std::numeric_limits<double>::infinity();
    check(!timeline.configure(30,&profile).has_value(),"nonfinite constant rejected");
}
void test_sampled_prefixes() {
    for(unsigned hz:{30u,60u,120u}) {
        EmissionTimeline cached,fresh;
        check(cached.configure(hz).has_value() && fresh.configure(hz).has_value(),"sampled frequency accepted");
        std::uint64_t calls{},work{},fresh_work{};
        const auto sampled=[&](double t){++calls;return Result<double>::success(30+12*std::sin(t*3));};
        const auto pure=[](double t){return Result<double>::success(30+12*std::sin(t*3));};
        check(cached.extend(1,sampled,never,work).has_value(),"build first fixed prefix");
        const auto first_calls=calls;
        check(cached.extend(.2,sampled,never,work).has_value() && calls==first_calls,"reverse-time query reuses prefix");
        check(cached.extend(4.37,sampled,never,work).has_value(),"extend prefixes only over missing lattice");
        check(fresh.extend(4.37,pure,never,fresh_work).has_value(),"uncached comparison builds same lattice");
        check(calls==3*static_cast<std::uint64_t>(std::ceil(4.37*hz)),"each cached interval sampled exactly once");
        for(double t:{0.0,.2,1.0,2.137,4.37})
            check(cached.integral(t)==fresh.integral(t),"cached prefix equals uncached bit for bit");
        for(std::uint64_t id=0;id<90;++id)
            check(cached.birth(id)==fresh.birth(id),"cached birth inversion equals uncached bit for bit");
        const auto count=calls;
        check(cached.extend(4.2,sampled,never,work).has_value() && calls==count,"partial last segment query does not resample");
        check(!cached.extend(5,sampled,cancelled,work).has_value() && calls==count,"cancel before further sampling");
    }
    EmissionTimeline timeline;
    check(!timeline.configure(24).has_value(),"unadvertised frequency rejected");
    check(timeline.configure(30).has_value(),"reset sampled cache");
    std::uint64_t work=20'000'001;
    check(!timeline.extend(1,[](double){return Result<double>::success(10);},never,work).has_value(),"work budget handles already exhausted count without underflow");
    work=0;
    check(!timeline.extend(1,[](double){return Result<double>::success(-1);},never,work).has_value(),"invalid sampled rate rejected");
    double errors[3]{};unsigned index=0;
    for(unsigned hz:{30u,60u,120u}) {
        check(timeline.configure(hz).has_value(),"accuracy frequency configured");work=0;
        check(timeline.extend(1,[](double t){return Result<double>::success(100*std::exp(-t));},never,work).has_value(),"sample exponential rate");
        errors[index++]=std::abs(timeline.integral(1)-100*(1-std::exp(-1.0)));
    }
    check(errors[2]<errors[1] && errors[1]<errors[0],"60/120 Hz improve nonlinear integration accuracy");
}
Uuid128 uuid(unsigned value){Uuid128 id{};id.bytes[15]=static_cast<std::uint8_t>(value);return id;}
struct MetadataSampler final:TemporalGraphSampler {
    Graph graph;
    EmissionRateProfile profile;
    unsigned profile_calls{},rate_calls{},node_calls{};
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId) override {
        ++profile_calls;return Result<std::optional<EmissionRateProfile>>::success(profile);
    }
    Result<double> rate(NodeId,double) override {++rate_calls;return Result<double>::failure(ErrorCode::internal_failure,"analytic path sampled rate");}
    Result<GraphNode> node(NodeId id,double) override {
        ++node_calls;for(const auto& n:graph.nodes) if(n.id==id) return Result<GraphNode>::success(n);
        return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing fixture node");
    }
};
void test_evaluator_metadata() {
    Settings settings;settings.birth_rate=10;settings.particle_count=3;settings.particle_lifetime_seconds=2;
    MetadataSampler sampler;sampler.profile.constant=10;
    sampler.graph=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    const auto evaluated=evaluate_temporal_particle_graph(sampler.graph,{1000000000,1},never,{1080,1},sampler);
    check(evaluated.has_value(),"actual temporal evaluator uses metadata fast path at long times");
    if(evaluated.has_value()) {
        check(evaluated.value().particles.size()==3,"analytic evaluator respects newest-particle cap");
        check(evaluated.value().particles.back().id==10000000000ULL,"analytic evaluator preserves emission ordinals");
    }
    check(sampler.rate_calls==0 && sampler.profile_calls==1,"evaluator does not integrate certified constant stream");
    sampler.graph.nodes[0].parameters.push_back({graph_keys::kEmittingMode,std::uint32_t{1}});
    sampler.graph.nodes.back().parameters[0].value=std::uint32_t{100};
    const auto once=evaluate_temporal_particle_graph(sampler.graph,{1,1},never,{1080,1},sampler);
    check(once.has_value() && once.value().particles.size()==10,"Once creates one initial PPS-sized batch");
    if(once.has_value()) for(const auto& particle:once.value().particles)
        close(particle.age_seconds,1,"all Once particles share first-frame birth time");
    const auto static_once=evaluate_particle_graph(sampler.graph,{1,1},never,{1080,1});
    check(static_once.has_value() && static_once.value().particles.size()==10,"static graph uses the same Once batch semantics");
    const auto expired=evaluate_temporal_particle_graph(sampler.graph,{2,1},never,{1080,1},sampler);
    check(expired.has_value() && expired.value().particles.empty(),"Once does not replenish expired particles");
    for(auto& p:sampler.graph.nodes.back().parameters) if(p.key==graph_keys::kTimeSamplingHz) p.value=std::uint32_t{24};
    check(!evaluate_temporal_particle_graph(sampler.graph,{1,1},never,{1080,1},sampler).has_value(),"actual evaluator rejects invalid sampling setting");
}
}
int main(){test_constant_and_keys();test_sampled_prefixes();test_evaluator_metadata();std::printf("%d checks, %d failures\n",checks,failures);return failures?1:0;}
