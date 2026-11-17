#pragma once
// Shared fake May2023 SDK; no host process or product transaction algorithm.
#include "EffectGraphBackup.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
#include "ModelGraphTransaction.hpp"
#include "starfield/core/ModelResources.hpp"
#endif
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>

using namespace starfield::adapter;
namespace {
unsigned checks{},suites{},effect_refs{},stream_refs{},values{},handles{},locks{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" at "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
constexpr A_Err injected=516;
struct Effect {
    int kind{-1};AEGP_EffectFlags flags{AEGP_EffectFlags_ACTIVE|AEGP_EffectFlags_AUDIO_TOO};
    std::u16string name;std::map<A_long,double> numbers;
    std::map<A_long,std::vector<std::pair<double,double>>> animation;
    std::map<A_long,std::string> expressions;
    std::map<A_long,std::vector<std::uint8_t>> arbitrary;
    bool operator==(const Effect&) const=default;
};
using Ptr=std::shared_ptr<Effect>;
struct EffectRef {Ptr effect;};
struct StreamRef {int type;Ptr effect;A_long index{};}; // -3 root, -2 parade, -1 effect group
struct Memory {std::vector<A_UTF16Char> data;bool locked{};};
std::vector<Ptr> parade;
std::string failure,missing_suite;unsigned failure_at=1,seen{};bool silent{},duplicate_at_end{},duplicate_error_with_ref{},duplicate_wrong_kind{},wrong_type{};
bool cleanup_delete_error{},cleanup_delete_silent{},cleanup_delete_once{};unsigned cleanup_delete_calls{};
constexpr A_Err first_cleanup_error=515;
std::map<std::string,unsigned> calls;
std::vector<std::pair<int,A_long>> releases;
AEGP_EffectSuite4 effects{};AEGP_StreamSuite6 streams{};AEGP_DynamicStreamSuite4 dynamic{};AEGP_MemorySuite1 memory{};SPBasicSuite basic{};
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
AEGP_UtilitySuite6 utility{};
#endif
using UUID=std::array<std::uint8_t,16>;
const UUID operation{1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
AEGP_LayerH layer(){return reinterpret_cast<AEGP_LayerH>(1);}
A_long guard(int kind){return kind<0?kGraphSyncGuardId:native_nodes::sync_guard_index(static_cast<native_nodes::Kind>(kind));}
A_long first(int kind){return native_nodes::uuid_first_index(static_cast<native_nodes::Kind>(kind));}
Ptr effect(AEGP_EffectRefH h){CHECK(h!=nullptr);return reinterpret_cast<EffectRef*>(h)->effect;}
StreamRef& stream(AEGP_StreamRefH h){CHECK(h!=nullptr);return *reinterpret_cast<StreamRef*>(h);}
AEGP_EffectRefH effect_ref(const Ptr& p){++effect_refs;return reinterpret_cast<AEGP_EffectRefH>(new EffectRef{p});}
AEGP_StreamRefH ref(int type,const Ptr& p={},A_long index=0){++stream_refs;return reinterpret_cast<AEGP_StreamRefH>(new StreamRef{type,p,index});}
int fault(const char* name){++calls[name];return failure==name && ++seen==failure_at?(silent?2:1):0;}
void inject(const char* name,unsigned at=1,bool no_change=false){failure=name;failure_at=at;seen=0;silent=no_change;}
void no_refs(){CHECK(effect_refs==0);CHECK(stream_refs==0);CHECK(values==0);CHECK(handles==0);CHECK(locks==0);}
void no_owned(){no_refs();CHECK(suites==0);}
void uuid(Effect& e,const UUID& id){for(A_long i=0;i<8;++i)e.numbers[first(e.kind)+i]=(unsigned(id[i*2])<<8)|id[i*2+1];}
UUID uuid(const Effect& e){UUID id{};for(A_long i=0;i<8;++i){const auto v=static_cast<unsigned>(e.numbers.at(first(e.kind)+i));id[i*2]=static_cast<std::uint8_t>(v>>8);id[i*2+1]=static_cast<std::uint8_t>(v);}return id;}
std::vector<Effect> snapshot(){std::vector<Effect> result;for(const auto& p:parade)result.push_back(*p);return result;}
void equals(const std::vector<Effect>& saved){CHECK(parade.size()==saved.size());for(std::size_t i=0;i<saved.size();++i)CHECK(*parade[i]==saved[i]);}
Ptr make(int kind,unsigned id){auto p=std::make_shared<Effect>();p->kind=kind;p->name=u"Node \u6a21\u578b ";p->name+=static_cast<char16_t>('A'+id);
    if(kind>=-1){p->numbers[guard(kind)]=0;if(kind>=0){UUID identity{};for(std::size_t i=0;i<identity.size();++i)identity[i]=static_cast<std::uint8_t>(31+id+i);uuid(*p,identity);}}
    p->numbers[1]=kind==5?1:id+0.25;
    if(kind==5){p->numbers[4]=0;for(A_long axis=0;axis<6;++axis)p->numbers[95+axis]=axis<3?-.5:.5;}
    p->animation[2]={{0,12.25},{0.125,-23.5},{70,999.9}};
    p->expressions[3]="thisComp.layer(1).effect('Node')(2) + time";
    p->arbitrary[4]={0,1,2,3,255,254,253,0};p->arbitrary[5]=std::vector<std::uint8_t>(257,static_cast<std::uint8_t>(id));return p;}
void reset(){no_owned();parade.clear();failure.clear();missing_suite.clear();calls.clear();releases.clear();seen=0;failure_at=1;silent=duplicate_at_end=duplicate_error_with_ref=duplicate_wrong_kind=wrong_type=false;
    cleanup_delete_error=cleanup_delete_silent=cleanup_delete_once=false;cleanup_delete_calls=0;
    for(const auto kind:{-2,-1,5,-2,4,1,0,3})parade.push_back(make(kind,static_cast<unsigned>(parade.size()+1)));}
void initialize(){
    effects.AEGP_GetLayerNumEffects=[](AEGP_LayerH h,A_long* out)->A_Err{CHECK(h==layer());if(fault("count")==1)return injected;*out=static_cast<A_long>(parade.size());return 0;};
    effects.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID id,AEGP_LayerH h,A_long index,AEGP_EffectRefH* out)->A_Err{
        CHECK(id==7 && h==layer());if(fault("effect-ref")==1)return injected;CHECK(index>=0 && static_cast<std::size_t>(index)<parade.size());*out=effect_ref(parade[index]);return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH h)->A_Err{delete reinterpret_cast<EffectRef*>(h);CHECK(effect_refs>0);--effect_refs;return 0;};
    effects.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH h,AEGP_InstalledEffectKey* out)->A_Err{if(fault("key")==1)return injected;*out=static_cast<AEGP_InstalledEffectKey>(effect(h)->kind+3);return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err{
        if(fault("match")==1)return injected;const std::map<int,const char*> names{{-2,"third.party.blur"},{-1,"org.starfieldfx.particle"},{0,"org.starfieldfx.node.emitter"},
            {1,"org.starfieldfx.node.particle"},{3,"org.starfieldfx.node.force"},{4,"org.starfieldfx.node.transform"},{5,"org.starfieldfx.node.model"}};
        const auto found=names.find(static_cast<int>(key)-3);CHECK(found!=names.end());std::strcpy(out,found->second);return 0;};
    effects.AEGP_GetEffectFlags=[](AEGP_EffectRefH h,AEGP_EffectFlags* out)->A_Err{if(fault("flags-read")==1)return injected;*out=effect(h)->flags;return 0;};
    effects.AEGP_SetEffectFlags=[](AEGP_EffectRefH h,AEGP_EffectFlags mask,AEGP_EffectFlags flags)->A_Err{
        const auto mode=fault("flags-write");if(mode==1)return injected;auto p=effect(h);if(mode!=2)p->flags=(p->flags&~mask)|(flags&mask);return 0;};
    effects.AEGP_DuplicateEffect=[](AEGP_EffectRefH h,AEGP_EffectRefH* out)->A_Err{
        CHECK(stream_refs==0 && values==0 && handles==0 && locks==0);const auto mode=fault("duplicate");if(mode==1 && !duplicate_error_with_ref)return injected;
        const auto p=effect(h);auto copy=std::make_shared<Effect>(*p);if(duplicate_wrong_kind)copy->kind=-2;
        const auto slot=std::find(parade.begin(),parade.end(),p);CHECK(slot!=parade.end());parade.insert(duplicate_at_end?parade.end():slot+1,copy);*out=effect_ref(copy);return mode==1?injected:0;};
    effects.AEGP_DeleteLayerEffect=[](AEGP_EffectRefH h)->A_Err{
        CHECK(stream_refs==0 && values==0 && handles==0 && locks==0);const auto mode=fault("delete");if(mode==1)return injected;
        if((cleanup_delete_error || cleanup_delete_silent) && (!cleanup_delete_once || cleanup_delete_calls==0)){
            ++cleanup_delete_calls;if(cleanup_delete_silent)return 0;
            return cleanup_delete_calls==1?first_cleanup_error:injected;
        }
        const auto found=std::find(parade.begin(),parade.end(),effect(h));CHECK(found!=parade.end());if(mode!=2)parade.erase(found);return 0;};
    effects.AEGP_ReorderEffect=[](AEGP_EffectRefH h,A_long index)->A_Err{
        CHECK(stream_refs==0 && values==0 && handles==0 && locks==0);const auto mode=fault("reorder");if(mode==1)return injected;
        const auto p=effect(h);const auto found=std::find(parade.begin(),parade.end(),p);CHECK(found!=parade.end());CHECK(index>=0 && static_cast<std::size_t>(index)<parade.size());
        if(mode!=2){parade.erase(found);parade.insert(parade.begin()+index,p);}return 0;};
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID id,AEGP_EffectRefH h,A_long index,AEGP_StreamRefH* out)->A_Err{
        CHECK(id==7);if(fault("param-ref")==1)return injected;CHECK(effect(h)->numbers.contains(index));*out=ref(0,effect(h),index);return 0;};
    streams.AEGP_GetStreamType=[](AEGP_StreamRefH h,AEGP_StreamType* out)->A_Err{CHECK(stream(h).type==0);if(fault("param-type")==1)return injected;*out=wrong_type?AEGP_StreamType_ARB:AEGP_StreamType_OneD;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID id,AEGP_StreamRefH h,AEGP_LTimeMode mode,const A_Time* time,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
        bool metadata_time=time->value==0 && time->scale==1;
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
        metadata_time=metadata_time || (time->value==12 && time->scale==24);
#endif
        CHECK(id==7 && mode==AEGP_LTimeMode_LayerTime && metadata_time && pre==TRUE);if(fault("scalar-read")==1)return injected;
        auto& s=stream(h);out->streamH=h;out->val.one_d=s.effect->numbers.at(s.index);++values;return 0;};
    streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2* v)->A_Err{CHECK(v->streamH && values>0);--values;v->streamH=nullptr;return 0;};
    streams.AEGP_SetStreamValue=[](AEGP_PluginID id,AEGP_StreamRefH h,AEGP_StreamValue2* v)->A_Err{
        CHECK(id==7 && v->streamH==h);const auto mode=fault("scalar-write");if(mode==1)return injected;auto& s=stream(h);
        if(mode!=2)s.effect->numbers[s.index]=v->val.one_d;
        if(s.index==guard(s.effect->kind) && v->val.one_d==0)releases.push_back({s.effect->kind,s.index});return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH h)->A_Err{delete reinterpret_cast<StreamRef*>(h);CHECK(stream_refs>0);--stream_refs;return 0;};
    streams.AEGP_GetStreamName=[](AEGP_PluginID id,AEGP_StreamRefH h,A_Boolean english,AEGP_MemHandle* out)->A_Err{
        CHECK(id==7 && english==FALSE && stream(h).type==-1);if(fault("name-read")==1)return injected;auto* m=new Memory;
        for(auto c:stream(h).effect->name)m->data.push_back(static_cast<A_UTF16Char>(c));m->data.push_back(0);++handles;*out=reinterpret_cast<AEGP_MemHandle>(m);return 0;};
    dynamic.AEGP_GetNewStreamRefForLayer=[](AEGP_PluginID id,AEGP_LayerH h,AEGP_StreamRefH* out)->A_Err{CHECK(id==7 && h==layer());if(fault("root-ref")==1)return injected;*out=ref(-3);return 0;};
    dynamic.AEGP_GetNewStreamRefByMatchname=[](AEGP_PluginID id,AEGP_StreamRefH h,const A_char* name,AEGP_StreamRefH* out)->A_Err{
        CHECK(id==7 && stream(h).type==-3 && !std::strcmp(name,"ADBE Effect Parade"));if(fault("parade-ref")==1)return injected;*out=ref(-2);return 0;};
    dynamic.AEGP_GetNewStreamRefByIndex=[](AEGP_PluginID id,AEGP_StreamRefH h,A_long index,AEGP_StreamRefH* out)->A_Err{
        CHECK(id==7 && stream(h).type==-2);if(fault("group-ref")==1)return injected;CHECK(index>=0 && static_cast<std::size_t>(index)<parade.size());*out=ref(-1,parade[index]);return 0;};
    dynamic.AEGP_SetStreamName=[](AEGP_StreamRefH h,const A_UTF16Char* text)->A_Err{
        CHECK(stream(h).type==-1);const auto mode=fault("name-write");if(mode==1)return injected;if(mode!=2){auto& name=stream(h).effect->name;name.clear();while(*text)name+=static_cast<char16_t>(*text++);}return 0;};
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle h,AEGP_MemSize* out)->A_Err{if(fault("mem-size")==1)return injected;*out=static_cast<AEGP_MemSize>(reinterpret_cast<Memory*>(h)->data.size()*sizeof(A_UTF16Char));return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle h,void** out)->A_Err{if(fault("mem-lock")==1)return injected;auto* m=reinterpret_cast<Memory*>(h);CHECK(!m->locked);m->locked=true;++locks;*out=m->data.data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle h)->A_Err{auto* m=reinterpret_cast<Memory*>(h);CHECK(m->locked);m->locked=false;CHECK(locks>0);--locks;return fault("mem-unlock")==1?injected:0;};
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle h)->A_Err{auto* m=reinterpret_cast<Memory*>(h);CHECK(!m->locked);delete m;CHECK(handles>0);--handles;return 0;};
    basic.AcquireSuite=[](const char* name,int32 version,const void** out)->A_Err{
        if(missing_suite==name){*out=nullptr;return injected;}
        const std::map<std::string,std::pair<int32,const void*>> table{
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
            {kAEGPUtilitySuite,{kAEGPUtilitySuiteVersion6,&utility}},
#endif
            {kAEGPEffectSuite,{kAEGPEffectSuiteVersion4,&effects}},{kAEGPStreamSuite,{kAEGPStreamSuiteVersion6,&streams}},
            {kAEGPDynamicStreamSuite,{kAEGPDynamicStreamSuiteVersion4,&dynamic}},{kAEGPMemorySuite,{kAEGPMemorySuiteVersion1,&memory}}};
        const auto found=table.find(name);CHECK(found!=table.end() && found->second.first==version);*out=found->second.second;++suites;return 0;};
    basic.ReleaseSuite=[](const char*,int32)->A_Err{CHECK(suites>0);--suites;return 0;};
}
}
