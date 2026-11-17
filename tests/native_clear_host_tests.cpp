// Actual Host dispatcher + actual native Clear executor. Hidden test HWNDs,
// fake public SDK boundary; no AE, real focus or browser process is operated.
#include "EffectGraphBackup.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#include "NativeGraphEdit.hpp"
#include "GraphCommitRequest.hpp"
#include "GraphBindingRefreshRequest.hpp"
#include "NativeClearRequest.hpp"
#include "UiExclusionHost.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <stdexcept>
#include <vector>
namespace clear_fixture {
#define main native_edit_fixture_entry
#define basic edit_sdk
#include "native_graph_edit_tests.cpp"
#undef basic
#undef main
}
#undef CHECK
#define CHECK(x) do{++clear_fixture::checks;if(!(x)){std::cerr<<"check "<<clear_fixture::checks<<" at "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
namespace clear_focus {
HWND root{},focus{};
HWND foreground(){return root;}
HWND focused(){return focus;}
BOOL gui(DWORD,PGUITHREADINFO p){p->hwndActive=root;p->hwndFocus=focus;p->flags=0;return TRUE;}
BOOL visible(HWND h){return ::IsWindow(h);}
SHORT key(int v){return v==VK_DELETE?SHORT(-32768):0;}
}
#define STARFIELD_WITH_WEBVIEW2
#define STARFIELD_TEST_NO_JOURNAL
#define GetForegroundWindow clear_focus::foreground
#define GetFocus clear_focus::focused
#define GetGUIThreadInfo clear_focus::gui
#define IsWindowVisible clear_focus::visible
#define GetKeyState clear_focus::key
#include "../ae_plugin/WebPanelHost.cpp"
#undef GetForegroundWindow
#undef GetFocus
#undef GetGUIThreadInfo
#undef IsWindowVisible
#undef GetKeyState
namespace {
decltype(AEGP_CommandSuite1::AEGP_DoCommand) ae_clear{};
unsigned inner_passes{};
}
int main(){
    namespace host=starfield::adapter;
    host::initialize_ui_exclusion();clear_fixture::init_edit();
    host::basic=&clear_fixture::edit_sdk;host::plugin=7;host::stopped=false;host::ui_thread=std::this_thread::get_id();host::clear_command=21;
    clear_focus::root=CreateWindowExW(0,L"STATIC",L"",0,0,0,1,1,HWND_MESSAGE,nullptr,GetModuleHandleW(nullptr),nullptr);
    CHECK(clear_focus::root);clear_focus::focus=clear_focus::root;
    const auto graph_hwnd=CreateWindowExW(0,L"STATIC",L"",WS_CHILD,0,0,1,1,clear_focus::root,nullptr,GetModuleHandleW(nullptr),nullptr);
    CHECK(graph_hwnd);auto graph=std::make_shared<host::View>();graph->hwnd=graph_hwnd;host::views.push_back(graph);
    ae_clear=clear_fixture::edit_commands.AEGP_DoCommand;
    clear_fixture::edit_commands.AEGP_DoCommand=[](AEGP_Command command)->A_Err{
        A_Boolean inner=FALSE;const auto reads=clear_fixture::calls;
        host::dispatch_web_panel_command(nullptr,nullptr,command,AEGP_HP_BeforeAE,FALSE,&inner);
        CHECK(!inner && clear_fixture::calls==reads);++inner_passes;return ae_clear(command);
    };
    for(const int mode:{0,1}){
        clear_fixture::setup();clear_fixture::request({2});clear_fixture::clear_mode=mode;
        AEGP_CollectionItemV2 item{};item.type=AEGP_CollectionItemType_EFFECT;item.u.effect.layerH=clear_fixture::layer();item.u.effect.index=2;clear_fixture::selected_items.push_back(item);
        host::active_menu_window=AEGP_WindType_OTHER;host::author_delete_latched=false;host::author_delete_notice=false;
        A_Boolean outer=FALSE;clear_fixture::outer_clear_handled=&outer;
        host::dispatch_web_panel_command(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&outer);
        CHECK(outer && clear_fixture::clear_calls==1 && host::author_delete_notice && !host::native_clear_dispatch_active());
        if(!mode)CHECK(host::author_delete_result.committed && !host::author_delete_result.guarded && clear_fixture::parade.size()==4);
        else CHECK(host::author_delete_result.unchanged && !host::author_delete_result.committed && clear_fixture::parade.size()==5);
        clear_fixture::outer_clear_handled=nullptr;clear_fixture::no_owned();
    }
    CHECK(inner_passes==2);
    const auto text=CreateWindowExW(0,L"EDIT",L"value",WS_CHILD,0,0,1,1,clear_focus::root,nullptr,GetModuleHandleW(nullptr),nullptr);
    CHECK(text);clear_focus::focus=text;host::author_delete_latched=true;
    const auto calls_before=clear_fixture::calls;A_Boolean handled=FALSE;
    host::dispatch_web_panel_command(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);
    CHECK(!handled && clear_fixture::calls==calls_before && inner_passes==2);
    clear_focus::focus=graph_hwnd;host::author_delete_latched=false;handled=FALSE;
    host::dispatch_web_panel_command(nullptr,nullptr,21,AEGP_HP_BeforeAE,FALSE,&handled);
    CHECK(handled && inner_passes==2); // Existing graph-owned route, not AE Clear.
    graph->hwnd=nullptr;host::views.clear();
    if(text)DestroyWindow(text);if(graph_hwnd)DestroyWindow(graph_hwnd);if(clear_focus::root)DestroyWindow(clear_focus::root);
    host::stop_ui_exclusion();std::cout<<clear_fixture::checks<<" checks, 0 failures; actual Host + native Clear, fake SDK/GUI (not AE qualification)\n";return 0;
}
