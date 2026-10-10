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
void mutate(){
    // Full author edits, deleted nodes and newly created nodes after backup.
    parade.erase(std::remove_if(parade.begin(),parade.end(),[](const Ptr& p){return p->kind>=0 && p->numbers.at(guard(p->kind))!=2;}),parade.end());
    for(const auto& p:parade)if(p->kind==-1 && p->numbers.at(guard(-1))!=2){p->name=u"Changed renderer";p->animation.clear();p->arbitrary.clear();}
    parade.push_back(make(5,61));parade.push_back(make(4,62));
}
void success_tests(){
    for(bool at_end:{false,true}){reset();duplicate_at_end=at_end;const auto saved=snapshot();const auto third_a=parade[0],third_b=parade[3];
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);no_refs();CHECK(parade.size()==14);CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==1);
            std::vector<UUID> ids;unsigned copies{};
            for(const auto& p:parade)if(p->kind>=-1 && p->numbers.at(guard(p->kind))==2){++copies;CHECK(!(p->flags&AEGP_EffectFlags_ACTIVE));
                auto observed=*p;observed.flags|=AEGP_EffectFlags_ACTIVE;observed.numbers[guard(p->kind)]=0;
                const auto found=std::find_if(saved.begin(),saved.end(),[&](const Effect& old){return old.kind==p->kind;});CHECK(found!=saved.end());
                if(p->kind>=0){const auto id=uuid(*p);CHECK(id!=uuid(*found));CHECK(std::find(ids.begin(),ids.end(),id)==ids.end());ids.push_back(id);uuid(observed,uuid(*found));}
                CHECK(observed==*found);p->name=u"Renamed backup";}
            CHECK(copies==6);mutate();releases.clear();CHECK(backup.restore()==0);no_refs();CHECK(backup.rollback_error()==0);equals(saved);
            CHECK(parade[0]==third_a && parade[3]==third_b);CHECK(releases.size()==6 && releases.back().first==-1);CHECK(backup.restore()!=0);}
        no_owned();
    }
    reset();const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);mutate();}equals(saved);no_owned();
    reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);parade[1]->arbitrary[99]={9,9,9};const auto original=parade[1];
        CHECK(backup.discard()==0);CHECK(parade.size()==8 && parade[1]==original);CHECK(parade[1]->arbitrary.contains(99));CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==0);no_refs();}
    no_owned();
    reset();UUID collision=operation;collision[0]^=0x80;collision.back()^=1;
    {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation,std::span<const UUID>(&collision,1))==0);
        for(const auto& p:parade)if(p->kind>=0 && p->numbers.at(guard(p->kind))==2)CHECK(uuid(*p)!=collision);CHECK(backup.discard()==0);}no_owned();
    reset();UUID edge{};edge[0]=0x80;edge.back()=0xfe;
    {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(edge)==0);for(const auto& p:parade)if(p->kind>=0 && p->numbers.at(guard(p->kind))==2){UUID output{};output.back()=0xff;CHECK(uuid(*p)!=output);CHECK(uuid(*p)!=UUID{});}CHECK(backup.discard()==0);}no_owned();
}
void preflight_tests(){
    for(const char* missing:{kAEGPEffectSuite,kAEGPStreamSuite,kAEGPDynamicStreamSuite,kAEGPMemorySuite}){reset();missing_suite=missing;const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(saved);no_owned();}
    for(unsigned case_id=0;case_id<10;++case_id){reset();
        switch(case_id){case 0:parade[2]->numbers[guard(5)]=1;break;case 1:parade[2]->numbers[guard(5)]=2;break;
            case 2:uuid(*parade[2],{});break;case 3:uuid(*parade[4],uuid(*parade[2]));break;case 4:parade.push_back(make(-1,9));break;
            case 5:parade.erase(parade.begin()+1);break;case 6:parade[2]->name=std::u16string(512,u'x');break;
            case 7:parade[2]->numbers[first(5)]=0.5;break;case 8:parade[2]->numbers[first(5)]=65536;break;case 9:wrong_type=true;break;}
        const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);CHECK(calls["scalar-write"]==0 && calls["duplicate"]==0);no_owned();}
    reset();const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare({})!=0);}equals(saved);no_owned();
    reset();for(unsigned id=0;id<60;++id)parade.push_back(make(5,100+id));const auto many=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(many);CHECK(calls["duplicate"]==0);no_owned();
    for(const char* op:{"count","effect-ref","key","match","flags-read","param-ref","param-type","scalar-read","root-ref","parade-ref","group-ref","name-read","mem-size","mem-lock","mem-unlock"}){
        reset();const auto old=snapshot();inject(op);{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);}equals(old);CHECK(calls["scalar-write"]==0);no_owned();}
}
void prepare_failure_tests(){
    reset();unsigned writes{};{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);writes=calls["scalar-write"];CHECK(backup.discard()==0);}no_owned();
    for(bool no_change:{false,true})for(unsigned at=1;at<=writes;++at){reset();const auto saved=snapshot();inject("scalar-write",at,no_change);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    for(bool no_change:{false,true})for(unsigned at=1;at<=6;++at){reset();const auto saved=snapshot();inject("flags-write",at,no_change);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    for(bool with_ref:{false,true})for(unsigned at=1;at<=6;++at){reset();const auto saved=snapshot();duplicate_error_with_ref=with_ref;inject("duplicate",at);
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==injected);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();}
    reset();duplicate_wrong_kind=true;const auto saved=snapshot();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)!=0);CHECK(backup.rollback_error()==0);}equals(saved);no_owned();
}
void restore_failure_tests(){
    for(const char* op:{"delete","reorder","name-write","flags-write","scalar-write"})for(bool no_change:{false,true}){
        reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);mutate();
            // Ensure reorder and flags writes change a value, so a silent failure is observable.
            std::reverse(parade.begin(),parade.end());for(const auto& p:parade)if(p->kind>=-1 && p->numbers.at(guard(p->kind))==2)p->name=u"Changed backup";
            inject(op,1,no_change);CHECK(backup.restore()!=0);CHECK(backup.rollback_error()!=0);no_refs();}no_owned();}
    for(bool no_change:{false,true}){reset();{EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);parade[1]->arbitrary[99]={99};inject("delete",1,no_change);
        CHECK(backup.discard()!=0);CHECK(parade[1]->arbitrary.contains(99));CHECK(parade[1]->numbers.at(kGraphSyncGuardId)==0);
        CHECK(std::count_if(parade.begin(),parade.end(),[](const Ptr& p){return p->kind>=-1 && p->numbers.at(guard(p->kind))==2;})==1);no_refs();}no_owned();}
}
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
struct Transaction {
    std::vector<std::uint8_t> bytes;
    std::vector<ModelAssetRestore> assets;
    std::vector<UUID> ids;
    unsigned starts{},ends{},depth{},prepares{},commits{},generics{},written{};
    unsigned cancel_phase{},generic_fail_at{};
    bool start_fail{},end_fail{},prepare_fail{},commit_fail{},prepare_throw{},commit_throw{},unacknowledged{},local_failure{},
        wrong_target{},duplicate_target{},busy_target{},silent_revision{};
    ModelGraphTransactionPlan plan() {ModelGraphTransactionPlan p;p.basic=&basic;p.plugin=7;p.layer=layer();p.time={12,24};p.id=operation;p.desired_ids=ids;p.assets=assets;p.context=this;
        p.prepare=[](void* context)->A_Err{auto& t=*static_cast<Transaction*>(context);++t.prepares;CHECK(t.depth==1);no_refs();
            // The actual gateway callback will do these structural/ordinary edits.
            // This fixture exercises its executor contract without claiming AE script qualification.
            parade.erase(std::remove_if(parade.begin(),parade.end(),[](const Ptr& e){return e->kind>=0 && e->numbers.at(guard(e->kind))!=2;}),parade.end());
            for(const auto& asset:t.assets){auto p=make(5,61);auto target=asset.node_id;if(t.wrong_target)target[0]^=1;uuid(*p,target);if(t.busy_target)p->numbers[94]=1;parade.push_back(p);
                if(t.duplicate_target)parade.push_back(std::make_shared<Effect>(*p));}
            if(t.prepare_throw)throw std::bad_alloc();return t.prepare_fail?injected:0;};
        p.commit=[](void* context)->A_Err{auto& t=*static_cast<Transaction*>(context);++t.commits;CHECK(t.depth==1);no_refs();
            if(t.commit_throw)throw std::runtime_error("commit exception");return t.commit_fail?injected:0;};
        p.is_cancelled=[](void* context) noexcept->std::int32_t{const auto& t=*static_cast<Transaction*>(context);
            return t.cancel_phase==1 || (t.cancel_phase==2 && t.prepares) || (t.cancel_phase==3 && t.written>=1) || (t.cancel_phase==4 && t.written==t.assets.size());};return p;}
};
Transaction* transaction{};
Transaction make_transaction(unsigned assets=2){reset();Transaction t;
    starfield::core::NeverCancelled never;auto cube=starfield::core::make_unit_cube();CHECK(cube.has_value());
    auto encoded=starfield::core::encode_model_geometry(cube.value(),never);CHECK(encoded.has_value());
    for(auto b:encoded.value())t.bytes.push_back(std::to_integer<std::uint8_t>(b));
    for(unsigned i=0;i<assets;++i){UUID id{};id.back()=static_cast<std::uint8_t>(100+i);t.ids.push_back(id);t.assets.push_back({id,2,27+i,{},{-.5,-.5,-.5,.5,.5,.5}});}return t;}
