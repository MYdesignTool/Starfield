#pragma once
#include <algorithm>
namespace starfield::adapter {
enum class MainLauncherAction { none, panel, presets };
struct MainLauncherLayout {
    int width{}, image_height{}, action_top{}, action_width{};
};
inline MainLauncherLayout main_launcher_layout(int frame_width) noexcept {
    const int width=std::clamp(frame_width,34,304)-4;
    return {width,width/3,width/3+5,(width-4)/2};
}
inline MainLauncherAction main_launcher_action(const MainLauncherLayout& layout,int x,int y) noexcept {
    if(x<0 || x>=layout.width || y<0)return MainLauncherAction::none;
    if(y<layout.image_height)return MainLauncherAction::presets;
    if(y<layout.action_top || y>=layout.action_top+22)return MainLauncherAction::none;
    if(x<layout.action_width)return MainLauncherAction::panel;
    if(x>=layout.action_width+4)return MainLauncherAction::presets;
    return MainLauncherAction::none;
}
}
