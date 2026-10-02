#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/PluginApi.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

using namespace starfield::core;
using namespace starfield::core::graph_keys;
namespace {
int checks=0, failures=0;
void check(bool value,const char* name) { ++checks; if (!value) {++failures; std::printf("FAILED: %s\n",name);} }
Uuid128 uuid(unsigned value) { Uuid128 id{}; id.bytes[15]=static_cast<std::uint8_t>(value); return id; }
void set(GraphNode& node, ParameterKey key, ParameterValue value) {
    for (auto& p:node.parameters) if (p.key==key) {p.value=std::move(value);return;}
    node.parameters.push_back({key,std::move(value)});
}
const NeverCancelled never;
Graph auxiliary_graph() {
    Settings settings; settings.birth_rate=1; settings.velocity={1,0,0}; settings.velocity_spread=0;
    settings.particle_lifetime_seconds=1.1; settings.particle_count=100; settings.particle_size=8;
    auto made=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)});
    check(made.has_value(),"construct primary source");
    Graph graph=made.take_value();
    auto emitter=graph.nodes[0]; emitter.id=NodeId{uuid(4)};
    set(emitter,kEmittingMode,std::uint32_t{1}); set(emitter,kBirthRate,2.0); set(emitter,kVelocity,Vec3{});
    auto child=graph.nodes[1]; child.id=NodeId{uuid(5)}; set(child,kParticleLifetimeSeconds,2.0);
    graph.nodes.push_back(emitter); graph.nodes.push_back(child);
    graph.edges[1]={EdgeId{uuid(12)},NodeId{uuid(2)},kParticleParticlesOut,emitter.id,kEmitterParents};
    graph.edges.push_back({EdgeId{uuid(13)},emitter.id,kEmitterParticles,child.id,kParticleParticlesIn});
    graph.edges.push_back({EdgeId{uuid(14)},child.id,kParticleParticlesOut,NodeId{uuid(255)},kOutputParticles});
    return graph;
}
void test_auxiliary() {
    auto graph=auxiliary_graph();
    check(validate_graph(graph,particle_node_registry()).ok(),"Auxiliary parent input validates");
    auto result=evaluate_particle_graph(graph,{5,4},never);
    check(result.has_value(),"evaluate auxiliary births");
    if (!result.has_value()) {std::printf("%s\n",result.error().detail);return;}
    check(result.value().particles.size()==4,"emit at parent positions on 0,.5,1 clocks");
    check(std::any_of(result.value().particles.begin(),result.value().particles.end(),[](const auto& p) {return std::abs(p.position.x-.5)<1e-9 && p.age_seconds>.7;}),"child retains birth position after parent death");
    auto repeat=evaluate_particle_graph(graph,{5,4},never);
    check(repeat.has_value() && repeat.value().particles.size()==result.value().particles.size(),"auxiliary repeat count stable");
    if (repeat.has_value()) for (std::size_t i=0;i<repeat.value().particles.size();++i)
        check(repeat.value().particles[i].id==result.value().particles[i].id && repeat.value().particles[i].position.x==result.value().particles[i].position.x,"auxiliary identity and position stable");
    set(graph.nodes[3],kInheritVelocity,100.0);
    auto inherited=evaluate_particle_graph(graph,{5,4},never);
    check(inherited.has_value() && std::any_of(inherited.value().particles.begin(),inherited.value().particles.end(),[](const auto& p) {return std::abs(p.position.x-1.25)<1e-9;}),"inherit instantaneous parent velocity");
    set(graph.nodes[3],kEmitChance,0.0);
    auto zero=evaluate_particle_graph(graph,{5,4},never); check(zero.has_value() && zero.value().particles.empty(),"zero chance emits nothing");
    set(graph.nodes[3],kEmitChance,100.0); set(graph.nodes[2],kParticleCount,std::uint32_t{2});
    auto capped=evaluate_particle_graph(graph,{5,4},never); check(capped.has_value() && capped.value().particles.size()==2,"Output cap includes auxiliary births");
    set(graph.nodes[3],kEmitLifeStart,90.0);set(graph.nodes[3],kEmitLifeEnd,10.0);
    check(!evaluate_particle_graph(graph,{5,4},never).has_value(),"reversed parent life interval rejects");
    graph=auxiliary_graph();graph.edges.push_back({EdgeId{uuid(15)},NodeId{uuid(5)},kParticleParticlesOut,NodeId{uuid(4)},kEmitterParents});
    check(!validate_graph(graph,particle_node_registry()).ok(),"auxiliary feedback cycle rejects");
}
double centroid(const RenderOutput& output) {
    double sum=0,weight=0;
    for(std::uint32_t y=0;y<output.height();++y) for(std::uint32_t x=0;x<output.width();++x) {
        const auto* pixel=output.pixels.data()+y*output.row_bytes+x*bytes_per_pixel(output.format);
        double a{};
        if(output.format==PixelFormat::rgba8) a=std::to_integer<unsigned char>(pixel[3]);
        else if(output.format==PixelFormat::rgba16) {std::uint16_t value{};std::memcpy(&value,pixel+6,2);a=value;}
        else {float value{};std::memcpy(&value,pixel+12,4);a=value;}
        sum+=(x+.5)*a;weight+=a;
    }
    return weight ? sum/weight : -1;
}
void test_camera() {
    RenderRequest request; Settings settings;settings.particle_count=1;settings.velocity={};settings.velocity_spread=0;
    settings.emitter_origin={.2,0,0};settings.particle_size=10;request.settings=validate_settings(settings);
    request.frame={100,100,100,100,{0,0,100,100},{0,1},{1,24},PixelFormat::rgba8,ColorSpace::ae_working_space,AlphaMode::straight,1,Quality::full};
    const auto flat=CpuParticleRenderer{}.render(request,never);
    check(flat.has_value() && std::abs(centroid(flat.value())-70)<.1,"flat origin maps to pixels");
    auto& c=request.camera;c.enabled=true;c.layer_to_view={1,0,0,0,0,1,0,0,0,0,1,0,-50,-50,100,1};
    c.image_to_layer={1,0,0,0,1,0,0,0,1};c.focal_x=c.focal_y=100;c.center_x=c.center_y=50;
    const auto plane=CpuParticleRenderer{}.render(request,never);
    check(plane.has_value() && plane.value().pixels==flat.value().pixels,"default camera plane matches flat pixels");
    for(const auto format:{PixelFormat::rgba16,PixelFormat::rgba32f}) {
        request.frame.format=format;
        request.camera.enabled=false;const auto baseline=CpuParticleRenderer{}.render(request,never);
        request.camera.enabled=true;const auto projected=CpuParticleRenderer{}.render(request,never);
        check(baseline.has_value() && projected.has_value() && baseline.value().pixels==projected.value().pixels,"camera plane preserves 16/32-bpc output");
    }
    request.frame.format=PixelFormat::rgba8;
    request.settings.value.emitter_origin.z=1;
    const auto far=CpuParticleRenderer{}.render(request,never);
    check(far.has_value() && std::abs(centroid(far.value())-60)<.2,"Z affects perspective position");
    request.frame.region_of_interest={55,45,65,55};
    const auto cropped=CpuParticleRenderer{}.render(request,never);
    bool same=cropped.has_value() && far.has_value();
    if(same) for(std::uint32_t y=0;y<cropped.value().height();++y)
        same=same && std::memcmp(cropped.value().pixels.data()+y*cropped.value().row_bytes,
            far.value().pixels.data()+(y+45)*far.value().row_bytes+55*4,cropped.value().row_bytes)==0;
    check(same,"camera ROI equals exact full-frame crop");
    request.frame.frame_width=request.frame.frame_height=50;request.frame.region_of_interest={0,0,50,50};
    const auto half=CpuParticleRenderer{}.render(request,never);
    check(half.has_value() && std::abs(centroid(half.value())-30)<.2,"camera projection obeys downsample");
    request.frame.frame_width=request.frame.frame_height=100;request.frame.region_of_interest={0,0,100,100};
    request.camera.layer_to_view[12]-=20;
    const auto moved=CpuParticleRenderer{}.render(request,never);
    check(moved.has_value() && std::abs(centroid(moved.value())-50)<.2,"camera translation changes output");
    request.settings.value.emitter_origin.z=-2;
    const auto behind=CpuParticleRenderer{}.render(request,never);
    check(behind.has_value() && centroid(behind.value())<0,"behind-camera sprites clipped");
    request.camera.focal_x=std::numeric_limits<double>::quiet_NaN();
    check(!CpuParticleRenderer{}.render(request,never).has_value(),"non-finite camera rejects");
    SfCoreApi api{};check(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1,"paired ABI 2 loads");
    check(StarfieldCore_GetApi(1,sizeof(api),&api)==0,"ABI 1 cannot load new camera contract");
}
}
int main() {test_auxiliary();test_camera();std::printf("%d checks, %d failures\n",checks,failures);return failures?1:0;}
