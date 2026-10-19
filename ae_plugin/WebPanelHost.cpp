#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "WebPanelHost.hpp"
#include "WebPanelProtocol.hpp"
#include "ScriptDiagnostic.hpp"
#include "UiExclusionHost.hpp"
#include "AE_GeneralPlugPanels.h"
#include <windows.h>
#include <commctrl.h>
#include <WebView2.h>
#include <wrl.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <deque>
#include <filesystem>
#include <memory>
#include <string>
#include <stdexcept>
#include <thread>
#include <vector>

namespace starfield::adapter {
namespace {
using Microsoft::WRL::Callback;
using Microsoft::WRL::ComPtr;
namespace fs=std::filesystem;
namespace wire=web_panel;
constexpr char panel_match[]="org.starfieldfx.webview2.graph";
constexpr char panel_menu[]="Starfield Particle Controls (WebView2)";
constexpr char presets_menu[]="Starfield Presets (WebView2)";
constexpr wchar_t origin[]=L"https://starfield.invalid/";
constexpr wchar_t window_class[]=L"Starfield.WebView2.Panel.V1";
SPBasicSuite* basic{};
AEGP_PluginID plugin{};
AEGP_Command graph_command{},presets_command{};
std::thread::id ui_thread;
bool stopped{},stepping{},creating_environment{},com_owned{};
fs::path content_root,profile_root;
ComPtr<ICoreWebView2Environment> environment;
struct View;
std::vector<std::shared_ptr<View>> views;
struct Pending {
    std::weak_ptr<View> view;
    std::uint64_t epoch{};
    wire::Message message;
    std::chrono::steady_clock::time_point expires;
};
std::deque<Pending> requests;
std::size_t queued_bytes{};
template<class T> struct Suite {
    const char* name;A_long version;const T* value{};
    Suite(const char* n,A_long v):name(n),version(v){if(basic)basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct CoString {
    LPWSTR value{};
    ~CoString(){CoTaskMemFree(value);}
};
// AE can keep calling idle hooks from its modal save/quit message loop.
// Our transaction exclusion covers plug-in dialogs, not AE-owned dialogs.
bool host_accepts_panel_work() {
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    HWND main{};A_Boolean available{};
    return utility.value && utility.value->AEGP_GetMainHWND &&
        !utility.value->AEGP_GetMainHWND(&main) && main &&
        IsWindow(main) && IsWindowEnabled(main) &&
        utility.value->AEGP_IsScriptingAvailable &&
        !utility.value->AEGP_IsScriptingAvailable(&available) && available;
}
struct QuietScriptErrors {
    const AEGP_UtilitySuite6* utility;
    AEGP_ErrReportState state{};
    bool active{};
    explicit QuietScriptErrors(const AEGP_UtilitySuite6* value):utility(value) {
        active=utility && utility->AEGP_StartQuietErrors && utility->AEGP_EndQuietErrors &&
            !utility->AEGP_StartQuietErrors(&state);
    }
    ~QuietScriptErrors(){if(active)utility->AEGP_EndQuietErrors(FALSE,&state);}
};
std::wstring wide(std::string_view text) {
    if(text.empty())return {};
    const auto count=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0);
    if(!count)throw std::runtime_error("Invalid UTF-8 from AE");
    std::wstring result(static_cast<std::size_t>(count),L'\0');
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),count))throw std::runtime_error("UTF-8 conversion failed");
    return result;
}
std::string utf8(std::wstring_view text) {
    if(text.empty())return {};
    const auto count=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),nullptr,0,nullptr,nullptr);
    if(!count)throw std::runtime_error("Invalid UTF-16 request");
    std::string result(static_cast<std::size_t>(count),'\0');
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),static_cast<int>(text.size()),result.data(),count,nullptr,nullptr))throw std::runtime_error("UTF-16 conversion failed");
    return result;
}
std::wstring failure(std::wstring_view code,std::wstring_view message) {
    return L"{\"ok\":false,\"error\":{\"code\":"+wire::quote(code)+L",\"message\":"+wire::quote(message)+L"}}";
}
HMODULE own_module() {
    HMODULE module{};
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&initialize_web_panel_host),&module);
    return module;
}
bool locate_content() {
    std::wstring path(32768,L'\0');
    const auto count=GetModuleFileNameW(own_module(),path.data(),static_cast<DWORD>(path.size()));
    if(!count || count>=path.size())return false;path.resize(count);
    // canonical resolves the development Plug-ins/Starfield Junction. Never
    // derive a cache beside AfterFX.exe or silently fall back to APPDATA.
    auto directory=fs::canonical(path).parent_path();
    for(unsigned level=0;level<9 && !directory.empty();++level,directory=directory.parent_path()) {
        if(fs::is_regular_file(directory/L"AGENTS.md") && fs::is_regular_file(directory/L"web_panel/index.html") &&
            fs::is_regular_file(directory/L"web_panel/js/webview_host.js") &&
            fs::is_regular_file(directory/L"web_panel/jsx/starfield_gateway.jsx")) {
            content_root=directory/L"web_panel";profile_root=directory/L"artifacts/webview2/profile";return true;
        }
        if(directory==directory.parent_path())break;
    }
    return false;
}
void open_presets();
LRESULT CALLBACK view_proc(HWND,UINT,WPARAM,LPARAM);
LRESULT CALLBACK parent_proc(HWND,UINT,WPARAM,LPARAM,UINT_PTR,DWORD_PTR);

