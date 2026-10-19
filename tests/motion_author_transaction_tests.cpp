#include "MotionAuthorTransaction.hpp"
#include <cstdio>
#include <memory>
#include <numeric>

using namespace starfield::adapter;
namespace nodes=starfield::adapter::native_nodes;
namespace {
int checks{},failures{},suites{},live_effects{},live_streams{},peak_streams{},opens{},writes{},stale{};
int calls{},fail_call{-1},fail_write_index{-1};
bool discard_guard_restore{};
unsigned epoch{};
struct Source{A_long uuid{};double mode{3},guard{5};motion_prefix::Bank prefix{};};
std::vector<Source> sources;
std::vector<std::size_t> parade;
struct EffectRef{std::size_t source;std::shared_ptr<bool> alive{std::make_shared<bool>(true)};};
struct StreamRef{std::size_t source;A_long index;unsigned epoch;std::shared_ptr<bool> alive;};
AEGP_EffectSuite4 effects{};AEGP_StreamSuite6 streams{};SPBasicSuite basic{};
PF_InData data{};
void check(bool ok,const char* text){++checks;if(!ok){++failures;std::printf("FAIL: %s\n",text);}}
bool fail(){return ++calls==fail_call;}
bool valid(AEGP_StreamRefH handle){
    const auto& ref=*reinterpret_cast<StreamRef*>(handle);
    if(!*ref.alive || ref.epoch!=epoch){++stale;return false;}return true;
}
void initialize(std::size_t count){
    check(!suites && !live_effects && !live_streams,"previous transaction releases all SDK ownership");
    sources.assign(count,{});parade.resize(count);std::iota(parade.begin(),parade.end(),0);
    for(std::size_t i=0;i<count;++i)sources[i].uuid=static_cast<A_long>(i+1);
    calls=opens=writes=stale=peak_streams=0;fail_call=fail_write_index=-1;discard_guard_restore=false;++epoch;
}
void sdk(){
    effects.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* n)->A_Err{
        if(fail())return 1;*n=static_cast<A_long>(parade.size());return 0;};
    effects.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long index,AEGP_EffectRefH* out)->A_Err{
        if(fail())return 1;++opens;++live_effects;
        *out=reinterpret_cast<AEGP_EffectRefH>(new EffectRef{parade.at(index)});return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH h)->A_Err{
        auto* ref=reinterpret_cast<EffectRef*>(h);*ref->alive=false;delete ref;--live_effects;return 0;};
    effects.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH,AEGP_InstalledEffectKey* key)->A_Err{
        if(fail())return 1;*key=1;return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey,A_char* name)->A_Err{
        if(fail())return 1;std::strcpy(name,"org.starfieldfx.node.motion");return 0;};
    streams.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH h,A_long index,AEGP_StreamRefH* out)->A_Err{
        if(fail())return 1;auto* effect=reinterpret_cast<EffectRef*>(h);
        *out=reinterpret_cast<AEGP_StreamRefH>(new StreamRef{effect->source,index,epoch,effect->alive});
        peak_streams=std::max(peak_streams,++live_streams);return 0;};
    streams.AEGP_DisposeStream=[](AEGP_StreamRefH h)->A_Err{delete reinterpret_cast<StreamRef*>(h);--live_streams;return 0;};
    streams.AEGP_GetStreamType=[](AEGP_StreamRefH h,AEGP_StreamType* type)->A_Err{
        if(!valid(h) || fail())return 1;*type=AEGP_StreamType_OneD;return 0;};
    streams.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH h,AEGP_LTimeMode,const A_Time*,A_Boolean,AEGP_StreamValue2* out)->A_Err{
        if(!valid(h) || fail())return 1;auto& r=*reinterpret_cast<StreamRef*>(h);auto& s=sources[r.source];
        out->streamH=h;
        out->val.one_d=r.index==nodes::motion_layout::mode?s.mode:
            r.index>=143 && r.index<=186?s.prefix[r.index-143]:
            r.index==nodes::sync_guard_index(nodes::Kind::motion)?s.guard:
            r.index==nodes::uuid_first_index(nodes::Kind::motion)+7?s.uuid:0;
        return 0;};
    streams.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    streams.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH h,AEGP_StreamValue2* value)->A_Err{
        if(!valid(h) || fail())return 1;auto& r=*reinterpret_cast<StreamRef*>(h);auto& s=sources[r.source];
        ++writes;
        if(r.index==fail_write_index){fail_write_index=-1;return 1;}
        if(r.index==nodes::motion_layout::mode)s.mode=value->val.one_d;
        else if(r.index>=143 && r.index<=186)s.prefix[r.index-143]=value->val.one_d;
        else if(!(discard_guard_restore && value->val.one_d==5))s.guard=value->val.one_d;
        ++epoch; // Even another scalar write expires every old source stream.
        return 0;};
    basic.AcquireSuite=[](const char* name,int32,const void** out)->A_Err{
        if(fail())return 1;
        if(!std::strcmp(name,kAEGPEffectSuite))*out=&effects;
        else if(!std::strcmp(name,kAEGPStreamSuite))*out=&streams;
        else return 1;
        ++suites;return 0;};
    basic.ReleaseSuite=[](const char*,int32)->A_Err{--suites;return 0;};
    data.pica_basicP=&basic;data.current_time=0;data.time_scale=24;
}
AEGP_LayerH layer(){return reinterpret_cast<AEGP_LayerH>(1);}
void restored(){for(const auto& source:sources)check(source.mode==3 && source.guard==5,"rollback restores complete source before-image");}
void prefix_tests(){
    motion_prefix::Bank wanted{};std::wstring decoded;
    for(const auto text:{std::wstring{},std::wstring(L"Light_"),std::wstring(L"\u706f\u5149_\U0001f31f"),std::wstring(128,L'x')}){
        check(motion_prefix::encode(text,wanted) && motion_prefix::decode(wanted,decoded) && decoded==text,"UTF-8 bank roundtrip");
    }
    check(!motion_prefix::encode(std::wstring(129,L'x'),wanted),"overlength prefix rejected");
    check(!motion_prefix::encode(std::wstring(43,L'\u706f'),wanted),"UTF-8 byte limit enforced");
    check(!motion_prefix::encode(std::wstring(1,wchar_t(0xd800)),wanted),"unpaired surrogate rejected");
    wanted={};wanted[0]=1;wanted[1]=0xff;
    check(!motion_prefix::decode(wanted,decoded),"invalid UTF-8 rejected");
    wanted[1]=0;check(!motion_prefix::decode(wanted,decoded),"embedded zero rejected");
    check(motion_prefix::encode(L"Light_\u706f",wanted),"encode author prefix");
    node_sync::NativeEdit edit;edit.node_kind=7;edit.uuid[7]=1;edit.parameter_index=143;
    edit.motion_prefix_checked=true;edit.value[0]=wanted[0];edit.additional_count=44;
    for(unsigned i=0;i<44;++i){edit.additional_fields[i].index=143+i;edit.additional_fields[i].value[0]=wanted[i];}
    check(node_sync::valid_edit(edit),"complete prefix bank accepted");
    for(int fault=-1;fault<44;++fault){
        initialize(1);sources[0].guard=0;sources[0].mode=2;
        MotionAuthorTransaction transaction(&data,701);fail_write_index=fault<0?-1:143+fault;
        const auto error=transaction.prepare(layer(),&edit);
        if(fault<0){check(error==0 && sources[0].prefix==wanted && sources[0].guard==1,"prefix prepared under guard");
            check(transaction.accept()==0 && sources[0].prefix==wanted && sources[0].guard==0,"prefix committed");}
        else {check(error!=0,"each failed bank write rejects");check(transaction.rollback()==0,"partial prefix restored");
            check(sources[0].prefix==motion_prefix::Bank{} && sources[0].mode==2 && sources[0].guard==0,"old prefix and guard restored");}
        check(stale==0 && !live_effects && !live_streams,"prefix writes never reuse stale stream");
    }
    for(int fault=0;fault<4;++fault){
        initialize(fault==3?2:1);sources[0].guard=0;sources[0].mode=2;
        if(fault==0)sources[0].prefix=wanted;if(fault==1)sources[0].guard=1;if(fault==2)sources[0].mode=1;
        if(fault==3){sources[1]=sources[0];}
        MotionAuthorTransaction transaction(&data,701);
        check(transaction.prepare(layer(),&edit)!=0 && writes==0,"stale prefix, busy guard, Circle and duplicate identity reject before mutation");
    }
    initialize(1);sources[0].guard=0;sources[0].mode=2;
    {MotionAuthorTransaction transaction(&data,701);check(transaction.prepare(layer(),&edit)==0,"prepare publication rollback");}
    check(sources[0].prefix==motion_prefix::Bank{} && sources[0].guard==0,"unaccepted publication restores prefix");
}
}
int main(){
    sdk();node_sync::NativeEdit panel;panel.records_only=true;
    prefix_tests();
    for(const auto count:{1,8,128,512}){
        initialize(count);
        {
            MotionAuthorTransaction transaction(&data,701);
            check(transaction.prepare(layer(),&panel)==0,"prepare all requested Motion modes");
            check(!live_streams && !live_effects,"prepare retains only numeric state");
            std::reverse(parade.begin(),parade.end());++epoch;
            check(transaction.accept()==0,"accept resolves changed parade by UUID after global invalidation");
            for(const auto& source:sources)check(source.mode==1 && source.guard==0,"mode and source guard committed");
            check(peak_streams==1 && stale==0,"one local stream, no stale reads/writes");
            check(opens==5*count,"source resolution has linear effect-open cost across prepare and accept");
            std::printf("Motion batch %d: %d effect opens, peak %d stream refs\n",count,opens,peak_streams);
        }
    }
    initialize(512);
    {
        node_sync::NativeEdit unrelated;unrelated.node_kind=1;unrelated.parameter_index=1;
        MotionAuthorTransaction transaction(&data,701);
        check(transaction.prepare(layer(),&unrelated)==0 && transaction.accept()==0 && calls==0,
            "unrelated native control edit performs zero author SDK calls");
    }
    initialize(128);for(auto& s:sources)s.guard=0;
    {
        MotionAuthorTransaction transaction(&data,701);
        check(transaction.prepare(layer(),&panel)==0,"inactive sources need no author transaction");
        const auto before=calls;check(transaction.accept()==0 && calls==before,"no second inventory when there is no intent");
        check(peak_streams==1 && writes==0,"inactive inventory holds no bank of streams");
    }
    initialize(2);sources[1].uuid=sources[0].uuid;
    {
        MotionAuthorTransaction transaction(&data,701);
        check(transaction.prepare(layer(),&panel)!=0 && writes==0,"duplicate UUIDs reject before any writes");
    }
    for(const bool missing:{false,true}){
        initialize(2);
        MotionAuthorTransaction transaction(&data,701);
        check(transaction.prepare(layer(),&panel)==0,"prepare identity failure case");
        const auto before=writes;
        if(missing)parade.pop_back();else sources[1].uuid=sources[0].uuid;
        check(transaction.accept()!=0 && writes==before,"missing/duplicate source rejects without index-based writes");
        check(transaction.rollback()==PF_Err_INTERNAL_STRUCT_DAMAGED,"unresolvable rollback is explicit failure");
        check(writes==before,"ambiguous identity never writes a different source");
    }
    initialize(2);
    {
        MotionAuthorTransaction transaction(&data,701);check(transaction.prepare(layer(),&panel)==0,"prepare rollback");
        std::reverse(parade.begin(),parade.end());++epoch;
        check(transaction.rollback()==0,"rollback reacquires after reordering and invalidation");restored();
    }
    initialize(1);
    {
        MotionAuthorTransaction transaction(&data,701);check(transaction.prepare(layer(),&panel)==0,"prepare guard failure");
        discard_guard_restore=true;
        check(transaction.rollback()==PF_Err_INTERNAL_STRUCT_DAMAGED && sources[0].guard==1,"discarded guard restoration fails closed");
    }
    // Exhaust every SDK failure point on a two-source successful transaction.
    initialize(2);int prepare_calls{},accept_calls{};
    {
        MotionAuthorTransaction transaction(&data,701);check(transaction.prepare(layer(),&panel)==0,"failure baseline prepare");
        prepare_calls=calls;check(transaction.accept()==0,"failure baseline accept");accept_calls=calls-prepare_calls;
    }
    for(int phase=0;phase<2;++phase)for(int point=1;point<=(phase?accept_calls:prepare_calls);++point){
        initialize(2);
        {
            MotionAuthorTransaction transaction(&data,701);
            if(!phase)fail_call=point;
            auto error=transaction.prepare(layer(),&panel);
            if(phase){check(error==0,"failure injection prepared");fail_call=calls+point;error=transaction.accept();}
            check(error!=0,"injected SDK error rejects transaction");
            fail_call=-1;check(transaction.rollback()==0,"single failure can restore values explicitly");restored();
            check(stale==0,"failure paths do not use expired streams");
        }
    }
    initialize(0);
    std::printf("Motion author lifetime: %d checks, %d failures; SDK failure points %d + %d.\n",checks,failures,prepare_calls,accept_calls);
    return failures?1:0;
}
