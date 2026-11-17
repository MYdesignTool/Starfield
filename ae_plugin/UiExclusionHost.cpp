#include "UiExclusionHost.hpp"
#include <atomic>
#include <thread>
#include <array>

namespace {
std::atomic_bool enabled{false};
std::thread::id ui_thread;
std::uint64_t active_token{},next_token{};
bool source_scope{};
std::atomic_uint64_t graph_epoch{0};
std::array<starfield::adapter::SourceEditTelemetry,16> source_edits;
unsigned source_head{},source_count{};
}
namespace starfield::adapter {
void initialize_ui_exclusion() noexcept {
    enabled.store(false,std::memory_order_release);
    ui_thread=std::this_thread::get_id();active_token=0;source_scope=false;
    source_head=source_count=0;
    graph_epoch.store(0,std::memory_order_release);
    enabled.store(true,std::memory_order_release);
}
void stop_ui_exclusion() noexcept {enabled.store(false,std::memory_order_release);}
bool ui_exclusion_available() noexcept {
    return enabled.load(std::memory_order_acquire) && std::this_thread::get_id()==ui_thread && active_token==0;
}
bool take_source_edit(SourceEditTelemetry& result) noexcept {
    if(!ui_exclusion_available() || !source_count)return false;
    result=source_edits[source_head];source_head=(source_head+1)%16;--source_count;return true;
}
}
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginUiExclusionV1() noexcept {
    if(!starfield::adapter::ui_exclusion_available())return 0;
    if(++next_token==0)++next_token;
    source_scope=false;
    return active_token=next_token;
}
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginSourceUiExclusionV1() noexcept {
    const auto token=SFLD_BeginUiExclusionV1();if(token)source_scope=true;return token;
}
extern "C" __declspec(dllexport) void SFLD_EndUiExclusionV1(std::uint64_t token) noexcept {
    if(std::this_thread::get_id()==ui_thread && token && token==active_token){active_token=0;source_scope=false;}
}
extern "C" __declspec(dllexport) bool SFLD_UiExclusionActiveV1() noexcept {
    return enabled.load(std::memory_order_acquire) && std::this_thread::get_id()==ui_thread && active_token!=0;
}
extern "C" __declspec(dllexport) bool SFLD_SourceUiExclusionActiveV1() noexcept {
    return SFLD_UiExclusionActiveV1() && source_scope;
}
extern "C" __declspec(dllexport) void SFLD_RecordSourceEditV1(const starfield::adapter::SourceEditTelemetry* value) noexcept {
    if(!SFLD_SourceUiExclusionActiveV1() || !value || value->bytes!=sizeof(*value))return;
    if(source_count==source_edits.size()){source_head=(source_head+1)%16;--source_count;}
    source_edits[(source_head+source_count)%source_edits.size()]=*value;++source_count;
}
extern "C" __declspec(dllexport) std::uint64_t SFLD_GraphMutationEpochV1() noexcept {
    // No UI-only state or SDK access on render threads.
    return graph_epoch.load(std::memory_order_acquire);
}
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginGraphMutationV1() noexcept {
    if(!enabled.load(std::memory_order_acquire) || std::this_thread::get_id()!=ui_thread ||
        !active_token || source_scope)return 0;
    const auto previous=graph_epoch.load(std::memory_order_relaxed);
    if(previous&1)return 0;
    const auto token=previous+1;
    graph_epoch.store(token,std::memory_order_release);return token;
}
extern "C" __declspec(dllexport) void SFLD_EndGraphMutationV1(std::uint64_t token) noexcept {
    if(std::this_thread::get_id()==ui_thread && (token&1) &&
        graph_epoch.load(std::memory_order_relaxed)==token)
        graph_epoch.store(token+1,std::memory_order_release);
}
