#define STARFIELD_MODEL_BINDING_LIBRARY
#include "model_native_binding_tests.cpp"
#undef STARFIELD_MODEL_BINDING_LIBRARY
void compile_material_model(PF_InData& data){
    using namespace adapter;using Kind=native_nodes::Kind;
    ui_values[0].assign(native_nodes::parameter_count(Kind::material),{});
    ui_values[1].assign(native_nodes::parameter_count(Kind::model),{});
    ui_values[0][1].one_d=2;ui_values[0][2].color={1,.2,.4,.8};ui_values[0][3].one_d=75;ui_values[0][4].one_d=30;ui_values[0][5].one_d=40;ui_values[0][6].one_d=1;
    ui_values[1][layout::source].one_d=1;for(int a=0;a<3;++a)ui_values[1][layout::scale+a].one_d=100;
    for(int a=0;a<6;++a)ui_values[1][layout::author_bounds_first+a].one_d=a<3?-.5:.5;
    for(unsigned row=0;row<2;++row){auto kind=row==0?Kind::material:Kind::model;ui_values[row][native_nodes::uuid_first_index(kind)+7].one_d=row==0?3:4;
        ui_values[row][native_nodes::connection_count_index(kind)].one_d=1;
        ui_values[row][native_nodes::connection_uuid_index(kind,0,7)].one_d=row==0?4:255;
        ui_values[row][native_nodes::connection_edge_uuid_index(kind,0,7)].one_d=row+1;}
    ui_pf.AEGP_GetEffectLayer=[](PF_ProgPtr,AEGP_LayerH* out)->A_Err{*out=reinterpret_cast<AEGP_LayerH>(1);return 0;};
    ui_effect.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* out)->A_Err{*out=2;return 0;};
    ui_effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long index,AEGP_EffectRefH* out)->A_Err{*out=reinterpret_cast<AEGP_EffectRefH>(static_cast<uintptr_t>(index+1));return 0;};
    ui_effect.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH ref,AEGP_InstalledEffectKey* out)->A_Err{*out=static_cast<AEGP_InstalledEffectKey>(reinterpret_cast<uintptr_t>(ref)-1);return 0;};
    ui_effect.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err{std::strcpy(out,key==0?"org.starfieldfx.node.material":"org.starfieldfx.node.model");return 0;};
    ui_effect.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{return 0;};
    ui_stream.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH ref,A_long index,AEGP_StreamRefH* out)->A_Err{auto row=static_cast<unsigned>(reinterpret_cast<uintptr_t>(ref)-1);
        if(index<0||static_cast<size_t>(index)>=ui_values[row].size())return 516;*out=reinterpret_cast<AEGP_StreamRefH>(new UiStream{row,index});++ui_refs;return 0;};
    ui_stream.AEGP_GetStreamType=[](AEGP_StreamRefH ref,AEGP_StreamType* out)->A_Err{const auto key=*reinterpret_cast<UiStream*>(ref);*out=key.effect==0&&key.index==2?AEGP_StreamType_COLOR:AEGP_StreamType_OneD;return 0;};
    ui_stream.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out)->A_Err{auto key=*reinterpret_cast<UiStream*>(ref);out->streamH=ref;out->val=ui_values[key.effect][key.index];return 0;};
    ui_stream.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    ui_stream.AEGP_DisposeStream=[](AEGP_StreamRefH ref)->A_Err{delete reinterpret_cast<UiStream*>(ref);--ui_refs;return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=[](const char* name,int32,const void** out)->A_Err{if(!std::strcmp(name,kAEGPPFInterfaceSuite))*out=&ui_pf;
        else if(!std::strcmp(name,kAEGPEffectSuite))*out=&ui_effect;else if(!std::strcmp(name,kAEGPStreamSuite))*out=&ui_stream;else return 516;++ui_suites;return 0;};
    basic.ReleaseSuite=[](const char*,int32)->A_Err{--ui_suites;return 0;};
    std::array<PF_ParamDef,kTotalEffectParameterCount+1> main{};std::array<PF_ParamDef*,kTotalEffectParameterCount+1> pointers{};
    for(unsigned i=0;i<main.size();++i){main[i].param_type=PF_Param_FLOAT_SLIDER;pointers[i]=&main[i];}
    main[kGraphParameterId].param_type=PF_Param_ARBITRARY_DATA;main[kMaxParticlesId].u.fs_d.value=1000;
    for(int i:{kTimeSamplingHzId,kAccelerationId}){main[i].param_type=PF_Param_POPUP;main[i].u.pd.value=1;}
    for(int i:{kTimeRemapEnabledId,kPreviewEnabledId})main[i].param_type=PF_Param_CHECKBOX;main[kPreviewChanceId].u.fs_d.value=100;
    for(unsigned f=0;f<kMotionParameterIds.size();++f){auto& d=main[kMotionParameterIds[f]];d.param_type=motion_popup(f)?PF_Param_POPUP:PF_Param_FLOAT_SLIDER;
        if(motion_popup(f))d.u.pd.value=static_cast<A_long>(kMotionDefaults[f]+1);else d.u.fs_d.value=kMotionDefaults[f];}
    data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.width=data.height=100;data.time_scale=24;data.pixel_aspect_ratio={1,1};
    core::Graph graph;bool found{};CHECK(compile_native_node_graph(&data,pointers.data(),graph,found,1)==0&&found&&graph.nodes.size()==3&&graph.edges.size()==2);
    CHECK(ui_refs==0&&ui_suites==0);auto material=std::find_if(graph.nodes.begin(),graph.nodes.end(),[](const auto& n){return n.type_key==kMaterialNode;});CHECK(material!=graph.nodes.end());
    if(material!=graph.nodes.end()){CHECK(std::get<double>(value(*material,kMaterialRoughness))==.75&&std::get<core::Vec3>(value(*material,kMaterialColor)).z==.8);}
    CHECK(core::validate_graph(graph,core::particle_node_registry()).ok());CHECK(adapter::NativeAnimationPlan(graph,100,100,24).valid());
    for(const auto& e:graph.edges)CHECK(e.destination_port.value==(e.source_node==nid(3)?2u:1u));
}
int main(){PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    try{
        PF_OutData out{};CHECK(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&out,nullptr,nullptr,nullptr)==0);
        CHECK(out.num_params==83&&registered.size()==82);print_native_node_catalog("material",registered);
        for(unsigned i=0;i<6;++i){CHECK(registered[i].uu.id==1601+i);CHECK(registered[i].flags&PF_ParamFlag_SUPERVISE);}
        CHECK(registered[0].u.pd.value==1&&registered[2].u.fs_d.value==50&&registered[4].u.fs_d.value==100&&registered[5].u.bd.value==1);
        CHECK(registered[6].uu.id==400&&registered[73].uu.id==600&&registered[81].uu.id==608);clear();
        adapter::node_sync::NativeEdit edit;edit.node_kind=6;edit.uuid[7]=3;
        for(unsigned i=1;i<=6;++i){edit.parameter_index=i;edit.value_kind=i==2?adapter::node_sync::ValueKind::color:adapter::node_sync::ValueKind::scalar;CHECK(adapter::node_sync::valid_edit(edit));}
        edit.parameter_index=2;edit.value_kind=adapter::node_sync::ValueKind::scalar;CHECK(!adapter::node_sync::valid_edit(edit));
        core::Graph graph;graph.nodes.push_back({nid(3),kMaterialNode,1,{{kMaterialColorSource,std::uint32_t{0}},{kMaterialColor,core::Vec3{1,1,1}},
            {kMaterialRoughness,.5},{kMaterialMetallic,0.},{kMaterialOpacity,1.},{kMaterialLights,std::uint32_t{1}}}});
        core::OpaqueBytes bytes;append(bytes,0x8002,2);append(bytes,8,2);append(bytes,0,4);append(bytes,1,4);
        for(auto b:nid(3).value.bytes)append(bytes,b,1);append(bytes,6,2);append(bytes,6,2);
        unsigned slot=0;for(unsigned i=1;i<=6;++i){append(bytes,i,2);append(bytes,i==2?3:0,2);append(bytes,slot,2);append(bytes,0,2);
            for(unsigned n=0;n<3;++n)append(bytes,0,8);slot+=i==2?3:1;}
        put(bytes,4,bytes.size(),4);graph.optional_records.push_back(bytes);
        SPBasicSuite basic{};basic.AcquireSuite=acquire_forbidden;basic.ReleaseSuite=release_forbidden;data.pica_basicP=&basic;
        data.width=data.height=100;data.time_scale=1;data.pixel_aspect_ratio={1,1};data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
        pose_aliases={1,.2,.4,.8,75,30,40,1,0,0,0,0,0,0};checked_out=checked_in=0;
        adapter::NativeAnimationPlan plan(graph,100,100,1);CHECK(plan.valid());
        core::GraphNode output;CHECK(plan.sample(&data,nid(3),output)==0&&checked_out==8&&checked_in==8);
        CHECK(std::get<core::Vec3>(value(output,kMaterialColor)).z==.8);
        CHECK(std::get<double>(value(output,kMaterialRoughness))==.75&&std::get<double>(value(output,kMaterialMetallic))==.3&&std::get<double>(value(output,kMaterialOpacity))==.4);
        for(int failed=0;failed<8;++failed){fail_checkout=failed;CHECK(plan.sample(&data,nid(3),output)!=0);}fail_checkout=-1;
        CHECK(handles.empty()&&locks==0);
        compile_material_model(data);
        std::printf("Material registration/binding: %u checks, %u failures\n",checks,failures);return failures?1:0;
    }catch(const std::exception& e){std::printf("FAILED %s\n",e.what());clear();return 1;}
}
