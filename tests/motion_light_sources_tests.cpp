#include "MotionLightSources.hpp"
#include "MotionPointCapture.hpp"
#include "SPBasic.h"
#include "starfield/core/Render.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>

namespace {std::atomic<int> allocation_countdown{-1};}
void* operator new(std::size_t size){
    const auto left=allocation_countdown.load();
    if(left>=0&&allocation_countdown.fetch_sub(1)==0){allocation_countdown=-1;throw std::bad_alloc{};}
    if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};
}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
using namespace starfield::adapter;
using namespace starfield::core;
namespace {
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};AEGP_MemorySuite1 memory{};SPBasicSuite basic{};PF_InData data{};
std::array<AEGP_ObjectType,258> types{};std::array<AEGP_LayerIDVal,258> ids{};
std::array<std::array<A_UTF16Char,4098>,258> names{};std::array<std::size_t,258> lengths{};
std::array<bool,258> handles{},locks{};std::array<unsigned,258> matrix_calls{};
unsigned calls{},fail_at{},acquired{},released{},active_suites{},active_handles{},active_locks{},release_calls{},release_fail{},cleanup_fail{},null_suite{};
A_long layer_count=5;int duplicate_index=-1;bool no_owner{},no_comp{},no_layer{},wrong_comp{},zero_time{},null_name{},null_pointer{},name_fail_with_handle{};
AEGP_MemSize override_bytes{};A_long time_offset{};unsigned time_multiplier=1;A_Time expected_time{};
AEGP_LayerH layer_handle(unsigned i){return reinterpret_cast<AEGP_LayerH>(static_cast<std::uintptr_t>(i+1));}
AEGP_CompH comp_handle(unsigned i=42){return reinterpret_cast<AEGP_CompH>(static_cast<std::uintptr_t>(i));}
AEGP_MemHandle name_handle(unsigned i){return reinterpret_cast<AEGP_MemHandle>(static_cast<std::uintptr_t>(i+1));}
unsigned index_of(const void* p){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(p)-1);}
A_Err step(){return ++calls==fail_at?77:0;}
void name(unsigned i,std::u16string_view text){names[i].fill(0);lengths[i]=text.size();for(std::size_t j=0;j<text.size();++j)names[i][j]=static_cast<A_UTF16Char>(text[j]);}
void reset(){
    calls=fail_at=acquired=released=active_suites=active_handles=active_locks=release_calls=release_fail=cleanup_fail=null_suite=0;
    layer_count=5;duplicate_index=-1;no_owner=no_comp=no_layer=wrong_comp=zero_time=null_name=null_pointer=name_fail_with_handle=false;
    override_bytes=0;time_offset=0;time_multiplier=1;allocation_countdown=-1;
    handles.fill(false);locks.fill(false);matrix_calls.fill(0);types.fill(AEGP_ObjectType_AV);
    for(unsigned i=0;i<ids.size();++i){ids[i]=1000+i;name(i,u"A long source name with copied ownership");}
    types[1]=types[3]=AEGP_ObjectType_LIGHT;types[2]=AEGP_ObjectType_CAMERA;
    name(1,u"Orbit_灯光_\U0001f680");name(3,u"LookAt_灯光");
    basic={};basic.AcquireSuite=[](const char* key,int32 version,const void** out)->A_Err {
        if(const auto e=step())return e;++acquired;++active_suites;
        if(!std::strcmp(key,kAEGPPFInterfaceSuite)){CHECK(version==kAEGPPFInterfaceSuiteVersion1);*out=null_suite==1?nullptr:&pf;}
        else if(!std::strcmp(key,kAEGPLayerSuite)){CHECK(version==kAEGPLayerSuiteVersion9);*out=null_suite==2?nullptr:&layers;}
        else{CHECK(!std::strcmp(key,kAEGPMemorySuite));CHECK(version==kAEGPMemorySuiteVersion1);*out=null_suite==3?nullptr:&memory;}
        return 0;
    };
    basic.ReleaseSuite=[](const char*,int32)->A_Err {CHECK(active_suites>0);--active_suites;++released;return ++release_calls==release_fail?88:0;};
    pf={};pf.AEGP_GetEffectLayer=[](PF_ProgPtr effect,AEGP_LayerH* out)->A_Err {CHECK(effect==data.effect_ref);if(const auto e=step())return e;*out=no_owner?nullptr:layer_handle(0);return 0;};
    pf.AEGP_ConvertEffectToCompTime=[](PF_ProgPtr effect,A_long value,A_u_long scale,A_Time* out)->A_Err {
        CHECK(effect==data.effect_ref);CHECK(value==data.current_time&&scale==data.time_scale);if(const auto e=step())return e;
        expected_time={static_cast<A_long>(static_cast<std::int64_t>(value)*time_multiplier+static_cast<std::int64_t>(time_offset)*scale),zero_time?0:scale};*out=expected_time;return 0;
    };
    layers={};layers.AEGP_GetLayerParentComp=[](AEGP_LayerH layer,AEGP_CompH* out)->A_Err {if(const auto e=step())return e;*out=no_comp?nullptr:comp_handle(wrong_comp&&index_of(layer)!=0?43:42);return 0;};
    layers.AEGP_GetCompNumLayers=[](AEGP_CompH comp,A_long* out)->A_Err {CHECK(comp==comp_handle());if(const auto e=step())return e;*out=layer_count;return 0;};
    layers.AEGP_GetCompLayerByIndex=[](AEGP_CompH comp,A_long i,AEGP_LayerH* out)->A_Err {CHECK(comp==comp_handle());CHECK(i>=0&&i<layer_count);if(const auto e=step())return e;*out=no_layer?nullptr:layer_handle(i);return 0;};
    layers.AEGP_GetLayerObjectType=[](AEGP_LayerH layer,AEGP_ObjectType* out)->A_Err {if(const auto e=step())return e;*out=types[index_of(layer)];return 0;};
    layers.AEGP_GetLayerID=[](AEGP_LayerH layer,AEGP_LayerIDVal* out)->A_Err {if(const auto e=step())return e;const auto i=index_of(layer);*out=ids[static_cast<int>(i)==duplicate_index?1:i];return 0;};
    layers.AEGP_IsVideoActive=[](AEGP_LayerH layer,AEGP_LTimeMode mode,const A_Time* time,A_Boolean* out)->A_Err {
        CHECK(mode==AEGP_LTimeMode_CompTime);CHECK(time->value==expected_time.value&&time->scale==expected_time.scale);if(const auto e=step())return e;
        *out=index_of(layer)==1||time->value>=static_cast<std::int64_t>(time->scale)*2;return 0;
    };
    layers.AEGP_GetLayerName=[](AEGP_PluginID plugin,AEGP_LayerH layer,AEGP_MemHandle* out,AEGP_MemHandle* source)->A_Err {
        CHECK(plugin==17);CHECK(source==nullptr);if(const auto e=step())return e;
        const auto i=index_of(layer);CHECK(!handles[i]);*out=null_name?nullptr:name_handle(i);
        if(*out){handles[i]=true;++active_handles;}return name_fail_with_handle?77:0;
    };
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH comp,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err {
        CHECK(comp==comp_handle());if(const auto e=step())return e;
        const auto found=std::find(ids.begin(),ids.end(),id);CHECK(found!=ids.end());*out=layer_handle(static_cast<unsigned>(found-ids.begin()));return 0;
    };
    layers.AEGP_GetLayerToWorldXform=[](AEGP_LayerH layer,const A_Time* time,A_Matrix4* out)->A_Err {
        CHECK(time->value==expected_time.value&&time->scale==expected_time.scale);if(const auto e=step())return e;
        const auto i=index_of(layer);++matrix_calls[i];*out={};for(unsigned a=0;a<4;++a)out->mat[a][a]=1;
        out->mat[3][0]=400+static_cast<double>(i)*100;out->mat[3][1]=300;return 0;
    };
    memory={};memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle handle,AEGP_MemSize* out)->A_Err {const auto i=index_of(handle);CHECK(handles[i]);if(const auto e=step())return e;*out=override_bytes?override_bytes:static_cast<AEGP_MemSize>((lengths[i]+1)*sizeof(A_UTF16Char));return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle handle,void** out)->A_Err {const auto i=index_of(handle);CHECK(handles[i]&&!locks[i]);if(const auto e=step())return e;locks[i]=true;++active_locks;*out=null_pointer?nullptr:names[i].data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle handle)->A_Err {const auto i=index_of(handle);CHECK(handles[i]&&locks[i]);locks[i]=false;--active_locks;return cleanup_fail==1?88:0;};
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle handle)->A_Err {const auto i=index_of(handle);CHECK(handles[i]&&!locks[i]);handles[i]=false;--active_handles;return cleanup_fail==2?88:0;};
    data={};data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.pica_basicP=&basic;data.time_scale=24;
}
void balanced(){CHECK(active_suites==0);CHECK(acquired==released);CHECK(active_handles==0);CHECK(active_locks==0);}
std::vector<MotionLightSource> sentinel(){return {{99,77,u"prior",true}};}
void preserved(const std::vector<MotionLightSource>& output){CHECK(output.size()==1);CHECK(output[0].layer_id==99&&output[0].comp_index==77&&output[0].name==u"prior"&&output[0].video_active);balanced();}
struct CancelAfter final:Cancellation{mutable unsigned calls{};unsigned limit;explicit CancelAfter(unsigned n):limit(n){};bool is_cancelled()const noexcept override{return calls++>=limit;}};
}
int main(){
    NeverCancelled never;const auto run=[&](std::vector<MotionLightSource>& output){return capture_motion_light_sources(&data,17,never,output);};
    reset();auto output=sentinel();CHECK(run(output)==0);CHECK(output.size()==2);CHECK(output[0].layer_id==1001&&output[0].comp_index==1&&output[0].name==u"Orbit_灯光_\U0001f680"&&output[0].video_active);
    CHECK(output[1].layer_id==1003&&output[1].comp_index==3&&output[1].name==u"LookAt_灯光"&&!output[1].video_active);balanced();const auto baseline_calls=calls;
    std::vector<std::uint32_t> selected{999};CHECK(select_motion_light_source_ids(output,u"Orbit_",selected)==0);CHECK(selected==std::vector<std::uint32_t>{1001});
    CHECK(select_motion_light_source_ids(output,u"orbit_",selected)==0);CHECK(selected.empty());
    CHECK(select_motion_light_source_ids(output,u"",selected)==0);CHECK(selected.size()==2&&selected[0]==1001&&selected[1]==1003);
    CHECK(select_motion_light_source_ids(output,u"LookAt_灯光",selected)==0);CHECK(selected==std::vector<std::uint32_t>{1003});
    CHECK(select_motion_light_source_ids(output,std::u16string_view(u"A\0B",3),selected)==PF_Err_BAD_CALLBACK_PARAM);CHECK(selected==std::vector<std::uint32_t>{1003});
    allocation_countdown=0;CHECK(select_motion_light_source_ids(output,u"",selected)==PF_Err_OUT_OF_MEMORY);allocation_countdown=-1;CHECK(selected==std::vector<std::uint32_t>{1003});
    const auto invalid_selection=[&](std::vector<MotionLightSource> sources){CHECK(select_motion_light_source_ids(sources,u"",selected)==PF_Err_BAD_CALLBACK_PARAM);CHECK(selected==std::vector<std::uint32_t>{1003});};
    auto malformed=output;malformed[1].layer_id=malformed[0].layer_id;invalid_selection(malformed);
    malformed=output;malformed[1].comp_index=malformed[0].comp_index;invalid_selection(malformed);
    malformed=output;std::reverse(malformed.begin(),malformed.end());invalid_selection(malformed);
    malformed=output;malformed[0].layer_id=0x80000000u;invalid_selection(malformed);
    malformed=output;malformed[0].name.assign({u'A',char16_t{},u'B'});invalid_selection(malformed);
    name(1,u"Renamed");CHECK(output[0].name==u"Orbit_灯光_\U0001f680");data.current_time=24;time_offset=1;time_multiplier=2;CHECK(run(output)==0);CHECK(output[0].name==u"Renamed"&&output[1].video_active);balanced();
    std::vector<MotionLayerPointRequest> requests;for(const auto& source:output)requests.push_back({source.layer_id,{}});
    std::vector<MotionLayerPoint> points;CHECK(capture_motion_layer_points(&data,{800,600,1},requests,never,points)==0);CHECK(points.size()==2);CHECK(std::abs(points[0].position.x+.5)<1e-12&&std::abs(points[1].position.x+1./6)<1e-12);CHECK(points[0].position.y==.5&&points[0].position.z==-.5);CHECK(matrix_calls[0]==1&&matrix_calls[1]==1&&matrix_calls[3]==1);balanced();
    for(unsigned fail=1;fail<=baseline_calls;++fail){reset();output=sentinel();fail_at=fail;CHECK(run(output)==77);preserved(output);}
    for(unsigned fail=1;fail<=3;++fail){reset();output=sentinel();release_fail=fail;CHECK(run(output)==88);preserved(output);}
    for(unsigned fail=1;fail<=2;++fail){reset();output=sentinel();cleanup_fail=fail;CHECK(run(output)==88);preserved(output);}
    for(unsigned absent=1;absent<=3;++absent){reset();output=sentinel();null_suite=absent;CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    for(unsigned limit=0;limit<8;++limit){reset();output=sentinel();CancelAfter cancel(limit);CHECK(capture_motion_light_sources(&data,17,cancel,output)==PF_Interrupt_CANCEL);preserved(output);}
    for(unsigned test=0;test<14;++test){
        reset();output=sentinel();switch(test){case 0:no_owner=true;break;case 1:no_comp=true;break;case 2:no_layer=true;break;case 3:wrong_comp=true;break;case 4:zero_time=true;break;case 5:null_name=true;break;case 6:null_pointer=true;break;case 7:duplicate_index=3;break;case 8:ids[1]=0;break;case 9:ids[1]=-1;break;case 10:layer_count=-1;break;case 11:layer_count=65537;break;case 12:override_bytes=1;break;case 13:override_bytes=(kMaxMotionLightNameUnits+2)*sizeof(A_UTF16Char);break;}
        CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);
    }
    reset();output=sentinel();name_fail_with_handle=true;CHECK(run(output)==77);preserved(output);
    reset();output=sentinel();names[1].fill('x');override_bytes=(kMaxMotionLightNameUnits+1)*sizeof(A_UTF16Char);CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);
    reset();output=sentinel();name(1,u"");CHECK(run(output)==0);CHECK(output[0].name.empty());balanced();
    reset();output=sentinel();names[1].fill('x');names[1][kMaxMotionLightNameUnits]=0;lengths[1]=kMaxMotionLightNameUnits;CHECK(run(output)==0);CHECK(output[0].name.size()==kMaxMotionLightNameUnits);balanced();
    reset();output=sentinel();layer_count=256;types.fill(AEGP_ObjectType_LIGHT);CHECK(run(output)==0);CHECK(output.size()==256);for(unsigned i=0;i<256;++i)CHECK(output[i].layer_id==1000+i&&output[i].comp_index==i);balanced();
    reset();output=sentinel();layer_count=257;types.fill(AEGP_ObjectType_LIGHT);CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);
    reset();output=sentinel();layer_count=0;CHECK(run(output)==0);CHECK(output.empty());balanced();
    reset();output=sentinel();types.fill(AEGP_ObjectType_AV);CHECK(run(output)==0);CHECK(output.empty());balanced();
    for(int fail=0;fail<4;++fail){reset();output=sentinel();name(1,u"This sufficiently long name must allocate owned storage");name(3,u"Another sufficiently long name must allocate owned storage");allocation_countdown=fail;CHECK(run(output)==PF_Err_OUT_OF_MEMORY);allocation_countdown=-1;preserved(output);}
    for(unsigned missing=0;missing<7;++missing){reset();output=sentinel();switch(missing){case 0:layers.AEGP_GetLayerName=nullptr;break;case 1:layers.AEGP_IsVideoActive=nullptr;break;case 2:layers.AEGP_GetLayerObjectType=nullptr;break;case 3:memory.AEGP_FreeMemHandle=nullptr;break;case 4:memory.AEGP_UnlockMemHandle=nullptr;break;case 5:memory.AEGP_LockMemHandle=nullptr;break;case 6:memory.AEGP_GetMemHandleSize=nullptr;break;}CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    reset();output=sentinel();CHECK(capture_motion_light_sources(nullptr,17,never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(capture_motion_light_sources(&data,0,never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(capture_motion_light_sources(&data,-1,never,output)==PF_Err_BAD_CALLBACK_PARAM);data.time_scale=0;CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);
    std::cout<<"motion_light_sources_tests: "<<checks<<" checks passed; owned read-only inventory/point capture; no author policy or AE qualification\n";
}
