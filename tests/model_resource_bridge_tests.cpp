#define main model_controls_fixture_main
#include "model_controls_tests.cpp"
#undef main
#include "ModelMirrorTransaction.hpp"
#include "ModelRenderResources.hpp"
#include "SPBasic.h"
#include <string>

namespace {
constexpr int renderer=99;
struct Ref{int effect;int index;};
AEGP_StreamSuite6 stream_suite{};AEGP_EffectSuite4 effect_suite{};SPBasicSuite basic{};
PF_InData* current_data{};unsigned refs{},suites{},sets{},mesh_reads{},checkouts{},checkins{},mixes{};
int fail_write=-1;bool silent_write{},fail_checkin{},wrong_type{},wrong_count{};
std::array<AEGP_StreamVal2,1012> main_values{};
std::array<AEGP_StreamVal2,95> author_values{};
PF_ArbitraryH clone(PF_ArbitraryH h,bool mirror){PF_ArbitraryH copy{};PF_ArbParamsExtra e{};e.which_function=PF_Arbitrary_COPY_FUNC;
    e.u.copy_func_params.src_arbH=h;e.u.copy_func_params.dst_arbPH=&copy;
    CHECK((mirror?(e.id=1900,adapter::model_mirror_arbitrary_callback(current_data,&e)):
        (e.id=1503,adapter::model_geometry_arbitrary_callback(current_data,&e,1503)))==0);return copy;}
SPAPI SPErr acquire(const char* name,int32 version,const void** out){(void)version;
    if(!std::strcmp(name,kAEGPStreamSuite))*out=&stream_suite;
    else if(!std::strcmp(name,kAEGPEffectSuite))*out=&effect_suite;
    else return 516;++suites;return 0;}
SPAPI SPErr release(const char*,int32){CHECK(suites);--suites;return 0;}
SPAPI A_Err get_stream(AEGP_PluginID,AEGP_EffectRefH effect,A_long index,AEGP_StreamRefH* out){
    *out=reinterpret_cast<AEGP_StreamRefH>(new Ref{int(reinterpret_cast<std::uintptr_t>(effect)),int(index)});++refs;return 0;}
SPAPI A_Err dispose_stream(AEGP_StreamRefH ref){delete reinterpret_cast<Ref*>(ref);CHECK(refs);--refs;return 0;}
bool arb(Ref r){return r.effect==renderer?r.index>=755&&r.index<=1010:r.index==3;}
SPAPI A_Err get_type(AEGP_StreamRefH raw,AEGP_StreamType* type){*type=arb(*reinterpret_cast<Ref*>(raw))?AEGP_StreamType_ARB:AEGP_StreamType_OneD;return 0;}
SPAPI A_Err get_value(AEGP_PluginID,AEGP_StreamRefH raw,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* value){
    const auto r=*reinterpret_cast<Ref*>(raw);value->streamH=raw;value->val=r.effect==renderer?main_values.at(r.index):author_values.at(r.index);
    if(arb(r)){if(r.effect!=renderer)++mesh_reads;value->val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(clone(reinterpret_cast<PF_ArbitraryH>(value->val.arbH),r.effect==renderer));}return 0;}
SPAPI A_Err dispose_value(AEGP_StreamValue2* value){const auto r=*reinterpret_cast<Ref*>(value->streamH);
    if(arb(r)&&value->val.arbH)dispose(reinterpret_cast<PF_ArbitraryH>(value->val.arbH));return 0;}
SPAPI A_Err set_value(AEGP_PluginID,AEGP_StreamRefH raw,AEGP_StreamValue2* value){const auto r=*reinterpret_cast<Ref*>(raw);CHECK(r.effect==renderer);++sets;
    if(!silent_write){if(arb(r)){const auto copied=clone(reinterpret_cast<PF_ArbitraryH>(value->val.arbH),true);
        dispose(reinterpret_cast<PF_ArbitraryH>(main_values[r.index].arbH));main_values[r.index].arbH=reinterpret_cast<AEGP_ArbBlockVal>(copied);}
        else main_values[r.index]=value->val;}
    if(r.index==fail_write){fail_write=-1;return 512;}return 0;}
SPAPI A_Err num_effects(AEGP_LayerH,A_long* n){*n=1;return 0;}
SPAPI A_Err get_effect(AEGP_PluginID,AEGP_LayerH,A_long,AEGP_EffectRefH* out){*out=reinterpret_cast<AEGP_EffectRefH>(1);return 0;}
SPAPI A_Err effect_key(AEGP_EffectRefH,AEGP_InstalledEffectKey* key){*key=1;return 0;}
SPAPI A_Err effect_name(AEGP_InstalledEffectKey,A_char* name){std::strcpy(name,"org.starfieldfx.node.model");return 0;}
SPAPI A_Err dispose_effect(AEGP_EffectRefH){return 0;}
PF_Err checkout(PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out){++checkouts;
    if(index==1011){out->param_type=PF_Param_SLIDER;out->u.sd.value=wrong_count?2:static_cast<A_long>(main_values[index].one_d);}
    else {CHECK(index>=755&&index<=1010);out->param_type=wrong_type?PF_Param_SLIDER:PF_Param_ARBITRARY_DATA;
        out->u.arb_d.value=reinterpret_cast<PF_ArbitraryH>(main_values[index].arbH);}return 0;}
PF_Err checkin(PF_ProgPtr,PF_ParamDef*){++checkins;return fail_checkin?516:0;}
PF_Err mix(PF_ProgPtr,A_u_long size,const void* bytes){CHECK(size&&bytes&&locks==0);++mixes;return 0;}
core::Graph resource_graph(unsigned id=7,unsigned revision=77){
    core::Settings settings;settings.birth_rate=1;settings.emission_speed=0;settings.velocity={};settings.velocity_spread=0;settings.particle_size=10;
    auto graph=take(core::make_emitter_particle_output_graph(settings,nid(1),nid(2),nid(3),{nid(1).value},{nid(2).value}));
    for(auto& n:graph.nodes)if(n.id==nid(2))n.parameters.push_back({kParticleShape,std::uint32_t{4}});
    adapter::ModelAuthorCapture state;state.source=1;state.revision=revision;state.geometry=take(core::make_unit_cube());
    graph.nodes.push_back(take(adapter::model_author_graph_node(nid(id),state,never)));
    graph.edges.push_back({{nid(4).value},nid(id),kModelGeometryOut,nid(2),kParticleModelsIn});return graph;
}
void setup(PF_InData& data){clear();current_data=&data;refs=suites=sets=mesh_reads=checkouts=checkins=mixes=0;main_values={};author_values={};
    fail_write=-1;silent_write=fail_checkin=wrong_type=wrong_count=false;data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);
    data.current_time=24;data.time_step=1;data.time_scale=24;data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
    for(unsigned slot=0;slot<3;++slot){PF_ArbitraryH empty{};CHECK(adapter::create_model_mirror_parameter(&data,{},&empty,never)==0);
        main_values[755+slot].arbH=reinterpret_cast<AEGP_ArbBlockVal>(empty);}
    PF_ArbitraryH mesh{};CHECK(adapter::create_model_geometry_parameter(&data,take(core::make_unit_cube()),&mesh,never)==0);
    author_values[3].arbH=reinterpret_cast<AEGP_ArbBlockVal>(mesh);author_values[1].one_d=2;author_values[4].one_d=77;
    author_values[93].one_d=7;
}
core::OpaqueBytes mirror_bytes(){return handles.at(reinterpret_cast<PF_ArbitraryH>(main_values[755].arbH));}
void transaction(PF_InData& data){setup(data);auto graph=resource_graph();const auto before=mirror_bytes();
    {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));
        CHECK(tx.install(graph)==0&&main_values[1011].one_d==1&&mesh_reads==1);
        const auto value=adapter::read_model_mirror_parameter(&data,reinterpret_cast<PF_ArbitraryH>(main_values[755].arbH),never);
        CHECK(value.has_value()&&value.value().id==nid(7).value.bytes&&value.value().revision==77);
        CHECK(tx.rollback()==0&&main_values[1011].one_d==0&&mirror_bytes()==before);
        CHECK(tx.install(graph)==PF_Err_BAD_CALLBACK_PARAM);}
    CHECK(refs==0&&suites==0&&locks==0);
    {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(graph)==0);tx.accept();}
    CHECK(main_values[1011].one_d==1&&refs==0&&suites==0);const auto unchanged_sets=sets,unchanged_reads=mesh_reads;
    {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(graph)==0);tx.accept();}
    CHECK(sets==unchanged_sets&&mesh_reads==unchanged_reads);
    auto empty=graph;empty.nodes.erase(std::remove_if(empty.nodes.begin(),empty.nodes.end(),[](auto& n){return n.type_key==kModelNode;}),empty.nodes.end());
    {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(empty)==0&&main_values[1011].one_d==0);}
    CHECK(main_values[1011].one_d==1&&refs==0&&suites==0);clear();
    for(auto failed:{755,1011}){setup(data);const auto prior=mirror_bytes();fail_write=failed;
        {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(graph)==512);}
        CHECK(main_values[1011].one_d==0&&mirror_bytes()==prior&&refs==0&&suites==0);clear();}
    setup(data);silent_write=true;{adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));
        CHECK(tx.install(graph)==PF_Err_INTERNAL_STRUCT_DAMAGED);}CHECK(main_values[1011].one_d==0&&refs==0&&suites==0);clear();
    setup(data);author_values[4].one_d=78;{adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(graph)==PF_Err_BAD_CALLBACK_PARAM);}
    CHECK(main_values[1011].one_d==0&&refs==0&&suites==0);clear();
}
void capture_resources(PF_InData& data){setup(data);auto graph=resource_graph();
    {adapter::ModelMirrorTransaction tx(&data,1,reinterpret_cast<AEGP_EffectRefH>(renderer),reinterpret_cast<AEGP_LayerH>(1));CHECK(tx.install(graph)==0);tx.accept();}
    PF_PreRenderCallbacks callbacks{};callbacks.GuidMixInPtr=mix;PF_PreRenderExtra extra{};extra.cb=&callbacks;PF_OutData output{};
    adapter::ModelRenderResources resources;adapter::MotionExposure motion;
    const auto capture=[&](const core::Graph& g){return adapter::prepare_model_render_resources(&data,&output,&extra,g,motion,64,1,never,resources);};
    const auto first_capture=capture(graph);CHECK(first_capture==0&&resources.sources.size()==1&&checkouts==2&&checkins==2&&mixes>=4);
    if(first_capture||resources.sources.empty())throw std::runtime_error(output.return_msg[0]?output.return_msg:"fixture produced no Model resources");
    CHECK(resources.sources[0].positions==resources.buffers[0].positions.data()&&resources.sources[0].position_count==8&&resources.sources[0].triangle_count==12&&
        resources.sources[0].resource_id[15]==7&&refs==0&&suites==0&&locks==0);
    wrong_type=true;CHECK(capture(graph)==PF_Err_BAD_CALLBACK_PARAM&&resources.sources.empty()&&checkouts==checkins);wrong_type=false;
    wrong_count=true;CHECK(capture(graph)==PF_Err_BAD_CALLBACK_PARAM&&resources.sources.empty()&&checkouts==checkins);wrong_count=false;
    fail_checkin=true;CHECK(capture(graph)==516&&resources.sources.empty()&&checkouts==checkins);fail_checkin=false;
    auto parked=graph;for(auto& n:parked.nodes)if(n.id==nid(2))for(auto& p:n.parameters)if(p.key==kParticleShape)p.value=std::uint32_t{0};
    auto zero=graph;for(auto& n:zero.nodes)if(n.id==nid(3))for(auto& p:n.parameters)if(p.key==kParticleCount)p.value=std::uint32_t{0};
    const auto used=checkouts;CHECK(capture(parked)==0&&resources.sources.empty()&&checkouts==used);CHECK(capture(zero)==0&&checkouts==used);
    data.current_time=-24;CHECK(capture(graph)==0&&resources.sources.empty()&&checkouts==used);data.current_time=24;
    auto recipe=take(adapter::model_mirror_recipes(graph));CHECK(recipe.size()==1&&recipe[0].node==nid(7)&&recipe[0].revision==77);
    auto wrong=graph;for(auto& n:wrong.nodes)if(n.type_key==kModelNode)for(auto& p:n.parameters)if(p.key==kModelRevision)p.value=std::uint32_t{78};
    CHECK(capture(wrong)==PF_Err_BAD_CALLBACK_PARAM&&resources.sources.empty()&&checkouts==checkins);
    const auto original=mirror_bytes();auto& damaged=handles.at(reinterpret_cast<PF_ArbitraryH>(main_values[755].arbH));
    damaged.back()^=std::byte{1};CHECK(capture(graph)==PF_Err_BAD_CALLBACK_PARAM&&resources.sources.empty()&&checkouts==checkins);
    damaged=original;
    CHECK(adapter::prepare_model_render_resources(&data,&output,&extra,graph,motion,64,1,cancelled,resources)==PF_Interrupt_CANCEL&&
        resources.sources.empty()&&checkouts==checkins&&locks==0);
    const auto shutter=core::serialize_graph(graph,core::particle_node_registry());
    const auto inactive=core::serialize_graph(parked,core::particle_node_registry());
    CHECK(shutter.has_value()&&inactive.has_value());
    const auto& shutter_graph=shutter.value();const auto& parked_graph=inactive.value();
    motion.enabled=true;motion.samples={{{-1,24},shutter_graph,{}},{{24,24},shutter_graph,{}},{{25,24},shutter_graph,{}}};
    const auto shutter_checkouts=checkouts;CHECK(capture(graph)==0&&resources.sources.size()==1&&checkouts==shutter_checkouts+2&&checkouts==checkins);
    motion.samples={{{24,24},parked_graph,{}}};const auto inactive_checkouts=checkouts;
    CHECK(capture(graph)==0&&resources.sources.empty()&&checkouts==inactive_checkouts);
    motion={};
    CHECK(capture(graph)==0);const auto graph_bytes=core::serialize_graph(graph,core::particle_node_registry());CHECK(graph_bytes.has_value());
    clear();CHECK(handles.empty());SfCoreApi api{};CHECK(StarfieldCore_GetApi(SF_CORE_ABI_VERSION,sizeof(api),&api)==1);
    SfCoreRenderRequest request{};request.struct_size=sizeof(request);request.graph_bytes=graph_bytes.value().data();request.graph_byte_count=graph_bytes.value().size();
    request.frame={64,64,64,64,{0,0,64,64},24,24,1,24,0,0,1,1,1};request.model_source_count=static_cast<std::uint32_t>(resources.sources.size());request.model_sources=resources.sources.data();
    SfCoreRenderResult result{};result.struct_size=sizeof(result);const auto rendered=api.render(&request,&result);
    if(rendered!=SF_CORE_OK)std::printf("Core render failed: %u %s\n",unsigned(rendered),result.detail);
    CHECK(rendered==SF_CORE_OK&&result.pixels);unsigned pixels{};
    if(result.pixels){const auto* rgba=static_cast<const unsigned char*>(result.pixels);for(std::uint64_t i=3;i<result.pixel_byte_count;i+=4)if(rgba[i])++pixels;}
    CHECK(pixels>0);api.release_render_result(&result);
    data.pica_basicP=nullptr;CHECK(refs==0&&suites==0);
}
}
int main(){std::setvbuf(stdout,nullptr,_IONBF,0);if(model_controls_fixture_main())return 1;PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;
    utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    basic.AcquireSuite=acquire;basic.ReleaseSuite=release;stream_suite.AEGP_GetNewEffectStreamByIndex=get_stream;stream_suite.AEGP_GetStreamType=get_type;
    stream_suite.AEGP_GetNewStreamValue=get_value;stream_suite.AEGP_DisposeStreamValue=dispose_value;stream_suite.AEGP_SetStreamValue=set_value;stream_suite.AEGP_DisposeStream=dispose_stream;
    effect_suite.AEGP_GetLayerNumEffects=num_effects;effect_suite.AEGP_GetLayerEffectByIndex=get_effect;effect_suite.AEGP_GetInstalledKeyFromLayerEffect=effect_key;
    effect_suite.AEGP_GetEffectMatchName=effect_name;effect_suite.AEGP_DisposeEffect=dispose_effect;
    try{transaction(data);capture_resources(data);std::printf("Model resource bridge: %u checks, %u failures (May2023 SDK/fake host/C ABI pixels)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& error){std::printf("FAILED: %s\n",error.what());clear();return 1;}}
