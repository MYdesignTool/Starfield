#define main model_controls_fixture_main
#include "model_controls_tests.cpp"
#undef main
#include "ModelMirrorParameter.hpp"
#include <string>

namespace {
PF_Err mirror_call(PF_InData& data,PF_ArbParamsExtra& extra){extra.id=adapter::kModelMirrorFirstDiskId;return adapter::model_mirror_arbitrary_callback(&data,&extra);}
adapter::ModelMirrorValue mirror_value(){adapter::ModelMirrorValue v;v.id=nid(7).value.bytes;v.revision=77;v.geometry=take(core::make_unit_cube());return v;}
void mirrors(PF_InData& data){
    CHECK(adapter::kModelMirrorFirstIndex==755&&adapter::kModelMirrorCountIndex==1011);
    CHECK(adapter::register_model_mirror_parameters(nullptr)==PF_Err_BAD_CALLBACK_PARAM);
    CHECK(adapter::register_model_mirror_parameters(&data)==0&&registered.size()==257&&handles.size()==256);
    for(int i=0;i<256;++i){const auto& def=registered[i];CHECK(def.param_type==PF_Param_ARBITRARY_DATA&&def.flags==PF_ParamFlag_NONE&&
        def.uu.id==1900+i&&def.u.arb_d.id==1900+i&&def.u.arb_d.dephault);
        auto empty=adapter::read_model_mirror_parameter(&data,def.u.arb_d.dephault,never);
        CHECK(empty.has_value()&&empty.value().id==core::ModelResourceId{}&&empty.value().revision==0&&empty.value().geometry.positions.empty());
    }
    CHECK(registered.back().param_type==PF_Param_SLIDER&&registered.back().uu.id==2200&&registered.back().u.sd.valid_max==256&&
        registered.back().u.sd.value==0&&registered.back().flags==PF_ParamFlag_CANNOT_TIME_VARY);clear();
    for(auto i:{1u,128u,256u,257u}){fail_add=i;CHECK(adapter::register_model_mirror_parameters(&data)==516&&adds==i);
        CHECK(handles.size()==(i==257?256:i-1));clear();}
    fail_allocate=true;CHECK(adapter::register_model_mirror_parameters(&data)==PF_Err_OUT_OF_MEMORY&&registered.empty()&&handles.empty());clear();
    auto v=mirror_value();PF_ArbitraryH h{};CHECK(adapter::create_model_mirror_parameter(&data,v,&h,never)==0&&h);
    auto read=adapter::read_model_mirror_parameter(&data,h,never);CHECK(read.has_value()&&read.value().id==v.id&&read.value().revision==77&&
        read.value().geometry.positions.size()==8&&read.value().geometry.triangles.size()==12);
    const auto bytes=handles.at(h);CHECK(bytes.size()>64&&!std::memcmp(bytes.data(),"SFMR",4));
    for(std::size_t at=0;at<bytes.size();++at){handles.at(h)=bytes;handles.at(h)[at]^=std::byte{1};CHECK(!adapter::read_model_mirror_parameter(&data,h,never).has_value());}
    for(std::size_t length=0;length<bytes.size();++length){handles.at(h).assign(bytes.begin(),bytes.begin()+length);CHECK(!adapter::read_model_mirror_parameter(&data,h,never).has_value());}
    handles.at(h)=bytes;CHECK(!adapter::read_model_mirror_parameter(&data,h,cancelled).has_value());
    struct UnlockedCancel:core::Cancellation {mutable unsigned calls{};
        bool is_cancelled()const noexcept override{CHECK(locks==0);return ++calls>1;}} unlocked_cancel;
    const auto stopped=adapter::read_model_mirror_parameter(&data,h,unlocked_cancel);
    CHECK(!stopped.has_value()&&stopped.error().code==core::ErrorCode::cancelled&&locks==0&&handles.at(h)==bytes);
    for(unsigned rev:{0u,2147483648u,4294967295u}){v.revision=rev;PF_ArbitraryH bad=reinterpret_cast<PF_ArbitraryH>(1);
        CHECK(adapter::create_model_mirror_parameter(&data,v,&bad,never)==PF_Err_BAD_CALLBACK_PARAM&&!bad);}
    v.revision=77;v.id={};PF_ArbitraryH bad{};CHECK(adapter::create_model_mirror_parameter(&data,v,&bad,never)==PF_Err_BAD_CALLBACK_PARAM&&!bad);
    v=mirror_value();CHECK(adapter::create_model_mirror_parameter(&data,v,&bad,cancelled)==PF_Interrupt_CANCEL&&!bad);
    fail_allocate=true;CHECK(adapter::create_model_mirror_parameter(&data,v,&bad,never)==PF_Err_OUT_OF_MEMORY&&!bad);
    PF_ArbParamsExtra extra{};extra.which_function=PF_Arbitrary_COPY_FUNC;PF_ArbitraryH copy{};
    extra.u.copy_func_params.src_arbH=h;extra.u.copy_func_params.dst_arbPH=&copy;CHECK(mirror_call(data,extra)==0&&copy&&copy!=h&&handles.at(copy)==bytes);
    handles.at(copy)[16]^=std::byte{1};CHECK(handles.at(h)==bytes&&!adapter::read_model_mirror_parameter(&data,copy,never).has_value());handles.at(copy)=bytes;
    extra={};extra.which_function=PF_Arbitrary_COMPARE_FUNC;PF_ArbCompareResult compare{};
    extra.u.compare_func_params.a_arbH=h;extra.u.compare_func_params.b_arbH=copy;extra.u.compare_func_params.compareP=&compare;
    CHECK(mirror_call(data,extra)==0&&compare==PF_ArbCompare_EQUAL);
    A_u_long length{};extra={};extra.which_function=PF_Arbitrary_FLAT_SIZE_FUNC;extra.u.flat_size_func_params.arbH=h;extra.u.flat_size_func_params.flat_data_sizePLu=&length;
    CHECK(mirror_call(data,extra)==0&&length==bytes.size());std::vector<std::byte> flat(length);
    extra={};extra.which_function=PF_Arbitrary_FLATTEN_FUNC;extra.u.flatten_func_params.arbH=h;extra.u.flatten_func_params.flat_dataPV=flat.data();extra.u.flatten_func_params.buf_sizeLu=length;
    CHECK(mirror_call(data,extra)==0&&flat==bytes);extra.u.flatten_func_params.buf_sizeLu=length-1;CHECK(mirror_call(data,extra)==PF_Err_BAD_CALLBACK_PARAM);
    PF_ArbitraryH restored{};extra={};extra.which_function=PF_Arbitrary_UNFLATTEN_FUNC;extra.u.unflatten_func_params.flat_dataPV=flat.data();
    extra.u.unflatten_func_params.buf_sizeLu=length;extra.u.unflatten_func_params.arbPH=&restored;CHECK(mirror_call(data,extra)==0&&handles.at(restored)==bytes);
    flat[16]^=std::byte{1};PF_ArbitraryH invalid{};extra.u.unflatten_func_params.arbPH=&invalid;CHECK(mirror_call(data,extra)==PF_Err_BAD_CALLBACK_PARAM&&!invalid);
    PF_ArbitraryH empty{};extra={};extra.which_function=PF_Arbitrary_NEW_FUNC;extra.u.new_func_params.arbPH=&empty;CHECK(mirror_call(data,extra)==0&&handles.at(empty).size()==64);
    for(double t:{0.0,0.5,1.0}){PF_ArbitraryH interpolated{};extra={};extra.which_function=PF_Arbitrary_INTERP_FUNC;
        extra.u.interp_func_params.left_arbH=h;extra.u.interp_func_params.right_arbH=empty;extra.u.interp_func_params.tF=t;
        extra.u.interp_func_params.interpPH=&interpolated;CHECK(mirror_call(data,extra)==0&&handles.at(interpolated)==handles.at(t<1?h:empty));dispose(interpolated);}
    extra={};extra.which_function=PF_Arbitrary_PRINT_SIZE_FUNC;extra.u.print_size_func_params.arbH=h;extra.u.print_size_func_params.print_sizePLu=&length;
    CHECK(mirror_call(data,extra)==0);std::string text(length,'\0');extra={};extra.which_function=PF_Arbitrary_PRINT_FUNC;
    extra.u.print_func_params.arbH=h;extra.u.print_func_params.print_bufferPC=text.data();extra.u.print_func_params.print_sizeLu=length;CHECK(mirror_call(data,extra)==0&&text.starts_with("SFMIRROR1:"));
    PF_ArbitraryH scanned{};extra={};extra.which_function=PF_Arbitrary_SCAN_FUNC;extra.u.scan_func_params.bufPC=text.data();extra.u.scan_func_params.bytes_to_scanLu=length;
    extra.u.scan_func_params.arbPH=&scanned;CHECK(mirror_call(data,extra)==0&&handles.at(scanned)==bytes);
    extra.id=adapter::kModelMirrorFirstDiskId-1;CHECK(adapter::model_mirror_arbitrary_callback(&data,&extra)==PF_Err_BAD_CALLBACK_PARAM);
    extra.id=adapter::kModelMirrorFirstDiskId+256;CHECK(adapter::model_mirror_arbitrary_callback(&data,&extra)==PF_Err_BAD_CALLBACK_PARAM);
    extra={};extra.which_function=PF_Arbitrary_DISPOSE_FUNC;extra.u.dispose_func_params.arbH=scanned;CHECK(mirror_call(data,extra)==0&&!handles.contains(scanned));
    CHECK(locks==0);clear();
}
}
int main(){if(model_controls_fixture_main())return 1;PF_UtilCallbacks utils{};utils.host_new_handle=allocate;utils.host_lock_handle=lock;
    utils.host_unlock_handle=unlock;utils.host_dispose_handle=dispose;utils.host_get_handle_size=size_of;
    PF_InData data{};data.utils=&utils;data.inter.add_param=add;
    try{mirrors(data);CHECK(handles.empty()&&locks==0);std::printf("Model mirrors: %u checks, %u failures (May2023 SDK/fake host)\n",checks,failures);return failures?1:0;}
    catch(const std::exception& error){std::printf("FAILED: %s\n",error.what());clear();return 1;}}