struct View:std::enable_shared_from_this<View> {
    HWND hwnd{},parent{},status{};
    bool presets{},alive=true,subclassed{};
    std::uint64_t epoch{},last_request{};
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> web;
    std::wstring url()const{return std::wstring(origin)+(presets?L"presets.html":L"index.html");}
    void error(std::wstring_view message) {
        if(!alive || !hwnd)return;
        if(!status)status=CreateWindowExW(0,L"STATIC",L"",WS_CHILD|WS_VISIBLE|SS_LEFT,12,12,450,160,hwnd,nullptr,own_module(),nullptr);
        if(status){SetWindowTextW(status,std::wstring(message).c_str());ShowWindow(status,SW_SHOW);}
    }
    void resize() {
        if(!alive || !hwnd)return;
        RECT rect{};GetClientRect(hwnd,&rect);
        if(status)MoveWindow(status,12,12,std::max(1L,rect.right-24),std::max(1L,rect.bottom-24),TRUE);
        if(controller){controller->put_Bounds(rect);controller->NotifyParentWindowPositionChanged();
            controller->put_IsVisible(IsWindowVisible(hwnd)?TRUE:FALSE);}
    }
    void reply(std::wstring_view id,std::wstring_view result) {
        if(!alive || !web || stopped)return;
        const auto json=L"{\"channel\":"+wire::quote(wire::channel)+L",\"id\":"+wire::quote(id)+L",\"result\":"+wire::quote(result)+L"}";
        (void)web->PostWebMessageAsJson(json.c_str());
    }
    void close() noexcept {
        if(!alive)return;alive=false;++epoch;
        if(subclassed && parent){RemoveWindowSubclass(parent,parent_proc,reinterpret_cast<UINT_PTR>(this));subclassed=false;}
        if(controller){controller->Close();web.Reset();controller.Reset();}
    }
    void receive(ICoreWebView2WebMessageReceivedEventArgs* args) {
        if(!alive || stopped)return;
        CoString source,json;
        if(FAILED(args->get_Source(&source.value)) || !source.value || source.value!=url() ||
            FAILED(args->get_WebMessageAsJson(&json.value)) || !json.value)return;
        wire::Message message;
        if(!wire::parse(json.value,message))return;
        // The adapter uses monotonically increasing IDs. Duplicate/replayed
        // messages never execute twice, even after the previous reply finished.
        if(message.id.substr(0,3)!=L"wv-")return;
        std::uint64_t serial{};
        for(std::size_t i=3;i<message.id.size();++i){const auto c=message.id[i];
            if(c<L'0'||c>L'9'||serial>900719925474099ULL)return;serial=serial*10+unsigned(c-L'0');}
        if(!serial || serial<=last_request)return;last_request=serial;
        const auto size=message.script.size()*sizeof(wchar_t);
        if(requests.size()>=wire::max_requests || size>wire::max_queued_bytes-queued_bytes) {
            reply(message.id,failure(L"host_busy",L"The native panel queue is full; refresh before retrying."));return;
        }
        requests.push_back({weak_from_this(),epoch,std::move(message),std::chrono::steady_clock::now()+std::chrono::seconds(7)});
        queued_bytes+=size;
    }
    void configure() {
        ComPtr<ICoreWebView2Settings> settings;
        if(SUCCEEDED(web->get_Settings(&settings)) && settings) {
            settings->put_AreDevToolsEnabled(FALSE);settings->put_AreDefaultContextMenusEnabled(FALSE);
            settings->put_IsStatusBarEnabled(FALSE);settings->put_IsZoomControlEnabled(FALSE);
            ComPtr<ICoreWebView2Settings3> newer;
            if(SUCCEEDED(settings.As(&newer)))newer->put_AreBrowserAcceleratorKeysEnabled(FALSE);
        }
        ComPtr<ICoreWebView2_3> mapping;
        if(FAILED(web.As(&mapping)) || FAILED(mapping->SetVirtualHostNameToFolderMapping(L"starfield.invalid",content_root.c_str(),COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_DENY))) {
            error(L"WebView2 local-content mapping is unavailable. Restore a verified paired build if reopening does not help.");return;
        }
        const auto weak=weak_from_this();EventRegistrationToken token{};
        web->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [weak](ICoreWebView2*,ICoreWebView2NavigationStartingEventArgs* args)->HRESULT {try {
                const auto view=weak.lock();CoString uri;
                if(!view || !view->alive || FAILED(args->get_Uri(&uri.value)) || !uri.value || uri.value!=view->url())args->put_Cancel(TRUE);
                else{++view->epoch;view->last_request=0;}return S_OK;
            }catch(...){args->put_Cancel(TRUE);return S_OK;}}).Get(),&token);
        web->add_FrameNavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
            [](ICoreWebView2*,ICoreWebView2NavigationStartingEventArgs* args)->HRESULT {args->put_Cancel(TRUE);return S_OK;}).Get(),&token);
        web->add_NewWindowRequested(Callback<ICoreWebView2NewWindowRequestedEventHandler>(
            [](ICoreWebView2*,ICoreWebView2NewWindowRequestedEventArgs* args)->HRESULT {args->put_Handled(TRUE);return S_OK;}).Get(),&token);
        web->add_PermissionRequested(Callback<ICoreWebView2PermissionRequestedEventHandler>(
            [](ICoreWebView2*,ICoreWebView2PermissionRequestedEventArgs* args)->HRESULT {args->put_State(COREWEBVIEW2_PERMISSION_STATE_DENY);return S_OK;}).Get(),&token);
        web->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
            [weak](ICoreWebView2*,ICoreWebView2WebMessageReceivedEventArgs* args)->HRESULT {try{if(const auto view=weak.lock())view->receive(args);return S_OK;}catch(...){return E_FAIL;}}).Get(),&token);
        web->add_ProcessFailed(Callback<ICoreWebView2ProcessFailedEventHandler>(
            [weak](ICoreWebView2*,ICoreWebView2ProcessFailedEventArgs*)->HRESULT {try{if(const auto view=weak.lock()){
                ++view->epoch;view->error(L"The WebView2 process stopped. Close and reopen this panel; no edit will be retried automatically.");}return S_OK;}catch(...){return E_FAIL;}}).Get(),&token);
        web->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [weak](ICoreWebView2*,ICoreWebView2NavigationCompletedEventArgs* args)->HRESULT {try{if(const auto view=weak.lock()){
                BOOL success{};args->get_IsSuccess(&success);
                if(success && view->status)ShowWindow(view->status,SW_HIDE);
                else if(!success)view->error(L"Unable to load the bundled Starfield panel. Restore a verified paired build if reopening does not help.");}return S_OK;}catch(...){return E_FAIL;}}).Get(),&token);
        const auto bootstrap=L"window.__starfieldWebViewConfig=Object.freeze({root:"+wire::quote(content_root.generic_wstring())+L"});";
        const auto result=web->AddScriptToExecuteOnDocumentCreated(bootstrap.c_str(),
            Callback<ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [weak](HRESULT code,LPCWSTR)->HRESULT {try{if(const auto view=weak.lock();view && view->alive && !stopped){
                    if(FAILED(code) || FAILED(view->web->Navigate(view->url().c_str())))view->error(L"The WebView2 bridge could not initialize.");}return S_OK;}catch(...){return E_FAIL;}}).Get());
        if(FAILED(result))error(L"The WebView2 bridge could not initialize.");
        resize();
    }
    void create_controller() {
        if(!alive || stopped || !environment || controller)return;
        const auto weak=weak_from_this();
        const auto code=environment->CreateCoreWebView2Controller(hwnd,
            Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [weak](HRESULT result,ICoreWebView2Controller* value)->HRESULT {try{
                    const auto view=weak.lock();if(!view || !view->alive || stopped){if(value)value->Close();return S_OK;}
                    if(FAILED(result) || !value){view->error(L"WebView2 controller creation failed. Reopen the panel to retry.");return S_OK;}
                    view->controller=value;
                    if(FAILED(value->get_CoreWebView2(&view->web)) || !view->web){view->error(L"WebView2 content initialization failed.");return S_OK;}
                    view->configure();return S_OK;
                }catch(...){return E_FAIL;}}).Get());
        if(FAILED(code))error(L"WebView2 controller creation could not start.");
    }
};

