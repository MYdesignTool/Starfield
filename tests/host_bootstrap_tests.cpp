// Fixture-only inclusion exposes numeric cursors without installed test exports.
#include "../ae_plugin/StarfieldHost.cpp"
#include <iostream>
namespace {
unsigned checks{},failures{},gets{},disposes{},acquisitions{},releases{},calls{};
bool active=true,fail_message{},fail_registration{};
A_long effects_count=3;
AEGP_IdleHook idle_callback{};AEGP_DeathHook death_callback{};
AEGP_RegisterSuite5 registrations{};AEGP_ItemSuite9 items{};AEGP_CompSuite11 comps{};
AEGP_LayerSuite9 layers{};AEGP_EffectSuite4 effects{};
void check(bool ok,const char* name){++checks;if(!ok){++failures;std::cerr<<name<<'\n';}}
SPErr acquire(const char* name,int32,const void** value) {
    ++acquisitions;*value=nullptr;
    if(!std::strcmp(name,kAEGPRegisterSuite))*value=&registrations;
    if(!std::strcmp(name,kAEGPItemSuite))*value=&items;
    if(!std::strcmp(name,kAEGPCompSuite))*value=&comps;
    if(!std::strcmp(name,kAEGPLayerSuite))*value=&layers;
    if(!std::strcmp(name,kAEGPEffectSuite))*value=&effects;
    return *value?kSPNoError:kSPBadParameterError;
}
SPErr release(const char*,int32){++releases;return kSPNoError;}
}
int main() {
    registrations.AEGP_RegisterDeathHook=[](AEGP_PluginID,AEGP_DeathHook hook,AEGP_DeathRefcon)->A_Err{death_callback=hook;return 0;};
    registrations.AEGP_RegisterIdleHook=[](AEGP_PluginID,AEGP_IdleHook hook,AEGP_IdleRefcon)->A_Err{idle_callback=hook;return fail_registration?A_Err_GENERIC:0;};
    items.AEGP_GetActiveItem=[](AEGP_ItemH* item)->A_Err{*item=active?reinterpret_cast<AEGP_ItemH>(1):nullptr;return 0;};
    items.AEGP_GetItemType=[](AEGP_ItemH,AEGP_ItemType* type)->A_Err{*type=AEGP_ItemType_COMP;return 0;};
    items.AEGP_GetItemID=[](AEGP_ItemH,A_long* id)->A_Err{*id=41;return 0;};
    comps.AEGP_GetCompFromItem=[](AEGP_ItemH,AEGP_CompH* comp)->A_Err{*comp=reinterpret_cast<AEGP_CompH>(2);return 0;};
    layers.AEGP_GetCompNumLayers=[](AEGP_CompH,A_long* count)->A_Err{*count=1;return 0;};
    layers.AEGP_GetCompLayerByIndex=[](AEGP_CompH,A_long,AEGP_LayerH* layer)->A_Err{*layer=reinterpret_cast<AEGP_LayerH>(3);return 0;};
    layers.AEGP_GetLayerCurrentTime=[](AEGP_LayerH,AEGP_LTimeMode,A_Time* time)->A_Err{*time={8,24};return 0;};
    effects.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* count)->A_Err{*count=effects_count;return 0;};
    effects.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long index,AEGP_EffectRefH* effect)->A_Err{++gets;*effect=reinterpret_cast<AEGP_EffectRefH>(std::intptr_t(index+1));return 0;};
    effects.AEGP_GetInstalledKeyFromLayerEffect=[](AEGP_EffectRefH effect,AEGP_InstalledEffectKey* key)->A_Err{*key=AEGP_InstalledEffectKey(reinterpret_cast<std::intptr_t>(effect));return 0;};
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* name)->A_Err{std::strcpy(name,key==1?"org.starfieldfx.particle":key==2?"org.starfieldfx.node.particle":"third.party.effect");return 0;};
    effects.AEGP_DisposeEffect=[](AEGP_EffectRefH)->A_Err{++disposes;return 0;};
    effects.AEGP_EffectCallGeneric=[](AEGP_PluginID,AEGP_EffectRefH,const A_Time* time,PF_Cmd command,void* extra)->A_Err {
        ++calls;auto* request=static_cast<starfield::adapter::NativeBootstrapRequest*>(extra);
        check(command==PF_Cmd_COMPLETELY_GENERAL && time->scale==24 && request->magic==0x53464931 && request->bytes==sizeof(*request),"bounded read-only UI request");
        request->acknowledged=1;return fail_message?A_Err_GENERIC:0;
    };
    SPBasicSuite sdk{};sdk.AcquireSuite=acquire;sdk.ReleaseSuite=release;AEGP_GlobalRefcon refcon{};
    check(StarfieldHostEntry(&sdk,0,0,8,&refcon)==0 && idle_callback && death_callback,"General AEGP hooks owned for the session");
    A_long sleep=60;idle_callback(nullptr,nullptr,&sleep);
    check(sleep==30 && calls==1 && gets==disposes && acquisitions==releases,"only renderer is messaged and references are callback-local");
    const auto throttle=acquisitions;idle_callback(nullptr,nullptr,&sleep);check(acquisitions==throttle,"idle throttle");
    next_scan={};std::thread worker([&]{idle_callback(nullptr,nullptr,&sleep);});worker.join();check(acquisitions==throttle,"worker rejected before AEGP access");
    active=false;next_scan={};idle_callback(nullptr,nullptr,&sleep);check(calls==1,"no active comp is harmless");
    active=true;fail_message=true;next_scan={};check(idle_callback(nullptr,nullptr,&sleep)==0 && gets==disposes,"generic miss is optional and releases refs");
    fail_message=false;effects_count=1000;next_scan={};const auto old_gets=gets;idle_callback(nullptr,nullptr,&sleep);
    check(gets-old_gets<=64 && effect_index>0,"large stacks have bounded numeric continuation");
    effects_count=1;next_scan={};idle_callback(nullptr,nullptr,&sleep);check(layer_index==0 && effect_index==0,"stack shrink recovers without stale handles");
    death_callback(nullptr,nullptr);next_scan={};const auto dead=acquisitions;idle_callback(nullptr,nullptr,&sleep);check(acquisitions==dead,"death prevents host access");
    stopped=false;fail_registration=true;check(StarfieldHostEntry(&sdk,0,0,8,&refcon)==0 && stopped,"partial registration remains resident and inert");
    check(acquisitions==releases,"suite references balanced");
    std::cout<<checks<<" General AEGP bootstrap checks, "<<failures<<" failures\n";return failures?1:0;
}
