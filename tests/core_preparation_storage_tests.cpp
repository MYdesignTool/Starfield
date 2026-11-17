#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/EmitterHistory.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "../src/core/EvaluationStorage.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <new>
#include <string>
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
std::atomic<bool> measured{};
std::atomic<std::uint64_t> allocation_count{},allocation_bytes{};
std::atomic<long long> fail_after{-1};
}
void* operator new(std::size_t size) {
    if(fail_after>=0 && fail_after.fetch_sub(1)==0)throw std::bad_alloc{};
    if(measured){++allocation_count;allocation_bytes+=size;}
    if(auto* p=std::malloc(size?size:1))return p;throw std::bad_alloc{};
}
void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
unsigned checks{},failures{};const NeverCancelled never;
void check(bool value,const char* name){++checks;if(!value){++failures;std::printf("FAIL: %s\n",name);}}
NodeId id(unsigned n){NodeId result{};result.value.bytes[12]=static_cast<std::uint8_t>(n>>24);result.value.bytes[13]=static_cast<std::uint8_t>(n>>16);result.value.bytes[14]=static_cast<std::uint8_t>(n>>8);result.value.bytes[15]=static_cast<std::uint8_t>(n);return result;}
void set(GraphNode& node,ParameterKey key,ParameterValue value){for(auto& p:node.parameters)if(p.key==key){p.value=std::move(value);return;}node.parameters.push_back({key,std::move(value)});}
Graph force_graph(unsigned count,unsigned population) {
    Settings settings;settings.birth_rate=population?population/2.:20;settings.particle_count=population;settings.particle_lifetime_seconds=2;
    settings.emission_speed=.1;settings.velocity_spread=.03;
    auto graph=make_emitter_particle_output_graph(settings,id(1),id(2),id(10000),{id(10001).value},{id(10002).value}).take_value();
    if(count)graph.edges.pop_back();auto previous=id(2);auto port=kParticleParticlesOut;
    for(unsigned n=0;n<count;++n){const auto current=id(10+n);graph.nodes.push_back({current,kForceNode,3,{{kGravity,Vec3{.001,.002,-.001}},{kLinearDrag,.001}}});
        graph.edges.push_back({{id(20000+n).value},previous,port,current,kForceParticlesIn});previous=current;port=kForceParticlesOut;}
    if(count)graph.edges.push_back({{id(30000).value},previous,port,id(10000),kOutputParticles});return graph;
}
Graph auxiliary_graph() {
    Settings settings;settings.birth_rate=24;settings.particle_count=2000;settings.particle_lifetime_seconds=1.5;settings.velocity={.02,.1,0};
    auto graph=make_emitter_particle_output_graph(settings,id(1),id(2),id(10000),{id(10001).value},{id(10002).value}).take_value();
    auto emitter=graph.nodes[0];emitter.id=id(4);set(emitter,kAuxiliarySource,std::uint32_t{1});set(emitter,kBirthRate,12.);
    set(emitter,kEmitChance,75.);set(emitter,kInheritVelocity,30.);set(emitter,kInheritSize,60.);
    auto child=graph.nodes[1];child.id=id(5);set(child,kParticleLifetimeSeconds,1.);
    graph.nodes.push_back(std::move(emitter));graph.nodes.push_back(std::move(child));
    graph.nodes.push_back({id(6),kForceNode,3,{{kGravity,Vec3{.02,-.03,.01}},{kLinearDrag,.1}}});
    graph.edges[1]={{id(10002).value},id(2),kParticleParticlesOut,id(4),kEmitterParents};
    graph.edges.push_back({{id(10003).value},id(4),kEmitterParticles,id(5),kParticleParticlesIn});
    graph.edges.push_back({{id(10004).value},id(5),kParticleParticlesOut,id(6),kForceParticlesIn});
    graph.edges.push_back({{id(10005).value},id(6),kForceParticlesOut,id(10000),kOutputParticles});return graph;
}
struct Payload {
    static inline unsigned moves{},copies{};
    std::array<double,512> values{};std::vector<int> owned;
    Payload()=default;Payload(const Payload& p):values(p.values),owned(p.owned){++copies;}
    Payload(Payload&& p)noexcept:values(p.values),owned(std::move(p.owned)){++moves;}
};
struct Immovable {
    static inline unsigned live{};int value;
    explicit Immovable(int n):value(n){++live;}
    Immovable(const Immovable&)=delete;Immovable(Immovable&&)=delete;
    ~Immovable(){--live;}
};
void ownership(bool baseline) {
    {
        evaluation_storage::FrameValue<Immovable> slot;
        check(slot.replace([]{return Immovable(17);}).value==17 && Immovable::live==1,"factory directly constructs an immovable payload");
        try{slot.replace([]()->Immovable{throw std::bad_alloc{};});check(false,"factory failure must propagate");}catch(const std::bad_alloc&){}
        check(!slot.has_value() && Immovable::live==0,"failed factory leaves owned optional disengaged");
        check(slot.replace([]{return Immovable(19);}).value==19,"frame can be reused after a failed factory");
    }
    check(Immovable::live==0,"frame destroys its final payload exactly once");
    Payload p;p.values[511]=37;p.owned={1,2,3};const auto* original=p.owned.data();Payload::moves=Payload::copies=0;
    auto value=Result<Payload>::success(std::move(p));
    check(value.has_value() && value.value().values[511]==37 && value.value().owned.data()==original && Payload::copies==0,"Result preserves large payload and container ownership");
    std::printf("Result payload moves: %u\n",Payload::moves);if(!baseline)check(Payload::moves==2,"Result directly constructs its variant member");
    Settings settings;settings.forces.resize(12);const auto* forces=settings.forces.data();
    allocation_count=allocation_bytes=0;measured=true;auto validated=validate_settings(std::move(settings));measured=false;
    check(validated.notices.empty() && validated.value.forces.size()==12,"valid Force settings retain data");
    std::printf("Owned settings validation: %llu allocations, %llu bytes\n",static_cast<unsigned long long>(allocation_count),static_cast<unsigned long long>(allocation_bytes));
    if(!baseline)check(validated.value.forces.data()==forces && allocation_count==0,"by-value Settings transfers its existing Force buffer without allocation");
}
struct Sampler final:TemporalGraphSampler {
    const Graph& graph;bool certify{},dynamic{};
    Sampler(const Graph& g,bool proof,bool animated):graph(g),certify(proof),dynamic(animated){}
    Result<GraphNode> node(NodeId wanted,double time) override {
        const auto found=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==wanted;});
        if(found==graph.nodes.end())return Result<GraphNode>::failure(ErrorCode::invalid_request,"probe node missing");
        auto n=*found;
        if(dynamic && n.type_key==kForceNode){set(n,kGravity,Vec3{.01,-.02+.01*time,.003});set(n,kWind,Vec3{.02,.01*time,0});set(n,kSpin,.03);set(n,kSpinFrequency,.4);}
        return Result<GraphNode>::success(std::move(n));
    }
    std::optional<GraphNode> constant_node(NodeId wanted) override {
        if(!certify)return {};const auto found=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==wanted;});
        if(found==graph.nodes.end() || (dynamic && found->type_key==kForceNode))return {};return *found;
    }
    Result<double> rate(NodeId wanted,double time) override {auto n=node(wanted,time);if(!n.has_value())return Result<double>::failure(n.error());
        for(const auto& p:n.value().parameters)if(p.key==kBirthRate)return Result<double>::success(std::get<double>(p.value));return Result<double>::failure(ErrorCode::invalid_request,"rate missing");}
    Result<std::optional<EmissionRateProfile>> rate_profile(NodeId wanted) override {
        auto r=rate(wanted,0);if(!r.has_value())return Result<std::optional<EmissionRateProfile>>::failure(r.error());EmissionRateProfile profile;profile.constant=r.value();return Result<std::optional<EmissionRateProfile>>::success(profile);
    }
    std::optional<double> lifetime_upper_bound(NodeId) override {return 2.;}
};
Result<EvaluatedGraph> evaluate(const Graph& graph,RationalTime time,unsigned temporal) {
    if(!temporal)return evaluate_particle_graph(graph,time,never);
    Sampler sampler(graph,temporal!=1,temporal==3);return evaluate_temporal_particle_graph(graph,time,never,{},sampler);
}
void record(const std::string& name,const Graph& graph,RationalTime time,const std::filesystem::path& directory,unsigned temporal=0) {
    auto warm=evaluate(graph,time,temporal);check(warm.has_value(),"warm graph evaluates");if(!warm.has_value()){std::printf("%s: %s\n",name.c_str(),warm.error().detail);return;}
    std::array<double,5> samples{};Result<EvaluatedGraph> result=Result<EvaluatedGraph>::failure(ErrorCode::internal_failure,"not measured");
    std::uint64_t count{},bytes{};
    for(auto& ms:samples){allocation_count=allocation_bytes=0;measured=true;const auto start=std::chrono::steady_clock::now();
        result=evaluate(graph,time,temporal);ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();measured=false;
        check(result.has_value(),"measured graph evaluates");count=allocation_count;bytes=allocation_bytes;}
    if(!result.has_value())return;std::sort(samples.begin(),samples.end());
    auto encoded=encode_evaluated_particles(result.value(),time);check(encoded.has_value(),"complete particle record encodes");
    auto scene=serialize_graph(graph,particle_node_registry());check(scene.has_value(),"authored graph/CRC encodes");
    std::printf("case,%s,%zu,%.6f,%llu,%llu\n",name.c_str(),result.value().particles.size(),samples[2],static_cast<unsigned long long>(count),static_cast<unsigned long long>(bytes));
    if(!directory.empty() && encoded.has_value() && scene.has_value()) {
        const auto write=[&](const char* suffix,const OpaqueBytes& data){std::ofstream out(directory/(name+suffix),std::ios::binary);out.write(reinterpret_cast<const char*>(data.data()),static_cast<std::streamsize>(data.size()));check(bool(out),"comparison artifact writes");};
        write(".particles",encoded.value());write(".graph",scene.value());
    }
}
void allocation_failures(const Graph& graph,unsigned temporal) {
    allocation_count=0;measured=true;
    auto warm=evaluate(graph,{1,1},temporal);measured=false;
    const auto sites=allocation_count.load();check(warm.has_value(),"allocation fixture warms");
    if(!warm.has_value())return;
    const auto expected=encode_evaluated_particles(warm.value(),{1,1});check(expected.has_value(),"allocation fixture baseline encodes");
    check(sites<4096,"allocation fault fixture remains bounded");if(sites>=4096)return;
    for(long long n=0;n<=static_cast<long long>(sites);++n){fail_after=n;auto result=evaluate(graph,{1,1},temporal);fail_after=-1;
        if(result.has_value()){auto bytes=encode_evaluated_particles(result.value(),{1,1});check(bytes.has_value() && bytes.value()==expected.value(),"allocation fault cannot return partial or stale values");}
        else check(result.error().code==ErrorCode::allocation_failed,"allocation failure remains typed");}
}
}
int main(int argc,char** argv)try {
    const bool baseline=argc>1 && std::string(argv[1])=="--baseline";std::filesystem::path directory;
    if(argc>2){directory=argv[2];std::filesystem::create_directories(directory);}
    ownership(baseline);
    for(const auto count:{0u,4u,32u,128u})record("sparse-"+std::to_string(count),force_graph(count,0),{1,1},directory);
    for(const auto count:{0u,4u,12u})for(const auto frame:{1,3})record("particles-"+std::to_string(count)+"-"+std::to_string(frame),force_graph(count,30000),{frame,1},directory);
    auto auxiliary=auxiliary_graph();for(const auto frame:{12,24,36})record("aux-"+std::to_string(frame),auxiliary,{frame,24},directory);
    auto reordered=force_graph(32,128);std::reverse(reordered.nodes.begin(),reordered.nodes.end());std::reverse(reordered.edges.begin(),reordered.edges.end());record("reordered",reordered,{3,2},directory);
    for(unsigned mode:{1u,2u,3u})record("temporal-force-"+std::to_string(mode),force_graph(4,64),{1,1},directory,mode);
    record("temporal-aux",auxiliary,{24,24},directory,2);
    auto ordered=force_graph(4,64);
    ordered.nodes.push_back({id(8),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionAxis,Vec3{0,0,1}},
        {kMotionAngularRate,.3},{kMotionOrigin,Vec3{.1,0,0}}}});
    for(auto& edge:ordered.edges)if(edge.source_node==id(2) && edge.destination_node==id(10)) {
        edge.destination_node=id(8);edge.destination_port=kMotionParticlesIn;
    }
    ordered.edges.push_back({{id(40000).value},id(8),kMotionParticlesOut,id(10),kForceParticlesIn});
    record("temporal-ordered-force",ordered,{1,1},directory,3);
    allocation_failures(force_graph(12,128),0);
    for(auto& node:ordered.nodes){if(node.type_key==kOutputNode)set(node,kParticleCount,std::uint32_t{4});if(node.type_key==kEmitterNode)set(node,kBirthRate,2.);}
    allocation_failures(ordered,3);
    std::printf("Core preparation storage: %u checks, %u failures\n",checks,failures);return failures?1:0;
}catch(...){measured=false;fail_after=-1;std::puts("Unexpected storage fixture exception");return 1;}
