#pragma once
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace starfield::adapter {
using AuthorNodeId = std::array<std::uint16_t,8>;
struct NativeConnection {AuthorNodeId target{},edge{};bool operator==(const NativeConnection&)const=default;};
struct NativeConnectionRow {AuthorNodeId source{};std::vector<NativeConnection> expected,desired;};
struct NativeDeleteRequest {
    A_long project{},comp{},layer{},revision{};
    std::uint32_t checksum{};
    std::wstring project_scope;
    std::vector<AuthorNodeId> nodes;
    bool patch{};
    std::vector<NativeConnectionRow> rows;
};
struct NativeDeleteResult {
    struct Timings {std::uint64_t total{},resolve{},preflight{},capture{},publish{},physical{},refresh{};} timing_us;
    std::uint64_t operation{};
    A_Err error{},rollback_error{},cleanup_error{},undo_error{},command_error{};
    bool committed{},guarded{},outcome_unknown{},unchanged{};
    bool clear_dispatched{};
    bool capacity_rejected{};
    A_long revision{},comp{},layer{},clear_stage{};
};
// Private, bounded panel delta. No JSON parser, scripts or borrowed references.
bool parse_native_delete(std::wstring_view,NativeDeleteRequest&);
bool parse_native_patch(std::wstring_view,NativeDeleteRequest&);
std::wstring native_delete_reply(const NativeDeleteResult&);
// Caller holds resident UI exclusion; all handles are borrowed for this call.
NativeDeleteResult delete_native_nodes(SPBasicSuite*,AEGP_PluginID,AEGP_LayerH,
    const NativeDeleteRequest&) noexcept;
std::wstring execute_native_delete(SPBasicSuite*,AEGP_PluginID,std::wstring_view,bool patch=false,bool diagnostics=false);
// Synchronous AE CommandHook entry, after the graph's Clear route declined.
// Busy reentry is consumed without SDK reads. Otherwise true identifies a
// complete same-layer native-node set (including a failed owned operation).
// With clear_command, native parameter/keyframe selections may also be observed;
// only AE's actual removal of captured UUIDs authorizes graph reconciliation.
bool delete_selected_native_nodes(SPBasicSuite*,AEGP_PluginID,NativeDeleteResult&,bool execute=true,
    AEGP_Command clear_command=0,A_Boolean* handled=nullptr) noexcept;
// Recursive application Clear passes through before any focus/busy/selection
// handling. AE chooses the target; the outer wrapper only observes its delta.
bool native_clear_dispatch_active() noexcept;
}