ModelGraphTransactionResult run(Transaction& t){transaction=&t;for(auto& asset:t.assets)if(asset.revision)asset.mesh=t.bytes;const auto result=apply_model_graph_transaction(t.plan());no_owned();CHECK(t.depth==0 && t.starts==t.ends);return result;}
void transaction_tests(){
    utility.AEGP_StartUndoGroup=[](const A_char* name)->A_Err{CHECK(!std::strcmp(name,"Starfield: apply graph and Model assets"));auto& t=*transaction;no_refs();CHECK(t.depth==0);
        if(t.start_fail)return injected;++t.starts;++t.depth;return 0;};
    utility.AEGP_EndUndoGroup=[]()->A_Err{auto& t=*transaction;no_refs();CHECK(t.depth==1);--t.depth;++t.ends;return t.end_fail?injected:0;};
    effects.AEGP_EffectCallGeneric=[](AEGP_PluginID plugin,AEGP_EffectRefH h,const A_Time* time,PF_Cmd command,void* extra)->A_Err{
        auto& t=*transaction;++t.generics;CHECK(plugin==7 && time->value==12 && time->scale==24 && command==PF_Cmd_COMPLETELY_GENERAL && t.depth==1);
        auto& r=*static_cast<ModelAssetWriteRequest*>(extra);CHECK(r.magic==0x53464d57 && r.version==1 && r.bytes==sizeof(r) && r.operation==1);
        auto p=effect(h);CHECK(p->kind==5 && p->numbers.at(94)==0 && effect_refs==1 && stream_refs==0 && values==0 && handles==0 && locks==0);
        UUID expected{};std::copy(std::begin(r.expected_uuid),std::end(r.expected_uuid),expected.begin());CHECK(uuid(*p)==expected);
        CHECK(r.expected_source==p->numbers.at(1) && r.expected_revision==p->numbers.at(4));CHECK(!r.is_cancelled(r.cancellation_context));
        if(t.generic_fail_at==t.generics)return injected;
        if(t.unacknowledged)return 0;r.acknowledged=1;
        p->arbitrary[3]=r.mesh_length?std::vector<std::uint8_t>(r.mesh_bytes,r.mesh_bytes+r.mesh_length):std::vector<std::uint8_t>{};
        p->numbers[1]=r.desired_source;if(!t.silent_revision)p->numbers[4]=r.desired_revision;
        for(unsigned i=0;i<6;++i)p->numbers[95+i]=r.desired_bounds[i];
        if(t.local_failure){r.error=ModelAssetError::host_error;r.host_error=injected;r.rollback_error=512;return 0;}
        r.error=ModelAssetError::none;++t.written;return 0;};
    {auto t=make_transaction();auto r=run(t);CHECK(r.committed && r.error==0 && r.cleanup_error==0 && r.undo_error==0 && r.rollback_error==0 &&
        r.stage==ModelTransactionStage::complete && t.prepares==1 && t.commits==1 && t.generics==2);
        CHECK(parade.size()==5);for(const auto& a:t.assets){const auto p=std::find_if(parade.begin(),parade.end(),[&](const Ptr& e){return e->kind==5 && uuid(*e)==a.node_id;});
            CHECK(p!=parade.end() && (*p)->arbitrary.at(3)==t.bytes && (*p)->numbers.at(1)==2 && (*p)->numbers.at(4)==a.revision);}}
    {auto t=make_transaction(1);t.assets[0].source=1;t.assets[0].revision=0;t.bytes.clear();auto r=run(t);CHECK(r.committed && t.generics==1);}
    for(unsigned which=0;which<12;++which){auto t=make_transaction();const auto before=snapshot();
        switch(which){case 0:t.prepare_fail=true;break;case 1:t.commit_fail=true;break;case 2:t.prepare_throw=true;break;case 3:t.commit_throw=true;break;
            case 4:t.generic_fail_at=1;break;case 5:t.generic_fail_at=2;break;case 6:t.unacknowledged=true;break;case 7:t.wrong_target=true;break;
            case 8:t.duplicate_target=true;break;case 9:t.busy_target=true;break;case 10:t.silent_revision=true;break;case 11:t.local_failure=true;break;}
        const auto r=run(t);CHECK(!r.committed && r.error!=0 && r.rollback_error==0);equals(before);CHECK(t.starts==1 && t.ends==1);
        if(which==11)CHECK(r.asset_rollback_error==512);}
    for(unsigned phase=1;phase<=4;++phase){auto t=make_transaction();const auto before=snapshot();t.cancel_phase=phase;const auto r=run(t);
        CHECK(!r.committed && r.error==PF_Interrupt_CANCEL && r.rollback_error==0);equals(before);if(phase==1)CHECK(t.starts==0);}
    for(unsigned which=0;which<9;++which){auto t=make_transaction();const auto before=snapshot();
        switch(which){case 0:t.assets[1].node_id=t.assets[0].node_id;break;case 1:t.assets[1].source=3;break;case 2:t.assets[1].revision=2147483648u;break;
            case 3:t.assets[1].bounds[0]=-2;break;case 4:t.bytes[0]=0;break;case 5:t.assets[1].node_id={};break;
            case 6:t.ids[1]=t.ids[0];break;case 7:t.assets[1].bounds[2]=std::numeric_limits<double>::infinity();break;case 8:t.assets[1].node_id.back()=99;break;}
        const auto r=run(t);CHECK(!r.committed && r.error!=0 && t.starts==0 && t.prepares==0 && t.generics==0);equals(before);}
    {auto t=make_transaction(9);transaction=&t;const auto before=snapshot();for(auto& asset:t.assets)asset.mesh={reinterpret_cast<const std::uint8_t*>(1),8*1024*1024};
        auto r=apply_model_graph_transaction(t.plan());CHECK(r.error!=0 && t.starts==0);equals(before);no_owned();}
    {auto t=make_transaction();t.start_fail=true;const auto before=snapshot();const auto r=run(t);CHECK(r.error==injected && t.prepares==0);equals(before);}
    {auto t=make_transaction();missing_suite=kAEGPUtilitySuite;const auto before=snapshot();const auto r=run(t);CHECK(r.error!=0 && t.starts==0);equals(before);}
    {auto t=make_transaction();inject("duplicate",2);const auto before=snapshot();const auto r=run(t);CHECK(r.error==injected && t.prepares==0 && r.rollback_error==0);equals(before);}
    {auto t=make_transaction();t.commit_fail=true;inject("delete",1);const auto r=run(t);CHECK(!r.committed && r.error==injected && r.rollback_error==injected);}
    {auto t=make_transaction();inject("delete",1);const auto r=run(t);CHECK(r.committed && r.error==0 && r.cleanup_error==injected && t.commits==1);}
    {auto t=make_transaction();t.end_fail=true;const auto r=run(t);CHECK(r.committed && r.error==0 && r.undo_error==injected);}
    transaction=nullptr;
}
#endif
}
int main(){initialize();success_tests();preflight_tests();prepare_failure_tests();restore_failure_tests();
#ifdef STARFIELD_MODEL_GRAPH_TRANSACTION_TEST
    transaction_tests();std::cout<<"Model whole-graph transaction: "<<checks<<" checks, 0 failures (actual backup/SFMG1, fake May2023 callbacks)\n";
#else
    std::cout<<"Complete effect graph backup: "<<checks<<" checks, 0 failures (May2023 SDK/fake host)\n";
#endif
    return 0;}
