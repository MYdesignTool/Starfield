#include "MotionPointCapture.hpp"
#include "AE_GeneralPlug.h"
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

namespace {std::atomic<bool> fail_allocation{};}
void* operator new(std::size_t size){if(fail_allocation.exchange(false))throw std::bad_alloc{};if(auto* p=std::malloc(std::max<std::size_t>(1,size)))return p;throw std::bad_alloc{};}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}
using namespace starfield::adapter;
using namespace starfield::core;
namespace {
unsigned checks{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
AEGP_PFInterfaceSuite1 pf{};AEGP_LayerSuite9 layers{};SPBasicSuite basic{};PF_InData data{};
std::array<A_Matrix4,258> matrices{};
std::array<unsigned,258> matrix_calls{};
unsigned calls{},fail_at{},acquired{},released{},active{},release_fail{},release_calls{},null_suite{};
bool missing_owner{},missing_comp{},missing_source{},wrong_id{},wrong_comp{},zero_time{};
bool timed_mode{},quadratic{};
struct Sample {unsigned layer;A_Time time;};std::array<Sample,kMaxMotionPointRequests+1> samples{};unsigned sample_count{};
A_long time_offset{};unsigned time_multiplier=1;double animate_x{};
A_Time expected_time{};
AEGP_LayerH layer_handle(std::uint32_t id){return reinterpret_cast<AEGP_LayerH>(static_cast<std::uintptr_t>(id));}
AEGP_CompH comp_handle(std::uintptr_t id=42){return reinterpret_cast<AEGP_CompH>(id);}
unsigned layer_number(AEGP_LayerH layer){return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(layer));}
bool near(Vec3 a,Vec3 b,double tolerance=1e-9){return std::abs(a.x-b.x)<=tolerance && std::abs(a.y-b.y)<=tolerance && std::abs(a.z-b.z)<=tolerance;}
A_Err step(){return ++calls==fail_at?77:0;}
void identity(A_Matrix4& m){m={};for(unsigned i=0;i<4;++i)m.mat[i][i]=1;}
void reset(){
    calls=fail_at=acquired=released=active=release_fail=release_calls=null_suite=0;
    missing_owner=missing_comp=missing_source=wrong_id=wrong_comp=zero_time=false;time_offset=0;time_multiplier=1;animate_x=0;
    timed_mode=quadratic=false;sample_count=0;
    matrix_calls.fill(0);for(auto& m:matrices)identity(m);
    matrices[2].mat[3][0]=320;matrices[2].mat[3][1]=240;
    matrices[3].mat[3][0]=800;matrices[3].mat[3][1]=240;
    basic={};basic.AcquireSuite=[](const char* name,int32 version,const void** out)->A_Err {
        if(const auto error=step())return error;
        ++acquired;++active;
        if(!std::strcmp(name,kAEGPPFInterfaceSuite)){CHECK(version==kAEGPPFInterfaceSuiteVersion1);*out=null_suite==1?nullptr:&pf;}
        else {CHECK(!std::strcmp(name,kAEGPLayerSuite));CHECK(version==kAEGPLayerSuiteVersion9);*out=null_suite==2?nullptr:&layers;}
        return 0;
    };
    basic.ReleaseSuite=[](const char* name,int32 version)->A_Err {
        CHECK(active>0);--active;++released;++release_calls;
        CHECK((!std::strcmp(name,kAEGPLayerSuite) && version==kAEGPLayerSuiteVersion9) || (!std::strcmp(name,kAEGPPFInterfaceSuite) && version==kAEGPPFInterfaceSuiteVersion1));
        return release_calls==release_fail?88:0;
    };
    pf={};pf.AEGP_GetEffectLayer=[](PF_ProgPtr effect,AEGP_LayerH* out)->A_Err {
        CHECK(effect==data.effect_ref);if(const auto error=step())return error;*out=missing_owner?nullptr:layer_handle(1);return 0;
    };
    pf.AEGP_ConvertEffectToCompTime=[](PF_ProgPtr effect,A_long value,A_u_long scale,A_Time* out)->A_Err {
        CHECK(effect==data.effect_ref);CHECK(value==data.current_time && scale==data.time_scale);
        if(const auto error=step())return error;
        expected_time={static_cast<A_long>(static_cast<std::int64_t>(value)*time_multiplier+static_cast<std::int64_t>(time_offset)*scale),zero_time?0:scale};*out=expected_time;return 0;
    };
    layers={};layers.AEGP_GetLayerParentComp=[](AEGP_LayerH layer,AEGP_CompH* out)->A_Err {
        if(const auto error=step())return error;*out=missing_comp?nullptr:comp_handle(wrong_comp && layer_number(layer)!=1?43:42);return 0;
    };
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH comp,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err {
        CHECK(comp==comp_handle());CHECK(id>0 && id<258);if(const auto error=step())return error;*out=missing_source?nullptr:layer_handle(id);return 0;
    };
    layers.AEGP_GetLayerID=[](AEGP_LayerH layer,AEGP_LayerIDVal* out)->A_Err {
        if(const auto error=step())return error;*out=static_cast<AEGP_LayerIDVal>(layer_number(layer)+(wrong_id?1:0));return 0;
    };
    layers.AEGP_GetLayerToWorldXform=[](AEGP_LayerH layer,const A_Time* time,A_Matrix4* out)->A_Err {
        const auto id=layer_number(layer);CHECK(time->scale>0);
        if(!timed_mode||(id==1&&matrix_calls[1]==0))CHECK(time->value==expected_time.value && time->scale==expected_time.scale);
        if(const auto error=step())return error;CHECK(id>0 && id<258);++matrix_calls[id];*out=matrices[id];
        CHECK(sample_count<samples.size());samples[sample_count++]={id,*time};
        if(id!=1||timed_mode){const auto t=static_cast<double>(time->value)/time->scale;out->mat[3][0]+=animate_x*(quadratic?t*t:t);}return 0;
    };
    data={};data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.pica_basicP=&basic;data.time_scale=24;
}
void preserved(const std::vector<MotionLayerPoint>& output){CHECK(output.size()==1);CHECK(output[0].layer_id==999);CHECK(near(output[0].position,{1,2,3},0));CHECK(active==0);CHECK(acquired==released);}
struct CancelAfter final:Cancellation{mutable unsigned calls{};unsigned limit;explicit CancelAfter(unsigned n):limit(n){};bool is_cancelled()const noexcept override{return calls++>=limit;}};
void timed_points(){
    NeverCancelled never;const LayerUnits units{640,480,1};
    const std::array<MotionTimedLayerPointRequest,6> requests{{{2,{}, {2,1}},{2,{480,0,240},{4,2}},
        {3,{}, {7,2}},{2,{}, {0,1}},{1,{320,240,0},{2,1}},{1,{320,240,0},{10,2}}}};
    const auto setup=[](){reset();timed_mode=true;data.current_time=24;time_offset=3;time_multiplier=2;animate_x=10;};
    std::vector<MotionTimedLayerPoint> output;
    const auto run=[&](){return capture_motion_timed_layer_points(&data,units,requests,never,output);};
    const auto sentinel=[&](){output={{999,{1,2,3},{-3,7}}};};
    const auto preserved=[&](){CHECK(output.size()==1&&output[0].layer_id==999&&near(output[0].position,{1,2,3},0));CHECK(output[0].source_comp_time.value==-3&&output[0].source_comp_time.scale==7);CHECK(!active&&acquired==released);};
    setup();CHECK(run()==0);const auto successful=calls;CHECK(output.size()==6);
    CHECK(near(output[0].position,{-1./16,0,-.5}));CHECK(near(output[1].position,{15./16,0,0}));CHECK(near(output[2].position,{31./32,0,-.5}));
    CHECK(near(output[3].position,{-5./48,0,-.5}));CHECK(near(output[4].position,{-1./16,0,-.5}));CHECK(near(output[5].position,{0,0,-.5},0));
    CHECK(matrix_calls[1]==2&&matrix_calls[2]==2&&matrix_calls[3]==1&&sample_count==5);CHECK(!active&&acquired==released);
    CHECK(samples[0].layer==1&&samples[0].time.value==120&&samples[0].time.scale==24);
    for(unsigned i=0;i<5;++i){const std::array<Sample,5> expected{{{1,{120,24}},{2,{2,1}},{3,{7,2}},{2,{0,1}},{1,{2,1}}}};CHECK(samples[i].layer==expected[i].layer&&samples[i].time.value==expected[i].time.value&&samples[i].time.scale==expected[i].time.scale);}
    CHECK(output[1].source_comp_time.value==2&&output[1].source_comp_time.scale==1);CHECK(output[5].source_comp_time.value==5&&output[5].source_comp_time.scale==1);
    setup();quadratic=true;CHECK(run()==0);CHECK(near(output[0].position,{-7./16,0,-.5}));CHECK(near(output[2].position,{47./64,0,-.5}));CHECK(matrix_calls[2]==2);
    setup();matrices[1].mat[0][0]=2;matrices[1].mat[1][0]=.5;matrices[1].mat[1][1]=3;matrices[1].mat[2][2]=-4;
    matrices[1].mat[3][0]=100;matrices[1].mat[3][1]=200;matrices[1].mat[3][2]=300;
    matrices[2]={};matrices[2].mat[0][0]=.5;matrices[2].mat[0][1]=3;matrices[2].mat[1][0]=-2;matrices[2].mat[2][2]=-8;
    matrices[2].mat[3][0]=860;matrices[2].mat[3][1]=920;matrices[2].mat[3][2]=300;matrices[2].mat[3][3]=1;
    const MotionTimedLayerPointRequest affine{2,{480,0,120},{2,1}};
    CHECK(capture_motion_timed_layer_points(&data,{640,480,2},std::span(&affine,1),never,output)==0);CHECK(near(output[0].position,{-1./16,-1,0}));CHECK(!active);
    for(unsigned fail=1;fail<=successful;++fail){setup();sentinel();fail_at=fail;CHECK(run()==77);preserved();}
    for(unsigned fail=1;fail<=2;++fail){setup();sentinel();release_fail=fail;CHECK(run()==88);preserved();}
    bool success=false;for(unsigned limit=0;limit<32;++limit){setup();sentinel();CancelAfter cancel(limit);auto result=capture_motion_timed_layer_points(&data,units,requests,cancel,output);if(!result){CHECK(!active);success=true;break;}CHECK(result==PF_Interrupt_CANCEL);preserved();}CHECK(success);
    setup();sentinel();fail_allocation=true;CHECK(run()==PF_Err_OUT_OF_MEMORY);CHECK(!fail_allocation);preserved();
    for(RationalTime bad:std::array<RationalTime,6>{{{1,0},{1,-1},{std::numeric_limits<std::int64_t>::max(),1},{std::numeric_limits<std::int64_t>::min(),1},{1,std::int64_t{1}<<32},{std::int64_t{1}<<31,1}}}){
        setup();sentinel();const MotionTimedLayerPointRequest invalid{2,{},bad};CHECK(capture_motion_timed_layer_points(&data,units,std::span(&invalid,1),never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(!calls);preserved();}
    for(RationalTime boundary:std::array<RationalTime,4>{{{4294967294LL,2},{-4294967296LL,2},{1,4294967295LL},{0,std::numeric_limits<std::int64_t>::max()}}}){
        setup();animate_x=0;const MotionTimedLayerPointRequest valid{2,{},boundary};CHECK(capture_motion_timed_layer_points(&data,units,std::span(&valid,1),never,output)==0);CHECK(compare(output[0].source_comp_time,boundary)==0);CHECK(output[0].source_comp_time.scale<=4294967295LL);CHECK(!active);}
    setup();std::array<MotionTimedLayerPointRequest,kMaxMotionPointRequests> maximum{};for(unsigned i=0;i<maximum.size();++i)maximum[i]={2,{}, {i,60}};
    CHECK(capture_motion_timed_layer_points(&data,units,maximum,never,output)==0);CHECK(output.size()==maximum.size());CHECK(matrix_calls[1]==1&&matrix_calls[2]==kMaxMotionPointRequests&&sample_count==kMaxMotionPointRequests+1);CHECK(!active);
    setup();sentinel();CHECK(capture_motion_timed_layer_points(&data,units,{},never,output)==0);CHECK(output.empty()&&!calls);
    setup();sentinel();std::array<MotionTimedLayerPointRequest,kMaxMotionPointRequests+1> excessive{};CHECK(capture_motion_timed_layer_points(&data,units,excessive,never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(!calls);preserved();
    for(unsigned fault=0;fault<4;++fault){setup();sentinel();if(fault==0)wrong_comp=true;else if(fault==1)wrong_id=true;else if(fault==2)matrices[1]={};else matrices[2].mat[3][0]=1e20;CHECK(run()==PF_Err_BAD_CALLBACK_PARAM);preserved();}
}
}
int main(){
    NeverCancelled never;const LayerUnits units{640,480,1};
    const std::array<MotionLayerPointRequest,3> requests{{{2,{}},{3,{}},{2,{480,0,240}}}};
    const auto run=[&](std::vector<MotionLayerPoint>& output){return capture_motion_layer_points(&data,units,requests,never,output);};
    std::vector<MotionLayerPoint> output;
    reset();CHECK(run(output)==0);CHECK(output.size()==3);CHECK(near(output[0].position,{0,0,-.5}));CHECK(near(output[1].position,{1,0,-.5}));CHECK(near(output[2].position,{1,0,0}));
    CHECK(output[0].layer_id==2 && output[1].layer_id==3 && output[2].layer_id==2);CHECK(matrix_calls[1]==1 && matrix_calls[2]==1 && matrix_calls[3]==1);CHECK(active==0 && acquired==released);
    const auto successful_calls=calls;
    reset();matrices[1].mat[0][0]=2;matrices[1].mat[1][0]=.5;matrices[1].mat[1][1]=3;matrices[1].mat[2][2]=-4;
    matrices[1].mat[3][0]=100;matrices[1].mat[3][1]=200;matrices[1].mat[3][2]=300;
    matrices[2]={};matrices[2].mat[0][0]=.5;matrices[2].mat[0][1]=3;matrices[2].mat[1][0]=-2;matrices[2].mat[2][2]=-8;
    matrices[2].mat[3][0]=860;matrices[2].mat[3][1]=920;matrices[2].mat[3][2]=300;matrices[2].mat[3][3]=1;
    const std::array<MotionLayerPointRequest,2> transformed{{{2,{480,0,120}},{1,{320,240,0}}}};
    CHECK(capture_motion_layer_points(&data,{640,480,2},transformed,never,output)==0);CHECK(near(output[0].position,{0,-1,0}));CHECK(near(output[1].position,{0,0,-.5},0));CHECK(matrix_calls[1]==1);
    for(A_long time:{48,0,24,-24,48}){reset();time_offset=3;time_multiplier=2;animate_x=10;data.current_time=time;
        CHECK(run(output)==0);CHECK(near(output[0].position,{(2*static_cast<double>(time)/24+3)*10/480,0,-.5}));CHECK(active==0);}
    for(unsigned fault=1;fault<=successful_calls;++fault){reset();output={{999,{1,2,3}}};fail_at=fault;CHECK(run(output)==77);preserved(output);}
    for(unsigned fault=1;fault<=2;++fault){reset();output={{999,{1,2,3}}};release_fail=fault;CHECK(run(output)==88);preserved(output);}
    for(unsigned mode=1;mode<=2;++mode){reset();output={{999,{1,2,3}}};null_suite=mode;CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    for(unsigned mode=0;mode<6;++mode){reset();output={{999,{1,2,3}}};
        if(mode==0)pf.AEGP_GetEffectLayer=nullptr;if(mode==1)pf.AEGP_ConvertEffectToCompTime=nullptr;
        if(mode==2)layers.AEGP_GetLayerParentComp=nullptr;if(mode==3)layers.AEGP_GetLayerFromLayerID=nullptr;
        if(mode==4)layers.AEGP_GetLayerID=nullptr;if(mode==5)layers.AEGP_GetLayerToWorldXform=nullptr;
        CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    for(unsigned mode=0;mode<12;++mode){reset();output={{999,{1,2,3}}};
        if(mode==0)missing_owner=true;if(mode==1)missing_comp=true;if(mode==2)missing_source=true;if(mode==3)wrong_id=true;if(mode==4)wrong_comp=true;if(mode==5)zero_time=true;
        if(mode==6)matrices[1]={};if(mode==7)matrices[2].mat[0][3]=.5;
        if(mode==8)matrices[2].mat[0][0]=std::numeric_limits<double>::infinity();if(mode==9)matrices[2].mat[3][0]=1e20;
        if(mode==10)matrices[2].mat[0][0]=1e308;if(mode==11)matrices[1].mat[2][2]=1e-20;
        CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    const double invalid=std::numeric_limits<double>::quiet_NaN();
    for(LayerUnits bad:std::array<LayerUnits,5>{{{invalid,480,1},{640,0,1},{640,480,-1},{640,480,invalid},{640,1e-320,1}}}){
        reset();output={{999,{1,2,3}}};CHECK(capture_motion_layer_points(&data,bad,requests,never,output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);}
    for(MotionLayerPointRequest bad:std::array<MotionLayerPointRequest,4>{{{0,{}},{0x80000000,{}},{2,{invalid,0,0}},{2,{1e9+1,0,0}}}}){
        reset();output={{999,{1,2,3}}};CHECK(capture_motion_layer_points(&data,units,std::span(&bad,1),never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(calls==0);preserved(output);}
    reset();output={{999,{1,2,3}}};std::array<MotionLayerPointRequest,kMaxMotionPointRequests+1> excessive{};
    CHECK(capture_motion_layer_points(&data,units,excessive,never,output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(calls==0);preserved(output);
    for(unsigned mode=0;mode<5;++mode){reset();output={{999,{1,2,3}}};if(mode==0)data.pica_basicP=nullptr;if(mode==1)basic.AcquireSuite=nullptr;if(mode==2)basic.ReleaseSuite=nullptr;if(mode==3)data.effect_ref=nullptr;if(mode==4)data.time_scale=0;
        CHECK(run(output)==PF_Err_BAD_CALLBACK_PARAM);CHECK(calls==0);preserved(output);}
    reset();output={{999,{1,2,3}}};CHECK(capture_motion_layer_points(nullptr,units,requests,never,output)==PF_Err_BAD_CALLBACK_PARAM);preserved(output);
    reset();output={{999,{1,2,3}}};CHECK(capture_motion_layer_points(&data,units,{},never,output)==0);CHECK(output.empty());CHECK(calls==0);
    for(unsigned limit=0;limit<=requests.size()+2;++limit){reset();output={{999,{1,2,3}}};CancelAfter cancel(limit);
        CHECK(capture_motion_layer_points(&data,units,requests,cancel,output)==PF_Interrupt_CANCEL);preserved(output);}
    reset();output={{999,{1,2,3}}};fail_allocation=true;CHECK(run(output)==PF_Err_OUT_OF_MEMORY);CHECK(!fail_allocation);preserved(output);
    reset();std::array<MotionLayerPointRequest,kMaxMotionPointRequests> maximum{};for(unsigned i=0;i<maximum.size();++i)maximum[i]={i+2,{}};
    CHECK(capture_motion_layer_points(&data,units,maximum,never,output)==0);CHECK(output.size()==maximum.size());
    for(unsigned i=1;i<258;++i)CHECK(matrix_calls[i]==1);CHECK(active==0);
    timed_points();std::cout<<"motion_point_capture_tests: "<<checks<<" checks passed; current/mapped-time fake May2023 callbacks, no AE qualification\n";
}