void ensure_environment(const std::shared_ptr<View>& view) {
    if(environment){view->create_controller();return;}
    if(creating_environment)return;
    if(!com_owned){const auto result=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
        if(FAILED(result)){view->error(L"AE's panel thread is not compatible with WebView2 STA. Restore a verified paired build if reopening does not help.");return;}com_owned=true;}
    if(!locate_content()){view->error(L"The paired Starfield HTML/JSX sources were not found in this checkout. Restore a verified paired build if reopening does not help.");return;}
    fs::create_directories(profile_root);
    creating_environment=true;
    const auto result=CreateCoreWebView2EnvironmentWithOptions(nullptr,profile_root.c_str(),nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [](HRESULT code,ICoreWebView2Environment* value)->HRESULT {try{
                creating_environment=false;if(stopped)return S_OK;
                if(FAILED(code) || !value){for(const auto& current:views)current->error(L"Microsoft WebView2 Runtime is unavailable. No runtime was installed; Restore a verified paired build if reopening does not help.");return S_OK;}
                environment=value;for(const auto& current:views)current->create_controller();return S_OK;
            }catch(...){return E_FAIL;}}).Get());
    if(FAILED(result)){creating_environment=false;view->error(L"WebView2 Runtime could not start. Restore a verified paired build if reopening does not help.");}
}
LRESULT CALLBACK view_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam) {
    auto* view=reinterpret_cast<View*>(GetWindowLongPtrW(hwnd,GWLP_USERDATA));
    if(message==WM_NCCREATE){view=static_cast<View*>(reinterpret_cast<CREATESTRUCTW*>(lparam)->lpCreateParams);
        SetWindowLongPtrW(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(view));view->hwnd=hwnd;}
    try {
        if(view)switch(message) {
        case WM_SIZE:case WM_MOVE:case WM_SHOWWINDOW:case WM_WINDOWPOSCHANGED:view->resize();break;
        case WM_SETFOCUS:if(view->controller)view->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);break;
        case WM_GETMINMAXINFO:if(view->presets){auto* info=reinterpret_cast<MINMAXINFO*>(lparam);info->ptMinTrackSize={480,450};return 0;}break;
        case WM_NCDESTROY:view->close();view->hwnd=nullptr;SetWindowLongPtrW(hwnd,GWLP_USERDATA,0);break;
        default:break;
        }
    }catch(...){}
    return DefWindowProcW(hwnd,message,wparam,lparam);
}
LRESULT CALLBACK parent_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,UINT_PTR id,DWORD_PTR ref) {
    auto* view=reinterpret_cast<View*>(ref);
    try {
        if(message==WM_NCDESTROY){RemoveWindowSubclass(hwnd,parent_proc,id);view->subclassed=false;view->close();}
        else if(message==WM_SIZE || message==WM_SHOWWINDOW || message==WM_WINDOWPOSCHANGED) {
            RECT rect{};GetClientRect(hwnd,&rect);if(view->hwnd)MoveWindow(view->hwnd,0,0,rect.right,rect.bottom,TRUE);view->resize();
        }else if(message==WM_SETFOCUS && view->controller)view->controller->MoveFocus(COREWEBVIEW2_MOVE_FOCUS_REASON_PROGRAMMATIC);
    }catch(...){}
    return DefSubclassProc(hwnd,message,wparam,lparam);
}
std::shared_ptr<View> make_view(HWND parent,bool presets) {
    if(views.size()>=16)throw std::runtime_error("Too many Starfield views");
    WNDCLASSEXW cls{};cls.cbSize=sizeof(cls);cls.lpfnWndProc=view_proc;cls.hInstance=own_module();
    cls.hCursor=LoadCursorW(nullptr,IDC_ARROW);cls.hbrBackground=static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH));cls.lpszClassName=window_class;
    if(!RegisterClassExW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Window class registration failed");
    auto view=std::make_shared<View>();view->parent=parent;view->presets=presets;
    const auto style=presets?WS_OVERLAPPEDWINDOW:WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS;
    if(!CreateWindowExW(0,window_class,presets?L"Starfield Presets":L"Starfield Particle Controls",style|WS_VISIBLE,
        presets?CW_USEDEFAULT:0,presets?CW_USEDEFAULT:0,presets?1040:580,presets?700:400,parent,nullptr,own_module(),view.get()))
        throw std::runtime_error("Panel window creation failed");
    views.push_back(view);
    if(!presets){view->subclassed=SetWindowSubclass(parent,parent_proc,reinterpret_cast<UINT_PTR>(view.get()),reinterpret_cast<DWORD_PTR>(view.get()))!=FALSE;
        if(!view->subclassed){view->error(L"The dockable panel resize hook could not initialize.");return view;}
        RECT rect{};GetClientRect(parent,&rect);MoveWindow(view->hwnd,0,0,rect.right,rect.bottom,TRUE);}
    view->error(L"Loading Starfield WebView2...");ensure_environment(view);return view;
}
void open_presets() {
    for(const auto& view:views)if(view->alive && view->presets){ShowWindow(view->hwnd,SW_RESTORE);SetForegroundWindow(view->hwnd);return;}
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);HWND owner{};
    if(utility.value)utility.value->AEGP_GetMainHWND(&owner);
    (void)make_view(owner,true);
}
A_Err snap_sizes(AEGP_PanelRefcon,A_LPoint*,A_long* count) noexcept {if(count)*count=0;return A_Err_NONE;}
A_Err flyout_items(AEGP_PanelRefcon,AEGP_FlyoutMenuItem*,A_long* count) noexcept {if(count)*count=0;return A_Err_NONE;}
A_Err flyout_command(AEGP_PanelRefcon,AEGP_FlyoutMenuCmdID) noexcept {return A_Err_NONE;}
A_Err create_panel(AEGP_GlobalRefcon,AEGP_CreatePanelRefcon,AEGP_PlatformViewRef container,
    AEGP_PanelH panel,AEGP_PanelFunctions1* functions,AEGP_PanelRefcon* refcon) noexcept try {
    if(stopped || std::this_thread::get_id()!=ui_thread || !container || !functions || !refcon)return A_Err_GENERIC;
    *functions={snap_sizes,flyout_items,flyout_command};*refcon=nullptr;
    const auto view=make_view(container,false);*refcon=reinterpret_cast<AEGP_PanelRefcon>(view.get());
    Suite<AEGP_PanelSuite1> panels(kAEGPPanelSuite,kAEGPPanelSuiteVersion1);
    if(panels.value)panels.value->AEGP_SetTitle(panel,reinterpret_cast<const A_u_char*>("Starfield Particle Controls"));
    return A_Err_NONE;
}catch(...){return A_Err_GENERIC;}
A_Err command_hook(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,AEGP_HookPriority,A_Boolean,A_Boolean* handled) noexcept try {
    if(command!=graph_command && command!=presets_command)return A_Err_NONE;
    if(handled)*handled=TRUE;
    if(stopped || std::this_thread::get_id()!=ui_thread)return A_Err_NONE;
    if(command==presets_command)open_presets();
    else {Suite<AEGP_PanelSuite1> panels(kAEGPPanelSuite,kAEGPPanelSuiteVersion1);
        if(panels.value)panels.value->AEGP_ToggleVisibility(reinterpret_cast<const A_u_char*>(panel_match));}
    return A_Err_NONE;
}catch(...){return A_Err_NONE;}
A_Err menu_hook(AEGP_GlobalRefcon,AEGP_UpdateMenuRefcon,AEGP_WindowType) noexcept {
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    if(!stopped && commands.value){if(graph_command)commands.value->AEGP_EnableCommand(graph_command);if(presets_command)commands.value->AEGP_EnableCommand(presets_command);}
    return A_Err_NONE;
}
std::wstring execute(std::wstring_view body) {
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_MemorySuite1> memory(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    A_Boolean available{};
    if(!utility.value || !memory.value || utility.value->AEGP_IsScriptingAvailable(&available) || !available)
        return failure(L"no_host",L"AE scripting is unavailable.");
    AEGP_MemHandle result{},diagnostic{};
    struct Handles {const AEGP_MemorySuite1* memory;AEGP_MemHandle& result;AEGP_MemHandle& diagnostic;
        ~Handles(){if(result)memory->AEGP_FreeMemHandle(result);if(diagnostic)memory->AEGP_FreeMemHandle(diagnostic);}} handles{memory.value,result,diagnostic};
    const auto script=utf8(body);
    A_Err script_error{};
    {
        // Capture only this call's host errors. In particular a modal-state
        // rejection must not create another modal alert from background polls.
        QuietScriptErrors quiet(utility.value);
        if(!quiet.active)return failure(L"host_busy",L"AE cannot safely run the panel script. Refresh before retrying.");
        script_error=utility.value->AEGP_ExecuteScript(plugin,script.c_str(),FALSE,&result,&diagnostic);
    }
    if(script_error ||
        !script_diagnostic_empty(memory.value,diagnostic))return failure(L"host_script_failed",L"AE rejected the panel script. Refresh before retrying.");
    if(!result)return L"undefined";
    AEGP_MemSize size{};
    if(memory.value->AEGP_GetMemHandleSize(result,&size) || size>8*1024*1024)return failure(L"size_limit_exceeded",L"AE returned an oversized response.");
    if(!size)return L"";
    void* pointer{};
    if(memory.value->AEGP_LockMemHandle(result,&pointer) || !pointer)return failure(L"host_read_failed",L"AE did not return readable data.");
    struct Lock {const AEGP_MemorySuite1* memory;AEGP_MemHandle handle;~Lock(){memory->AEGP_UnlockMemHandle(handle);}} lock{memory.value,result};
    const auto* bytes=static_cast<const char*>(pointer);const auto* end=static_cast<const char*>(std::memchr(bytes,0,size));
    if(!end)return failure(L"bad_response",L"AE returned an unterminated response.");
    return wide(std::string_view(bytes,static_cast<std::size_t>(end-bytes)));
}
}

