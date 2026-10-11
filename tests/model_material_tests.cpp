#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/ModelLighting.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include "starfield/core/MotionBlur.hpp"
#include "starfield/core/PluginApi.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <limits>
using namespace starfield::core;
using namespace graph_keys;
static unsigned checks{},failures{};
void check(bool value,const char* name){++checks;if(!value){++failures;std::printf("FAIL %s\n",name);}}
NodeId node(unsigned n){NodeId id;id.value.bytes[15]=static_cast<unsigned char>(n);return id;}
EdgeId edge(unsigned n){return {node(n).value};}
Graph fixture(bool legacy=false){return {{
    {node(1),kOutputNode,4,{{kParticleCount,std::uint32_t{1000}}}},
    {node(2),kModelNode,1,{}},
    {node(3),kMaterialNode,1,{{kMaterialColorSource,std::uint32_t{1}},{kMaterialColor,Vec3{.2,.4,.8}},
        {kMaterialRoughness,.5},{kMaterialMetallic,0.},{kMaterialOpacity,1.},{kMaterialLights,std::uint32_t{1}}}}},
    legacy?std::vector<GraphEdge>{{edge(1),node(2),kModelGeometryOut,node(1),kOutputParticles},{edge(2),node(3),kMaterialOut,node(2),kModelMaterialIn}}:
        std::vector<GraphEdge>{{edge(1),node(3),kMaterialOut,node(1),kOutputParticles},{edge(2),node(2),kModelGeometryOut,node(3),kMaterialGeometryIn}}, {}};}
struct Sampler:TemporalGraphSampler{Graph graph;explicit Sampler(Graph g):graph(std::move(g)){}
    Result<GraphNode> node(NodeId id,double seconds)override{for(auto n:graph.nodes)if(n.id==id){if(n.type_key==kMaterialNode)n.parameters[2].value=std::clamp(seconds,0.,1.);return Result<GraphNode>::success(n);}return Result<GraphNode>::failure(ErrorCode::invalid_request,"missing");}
    Result<double> rate(NodeId,double)override{return Result<double>::success(0);}};
