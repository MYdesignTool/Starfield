#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/PluginApi.h"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/ColorGradient.hpp"
#include "starfield/core/Random.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <functional>
#include <fstream>

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
    set(emitter,kAuxiliarySource,std::uint32_t{1}); set(emitter,kBirthRate,2.0); set(emitter,kVelocity,Vec3{});
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
    SfCoreApi api{};check(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1,"paired current ABI loads");
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
void test_birth_origins() {
    struct Path final:EmitterOriginSampler {
        std::vector<EmitterOriginSample> captured;
        Result<Vec3> sample(NodeId node,double time) override {
            const Vec3 value{(node.value.bytes[15]==4 ? 20.0 : 10.0)*time*time,0,0};
            if(std::none_of(captured.begin(),captured.end(),[&](const auto& s){return s.emitter==node && s.birth_seconds==time;})) captured.push_back({node,time,value});
            return Result<Vec3>::success(value);
        }
    } path;
    Settings settings;settings.birth_rate=2;settings.particle_lifetime_seconds=5;settings.velocity={};settings.velocity_spread=0;settings.emitter_origin={99,0,0};
    auto made=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)});
    auto graph=made.take_value();
    const auto first=evaluate_particle_graph(graph,{1,1},never,{},&path);
    check(first.has_value() && first.value().particles.size()==3,"birth path emits at distinct historical origins");
    if(!first.has_value()) return;
    for(const auto& p:first.value().particles) check(std::abs(p.position.x-10*std::pow(double(p.id)/2,2))<1e-12,"nonlinear origin sampled at subframe birth");
    const auto encoded=encode_emitter_origin_history(path.captured);
    check(encoded.has_value(),"encode immutable birth history");
    if(!encoded.has_value()) return;
    graph.optional_records.push_back(encoded.value());
    const auto bytes=serialize_graph(graph,particle_node_registry());
    check(bytes.has_value(),"birth history survives Core graph transport");
    auto decoded=deserialize_graph(bytes.value(),particle_node_registry());
    check(decoded.has_value(),"decode transported birth history");
    const auto replay=evaluate_particle_graph(decoded.value(),{1,1},never);
    check(replay.has_value() && replay.value().particles.size()==first.value().particles.size(),"DLL evaluation uses frozen birth positions without host callbacks");
    if(replay.has_value()) for(std::size_t i=0;i<replay.value().particles.size();++i)
        check(replay.value().particles[i].position.x==first.value().particles[i].position.x,"frozen history repeats exact positions");
    graph.optional_records.clear();
    const auto later=evaluate_particle_graph(graph,{2,1},never,{},&path);
    check(later.has_value() && later.value().particles.size()==5,"later frame includes new births");
    if(later.has_value()) for(const auto& old:first.value().particles) {
        const auto found=std::find_if(later.value().particles.begin(),later.value().particles.end(),[&](const auto& p){return p.id==old.id;});
        check(found!=later.value().particles.end() && found->position.x==old.position.x,"emitter movement leaves older stationary particles in place");
    }
    const auto reverse=evaluate_particle_graph(graph,{1,1},never,{},&path);
    check(reverse.has_value() && reverse.value().particles[1].position.x==first.value().particles[1].position.x,"reverse-frame evaluation is independent of later history");
    auto missing=encoded.value();missing.resize(12);missing[4]=std::byte{12};missing[5]=std::byte{0};missing[6]=std::byte{0};missing[7]=std::byte{0};
    for(std::size_t i=8;i<12;++i) missing[i]=std::byte{0};
    graph.optional_records={missing};check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"missing birth positions do not fall back to current origin");
    graph.optional_records={encoded.value()};graph.optional_records[0][4]=std::byte{0};
    check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"malformed history rejected before simulation");
    auto auxiliary=auxiliary_graph();Path aux_path;
    const auto children=evaluate_particle_graph(auxiliary,{5,4},never,{},&aux_path);
    check(children.has_value() && std::any_of(children.value().particles.begin(),children.value().particles.end(),[](const auto& p){return std::abs(p.position.x-5.5)<1e-9;}),
          "Auxiliary children keep parent's historical origin and their own birth offset");
}
struct TemporalFixture final:TemporalGraphSampler {
    Graph graph;
    std::function<double(NodeId,double)> emission;
    std::function<void(GraphNode&,double)> animate;
    Result<GraphNode> node(NodeId id,double seconds) override {
        auto found=std::find_if(graph.nodes.begin(),graph.nodes.end(),[&](const auto& n){return n.id==id;});
        if(found==graph.nodes.end()) return Result<GraphNode>::failure(ErrorCode::invalid_request,"fixture node missing");
        auto sampled=*found;if(animate) animate(sampled,seconds);return Result<GraphNode>::success(std::move(sampled));
    }
    Result<double> rate(NodeId id,double seconds) override {
        if(emission) return Result<double>::success(emission(id,seconds));
        auto sampled=node(id,seconds);if(!sampled.has_value()) return Result<double>::failure(sampled.error());
        for(const auto& p:sampled.value().parameters) if(p.key==kBirthRate) return Result<double>::success(std::get<double>(p.value));
        return Result<double>::failure(ErrorCode::invalid_request,"fixture rate missing");
    }
};
void test_temporal_controls() {
    Settings settings;settings.birth_rate=4;settings.particle_lifetime_seconds=5;settings.velocity={};settings.velocity_spread=0;
    settings.particle_count=100;settings.particle_size=10;settings.opacity=1;
    auto made=make_emitter_particle_force_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(3)},NodeId{uuid(255)},
        EdgeId{uuid(11)},EdgeId{uuid(12)},EdgeId{uuid(13)});
    check(made.has_value(),"construct temporal reference graph");
    TemporalFixture fixture;fixture.graph=made.take_value();
    fixture.emission=[](NodeId,double t){return t<1?4.0:0.0;};
    fixture.animate=[](GraphNode& n,double t) {
        if(n.type_key==kEmitterNode) {
            set(n,kEmitterOrigin,Vec3{t*t,0,0});set(n,kVelocity,Vec3{1+t,0,0});
            set(n,kEmitterShape,std::uint32_t{0});set(n,kSeed,std::uint32_t(100+t*100));
            set(n,kEmissionSpeed,0.0);
        }
        if(n.type_key==kParticleNode) {
            set(n,kParticleLifetimeSeconds,5+t);set(n,kSizeStart,10+t);set(n,kOpacityStart,1-t/10);
            set(n,kParticleColorMode,std::uint32_t{0});set(n,kColorStart,Vec3{t/2,0.25,0.5});
        }
    };
    auto evaluate=[&](double t) {return evaluate_temporal_particle_graph(fixture.graph,{std::int64_t(std::llround(t*1000000)),1000000},never,{1080,1},fixture);};
    const auto first=evaluate(0.75), later=evaluate(2), reverse=evaluate(0.75);
    check(first.has_value() && first.value().particles.size()==4,"historical rate produces stable first four births");
    check(later.has_value() && later.value().particles.size()==5,"rate zero keeps historical births alive including boundary threshold");
    check(reverse.has_value() && first.has_value() && reverse.value().particles.size()==first.value().particles.size(),"reverse timeline count identical");
    if(first.has_value() && later.has_value()) for(const auto& old:first.value().particles) {
        const double birth=double(old.id)/4;
        const auto found=std::find_if(later.value().particles.begin(),later.value().particles.end(),[&](const auto& p){return p.id==old.id;});
        check(found!=later.value().particles.end(),"later rate never removes a live older birth");
        if(found==later.value().particles.end()) continue;
        check(std::abs(found->position.x-(birth*birth+(1+birth)*(2-birth)))<1e-8,"speed/direction and Origin are birth values");
        check(found->size_pixels==old.size_pixels && found->size_pixels==10+birth,"base Size is sampled at birth");
        check(found->opacity==old.opacity && std::abs(found->opacity-(1-birth/10))<1e-10,"base Opacity is sampled at birth");
        check(found->color.x==old.color.x && std::abs(found->color.x-birth/2)<1e-10,"animated solid Color is sampled at birth");
        check(std::abs(found->lifetime_seconds-(5+birth))<1e-10,"Life is sampled at birth");
    }
    if(later.has_value()) {
        auto frozen=encode_evaluated_particles(later.value(),{2,1});check(frozen.has_value(),"encode whole immutable temporal result");
        auto graph=fixture.graph;graph.optional_records.push_back(frozen.value());
        auto serialized=serialize_graph(graph,particle_node_registry());check(serialized.has_value(),"temporal snapshot crosses Core codec");
        auto decoded=deserialize_graph(serialized.value(),particle_node_registry());check(decoded.has_value(),"decode temporal snapshot graph");
        auto replay=evaluate_particle_graph(decoded.value(),{48,24},never,{1080,1});
        check(replay.has_value() && replay.value().particles.size()==later.value().particles.size(),"frozen render accepts equivalent rational time");
        if(replay.has_value()) for(std::size_t i=0;i<replay.value().particles.size();++i)
            check(replay.value().particles[i].position.x==later.value().particles[i].position.x &&
                  replay.value().particles[i].color.x==later.value().particles[i].color.x,"frozen pixels preserve double-valued trajectory and colors");
        check(!evaluate_particle_graph(decoded.value(),{3,1},never).has_value(),"stale frozen frame is rejected");
        auto invalid=frozen.value();invalid.pop_back();check(!decode_evaluated_particles(invalid,{2,1}).has_value(),"truncated temporal record rejected");
    }
    // Linear rate 2+2t has integral 2t+t*t, independent of current rate 6.
    fixture.emission=[](NodeId,double t){return 2+2*t;};
    fixture.animate=[](GraphNode& n,double t) {
        if(n.type_key==kEmitterNode) {set(n,kVelocity,Vec3{});set(n,kEmitterOrigin,Vec3{t,0,0});}
        if(n.type_key==kParticleNode) set(n,kParticleLifetimeSeconds,5.0);
    };
    auto ramp=evaluate(2);
    check(ramp.has_value() && ramp.value().particles.size()==9,"linear rate uses integrated emission rather than current PPS");
    if(ramp.has_value()) for(const auto& p:ramp.value().particles)
        check(std::abs(p.position.x-(-1+std::sqrt(1+double(p.id))))<1e-8,"integrated rate inversion gives each actual birth time");
    // A positive-to-zero hold and a delayed start must not divide by zero.
    fixture.emission=[](NodeId,double t){return t<1?0.0:4.0;};
    auto delayed=evaluate(1.5);
    check(delayed.has_value() && delayed.value().particles.size()==3,"zero interval creates no phantom births");
    if(delayed.has_value()) for(const auto& p:delayed.value().particles)
        check(std::abs(p.position.x-(1+double(p.id)/4))<1e-8,"delayed emission preserves absolute birth times");
    fixture.emission=[](NodeId,double){return 4.0;};
    fixture.animate=[](GraphNode& n,double t) {
        if(n.type_key==kEmitterNode) {set(n,kVelocity,Vec3{});set(n,kEmitterOrigin,Vec3{});}
        if(n.type_key==kParticleNode) set(n,kParticleLifetimeSeconds,t<1?3.0:0.2);
        if(n.type_key==kForceNode) set(n,kGravity,Vec3{t<1?0.0:2.0,0,0});
    };
    auto force=evaluate(2);
    check(force.has_value() && force.value().particles.size()==5,"later Life keys do not kill earlier births");
    if(force.has_value()) {
        auto oldest=std::find_if(force.value().particles.begin(),force.value().particles.end(),[](const auto& p){return p.id==0;});
        check(oldest!=force.value().particles.end() && std::abs(oldest->position.x-1)<1e-8 &&
            std::abs(oldest->velocity.x-2)<1e-8,"Force acts only during the time after its key");
    }
    fixture.animate=[](GraphNode& n,double t) {
        if(n.type_key==kEmitterNode) {set(n,kVelocity,Vec3{1,0,0});set(n,kEmitterOrigin,Vec3{});}
        if(n.type_key==kParticleNode) set(n,kParticleLifetimeSeconds,5.0);
        if(n.type_key==kForceNode) set(n,kLinearDrag,t<1?0.0:1.0);
    };
    auto drag=evaluate(2);
    if(drag.has_value()) {
        const auto& p=drag.value().particles.front();
        check(std::abs(p.position.x-(2-std::exp(-1)))<1e-8 && std::abs(p.velocity.x-std::exp(-1))<1e-8,
            "Air Density integrates its lived interval without retroactive drag");
    } else check(false,"animated drag evaluates");
    fixture.animate=[](GraphNode& n,double) {
        if(n.type_key==kEmitterNode) {set(n,kVelocity,Vec3{});set(n,kEmitterOrigin,Vec3{});}
        if(n.type_key==kParticleNode) set(n,kParticleLifetimeSeconds,5.0);
        if(n.type_key==kForceNode) {set(n,kGravity,Vec3{});set(n,kLinearDrag,0.0);set(n,kSpin,1.0);set(n,kSpinFrequency,0.5);}
    };
    auto spin=evaluate(0.5);
    check(spin.has_value(),"temporal Spin evaluates");
    if(spin.has_value()) {
        const auto& p=spin.value().particles.front();
        check(std::abs(p.velocity.x+3.14159265358979323846)<1e-8 && std::abs(p.velocity.y)<1e-8,
              "Auxiliary inheritance receives instantaneous Spin field velocity");
    }
    // Auxiliary traversal must not leak parent particles to the final Output.
    fixture.graph=auxiliary_graph();fixture.emission={};fixture.animate={};
    auto aux=evaluate(1.25);
    check(aux.has_value() && !aux.value().particles.empty(),"temporal Auxiliary graph evaluates parent birth history");
    if(aux.has_value()) for(const auto& p:aux.value().particles)
        check(p.emitter_id==NodeId{uuid(4)},"Auxiliary-only Output does not draw its disconnected parents");
    struct Cancel final:Cancellation {bool is_cancelled() const noexcept override{return true;}} cancel;
    auto cancelled=evaluate_temporal_particle_graph(fixture.graph,{2,1},cancel,{1080,1},fixture);
    check(!cancelled.has_value() && cancelled.error().code==ErrorCode::cancelled,"temporal cancellation is a normal cancelled result");
    fixture.emission=[](NodeId,double){return std::numeric_limits<double>::quiet_NaN();};
    check(!evaluate(2).has_value(),"nonfinite historical rates are rejected");
}
void test_particle_gradient() {
    auto gradient=white_gradient();gradient.count=3;
    gradient.stops[0]={0,{1,0,0}};gradient.stops[1]={0.3,{0,1,0}};gradient.stops[2]={1,{0,0,1}};
    auto bytes=encode_color_gradient(gradient);ColorGradient decoded;
    check(decode_color_gradient(bytes,decoded) && decoded.count==3,"multi-stop Color Gradient round trips");
    auto sampled=evaluate_color_gradient(decoded,.15);
    check(sampled.x==.5 && sampled.y==.5 && sampled.z==0,"gradient honors nonuniform stop positions");
    auto invalid=bytes;invalid[3]=std::byte{1};check(!decode_color_gradient(invalid,decoded),"gradient reserved byte checked");
    invalid=bytes;invalid[2]=std::byte{2};check(!decode_color_gradient(invalid,decoded),"unsupported gradient interpolation checked");
    auto hold=bytes;hold[2]=std::byte{1};check(decode_color_gradient(hold,decoded) && decoded.interpolation==ColorInterpolation::hold,"gradient Hold mode remains valid");
    auto inset=gradient;inset.stops[0].position=.2;inset.stops[2].position=.8;
    const auto inset_bytes=encode_color_gradient(inset);
    check(!inset_bytes.empty() && decode_color_gradient(inset_bytes,decoded),"inset first/last positions round trip in the gradient codec");
    check(evaluate_color_gradient(decoded,0).x==1 && evaluate_color_gradient(decoded,1).z==1,"inset gradient holds both end colors without extrapolation");
    const auto interior=evaluate_color_gradient(decoded,.25);
    check(std::abs(interior.x-.5)<1e-12 && std::abs(interior.y-.5)<1e-12,"interpolation uses the actual movable boundary positions");
    gradient.stops[1].position=0;check(encode_color_gradient(gradient).empty(),"duplicate color stops rejected");
    TemporalFixture fixture;Settings settings;settings.birth_rate=2;settings.particle_lifetime_seconds=4;
    settings.velocity={};settings.velocity_spread=0;
    fixture.graph=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    set(fixture.graph.nodes[1],kColorGradient,bytes);
    for(std::uint32_t mode=0;mode<4;++mode) {
        set(fixture.graph.nodes[1],kParticleColorMode,mode);
        auto a=evaluate_temporal_particle_graph(fixture.graph,{1,1},never,{1080,1},fixture);
        auto b=evaluate_temporal_particle_graph(fixture.graph,{2,1},never,{1080,1},fixture);
        check(a.has_value() && b.has_value(),"all four Particle Color modes render");
        if(!a.has_value() || !b.has_value()) continue;
        const auto& p=a.value().particles[0];const auto& later=b.value().particles[0];
        if(mode==0) check(p.color.x==1 && p.color.y==1 && later.color.x==1,"Solid color ignores gradient");
        if(mode==1) check(p.color.x>0 && p.color.y>0 && later.color.z>0,"Color over life walks through multi-stop gradient");
        if(mode==2) check(p.color.x==later.color.x && p.color.y==later.color.y && p.color.z==later.color.z,"random gradient color stays attached to particle identity");
        if(mode==3) check(p.color.x!=later.color.x || p.color.y!=later.color.y || p.color.z!=later.color.z,"loop gradient ages from stable random offset");
    }
    set(fixture.graph.nodes[1],kColorGradient,inset_bytes);
    for(std::uint32_t mode=1;mode<4;++mode) {
        set(fixture.graph.nodes[1],kParticleColorMode,mode);
        auto rendered=evaluate_temporal_particle_graph(fixture.graph,{1,1},never,{1080,1},fixture);
        check(rendered.has_value(),"all gradient modes accept movable boundary colors during temporal evaluation");
        if(rendered.has_value())for(const auto& p:rendered.value().particles)
            check(p.color.x>=0 && p.color.x<=1 && p.color.y>=0 && p.color.y<=1 && p.color.z>=0 && p.color.z<=1,"movable boundaries never extrapolate invalid particle colors");
    }
}
void test_birth_parameter_matrix() {
    Settings settings;settings.birth_rate=4;settings.particle_lifetime_seconds=5;settings.velocity_spread=.2;
    settings.emission_speed=3;settings.emission_speed_random=.3;settings.particle_size=10;settings.opacity=.8;
    settings.emitter_size_pixels={100,200,300};
    auto graph=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    struct Change {const char* name;std::string type;ParameterKey key;ParameterValue value;};
    const Change cases[]={
        {"Shape",kEmitterNode,kEmitterShape,std::uint32_t{1}},
        {"Seed",kEmitterNode,kSeed,std::uint32_t{913}},
        {"Speed",kEmitterNode,kEmissionSpeed,11.0},
        {"Speed amplitude",kEmitterNode,kEmissionSpeedRandom,1.5},
        {"Speed Random",kEmitterNode,kEmissionSpeedRandomPercent,80.0},
        {"Angle X",kEmitterNode,kEmissionAngleX,45.0},{"Angle Y",kEmitterNode,kEmissionAngleY,90.0},
        {"Angle Z",kEmitterNode,kEmissionAngleZ,135.0},{"Direction",kEmitterNode,kDirectionMode,std::uint32_t{1}},
        {"Direction Span",kEmitterNode,kDirectionSpan,12.0},
        {"Velocity",kEmitterNode,kVelocity,Vec3{2,3,4}},{"Velocity spread",kEmitterNode,kVelocitySpread,2.0},
        {"Size X",kEmitterNode,kEmitterSizeX,400.0},{"Size Y",kEmitterNode,kEmitterSizeY,500.0},
        {"Size Z",kEmitterNode,kEmitterSizeZ,600.0},{"Disc extent",kEmitterNode,kEmitterSize,.8},
        {"Life",kParticleNode,kParticleLifetimeSeconds,1.5},
        {"Life Random",kParticleNode,kLifeRandom,50.0},
        {"Particle Shape",kParticleNode,kParticleShape,std::uint32_t{1}},
        {"Size Y",kParticleNode,kSizeY,3.0},
        {"Particle angles",kParticleNode,kParticleAngles,Vec3{30,60,90}},
        {"Particle spin",kParticleNode,kRotationSpeed,Vec3{20,30,40}},
        {"Angle random",kParticleNode,kAngleRandom,75.0},
        {"Spin random",kParticleNode,kRotationSpeedRandom,75.0},
        {"Orient To",kParticleNode,kOrientTo,std::uint32_t{1}},
        {"Limit to 2D",kParticleNode,kLimitTo2D,std::uint32_t{0}},
        {"Feather",kParticleNode,kParticleFeather,75.0},
        {"Up Axis",kParticleNode,kUpAxis,std::uint32_t{1}},
        {"Size",kParticleNode,kSizeStart,40.0},{"Opacity",kParticleNode,kOpacityStart,.25},
        {"Size Random",kParticleNode,kSizeRandom,75.0},{"Opacity Random",kParticleNode,kOpacityRandom,75.0},
        {"Color",kParticleNode,kColorStart,Vec3{.2,.4,.6}},
        {"Size over life multiplier",kParticleNode,kSizeEnd,20.0},
        {"Opacity over life multiplier",kParticleNode,kOpacityEnd,30.0},
    };
    for(const auto& change:cases) {
        TemporalFixture fixture;fixture.graph=graph;
        fixture.animate=[&](GraphNode& node,double time){if(node.type_key==change.type && time>=.5) set(node,change.key,change.value);};
        auto temporal=evaluate_temporal_particle_graph(fixture.graph,{1,1},never,{1080,1},fixture);
        auto old=evaluate_particle_graph(graph,{1,1},never,{1080,1});
        auto changed=graph;for(auto& node:changed.nodes) if(node.type_key==change.type) set(node,change.key,change.value);
        auto newborns=evaluate_particle_graph(changed,{1,1},never,{1080,1});
        check(temporal.has_value() && old.has_value() && newborns.has_value(),change.name);
        if(!temporal.has_value() || !old.has_value() || !newborns.has_value()) continue;
        for(std::uint64_t id:{0ULL,3ULL}) {
            const auto& expected=id==0?old.value().particles[id]:newborns.value().particles[id];
            const auto found=std::find_if(temporal.value().particles.begin(),temporal.value().particles.end(),[&](const auto& p){return p.id==id;});
            check(found!=temporal.value().particles.end(),change.name);
            if(found==temporal.value().particles.end()) continue;
            check(std::abs(found->position.x-expected.position.x)<1e-9 && std::abs(found->position.y-expected.position.y)<1e-9 &&
                std::abs(found->position.z-expected.position.z)<1e-9 && std::abs(found->size_pixels-expected.size_pixels)<1e-9 &&
                std::abs(found->opacity-expected.opacity)<1e-9 && std::abs(found->color.x-expected.color.x)<1e-9 &&
                std::abs(found->lifetime_seconds-expected.lifetime_seconds)<1e-9 &&
                found->shape==expected.shape && found->limit_to_2d==expected.limit_to_2d && found->up_axis==expected.up_axis &&
                found->size_y_pixels==expected.size_y_pixels && found->feather_percent==expected.feather_percent &&
                std::abs(found->rotation_degrees.x-expected.rotation_degrees.x)<1e-9 &&
                std::abs(found->rotation_degrees.y-expected.rotation_degrees.y)<1e-9 &&
                std::abs(found->rotation_degrees.z-expected.rotation_degrees.z)<1e-9,change.name);
        }
    }
}
void test_particle_geometry() {
    Settings settings;settings.birth_rate=20;settings.particle_lifetime_seconds=2;settings.particle_count=5;
    auto graph=make_emitter_particle_output_graph(settings,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    set(graph.nodes[1],kLifeRandom,100.0);
    auto random=evaluate_particle_graph(graph,{2,1},never);
    check(random.has_value() && random.value().particles.size()==5,"Life Random cap counts actual survivors");
    if(random.has_value()) for(const auto& p:random.value().particles)
        check(p.age_seconds<p.lifetime_seconds && p.lifetime_seconds<=2,"random lifetime is bounded and expired births are excluded");
    set(graph.nodes[1],kLifeRandom,101.0);check(!evaluate_particle_graph(graph,{2,1},never).has_value(),"invalid Life Random is rejected");
    EvaluatedGraph frozen;ParticleInstance particle;particle.id=7;particle.lifetime_seconds=2;particle.age_seconds=.5;
    particle.size_pixels=20;particle.size_y_pixels=6;particle.opacity=.5;
    const auto render=[&](ParticleInstance value,PixelFormat format=PixelFormat::rgba8,RectI roi={0,0,100,100}) {
        frozen.particles={value};auto bytes=encode_evaluated_particles(frozen,{1,2});
        check(bytes.has_value(),"sprite snapshot encodes additional fields");
        auto captured=graph;captured.optional_records={bytes.take_value()};
        RenderRequest request;request.settings=validate_settings(settings);request.graph=std::make_shared<const Graph>(captured);
        request.frame={100,100,100,100,roi,{1,2},{1,24},format,ColorSpace::ae_working_space,AlphaMode::straight,1,Quality::full};
        return CpuParticleRenderer{}.render(request,never);
    };
    particle.shape=0;auto circle=render(particle);particle.shape=1;auto rectangle=render(particle);
    particle.shape=2;auto cloud=render(particle);
    check(circle.has_value() && rectangle.has_value() && cloud.has_value(),"Circle Rectangle and clustered Cloud render");
    if(circle.has_value() && rectangle.has_value() && cloud.has_value()) {
        check(circle.value().pixels!=rectangle.value().pixels && circle.value().pixels!=cloud.value().pixels,"shape choice changes output pixels");
        check(std::to_integer<unsigned>(rectangle.value().pixels[(50*100+50)*4+3])==128,"rectangle retains translucent alpha without black matte");
    }
    particle.shape=1;particle.rotation_degrees.z=90;auto rotated=render(particle);
    check(rotated.has_value() && rectangle.has_value() && rotated.value().pixels!=rectangle.value().pixels,"Z angle rotates nonsquare rectangles");
    particle.rotation_degrees={90,0,0};particle.limit_to_2d=false;auto edge=render(particle);
    check(edge.has_value() && std::all_of(edge.value().pixels.begin(),edge.value().pixels.end(),[](auto b){return b==std::byte{0};}),"3D edge-on rectangle has no coverage");
    particle.rotation_degrees={};particle.limit_to_2d=true;particle.feather_percent=80;auto feather=render(particle);
    check(feather.has_value() && rectangle.has_value() && feather.value().pixels!=rectangle.value().pixels,"Particle Feather changes rectangle coverage");
    for(auto format:{PixelFormat::rgba8,PixelFormat::rgba16,PixelFormat::rgba32f}) {
        auto full=render(particle,format),crop=render(particle,format,{43,47,57,53});
        check(full.has_value() && crop.has_value(),"rotated and feathered sprites render every bit depth and ROI");
        if(full.has_value() && crop.has_value()) for(unsigned y=0;y<6;++y)
            check(std::memcmp(crop.value().pixels.data()+y*crop.value().row_bytes,
                full.value().pixels.data()+(y+47)*full.value().row_bytes+43*bytes_per_pixel(format),crop.value().row_bytes)==0,"ROI pixels match full frame");
    }
    auto bytes=encode_evaluated_particles(frozen,{1,2});auto restored=decode_evaluated_particles(bytes.value(),{1,2});
    check(restored.has_value() && restored.value().particles[0].shape==particle.shape &&
        restored.value().particles[0].feather_percent==particle.feather_percent,"version 2 preserves sprite fields");
    auto malformed=bytes.take_value();malformed[2]=std::byte{1};check(!decode_evaluated_particles(malformed,{1,2}).has_value(),"unpaired old transient snapshot is rejected");
}
void test_rotation_controls() {
    Settings s;s.birth_rate=10;s.particle_lifetime_seconds=2;s.particle_count=100;s.emission_speed=1;s.direction_mode=DirectionMode::directional;
    auto graph=make_emitter_particle_output_graph(s,NodeId{uuid(1)},NodeId{uuid(2)},NodeId{uuid(255)},EdgeId{uuid(11)},EdgeId{uuid(12)}).take_value();
    auto base=evaluate_particle_graph(graph,{1,1},never);check(base.has_value() && !base.value().particles.empty(),"rotation baseline emits particles");
    if(!base.has_value() || base.value().particles.empty())return;
    check(!base.value().particles[0].limit_to_2d,"Limit To 2D defaults off");
    set(graph.nodes[1],kAnchorX,25.0);set(graph.nodes[1],kAnchorY,75.0);
    AgeCurve curve;curve.count=2;curve.points[0]={0,0};curve.points[1]={1,360};set(graph.nodes[1],kRotationOverLife,encode_age_curve(curve));
    auto rotated=evaluate_particle_graph(graph,{1,1},never);check(rotated.has_value(),"degree Rotation Over Life evaluates");
    if(rotated.has_value()) {
        for(const auto& p:rotated.value().particles){check(std::abs(p.rotation_degrees.z-p.age_seconds/p.lifetime_seconds*360)<1e-9,"rotation curve uses normalized particle age");check(p.anchor_x_percent==25 && p.anchor_y_percent==75,"anchors reach evaluated particles");}
        auto bytes=encode_evaluated_particles(rotated.value(),{1,1});auto decoded=decode_evaluated_particles(bytes.value(),{1,1});
        check(decoded.has_value() && decoded.value().particles[0].anchor_x_percent==25 && decoded.value().particles[0].anchor_y_percent==75,"snapshot3 preserves both anchors");
    }
    curve.points[1].value=0;set(graph.nodes[1],kRotationOverLife,encode_age_curve(curve));
    set(graph.nodes[1],kAngleRandom,100.0);set(graph.nodes[1],kRandomLimit,std::uint32_t{1});set(graph.nodes[1],kLimitAngle,10.0);
    auto limited=evaluate_particle_graph(graph,{1,1},never);check(limited.has_value(),"All Axis random limit evaluates");
    if(limited.has_value())for(const auto& p:limited.value().particles)check(std::abs(p.rotation_degrees.x)<=10 && std::abs(p.rotation_degrees.y)<=10 && std::abs(p.rotation_degrees.z)<=10,"All Axis bounds birth angle variation");
    set(graph.nodes[1],kRandomLimit,std::uint32_t{5});check(!evaluate_particle_graph(graph,{1,1},never).has_value(),"invalid random limit rejected");
    set(graph.nodes[1],kRandomLimit,std::uint32_t{0});set(graph.nodes[1],kAngleRandom,0.0);
    set(graph.nodes[0],kEmissionAngleZ,90.0);auto shape_only=evaluate_particle_graph(graph,{1,1},never);
    check(shape_only.has_value() && std::abs(shape_only.value().particles[0].velocity.x-base.value().particles[0].velocity.x)<1e-9,"Emitter Angle does not rotate directional cone");
    set(graph.nodes[0],kEmitterOrient,Vec3{0,0,90});auto direction=evaluate_particle_graph(graph,{1,1},never);
    check(direction.has_value() && std::abs(direction.value().particles[0].velocity.x-base.value().particles[0].velocity.x)>0.01,"Emitter Orient rotates directional cone");
}
void test_preset_catalog(const char* path) {
    std::ifstream catalog(path);check(bool(catalog),"paired JavaScript catalog exists");
    std::string line;unsigned count=0;
    while(std::getline(catalog,line)) {
        if(line.empty())continue;++count;OpaqueBytes bytes;
        for(std::size_t i=0;i+1<line.size();i+=2)bytes.push_back(static_cast<std::byte>(std::stoul(line.substr(i,2),nullptr,16)));
        auto decoded=deserialize_graph(bytes,particle_node_registry());check(decoded.has_value(),"actual manager preset passes authoritative Core codec/registry");
        if(!decoded.has_value())continue;
        auto evaluated=evaluate_particle_graph(decoded.value(),{1,1},never,{1080,1});
        check(evaluated.has_value() && !evaluated.value().particles.empty(),"manager preset emits particles through actual Core evaluation");
    }
    check(count==6,"all six manager presets evaluated");
}
int main(int argc,char* argv[]) {test_auxiliary();test_camera();test_reference_force_and_globals();test_birth_origins();test_temporal_controls();test_particle_gradient();test_birth_parameter_matrix();test_particle_geometry();test_rotation_controls();if(argc>1)test_preset_catalog(argv[1]);std::printf("%d checks, %d failures\n",checks,failures);return failures?1:0;}
