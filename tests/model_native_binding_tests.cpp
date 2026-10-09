// Reuse the owned Model handle/control fixture without changing its source.
#define main model_controls_fixture_main
#include "model_controls_tests.cpp"
#undef main
#include "NativeNodeGraph.hpp"
#include "NativeTemporalCache.hpp"
#include "NodeEffects.hpp"
#include "NodeEffectFlags.h"
#include "PluginVersion.h"
#include "NodeGraphSync.hpp"
#include "Parameters.hpp"
#include "GpuRender.hpp"
#include "ParticleGradientUI.hpp"
#include "TransformNullUI.hpp"
#include "MotionBlur.hpp"
#include "ModelImportUI.hpp"

PF_Err register_node_graph_sync(PF_InData*) noexcept {return PF_Err_NONE;}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef*[],const PF_UserChangedParamExtra*,bool) noexcept {return PF_Err_BAD_CALLBACK_PARAM;}
namespace starfield::adapter {
PF_Err import_model_obj(PF_InData*,PF_OutData*,PF_ParamDef*[]) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err gpu_device_setup(PF_InData*,PF_OutData*,PF_GPUDeviceSetupExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err gpu_device_setdown(PF_InData*,PF_GPUDeviceSetdownExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
bool gpu_device_matches(const void*,PF_GPU_Framework,A_u_long) noexcept{return false;}
PF_Err copy_gpu_pixels(PF_InData*,const void*,PF_GPU_Framework,A_u_long,PF_EffectWorld*,PF_EffectWorld*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_gradient_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
PF_Err particle_gradient_param_ui(PF_InData*,PF_ParamDef*[]) noexcept{return PF_Err_NONE;}
void clear_particle_gradient_ui() noexcept{}
PF_Err transform_null_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_BAD_CALLBACK_PARAM;}
}
namespace {
struct UiStream{unsigned effect;A_long index;};
std::array<std::vector<AEGP_StreamVal2>,3> ui_values;
AEGP_PFInterfaceSuite1 ui_pf{};AEGP_EffectSuite4 ui_effect{};AEGP_StreamSuite6 ui_stream{};
unsigned ui_refs{},ui_suites{},ui_mesh_reads{};bool wrong_mesh_type{},fail_mesh_read{};
unsigned ui_last_effect{};A_long ui_last_index{};
const char* ui_names[]={"org.starfieldfx.node.model","org.starfieldfx.node.emitter","org.starfieldfx.node.particle"};
unsigned checked_out{},checked_in{},aegp_queries{};int fail_checkout=-1;bool fail_checkin{},bad_numeric{};
std::array<double,14> pose_aliases{};
constexpr unsigned record_header=12,node_header=20,field_bytes=32;
void append(core::OpaqueBytes& bytes,std::uint64_t v,unsigned count){for(unsigned b=0;b<count;++b)bytes.push_back(static_cast<std::byte>((v>>(8*b))&255));}
void put(core::OpaqueBytes& bytes,unsigned offset,std::uint64_t v,unsigned count){for(unsigned b=0;b<count;++b)bytes[offset+b]=static_cast<std::byte>((v>>(8*b))&255);}
core::OpaqueBytes bindings(unsigned source=1,unsigned revision=0){
    core::OpaqueBytes bytes;append(bytes,0x8002,2);append(bytes,7,2);append(bytes,0,4);append(bytes,1,4);
    for(auto b:nid(4).value.bytes)append(bytes,b,1);append(bytes,5,2);append(bytes,22,2);
    for(int index=1;index<=layout::bounds_last;++index){if(!layout::binding_field(index))continue;
        append(bytes,index,2);append(bytes,0,2);append(bytes,layout::animated(index)?index-layout::origin:65535,2);append(bytes,0,2);
        double value=0;if(index==layout::source)value=source;else if(index==layout::revision)value=revision;
        else if(index>=layout::scale && index<=layout::scale+2)value=100;
        else if(index>=layout::bounds_first)value=index<layout::bounds_first+3?-.5:.5;
        append(bytes,std::bit_cast<std::uint64_t>(value),8);append(bytes,0,8);append(bytes,0,8);
    }
    put(bytes,4,bytes.size(),4);return bytes;
}
unsigned field_offset(int index){unsigned field=0;for(int i=1;i<index;++i)if(layout::binding_field(i))++field;return record_header+node_header+field_bytes*field;}
core::Graph bound_graph(unsigned source=1,unsigned revision=0){
    core::Settings settings;auto graph=take(core::make_emitter_particle_output_graph(settings,nid(1),nid(2),nid(3),{nid(1).value},{nid(2).value}));
    for(auto& node:graph.nodes)if(node.id==nid(2))for(auto& p:node.parameters)if(p.key==kParticleShape)p.value=std::uint32_t{4};
    graph.nodes.push_back({nid(4),kModelNode,1,{}});graph.edges.push_back({{nid(4).value},nid(4),kModelGeometryOut,nid(2),kParticleModelsIn});
    graph.optional_records.push_back(bindings(source,revision));return graph;
}
PF_Err checkout(PF_ProgPtr,PF_ParamIndex index,A_long time,A_long,A_u_long scale,PF_ParamDef* out){
    CHECK(index>=adapter::kNativeBindingFirstIndex&&index<adapter::kNativeBindingFirstIndex+14&&scale);++checked_out;
    const auto slot=index-adapter::kNativeBindingFirstIndex;if(slot==fail_checkout)return 516;
    out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=bad_numeric?std::numeric_limits<double>::quiet_NaN():pose_aliases.at(slot);
    if(slot==0)out->u.fs_d.value+=double(time)/scale;return PF_Err_NONE;
}
PF_Err checkin(PF_ProgPtr,PF_ParamDef*){++checked_in;return fail_checkin?516:PF_Err_NONE;}
A_Err acquire_forbidden(const char*,int32,const void**){++aegp_queries;return 516;}
A_Err release_forbidden(const char*,int32){++aegp_queries;return 516;}
void registration_module(PF_InData& data){
    PF_OutData output{};
    CHECK(EffectMain(PF_Cmd_GLOBAL_SETUP,&data,&output,nullptr,nullptr,nullptr)==PF_Err_NONE);
    CHECK(output.my_version==STARFIELD_VERSION_PACKED&&output.out_flags==STARFIELD_NODE_OUT_FLAGS&&output.out_flags2==STARFIELD_NODE_OUT_FLAGS2);
    CHECK(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&output,nullptr,nullptr,nullptr)==PF_Err_NONE&&output.num_params==95&&registered.size()==94);
    CHECK(registered[18].uu.id==adapter::native_nodes::disk_ids::kLayoutXId&&!std::strcmp(registered[18].name,"Node Layout X"));
    CHECK(registered[85].uu.id==adapter::native_nodes::uuid_id(0)&&!std::strcmp(registered[85].name,"Node UUID 0"));
    CHECK(registered[93].uu.id==adapter::native_nodes::disk_ids::kSyncGuardId);
    for(unsigned index=18;index<registered.size();++index)CHECK(registered[index].flags&PF_ParamFlag_CANNOT_TIME_VARY);
    PF_ArbitraryH created=nullptr;PF_ArbParamsExtra call{};call.id=1503;call.which_function=PF_Arbitrary_NEW_FUNC;call.u.new_func_params.arbPH=&created;
    CHECK(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&data,&output,nullptr,nullptr,&call)==PF_Err_NONE&&created);
    auto restored=take(adapter::read_model_geometry_parameter(&data,created,never));CHECK(restored.positions.size()==8&&restored.triangles.size()==12);
    call.id=1504;created=nullptr;CHECK(EffectMain(PF_Cmd_ARBITRARY_CALLBACK,&data,&output,nullptr,nullptr,&call)==PF_Err_BAD_CALLBACK_PARAM&&!created);
    CHECK(adapter::native_nodes::parameter_count(adapter::native_nodes::Kind::model)==95);
    adapter::node_sync::NativeEdit edit;edit.node_kind=5;edit.uuid[7]=4;
    for(int index=1;index<=24;++index){edit.parameter_index=index;
        CHECK(adapter::node_sync::valid_edit(edit)==adapter::native_nodes::authored_parameter(adapter::native_nodes::Kind::model,index));}
    edit.parameter_index=5;edit.value_kind=adapter::node_sync::ValueKind::point3;CHECK(!adapter::node_sync::valid_edit(edit));
    edit.value_kind=adapter::node_sync::ValueKind::scalar;edit.additional_count=1;CHECK(!adapter::node_sync::valid_edit(edit));
    clear();
}
void animation(PF_InData& data){
    SPBasicSuite basic{};basic.AcquireSuite=acquire_forbidden;basic.ReleaseSuite=release_forbidden;
    data.pica_basicP=&basic;data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
    data.width=1920;data.height=1080;data.current_time=48;data.time_scale=24;data.pixel_aspect_ratio={1,1};
    pose_aliases={0,3,4, 0,0,90, 100,200,50, 0,0,0,0,0};
    for(unsigned source:{1u,2u})for(unsigned revision:{0u,77u}){
        auto graph=bound_graph(source,revision);adapter::NativeAnimationPlan plan(graph,1920,1080,1);CHECK(plan.valid());
        core::GraphNode sampled;A_long failed=-1;const auto before=checked_out;
        CHECK(plan.sample(&data,nid(4),sampled,&failed)==PF_Err_NONE&&checked_out-before==14&&aegp_queries==0);
        CHECK(sampled.type_key==kModelNode&&sampled.id==nid(4)&&sampled.parameters.size()==12&&
            std::get<core::Vec3>(value(sampled,kModelOrigin)).x==2&&std::get<core::Vec3>(value(sampled,kModelOrigin)).y==3);
        auto resource=std::get<core::OpaqueBytes>(value(sampled,kModelResource));
        CHECK((resource==core::OpaqueBytes(16))==(source==1||revision==0));
        CHECK(std::get<std::uint32_t>(value(sampled,kModelRevision))==((source==2&&revision)?revision:0));
        const auto before_all=checked_out;CHECK(adapter::sample_native_node_animation(&data,graph,1920,1080)==PF_Err_NONE&&checked_out-before_all==14);
        auto evaluated=take(core::evaluate_particle_graph(graph,{1,1},never));const auto& matrix=evaluated.model_styles.at(0).instances.at(0).model_to_particle;
        CHECK(std::abs(matrix[12]-2)<1e-12&&std::abs(matrix[13]-3)<1e-12&&std::abs(matrix[14]-4)<1e-12);
    }
    auto graph=bound_graph();adapter::NativeAnimationPlan plan(graph,1920,1080,1);core::GraphNode sampled;
    for(int slot=0;slot<14;++slot){fail_checkout=slot;const auto before=checked_in;A_long failed=-1;
        CHECK(plan.sample(&data,nid(4),sampled,&failed)==516&&failed==adapter::kNativeBindingFirstIndex+slot&&checked_in-before==static_cast<unsigned>(slot));}
    fail_checkout=-1;fail_checkin=true;CHECK(plan.sample(&data,nid(4),sampled)==516);fail_checkin=false;
    bad_numeric=true;CHECK(plan.sample(&data,nid(4),sampled)==PF_Err_BAD_CALLBACK_PARAM);bad_numeric=false;
    for(double bad:{100001.,std::numeric_limits<double>::infinity()}){pose_aliases[6]=bad;CHECK(plan.sample(&data,nid(4),sampled)==PF_Err_BAD_CALLBACK_PARAM);}pose_aliases[6]=100;
    pose_aliases[9]=2;CHECK(plan.sample(&data,nid(4),sampled)==PF_Err_BAD_CALLBACK_PARAM);pose_aliases[9]=0;
    CHECK(aegp_queries==0);
}
void invalid_records(PF_InData& data){
    const auto original=bound_graph();
    for(unsigned length=2;length<original.optional_records[0].size();++length){auto bad=original;bad.optional_records[0].resize(length);
        adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
    for(unsigned version:{0u,1u,2u,3u,4u,5u,6u,8u}){auto bad=original;put(bad.optional_records[0],2,version,2);adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
    for(unsigned kind:{2u,6u,65535u}){auto bad=original;put(bad.optional_records[0],28,kind,2);adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
    for(int index=1;index<=24;++index){if(!layout::binding_field(index))continue;const auto offset=field_offset(index);
        for(unsigned type:{1u,2u,3u}){auto bad=original;put(bad.optional_records[0],offset+2,type,2);adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
        for(unsigned component:{1u,2u}){auto bad=original;put(bad.optional_records[0],offset+8+8*component,std::bit_cast<std::uint64_t>(1.),8);adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
        auto missing=original;missing.optional_records[0].erase(missing.optional_records[0].begin()+offset,missing.optional_records[0].begin()+offset+32);
        put(missing.optional_records[0],30,21,2);put(missing.optional_records[0],4,missing.optional_records[0].size(),4);adapter::NativeAnimationPlan no_field(missing,1920,1080,1);CHECK(!no_field.valid());
        auto slot=original;put(slot.optional_records[0],offset+4,layout::animated(index)?65535u:0u,2);adapter::NativeAnimationPlan wrong_slot(slot,1920,1080,1);CHECK(!wrong_slot.valid());
    }
    for(unsigned index:{2u,3u}){auto bad=original;put(bad.optional_records[0],field_offset(4),index,2);adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(!plan.valid());}
    for(double source:{0.,3.,1.5}){auto bad=original;put(bad.optional_records[0],field_offset(1)+8,std::bit_cast<std::uint64_t>(source),8);
        core::GraphNode sampled;adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(plan.valid()&&plan.sample(&data,nid(4),sampled)==PF_Err_BAD_CALLBACK_PARAM);}
    for(double revision:{-1.,2147483648.,1.5}){auto bad=original;put(bad.optional_records[0],field_offset(4)+8,std::bit_cast<std::uint64_t>(revision),8);
        core::GraphNode sampled;adapter::NativeAnimationPlan plan(bad,1920,1080,1);CHECK(plan.valid()&&plan.sample(&data,nid(4),sampled)==PF_Err_BAD_CALLBACK_PARAM);}
    auto duplicate=original;put(duplicate.optional_records[0],field_offset(6)+4,0,2);adapter::NativeAnimationPlan duplicate_slot(duplicate,1920,1080,1);CHECK(!duplicate_slot.valid());
    for(unsigned version=1;version<=7;++version){auto empty=original;empty.optional_records[0].resize(12);put(empty.optional_records[0],2,version,2);put(empty.optional_records[0],4,12,4);put(empty.optional_records[0],8,0,4);
        adapter::NativeAnimationPlan plan(empty,1920,1080,1);CHECK(plan.valid());}
}
void ui_capture(PF_InData& data){
    using namespace adapter;using records=adapter::native_nodes::Kind;
    CHECK(register_model_author_controls(&data)==PF_Err_NONE);
    const auto original=registered[layout::mesh-1].u.arb_d.dephault;
    ui_values[0].assign(native_nodes::parameter_count(records::model),{});
    ui_values[1].assign(native_nodes::parameter_count(records::emitter),{});
    ui_values[2].assign(native_nodes::parameter_count(records::particle),{});
    const auto identity=[&](unsigned row,A_long first,unsigned id){ui_values[row][first+7].one_d=id;};
    const auto connect=[&](unsigned row,records kind,unsigned target,unsigned edge){
        ui_values[row][native_nodes::connection_count_index(kind)].one_d=1;
        identity(row,native_nodes::connection_uuid_index(kind,0,0),target);
        identity(row,native_nodes::connection_edge_uuid_index(kind,0,0),edge);
    };
    identity(0,native_nodes::uuid_first_index(records::model),4);
    identity(1,native_nodes::uuid_first_index(records::emitter),1);
    identity(2,native_nodes::uuid_first_index(records::particle),2);
    ui_values[0][1].one_d=1;ui_values[0][3].arbH=reinterpret_cast<AEGP_ArbBlockVal>(original);
    for(int axis=0;axis<3;++axis)ui_values[0][layout::scale+axis].one_d=100;
    ui_values[1][1].one_d=1;ui_values[1][2].one_d=1;ui_values[1][3].one_d=10;ui_values[1][15].one_d=1;
    ui_values[1][19].one_d=60;for(int index:{8,9,10,20,22,28,29})ui_values[1][index].one_d=100;
    namespace p=native_nodes::particle_layout;
    ui_values[2][p::shape].one_d=1;ui_values[2][p::life].one_d=2;
    ui_values[2][p::size].one_d=ui_values[2][p::size_y].one_d=10;
    ui_values[2][p::opacity].one_d=100;ui_values[2][p::orient].one_d=1;ui_values[2][p::up_axis].one_d=3;ui_values[2][p::random_limit].one_d=1;
    ui_values[2][p::color_mode].one_d=1;ui_values[2][p::gradient].one_d=2;
    ui_values[2][p::gradient_first+2].one_d=100;
    for(int index:{p::color,p::gradient_first+1,p::gradient_first+3})ui_values[2][index].color={1,1,1,1};
    ui_values[2][p::size_over_life].one_d=ui_values[2][p::opacity_over_life].one_d=100;
    for(int index:{p::transfer,p::texture_time,p::texture_color,p::texture_ratio})ui_values[2][index].one_d=1;
    connect(1,records::emitter,2,11);connect(2,records::particle,255,12);connect(0,records::model,2,13);
    ui_pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* out)->A_Err{*out=reinterpret_cast<AEGP_LayerH>(1);return 0;};
    ui_effect.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* out)->A_Err{*out=3;return 0;};
    ui_effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long index,AEGP_EffectRefH* out)->A_Err{*out=reinterpret_cast<AEGP_EffectRefH>(static_cast<std::uintptr_t>(index+1));return 0;};
    ui_effect.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH ref,AEGP_InstalledEffectKey* out)->A_Err{*out=static_cast<AEGP_InstalledEffectKey>(reinterpret_cast<std::uintptr_t>(ref)-1);return 0;};
    ui_effect.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err{std::strcpy(out,ui_names[key]);return 0;};
    ui_effect.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{return 0;};
    ui_stream.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH effect,A_long index,AEGP_StreamRefH* out)->A_Err{
        const auto row=static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(effect)-1);
        ui_last_effect=row;ui_last_index=index;
        if(index<0 || static_cast<std::size_t>(index)>=ui_values.at(row).size())return 516;
        *out=reinterpret_cast<AEGP_StreamRefH>(new UiStream{row,index});++ui_refs;return 0;};
    ui_stream.AEGP_DisposeStream=[](AEGP_StreamRefH ref)->A_Err{CHECK(ui_refs>0);--ui_refs;delete reinterpret_cast<UiStream*>(ref);return 0;};
    ui_stream.AEGP_GetStreamType=[](AEGP_StreamRefH ref,AEGP_StreamType* out)->A_Err{
        const auto key=*reinterpret_cast<UiStream*>(ref);
        *out=key.effect==0&&key.index==layout::mesh&&!wrong_mesh_type?AEGP_StreamType_ARB:
            key.effect==2&&(key.index==native_nodes::particle_layout::texture_front||key.index==native_nodes::particle_layout::texture_back)?
            AEGP_StreamType_LAYER_ID:AEGP_StreamType_OneD;return 0;};
    ui_stream.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out)->A_Err{
        const auto key=*reinterpret_cast<UiStream*>(ref);out->streamH=ref;out->val=ui_values[key.effect][key.index];
        if(key.effect==1&&key.index==4)out->val.two_d={960,540};
        if(key.effect==0&&key.index==layout::mesh){++ui_mesh_reads;if(fail_mesh_read)return 516;
            const auto input=reinterpret_cast<PF_Handle>(out->val.arbH);auto copy=allocate(handles.at(input).size());handles.at(copy)=handles.at(input);out->val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(copy);}
        return 0;};
    ui_stream.AEGP_DisposeStreamValue=[](AEGP_StreamValue2* value)->A_Err{const auto key=*reinterpret_cast<UiStream*>(value->streamH);
        if(key.effect==0&&key.index==layout::mesh)dispose(reinterpret_cast<PF_Handle>(value->val.arbH));return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=[](const char* name,int32,const void** out)->A_Err{
        if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&ui_pf;else if(!std::strcmp(name,kAEGPEffectSuite))*out=&ui_effect;
        else if(!std::strcmp(name,kAEGPStreamSuite))*out=&ui_stream;else return 516;++ui_suites;return 0;};
    basic.ReleaseSuite=[](const char*,int32)->A_Err{CHECK(ui_suites>0);--ui_suites;return 0;};
    std::array<PF_ParamDef,kTotalEffectParameterCount+1> main{};std::array<PF_ParamDef*,kTotalEffectParameterCount+1> pointers{};
    for(unsigned index=0;index<main.size();++index){main[index].param_type=PF_Param_FLOAT_SLIDER;pointers[index]=&main[index];}
    main[kGraphParameterId].param_type=PF_Param_ARBITRARY_DATA;main[kMaxParticlesId].u.fs_d.value=1000;
    for(int index:{kTimeSamplingHzId,kAccelerationId}){main[index].param_type=PF_Param_POPUP;main[index].u.pd.value=1;}
    for(int index:{kTimeRemapEnabledId,kPreviewEnabledId})main[index].param_type=PF_Param_CHECKBOX;
    main[kPreviewChanceId].u.fs_d.value=100;
    for(unsigned field=0;field<kMotionParameterIds.size();++field){auto& d=main[kMotionParameterIds[field]];
        d.param_type=motion_popup(field)?PF_Param_POPUP:PF_Param_FLOAT_SLIDER;
        if(motion_popup(field))d.u.pd.value=static_cast<A_long>(kMotionDefaults[field]+1);else d.u.fs_d.value=kMotionDefaults[field];}
    data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.width=1920;data.height=1080;data.time_scale=24;data.pixel_aspect_ratio={1,1};
    core::Graph graph;bool found=false;
    const auto compiled=compile_native_node_graph(&data,pointers.data(),graph,found,1);
    if(compiled)std::printf("UI compile error %d, last effect %u stream %ld, nodes %zu edges %zu\n",compiled,ui_last_effect,long(ui_last_index),graph.nodes.size(),graph.edges.size());
    CHECK(compiled==PF_Err_NONE&&found&&graph.nodes.size()==4&&ui_mesh_reads==0);
    if(compiled==PF_Err_NONE){CHECK(std::any_of(graph.edges.begin(),graph.edges.end(),[](const auto& e){return e.source_node==nid(4)&&e.destination_node==nid(2)&&e.destination_port==kParticleModelsIn;}));
        CHECK(graph.optional_records.size()==2&&graph.optional_records[1][2]==std::byte{7});}
    PF_ArbitraryH imported=nullptr;CHECK(prepare_model_obj_parameter(&data,"v 10 20 30\nv 14 20 30\nv 14 28 30\nf 1 2 3\n",&imported,never)==PF_Err_NONE);
    ui_values[0][layout::source].one_d=2;ui_values[0][layout::revision].one_d=77;ui_values[0][layout::mesh].arbH=reinterpret_cast<AEGP_ArbBlockVal>(imported);
    CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_NONE&&ui_mesh_reads==1&&handles.contains(imported));
    const auto model=std::find_if(graph.nodes.begin(),graph.nodes.end(),[](const auto& n){return n.id==nid(4);});
    CHECK(model!=graph.nodes.end()&&std::get<core::OpaqueBytes>(value(*model,kModelResource))[15]==std::byte{4}&&
        std::get<std::uint32_t>(value(*model,kModelRevision))==77);
    wrong_mesh_type=true;CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_BAD_CALLBACK_PARAM&&ui_refs==0&&ui_suites==0);wrong_mesh_type=false;
    fail_mesh_read=true;CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_BAD_CALLBACK_PARAM&&ui_refs==0&&ui_suites==0);fail_mesh_read=false;
    handles.at(imported).back()^=std::byte{1};CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_BAD_CALLBACK_PARAM&&ui_refs==0&&ui_suites==0);
    ui_values[0][layout::source].one_d=1;const auto reads=ui_mesh_reads;
    CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_NONE&&ui_mesh_reads==reads);
    connect(0,records::model,255,13);CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==PF_Err_BAD_CALLBACK_PARAM);
    CHECK(ui_refs==0&&ui_suites==0&&locks==0);data.pica_basicP=nullptr;clear();
}
}
int main(){if(model_controls_fixture_main())return 1;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    try{registration_module(data);animation(data);invalid_records(data);ui_capture(data);CHECK(handles.empty()&&locks==0);
        std::printf("Model native binding/module: %u checks, %u failures (May2023 SDK/fake host)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& error){std::printf("FAILED: %s\n",error.what());clear();return 1;}}
