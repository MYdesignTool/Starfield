#define STARFIELD_MODEL_BINDING_LIBRARY
#include "model_native_binding_tests.cpp"
#undef STARFIELD_MODEL_BINDING_LIBRARY
#include <numbers>
namespace starfield::adapter {
PF_Err motion_prefix_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept{return PF_Err_NONE;}
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*,bool) noexcept{return PF_Err_NONE;}
}
int main(){
    namespace m=adapter::native_nodes::motion_layout;using Kind=adapter::native_nodes::Kind;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;
    utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;PF_OutData out{};
    try{
        CHECK(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&out,nullptr,nullptr,nullptr)==0);
        constexpr auto parameter_count=adapter::native_nodes::parameter_count(Kind::motion);
        CHECK(out.num_params==parameter_count&&registered.size()==static_cast<std::size_t>(parameter_count-1));print_native_node_catalog("motion",registered);
        for(int i=1;i<=m::last;++i)CHECK(registered[i-1].uu.id==m::disk_id(i));
        CHECK(registered[m::mode-1].u.pd.value==1&&registered[m::mode-1].u.pd.num_choices==3);
        CHECK(registered[m::mode-1].flags&PF_ParamFlag_CANNOT_TIME_VARY);
        CHECK(registered[m::mode-1].flags&PF_ParamFlag_SUPERVISE);
        CHECK(registered[m::path_end_behavior-1].flags&PF_ParamFlag_SUPERVISE);
        std::uint32_t core_mode{};
        CHECK(m::core_mode_from_popup(1,core_mode)&&core_mode==m::core_circle_mode);
        CHECK(m::core_mode_from_popup(2,core_mode)&&core_mode==m::core_light_path_mode);
        CHECK(m::core_mode_from_popup(3,core_mode)&&core_mode==m::core_look_at_mode);
        CHECK(!m::core_mode_from_popup(0,core_mode)&&!m::core_mode_from_popup(4,core_mode));
        CHECK(registered[m::origin_type-1].u.pd.num_choices==3&&registered[m::origin_type-1].u.pd.value==1);
        CHECK(registered[m::speed-1].u.fs_d.value==100&&registered[m::speed_random-1].u.fs_d.value==0);
        CHECK(registered[m::speed-1].u.fs_d.display_flags&PF_ValueDisplayFlag_PERCENT);
        CHECK(registered[m::speed_random-1].u.fs_d.display_flags&PF_ValueDisplayFlag_PERCENT);
        CHECK(registered[m::speed-1].u.fs_d.valid_max==100000&&registered[m::speed-1].u.fs_d.slider_max==200);
        CHECK(registered[m::curve-1].ui_flags&PF_PUI_CONTROL);
        CHECK(registered[m::path_prefix_length-1].ui_flags&PF_PUI_CONTROL);
        CHECK(!(registered[m::path_prefix_length-1].ui_flags&PF_PUI_TOPIC));
        CHECK(!(registered[m::path_prefix_length-1].ui_flags&(PF_PUI_NO_ECW_UI|PF_PUI_INVISIBLE)));
        CHECK(std::strcmp(registered[m::path_prefix_length-1].name,"Starting With Length")==0);
        CHECK(registered[m::path_prefix_length-1].uu.id==1715&&
            registered[m::path_max_delay-1].uu.id==1760&&registered[m::path_end_behavior-1].uu.id==1763);
        std::array<PF_ParamDef,parameter_count> curve_params{};std::array<PF_ParamDef*,parameter_count> curve_pointers{};
        for(unsigned i=1;i<parameter_count;++i){curve_params[i]=registered[i-1];curve_pointers[i]=&curve_params[i];}
        adapter::node_sync::NativeEdit curve_edit;curve_edit.node_kind=7;curve_edit.uuid[7]=3;curve_edit.parameter_index=m::curve;
        curve_params[m::curve].u.fs_d.value=64;CHECK(adapter::node_sync::capture_age_bank(curve_pointers.data(),curve_edit));
        CHECK(curve_edit.additional_count==130);CHECK(adapter::node_sync::valid_edit(curve_edit));
        curve_params[m::curve+3].u.fs_d.value=std::numeric_limits<double>::quiet_NaN();
        CHECK(!adapter::node_sync::capture_age_bank(curve_pointers.data(),curve_edit));
        clear();
        adapter::node_sync::NativeEdit edit;edit.node_kind=7;edit.uuid[7]=3;
        for(int i=1;i<=m::last;++i){if(!adapter::native_nodes::authored_parameter(Kind::motion,i)||i==m::path_prefix_length)continue;
            edit.parameter_index=i;edit.value_kind=i==m::origin_xy?adapter::node_sync::ValueKind::point2:adapter::node_sync::ValueKind::scalar;
            CHECK(adapter::node_sync::valid_edit(edit));}
        edit.parameter_index=143;CHECK(!adapter::node_sync::valid_edit(edit));
        const core::OpaqueBytes frame_light_resource{std::byte{0x53},std::byte{0x46},std::byte{0x50},std::byte{0x4c}};
        const core::OpaqueBytes frame_look_at_resource{std::byte{0x53},std::byte{0x46},std::byte{0x4c},std::byte{0x41}};
        core::Graph graph;graph.nodes.push_back({nid(3),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,core::Vec3{}},
            {kMotionAxis,core::Vec3{0,0,1}},{kMotionAngularRate,0.},{kMotionPathLightResource,frame_light_resource},
            {kMotionLookAtTargets,frame_look_at_resource}}});
        graph.nodes.push_back({nid(255),kOutputNode,4,{{core::ParameterKey{1},std::uint32_t{1000}}}});
        graph.edges.push_back({core::EdgeId{nid(4).value},nid(3),kMotionParticlesOut,nid(255),kOutputParticles});
        core::OpaqueBytes bytes;append(bytes,0x8002,2);append(bytes,10,2);append(bytes,0,4);append(bytes,1,4);
        for(auto b:nid(3).value.bytes)append(bytes,b,1);
        constexpr std::array<int,17> fields{1,2,3,4,5,6,7,8,9,10,11,12,13,
            m::path_max_delay,m::path_delay_random,m::path_orient,m::path_end_behavior};
        append(bytes,7,2);append(bytes,static_cast<std::uint16_t>(fields.size()),2);
        unsigned slot=0;for(const auto i:fields){append(bytes,i,2);append(bytes,i==m::origin_xy?1:0,2);
            append(bytes,m::animated(i)?slot:65535,2);append(bytes,0,2);
            const double base=i==m::mode||i==m::origin_type||i==m::path_end_behavior?1.:i==m::path_max_delay?.5:0.;
            append(bytes,std::bit_cast<std::uint64_t>(base),8);append(bytes,0,8);append(bytes,0,8);
            if(m::animated(i))slot+=i==m::origin_xy?2:1;}
        put(bytes,4,bytes.size(),4);graph.optional_records.push_back(bytes);
        SPBasicSuite basic{};basic.AcquireSuite=acquire_forbidden;basic.ReleaseSuite=release_forbidden;data.pica_basicP=&basic;
        data.width=data.height=100;data.time_scale=1;data.pixel_aspect_ratio={1,1};data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
        pose_aliases={3,50,50,0,10,20,30,100,200,300,100,25,.75,20,1};checked_out=checked_in=0;
        adapter::NativeAnimationPlan plan(graph,100,100,1);CHECK(plan.valid());core::GraphNode output;
        CHECK(plan.sample(&data,nid(3),output)==0&&checked_out==15&&checked_in==15);
        CHECK(std::get<std::uint32_t>(value(output,kMotionOriginType))==2);
        CHECK(std::get<core::Vec3>(value(output,kMotionOrigin)).x==0);
        CHECK(std::get<core::Vec3>(value(output,kMotionAxisAngles)).x==-10);
        CHECK(std::get<core::Vec3>(value(output,kMotionOffset)).y==-2);
        CHECK(std::abs(std::get<double>(value(output,kMotionAngularRate))-2*std::numbers::pi)<1e-12);
        CHECK(std::get<double>(value(output,kMotionSpeedRandom))==25);
        CHECK(std::get<double>(value(output,kMotionPathDelay))==.75);
        CHECK(std::get<double>(value(output,kMotionPathDelayRandom))==20);
        CHECK(std::get<std::uint32_t>(value(output,kMotionPathOrient))==1);
        CHECK(std::get<std::uint32_t>(value(output,kMotionPathEndBehavior))==0);
        CHECK(std::get<core::OpaqueBytes>(value(output,kMotionPathLightResource))==frame_light_resource);
        CHECK(std::get<core::OpaqueBytes>(value(output,kMotionLookAtTargets))==frame_look_at_resource);
        for(int failed=0;failed<15;++failed){fail_checkout=failed;CHECK(plan.sample(&data,nid(3),output)!=0);}fail_checkout=-1;
        auto bad=graph;put(bad.optional_records[0],2,8,2);CHECK(!adapter::NativeAnimationPlan(bad,100,100,1).valid());
        bad=graph;put(bad.optional_records[0],record_header+node_header+2*field_bytes+2,0,2);CHECK(!adapter::NativeAnimationPlan(bad,100,100,1).valid());
        pose_aliases[0]=4;CHECK(plan.sample(&data,nid(3),output)!=0);
        CHECK(handles.empty()&&locks==0);
        std::printf("Motion registration/binding10: %u checks, %u failures\n",checks,failures);return failures?1:0;
    }catch(const std::exception& e){std::printf("FAILED %s\n",e.what());clear();return 1;}
}
