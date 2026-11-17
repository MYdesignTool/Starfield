// Actual resident Host entry; public fake suites only, no AE or browser.
#define STARFIELD_WITH_WEBVIEW2
#define STARFIELD_TEST_NO_JOURNAL
#include "../ae_plugin/StarfieldHost.cpp"
#include <iostream>
namespace {
unsigned checks{},failures{},registrations_count{},defines{},enables{},web_calls{},asset_calls{},model_calls{},preset_calls{},acquisitions{};
AEGP_Command next_command=3000;AEGP_CommandHook registered_hook{};AEGP_Command registered_filter{};
A_Err web_registration=-1;bool fail_hook{};
AEGP_RegisterSuite5 registry{};AEGP_CommandSuite1 command_suite{};
void check(bool value,const char* label){++checks;if(!value){++failures;std::cerr<<label<<'\n';}}
SPErr acquire(const char* name,int32,const void** value){++acquisitions;*value=nullptr;
 if(!std::strcmp(name,kAEGPRegisterSuite))*value=&registry;
 if(!std::strcmp(name,kAEGPCommandSuite))*value=&command_suite;
 return *value?kSPNoError:kSPBadParameterError;}
SPErr release(const char*,int32){return kSPNoError;}
}
namespace starfield::adapter {
void initialize_model_asset_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_asset_export() noexcept {++asset_calls;}
bool step_model_asset_host() noexcept {return false;}
void stop_model_asset_host() noexcept {}
void initialize_model_transaction_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_graph_transaction() noexcept {++model_calls;}
bool step_model_transaction_host() noexcept {return false;}
void stop_model_transaction_host() noexcept {}
void initialize_preset_file_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_preset_file_dialog() noexcept {++preset_calls;}
bool step_preset_file_host() noexcept {return false;}
void stop_preset_file_host() noexcept {}
void initialize_web_panel_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
bool step_web_panel_host() noexcept {return false;}
void stop_web_panel_host() noexcept {}
void set_web_panel_command_registration(A_Err result) noexcept {web_registration=result;}
A_Err dispatch_web_panel_command(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,AEGP_HookPriority,A_Boolean already,A_Boolean* handled) noexcept {
 ++web_calls;if(!already && handled && (command==21 || command==9001 || command==9002))*handled=TRUE;return 0;
}
}
int main(){
 registry.AEGP_RegisterDeathHook=[](AEGP_PluginID,AEGP_DeathHook,AEGP_DeathRefcon)->A_Err{return 0;};
 registry.AEGP_RegisterIdleHook=[](AEGP_PluginID,AEGP_IdleHook,AEGP_IdleRefcon)->A_Err{return 0;};
 registry.AEGP_RegisterUpdateMenuHook=[](AEGP_PluginID,AEGP_UpdateMenuHook,AEGP_UpdateMenuRefcon)->A_Err{return 0;};
 registry.AEGP_RegisterCommandHook=[](AEGP_PluginID id,AEGP_HookPriority priority,AEGP_Command command,AEGP_CommandHook hook,AEGP_CommandRefcon)->A_Err{
   check(id==8 && priority==AEGP_HP_BeforeAE,"resident command hook uses current plugin and SDK priority");
   ++registrations_count;registered_filter=command;registered_hook=hook;return fail_hook?1:0;};
 command_suite.AEGP_GetUniqueCommand=[](AEGP_Command* value)->A_Err{*value=++next_command;return 0;};
 command_suite.AEGP_InsertMenuCommand=[](AEGP_Command,const A_char*,AEGP_MenuID menu,A_long)->A_Err{++defines;check(menu==AEGP_Menu_NONE,"six internal command definitions remain invisible");return 0;};
 command_suite.AEGP_EnableCommand=[](AEGP_Command)->A_Err{++enables;return 0;};
 SPBasicSuite sdk{};sdk.AcquireSuite=acquire;sdk.ReleaseSuite=release;AEGP_GlobalRefcon refcon{};
 check(StarfieldHostEntry(&sdk,0,0,8,&refcon)==0,"actual Host entry starts");
 check(registrations_count==1,"exactly one command hook registration for the entire Host entry");
 if(registrations_count!=1){std::cout<<checks<<" checks, "<<failures<<" failures; actual entry registered "<<registrations_count<<" hooks (not a claim of AE overwrite semantics)\n";return 1;}
 check(registered_filter==AEGP_Command_ALL && registered_hook && web_registration==0,"final ALL dispatcher and actual registration result published");
 check(defines==6 && enables==6,"six session command definitions retained");
 for(const auto& binding:starfield::adapter::native_command_bindings){check(binding.command>3000,"six bound IDs remain valid");A_Boolean handled=FALSE;
   const auto before=web_calls;(void)registered_hook(nullptr,nullptr,binding.command,AEGP_HP_BeforeAE,FALSE,&handled);
   check(handled && web_calls==before,"each native handler is dispatched once without web fallthrough");}
 check(asset_calls==1 && model_calls==1 && preset_calls==1,"all deferred internal services retain their queue handlers");
 for(const auto command:{21,9001,9002}){A_Boolean handled=FALSE;const auto before=web_calls;
   (void)registered_hook(nullptr,nullptr,command,AEGP_HP_BeforeAE,FALSE,&handled);check(handled && web_calls==before+1,"Clear and both panel entries reach web dispatcher exactly once");}
 A_Boolean handled=FALSE;registered_hook(nullptr,nullptr,987654,AEGP_HP_BeforeAE,FALSE,&handled);check(!handled,"unowned AE commands are not consumed");
 const auto before=web_calls;registered_hook(nullptr,nullptr,21,AEGP_HP_BeforeAE,TRUE,&handled);check(!handled && web_calls==before,"already handled command is never executed twice");
 registered_hook(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,nullptr);check(web_calls==before,"missing output pointer cannot execute");
 const auto sdk_before=acquisitions;std::thread worker([&]{registered_hook(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);});worker.join();
 check(web_calls==before && acquisitions==sdk_before,"worker rejects before adapter or SDK access");
 stopped=true;registered_hook(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);check(web_calls==before,"stopped resident does not dispatch");
 stopped=false;fail_hook=true;registrations_count=0;
 StarfieldHostEntry(&sdk,0,0,8,&refcon);check(registrations_count==1 && web_registration==1,"failed sole registration is reported accurately");
 for(const auto& binding:starfield::adapter::native_command_bindings)check(binding.command==0,"failed router cannot advertise executable native bindings");
 std::cout<<checks<<" checks, "<<failures<<" failures; actual Host entry/router, fake SDK, no AE/browser\n";return failures?1:0;
}
