#include "MotionLightTiming.hpp"
#include "SPBasic.h"
#include "starfield/core/Render.hpp"
#include <array>
#include <atomic>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <new>
namespace {std::atomic<unsigned> allocations{};}
void* operator new(std::size_t n){++allocations;if(auto* p=std::malloc(n?n:1))return p;throw std::bad_alloc{};}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
using namespace starfield::adapter;using namespace starfield::core;
namespace {
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layer{};AEGP_LightSuite2 light{};
AEGP_StreamSuite6 stream{};AEGP_DynamicStreamSuite4 dynamic{};AEGP_KeyframeSuite5 keys{};
SPBasicSuite basic{};PF_InData data{};
unsigned calls{},fail_at{},active_suites{},acquired{},released{},active_refs{},disposed{},cleanup_calls{},cleanup_fail{},null_suite{};
bool leader=true,separated{},no_owner{},no_comp{},no_source{},wrong_comp{},wrong_id{},failed_leader{},failed_follower{};
int null_ref=-1,bad_follower=-1;AEGP_ObjectType object_type=AEGP_ObjectType_LIGHT;AEGP_LightType light_type=AEGP_LightType_POINT;
AEGP_StreamType stream_type=AEGP_StreamType_ThreeD_SPATIAL;
std::array<A_long,4> counts{};std::array<A_Time,4> starts{},ends{};
AEGP_LayerH owner(){return reinterpret_cast<AEGP_LayerH>(1);}AEGP_LayerH source(){return reinterpret_cast<AEGP_LayerH>(2);}
AEGP_CompH comp(unsigned n=42){return reinterpret_cast<AEGP_CompH>(static_cast<std::uintptr_t>(n));}
unsigned index(AEGP_StreamRefH h){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(h)-1);}
A_Err step(){return ++calls==fail_at?77:0;}
AEGP_StreamRefH create_ref(unsigned i){if(static_cast<int>(i)==null_ref)return nullptr;CHECK(!(active_refs&(1u<<i)));active_refs|=1u<<i;return reinterpret_cast<AEGP_StreamRefH>(static_cast<std::uintptr_t>(i+1));}
void reset(){
    calls=fail_at=active_suites=acquired=released=active_refs=disposed=cleanup_calls=cleanup_fail=null_suite=0;
    leader=true;separated=no_owner=no_comp=no_source=wrong_comp=wrong_id=failed_leader=failed_follower=false;
    null_ref=bad_follower=-1;object_type=AEGP_ObjectType_LIGHT;light_type=AEGP_LightType_POINT;stream_type=AEGP_StreamType_ThreeD_SPATIAL;
    counts={4,2,0,3};starts={A_Time{30,60},A_Time{-15,30},A_Time{},A_Time{4,1}};ends={A_Time{180,60},A_Time{9,6},A_Time{},A_Time{20,5}};
    basic={};basic.AcquireSuite=[](const char* name,int32 version,const void** out)->A_Err {
        if(auto e=step())return e;unsigned id{};
        if(!std::strcmp(name,kAEGPPFInterfaceSuite)){id=1;CHECK(version==kAEGPPFInterfaceSuiteVersion1);*out=&pf;}
        else if(!std::strcmp(name,kAEGPLayerSuite)){id=2;CHECK(version==kAEGPLayerSuiteVersion9);*out=&layer;}
        else if(!std::strcmp(name,kAEGPLightSuite)){id=3;CHECK(version==kAEGPLightSuiteVersion2);*out=&light;}
        else if(!std::strcmp(name,kAEGPStreamSuite)){id=4;CHECK(version==kAEGPStreamSuiteVersion6);*out=&stream;}
        else if(!std::strcmp(name,kAEGPDynamicStreamSuite)){id=5;CHECK(version==kAEGPDynamicStreamSuiteVersion4);*out=&dynamic;}
        else{CHECK(!std::strcmp(name,kAEGPKeyframeSuite));id=6;CHECK(version==kAEGPKeyframeSuiteVersion5);*out=&keys;}
        ++acquired;++active_suites;if(null_suite==id)*out=nullptr;return 0;
    };
    basic.ReleaseSuite=[](const char*,int32)->A_Err {CHECK(active_suites>0);--active_suites;++released;return ++cleanup_calls==cleanup_fail?88:0;};
    pf={};pf.AEGP_GetEffectLayer=[](PF_ProgPtr effect,AEGP_LayerH* out)->A_Err {CHECK(effect==data.effect_ref);if(auto e=step())return e;*out=no_owner?nullptr:owner();return 0;};
    layer={};layer.AEGP_GetLayerParentComp=[](AEGP_LayerH h,AEGP_CompH* out)->A_Err {CHECK(h==owner()||h==source());if(auto e=step())return e;*out=no_comp?nullptr:comp(wrong_comp&&h==source()?43:42);return 0;};
    layer.AEGP_GetLayerFromLayerID=[](AEGP_CompH h,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err {CHECK(h==comp()&&id==1001);if(auto e=step())return e;*out=no_source?nullptr:source();return 0;};
    layer.AEGP_GetLayerID=[](AEGP_LayerH h,AEGP_LayerIDVal* out)->A_Err {CHECK(h==source());if(auto e=step())return e;*out=wrong_id?1002:1001;return 0;};
    layer.AEGP_GetLayerObjectType=[](AEGP_LayerH h,AEGP_ObjectType* out)->A_Err {CHECK(h==source());if(auto e=step())return e;*out=object_type;return 0;};
    light={};light.AEGP_GetLightType=[](AEGP_LayerH h,AEGP_LightType* out)->A_Err {CHECK(h==source());if(auto e=step())return e;*out=light_type;return 0;};
    stream={};stream.AEGP_GetNewLayerStream=[](AEGP_PluginID plugin,AEGP_LayerH h,AEGP_LayerStream which,AEGP_StreamRefH* out)->A_Err {
        CHECK(plugin==17&&h==source()&&which==AEGP_LayerStream_POSITION);if(auto e=step())return e;*out=create_ref(0);return failed_leader?79:0;
    };
    stream.AEGP_DisposeStream=[](AEGP_StreamRefH h)->A_Err {const auto bit=1u<<index(h);CHECK(active_refs&bit);active_refs&=~bit;++disposed;return ++cleanup_calls==cleanup_fail?88:0;};
    stream.AEGP_GetStreamType=[](AEGP_StreamRefH h,AEGP_StreamType* out)->A_Err {if(auto e=step())return e;const auto i=index(h);*out=i==0?stream_type:static_cast<int>(i)==bad_follower?AEGP_StreamType_ThreeD:AEGP_StreamType_OneD;return 0;};
    dynamic={};dynamic.AEGP_IsSeparationLeader=[](AEGP_StreamRefH h,A_Boolean* out)->A_Err {CHECK(index(h)==0);if(auto e=step())return e;*out=leader;return 0;};
    dynamic.AEGP_AreDimensionsSeparated=[](AEGP_StreamRefH h,A_Boolean* out)->A_Err {CHECK(index(h)==0&&leader);if(auto e=step())return e;*out=separated;return 0;};
    dynamic.AEGP_GetSeparationFollower=[](AEGP_StreamRefH h,A_short dim,AEGP_StreamRefH* out)->A_Err {
        CHECK(index(h)==0&&leader&&separated&&dim>=0&&dim<3);if(auto e=step())return e;*out=create_ref(dim+1);return failed_follower&&dim==1?79:0;
    };
    keys={};keys.AEGP_GetStreamNumKFs=[](AEGP_StreamRefH h,A_long* out)->A_Err {CHECK(active_refs&(1u<<index(h)));if(auto e=step())return e;*out=counts[index(h)];return 0;};
    keys.AEGP_GetKeyframeTime=[](AEGP_StreamRefH h,AEGP_KeyframeIndex key,AEGP_LTimeMode mode,A_Time* out)->A_Err {
        const auto i=index(h);CHECK(mode==AEGP_LTimeMode_CompTime&&key>=0&&key<counts[i]);CHECK(key==0||key==counts[i]-1);if(auto e=step())return e;*out=key==0?starts[i]:ends[i];return 0;
    };
    data={};data.effect_ref=reinterpret_cast<PF_ProgPtr>(7);data.pica_basicP=&basic;
}
struct Never:Cancellation{bool is_cancelled()const noexcept override{return false;}}never;
struct Stop:Cancellation{mutable unsigned calls{};unsigned limit;explicit Stop(unsigned n):limit(n){}bool is_cancelled()const noexcept override{return calls++>=limit;}};
MotionLightTiming sentinel(){return {999,19,{-1,3},{8,7},true};}
bool same(const MotionLightTiming& a,const MotionLightTiming& b){return a.layer_id==b.layer_id&&a.position_keyframes==b.position_keyframes&&a.first_keyframe.value==b.first_keyframe.value&&a.first_keyframe.scale==b.first_keyframe.scale&&a.last_keyframe.value==b.last_keyframe.value&&a.last_keyframe.scale==b.last_keyframe.scale&&a.separated==b.separated;}
void balanced(){CHECK(!active_refs&&!active_suites);CHECK(acquired==released);}
PF_Err run(MotionLightTiming& out){return capture_motion_light_timing(&data,17,1001,never,out);}
void preserved(const MotionLightTiming& out){CHECK(same(out,sentinel()));balanced();}
void time(const RationalTime& actual,RationalTime expected){CHECK(actual.value==expected.value&&actual.scale==expected.scale);}
void modes(){
    reset();auto out=sentinel();auto before=allocations.load();CHECK(run(out)==0);CHECK(allocations==before);CHECK(out.layer_id==1001&&out.position_keyframes==4&&!out.separated);time(out.first_keyframe,{1,2});time(out.last_keyframe,{3,1});balanced();CHECK(disposed==1);
    reset();leader=false;CHECK(run(out)==0);time(out.first_keyframe,{1,2});balanced();
    reset();stream_type=AEGP_StreamType_ThreeD;CHECK(run(out)==0);balanced();
    reset();counts[0]=0;CHECK(run(out)==0);CHECK(out.position_keyframes==0);time(out.first_keyframe,{0,1});time(out.last_keyframe,{0,1});balanced();
    reset();counts[0]=1;starts[0]={-9,18};CHECK(run(out)==0);CHECK(out.position_keyframes==1);time(out.first_keyframe,{-1,2});time(out.last_keyframe,{-1,2});balanced();
    reset();std::swap(starts[0],ends[0]);CHECK(run(out)==0);time(out.first_keyframe,{1,2});time(out.last_keyframe,{3,1});balanced();
    reset();counts[0]=std::numeric_limits<A_long>::max();CHECK(run(out)==0);CHECK(out.position_keyframes==static_cast<std::uint64_t>(counts[0]));balanced();
    reset();starts[0]={std::numeric_limits<A_long>::min(),std::numeric_limits<A_u_long>::max()};ends[0]={std::numeric_limits<A_long>::max(),std::numeric_limits<A_u_long>::max()};CHECK(run(out)==0);CHECK(compare(out.first_keyframe,out.last_keyframe)<0);balanced();
    reset();separated=true;ends[3]={25,5};CHECK(run(out)==0);CHECK(out.separated&&out.position_keyframes==5);time(out.first_keyframe,{-1,2});time(out.last_keyframe,{5,1});balanced();CHECK(disposed==4);
    for(unsigned axis=1;axis<=3;++axis){reset();separated=true;counts.fill(0);counts[axis]=1;starts[axis]={static_cast<A_long>(axis),2};CHECK(run(out)==0);CHECK(out.position_keyframes==1);CHECK(compare(out.first_keyframe,{axis,2})==0&&compare(out.first_keyframe,out.last_keyframe)==0);balanced();}
    reset();separated=true;counts.fill(0);CHECK(run(out)==0);CHECK(out.separated&&out.position_keyframes==0);time(out.first_keyframe,{0,1});time(out.last_keyframe,{0,1});balanced();
}
void failures(){
    for(bool split:{false,true}){
        reset();separated=split;ends[3]={25,5};auto out=sentinel();CHECK(run(out)==0);const auto baseline=calls;
        for(unsigned fail=1;fail<=baseline;++fail){reset();separated=split;ends[3]={25,5};out=sentinel();fail_at=fail;CHECK(run(out)==77);preserved(out);}
        for(unsigned fail=1;fail<=(split?10u:7u);++fail){reset();separated=split;ends[3]={25,5};out=sentinel();cleanup_fail=fail;CHECK(run(out)==88);preserved(out);}
        bool success=false;for(unsigned limit=0;limit<32;++limit){reset();separated=split;ends[3]={25,5};out=sentinel();Stop stop(limit);auto result=capture_motion_light_timing(&data,17,1001,stop,out);if(!result){balanced();success=true;break;}CHECK(result==PF_Interrupt_CANCEL);preserved(out);}CHECK(success);
    }
    for(unsigned which=1;which<=6;++which){reset();auto out=sentinel();null_suite=which;CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
    for(int ref=0;ref<4;++ref){reset();separated=true;ends[3]={25,5};null_ref=ref;auto out=sentinel();CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
    for(int ref=1;ref<4;++ref){reset();separated=true;bad_follower=ref;auto out=sentinel();CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
    reset();failed_leader=true;auto out=sentinel();CHECK(run(out)==79);preserved(out);CHECK(disposed==1);
    reset();separated=true;failed_follower=true;out=sentinel();CHECK(run(out)==79);preserved(out);CHECK(disposed==3);
    for(unsigned test=0;test<13;++test){reset();out=sentinel();switch(test){case 0:no_owner=true;break;case 1:no_comp=true;break;case 2:no_source=true;break;case 3:wrong_comp=true;break;case 4:wrong_id=true;break;case 5:object_type=AEGP_ObjectType_CAMERA;break;case 6:light_type=AEGP_LightType_SPOT;break;case 7:stream_type=AEGP_StreamType_OneD;break;case 8:counts[0]=AEGP_NumKF_NO_DATA;break;case 9:counts[0]=-2;break;case 10:starts[0].scale=0;break;case 11:ends[0].scale=0;break;case 12:ends[0]=starts[0];break;}CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
    for(unsigned missing=0;missing<14;++missing){reset();out=sentinel();switch(missing){case 0:pf.AEGP_GetEffectLayer=nullptr;break;case 1:layer.AEGP_GetLayerParentComp=nullptr;break;case 2:layer.AEGP_GetLayerFromLayerID=nullptr;break;case 3:layer.AEGP_GetLayerID=nullptr;break;case 4:layer.AEGP_GetLayerObjectType=nullptr;break;case 5:light.AEGP_GetLightType=nullptr;break;case 6:stream.AEGP_GetNewLayerStream=nullptr;break;case 7:stream.AEGP_DisposeStream=nullptr;break;case 8:stream.AEGP_GetStreamType=nullptr;break;case 9:dynamic.AEGP_IsSeparationLeader=nullptr;break;case 10:dynamic.AEGP_AreDimensionsSeparated=nullptr;break;case 11:dynamic.AEGP_GetSeparationFollower=nullptr;break;case 12:keys.AEGP_GetStreamNumKFs=nullptr;break;case 13:keys.AEGP_GetKeyframeTime=nullptr;break;}CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
    reset();out=sentinel();CHECK(capture_motion_light_timing(nullptr,17,1001,never,out)==PF_Err_BAD_CALLBACK_PARAM);CHECK(capture_motion_light_timing(&data,0,1001,never,out)==PF_Err_BAD_CALLBACK_PARAM);
    for(auto id:{0u,0x80000000u})CHECK(capture_motion_light_timing(&data,17,id,never,out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);
    data.effect_ref=nullptr;CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);
    for(unsigned test=0;test<3;++test){reset();out=sentinel();if(test==0)data.pica_basicP=nullptr;else if(test==1)basic.AcquireSuite=nullptr;else basic.ReleaseSuite=nullptr;CHECK(run(out)==PF_Err_BAD_CALLBACK_PARAM);preserved(out);}
}
}
int main(){modes();failures();std::cout<<"Motion light timing: "<<checks<<" checks passed; fake May2023 callbacks, no AE qualification\n";}
