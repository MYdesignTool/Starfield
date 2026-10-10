#include "PresetFileHost.hpp"
#include "UiExclusionHost.hpp"
#include "ScriptDiagnostic.hpp"
#include <algorithm>
#include <cstring>
#include <string_view>
#include <thread>

namespace starfield::adapter {
namespace {
SPBasicSuite* basic{};AEGP_PluginID plugin{};std::thread::id ui_thread;
bool queued{},stopped{},running{};
template<class T> struct Suite {
    const char* name;A_long version;const T* value{};
    Suite(const char* n,A_long v):name(n),version(v){if(basic)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
bool script(const char* name,const std::string& arguments,std::string& output) {
    output.clear();
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_MemorySuite1> memory(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    if(!utility.value || !memory.value)return false;
    A_Boolean available{};
    if(utility.value->AEGP_IsScriptingAvailable(&available) || !available)return false;
    AEGP_MemHandle result{},error{};
    struct Handles {const AEGP_MemorySuite1* suite;AEGP_MemHandle& result;AEGP_MemHandle& error;
        ~Handles(){if(error)suite->AEGP_FreeMemHandle(error);if(result)suite->AEGP_FreeMemHandle(result);}} handles{memory.value,result,error};
    const auto body="(typeof "+std::string(name)+"==='function'?"+name+"("+arguments+"): '0')";
    if(utility.value->AEGP_ExecuteScript(plugin,body.c_str(),FALSE,&result,&error) ||
       !script_diagnostic_empty(memory.value,error) || !result)return false;
    AEGP_MemSize size{};
    if(memory.value->AEGP_GetMemHandleSize(result,&size) || !size || size>128)return false;
    void* data{};if(memory.value->AEGP_LockMemHandle(result,&data) || !data)return false;
    struct Lock {const AEGP_MemorySuite1* suite;AEGP_MemHandle handle;~Lock(){suite->AEGP_UnlockMemHandle(handle);}} lock{memory.value,result};
    const auto* begin=static_cast<const char*>(data);const auto* end=static_cast<const char*>(std::memchr(begin,0,size));
    if(!end || end!=begin+size-1)return false;
    output.assign(begin,end);return true;
}
bool id_valid(std::string_view id) {
    return id.size()==32 && id!=std::string(32,'0') && std::all_of(id.begin(),id.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');});
}
bool path_hex(const std::u16string& path,std::string& hex) {
    if(path.empty() || path.size()>32767)return false;
    const bool drive=path.size()>2 && ((path[0]>=u'A'&&path[0]<=u'Z')||(path[0]>=u'a'&&path[0]<=u'z')) && path[1]==u':' && (path[2]==u'\\'||path[2]==u'/');
    if(!drive && !(path.size()>2 && path[0]==u'\\' && path[1]==u'\\'))return false;
    constexpr char digits[]="0123456789abcdef";hex.clear();hex.reserve(path.size()*4);
    for(std::size_t i=0;i<path.size();++i){const auto c=path[i];if(c<32)return false;
        if(c>=0xd800 && c<=0xdbff){if(i+1==path.size() || path[i+1]<0xdc00 || path[i+1]>0xdfff)return false;}
        if(c>=0xdc00 && c<=0xdfff && (i==0 || path[i-1]<0xd800 || path[i-1]>0xdbff))return false;
        for(int shift=12;shift>=0;shift-=4)hex+=digits[(c>>shift)&15];}
    return true;
}
bool owner_window(std::uintptr_t& owner) {
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!utility.value)return false;
    void* window{};
    if(utility.value->AEGP_GetMainHWND(&window) || !window)return false;
    owner=reinterpret_cast<std::uintptr_t>(window);return true;
}
void finish(const std::string& id,const std::string& hex,PresetFileChoice choice) {
    if(stopped)return;
    std::string response;
    const auto identity="'"+id+"'";
    if(!script("SFLD_presetFileHostValidate",identity,response) || response!="1" || stopped)return;
    // Completion rechecks ID, stage and expiry itself before any file IO. Its
    // retained terminal receipt is polled; a lost acknowledgement is not replayed.
    (void)script("SFLD_presetFileHostComplete",identity+",'"+hex+"',"+std::to_string(static_cast<std::uint32_t>(choice)),response);
}
}
void initialize_preset_file_host(SPBasicSuite* suites,AEGP_PluginID id) noexcept {
    basic=suites;plugin=id;ui_thread=std::this_thread::get_id();queued=stopped=running=false;
}
void queue_preset_file_dialog() noexcept {if(!stopped && std::this_thread::get_id()==ui_thread)queued=true;}
void stop_preset_file_host() noexcept {stopped=true;queued=false;}
bool step_preset_file_host() noexcept {
    if(stopped || running || !queued || std::this_thread::get_id()!=ui_thread)return false;
    HostUiExclusion exclusion;if(!exclusion)return true;
    running=true;struct Running {~Running(){running=false;}} scope;
    queued=false;std::string id;
    try {
        std::string descriptor;
        if(!script("SFLD_presetFileHostRequest","",descriptor) || stopped)return queued;
        if(descriptor.size()!=34 || descriptor[32]!='|' || (descriptor[33]!='1' && descriptor[33]!='2') || !id_valid(std::string_view(descriptor).substr(0,32)))return queued;
        id=descriptor.substr(0,32);const bool save=descriptor[33]=='2';
        std::string claimed;
        if(!script("SFLD_presetFileHostBegin","'"+id+"'",claimed) || claimed!="1" || stopped){finish(id,"",PresetFileChoice::failed);return queued;}
        std::uintptr_t owner{};
        if(!owner_window(owner)){finish(id,"",PresetFileChoice::failed);return queued;}
        // Every SDK suite/result/lock has been released before this call.
        std::u16string path;const auto choice=choose_preset_file(owner,save,path);
        if(stopped)return false;
        std::string hex;auto result=choice;
        if(choice==PresetFileChoice::selected && !path_hex(path,hex)){hex.clear();result=PresetFileChoice::failed;}
        finish(id,hex,result);
    }catch(...){if(!id.empty())try{finish(id,"",PresetFileChoice::failed);}catch(...){}}
    return queued;
}
}
