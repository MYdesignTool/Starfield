#include "MainLauncher.hpp"
#include "PresetsUI.hpp"
#include "GraphCarrier.hpp"
#include "SPBasic.h"
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>
namespace starfield::adapter { AEGP_PluginID graph_carrier_plugin_id() noexcept {return 7;} }
using namespace starfield::adapter;
namespace {
int checks{},failures{},acquires{},releases{};
#define CHECK(x) do {++checks;if(!(x)){++failures;std::printf("FAIL %d: %s\n",__LINE__,#x);}} while(0)
std::vector<std::string> scripts;
AEGP_UtilitySuite6 utility{};
A_Err execute(AEGP_PluginID id,const A_char* script,A_Boolean,AEGP_MemHandle*,AEGP_MemHandle*) {
    CHECK(id==7);scripts.emplace_back(script);return 0;
}
SPErr acquire(const char* name,int version,const void** suite) {
    if(std::strcmp(name,kAEGPUtilitySuite) || version!=kAEGPUtilitySuiteVersion6)return 1;
    ++acquires;*suite=&utility;return 0;
}
SPErr release(const char*,int) {++releases;return 0;}
}
int main() {
    for(int width:{std::numeric_limits<int>::min(),0,34,100,240,304,900,std::numeric_limits<int>::max()}) {
        const auto layout=main_launcher_layout(width);CHECK(layout.width>=30 && layout.width<=300);
        CHECK(layout.action_top+22<=127);
        CHECK(main_launcher_action(layout,0,0)==MainLauncherAction::presets);
        CHECK(main_launcher_action(layout,1,layout.action_top)==MainLauncherAction::panel);
        CHECK(main_launcher_action(layout,layout.width-1,layout.action_top+21)==MainLauncherAction::presets);
        CHECK(main_launcher_action(layout,layout.action_width+1,layout.action_top+1)==MainLauncherAction::none);
        CHECK(main_launcher_action(layout,0,layout.image_height+1)==MainLauncherAction::none);
        CHECK(main_launcher_action(layout,-1,0)==MainLauncherAction::none);
        CHECK(main_launcher_action(layout,layout.width,0)==MainLauncherAction::none);
        CHECK(main_launcher_action(layout,0,-1)==MainLauncherAction::none);
        CHECK(main_launcher_action(layout,0,layout.action_top+22)==MainLauncherAction::none);
    }
    utility.AEGP_ExecuteScript=execute;SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData data{};data.pica_basicP=&basic;
    PF_Context context{};context.w_type=PF_Window_EFFECT;auto* pointer=&context;
    PF_EventExtra event{};event.contextH=&pointer;event.e_type=PF_Event_DO_CLICK;
    event.effect_win.index=1;event.effect_win.area=PF_EA_CONTROL;
    auto& frame=event.effect_win.current_frame;frame.left=10;frame.top=20;frame.right=314;frame.bottom=150;
    const auto layout=main_launcher_layout(frame.right-frame.left);
    auto click=[&](int x,int y){event.u.do_click.screen_point.h=static_cast<A_short>(frame.left+2+x);
        event.u.do_click.screen_point.v=static_cast<A_short>(frame.top+2+y);return main_presets_event(&data,nullptr,&event);};
    CHECK(click(10,layout.action_top+10)==0);CHECK(scripts.size()==1 && scripts[0].find("findMenuCommandId('Starfield Particle Controls')")!=std::string::npos);
    CHECK(click(layout.width-10,layout.action_top+10)==0);CHECK(scripts.size()==2 && scripts[1].find("findMenuCommandId('Starfield Presets')")!=std::string::npos);
    CHECK(click(10,10)==0);CHECK(scripts.size()==3 && scripts[2].find("findMenuCommandId('Starfield Presets')")!=std::string::npos);
    CHECK(click(layout.action_width+1,layout.action_top+5)==0 && scripts.size()==3);
    CHECK(click(0,layout.action_top+22)==0 && scripts.size()==3);
    event.effect_win.area=PF_EA_PARAM_TITLE;CHECK(click(10,10)==0 && scripts.size()==3);
    event.effect_win.area=PF_EA_CONTROL;event.effect_win.index=2;CHECK(click(10,10)==0 && scripts.size()==3);
    event.effect_win.index=1;context.w_type=PF_Window_COMP;CHECK(click(10,10)==0 && scripts.size()==3);
    CHECK(acquires==releases);
    clear_main_presets_ui();std::printf("Main launcher: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
