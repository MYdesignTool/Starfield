#include "GraphParameter.hpp"

#include "AE_EffectCB.h"
#include "starfield/core/SequenceCodec.hpp"

#include <cmath>
#include <cstring>
#include <new>
#include <span>
#include <string_view>

namespace starfield::adapter {
namespace {

constexpr std::string_view kTextPrefix = "SFLDGRAPH1:";
constexpr char kHex[] = "0123456789abcdef";

bool handles_available(const PF_InData* data) noexcept {
    return data && data->utils && data->utils->host_new_handle && data->utils->host_lock_handle &&
        data->utils->host_unlock_handle && data->utils->host_dispose_handle && data->utils->host_get_handle_size;
}

// Borrowed only; no handle ownership transfer. The lock cannot outlive a callback.
class LockedBytes {
public:
    LockedBytes(PF_InData* data, PF_ArbitraryH handle) : data_(data), handle_(handle) {
        if (!handles_available(data) || !handle) return;
        const auto size = data->utils->host_get_handle_size(handle);
        if (size < core::kSequenceHeaderSize || size > core::kMaxGraphPayloadBytes) return;
        pointer_ = data->utils->host_lock_handle(handle);
        if (pointer_) bytes_ = {static_cast<const std::byte*>(pointer_), static_cast<std::size_t>(size)};
    }
    ~LockedBytes() { if (pointer_) data_->utils->host_unlock_handle(handle_); }
    LockedBytes(const LockedBytes&) = delete;
    LockedBytes& operator=(const LockedBytes&) = delete;
    [[nodiscard]] bool valid() const noexcept { return pointer_ != nullptr; }
    [[nodiscard]] std::span<const std::byte> bytes() const noexcept { return bytes_; }
private:
    PF_InData* data_;
    PF_ArbitraryH handle_;
    void* pointer_{nullptr};
    std::span<const std::byte> bytes_;
};

PF_Err copy_bytes(PF_InData* data, std::span<const std::byte> bytes, PF_ArbitraryH* output) {
    if (!output) return PF_Err_BAD_CALLBACK_PARAM;
    *output = nullptr;
    if (!handles_available(data) || bytes.size() < core::kSequenceHeaderSize || bytes.size() > core::kMaxGraphPayloadBytes)
        return PF_Err_BAD_CALLBACK_PARAM;
    const auto handle = data->utils->host_new_handle(bytes.size());
    if (!handle) return PF_Err_OUT_OF_MEMORY;
    void* pointer = data->utils->host_lock_handle(handle);
    if (!pointer) { data->utils->host_dispose_handle(handle); return PF_Err_OUT_OF_MEMORY; }
    std::memcpy(pointer, bytes.data(), bytes.size());
    data->utils->host_unlock_handle(handle);
    *output = handle;
    return PF_Err_NONE;
}

PF_Err codec_error(const core::SequenceError& error) noexcept {
    return error.code == core::SequenceErrorCode::allocation_failed ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
}

PF_Err accept_bytes(PF_InData* data, std::span<const std::byte> bytes, PF_ArbitraryH* output) {
    if (!output) return PF_Err_BAD_CALLBACK_PARAM;
    *output = nullptr;
    const auto graph = core::deserialize_graph(bytes, core::particle_node_registry());
    if (!graph.has_value()) return codec_error(graph.error());
    // Normalize vector order so COMPARE is independent of source record order.
    return create_graph_parameter(data, graph.value(), output);
}

PF_Err clone_handle(PF_InData* data, PF_ArbitraryH source, PF_ArbitraryH* output) {
    if (!output) return PF_Err_BAD_CALLBACK_PARAM;
    *output = nullptr;
    const LockedBytes locked(data, source);
    if (!locked.valid()) return PF_Err_BAD_CALLBACK_PARAM;
    return copy_bytes(data, locked.bytes(), output);
}

int hex_digit(char value) noexcept {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

PF_Err dispatch_arbitrary(PF_InData* data, PF_ArbParamsExtra& extra) {
    switch (extra.which_function) {
        case PF_Arbitrary_NEW_FUNC: {
            if (!extra.u.new_func_params.arbPH) return PF_Err_BAD_CALLBACK_PARAM;
            *extra.u.new_func_params.arbPH = nullptr;
            const auto graph = graph_from_controls(core::Settings{});
            if (!graph.has_value()) return PF_Err_OUT_OF_MEMORY;
            return create_graph_parameter(data, graph.value(), extra.u.new_func_params.arbPH);
        }
        case PF_Arbitrary_DISPOSE_FUNC:
            if (extra.u.dispose_func_params.arbH) data->utils->host_dispose_handle(extra.u.dispose_func_params.arbH);
            return PF_Err_NONE;
        case PF_Arbitrary_COPY_FUNC:
            return clone_handle(data, extra.u.copy_func_params.src_arbH, extra.u.copy_func_params.dst_arbPH);
        case PF_Arbitrary_FLAT_SIZE_FUNC: {
            auto& p = extra.u.flat_size_func_params;
            if (!p.flat_data_sizePLu) return PF_Err_BAD_CALLBACK_PARAM;
            *p.flat_data_sizePLu = 0;
            const LockedBytes value(data, p.arbH);
            if (!value.valid()) return PF_Err_BAD_CALLBACK_PARAM;
            *p.flat_data_sizePLu = static_cast<A_u_long>(value.bytes().size());
            return PF_Err_NONE;
        }
        case PF_Arbitrary_FLATTEN_FUNC: {
            auto& p = extra.u.flatten_func_params;
            const LockedBytes value(data, p.arbH);
            if (!value.valid() || !p.flat_dataPV || p.buf_sizeLu < value.bytes().size()) return PF_Err_BAD_CALLBACK_PARAM;
            std::memcpy(p.flat_dataPV, value.bytes().data(), value.bytes().size());
            return PF_Err_NONE;
        }
        case PF_Arbitrary_UNFLATTEN_FUNC: {
            auto& p = extra.u.unflatten_func_params;
            if (!p.arbPH) return PF_Err_BAD_CALLBACK_PARAM;
            *p.arbPH = nullptr;
            if (!p.flat_dataPV || p.buf_sizeLu > core::kMaxGraphPayloadBytes) return PF_Err_BAD_CALLBACK_PARAM;
            return accept_bytes(data, {static_cast<const std::byte*>(p.flat_dataPV), p.buf_sizeLu}, p.arbPH);
        }
        case PF_Arbitrary_INTERP_FUNC: {
            auto& p = extra.u.interp_func_params;
            if (!p.interpPH) return PF_Err_BAD_CALLBACK_PARAM;
            *p.interpPH = nullptr;
            if (!std::isfinite(p.tF) || p.tF < 0.0 || p.tF > 1.0) return PF_Err_BAD_CALLBACK_PARAM;
            return clone_handle(data, p.tF < 1.0 ? p.left_arbH : p.right_arbH, p.interpPH);
        }
        case PF_Arbitrary_COMPARE_FUNC: {
            auto& p = extra.u.compare_func_params;
            if (!p.compareP) return PF_Err_BAD_CALLBACK_PARAM;
            *p.compareP = PF_ArbCompare_NOT_EQUAL;
            const LockedBytes left(data, p.a_arbH), right(data, p.b_arbH);
            if (!left.valid() || !right.valid()) return PF_Err_BAD_CALLBACK_PARAM;
            if (left.bytes().size() == right.bytes().size() &&
                std::memcmp(left.bytes().data(), right.bytes().data(), left.bytes().size()) == 0)
                *p.compareP = PF_ArbCompare_EQUAL;
            return PF_Err_NONE;
        }
        case PF_Arbitrary_PRINT_SIZE_FUNC: {
            auto& p = extra.u.print_size_func_params;
            if (!p.print_sizePLu) return PF_Err_BAD_CALLBACK_PARAM;
            *p.print_sizePLu = 0;
            const LockedBytes value(data, p.arbH);
            if (!value.valid()) return PF_Err_BAD_CALLBACK_PARAM;
            *p.print_sizePLu = static_cast<A_u_long>(kTextPrefix.size() + 2 * value.bytes().size() + 1);
            return PF_Err_NONE;
        }
        case PF_Arbitrary_PRINT_FUNC: {
            auto& p = extra.u.print_func_params;
            const LockedBytes value(data, p.arbH);
            if (!value.valid() || !p.print_bufferPC ||
                p.print_sizeLu < kTextPrefix.size() + 2 * value.bytes().size() + 1) return PF_Err_BAD_CALLBACK_PARAM;
            std::memcpy(p.print_bufferPC, kTextPrefix.data(), kTextPrefix.size());
            auto* cursor = p.print_bufferPC + kTextPrefix.size();
            for (const auto byte : value.bytes()) {
                const auto number = std::to_integer<unsigned>(byte);
                *cursor++ = kHex[number >> 4]; *cursor++ = kHex[number & 15];
            }
            *cursor = '\0';
            return PF_Err_NONE;
        }
        case PF_Arbitrary_SCAN_FUNC: {
            auto& p = extra.u.scan_func_params;
            if (!p.arbPH) return PF_Err_BAD_CALLBACK_PARAM;
            *p.arbPH = nullptr;
            if (!p.bufPC || p.bytes_to_scanLu > kTextPrefix.size() + 2 * core::kMaxGraphPayloadBytes + 1)
                return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
            std::string_view text(p.bufPC, p.bytes_to_scanLu);
            if (!text.empty() && text.back() == '\0') text.remove_suffix(1);
            if (!text.starts_with(kTextPrefix)) return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
            text.remove_prefix(kTextPrefix.size());
            if (text.size() % 2 != 0 || text.size() < 2 * core::kSequenceHeaderSize) return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
            core::OpaqueBytes bytes(text.size() / 2);
            for (std::size_t i = 0; i < bytes.size(); ++i) {
                const int high = hex_digit(text[2 * i]), low = hex_digit(text[2 * i + 1]);
                if (high < 0 || low < 0) return PF_Err_CANNOT_PARSE_KEYFRAME_TEXT;
                bytes[i] = static_cast<std::byte>((high << 4) | low);
            }
            const auto err = accept_bytes(data, bytes, p.arbPH);
            return err == PF_Err_BAD_CALLBACK_PARAM ? PF_Err_CANNOT_PARSE_KEYFRAME_TEXT : err;
        }
        default: return PF_Err_BAD_CALLBACK_PARAM;
    }
}

} // namespace

core::Result<core::Graph> graph_from_controls(const core::Settings& settings) {
    // The single-emitter Alpha chain is emitter -> force -> appearance -> output
    // (current version goal 1). Identities are scoped to one graph and only have to
    // be stable inside it; the last byte distinguishes nodes and edges. The legacy
    // two-stage constructor stays available for tests and for stored graphs that
    // predate the force/appearance stages.
    core::Uuid128 emitter{{0x81,0xcb,0x8b,0xb1,0xf3,0x20,0x41,0x14,0x98,0xf5,0xd2,0x5b,0x54,0x91,0x2c,0x01}};
    auto output = emitter; output.bytes[15] = 2;
    auto force = emitter; force.bytes[15] = 4;
    auto appearance = emitter; appearance.bytes[15] = 5;
    auto emitter_to_force = emitter; emitter_to_force.bytes[15] = 6;
    auto force_to_appearance = emitter; force_to_appearance.bytes[15] = 7;
    auto appearance_to_output = emitter; appearance_to_output.bytes[15] = 8;
    return core::make_emitter_force_appearance_output_graph(
        settings, core::NodeId{emitter}, core::NodeId{force}, core::NodeId{appearance}, core::NodeId{output},
        core::EdgeId{emitter_to_force}, core::EdgeId{force_to_appearance}, core::EdgeId{appearance_to_output});
}

core::Result<core::Graph> read_graph_parameter(PF_InData* data, PF_ArbitraryH handle) {
    const LockedBytes value(data, handle);
    if (!value.valid()) return core::Result<core::Graph>::failure(core::ErrorCode::invalid_request, "graph parameter handle is invalid");
    auto decoded = core::deserialize_graph(value.bytes(), core::particle_node_registry());
    if (!decoded.has_value()) return core::Result<core::Graph>::failure(
        decoded.error().code == core::SequenceErrorCode::allocation_failed ? core::ErrorCode::allocation_failed : core::ErrorCode::invalid_request,
        decoded.error().detail);
    return core::Result<core::Graph>::success(decoded.take_value());
}

PF_Err create_graph_parameter(PF_InData* data, const core::Graph& graph, PF_ArbitraryH* output) noexcept {
    if (!output) return PF_Err_BAD_CALLBACK_PARAM;
    *output = nullptr;
    try {
        const auto encoded = core::serialize_graph(graph, core::particle_node_registry());
        if (!encoded.has_value()) return codec_error(encoded.error());
        return copy_bytes(data, encoded.value(), output);
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err graph_arbitrary_callback(PF_InData* data, PF_ArbParamsExtra* extra) noexcept {
    if (!handles_available(data) || !extra || extra->id != kGraphParameterId) return PF_Err_BAD_CALLBACK_PARAM;
    try { return dispatch_arbitrary(data, *extra); }
    catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

} // namespace starfield::adapter
