// Real production queue/AE script dispatch with an invisible message-only
// Win32 window; no AE, COM initialization or browser subprocess is created.
#define STARFIELD_WITH_WEBVIEW2
#define STARFIELD_TEST_NO_JOURNAL
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <functional>
namespace focus_fixture {
HWND focused{},foreground{};DWORD input_time{42};unsigned restores{};
bool input_available=true,visible=true;
bool gui_available=true,separate_gui_queue{};HWND gui_focused{},gui_active{};DWORD gui_flags{};
BOOL gui_info(DWORD thread,PGUITHREADINFO value){
    if(thread || value->cbSize!=sizeof(GUITHREADINFO) || !gui_available)return FALSE;
    value->hwndFocus=separate_gui_queue?gui_focused:focused;
    value->hwndActive=separate_gui_queue?gui_active:foreground;value->flags=gui_flags;return TRUE;
}
HWND get_focus(){return focused;}
HWND get_foreground(){return foreground;}
BOOL last_input(PLASTINPUTINFO value){value->dwTime=input_time;return input_available;}
HWND set_focus(HWND value){const auto old=focused;focused=value;++restores;return old;}
BOOL is_visible(HWND value){return visible && ::IsWindow(value);}
std::function<void()> during_script;
int pressed_key{},modifier{};
SHORT key_state(int key){return (key==pressed_key || key==modifier)?SHORT(-32768):0;}
}
#define GetFocus focus_fixture::get_focus
#define GetGUIThreadInfo focus_fixture::gui_info
#define GetForegroundWindow focus_fixture::get_foreground
#define GetLastInputInfo focus_fixture::last_input
#define SetFocus focus_fixture::set_focus
#define IsWindowVisible focus_fixture::is_visible
#define GetKeyState focus_fixture::key_state
#include "../ae_plugin/WebPanelHost.cpp"
#undef GetFocus
#undef GetGUIThreadInfo
#undef GetForegroundWindow
#undef GetLastInputInfo
#undef SetFocus
#undef IsWindowVisible
#undef GetKeyState
#include <iostream>
using namespace starfield::adapter;
namespace {
unsigned checks{},failures{},executions{},acquisitions{},releases{},freed{};
bool suppressed=true,script_failure{};
bool override_diagnostic{};
std::string diagnostic_text;
bool scripting_available=true,window_lookup_failure{},quiet_start_failure{},quiet_active{};
unsigned quiet_starts{},quiet_ends{};
HWND main_window{};
AEGP_ErrReportState* quiet_state{};
std::string last_script;
std::vector<std::string> executed;
AEGP_UtilitySuite6 utility{};AEGP_MemorySuite1 memory{};
AEGP_CommandSuite1 commands{};
AEGP_PanelSuite1 panel_api{};AEGP_RegisterSuite5* registration_api{};
unsigned panel_hook_registrations{},panel_menu_definitions{},panel_toggles{};
unsigned native_calls{};
void check(bool ok,const char* label){++checks;if(!ok){++failures;std::cerr<<label<<'\n';}}
SPErr acquire(const char* name,int32,const void** output){++acquisitions;*output=nullptr;
    if(!std::strcmp(name,kAEGPUtilitySuite))*output=&utility;
    if(!std::strcmp(name,kAEGPMemorySuite))*output=&memory;
    if(!std::strcmp(name,kAEGPCommandSuite))*output=&commands;
    if(!std::strcmp(name,kAEGPPanelSuite))*output=&panel_api;
    if(!std::strcmp(name,kAEGPRegisterSuite))*output=registration_api;
    return *output?kSPNoError:kSPBadParameterError;
}
SPErr release(const char*,int32){++releases;return kSPNoError;}
struct Text {std::string bytes;};
std::wstring message(std::wstring_view id,std::wstring_view script,std::wstring_view kind=L"evalScript"){
    return L"{\"channel\":\"org.starfieldfx.webview2/1\",\"kind\":"+web_panel::quote(kind)+L",\"id\":"+web_panel::quote(id)+L",\"script\":"+web_panel::quote(script)+L"}";
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
// Independent COM boundary: fire the actual production event subscriptions.
class FocusController final:public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,ICoreWebView2Controller> {
public:
    ComPtr<ICoreWebView2FocusChangedEventHandler> got,lost;
    bool fail_got{},fail_lost{},closed{};unsigned removed_got{},removed_lost{};
    void fire(bool own){const auto handler=own?got:lost;if(handler)handler->Invoke(this,nullptr);}
    HRESULT STDMETHODCALLTYPE get_IsVisible(BOOL* output)override{*output=TRUE;return S_OK;}
    HRESULT STDMETHODCALLTYPE put_IsVisible(BOOL)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE get_Bounds(RECT* output)override{*output={};return S_OK;}
    HRESULT STDMETHODCALLTYPE put_Bounds(RECT)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE get_ZoomFactor(double* output)override{*output=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE put_ZoomFactor(double)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE add_ZoomFactorChanged(ICoreWebView2ZoomFactorChangedEventHandler*,EventRegistrationToken*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE remove_ZoomFactorChanged(EventRegistrationToken)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE SetBoundsAndZoomFactor(RECT,double)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON)override{fire(true);return S_OK;}
    HRESULT STDMETHODCALLTYPE add_MoveFocusRequested(ICoreWebView2MoveFocusRequestedEventHandler*,EventRegistrationToken*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE remove_MoveFocusRequested(EventRegistrationToken)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE add_GotFocus(ICoreWebView2FocusChangedEventHandler* handler,EventRegistrationToken* token)override{if(fail_got)return E_FAIL;got=handler;token->value=1;return S_OK;}
    HRESULT STDMETHODCALLTYPE remove_GotFocus(EventRegistrationToken)override{++removed_got;got.Reset();return S_OK;}
    HRESULT STDMETHODCALLTYPE add_LostFocus(ICoreWebView2FocusChangedEventHandler* handler,EventRegistrationToken* token)override{if(fail_lost)return E_FAIL;lost=handler;token->value=2;return S_OK;}
    HRESULT STDMETHODCALLTYPE remove_LostFocus(EventRegistrationToken)override{++removed_lost;lost.Reset();return S_OK;}
    HRESULT STDMETHODCALLTYPE add_AcceleratorKeyPressed(ICoreWebView2AcceleratorKeyPressedEventHandler*,EventRegistrationToken*)override{return E_NOTIMPL;}
    HRESULT STDMETHODCALLTYPE remove_AcceleratorKeyPressed(EventRegistrationToken)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE get_ParentWindow(HWND* output)override{*output=nullptr;return S_OK;}
    HRESULT STDMETHODCALLTYPE put_ParentWindow(HWND)override{return S_OK;}
    HRESULT STDMETHODCALLTYPE NotifyParentWindowPositionChanged()override{return S_OK;}
    HRESULT STDMETHODCALLTYPE Close()override{closed=true;return S_OK;}
    HRESULT STDMETHODCALLTYPE get_CoreWebView2(ICoreWebView2** output)override{*output=nullptr;return E_NOTIMPL;}
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
    check(protocol::parse(message(L"wv-1",L"p0-c88-l111|7|00010002||0102030405060708090a0b0c0d0e0f10",L"nativeGraphDelete"),decoded),"bounded native delta is an owned string, not a JSX request");
    check(!protocol::parse(message(L"wv-1",std::wstring(6401,L'x'),L"nativeGraphDelete"),decoded),"native deletion has its own small bound");
    check(protocol::parse(message(L"wv-1",L"p0-c88-l111|7|00010002|||source:>",L"nativeGraphPatch"),decoded),"native patch carries an owned payload to its bounded executor parser");
    check(!protocol::parse(message(L"wv-1",std::wstring(48001,L'x'),L"nativeGraphPatch"),decoded),"native patch has a separate capacity bound");
    check(protocol::parse(message(L"wv-1",L"p0-c88-l111|7|00010002||",L"nativeGraphDiagnostics"),decoded),"read-only diagnostics carry the expected saved receipt");
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
        if(focus_fixture::during_script)focus_fixture::during_script();
        *result=reinterpret_cast<AEGP_MemHandle>(new Text{last_script.find("app.findMenuCommandId('Clear')")!=std::string::npos?"21":"{\"ok\":true,\"name\":\"\xE4\xB8\xAD\xE6\x96\x87\"}"});
        *error=reinterpret_cast<AEGP_MemHandle>(new Text{override_diagnostic?diagnostic_text:script_failure?"error":""});return script_failure?1:0;
    };
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle handle,AEGP_MemSize* size)->A_Err{*size=static_cast<AEGP_MemSize>(reinterpret_cast<Text*>(handle)->bytes.size()+1);return 0;};
    memory.AEGP_LockMemHandle=[](AEGP_MemHandle handle,void** output)->A_Err{*output=reinterpret_cast<Text*>(handle)->bytes.data();return 0;};
    memory.AEGP_UnlockMemHandle=[](AEGP_MemHandle)->A_Err{return 0;};
    memory.AEGP_FreeMemHandle=[](AEGP_MemHandle handle)->A_Err{delete reinterpret_cast<Text*>(handle);++freed;return 0;};
    SPBasicSuite sdk{};sdk.AcquireSuite=acquire;sdk.ReleaseSuite=release;
    initialize_ui_exclusion();initialize_web_panel_host(&sdk,8);
    clear_lookup_attempted=true; // Existing queue scenarios exclude the one-time menu lookup.
    check(!environment && !com_owned && !creating_environment && views.empty(),"headless registration never initializes COM or a browser");
    commands.AEGP_GetUniqueCommand=[](AEGP_Command* output)->A_Err{*output=7001;return 0;};
    commands.AEGP_InsertMenuCommand=[](AEGP_Command id,const A_char* name,AEGP_MenuID menu,A_long position)->A_Err{
        check(id==7001 && !std::strcmp(name,"Starfield test internal command") && menu==AEGP_Menu_NONE && position==AEGP_MENU_INSERT_SORTED,
            "production internal registration keeps its name and never inserts into Window");return 0;};
    commands.AEGP_EnableCommand=[](AEGP_Command id)->A_Err{check(id==7001,"hidden command enabled");return 0;};
    AEGP_RegisterSuite5 registrations{};
    registrations.AEGP_RegisterCommandHook=[](AEGP_PluginID id,AEGP_HookPriority priority,AEGP_Command command,AEGP_CommandHook,AEGP_CommandRefcon)->A_Err{
        check(id==8 && priority==AEGP_HP_BeforeAE && command==7001,"hidden command hook remains session resident");return 0;};
    check(register_hidden_native_command(&commands,&registrations,8,"Starfield test internal command",nullptr)==7001,"registered internal ID published");
    commands.AEGP_InsertMenuCommand=[](AEGP_Command,const A_char*,AEGP_MenuID,A_long)->A_Err{return 1;};
    check(!register_hidden_native_command(&commands,&registrations,8,"failed",nullptr),"failed hidden registration never publishes an executable ID");
    native_command_bindings={{{"Starfield Reveal Selected Effect",7001},{"Starfield Prepare Model Asset Export",7002},
        {"Starfield Apply Model Graph Transaction",7003},{"Starfield Choose Preset File",7004},
        {"Starfield Commit Node Graph",7005},{"Starfield Repair Selected Graph",7006}}};
    const auto prelude=native_command_script();
    check(prelude.find("\"Starfield Commit Node Graph\":7005")!=std::string::npos,"session map contains allocated IDs, no hardcoded frontend IDs");
    commands.AEGP_DoCommand=[](AEGP_Command id)->A_Err{check(id==7006,"diagnostic dispatch uses the repair binding only");++native_calls;return 0;};
    const auto test_window=CreateWindowExW(0,L"STATIC",L"",0,0,0,0,0,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
    check(test_window && !IsWindowVisible(test_window),"invisible message-only host stand-in created");
    if(!test_window)return 1;
    main_window=test_window;
    auto view=std::make_shared<View>();views.push_back(view);view->epoch=1;
    auto event=Microsoft::WRL::Make<WebMessage>();event->json=message(L"wv-1",L"中文 script");
    event->source=L"https://untrusted.invalid/index.html";view->receive(event.Get());check(requests.empty(),"untrusted origin cannot enqueue");
    event->source=view->url();view->receive(event.Get());check(requests.size()==1 && executions==0,"browser callback only enqueues owned strings");
    view->receive(event.Get());check(requests.size()==1,"duplicate admission never writes twice");
    {HostUiExclusion excluded;check(bool(excluded),"exclusion acquired");check(!step_web_panel_host() && executions==0 && requests.size()==1,"existing native transactions defer browser work");}
    std::thread worker([]{check(!step_web_panel_host() && executions==0,"worker rejects before host access");});worker.join();
    EnableWindow(test_window,FALSE);const auto reserved=queued_bytes;
    for(unsigned i=0;i<3;++i)check(!step_web_panel_host() && executions==0 && requests.size()==1 && queued_bytes==reserved && !quiet_starts,
        "modal host defers FIFO work without script calls, spinning, or losing reservations");
    EnableWindow(test_window,TRUE);scripting_available=false;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"host scripting unavailable defers without consuming work");scripting_available=true;
    window_lookup_failure=true;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"failed main-window lookup fails closed");window_lookup_failure=false;
    const auto valid_main=main_window;main_window=nullptr;
    check(!step_web_panel_host() && executions==0 && requests.size()==1,"missing main window defers shutdown work");main_window=valid_main;
    check(!step_web_panel_host() && executions==1 && last_script==prelude+"\xE4\xB8\xAD\xE6\x96\x87 script" && freed==2,"UI idle binds current IDs then executes exactly once and frees both AE handles");
    check(acquisitions==releases,"AE suites released after callback");
    event->json=message(L"wv-2",L"expired");view->receive(event.Get());requests.front().expires={};step_web_panel_host();check(executions==1,"expired edit cannot write later");
    event->json=message(L"wv-3",L"old document");view->receive(event.Get());++view->epoch;step_web_panel_host();check(executions==1,"navigation invalidates queued edits");
    for(unsigned i=4;i<68;++i){event->json=message(L"wv-"+std::to_wstring(i),L"ordered "+std::to_wstring(i));view->receive(event.Get());}
    check(requests.size()==64,"bounded queue accepts its exact capacity");event->json=message(L"wv-68",L"overflow");view->receive(event.Get());check(requests.size()==64,"count overflow rejects without mutation");
    while(!requests.empty())step_web_panel_host();check(executions==65 && executed[1]==prelude+"ordered 4" && executed.back()==prelude+"ordered 67" && queued_bytes==0,"FIFO execution and accounting");
    script_failure=true;event->json=message(L"wv-69",L"failing");view->receive(event.Get());step_web_panel_host();check(executions==66 && freed==132,"failed script still frees all memory");script_failure=false;
    check(wide("\xE4\xB8\xAD\xE6\x96\x87")==L"中文" && utf8(L"中文")=="\xE4\xB8\xAD\xE6\x96\x87","UTF-8 result roundtrip");
    for(unsigned i=70;i<74;++i){event->json=message(L"wv-"+std::to_wstring(i),std::wstring(1040000,L'x'));view->receive(event.Get());}
    check(requests.size()==4 && queued_bytes==8320000,"byte capacity is independent of count capacity");
    event->json=message(L"wv-74",std::wstring(1040000,L'x'));view->receive(event.Get());check(requests.size()==4 && queued_bytes==8320000,"byte overflow rejects atomically");
    for(auto& queued:requests)queued.expires={};while(!requests.empty())step_web_panel_host();check(queued_bytes==0 && executions==66,"expired large messages free their byte reservations");
    A_long snaps=5,items=0;check(!snap_sizes(nullptr,nullptr,&snaps) && snaps==0 && !flyout_items(nullptr,nullptr,&items) && items==2,"diagnostic flyout advertises its exact size");
    AEGP_FlyoutMenuItem menu[2]{};
    check(!flyout_items(nullptr,menu,&items) && menu[0].indent==1 && menu[1].indent==2 && menu[1].cmdID==1 && menu[1].enabledB,
        "repair belongs to the native panel Diagnostics submenu");
    items=1;check(flyout_items(nullptr,menu,&items)!=0,"short flyout allocation rejects without overflow");
    const auto panel_ref=reinterpret_cast<AEGP_PanelRefcon>(view.get());
    check(!flyout_command(panel_ref,1) && native_calls==1,"manual diagnostic repair reaches native callback");
    EnableWindow(test_window,FALSE);check(flyout_command(panel_ref,1)!=0 && native_calls==1,"modal host cannot run diagnostic repair");EnableWindow(test_window,TRUE);
    {HostUiExclusion excluded;check(flyout_command(panel_ref,1)!=0 && native_calls==1,"in-flight transaction blocks diagnostic repair");}
    check(flyout_command(nullptr,1)!=0 && native_calls==1,"stale panel ref cannot run repair");
    native_command_bindings[5].command=0;items=2;
    check(!flyout_items(nullptr,menu,&items) && !menu[1].enabledB && flyout_command(panel_ref,1)!=0,"unavailable repair is disabled and cannot execute command zero");
    check(quiet_starts==66 && quiet_ends==66 && !quiet_active,"success and script-error paths restore quiet-error state");
    quiet_start_failure=true;
    check(execute(L"must not run").find(L"host_busy")!=std::wstring::npos && executions==66 && quiet_starts==quiet_ends,
        "quiet-error setup failure never executes an unprotected script");quiet_start_failure=false;
    event->json=message(L"wv-75",L"expired during modal");view->receive(event.Get());EnableWindow(test_window,FALSE);
    requests.front().expires={};step_web_panel_host();check(executions==66 && requests.empty() && queued_bytes==0,"modal-expired edit is discarded without executing");
    EnableWindow(test_window,TRUE);step_web_panel_host();check(executions==66,"dismissed modal cannot replay an expired write");
    event->json=message(L"wv-76",L"closed");view->receive(event.Get());view->close();step_web_panel_host();check(executions==66,"closed view cannot apply queued edits");
    override_diagnostic=true;diagnostic_text=std::string(65536,'x');diagnostic_text.front()=0;
    check(execute(L"large empty diagnostic").find(L"\"ok\":true")!=std::wstring::npos,
        "large empty diagnostic allocation does not reject a successful script");
    diagnostic_text="Error: \xE4\xB8\xAD\xE6\x96\x87\nsecond line";
    const auto diagnostic_failure=execute(L"real diagnostic, zero status");
    check(diagnostic_failure.find(L"host_script_failed")!=std::wstring::npos && diagnostic_failure.find(L"error 0")!=std::wstring::npos &&
        diagnostic_failure.find(L"中文")!=std::wstring::npos && diagnostic_failure.find(L"\\u000asecond line")!=std::wstring::npos,
        "nonempty diagnostic still rejects and reports bounded escaped AE text/code");
    script_failure=true;diagnostic_text.clear();
    check(execute(L"nonzero status, empty diagnostic").find(L"error 1")!=std::wstring::npos,
        "nonzero SDK status rejects even with an empty diagnostic");script_failure=false;
    diagnostic_text=std::string(65536,'x');
    const auto long_failure=execute(L"large actual error");
    check(long_failure.find(L"host_script_failed")!=std::wstring::npos && long_failure.find(L"truncated")!=std::wstring::npos && long_failure.size()<1400,
        "large nonempty errors remain failures with bounded display text");
    diagnostic_text=std::string(1023,'x')+"\xE4\xB8\xAD";
    check(execute(L"UTF-8 boundary").find(L"host_script_failed")!=std::wstring::npos,
        "UTF-8 truncation cannot turn diagnostic handling into an exception or success");
    override_diagnostic=false;diagnostic_text.clear();
    {
        // The query values are controlled; this does not move real user focus.
        // Actual production queue/epoch/exception handling runs against Win32
        // message-only HWND lifetimes, with no AE/browser process.
        const auto graph_window=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,test_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        const auto controls=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,test_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        auto focused_view=std::make_shared<View>();focused_view->hwnd=graph_window;views.push_back(focused_view);
        const auto outside=CreateWindowExW(0,L"STATIC",L"",0,0,0,1,1,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        const auto child=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,graph_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        check(outside && child && graph_window && controls,"focus fixture has separate AE root, graph, graph child and ECW siblings");
        {
            using namespace focus_fixture;
            foreground=test_window;focused=controls;separate_gui_queue=false;gui_available=true;gui_flags=0;
            menu_hook(nullptr,nullptr,AEGP_WindType_EFFECT);check(effect_has_keyboard(),"positive current ECW child/context allows native execution");
            focused=test_window;check(!effect_has_keyboard(),"root-only focus cannot reuse a remembered Effect menu context");
            menu_hook(nullptr,nullptr,AEGP_WindType_EFFECT);check(!effect_has_keyboard(),"root-only menu update cannot authorize deletion");
            focused=controls;check(!effect_has_keyboard(),"new focus needs a matching menu observation");
            menu_hook(nullptr,nullptr,AEGP_WindType_EFFECT);gui_available=false;check(!effect_has_keyboard(),"failed GUI query never authorizes ECW");gui_available=true;
            focused=child;menu_hook(nullptr,nullptr,AEGP_WindType_EFFECT);check(!effect_has_keyboard(),"browser descendants never become ECW targets");
            focused=controls;menu_hook(nullptr,nullptr,AEGP_WindType_OTHER);check(!effect_has_keyboard(),"non-Effect SDK context never authorizes ECW");
            const auto editor=CreateWindowExW(0,L"EDIT",L"name",WS_CHILD,0,0,1,1,controls,nullptr,GetModuleHandleW(nullptr),nullptr);
            const auto editor_child=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,editor,nullptr,GetModuleHandleW(nullptr),nullptr);
            check(editor && editor_child,"native text fixture has actual Edit and nested HWNDs");
            clear_command=21;
            for(const auto text_focus:{editor,editor_child})for(const bool latched:{false,true}) {
                focused=text_focus;menu_hook(nullptr,nullptr,AEGP_WindType_EFFECT);author_delete_latched=latched;
                const auto reads=acquisitions;A_Boolean handled=FALSE;
                check(native_text_has_keyboard(),"native text focus includes editor ancestry");
                dispatch_web_panel_command(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);
                check(!handled && acquisitions==reads,"native text Clear passes through before capture, mutation or repeat latch");
            }
            author_delete_latched=false;if(editor_child)DestroyWindow(editor_child);if(editor)DestroyWindow(editor);
            focused=controls;check(!native_text_has_keyboard(),"ordinary native ECW child remains eligible for effect selection");
            active_menu_window=AEGP_WindType_NONE;effect_menu_focus=effect_menu_root=nullptr;
        }
        for(unsigned scenario=0;scenario<18;++scenario){
            using namespace focus_fixture;
            focused_view->alive=true;focused_view->epoch=100+scenario;focused_view->presets=scenario==6;
            focused_view->hwnd=scenario==16?child:graph_window;
            focused=scenario==7?controls:(scenario==14 || scenario==16)?child:graph_window;
            foreground=scenario==12?nullptr:scenario==15?outside:test_window;input_time=42;restores=0;input_available=scenario!=8;visible=scenario!=9;
            during_script=[&]{
                focused=scenario==11?graph_window:controls;
                if(scenario==2)++input_time;
                if(scenario==3)foreground=outside;
                if(scenario==4)++focused_view->epoch;
                if(scenario==5)focused_view->alive=false;
                if(scenario==13)EnableWindow(graph_window,FALSE);
                if(scenario==16)SetParent(child,outside);
                if(scenario==17)focused_view->hwnd=child;
                if(scenario==10)throw std::bad_alloc();
            };
            event->source=focused_view->url();
            event->json=message(L"wv-"+std::to_wstring(1000+scenario),L"selection",scenario==0?L"evalScript":L"evalScriptKeepGraphFocus");
            focused_view->receive(event.Get());check(requests.size()==1,"one typed focus request queued");
            step_web_panel_host();
            const bool expected=scenario==1 || scenario==10 || scenario==14;
            check(restores==unsigned(expected),"focus restoration is limited to an uninterrupted graph selection, including exception unwind");
            check(!expected || focused==graph_window,"native graph window regains keyboard ownership");
            check(requests.empty() && queued_bytes==0,"focus handling never retries a project script");
            EnableWindow(graph_window,TRUE);
            SetParent(child,graph_window);
        }
        focus_fixture::during_script={};focus_fixture::focused=nullptr;focus_fixture::foreground=nullptr;
        focused_view->hwnd=nullptr;if(child)DestroyWindow(child);if(outside)DestroyWindow(outside);
        if(graph_window)DestroyWindow(graph_window);if(controls)DestroyWindow(controls);
    }
    {
        using namespace focus_fixture;
        auto graph=std::make_shared<View>();graph->hwnd=test_window;views.push_back(graph);
        focused=test_window;foreground=test_window;visible=true;pressed_key=VK_DELETE;clear_command=21;
        event->source=graph->url();
        auto metadata=[&](unsigned id,const wchar_t* value){event->json=message(L"wv-"+std::to_wstring(id),value,L"graphInputState");graph->receive(event.Get());};
        metadata(2000,L"document-a|1|0");
        check(graph->input.page==L"document-a" && requests.empty(),"metadata is bounded plain state, not deferred scripting");
        const auto before=executions;
        auto clear=[&](AEGP_Command command=21,A_Boolean already=FALSE){A_Boolean handled=FALSE;clear_hook(nullptr,nullptr,command,AEGP_HP_BeforeAE,already,&handled);return handled;};
        check(clear() && graph->delete_pending && executions==before,"graph Clear is consumed before native deletion with no script in command hook");
        const auto payload=take_graph_delete(*graph);
        check(payload.find(L"graphDelete")!=std::wstring::npos && payload.find(L"document-a")!=std::wstring::npos && take_graph_delete(*graph).empty(),"notification delivered once with page/input context");
        check(clear() && !graph->delete_pending,"held key never repeats mutation");
        metadata(2001,L"document-a|2|1");
        check(clear() && !graph->delete_pending,"DOM-owned key never forwarded twice");
        metadata(2002,L"document-a|2|0");clear();metadata(2003,L"document-a|3|0");
        check(take_graph_delete(*graph).empty(),"later pointer/focus context invalidates pending key");
        for(const auto key:{VK_CONTROL,VK_MENU,VK_SHIFT,VK_LWIN,VK_RWIN}) {modifier=key;check(!clear(),"modified shortcuts keep AE ownership");}modifier=0;
        check(!clear(22) && !clear(21,TRUE),"other commands and already handled input untouched");
        pressed_key=0;check(!clear(),"menu/script Clear without Delete is untouched");pressed_key=VK_DELETE;
        graph->presets=true;check(!clear(),"preset windows do not acquire graph Clear");graph->presets=false;
        foreground=nullptr;check(!clear(),"background graph does not acquire Clear");foreground=test_window;
        focused=nullptr;check(!clear(),"AE Effect Controls focus retains native Clear");focused=test_window;
        metadata(2004,L"document-a|4|0");clear();graph->delete_expires={};check(take_graph_delete(*graph).empty(),"expired notification cannot delete later");
        metadata(2005,L"document-a|5|0");clear();++graph->epoch;check(take_graph_delete(*graph).empty(),"navigation invalidates native key");
        metadata(2006,L"document-a|6|0");{HostUiExclusion busy;check(clear() && !graph->delete_pending,"busy transaction consumes and drops without delayed retry");}
        metadata(2007,L"bad|7|0");check(graph->input.epoch==6,"malformed metadata cannot replace current context");
        metadata(2008,L"document-a|2|0");check(graph->input.epoch==6,"older input metadata rejects");
        metadata(2009,L"document-a|7|0");clear();++input_time;check(take_graph_delete(*graph).empty(),"any later native input invalidates pending Delete");
        metadata(2010,L"document-a|8|0");clear();graph->hwnd=nullptr;check(take_graph_delete(*graph).empty(),"replaced HWND invalidates pending Delete");graph->hwnd=test_window;
        pressed_key=0;clear_lookup_attempted=false;clear_command=0;
        const auto lookup_before=executions;step_web_panel_host();
        check(clear_command==21 && clear_lookup_attempted && executions==lookup_before+1,"public Clear lookup is a single safe idle script, no hardcoded production ID");
        step_web_panel_host();check(executions==lookup_before+1,"menu lookup is not repeated on every idle");pressed_key=VK_DELETE;
        std::thread command_worker([&]{check(!clear(),"worker command hook never accesses UI or AE");});command_worker.join();
        graph->hwnd=nullptr;pressed_key=0;focused=nullptr;foreground=nullptr;
    }
    {
        using namespace focus_fixture;
        auto graph=std::make_shared<View>();graph->hwnd=test_window;views.push_back(graph);
        const auto child=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,test_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        check(child!=nullptr,"cross-GUI fixture owns a real child HWND");
        separate_gui_queue=true;gui_focused=child;gui_active=foreground=test_window;focused=nullptr;visible=true;
        check(graph_has_keyboard(*graph),"foreground graph child in another GUI queue is recognized even when local GetFocus is null");
        const auto outside=CreateWindowExW(0,L"STATIC",L"",0,0,0,1,1,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        focused=outside;check(graph_has_keyboard(*graph),"local queue's stale focus cannot override foreground graph focus");
        focused=child;gui_focused=outside;check(!graph_has_keyboard(*graph),"stale local graph focus cannot override actual outside focus");gui_focused=child;
        gui_available=false;check(!graph_has_keyboard(*graph),"failed GUI queue query fails closed");gui_available=true;
        for(const auto flags:{GUI_INMENUMODE,GUI_SYSTEMMENUMODE,GUI_POPUPMENUMODE,GUI_INMOVESIZE}) {
            gui_flags=flags;check(!graph_has_keyboard(*graph),"foreground menu and move loops cannot own graph Delete");
        }gui_flags=0;
        gui_active=outside;check(!graph_has_keyboard(*graph),"foreground/active mismatch rejects transient activation");gui_active=test_window;
        graph->input={L"cross-gui-page",1,false};pressed_key=VK_DELETE;clear_command=21;A_Boolean handled=FALSE;
        clear_hook(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);
        check(handled && !take_graph_delete(*graph).empty(),"cross-GUI graph Clear is consumed and forwards once");
        graph->hwnd=child;pressed_key=0;restores=0;focused=nullptr;gui_focused=child;input_time=52;
        during_script=[&]{gui_focused=test_window;}; // AE root/ECW now has keyboard focus.
        event->source=graph->url();event->json=message(L"wv-9000",L"select across GUI queue",L"evalScriptKeepGraphFocus");graph->receive(event.Get());step_web_panel_host();
        check(restores==1 && focused==child,"cross-GUI selection retains the graph's pre-script keyboard ownership");
        during_script={};if(outside)DestroyWindow(outside);
        separate_gui_queue=false;gui_focused=gui_active=nullptr;graph->hwnd=nullptr;foreground=nullptr;
        if(child)DestroyWindow(child);
    }
    {
        using namespace focus_fixture;
        const auto graph_hwnd=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,test_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        const auto controls=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,test_window,nullptr,GetModuleHandleW(nullptr),nullptr);
        check(graph_hwnd && controls,"controller fixture owns graph and ECW siblings");
        if(!graph_hwnd || !controls)return 1;
        auto graph=std::make_shared<View>();graph->hwnd=graph_hwnd;views.push_back(graph);
        auto control=Microsoft::WRL::Make<FocusController>();graph->controller=control;graph->bind_focus_events();
        foreground=focused=test_window;visible=true;input_time=70;
        check(graph->focus_events && control->got && control->lost,"actual controller Got/Lost subscriptions installed together");
        check(!graph_has_keyboard(*graph),"root HWND alone is not graph ownership");
        control->fire(true);check(graph_has_keyboard(*graph),"controller GotFocus supplies graph ownership when native focus stays at AE root");
        graph->input={L"controller-page",1,false};pressed_key=VK_DELETE;clear_command=21;A_Boolean handled=FALSE;
        dispatch_web_panel_command(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);
        check(handled && graph->delete_pending,"actual Clear dispatcher consumes root-focused controller key");
        control->fire(false);check(!graph_has_keyboard(*graph) && take_graph_delete(*graph).empty(),"LostFocus revokes ownership and cancels an undelivered Delete");
        control->fire(true);focused=controls;check(!graph_has_keyboard(*graph),"explicit ECW child wins over an old controller focus signal");
        focused=test_window;graph->presets=true;check(!graph_has_keyboard(*graph),"presets never acquire graph Clear");graph->presets=false;
        gui_available=false;check(!graph_has_keyboard(*graph),"logical control focus cannot override failed native query");gui_available=true;
        control->fire(false);auto foreign=Microsoft::WRL::Make<FocusController>();control->got->Invoke(foreign.Get(),nullptr);
        check(!graph_has_keyboard(*graph),"foreign controller cannot claim this view's logical focus");
        std::thread focus_worker([&]{control->fire(true);});focus_worker.join();
        check(!graph->controller_focused,"worker GotFocus cannot mutate UI ownership");control->fire(true);
        auto other=std::make_shared<View>();other->hwnd=controls;auto other_control=Microsoft::WRL::Make<FocusController>();other->controller=other_control;views.push_back(other);other->bind_focus_events();
        graph->delete_pending=true;other_control->fire(true);
        check(!graph_has_keyboard(*graph) && !graph->delete_pending && graph_has_keyboard(*other),"new controller focus exclusively owns the root and drops the old view's pending Delete");
        control->fire(false);check(graph_has_keyboard(*other),"late old LostFocus cannot revoke the new controller's owner");
        other->close();other->hwnd=nullptr;control->fire(true);
        const auto detached_root=CreateWindowExW(0,L"STATIC",L"",0,0,0,1,1,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
        if(!detached_root)return 1;
        SetParent(graph_hwnd,detached_root);focused=foreground=detached_root;
        check(!graph_has_keyboard(*graph),"controller GotFocus from the old root cannot authorize a reparented view");
        SetParent(graph_hwnd,test_window);focused=foreground=test_window;if(detached_root)DestroyWindow(detached_root);
        control->fire(true);pressed_key=0;restores=0;
        during_script=[&]{control->fire(false);};
        event->source=graph->url();event->json=message(L"wv-11000",L"reveal node from root-focused controller",L"evalScriptKeepGraphFocus");graph->receive(event.Get());step_web_panel_host();
        check(restores==1 && focused==graph_hwnd,"node selection restores preexisting logical graph owner after host reveals ECW");during_script={};
        const auto late=control->got;graph->close();late->Invoke(control.Get(),nullptr);
        check(!graph->controller_focused && !graph->focus_events && control->closed && control->removed_got==1 && control->removed_lost==1,"close detaches subscriptions and rejects late GotFocus");
        for(unsigned failure=0;failure<2;++failure){auto v=std::make_shared<View>();auto c=Microsoft::WRL::Make<FocusController>();v->controller=c;
            c->fail_got=failure==0;c->fail_lost=failure==1;v->bind_focus_events();c->fire(true);
            check(!v->focus_events && !v->controller_focused,"partial event registration never grants logical focus");
            check(!failure || c->removed_got==1,"LostFocus subscription failure removes GotFocus handler");v->close();}
        graph->hwnd=nullptr;focused=foreground=nullptr;
        if(graph_hwnd)DestroyWindow(graph_hwnd);if(controls)DestroyWindow(controls);
    }
    {
        // The resident Host owns the sole command hook. Web initialization may
        // define its two menu IDs, but must not register additional callbacks.
        AEGP_Command menu_id=10000;
        registration_api=&registrations;suppressed=false;
        registrations.AEGP_RegisterCommandHook=[](AEGP_PluginID,AEGP_HookPriority,AEGP_Command,AEGP_CommandHook,AEGP_CommandRefcon)->A_Err{++panel_hook_registrations;return 0;};
        panel_api.AEGP_RegisterCreatePanelHook=[](AEGP_PluginID,const A_u_char*,AEGP_CreatePanelHook,AEGP_CreatePanelRefcon,A_Boolean)->A_Err{return 0;};
        panel_api.AEGP_ToggleVisibility=[](const A_u_char*)->A_Err{++panel_toggles;return 0;};
        registrations.AEGP_RegisterUpdateMenuHook=[](AEGP_PluginID,AEGP_UpdateMenuHook,AEGP_UpdateMenuRefcon)->A_Err{return 0;};
        commands.AEGP_GetUniqueCommand=[](AEGP_Command* output)->A_Err{static AEGP_Command id=10000;*output=++id;return 0;};
        commands.AEGP_InsertMenuCommand=[](AEGP_Command,const A_char*,AEGP_MenuID menu,A_long)->A_Err{++panel_menu_definitions;return menu==AEGP_Menu_WINDOW?0:1;};
        const auto before=executions;initialize_web_panel_host(&sdk,8);
        check(graph_command==menu_id+1 && presets_command==menu_id+2 && executions==before,"Web initialization defines distinct panel menu IDs without scripting");
        check(panel_hook_registrations==0 && panel_menu_definitions==2,"Web initialization never overwrites the sole resident command hook");
        A_Boolean handled=FALSE;dispatch_web_panel_command(nullptr,nullptr,graph_command,AEGP_HP_BeforeAE,FALSE,&handled);
        check(handled && panel_toggles==1,"sole Host dispatcher can still toggle graph panel through public adapter entry");
        set_web_panel_command_registration(17);check(clear_registration==17,"final resident registration status reaches Web diagnostics");
    }
    check(acquisitions==releases,"all paths release SDK suites");stop_web_panel_host();stop_ui_exclusion();
    check(requests.empty() && views.empty() && !environment,"shutdown clears all owned pending state");
    check(locate_content() && fs::canonical(content_root)==fs::canonical(fs::current_path()/L"web_panel"),
        "production content discovery resolves renamed web_panel from the real executable location without a browser");
    DestroyWindow(test_window);
    std::cout<<checks<<" checks, "<<failures<<" failures; actual WebPanelHost queue, no AE/browser process\n";
    return failures?1:0;
}

