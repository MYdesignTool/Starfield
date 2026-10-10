#include "UiExclusionHost.hpp"
#include <atomic>
#include <thread>

namespace {
std::atomic_bool enabled{false};
std::thread::id ui_thread;
std::uint64_t active_token{},next_token{};
}
namespace starfield::adapter {
void initialize_ui_exclusion() noexcept {
    enabled.store(false,std::memory_order_release);
    ui_thread=std::this_thread::get_id();active_token=0;
    enabled.store(true,std::memory_order_release);
}
void stop_ui_exclusion() noexcept {enabled.store(false,std::memory_order_release);}
bool ui_exclusion_available() noexcept {
    return enabled.load(std::memory_order_acquire) && std::this_thread::get_id()==ui_thread && active_token==0;
}
}
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginUiExclusionV1() noexcept {
    if(!starfield::adapter::ui_exclusion_available())return 0;
    if(++next_token==0)++next_token;
    return active_token=next_token;
}
extern "C" __declspec(dllexport) void SFLD_EndUiExclusionV1(std::uint64_t token) noexcept {
    if(std::this_thread::get_id()==ui_thread && token && token==active_token)active_token=0;
}