int main(){NeverCancelled cancel;auto graph=fixture();
    check(validate_graph(graph,particle_node_registry()).ok(),"typed Material/Model/Output graph");
    auto evaluated=evaluate_particle_graph(graph,{0,1},cancel,{100,1});check(evaluated.has_value(),"direct model evaluates");
    if(evaluated.has_value()){
        check(evaluated.value().particles.size()==1,"one direct model");
        const auto& p=evaluated.value().particles[0];check(p.shape==4&&p.size_pixels==100,"visible default cube");
        check(evaluated.value().model_styles[p.model_style_index-1].instances[0].material.enabled,"material attached");
        auto bytes=encode_evaluated_particles(evaluated.value(),{0,1});check(bytes.has_value(),"snapshot encode");
        if(bytes.has_value()){check(std::to_integer<unsigned>(bytes.value()[2])==11,"snapshot11");auto decoded=decode_evaluated_particles(bytes.value(),{0,1});check(decoded.has_value(),"snapshot11 roundtrip");
            if(decoded.has_value())check(decoded.value().model_styles.back().instances[0].material.color.y==.4,"snapshot material retained");
            auto truncated=bytes.value();truncated.pop_back();check(!decode_evaluated_particles(truncated,{0,1}).has_value(),"truncated snapshot rejected");}
    }
    Sampler sampler(graph);auto temporal=evaluate_temporal_particle_graph(graph,{3,4},cancel,{100,1},sampler);
    check(temporal.has_value(),"temporal direct model");if(temporal.has_value())check(temporal.value().model_styles.back().instances[0].material.roughness==.75,"animated material sampled");
    auto saved=serialize_graph(graph,particle_node_registry());check(saved.has_value(),"material graph persists");
    if(saved.has_value())check(deserialize_graph(saved.value(),particle_node_registry()).has_value(),"material graph reads");
    ModelMaterial m;m.enabled=true;m.color_source=1;m.color={.2,.4,.8};
    const ModelLight ambient{ModelLightType::ambient};
    auto c=model_shading::shade(m,{1,0,0},{},{0,0,-1},{0,0,-1},{&ambient,1});check(c.x==.2&&c.z==.8,"independent color");
    m.color_source=0;c=model_shading::shade(m,{1,0,0},{},{0,0,-1},{0,0,-1},{&ambient,1});check(c.x==1&&c.y==0,"inherited color");
    ModelLight parallel;parallel.type=ModelLightType::parallel;m.color_source=1;m.roughness=.1;
    auto glossy=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&parallel,1});m.roughness=.9;
    auto rough=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&parallel,1});check(glossy.x>rough.x,"roughness changes GGX highlight");
    m.metallic=1;auto metal=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&parallel,1});check(std::abs(metal.z-rough.z)>1e-3,"metallic changes reflection");
    m.roughness=std::numeric_limits<double>::quiet_NaN();check(!valid_model_material(m),"NaN material rejected");
    ModelLighting lights;lights.lights.resize(kMaxModelLights+1);check(!valid_model_lighting(lights),"light count bounded");
    CpuParticleRenderer renderer;RenderRequest request;request.graph=std::make_shared<Graph>(graph);
    request.frame={100,100,100,100,{0,0,100,100},{0,1},{1,30},PixelFormat::rgba32f};
    request.lighting.lights={ambient};auto pixels=renderer.render(request,cancel);check(pixels.has_value(),"material pixels render");
    if(pixels.has_value()){const auto* p=reinterpret_cast<const float*>(pixels.value().pixels.data());const auto i=4*(50*100+50);
        check(std::abs(p[i]-.2)<1e-5&&std::abs(p[i+2]-.8)<1e-5&&p[i+3]==1,"material pixel color");}
    graph.nodes[2].parameters[4].value=.5;request.graph=std::make_shared<Graph>(graph);pixels=renderer.render(request,cancel);
    check(pixels.has_value(),"opacity pixels render");if(pixels.has_value()){const auto* p=reinterpret_cast<const float*>(pixels.value().pixels.data());check(std::abs(p[4*(50*100+50)+3]-.5)<1e-5,"material opacity");}
    graph.nodes[2].parameters[2].value=2.;check(!evaluate_particle_graph(graph,{0,1},cancel,{100,1}).has_value(),"out-of-range material rejects");
    graph=fixture();graph.nodes.push_back({node(4),kModelNode,1,{}});
    graph.edges.push_back({edge(3),node(4),kModelGeometryOut,node(3),kMaterialGeometryIn});
    check(!validate_graph(graph,particle_node_registry()),"Material rejects two geometry inputs");
    auto legacy=evaluate_particle_graph(fixture(true),{0,1},cancel,{100,1});
    check(legacy.has_value(),"legacy Material-to-Model remains readable");
    if(legacy.has_value())check(legacy.value().model_styles.back().instances[0].material.color.z==.8,"legacy shading retained");
    auto branches=fixture();auto second=branches.nodes[2];second.id=node(4);second.parameters[1].value=Vec3{1,0,0};branches.nodes.push_back(second);
    branches.edges.push_back({edge(3),node(2),kModelGeometryOut,node(4),kMaterialGeometryIn});
    branches.edges.push_back({edge(4),node(4),kMaterialOut,node(1),kOutputParticles});
    auto fanout=evaluate_particle_graph(branches,{0,1},cancel,{100,1});check(fanout.has_value(),"independent material branches evaluate");
    if(fanout.has_value()){check(fanout.value().particles.size()==2,"one geometry instance per branch");
        const auto& a=fanout.value().model_styles[fanout.value().particles[0].model_style_index-1].instances[0].material;
        const auto& b=fanout.value().model_styles[fanout.value().particles[1].model_style_index-1].instances[0].material;
        check(a.color.z!=b.color.z,"branch materials do not mutate shared geometry");}
    m={};m.enabled=true;m.color_source=1;m.color={.8,.2,.1};
    ModelLight point;point.type=ModelLightType::point;point.position={0,0,-50};
    auto near_light=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});
    check(near_light.x>near_light.y&&near_light.y>0,"point light illuminates color");
    point.falloff=1;point.radius=50;point.falloff_distance=100;point.position.z=-150;
    c=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});check(c.x==0,"smooth falloff end");
    point.falloff=2;point.position.z=-100;c=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});check(std::abs(c.x-near_light.x*.25)<1e-6,"inverse square falloff");
    point.type=ModelLightType::spot;point.falloff=0;point.cone_degrees=45;point.position={100,0,-100};
    c=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});check(c.x==0,"spot outside cone");
    point.position={0,0,-50};c=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});check(std::abs(c.x-near_light.x)<1e-6,"spot inside cone");
    m.ae_lights=false;c=model_shading::shade(m,{},{},{0,0,-1},{0,0,-1},{&point,1});check(c.x==.8,"lights disabled preserves base");
    auto transform=kIdentityModelMatrix;transform[0]=-2;transform[5]=3;transform[10]=4;
    auto normal=model_shading::normal({1,1,0},transform);check(normal.x<0&&normal.y>0&&std::abs(normal.x/normal.y+1.5)<1e-6,"normal inverse transpose with reflection");
    auto unlit=fixture(true);unlit.nodes.erase(unlit.nodes.begin()+2);unlit.edges.resize(1);
    auto old=evaluate_particle_graph(unlit,{0,1},cancel,{100,1});check(old.has_value(),"direct unlit model");
    if(old.has_value()){auto bytes=encode_evaluated_particles(old.value(),{0,1});check(bytes.has_value()&&std::to_integer<unsigned>(bytes.value()[2])<=10,"old unlit snapshot version retained");}
    graph=fixture();auto graph_bytes=serialize_graph(graph,particle_node_registry());SfCoreApi api{};
    check(StarfieldCore_GetApi(9,sizeof(api),&api)==1&&api.abi_version==9,"ABI9 available");
    SfCoreRenderRequest input{};input.struct_size=sizeof(input);input.frame={100,100,100,100,{0,0,100,100},0,1,1,30,2,0,0,1,1};
    input.graph_bytes=graph_bytes.value().data();input.graph_byte_count=graph_bytes.value().size();SfModelLight light{};light.struct_size=sizeof(light);light.intensity=1;
    light.color[0]=light.color[1]=light.color[2]=1;light.cone_degrees=90;light.cone_feather=.5;input.light_count=1;input.lights=&light;
    for(unsigned i=0;i<4;++i)input.lighting_layer_to_world[5*i]=1;
    SfCoreRenderResult out{};out.struct_size=sizeof(out);check(api.render(&input,&out)==SF_CORE_OK&&out.pixels,"ABI9 lit pixels");
    if(out.pixels){const float* pixel=static_cast<const float*>(out.pixels)+4*(50*100+50);check(std::abs(pixel[0]-.2)<1e-5&&std::abs(pixel[2]-.8)<1e-5,"ABI9 material light transport");}api.release_render_result(&out);
    input.light_count=65;check(api.render(&input,&out)==SF_CORE_INVALID_REQUEST&&!out.pixels,"ABI9 bad light count rejects");
    input.light_count=1;light.reserved=1;check(api.render(&input,&out)==SF_CORE_INVALID_REQUEST&&!out.pixels,"ABI9 reserved rejects");light.reserved=0;
    input.light_count=65;
    for(unsigned version:{7u,8u}){check(StarfieldCore_GetApi(version,sizeof(api),&api)==1,"legacy API available");
        const auto length=version==7?SF_CORE_ABI7_RENDER_REQUEST_SIZE:SF_CORE_ABI8_RENDER_REQUEST_SIZE;input.struct_size=length;
        std::vector<std::byte> storage(length);std::memcpy(storage.data(),&input,length);
        check(api.render(reinterpret_cast<const SfCoreRenderRequest*>(storage.data()),&out)==SF_CORE_OK&&out.pixels,"exact legacy prefix ignores absent lighting tail");api.release_render_result(&out);}
    std::printf("Model material: %u checks, %u failures\n",checks,failures);return failures?1:0;
}
