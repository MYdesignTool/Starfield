#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/AgeCurve.hpp"
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
void test_reference_force_and_globals() {
    check(Settings{}.particle_count==1000000,"fresh Core cap is one million");
    Settings s; s.birth_rate=1; s.velocity={}; s.velocity_spread=0; s.particle_lifetime_seconds=2;
    auto made=make_emitter_particle_force_output_graph(s,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(3)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)},EdgeId{uuid(13)});
    check(made.has_value(),"construct reference Force schema 2");
    if(!made.has_value()) return;
    Graph g=made.take_value(); auto& f=g.nodes.back();
    auto evaluate=[&](double seconds) {return evaluate_particle_graph(g,RationalTime{std::int64_t(seconds*1000000),1000000},never);};
    auto newest=[&](double seconds) { auto result=evaluate(seconds); check(result.has_value(),"Force graph evaluates"); return result.has_value() && !result.value().particles.empty() ? result.value().particles.back() : ParticleInstance{}; };
    set(f,kGravity,Vec3{0,-1,0});
    check(std::abs(newest(.5).position.y+.125)<1e-12,"Gravity is exact constant acceleration");
    set(f,kWind,Vec3{2,0,0});
    check(std::abs(newest(.5).position.x-.25)<1e-12,"Wind contributes acceleration");
    set(f,kLinearDrag,1.0);
    const auto damped=newest(.5);
    check(std::abs(damped.position.x-2*(.5+std::expm1(-.5)))<1e-12,"Air Density analytically damps Wind");
    set(f,kLinearDrag,0.0); set(f,kGravity,Vec3{});
    AgeCurve ramp{}; ramp.count=2; ramp.points[0]={0,0};ramp.points[1]={1,100};
    set(f,kWindSpinCurve,encode_age_curve(ramp));
    const auto curved=newest(.5);
    check(std::abs(curved.position.x-1.0/48)<1e-12,"Wind curve is integrated over particle life");
    check(std::abs(curved.velocity.x-.125)<1e-12,"Wind curve exposes exact instantaneous velocity");
    set(f,kLinearDrag,1e-8);
    check(std::abs(newest(.5).position.x-curved.position.x)<1e-9,"near-zero drag is numerically continuous");
    set(f,kLinearDrag,0.0);set(f,kWind,Vec3{});set(f,kWindSpinCurve,encode_age_curve(AgeCurve{{AgeCurvePoint{0,100},AgeCurvePoint{1,100}},2}));
    set(f,kSpin,1.0);set(f,kSpinFrequency,1.0);
    const auto spin=newest(.25);
    check(std::abs(spin.position.x+1)<1e-12 && std::abs(spin.position.y-1)<1e-12,"Spin changes position in 3D particle stream");
    set(f,kSpinDelay,1.0);
    check(std::abs(newest(.25).position.x)<1e-12,"Spin Delay defers orbital motion");
    set(f,kSpin,0.0);set(f,kGravity,Vec3{0,-1,0});set(f,kGravityRandom,100.0);
    const auto random_a=newest(.5), random_b=newest(.5);
    check(random_a.position.y==random_b.position.y && random_a.position.y>=-.125 && random_a.position.y<=0,"Gravity random stays deterministic and bounded");
    set(f,kGravityRandom,101.0);
    check(!evaluate(.5).has_value(),"invalid Force percentages rejected");
    set(f,kGravityRandom,0.0);
    auto& output=g.nodes[2];
    set(output,kTimeRemapEnabled,std::uint32_t{1});set(output,kTimeRemapSeconds,.5);
    check(std::abs(newest(100).age_seconds-.5)<1e-12,"main Time Remapping controls simulation time");
    set(output,kTimeRemapEnabled,std::uint32_t{0});
    auto small=evaluate(1);
    check(small.has_value() && small.value().particles.capacity()<1000,"million cap does not reserve a million for tiny populations");
    s.birth_rate=1000000;
    auto slots=live_particle_slot_range(validate_settings(s),1);
    check(slots.has_value() && slots.value().count==1000000,"million cap is respected without slot allocation");
    Settings plain;plain.birth_rate=1;plain.velocity={};plain.velocity_spread=0;plain.particle_size=12;
    auto preview=make_emitter_particle_output_graph(plain,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    RenderRequest request{};request.frame.layer_width=request.frame.layer_height=request.frame.frame_width=request.frame.frame_height=64;
    request.frame={64,64,64,64,{0,0,64,64},{1,1},{1,24},PixelFormat::rgba8,ColorSpace::ae_working_space,AlphaMode::straight,1,Quality::full};
    request.settings=validate_settings(plain);
    auto render=[&]() {request.graph=std::make_shared<const Graph>(preview);return CpuParticleRenderer{}.render(request,never);};
    const auto normal=render();check(normal.has_value() && centroid(normal.value())>0,"normal renderer produces particles");
    set(preview.nodes[2],kPreviewEnabled,std::uint32_t{1});set(preview.nodes[2],kPreviewChance,0.0);
    const auto empty=render();check(empty.has_value() && centroid(empty.value())<0,"Preview zero chance produces transparent output");
    auto population=evaluate_particle_graph(preview,RationalTime{1,1},never);
    check(population.has_value() && population.value().particles.size()==2,"Preview preserves simulation and Auxiliary parent population");
    set(preview.nodes[2],kPreviewChance,100.0);const auto all=render();
    check(all.has_value() && normal.has_value() && all.value().pixels==normal.value().pixels,"Preview 100 matches normal pixels exactly");
    set(preview.nodes[2],kPreviewChance,101.0);check(!render().has_value(),"invalid global percentages rejected");
}
int main() {test_auxiliary();test_camera();test_reference_force_and_globals();std::printf("%d checks, %d failures\n",checks,failures);return failures?1:0;}
