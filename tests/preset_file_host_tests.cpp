#include "PresetFileHost.hpp"
#include "UiExclusionHost.hpp"
#define NOMINMAX
#include <Windows.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

using namespace starfield::adapter;
namespace {
unsigned checks{},suites{},handles{},locks{},scripts{},choosers{},completions{},depth{},message_pumps{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
constexpr char id[]="0123456789abcdef0123456789abcdef";
AEGP_UtilitySuite6 utility{};AEGP_MemorySuite1 memory{};SPBasicSuite basic{};
struct Memory {std::string text;};
std::string descriptor,completion,missing_suite;
std::u16string selected;
int script_fault{},memory_fault{};
bool available{},owner_ok{},current{},stop_in_modal{},queue_in_modal{},throw_in_modal{},lost_ack{};
PresetFileChoice choice{};
AEGP_MemHandle mem(std::string text){++handles;return reinterpret_cast<AEGP_MemHandle>(new Memory{std::move(text)});}
void no_owned(){CHECK(suites==0);CHECK(handles==0);CHECK(locks==0);CHECK(depth==0);}
void reset(){stop_preset_file_host();no_owned();initialize_ui_exclusion();initialize_preset_file_host(&basic,7);
    descriptor=std::string(id)+"|1";selected=u"D:/\u6d4b\u8bd5/'safe$().sfldpreset";completion.clear();missing_suite.clear();
    script_fault=memory_fault=0;available=owner_ok=current=true;stop_in_modal=queue_in_modal=throw_in_modal=lost_ack=false;
    choice=PresetFileChoice::selected;scripts=choosers=completions=0;}
void run(){queue_preset_file_dialog();(void)step_preset_file_host();no_owned();CHECK(ui_exclusion_available());}
LRESULT CALLBACK pump(HWND window,UINT message,WPARAM w,LPARAM l){
    if(message==WM_APP){const auto before=scripts;CHECK(!step_preset_file_host());CHECK(scripts==before);CHECK(!ui_exclusion_available());++message_pumps;return 0;}
    return DefWindowProcW(window,message,w,l);
}
void initialize(){
    utility.AEGP_IsScriptingAvailable=[](A_Boolean* out)->A_Err{*out=available;return 0;};
    utility.AEGP_GetMainHWND=[](void* out)->A_Err{*static_cast<void**>(out)=owner_ok?reinterpret_cast<void*>(1):nullptr;return 0;};
    utility.AEGP_ExecuteScript=[](AEGP_PluginID plugin,const A_char* body,A_Boolean platform,AEGP_MemHandle* result,AEGP_MemHandle* error)->A_Err{
        CHECK(plugin==7);CHECK(!platform);CHECK(depth++==0);CHECK(!ui_exclusion_available());CHECK(!step_preset_file_host());++scripts;
        const std::string source(body);std::string reply="1";int stage{};
        if(source.find("SFLD_presetFileHostRequest(")!=std::string::npos){stage=1;reply=descriptor;descriptor="0";}
        else if(source.find("SFLD_presetFileHostBegin(")!=std::string::npos){stage=4;reply="1";}
        else if(source.find("SFLD_presetFileHostValidate(")!=std::string::npos){stage=2;reply=current?"1":"0";CHECK(source.find(std::string("'")+id+"'")!=std::string::npos);}
        else if(source.find("SFLD_presetFileHostComplete(")!=std::string::npos){stage=3;++completions;completion=source;}
        else CHECK(false);
        --depth;if(script_fault==stage || stage==3 && lost_ack){*error=mem("error");*result=mem("ignored");return 512;}
        if(memory_fault==1)reply=std::string(129,'x');
        if(memory_fault!=2)reply+='\0';*result=mem(reply);*error=mem(std::string(1,'\0'));return 0;
    };
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle handle)->A_Err{delete reinterpret_cast<Memory*>(handle);CHECK(handles>0);--handles;return 0;};
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle handle,AEGP_MemSize* out)->A_Err{*out=static_cast<AEGP_MemSize>(reinterpret_cast<Memory*>(handle)->text.size());return memory_fault==3?512:0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle handle,void** out)->A_Err{if(memory_fault==4)return 512;*out=reinterpret_cast<Memory*>(handle)->text.data();++locks;return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle)->A_Err{CHECK(locks>0);--locks;return 0;};
    basic.AcquireSuite=[](const char* name,int32,const void** out)->A_Err{
        if(missing_suite==name){*out=nullptr;return 512;}
        if(!std::strcmp(name,kAEGPUtilitySuite))*out=&utility;
        else if(!std::strcmp(name,kAEGPMemorySuite))*out=&memory;else {CHECK(false);return 512;}
        ++suites;return 0;
    };
    basic.ReleaseSuite=[](const char*,int32)->A_Err{CHECK(suites>0);--suites;return 0;};
}
}
namespace starfield::adapter {
PresetFileChoice choose_preset_file(std::uintptr_t owner,bool save,std::u16string& path){
    no_owned();CHECK(owner==1);CHECK(!ui_exclusion_available());CHECK(SFLD_BeginUiExclusionV1()==0);CHECK(!step_preset_file_host());++choosers;
    CHECK(save==(descriptor=="0" && selected==u"D:/save.sfldpreset"));
    WNDCLASSW cls{};cls.lpfnWndProc=pump;cls.lpszClassName=L"StarfieldPresetFileHostTest";cls.hInstance=GetModuleHandleW(nullptr);
    (void)RegisterClassW(&cls);HWND window=CreateWindowExW(0,cls.lpszClassName,L"",0,0,0,0,0,HWND_MESSAGE,nullptr,cls.hInstance,nullptr);CHECK(window!=nullptr);
    CHECK(PostMessageW(window,WM_APP,0,0));MSG message{};CHECK(GetMessageW(&message,window,WM_APP,WM_APP)>0);DispatchMessageW(&message);CHECK(DestroyWindow(window));
    if(queue_in_modal)queue_preset_file_dialog();if(stop_in_modal)stop_preset_file_host();
    if(throw_in_modal)throw 1;path=selected;return choice;
}
}
int main(){initialize();reset();CHECK(!step_preset_file_host());CHECK(scripts==0);
    run();CHECK(choosers==1);CHECK(completions==1);CHECK(completion.find("00270073006100660065")!=std::string::npos);CHECK(completion.find("$()")==std::string::npos);
    const auto saved=completion;queue_preset_file_dialog();CHECK(!step_preset_file_host());CHECK(choosers==1);CHECK(completion==saved);no_owned();
    reset();descriptor=std::string(id)+"|2";selected=u"D:/save.sfldpreset";run();CHECK(choosers==1);CHECK(completions==1);
    for(auto c:{PresetFileChoice::cancelled,PresetFileChoice::failed}){reset();choice=c;run();CHECK(completions==1);CHECK(completion.find(c==PresetFileChoice::cancelled?",'',1":",'',2")!=std::string::npos);}
    reset();current=false;run();CHECK(choosers==1);CHECK(completions==0);
    reset();stop_in_modal=true;run();CHECK(completions==0);CHECK(scripts==2);
    reset();throw_in_modal=true;run();CHECK(completions==1);CHECK(completion.find(",'',2")!=std::string::npos);
    reset();queue_in_modal=true;run();CHECK(!step_preset_file_host());CHECK(choosers==1);no_owned();
    reset();lost_ack=true;run();CHECK(completions==1);queue_preset_file_dialog();CHECK(!step_preset_file_host());CHECK(completions==1);no_owned();
    reset();owner_ok=false;run();CHECK(choosers==0);CHECK(completions==1);
    for(const auto* suite:{kAEGPUtilitySuite,kAEGPMemorySuite}){reset();missing_suite=suite;run();CHECK(choosers==0);CHECK(completions==0);}
    reset();available=false;run();CHECK(scripts==0);CHECK(choosers==0);
    for(int stage=1;stage<=3;++stage){reset();script_fault=stage;run();CHECK(choosers==(stage==1?0u:1u));CHECK(completions==(stage==3?1u:0u));}
    reset();script_fault=4;run();CHECK(choosers==0);CHECK(completions==1);
    for(int fault=1;fault<=4;++fault){reset();memory_fault=fault;run();CHECK(choosers==0);CHECK(completions==0);}
    for(const auto& line:{std::string("0"),std::string(32,'0')+"|1",std::string(32,'A')+"|1",std::string(id)+"|3",std::string(id)+"|1|extra"}){reset();descriptor=line;run();CHECK(choosers==0);CHECK(completions==0);}
    for(const auto& path:{std::u16string{},std::u16string(u"relative"),std::u16string(u"D:/\nunsafe"),std::u16string(u"D:/")+char16_t(0xd800),std::u16string(u"D:/")+char16_t(0xdc00),std::u16string(u"D:/")+std::u16string(32765,u'x')}){
        reset();selected=path;run();CHECK(completions==1);CHECK(completion.find(",'',2")!=std::string::npos);}
    reset();selected=u"\\\\server/share/\U0001f600.sfldpreset";run();CHECK(completions==1);CHECK(completion.find("d83dde00")!=std::string::npos);
    reset();{HostUiExclusion scope;CHECK(bool(scope));queue_preset_file_dialog();CHECK(step_preset_file_host());CHECK(scripts==0);}CHECK(!step_preset_file_host());CHECK(choosers==1);no_owned();
    reset();std::thread worker([]{queue_preset_file_dialog();CHECK(!step_preset_file_host());});worker.join();CHECK(scripts==0);CHECK(!step_preset_file_host());
    reset();stop_preset_file_host();queue_preset_file_dialog();CHECK(!step_preset_file_host());CHECK(scripts==0);no_owned();
    CHECK(message_pumps>10);std::cout<<"preset_file_host_tests: "<<checks<<" checks passed; actual Host with fake SDK/chooser and same-thread Windows message pump\n";
}
