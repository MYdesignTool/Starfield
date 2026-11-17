#include "../ae_plugin/GraphProjection.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <chrono>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace gp=starfield::adapter::graph_projection;
unsigned checks{};
void check(bool v,const char* message){++checks;if(!v)throw std::runtime_error(message);}
Uuid128 uuid(unsigned n){Uuid128 id;id.bytes[14]=static_cast<std::uint8_t>(n>>8);id.bytes[15]=static_cast<std::uint8_t>(n);return id;}
NodeId id(unsigned n){return NodeId{uuid(n)};}
void set(GraphNode& n,ParameterKey k,ParameterValue v){for(auto& p:n.parameters)if(p.key==k){p.value=std::move(v);return;}n.parameters.push_back({k,std::move(v)});}
Graph base(){auto r=make_emitter_particle_force_output_graph(Settings{},id(1),id(2),id(3),id(255),EdgeId{uuid(101)},EdgeId{uuid(102)},EdgeId{uuid(103)});check(r.has_value(),"base");return r.take_value();}
void edge(Graph& g,unsigned from,unsigned to,unsigned eid){const auto n=std::find_if(g.nodes.begin(),g.nodes.end(),[&](auto& v){return v.id==id(from);});const auto d=std::find_if(g.nodes.begin(),g.nodes.end(),[&](auto& v){return v.id==id(to);});
    check(n!=g.nodes.end() && d!=g.nodes.end(),"fixture connection endpoints exist");
    const auto port=n->type_key==kEmitterNode || n->type_key==kModelNode || n->type_key==kMaterialNode?1u:2u;
    unsigned target=d->type_key==kEmitterNode || d->type_key==kMaterialNode || d->type_key==kModelNode?2u:1u;
    if((n->type_key==kModelNode || n->type_key==kMaterialNode) && d->type_key==kParticleNode)target=3;
    g.edges.push_back({EdgeId{uuid(eid)},id(from),PortKey{port},id(to),PortKey{target}});
}
const NeverCancelled never;
void valid(const Graph& g){auto v=validate_particle_graph_topology(g,never);if(!v.ok)throw std::runtime_error(v.error.detail);++checks;}
void invalid(const Graph& g){auto v=validate_particle_graph_topology(g,never);check(!v.ok && v.connection_error,"typed topology rejection");auto p=gp::project(g,never);check(p.has_value(),"invalid topology projected");valid(p.value().graph);check(!p.value().issues.empty(),"warning retained");check(p.value().attempts<=g.edges.size()+3,"bounded attempts");auto saved=p.value().graph;saved.optional_records.push_back(gp::record(p.value().issues));std::vector<gp::Issue> issues;check(gp::read(saved,issues) && issues.size()==p.value().issues.size(),"persisted issue roundtrip");check(serialize_graph(saved,particle_node_registry()).has_value(),"projection serializable");auto evaluated=evaluate_particle_graph(saved,{0,1},never);if(!evaluated.has_value())throw std::runtime_error(evaluated.error().detail);++checks;}
int main()try{
    auto g=base();valid(g);auto exact=gp::project(g,never);check(exact.has_value() && exact.value().attempts==1 && exact.value().issues.empty() && exact.value().graph.edges.size()==g.edges.size(),"valid fast path");
    auto bad=g;edge(bad,3,3,104);invalid(bad);
    bad=g;bad.edges.erase(bad.edges.begin()+1);edge(bad,1,3,104);invalid(bad);
    bad=g;bad.edges.erase(bad.edges.begin()+1);edge(bad,1,255,104);invalid(bad);
    bad=g;auto second=g.nodes[1];second.id=id(4);bad.nodes.push_back(second);edge(bad,1,4,104);edge(bad,4,3,105);invalid(bad);
    GraphNode tf{id(5),kTransformNode,1,{{kTransformAnchor,Vec3{}},{kTransformPosition,Vec3{}},{kTransformRotation,Vec3{}},{kTransformSystemScale,Vec3{100,100,100}},{kTransformParticleScale,100.},{kTransformParticleOpacity,100.}}};
    bad=g;bad.nodes.push_back(tf);edge(bad,2,5,104);edge(bad,5,3,105);invalid(bad);
    auto dynamic=g;dynamic.nodes.push_back(tf);set(dynamic.nodes.back(),kTransformInheritLayer,std::uint32_t{1});dynamic.edges.erase(dynamic.edges.begin()+1);edge(dynamic,2,5,104);edge(dynamic,5,3,105);valid(dynamic);
    auto aux=g;auto source=g.nodes[0];source.id=id(6);set(source,kAuxiliarySource,std::uint32_t{1});aux.nodes.push_back(source);auto child=g.nodes[1];child.id=id(7);aux.nodes.push_back(child);edge(aux,2,6,104);edge(aux,6,7,105);edge(aux,7,255,106);valid(aux);auto real=evaluate_particle_graph(aux,{0,1},never);check(real.has_value(),"Aux actual evaluation");
    edge(aux,7,6,107);valid(aux); // direct reciprocal parent reference
    auto bad_life=aux;set(bad_life.nodes[4],kEmitLifeStart,90.);set(bad_life.nodes[4],kEmitLifeEnd,10.);
    auto life_plan=validate_particle_graph_topology(bad_life,never);check(!life_plan.ok && !life_plan.connection_error,"Aux life range remains hard numeric error");check(!gp::project(bad_life,never).has_value(),"Aux life range cannot be hidden by pruning");
    auto value=g;set(value.nodes[0],kBirthRate,-1.);auto p=gp::project(value,never);check(!p.has_value(),"bad numeric never hidden");
    struct Cancel:Cancellation{bool is_cancelled()const noexcept override{return true;}} cancel;
    auto cancelled=gp::project(g,cancel);check(!cancelled.has_value() && cancelled.error().code==ErrorCode::cancelled,"cancel never projected");
    auto malformed=g;malformed.optional_records.push_back(gp::record({}));malformed.optional_records.back()[8]=std::byte{255};std::vector<gp::Issue> issues;check(!gp::read(malformed,issues),"malformed record bounded");
    std::uint32_t random=42;auto next=[&](){random=random*1664525+1013904223;return random;};
    const auto start=std::chrono::steady_clock::now();
    for(unsigned trial=0;trial<300;++trial){auto raw=g;raw.nodes.push_back(tf);raw.edges.clear();const unsigned nodes[]{1,2,3,5,255};
        for(unsigned i=0;i<12;++i){const auto from=nodes[next()%4],to=nodes[next()%5];edge(raw,from,to,100+i);}
        auto projected=gp::project(raw,never);check(projected.has_value(),"seeded raw projection");valid(projected.value().graph);
        auto permuted=raw;std::reverse(permuted.nodes.begin(),permuted.nodes.end());std::reverse(permuted.edges.begin(),permuted.edges.end());auto other=gp::project(permuted,never);check(other.has_value(),"permuted projection");
        check(other.value().graph.edges.size()==projected.value().graph.edges.size() && other.value().issues.size()==projected.value().issues.size(),"stable admission counts");
        for(std::size_t i=0;i<other.value().graph.edges.size();++i)check(other.value().graph.edges[i].id==projected.value().graph.edges[i].id,"stable admission order");
        auto rendered=evaluate_particle_graph(projected.value().graph,{0,1},never);check(rendered.has_value(),"projected actual static execution");
    }
    Graph maximum;maximum.nodes.push_back(*std::find_if(g.nodes.begin(),g.nodes.end(),[](const auto& n){return n.type_key==kOutputNode;}));
    const auto force_template=*std::find_if(g.nodes.begin(),g.nodes.end(),[](const auto& n){return n.type_key==kForceNode;});
    for(unsigned i=1;i<=63;++i){auto n=force_template;n.id=id(i);maximum.nodes.push_back(n);}
    for(unsigned i=1;i<=63;++i)for(unsigned j=0;j<4;++j)edge(maximum,i,j==3?255:1+(i+j)%63,1000+i*4+j);
    const auto worst_start=std::chrono::steady_clock::now();auto bounded=gp::project(maximum,never);
    check(bounded.has_value() && bounded.value().attempts<=255,"maximum native graph projection bounded");valid(bounded.value().graph);
    std::cout<<"maximum 63-author/252-edge projection: "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-worst_start).count()<<"ms, "<<bounded.value().attempts<<" preflights\n";
    std::cout<<checks<<" topology checks; 300 seeded projections in "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()<<"ms\n";return 0;
}catch(const std::exception& e){std::cerr<<"FAIL after "<<checks<<": "<<e.what()<<'\n';return 1;}
