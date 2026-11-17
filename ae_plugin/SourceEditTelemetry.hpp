#pragma once
#include <cstdint>
#include <type_traits>
namespace starfield::adapter {
// Private paired-DLL observation only. Bounded POD, no SDK ownership or I/O.
struct SourceEditTelemetry {
    std::uint32_t bytes{sizeof(SourceEditTelemetry)},kind{},parameter{},revision{},queries{},reused{};
    std::int32_t error{};
    std::uint32_t misses{},source_failure{};
    std::uint64_t total_us{},compile_us{},query_us{},full_read_us{};
};
// OR-ed operation observations, not admission controls or user errors.
enum SourceReuseMiss : std::uint32_t { reuse_inactive=1,reuse_kind=2,reuse_capability=4,
    reuse_project=8,reuse_context=16,reuse_cold=32,reuse_identity=64,
    reuse_state_changed=128,reuse_resource=256,reuse_unstable=512,reuse_query_failed=1024 };
static_assert(std::is_trivially_copyable_v<SourceEditTelemetry> && std::is_standard_layout_v<SourceEditTelemetry>);
}
