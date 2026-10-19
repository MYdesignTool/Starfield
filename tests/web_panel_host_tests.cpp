// Real production queue/AE script dispatch with an invisible message-only
// Win32 window; no AE, COM initialization or browser subprocess is created.
#define STARFIELD_WITH_WEBVIEW2
#include "../ae_plugin/WebPanelHost.cpp"
#include <iostream>
using namespace starfield::adapter;
namespace {
unsigned checks{},failures{},executions{},acquisitions{},releases{},freed{};
bool suppressed=true,script_failure{};
bool scripting_available=true,window_lookup_failure{},quiet_start_failure{},quiet_active{};
unsigned quiet_starts{},quiet_ends{};
HWND main_window{};
AEGP_ErrReportState* quiet_state{};
std::string last_script;
std::vector<std::string> executed;
AEGP_UtilitySuite6 utility{};AEGP_MemorySuite1 memory{};
void check(bool ok,const char* label){++checks;if(!ok){++failures;std::cerr<<label<<'\n';}}
SPErr acquire(const char* name,int32,const void** output){++acquisitions;*output=nullptr;
    if(!std::strcmp(name,kAEGPUtilitySuite))*output=&utility;
    if(!std::strcmp(name,kAEGPMemorySuite))*output=&memory;
    return *output?kSPNoError:kSPBadParameterError;
}
SPErr release(const char*,int32){++releases;return kSPNoError;}
struct Text {std::string bytes;};
std::wstring message(std::wstring_view id,std::wstring_view script){
    return L"{\"channel\":\"org.starfieldfx.webview2/1\",\"kind\":\"evalScript\",\"id\":"+web_panel::quote(id)+L",\"script\":"+web_panel::quote(script)+L"}";
}
class WebMessage final:public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,ICoreWebView2WebMessageReceivedEventArgs> {
public:
    std::wstring source=std::wstring(origin)+L"index.html",json;
    HRESULT STDMETHODCALLTYPE get_Source(LPWSTR* output)override{return copy(source,output);}
    HRESULT STDMETHODCALLTYPE get_WebMessageAsJson(LPWSTR* output)override{return copy(json,output);}
    HRESULT STDMETHODCALLTYPE TryGetWebMessageAsString(LPWSTR*)override{return E_INVALIDARG;}
    static HRESULT copy(const std::wstring& text,LPWSTR* output){*output=static_cast<LPWSTR>(CoTaskMemAlloc((text.size()+1)*sizeof(wchar_t)));
        if(!*output)return E_OUTOFMEMORY;std::memcpy(*output,text.c_str(),(text.size()+1)*sizeof(wchar_t));return S_OK;}
};
}
int main() {
    namespace protocol=starfield::adapter::web_panel;
    protocol::Message decoded;decoded.id=L"unchanged";
    check(protocol::parse(message(L"wv-1",L"中文\\\"\n"),decoded) && decoded.script==L"中文\\\"\n","Unicode and escaped JSON decoded exactly");
    for(const auto bad:{L"",L"{}",L"[]",L"{\"channel\":\"wrong\"}",L"{\"channel\":null}",L"{\"id\":\"x\",\"id\":\"y\"}"})check(!protocol::parse(bad,decoded),"malformed object rejects");
    const auto good=message(L"wv-1",L"safe");
    for(std::size_t length=0;length<good.size();++length)check(!protocol::parse(std::wstring_view(good).substr(0,length),decoded),"every truncated request rejects");
    check(!protocol::parse(good+L" trailing",decoded),"trailing data rejects");
    check(!protocol::parse(message(L"../escape",L"x"),decoded),"unsafe id rejects");
    check(!protocol::parse(message(L"wv-1",std::wstring(protocol::max_message,L'x')),decoded),"oversized message rejects");
    check(!protocol::parse(L"{\"channel\":\"org.starfieldfx.webview2/1\",\"id\":\"wv-1\",\"kind\":\"unknown\"}",decoded),"unknown command rejects");
    utility.AEGP_GetSuppressInteractiveUI=[](A_Boolean* value)->A_Err{*value=suppressed;return 0;};
    utility.AEGP_GetMainHWND=[](void* value)->A_Err{*static_cast<HWND*>(value)=main_window;return window_lookup_failure?1:0;};
    utility.AEGP_IsScriptingAvailable=[](A_Boolean* value)->A_Err{*value=scripting_available;return 0;};
    utility.AEGP_StartQuietErrors=[](AEGP_ErrReportState* state)->A_Err{
        if(quiet_start_failure)return 1;
        check(!quiet_active,"quiet-error scopes never nest");quiet_active=true;quiet_state=state;++quiet_starts;return 0;};
    utility.AEGP_EndQuietErrors=[](A_Boolean report,AEGP_ErrReportState* state)->A_Err{
        check(quiet_active && state==quiet_state && !report,"restore exact quiet-error state without displaying a captured error");
        quiet_active=false;quiet_state=nullptr;++quiet_ends;return 0;};
    utility.AEGP_ExecuteScript=[](AEGP_PluginID,const A_char* script,A_Boolean encoding,AEGP_MemHandle* result,AEGP_MemHandle* error)->A_Err{
        ++executions;last_script=script;executed.push_back(last_script);check(!encoding,"UTF-8 scripting");
        check(quiet_active,"only script dispatch runs inside quiet-error scope");
        check(!step_web_panel_host(),"script recursion cannot dispatch another request");
        *result=reinterpret_cast<AEGP_MemHandle>(new Text{"{\"ok\":true,\"name\":\"\xE4\xB8\xAD\xE6\x96\x87\"}"});
        *error=reinterpret_cast<AEGP_MemHandle>(new Text{script_failure?"error":""});return script_failure?1:0;
    };
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle handle,AEGP_MemSize* size)->A_Err{*size=static_cast<AEGP_MemSize>(reinterpret_cast<Text*>(handle)->bytes.size()+1);return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle handle,void** output)->A_Err{*output=reinterpret_cast<Text*>(handle)->bytes.data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle)->A_Err{return 0;};
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle handle)->A_Err{delete reinterpret_cast<Text*>(handle);++freed;return 0;};
    SPBasicSuite sdk{};sdk.AcquireSuite=acquire;sdk.ReleaseSuite=release;
    initialize_ui_exclusion();initialize_web_panel_host(&sdk,8);
    check(!environment && !com_owned && !creating_environment && views.empty(),"headless registration never initializes COM or a browser");
    main_window=CreateWindowExW(0,L"STATIC",L"",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
    check(main_window && !IsWindowVisible(main_window),"invisible message-only host stand-in created");
    auto view=std::make_shared<View>();views.push_back(view);view->epoch=1;
    auto event=Microsoft::WRL::Make<WebMessage>();event->json=message(L"wv-1",L"中文 script");
    event->source=L"https://untrusted.invalid/index.html";view->receive(event.Get());check(requests.empty(),"untrusted origin cannot enqueue");
    event->source=view->url();view->receive(event.Get());check(requests.size()==1 && executions==0,"browser callback only enqueues owned strings");
    view->receive(event.Get());check(requests.size()==1,"duplicate admission never writes twice");
    {HostUiExclusion excluded;check(bool(excluded),"exclusion acquired");check(!step_web_panel_host() && executions==0 && requests.size()==1,"existing native transactions defer browser work");}
    std::thread worker([]{check(!step_web_panel_host() && executions==0,"worker rejects before host access");});worker.join();
    EnableWindow(main_window,FALSE);const auto reserved=queued_bytes;
    for(unsigned i=0;i<3;++i)check(!step_web_panel_host() && executions==0 && requests.size()==1 && queued_bytes==reserved && !quiet_starts,
        "modal host defers FIFO work without script calls, spinning, or losing reservations");
    EnableWindow(main_window,TRUE);scripting_available=false;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"host scripting unavailable defers without consuming work");scripting_available=true;
    window_lookup_failure=true;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"failed main-window lookup fails closed");window_lookup_failure=false;
    const auto valid_main=main_window;main_window=nullptr;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"missing main window defers shutdown work");main_window=valid_main;
    check(!step_web_panel_host() && executions==1 && last_script=="\xE4\xB8\xAD\xE6\x96\x87 script" && freed==2,"UI idle executes exactly once and frees both AE handles");
    check(acquisitions==releases,"AE suites released after callback");
    event->json=message(L"wv-2",L"expired");view->receive(event.Get());requests.front().expires={};step_web_panel_host();check(executions==1,"expired edit cannot write later");
    event->json=message(L"wv-3",L"old document");view->receive(event.Get());++view->epoch;step_web_panel_host();check(executions==1,"navigation invalidates queued edits");
    for(unsigned i=4;i<68;++i){event->json=message(L"wv-"+std::to_wstring(i),L"ordered "+std::to_wstring(i));view->receive(event.Get());}
    check(requests.size()==64,"bounded queue accepts its exact capacity");event->json=message(L"wv-68",L"overflow");view->receive(event.Get());check(requests.size()==64,"count overflow rejects without mutation");
    while(!requests.empty())step_web_panel_host();check(executions==65 && executed[1]=="ordered 4" && executed.back()=="ordered 67" && queued_bytes==0,"FIFO execution and accounting");
    script_failure=true;event->json=message(L"wv-69",L"failing");view->receive(event.Get());step_web_panel_host();check(executions==66 && freed==132,"failed script still frees all memory");script_failure=false;
    check(wide("\xE4\xB8\xAD\xE6\x96\x87")==L"中文" && utf8(L"中文")=="\xE4\xB8\xAD\xE6\x96\x87","UTF-8 result roundtrip");
    for(unsigned i=70;i<74;++i){event->json=message(L"wv-"+std::to_wstring(i),std::wstring(1040000,L'x'));view->receive(event.Get());}
    check(requests.size()==4 && queued_bytes==8320000,"byte capacity is independent of count capacity");
    event->json=message(L"wv-74",std::wstring(1040000,L'x'));view->receive(event.Get());check(requests.size()==4 && queued_bytes==8320000,"byte overflow rejects atomically");
    for(auto& queued:requests)queued.expires={};while(!requests.empty())step_web_panel_host();check(queued_bytes==0 && executions==66,"expired large messages free their byte reservations");
    A_long snaps=5,items=20;check(!snap_sizes(nullptr,nullptr,&snaps) && snaps==0 && !flyout_items(nullptr,nullptr,&items) && items==0,"SDK panel callback table has valid empty callbacks");
    check(quiet_starts==66 && quiet_ends==66 && !quiet_active,"success and script-error paths restore quiet-error state");
    quiet_start_failure=true;
    check(execute(L"must not run").find(L"host_busy")!=std::wstring::npos && executions==66 && quiet_starts==quiet_ends,
        "quiet-error setup failure never executes an unprotected script");quiet_start_failure=false;
    event->json=message(L"wv-75",L"expired during modal");view->receive(event.Get());EnableWindow(main_window,FALSE);
    requests.front().expires={};step_web_panel_host();check(executions==66 && requests.empty() && queued_bytes==0,"modal-expired edit is discarded without executing");
    EnableWindow(main_window,TRUE);step_web_panel_host();check(executions==66,"dismissed modal cannot replay an expired write");
    event->json=message(L"wv-76",L"closed");view->receive(event.Get());view->close();step_web_panel_host();check(executions==66,"closed view cannot apply queued edits");
    check(acquisitions==releases,"all paths release SDK suites");stop_web_panel_host();stop_ui_exclusion();
    check(requests.empty() && views.empty() && !environment,"shutdown clears all owned pending state");
    check(locate_content() && fs::canonical(content_root)==fs::canonical(fs::current_path()/L"web_panel"),
        "production content discovery resolves renamed web_panel from the real executable location without a browser");
    DestroyWindow(main_window);
    std::cout<<checks<<" checks, "<<failures<<" failures; actual WebPanelHost queue, no AE/browser process\n";
    return failures?1:0;
}
