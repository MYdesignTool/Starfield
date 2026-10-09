#pragma once
#include <cstdint>
#include <type_traits>

namespace starfield::adapter {
// Private synchronous UI message, not the persistent Core ABI. A sink must copy
// all bytes before returning. No allocations/host handles cross this boundary.
enum class ModelAssetError : std::uint32_t {
    none, invalid_request, unavailable, stale_author, invalid_geometry,
    allocation_failed, cancelled, sink_rejected, host_error
};
struct ModelAssetExportRequest {
    std::uint32_t magic{0x53464d58},bytes{sizeof(ModelAssetExportRequest)},version{1},operation{1};
    std::uint8_t expected_uuid[16]{};
    std::uint32_t expected_source{},expected_revision{};
    void* sink_context{};
    std::int32_t (*write_bytes)(void*,const std::uint8_t*,std::uint32_t) noexcept {};
    void* cancellation_context{};
    std::int32_t (*is_cancelled)(void*) noexcept {};
    std::uint32_t acknowledged{};
    ModelAssetError error{ModelAssetError::unavailable};
    std::int32_t host_error{};
    std::uint32_t payload_bytes{};
    double bounds[6]{};
};
static_assert(std::is_standard_layout_v<ModelAssetExportRequest> &&
    std::is_trivially_copyable_v<ModelAssetExportRequest>);
struct ModelAssetWriteRequest {
    std::uint32_t magic{0x53464d57},bytes{sizeof(ModelAssetWriteRequest)},version{1},operation{1};
    std::uint8_t expected_uuid[16]{};
    std::uint32_t expected_source{},expected_revision{},desired_source{},desired_revision{};
    const std::uint8_t* mesh_bytes{};
    std::uint32_t mesh_length{};
    double desired_bounds[6]{};
    void* cancellation_context{};
    std::int32_t (*is_cancelled)(void*) noexcept {};
    std::uint32_t acknowledged{};
    ModelAssetError error{ModelAssetError::unavailable};
    std::int32_t host_error{},rollback_error{};
};
static_assert(std::is_standard_layout_v<ModelAssetWriteRequest> &&
    std::is_trivially_copyable_v<ModelAssetWriteRequest>);
}
