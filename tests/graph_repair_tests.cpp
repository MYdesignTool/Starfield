// Actual repair coordinator, fake SDK operations; never opens or writes an AE project.
#include "../ae_plugin/GraphRepair.cpp"
#include <array>
#include <iostream>
#include <string>
namespace {
using namespace starfield::adapter;
struct Fx {std::array<double,256> values{};AEGP_EffectFlags flags{AEGP_EffectFlags_ACTIVE};int kind{};};
struct Layer {std::array<Fx,2> fx;AEGP_LayerFlags flags{AEGP_LayerFlag_VIDEO_ACTIVE|AEGP_LayerFlag_AUDIO_ACTIVE};};
std::array<Layer,2> state;
AEGP_EffectSuite4 effect{};AEGP_StreamSuite6 stream{};AEGP_LayerSuite9 layer{};AEGP_UtilitySuite6 utility{};
struct Ref {Fx* fx;A_long index;};
unsigned checks{},failures{},calls{},fail_at{},starts{},ends{},publishes{},dupes{},writes{},outside{},refs{},suites{};
bool undo{},publish_failure{},bad_receipt{},silent_disable{},busy_author{},unknown_author{},ambiguous{};
void check(bool value,const char* message){++checks;if(!value){++failures;std::cerr<<message<<" at "<<fail_at<<'\n';}}
A_Err fail(){return ++calls==fail_at?PF_Err_BAD_CALLBACK_PARAM:0;}
void mutation(){++writes;if(!undo)++outside;}
Fx* fx(AEGP_EffectRefH r){return reinterpret_cast<Fx*>(r);}
Layer* ly(AEGP_LayerH r){return reinterpret_cast<Layer*>(r);}
SPErr acquire(const char* name,int32,const void** out){*out=nullptr;if(auto e=fail())return e;
    if(!std::strcmp(name,kAEGPEffectSuite))*out=&effect;
    if(!std::strcmp(name,kAEGPStreamSuite))*out=&stream;
    if(!std::strcmp(name,kAEGPLayerSuite))*out=&layer;
    if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility;
    if(*out)++suites;return *out?0:kSPBadParameterError;}
SPErr release(const char*,int32){--suites;return 0;}
void reset(){state={};state[0].fx[1].kind=1;
    auto& v=state[0].fx[0].values;v[kGraphSyncGuardId]=1;v[kGraphEditCommitId]=790333;
    v[kGraphEditReceiptId]=-790333;v[kNodeEffectsReadyId]=1;v[kGraphRevisionId]=44;v[150]=987.25;
    state[0].fx[1].values[2]=123.75;
    calls=fail_at=starts=ends=publishes=dupes=writes=outside=refs=suites=0;
    undo=publish_failure=bad_receipt=silent_disable=busy_author=unknown_author=ambiguous=false;}
}
namespace starfield::adapter {
PF_Err commit_panel_graph(PF_InData*,AEGP_EffectRefH renderer,AEGP_LayerH owner,A_long nonce,AEGP_PluginID,PF_OutData* out,bool rebuild) noexcept {
    ++publishes;check(rebuild && owner==reinterpret_cast<AEGP_LayerH>(&state[0]) && fx(renderer)==&state[0].fx[0],"repair publishes original owner");
    check(!(fx(renderer)->flags&AEGP_EffectFlags_ACTIVE) && state[1].fx[0].values[kGraphSyncGuardId]==2 &&
        !(state[1].flags&(AEGP_LayerFlag_VIDEO_ACTIVE|AEGP_LayerFlag_AUDIO_ACTIVE)),"backup and renderer disable precede publication");
    if(auto e=fail())return e;
    if(publish_failure){std::strcpy(out->return_msg,"animation binding stream 237");return PF_Err_BAD_CALLBACK_PARAM;}
    mutation();auto& v=fx(renderer)->values;v[kGraphEditReceiptId]=bad_receipt?0:nonce;v[kGraphRevisionId]+=1;return 0;
}
}
int main(){
    auto module=LoadLibraryW(L"artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex");check(module!=nullptr,"load isolated exclusion fixture");
    if(!module)return 1;
    const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));
    check(initialize!=nullptr,"initialize isolated exclusion fixture");if(!initialize)return 1;initialize();
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_UtilCallbacks utils{};PF_InData data{};data.pica_basicP=&basic;data.utils=&utils;
    effect.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* n)->A_Err{*n=2;return fail();};
    effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH r,A_long i,AEGP_EffectRefH* out)->A_Err{
        if(auto e=fail())return e;*out=reinterpret_cast<AEGP_EffectRefH>(&ly(r)->fx[i]);++refs;return 0;};
    effect.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{--refs;return 0;};
    effect.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH r,AEGP_InstalledEffectKey* k)->A_Err{*k=fx(r)->kind;return fail();};
    effect.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey k,A_char* s)->A_Err{
        std::strcpy(s,k && !ambiguous?(unknown_author?"org.starfieldfx.node.unknown":"org.starfieldfx.node.motion"):"org.starfieldfx.particle");return fail();};
    effect.AEGP_GetEffectFlags=[](AEGP_EffectRefH r,AEGP_EffectFlags* f)->A_Err{*f=fx(r)->flags;return fail();};
    effect.AEGP_SetEffectFlags=[](AEGP_EffectRefH r,AEGP_EffectFlags mask,AEGP_EffectFlags f)->A_Err{
        mutation();if(!silent_disable)fx(r)->flags=(fx(r)->flags&~mask)|(f&mask);return fail();};
    stream.AEGP_GetNewEffectStreamByIndex=[](AEGP_PluginID,AEGP_EffectRefH r,A_long i,AEGP_StreamRefH* out)->A_Err{
        if(auto e=fail())return e;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{fx(r),i});++refs;return 0;};
    stream.AEGP_DisposeStream=[](AEGP_StreamRefH r)->A_Err{delete reinterpret_cast<Ref*>(r);--refs;return 0;};
    stream.AEGP_GetStreamType=[](AEGP_StreamRefH,AEGP_StreamType* t)->A_Err{*t=AEGP_StreamType_OneD;return fail();};
    stream.AEGP_GetNewStreamValue=[](AEGP_PluginID,AEGP_StreamRefH r,AEGP_LTimeMode,const A_Time* time,A_Boolean pre,AEGP_StreamValue2* out)->A_Err{
        check(time->value==0 && pre,"saved guard uses zero-time pre-expression read");
        const auto& p=*reinterpret_cast<Ref*>(r);out->val.one_d=p.fx->kind && busy_author?1:p.fx->values[p.index];return fail();};
    stream.AEGP_DisposeStreamValue=[](AEGP_StreamValue2*)->A_Err{return 0;};
    stream.AEGP_SetStreamValue=[](AEGP_PluginID,AEGP_StreamRefH r,AEGP_StreamValue2* v)->A_Err{
        mutation();const auto& p=*reinterpret_cast<Ref*>(r);p.fx->values[p.index]=v->val.one_d;return fail();};
    layer.AEGP_DuplicateLayer=[](AEGP_LayerH r,AEGP_LayerH* out)->A_Err{
        if(auto e=fail())return e;mutation();state[1]=*ly(r);*out=reinterpret_cast<AEGP_LayerH>(&state[1]);++dupes;return 0;};
    layer.AEGP_SetLayerFlag=[](AEGP_LayerH r,AEGP_LayerFlags f,A_Boolean b)->A_Err{
        mutation();if(b)ly(r)->flags|=f;else ly(r)->flags&=~f;return fail();};
    layer.AEGP_GetLayerFlags=[](AEGP_LayerH r,AEGP_LayerFlags* f)->A_Err{*f=ly(r)->flags;return fail();};
    layer.AEGP_SetLayerName=[](AEGP_LayerH,const A_UTF16Char*)->A_Err{mutation();return fail();};
    utility.AEGP_StartUndoGroup=[](const A_char*)->A_Err{if(auto e=fail())return e;undo=true;++starts;return 0;};
    utility.AEGP_EndUndoGroup=[]()->A_Err{undo=false;++ends;return fail();};
    const auto run=[&](bool exclusion=true){PF_OutData out{};GraphRepairRequest request;request.plugin=701;request.layer=reinterpret_cast<AEGP_LayerH>(&state[0]);
        PF_Err result{};if(exclusion){EffectUiExclusion scope;check(bool(scope),"acquire isolated UI transaction");result=repair_selected_graph(&data,request,&out);}
        else result=repair_selected_graph(&data,request,&out);
        check(refs==0 && suites==0 && !undo && starts==ends,"all SDK references and undo group released");
        check(outside==0,"no writes outside one undo group");
        check(state[0].fx[0].values[150]==987.25 && state[0].fx[1].values[2]==123.75,"authored data untouched");
        return std::pair{result,std::string(request.message)};};
    reset();auto result=run();const auto success_calls=calls;
    check(result.first==0 && publishes==1 && dupes==1 && state[0].fx[0].values[kGraphSyncGuardId]==0 &&
        state[0].fx[0].values[kGraphEditReceiptId]==790334 && state[0].fx[0].flags==AEGP_EffectFlags_ACTIVE,"original repaired in place with backup");
    for(unsigned n=1;n<=success_calls;++n){reset();fail_at=n;result=run();check(result.first!=0,"single host fault cannot report successful repair");
        if(publishes==0)check(state[0].fx[0].values[kGraphSyncGuardId]==1,"prepublication failure retains original guard");
        check(state[0].fx[0].flags==AEGP_EffectFlags_ACTIVE,"even partial disable failure restores original enabled state");}
    reset();publish_failure=true;result=run();check(result.first!=0 && result.second.find("stream 237")!=std::string::npos &&
        state[0].fx[0].values[kGraphEditReceiptId]==-790333 && state[0].fx[0].values[kGraphEditCommitId]==790333,"publisher detail and rejected request preserved");
    reset();bad_receipt=true;check(run().first!=0 && state[0].fx[0].values[kGraphSyncGuardId]==1,"silent receipt loss refuses unlock");
    reset();silent_disable=true;check(run().first!=0 && publishes==0,"silent renderer disable refuses publication");
    reset();busy_author=true;check(run().first!=0 && writes==0,"active native author refuses repair before writes");
    reset();unknown_author=true;check(run().first!=0 && writes==0,"unknown author layout refuses before writes");
    reset();ambiguous=true;check(run().first!=0 && writes==0,"ambiguous renderer refuses before writes");
    reset();check(run(false).first!=0 && writes==0,"missing resident exclusion refuses repair");
    for(double guard:{0.,2.}){reset();state[0].fx[0].values[kGraphSyncGuardId]=guard;check(run().first!=0 && writes==0,"only exact failed guard repairable");}
    reset();state[0].fx[0].values[kGraphEditReceiptId]=0;check(run().first!=0 && writes==0,"pending request refuses repair");
    reset();state[0].fx[0].flags=AEGP_EffectFlags_NONE;check(run().first==0 && state[0].fx[0].flags==0,"original disabled state preserved");
    check(FreeLibrary(module)!=0,"release isolated exclusion fixture");
    std::cout<<checks<<" repair coordinator checks, "<<failures<<" failures\n";return failures?1:0;
}