void initialize_web_panel_host(SPBasicSuite* suites,AEGP_PluginID id) noexcept try {
    basic=suites;plugin=id;ui_thread=std::this_thread::get_id();stopped=false;
    Suite<AEGP_UtilitySuite6> utility(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);A_Boolean suppressed=TRUE;
    if(!utility.value || utility.value->AEGP_GetSuppressInteractiveUI(&suppressed) || suppressed)return;
    Suite<AEGP_PanelSuite1> panels(kAEGPPanelSuite,kAEGPPanelSuiteVersion1);
    Suite<AEGP_RegisterSuite5> registrations(kAEGPRegisterSuite,kAEGPRegisterSuiteVersion5);
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    if(!panels.value || !registrations.value || !commands.value)return;
    if(panels.value->AEGP_RegisterCreatePanelHook(id,reinterpret_cast<const A_u_char*>(panel_match),create_panel,nullptr,TRUE))return;
    if(!commands.value->AEGP_GetUniqueCommand(&graph_command) &&
        !registrations.value->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,graph_command,command_hook,nullptr))
        commands.value->AEGP_InsertMenuCommand(graph_command,panel_menu,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM);
    if(!commands.value->AEGP_GetUniqueCommand(&presets_command) &&
        !registrations.value->AEGP_RegisterCommandHook(id,AEGP_HP_BeforeAE,presets_command,command_hook,nullptr))
        commands.value->AEGP_InsertMenuCommand(presets_command,presets_menu,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_AT_BOTTOM);
    (void)registrations.value->AEGP_RegisterUpdateMenuHook(id,menu_hook,nullptr);
}catch(...){}
bool step_web_panel_host() noexcept try {
    if(stopped || stepping || std::this_thread::get_id()!=ui_thread || !ui_exclusion_available())return false;
    views.erase(std::remove_if(views.begin(),views.end(),[](const auto& view){return !view->alive;}),views.end());
    if(requests.empty())return false;
    stepping=true;struct Stepping {~Stepping(){stepping=false;}} scope;
    const auto view=requests.front().view.lock();
    const bool stale=!view || !view->alive || view->epoch!=requests.front().epoch;
    const bool expired=std::chrono::steady_clock::now()>=requests.front().expires;
    // Retain FIFO/byte accounting while AE is modal, but still discard stale
    // and expired work. Never spin an immediate idle retry while blocked.
    if(!stale && !expired && !host_accepts_panel_work())return false;
    auto pending=std::move(requests.front());requests.pop_front();queued_bytes-=pending.message.script.size()*sizeof(wchar_t);
    if(stale)return !requests.empty();
    if(expired){view->reply(pending.message.id,failure(L"host_timeout",L"The queued edit expired without executing. Refresh before retrying."));return !requests.empty();}
    std::wstring response=L"{\"ok\":true}";
    try {
        const auto& kind=pending.message.kind;
        if(kind==L"evalScript")response=execute(pending.message.script);
        else if(kind==L"undo" || kind==L"redo")response=execute(kind==L"undo"?L"app.executeCommand(16); 'ok'":L"app.executeCommand(2035); 'ok'");
        else if(kind==L"openPresets")open_presets();
        else if(kind==L"close"){
            if(view->presets)DestroyWindow(view->hwnd);
            else{Suite<AEGP_PanelSuite1> panels(kAEGPPanelSuite,kAEGPPanelSuiteVersion1);if(panels.value)panels.value->AEGP_ToggleVisibility(reinterpret_cast<const A_u_char*>(panel_match));}
        }
    }catch(...){response=failure(L"host_failed",L"The native panel request failed. No automatic retry was made.");}
    if(view->alive && view->epoch==pending.epoch)view->reply(pending.message.id,response);
    return !requests.empty();
}catch(...){stepping=false;return false;}
void stop_web_panel_host() noexcept {
    stopped=true;requests.clear();queued_bytes=0;
    for(const auto& view:views){view->close();if(view->hwnd)DestroyWindow(view->hwnd);}
    views.clear();environment.Reset();
    if(com_owned){CoUninitialize();com_owned=false;}
}
}
