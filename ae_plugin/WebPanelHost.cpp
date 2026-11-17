#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "WebPanelHost.hpp"
#include "WebPanelProtocol.hpp"
#include "NativeCommandBindings.hpp"
#include "ScriptDiagnostic.hpp"
#include "UiExclusionHost.hpp"
#include "GraphRenderJournal.hpp"
#include "NativeGraphEdit.hpp"
#include "AE_GeneralPlugPanels.h"
#include <windows.h>
#include <commctrl.h>
#include <WebView2.h>
#include <wrl.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
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
AEGP_Command clear_command{};
bool clear_lookup_attempted{};
AEGP_WindowType active_menu_window=AEGP_WindType_NONE;
HWND effect_menu_focus{},effect_menu_root{};
bool author_delete_latched{},author_delete_notice{};
unsigned native_clear_attempts{},native_clear_owned{};
NativeDeleteResult author_delete_result;
A_Err clear_registration=A_Err_GENERIC;
struct ClearObservation {unsigned count{},gate{};HWND local{},gui{},foreground{},graph{};bool del{},back{},control_focus{},focus_events{};};
ClearObservation clear_observation;
std::thread::id ui_thread;
bool stopped{},stepping{},creating_environment{},com_owned{};
fs::path content_root,profile_root;
ComPtr<ICoreWebView2Environment> environment;
struct View;
std::vector<std::shared_ptr<View>> views;
std::weak_ptr<View> controller_focus_owner;
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
    bool controller_focused{},focus_events{};
    HWND controller_focus_root{};
    EventRegistrationToken got_focus_token{},lost_focus_token{};
    std::uint64_t epoch{},last_request{};
    wire::GraphInput input;
    bool delete_latched{},delete_pending{},delete_backspace{};
    std::uint64_t delete_epoch{},delete_serial{},delete_input_epoch{};
    std::wstring delete_page;
    HWND delete_hwnd{},delete_root{};
    DWORD delete_input_time{};
    std::chrono::steady_clock::time_point delete_expires;
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2Controller> focus_controller;
    ComPtr<ICoreWebView2> web;
    void revoke_controller_focus() noexcept {
        controller_focused=false;controller_focus_root=nullptr;delete_pending=false;
        if(controller_focus_owner.lock().get()==this)controller_focus_owner.reset();
    }
    void bind_focus_events() {
        if(!controller || focus_events)return;
        const auto weak=weak_from_this();
        const auto got=controller->add_GotFocus(Callback<ICoreWebView2FocusChangedEventHandler>(
            [weak](ICoreWebView2Controller* sender,IUnknown*)->HRESULT {
                if(const auto view=weak.lock();view && view->alive && !stopped && std::this_thread::get_id()==ui_thread && view->focus_events && view->controller.Get()==sender && view->focus_controller.Get()==sender) {
                    if(const auto previous=controller_focus_owner.lock();previous && previous!=view)previous->revoke_controller_focus();
                    controller_focus_owner=view;view->controller_focused=true;
                    view->controller_focus_root=view->hwnd?GetAncestor(view->hwnd,GA_ROOT):nullptr;
                }
                return S_OK;
            }).Get(),&got_focus_token);
        if(FAILED(got))return;
        const auto lost=controller->add_LostFocus(Callback<ICoreWebView2FocusChangedEventHandler>(
            [weak](ICoreWebView2Controller* sender,IUnknown*)->HRESULT {
                if(const auto view=weak.lock();view && view->alive && !stopped && std::this_thread::get_id()==ui_thread && view->focus_events && view->controller.Get()==sender && view->focus_controller.Get()==sender) {
                    view->revoke_controller_focus();
                }
                return S_OK;
            }).Get(),&lost_focus_token);
        if(FAILED(lost)){controller->remove_GotFocus(got_focus_token);return;}
        focus_controller=controller;focus_events=true;
    }
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
        if(!alive)return;alive=false;++epoch;revoke_controller_focus();
        if(subclassed && parent){RemoveWindowSubclass(parent,parent_proc,reinterpret_cast<UINT_PTR>(this));subclassed=false;}
        if(focus_events && focus_controller){focus_controller->remove_GotFocus(got_focus_token);focus_controller->remove_LostFocus(lost_focus_token);}
        focus_events=false;focus_controller.Reset();
        if(controller){controller->Close();web.Reset();controller.Reset();}
    }
    void receive(ICoreWebView2WebMessageReceivedEventArgs* args) {
        if(!alive || stopped)return;
        CoString source,json;
        if(FAILED(args->get_Source(&source.value)) || !source.value || source.value!=url() ||
            FAILED(args->get_WebMessageAsJson(&json.value)) || !json.value)return;
        wire::Message message;
        if(!wire::parse(json.value,message))return;
        if(presets && (message.kind==L"nativeGraphDelete" || message.kind==L"nativeGraphPatch" || message.kind==L"nativeGraphDiagnostics"))return;
        // The adapter uses monotonically increasing IDs. Duplicate/replayed
        // messages never execute twice, even after the previous reply finished.
        if(message.id.substr(0,3)!=L"wv-")return;
        std::uint64_t serial{};
        for(std::size_t i=3;i<message.id.size();++i){const auto c=message.id[i];
            if(c<L'0'||c>L'9'||serial>900719925474099ULL)return;serial=serial*10+unsigned(c-L'0');}
        if(!serial || serial<=last_request)return;last_request=serial;
        if(message.kind==L"graphInputState") {
            // Plain bounded metadata only: never execute script or touch AE here.
            wire::GraphInput next;
            if(presets || !wire::parse_graph_input(message.script,next))return;
            if(input.page==next.page && next.epoch<input.epoch)return;
            if(input.page!=next.page || input.epoch!=next.epoch)delete_pending=false;
            input=std::move(next);
            if(!input.down)delete_latched=false;
            return;
        }
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
                else{++view->epoch;view->last_request=0;view->input={};view->delete_pending=false;view->delete_latched=false;}return S_OK;
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
                ++view->epoch;view->focus_events=false;view->revoke_controller_focus();view->error(L"The WebView2 process stopped. Close and reopen this panel; no edit will be retried automatically.");}return S_OK;}catch(...){return E_FAIL;}}).Get(),&token);
        web->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
            [weak](ICoreWebView2*,ICoreWebView2NavigationCompletedEventArgs* args)->HRESULT {try{if(const auto view=weak.lock()){
                BOOL success{};args->get_IsSuccess(&success);
                if(success && view->status)ShowWindow(view->status,SW_HIDE);
                else if(!success)view->error(L"Unable to load the bundled Starfield panel. Restore a verified paired build if reopening does not help.");}return S_OK;}catch(...){return E_FAIL;}}).Get(),&token);
        const auto bootstrap=L"window.__starfieldWebViewConfig=Object.freeze({nativeGraphDelete:true,root:"+wire::quote(content_root.generic_wstring())+L"});";
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
                    view->bind_focus_events();
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
A_Err flyout_items(AEGP_PanelRefcon,AEGP_FlyoutMenuItem* items,A_long* count) noexcept {
    if(!count)return A_Err_GENERIC;
    if(items) {
        if(*count<2)return A_Err_GENERIC;
        items[0]={1,AEGP_FlyoutMenuMarkType_NORMAL,TRUE,AEGP_FlyoutMenuCmdID_NONE,
            reinterpret_cast<const A_u_char*>("Diagnostics")};
        items[1]={2,AEGP_FlyoutMenuMarkType_NORMAL,static_cast<A_Boolean>(native_command_bindings[5].command>0),1,
            reinterpret_cast<const A_u_char*>("Repair Selected Graph")};
    }
    *count=2;return A_Err_NONE;
}
A_Err flyout_command(AEGP_PanelRefcon refcon,AEGP_FlyoutMenuCmdID command) noexcept {
    if(command!=1)return A_Err_NONE;
    if(stopped || std::this_thread::get_id()!=ui_thread || !ui_exclusion_available() || !host_accepts_panel_work())return A_Err_GENERIC;
    const auto found=std::find_if(views.begin(),views.end(),[&](const auto& view){return view.get()==reinterpret_cast<View*>(refcon) && view->alive && !view->presets;});
    if(found==views.end() || native_command_bindings[5].command<=0)return A_Err_GENERIC;
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    return commands.value?commands.value->AEGP_DoCommand(native_command_bindings[5].command):A_Err_GENERIC;
}
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
bool foreground_keyboard_focus(HWND,HWND&);
A_Err menu_hook(AEGP_GlobalRefcon,AEGP_UpdateMenuRefcon,AEGP_WindowType window) noexcept {
    active_menu_window=window;
    effect_menu_focus=effect_menu_root=nullptr;
    if(window==AEGP_WindType_EFFECT){const auto root=GetForegroundWindow();HWND focus{};
        if(foreground_keyboard_focus(root,focus) && focus!=root &&
            std::none_of(views.begin(),views.end(),[&](const auto& view){return view->alive && view->hwnd &&
                (focus==view->hwnd || IsChild(view->hwnd,focus));})){effect_menu_focus=focus;effect_menu_root=root;}}
    Suite<AEGP_CommandSuite1> commands(kAEGPCommandSuite,kAEGPCommandSuiteVersion1);
    if(!stopped && commands.value){if(graph_command)commands.value->AEGP_EnableCommand(graph_command);if(presets_command)commands.value->AEGP_EnableCommand(presets_command);}
    return A_Err_NONE;
}
bool foreground_keyboard_focus(HWND foreground,HWND& focus) {
    GUITHREADINFO info{};info.cbSize=sizeof(info);focus=nullptr;
    // GetFocus sees only this thread's queue. WebView2 input can belong to
    // another GUI queue: observe its foreground focus without attaching queues.
    if(!foreground || GetForegroundWindow()!=foreground || !GetGUIThreadInfo(0,&info) ||
        info.hwndActive!=foreground || (info.flags&(GUI_INMENUMODE|GUI_SYSTEMMENUMODE|GUI_POPUPMENUMODE|GUI_INMOVESIZE)) ||
        !info.hwndFocus || !IsWindow(info.hwndFocus) || GetAncestor(info.hwndFocus,GA_ROOT)!=foreground ||
        GetForegroundWindow()!=foreground)return false;
    focus=info.hwndFocus;return true;
}
bool graph_has_keyboard(const View& view) {
    const auto foreground=GetForegroundWindow();HWND focus{};
    return view.alive && !view.presets && view.hwnd && IsWindow(view.hwnd) && IsWindowVisible(view.hwnd) &&
        IsWindowEnabled(view.hwnd) && foreground && GetAncestor(view.hwnd,GA_ROOT)==foreground &&
        foreground_keyboard_focus(foreground,focus) &&
        (focus==view.hwnd || IsChild(view.hwnd,focus) ||
            (focus==foreground && view.focus_events && view.controller_focused && view.controller_focus_root==foreground &&
                controller_focus_owner.lock().get()==&view && view.controller && view.focus_controller.Get()==view.controller.Get()));
}
bool effect_has_keyboard() {
    HWND focus{};const auto root=GetForegroundWindow();
    return active_menu_window==AEGP_WindType_EFFECT && effect_menu_focus && effect_menu_root==root &&
        foreground_keyboard_focus(root,focus) && focus!=root && focus==effect_menu_focus &&
        std::none_of(views.begin(),views.end(),[&](const auto& view){return view->alive && view->hwnd &&
            (focus==view->hwnd || IsChild(view->hwnd,focus));});
}
bool native_text_has_keyboard() {
    const auto root=GetForegroundWindow();HWND focus{};
    if(!foreground_keyboard_focus(root,focus))return false;
    // AE may keep a whole-effect collection while a native text editor is
    // active. Let its local Clear run before considering that collection.
    for(unsigned depth=0;focus && focus!=root && depth<64;++depth,focus=GetParent(focus)) {
        wchar_t name[256]{};
        if(GetClassNameW(focus,name,static_cast<int>(std::size(name))) &&
            (!_wcsicmp(name,L"Edit") || !_wcsnicmp(name,L"RichEdit",8) ||
             !_wcsicmp(name,L"ComboBox") || !_wcsicmp(name,L"ComboBoxEx32")))return true;
    }
    return false;
}
bool key_down(int key){return (GetKeyState(key)&0x8000)!=0;}
A_Err clear_hook(AEGP_GlobalRefcon,AEGP_CommandRefcon,AEGP_Command command,AEGP_HookPriority,A_Boolean already,A_Boolean* handled) noexcept try {
    if(stopped || !handled || !clear_command || command!=clear_command || std::this_thread::get_id()!=ui_thread)return A_Err_NONE;
    ClearObservation observation;observation.count=1;observation.gate=1;
    observation.local=GetFocus();observation.foreground=GetForegroundWindow();
    (void)foreground_keyboard_focus(observation.foreground,observation.gui);
    struct Observe {ClearObservation& value;~Observe(){
        if(!clear_observation.count)clear_observation=value;
        else if(clear_observation.count<1000)++clear_observation.count;
    }} observed{observation};
    if(already)return A_Err_NONE;
    const bool del=key_down(VK_DELETE),back=key_down(VK_BACK);
    observation.del=del;observation.back=back;observation.gate=2;
    if(del==back || key_down(VK_CONTROL) || key_down(VK_MENU) || key_down(VK_SHIFT) || key_down(VK_LWIN) || key_down(VK_RWIN))return A_Err_NONE;
    observation.gate=3;
    for(const auto& view:views)if(view->alive && !view->presets && view->hwnd) {
        observation.graph=view->hwnd;observation.control_focus=view->controller_focused;observation.focus_events=view->focus_events;
        if(view->focus_events && view->controller_focused)break;
    }
    for(const auto& view:views)if(graph_has_keyboard(*view)) {
        // Consume before AE can remove an Effect Controls selection. A busy or
        // unavailable graph drops this key; it must never become a delayed edit.
        *handled=TRUE;
        observation.gate=4;
        if(view->delete_latched || view->input.down)return A_Err_NONE;
        view->delete_latched=true;
        observation.gate=5;
        if(stepping || !ui_exclusion_available() || !requests.empty() || view->input.page.empty())return A_Err_NONE;
        observation.gate=6;
        LASTINPUTINFO input{sizeof(LASTINPUTINFO),0};if(!GetLastInputInfo(&input))return A_Err_NONE;
        view->delete_hwnd=view->hwnd;view->delete_root=GetForegroundWindow();view->delete_input_time=input.dwTime;
        view->delete_page=view->input.page;view->delete_input_epoch=view->input.epoch;
        view->delete_epoch=view->epoch;view->delete_backspace=back;
        view->delete_expires=std::chrono::steady_clock::now()+std::chrono::milliseconds(500);
        view->delete_pending=true;observation.gate=7;return A_Err_NONE;
    }
    return A_Err_NONE;
}catch(...){return A_Err_NONE;}
std::wstring take_graph_delete(View& view) {
    if(!key_down(VK_DELETE) && !key_down(VK_BACK))view.delete_latched=false;
    if(!view.delete_pending)return {};
    view.delete_pending=false;
    LASTINPUTINFO input{sizeof(LASTINPUTINFO),0};
    if(!requests.empty() || std::chrono::steady_clock::now()>=view.delete_expires || !graph_has_keyboard(view) ||
        view.hwnd!=view.delete_hwnd || GetForegroundWindow()!=view.delete_root ||
        GetAncestor(view.hwnd,GA_ROOT)!=view.delete_root || !GetLastInputInfo(&input) || input.dwTime!=view.delete_input_time ||
        view.epoch!=view.delete_epoch || view.input.page!=view.delete_page ||
        view.input.epoch!=view.delete_input_epoch || view.input.down)return {};
    return L"{\"channel\":"+wire::quote(wire::channel)+L",\"event\":\"graphDelete\",\"page\":"+wire::quote(view.delete_page)+
        L",\"inputEpoch\":"+std::to_wstring(view.delete_input_epoch)+L",\"serial\":"+std::to_wstring(++view.delete_serial)+
        L",\"expires\":"+std::to_wstring(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()+250)+
        L",\"backspace\":"+(view.delete_backspace?L"true":L"false")+L"}";
}
std::wstring script_failure_message(const AEGP_MemorySuite1* memory,AEGP_MemHandle diagnostic,A_Err error) {
    const auto prefix=L"AE script failed (error "+std::to_wstring(static_cast<long>(error))+L"). ";
    if(!diagnostic)return prefix+L"No diagnostic text was returned.";
    if(!memory || !memory->AEGP_GetMemHandleSize || !memory->AEGP_LockMemHandle || !memory->AEGP_UnlockMemHandle)
        return prefix+L"Diagnostic memory access is unavailable.";
    AEGP_MemSize size{};
    if(memory->AEGP_GetMemHandleSize(diagnostic,&size))return prefix+L"Diagnostic size could not be read.";
    if(!size)return prefix+L"No diagnostic text was returned.";
    void* pointer{};
    if(memory->AEGP_LockMemHandle(diagnostic,&pointer))return prefix+L"Diagnostic text could not be locked.";
    struct Lock {const AEGP_MemorySuite1* memory;AEGP_MemHandle value;~Lock(){memory->AEGP_UnlockMemHandle(value);}} lock{memory,diagnostic};
    if(!pointer)return prefix+L"Diagnostic text is unavailable.";
    const auto* bytes=static_cast<const char*>(pointer);
    if(!*bytes)return prefix+L"No diagnostic text was returned.";
    const auto limit=std::min<std::size_t>(size,1024);
    const auto* end=static_cast<const char*>(std::memchr(bytes,0,limit));
    const auto length=static_cast<int>(end?static_cast<std::size_t>(end-bytes):limit);
    // Diagnostics are display text, not protocol data. An invalid/truncated
    // UTF-8 unit must not throw away the original failure or invent success.
    const auto count=MultiByteToWideChar(CP_UTF8,0,bytes,length,nullptr,0);
    if(count<=0)return prefix+L"Diagnostic text could not be decoded.";
    std::wstring text(static_cast<std::size_t>(count),L'\0');
    if(!MultiByteToWideChar(CP_UTF8,0,bytes,length,text.data(),count))return prefix+L"Diagnostic text could not be decoded.";
    return prefix+text+(end?L"":L" [truncated]");
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
    const auto script=native_command_script()+utf8(body);
    A_Err script_error{};
    {
        // Capture only this call's host errors. In particular a modal-state
        // rejection must not create another modal alert from background polls.
        QuietScriptErrors quiet(utility.value);
        if(!quiet.active)return failure(L"host_busy",L"AE cannot safely run the panel script. Refresh before retrying.");
        script_error=utility.value->AEGP_ExecuteScript(plugin,script.c_str(),FALSE,&result,&diagnostic);
    }
    if(script_error ||
        !script_diagnostic_empty(memory.value,diagnostic))return failure(L"host_script_failed",script_failure_message(memory.value,diagnostic,script_error));
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
    if(!commands.value->AEGP_GetUniqueCommand(&graph_command))
        commands.value->AEGP_InsertMenuCommand(graph_command,panel_menu,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_SORTED);
    if(!commands.value->AEGP_GetUniqueCommand(&presets_command))
        commands.value->AEGP_InsertMenuCommand(presets_command,presets_menu,AEGP_Menu_WINDOW,AEGP_MENU_INSERT_SORTED);
    (void)registrations.value->AEGP_RegisterUpdateMenuHook(id,menu_hook,nullptr);
}catch(...){}
A_Err dispatch_web_panel_command(AEGP_GlobalRefcon global,AEGP_CommandRefcon refcon,AEGP_Command command,
    AEGP_HookPriority priority,A_Boolean already,A_Boolean* handled) noexcept {
    if(stopped || already || !handled || std::this_thread::get_id()!=ui_thread)return A_Err_NONE;
    if(clear_command && command==clear_command && native_clear_dispatch_active())return A_Err_NONE;
    if((graph_command && command==graph_command) || (presets_command && command==presets_command))return command_hook(global,refcon,command,priority,already,handled);
    const auto error=clear_hook(global,refcon,command,priority,already,handled);
    if(!*handled && clear_command && command==clear_command &&
        std::none_of(views.begin(),views.end(),[](const auto& view){return graph_has_keyboard(*view);})) {
        if(native_text_has_keyboard())return error;
        if(author_delete_latched){*handled=TRUE;return error;}
        if(native_clear_attempts<1000)++native_clear_attempts;
        NativeDeleteResult result;
        // AE chooses its own actual Clear target. A whole native selection only
        // permits observing that command; it never authorizes choosing/deleting
        // an effect from root focus or a remembered menu type.
        if(delete_selected_native_nodes(basic,plugin,result,true,clear_command,handled)){
            if(native_clear_owned<1000)++native_clear_owned;
            *handled=TRUE;author_delete_latched=key_down(VK_DELETE)||key_down(VK_BACK);
            author_delete_result=result;author_delete_notice=true;
        }
    }
    return error;
}
void set_web_panel_command_registration(A_Err result) noexcept {clear_registration=result;}
bool step_web_panel_host() noexcept try {
    if(stopped || stepping || std::this_thread::get_id()!=ui_thread || !ui_exclusion_available())return false;
    SourceEditTelemetry observation;
    for(unsigned n=0;n<4 && take_source_edit(observation);++n){
        char detail[384]{};
        std::snprintf(detail,sizeof(detail),"Native source edit: kind=%u parameter=%u revision=%u totalUs=%llu compileUs=%llu queryUs=%llu fullNodeReadUs=%llu stateQueries=%u reusedNodes=%u misses=%u sourceFailure=%u.",
            observation.kind,observation.parameter,observation.revision,static_cast<unsigned long long>(observation.total_us),
            static_cast<unsigned long long>(observation.compile_us),static_cast<unsigned long long>(observation.query_us),static_cast<unsigned long long>(observation.full_read_us),
            observation.queries,observation.reused,observation.misses,observation.source_failure);
        journal_graph_command(0,observation.error,detail);
    }
    views.erase(std::remove_if(views.begin(),views.end(),[](const auto& view){return !view->alive;}),views.end());
    if(!key_down(VK_DELETE) && !key_down(VK_BACK))author_delete_latched=false;
    if(author_delete_notice){author_delete_notice=false;
        char detail[512]{};const auto& r=author_delete_result;
        std::snprintf(detail,sizeof(detail),"Native Clear result: stage=%ld, dispatched=%d, unchanged=%d, committed=%d, guarded=%d, unknown=%d, command=%ld, error=%ld, cleanup=%ld, undo=%ld; op=%llu c=%ld l=%ld rev=%ld totalUs=%llu scanUs=%llu captureUs=%llu physicalUs=%llu publishUs=%llu finalBindingsUs=%llu.",
            long(r.clear_stage),int(r.clear_dispatched),int(r.unchanged),int(r.committed),int(r.guarded),int(r.outcome_unknown),long(r.command_error),long(r.error),long(r.cleanup_error),long(r.undo_error),
            static_cast<unsigned long long>(r.operation),long(r.comp),long(r.layer),long(r.revision),static_cast<unsigned long long>(r.timing_us.total),
            static_cast<unsigned long long>(r.timing_us.preflight),static_cast<unsigned long long>(r.timing_us.capture),static_cast<unsigned long long>(r.timing_us.physical),
            static_cast<unsigned long long>(r.timing_us.publish),static_cast<unsigned long long>(r.timing_us.refresh));
        journal_graph_command(0,r.error?r.error:r.cleanup_error?r.cleanup_error:r.undo_error,detail);
        const auto result=native_delete_reply(author_delete_result);
        for(const auto& view:views)if(view->alive && !view->presets && view->web)
            (void)view->web->PostWebMessageAsJson((L"{\"channel\":"+wire::quote(wire::channel)+L",\"event\":\"nativeGraphEdited\",\"result\":"+result+L"}").c_str());
    }
    if(clear_observation.count) {
        // Keep the first observation until idle; repeats only increment a capped
        // count. No file I/O, script or AE suite call occurs in clear_hook.
        char detail[320]{};const auto observed=clear_observation;clear_observation={};
        std::snprintf(detail,sizeof(detail),"Panel Clear observed: count=%u, gate=%u, local=%p, gui=%p, foreground=%p, graph=%p, controlFocus=%d, focusEvents=%d, Delete=%d, Backspace=%d, nativeTry=%u, nativeOwned=%u.",
            observed.count,observed.gate,static_cast<void*>(observed.local),static_cast<void*>(observed.gui),static_cast<void*>(observed.foreground),static_cast<void*>(observed.graph),int(observed.control_focus),int(observed.focus_events),int(observed.del),int(observed.back),native_clear_attempts,native_clear_owned);
        native_clear_attempts=native_clear_owned=0;
        journal_graph_command(0,0,detail);
    }
    // Resolve the public menu name once, on safe idle, never inside a hook.
    // No private numeric Clear ID and no change to AE's keyboard preferences.
    if(!clear_lookup_attempted && host_accepts_panel_work()) {
        stepping=true;struct Reset {~Reset(){stepping=false;}} reset;
        HostUiExclusion lookup_scope;if(!lookup_scope)return false;
        clear_lookup_attempted=true;
        const auto text=execute(L"String(app.findMenuCommandId('Clear'))");
        unsigned value{};bool valid=!text.empty() && text.size()<10;
        for(const auto c:text){if(c<L'0'||c>L'9'){valid=false;break;}value=value*10+unsigned(c-L'0');}
        if(valid && value && value<=0x7fffffffU)clear_command=static_cast<AEGP_Command>(value);
        char detail[192]{};
        std::snprintf(detail,sizeof(detail),"Panel Clear lookup: command=%ld, hookRegistration=%ld; no numeric fallback.",static_cast<long>(clear_command),static_cast<long>(clear_registration));
        journal_graph_command(0,clear_command?clear_registration:A_Err_GENERIC,detail);
    }
    for(const auto& view:views) {
        if(!view->delete_pending){if(!key_down(VK_DELETE) && !key_down(VK_BACK))view->delete_latched=false;continue;}
        if(!host_accepts_panel_work()){view->delete_pending=false;continue;}
        const auto notification=take_graph_delete(*view);
        if(!notification.empty() && view->web) {
            journal_graph_command(0,0,"Panel Clear consumed; forwarding bounded graph Delete notification (not a commit receipt).");
            (void)view->web->PostWebMessageAsJson(notification.c_str());
        }
    }
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
        if(kind==L"evalScript" || kind==L"evalScriptKeepGraphFocus"){
            // Node selection may reveal AE Effect Controls. Keep a graph click's
            // keyboard owner only when no user input/window/navigation change
            // intervened. Ordinary scripts and preset views never claim focus.
            const auto foreground=GetForegroundWindow();HWND focused{};
            LASTINPUTINFO input{sizeof(LASTINPUTINFO),0};
            const bool keep=kind==L"evalScriptKeepGraphFocus" && !view->presets && view->hwnd &&
                foreground && GetAncestor(view->hwnd,GA_ROOT)==foreground &&
                foreground_keyboard_focus(foreground,focused) &&
                graph_has_keyboard(*view) &&
                GetLastInputInfo(&input);
            struct RestoreFocus {
                const std::shared_ptr<View>& view;std::uint64_t epoch;HWND hwnd,foreground;DWORD input_time;bool keep;
                ~RestoreFocus() noexcept {
                    LASTINPUTINFO current{sizeof(LASTINPUTINFO),0};HWND focused{};
                    if(keep && view->alive && view->epoch==epoch && view->hwnd==hwnd && hwnd && IsWindow(hwnd) &&
                        IsWindowVisible(view->hwnd) && IsWindowEnabled(view->hwnd) &&
                        GetForegroundWindow()==foreground && GetAncestor(hwnd,GA_ROOT)==foreground &&
                        foreground_keyboard_focus(foreground,focused) &&
                        GetLastInputInfo(&current) && current.dwTime==input_time &&
                        !graph_has_keyboard(*view))SetFocus(view->hwnd);
                }
            } restore{view,pending.epoch,view->hwnd,foreground,input.dwTime,keep};
            response=execute(pending.message.script);
        }
        else if(kind==L"nativeGraphDelete" || kind==L"nativeGraphPatch" || kind==L"nativeGraphDiagnostics"){
            const auto queued=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-(pending.expires-std::chrono::seconds(7))).count();
            char detail[128]{};std::snprintf(detail,sizeof(detail),"Native panel delete dispatch: queueWaitUs=%lld.",static_cast<long long>(queued));
            journal_graph_command(0,0,detail);
            HostUiExclusion edit_scope;
            response=edit_scope?execute_native_delete(basic,plugin,pending.message.script,kind==L"nativeGraphPatch",kind==L"nativeGraphDiagnostics"):failure(L"graph_transaction_busy",L"Another native edit is active. No graph operation was executed.");
        }
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
