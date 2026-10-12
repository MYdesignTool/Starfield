#define STARFIELD_MODEL_BINDING_LIBRARY
#include "model_native_binding_tests.cpp"
#undef STARFIELD_MODEL_BINDING_LIBRARY
#include <numbers>
namespace starfield::adapter {
PF_Err particle_rotation_curve_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*,bool) noexcept{return PF_Err_NONE;}
}
int main(){
    namespace m=adapter::native_nodes::motion_layout;using Kind=adapter::native_nodes::Kind;
    PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;utils.host_unlock_handle=unlock;
    utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;PF_OutData out{};
    try{
        CHECK(EffectMain(PF_Cmd_PARAMS_SETUP,&data,&out,nullptr,nullptr,nullptr)==0);
        CHECK(out.num_params==219&&registered.size()==218);print_native_node_catalog("motion",registered);
        for(int i=1;i<=m::last;++i)CHECK(registered[i-1].uu.id==m::disk_id(i));
        CHECK(registered[m::mode-1].u.pd.value==1&&registered[m::mode-1].u.pd.num_choices==1);
        CHECK(registered[m::origin_type-1].u.pd.num_choices==3&&registered[m::origin_type-1].u.pd.value==1);
        CHECK(registered[m::speed-1].u.fs_d.value==100&&registered[m::speed_random-1].u.fs_d.value==0);
        CHECK(registered[m::speed-1].u.fs_d.display_flags&PF_ValueDisplayFlag_PERCENT);
        CHECK(registered[m::speed_random-1].u.fs_d.display_flags&PF_ValueDisplayFlag_PERCENT);
        CHECK(registered[m::speed-1].u.fs_d.valid_max==100000&&registered[m::speed-1].u.fs_d.slider_max==200);
        CHECK(registered[m::curve-1].ui_flags&PF_PUI_CONTROL);
        CHECK(registered[142].uu.id==400&&registered[209].uu.id==600&&registered[217].uu.id==608);
        std::array<PF_ParamDef,219> curve_params{};std::array<PF_ParamDef*,219> curve_pointers{};
        for(unsigned i=1;i<219;++i){curve_params[i]=registered[i-1];curve_pointers[i]=&curve_params[i];}
        adapter::node_sync::NativeEdit curve_edit;curve_edit.node_kind=7;curve_edit.uuid[7]=3;curve_edit.parameter_index=m::curve;
        curve_params[m::curve].u.fs_d.value=64;CHECK(adapter::node_sync::capture_age_bank(curve_pointers.data(),curve_edit));
        CHECK(curve_edit.additional_count==130);CHECK(adapter::node_sync::valid_edit(curve_edit));
        curve_params[m::curve+3].u.fs_d.value=std::numeric_limits<double>::quiet_NaN();
        CHECK(!adapter::node_sync::capture_age_bank(curve_pointers.data(),curve_edit));
        clear();
        adapter::node_sync::NativeEdit edit;edit.node_kind=7;edit.uuid[7]=3;
        for(int i=1;i<=m::last;++i){edit.parameter_index=i;edit.value_kind=i==m::origin_xy?adapter::node_sync::ValueKind::point2:adapter::node_sync::ValueKind::scalar;
            CHECK(adapter::node_sync::valid_edit(edit));}
        edit.parameter_index=143;CHECK(!adapter::node_sync::valid_edit(edit));
        core::Graph graph;graph.nodes.push_back({nid(3),kMotionNode,1,{{kMotionMode,std::uint32_t{1}},{kMotionOrigin,core::Vec3{}},
            {kMotionAxis,core::Vec3{0,0,1}},{kMotionAngularRate,0.}}});
        core::OpaqueBytes bytes;append(bytes,0x8002,2);append(bytes,9,2);append(bytes,0,4);append(bytes,1,4);
        for(auto b:nid(3).value.bytes)append(bytes,b,1);append(bytes,7,2);append(bytes,13,2);
        unsigned slot=0;for(int i=1;i<=13;++i){append(bytes,i,2);append(bytes,i==3?1:0,2);append(bytes,m::animated(i)?slot:65535,2);append(bytes,0,2);
            append(bytes,std::bit_cast<std::uint64_t>(i==1?1.:0.),8);append(bytes,0,8);append(bytes,0,8);
            if(m::animated(i))slot+=i==3?2:1;}
        put(bytes,4,bytes.size(),4);graph.optional_records.push_back(bytes);
        SPBasicSuite basic{};basic.AcquireSuite=acquire_forbidden;basic.ReleaseSuite=release_forbidden;data.pica_basicP=&basic;
        data.width=data.height=100;data.time_scale=1;data.pixel_aspect_ratio={1,1};data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
        pose_aliases={3,50,50,0,10,20,30,100,200,300,100,25,0,0};checked_out=checked_in=0;
        adapter::NativeAnimationPlan plan(graph,100,100,1);CHECK(plan.valid());core::GraphNode output;
        CHECK(plan.sample(&data,nid(3),output)==0&&checked_out==12&&checked_in==12);
        CHECK(std::get<std::uint32_t>(value(output,kMotionOriginType))==2);
        CHECK(std::get<core::Vec3>(value(output,kMotionOrigin)).x==0);
        CHECK(std::get<core::Vec3>(value(output,kMotionAxisAngles)).x==-10);
        CHECK(std::get<core::Vec3>(value(output,kMotionOffset)).y==-2);
        CHECK(std::abs(std::get<double>(value(output,kMotionAngularRate))-2*std::numbers::pi)<1e-12);
        CHECK(std::get<double>(value(output,kMotionSpeedRandom))==25);
        for(int failed=0;failed<12;++failed){fail_checkout=failed;CHECK(plan.sample(&data,nid(3),output)!=0);}fail_checkout=-1;
        auto bad=graph;put(bad.optional_records[0],2,8,2);CHECK(!adapter::NativeAnimationPlan(bad,100,100,1).valid());
        bad=graph;put(bad.optional_records[0],record_header+node_header+2*field_bytes+2,0,2);CHECK(!adapter::NativeAnimationPlan(bad,100,100,1).valid());
        pose_aliases[0]=4;CHECK(plan.sample(&data,nid(3),output)!=0);
        CHECK(handles.empty()&&locks==0);
        std::printf("Motion registration/binding9: %u checks, %u failures\n",checks,failures);return failures?1:0;
    }catch(const std::exception& e){std::printf("FAILED %s\n",e.what());clear();return 1;}
}
