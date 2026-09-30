#include "GraphCarrier.hpp"

#include "AE_GeneralPlug.h"
#include "GraphParameter.hpp"
#include "NodeGraphSync.hpp"
#include "Parameters.hpp"
#include "SPBasic.h"
#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <mutex>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace starfield::adapter {
namespace {

constexpr std::size_t kMaxCarrierGraphBytes = 24u * 1024u;
constexpr std::size_t kMaxCarrierExpressionChars = 2u * kMaxCarrierGraphBytes + 256u;
constexpr A_long kMaxTransactionNonce = 1000000;
constexpr std::string_view kSnapshotPrefix = "/*SFLDSNAP1:";
constexpr std::string_view kTransactionPrefix = "/*SFLDTXN1:";
constexpr std::string_view kSyncPrefix = "/*SFLDSYNC1:";
constexpr std::string_view kExpressionSuffix = "*/0";
constexpr char kHexDigits[] = "0123456789abcdef";

std::atomic<AEGP_PluginID> g_plugin_id{0};
std::atomic<bool> g_registered{false};
std::mutex g_registration_mutex;

std::uint32_t crc32(std::span<const std::byte> bytes) noexcept {
    std::uint32_t crc = 0xffffffffu;
    for (const std::byte byte : bytes) {
        crc ^= std::to_integer<std::uint8_t>(byte);
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1u) ^ ((0u - (crc & 1u)) & 0xedb88320u);
        }
    }
    return crc ^ 0xffffffffu;
}

template <typename T>
bool parse_unsigned(std::string_view text, T& value, int base = 10) noexcept {
    if (text.empty()) return false;
    T parsed{};
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed, base);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size()) return false;
    value = parsed;
    return true;
}

std::string crc_text(std::uint32_t value) {
    char output[9]{};
    std::snprintf(output, sizeof(output), "%08x", value);
    return output;
}

std::string hex_bytes(std::span<const std::byte> bytes) {
    std::string output;
    output.reserve(bytes.size() * 2u);
    for (const auto byte : bytes) {
        const auto value = std::to_integer<std::uint8_t>(byte);
        output.push_back(kHexDigits[value >> 4u]);
        output.push_back(kHexDigits[value & 0x0fu]);
    }
    return output;
}

int hex_digit(char value) noexcept {
    if (value >= '0' && value <= '9') return value - '0';
    if (value >= 'a' && value <= 'f') return value - 'a' + 10;
    if (value >= 'A' && value <= 'F') return value - 'A' + 10;
    return -1;
}

struct DecodedPayload {
    std::uint64_t revision{0};
    core::OpaqueBytes bytes;
};

bool decode_payload(std::string_view expression, std::string_view prefix,
                    std::uint64_t& revision, core::OpaqueBytes& bytes) {
    if (!expression.starts_with(prefix) || !expression.ends_with(kExpressionSuffix) ||
        expression.size() > kMaxCarrierExpressionChars) return false;
    expression.remove_prefix(prefix.size());
    expression.remove_suffix(kExpressionSuffix.size());

    const auto first = expression.find(':');
    if (first == std::string_view::npos) return false;
    const auto second = expression.find(':', first + 1);
    const auto third = second == std::string_view::npos ? second : expression.find(':', second + 1);
    if (second == std::string_view::npos || third == std::string_view::npos) return false;

    std::uint64_t parsed_revision = 0;
    std::uint32_t parsed_length = 0;
    std::uint32_t parsed_crc = 0;
    if (!parse_unsigned(expression.substr(0, first), parsed_revision) ||
        parsed_revision == 0 || parsed_revision > std::numeric_limits<std::uint32_t>::max() ||
        !parse_unsigned(expression.substr(first + 1, second - first - 1), parsed_length) ||
        parsed_length < core::kSequenceHeaderSize || parsed_length > kMaxCarrierGraphBytes ||
        !parse_unsigned(expression.substr(second + 1, third - second - 1), parsed_crc, 16)) return false;

    const auto encoded = expression.substr(third + 1);
    if (encoded.size() != static_cast<std::size_t>(parsed_length) * 2u) return false;
    core::OpaqueBytes decoded(parsed_length);
    for (std::size_t index = 0; index < decoded.size(); ++index) {
        const int high = hex_digit(encoded[2u * index]);
        const int low = hex_digit(encoded[2u * index + 1u]);
        if (high < 0 || low < 0) return false;
        decoded[index] = static_cast<std::byte>((high << 4) | low);
    }
    if (crc32(decoded) != parsed_crc) return false;
    revision = parsed_revision;
    bytes = std::move(decoded);
    return true;
}

std::string encode_snapshot_expression(std::uint64_t revision, std::span<const std::byte> bytes) {
    if (revision == 0 || revision > std::numeric_limits<std::uint32_t>::max() ||
        bytes.size() < core::kSequenceHeaderSize || bytes.size() > kMaxCarrierGraphBytes) return {};
    std::string expression(kSnapshotPrefix);
    expression += std::to_string(revision);
    expression.push_back(':');
    expression += std::to_string(bytes.size());
    expression.push_back(':');
    expression += crc_text(crc32(bytes));
    expression.push_back(':');
    expression += hex_bytes(bytes);
    expression += kExpressionSuffix;
    return expression;
}

bool split_fields(std::string_view text, std::vector<std::string_view>& fields) {
    fields.clear();
    for (;;) {
        const auto separator = text.find(':');
        if (separator == std::string_view::npos) {
            fields.push_back(text);
            return true;
        }
        fields.push_back(text.substr(0, separator));
        text.remove_prefix(separator + 1);
        if (fields.size() > 5) return false;
    }
}

struct TransactionRequest {
    bool sync{false};
    A_long nonce{0};
    std::uint64_t base_revision{0};
    core::OpaqueBytes bytes;
};

struct NodeEditRequest {
    A_long nonce{0};
    core::NodeId node{};
    core::ParameterKey key{};
    core::ParameterValue value{};
};

bool parse_finite_double(std::string_view text, double& value) noexcept {
    if (text.empty()) return false;
    double parsed = 0.0;
    const auto result = std::from_chars(text.data(), text.data() + text.size(), parsed,
                                        std::chars_format::general);
    if (result.ec != std::errc{} || result.ptr != text.data() + text.size() || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}

bool parse_node_edit_request(std::string_view expression, NodeEditRequest& request) noexcept {
    constexpr std::string_view prefix = node_sync::kRequestPrefix;
    constexpr std::string_view suffix = node_sync::kRequestSuffix;
    if (!expression.starts_with(prefix) || !expression.ends_with(suffix) ||
        expression.size() > node_sync::kMaxRequestBytes) return false;
    expression.remove_prefix(prefix.size());
    expression.remove_suffix(suffix.size());
    std::array<std::string_view, 5> fields{};
    for (std::size_t i = 0; i < fields.size(); ++i) {
        const auto separator = expression.find(':');
        if (i + 1 == fields.size()) {
            if (separator != std::string_view::npos) return false;
            fields[i] = expression;
        } else {
            if (separator == std::string_view::npos) return false;
            fields[i] = expression.substr(0, separator);
            expression.remove_prefix(separator + 1);
        }
    }
    A_long nonce = 0;
    std::uint64_t key = 0;
    if (!parse_unsigned(fields[0], nonce) || nonce < 0 || nonce > node_sync::kMaxNonce ||
        fields[1].size() != 32 || !parse_unsigned(fields[2], key) || key == 0 || fields[3].size() != 1) return false;
    NodeEditRequest parsed{};
    parsed.nonce = nonce;
    parsed.key = core::ParameterKey{key};
    for (std::size_t i = 0; i < parsed.node.value.bytes.size(); ++i) {
        const int high = hex_digit(fields[1][2u * i]);
        const int low = hex_digit(fields[1][2u * i + 1u]);
        if (high < 0 || low < 0) return false;
        parsed.node.value.bytes[i] = static_cast<std::uint8_t>((high << 4) | low);
    }
    if (parsed.node.value.is_zero()) return false;

    const char kind = fields[3].front();
    if (kind == static_cast<char>(node_sync::ValueKind::float64)) {
        double value = 0.0;
        if (!parse_finite_double(fields[4], value)) return false;
        parsed.value = value;
    } else if (kind == static_cast<char>(node_sync::ValueKind::uint32)) {
        std::uint32_t value = 0;
        if (!parse_unsigned(fields[4], value)) return false;
        parsed.value = value;
    } else if (kind == static_cast<char>(node_sync::ValueKind::vector3)) {
        const auto first = fields[4].find(',');
        const auto second = first == std::string_view::npos ? first : fields[4].find(',', first + 1);
        if (first == std::string_view::npos || second == std::string_view::npos ||
            fields[4].find(',', second + 1) != std::string_view::npos) return false;
        core::Vec3 value{};
        if (!parse_finite_double(fields[4].substr(0, first), value.x) ||
            !parse_finite_double(fields[4].substr(first + 1, second - first - 1), value.y) ||
            !parse_finite_double(fields[4].substr(second + 1), value.z)) return false;
        parsed.value = value;
    } else {
        return false;
    }
    request = std::move(parsed);
    return true;
}

bool parse_sync_request(std::string_view expression, TransactionRequest& request) noexcept {
    if (!expression.starts_with(kSyncPrefix) || !expression.ends_with(kExpressionSuffix)) return false;
    expression.remove_prefix(kSyncPrefix.size());
    expression.remove_suffix(kExpressionSuffix.size());
    if (expression.empty()) return false;
    A_long nonce = 0;
    if (!parse_unsigned(expression, nonce) || nonce < 1 || nonce > kMaxTransactionNonce) return false;
    request = {};
    request.sync = true;
    request.nonce = nonce;
    return true;
}

bool parse_transaction_request(std::string_view expression, TransactionRequest& request) {
    if (!expression.starts_with(kTransactionPrefix) || !expression.ends_with(kExpressionSuffix) ||
        expression.size() > kMaxCarrierExpressionChars) return false;
    expression.remove_prefix(kTransactionPrefix.size());
    expression.remove_suffix(kExpressionSuffix.size());
    std::vector<std::string_view> fields;
    if (!split_fields(expression, fields) || fields.size() != 5) return false;
    A_long nonce = 0;
    std::uint64_t base_revision = 0;
    std::uint32_t byte_count = 0;
    std::uint32_t expected_crc = 0;
    if (!parse_unsigned(fields[0], nonce) || nonce < 1 || nonce > kMaxTransactionNonce ||
        !parse_unsigned(fields[1], base_revision) || base_revision == 0 ||
        base_revision > std::numeric_limits<std::uint32_t>::max() ||
        !parse_unsigned(fields[2], byte_count) || byte_count < core::kSequenceHeaderSize ||
        byte_count > kMaxCarrierGraphBytes || !parse_unsigned(fields[3], expected_crc, 16) ||
        fields[4].size() != static_cast<std::size_t>(byte_count) * 2u) return false;
    core::OpaqueBytes bytes(byte_count);
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        const int high = hex_digit(fields[4][2u * index]);
        const int low = hex_digit(fields[4][2u * index + 1u]);
        if (high < 0 || low < 0) return false;
        bytes[index] = static_cast<std::byte>((high << 4) | low);
    }
    if (crc32(bytes) != expected_crc) return false;
    request = {};
    request.nonce = nonce;
    request.base_revision = base_revision;
    request.bytes = std::move(bytes);
    return true;
}

struct SuiteSet {
    explicit SuiteSet(PF_InData* data) : basic(data ? data->pica_basicP : nullptr) {}
    ~SuiteSet() {
        if (!basic) return;
        if (memory) basic->ReleaseSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1);
        if (stream) basic->ReleaseSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6);
        if (effect) basic->ReleaseSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4);
        if (pf_interface) basic->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    }
    PF_Err acquire() noexcept {
        if (!basic) return PF_Err_BAD_CALLBACK_PARAM;
        A_Err error = basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                          reinterpret_cast<const void**>(&pf_interface));
        if (!error) error = basic->AcquireSuite(kAEGPEffectSuite, kAEGPEffectSuiteVersion4,
                                                 reinterpret_cast<const void**>(&effect));
        if (!error) error = basic->AcquireSuite(kAEGPStreamSuite, kAEGPStreamSuiteVersion6,
                                                 reinterpret_cast<const void**>(&stream));
        if (!error) error = basic->AcquireSuite(kAEGPMemorySuite, kAEGPMemorySuiteVersion1,
                                                 reinterpret_cast<const void**>(&memory));
        return static_cast<PF_Err>(error);
    }
    SPBasicSuite* basic{nullptr};
    const AEGP_PFInterfaceSuite1* pf_interface{nullptr};
    const AEGP_EffectSuite4* effect{nullptr};
    const AEGP_StreamSuite6* stream{nullptr};
    const AEGP_MemorySuite1* memory{nullptr};
};

class EffectStream {
public:
    EffectStream(SuiteSet& suites, AEGP_PluginID plugin_id, PF_InData* data, A_long index)
        : suites_(suites), plugin_id_(plugin_id) {
        AEGP_EffectRefH effect_ref = nullptr;
        if (!data || !data->effect_ref ||
            suites_.pf_interface->AEGP_GetNewEffectForEffect(plugin_id_, data->effect_ref, &effect_ref)) return;
        if (suites_.stream->AEGP_GetNewEffectStreamByIndex(plugin_id_, effect_ref,
                                                            static_cast<PF_ParamIndex>(index), &stream_ref_)) {
            suites_.effect->AEGP_DisposeEffect(effect_ref);
            return;
        }
        effect_ref_ = effect_ref;
    }
    ~EffectStream() {
        if (stream_ref_) suites_.stream->AEGP_DisposeStream(stream_ref_);
        if (effect_ref_) suites_.effect->AEGP_DisposeEffect(effect_ref_);
    }
    [[nodiscard]] bool valid() const noexcept { return stream_ref_ != nullptr; }
    [[nodiscard]] AEGP_StreamRefH get() const noexcept { return stream_ref_; }
private:
    SuiteSet& suites_;
    AEGP_PluginID plugin_id_;
    AEGP_EffectRefH effect_ref_{nullptr};
    AEGP_StreamRefH stream_ref_{nullptr};
};

PF_Err read_expression(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_StreamRefH stream,
                       std::string& expression) {
    expression.clear();
    AEGP_MemHandle memory_handle = nullptr;
    A_Err error = suites.stream->AEGP_GetExpression(plugin_id, stream, &memory_handle);
    if (error || !memory_handle) {
        if (memory_handle) suites.memory->AEGP_FreeMemHandle(memory_handle);
        return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
    }
    AEGP_MemSize size = 0;
    error = suites.memory->AEGP_GetMemHandleSize(memory_handle, &size);
    if (error || size > 2u * kMaxCarrierExpressionChars + sizeof(A_UTF16Char) ||
        size < sizeof(A_UTF16Char) || size % sizeof(A_UTF16Char) != 0) {
        suites.memory->AEGP_FreeMemHandle(memory_handle);
        return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
    }
    void* pointer = nullptr;
    error = suites.memory->AEGP_LockMemHandle(memory_handle, &pointer);
    if (error || !pointer) {
        suites.memory->AEGP_FreeMemHandle(memory_handle);
        return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
    }
    const auto* characters = static_cast<const A_UTF16Char*>(pointer);
    const std::size_t capacity = static_cast<std::size_t>(size / sizeof(A_UTF16Char));
    bool terminated = false;
    for (std::size_t index = 0; index < capacity; ++index) {
        const auto character = static_cast<std::uint16_t>(characters[index]);
        if (character == 0) { terminated = true; break; }
        if (character > 0x7fu || expression.size() >= kMaxCarrierExpressionChars) {
            expression.clear();
            error = PF_Err_BAD_CALLBACK_PARAM;
            break;
        }
        expression.push_back(static_cast<char>(character));
    }
    suites.memory->AEGP_UnlockMemHandle(memory_handle);
    suites.memory->AEGP_FreeMemHandle(memory_handle);
    if (error) return static_cast<PF_Err>(error);
    return terminated ? PF_Err_NONE : PF_Err_BAD_CALLBACK_PARAM;
}

PF_Err write_expression(SuiteSet& suites, AEGP_PluginID plugin_id, AEGP_StreamRefH stream,
                        std::string_view expression) {
    if (expression.size() > kMaxCarrierExpressionChars) return PF_Err_BAD_CALLBACK_PARAM;
    A_Boolean previous_enabled = FALSE;
    A_Err error = suites.stream->AEGP_GetExpressionState(plugin_id, stream, &previous_enabled);
    if (error) return static_cast<PF_Err>(error);
    std::string previous_expression;
    const PF_Err read_error = read_expression(suites, plugin_id, stream, previous_expression);
    if (read_error != PF_Err_NONE) return read_error;

    std::vector<A_UTF16Char> characters;
    characters.reserve(expression.size() + 1u);
    for (const unsigned char character : expression) {
        if (character > 0x7fu) return PF_Err_BAD_CALLBACK_PARAM;
        characters.push_back(static_cast<A_UTF16Char>(character));
    }
    characters.push_back(0);
    error = suites.stream->AEGP_SetExpression(plugin_id, stream, characters.data());
    if (!error) error = suites.stream->AEGP_SetExpressionState(plugin_id, stream, FALSE);
    if (error) {
        std::vector<A_UTF16Char> previous_characters;
        previous_characters.reserve(previous_expression.size() + 1u);
        for (const unsigned char character : previous_expression) {
            previous_characters.push_back(static_cast<A_UTF16Char>(character));
        }
        previous_characters.push_back(0);
        (void)suites.stream->AEGP_SetExpression(plugin_id, stream, previous_characters.data());
        (void)suites.stream->AEGP_SetExpressionState(plugin_id, stream, previous_enabled);
    }
    return static_cast<PF_Err>(error);
}

PF_Err read_parameter_expression(PF_InData* in_data, A_long index, std::string& expression) {
    const auto plugin_id = g_plugin_id.load(std::memory_order_acquire);
    if (!g_registered.load(std::memory_order_acquire) || plugin_id == 0) return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    SuiteSet suites(in_data);
    PF_Err error = suites.acquire();
    if (error != PF_Err_NONE) return error;
    EffectStream stream(suites, plugin_id, in_data, index);
    if (!stream.valid()) return PF_Err_BAD_CALLBACK_PARAM;
    return read_expression(suites, plugin_id, stream.get(), expression);
}

PF_Err write_parameter_expression(PF_InData* in_data, A_long index, std::string_view expression) {
    const auto plugin_id = g_plugin_id.load(std::memory_order_acquire);
    if (!g_registered.load(std::memory_order_acquire) || plugin_id == 0) return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    SuiteSet suites(in_data);
    PF_Err error = suites.acquire();
    if (error != PF_Err_NONE) return error;
    EffectStream stream(suites, plugin_id, in_data, index);
    if (!stream.valid()) return PF_Err_BAD_CALLBACK_PARAM;
    return write_expression(suites, plugin_id, stream.get(), expression);
}

void set_receipt(PF_ParamDef* params[], A_long value) noexcept {
    if (!params || !params[kGraphEditReceiptId] ||
        params[kGraphEditReceiptId]->param_type != PF_Param_FLOAT_SLIDER) return;
    auto& receipt = *params[kGraphEditReceiptId];
    receipt.u.fs_d.value = static_cast<PF_FpLong>(value);
    receipt.uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
}

PF_Err reject_request(PF_OutData* out_data, PF_ParamDef* params[], A_long nonce,
                      const char* message, PF_Err error = PF_Err_NONE) noexcept {
    set_receipt(params, -nonce);
    if (out_data && message) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Starfield graph edit rejected: %s", message);
    }
    return error;
}

PF_Err read_current_snapshot(PF_InData* in_data, std::uint64_t& revision,
                             core::OpaqueBytes& bytes) {
    std::string expression;
    const PF_Err read = read_parameter_expression(in_data, kGraphSnapshotId, expression);
    if (read != PF_Err_NONE) return read;
    return decode_payload(expression, kSnapshotPrefix, revision, bytes) ? PF_Err_NONE : PF_Err_BAD_CALLBACK_PARAM;
}

PF_Err commit_node_parameter_edit(PF_InData* in_data, PF_OutData* out_data,
                                  PF_ParamDef* params[], const NodeEditRequest& request,
                                  A_long nonce) {
    const auto reject_edit = [&](const char* message, PF_Err error = PF_Err_NONE) {
        if (nonce != 0) return reject_request(out_data, params, nonce, message, error);
        if (out_data && message) {
            std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                          "Starfield node parameter edit rejected: %s", message);
        }
        return error != PF_Err_NONE ? error : PF_Err_BAD_CALLBACK_PARAM;
    };
    if (request.nonce != nonce) return reject_edit("node edit nonce does not match");
    if (!params[kGraphParameterId] || params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
        !params[kGraphParameterId]->u.arb_d.value || !params[kControlSourceId] ||
        params[kControlSourceId]->param_type != PF_Param_POPUP) {
        return reject_edit("node edit graph parameters are unavailable");
    }

    auto decoded = read_graph_parameter(in_data, params[kGraphParameterId]->u.arb_d.value);
    if (!decoded.has_value()) return reject_edit("stored node graph is invalid");
    core::Graph graph = decoded.take_value();
    auto node = std::find_if(graph.nodes.begin(), graph.nodes.end(), [&](const core::GraphNode& candidate) {
        return candidate.id == request.node;
    });
    if (node == graph.nodes.end()) return reject_edit("edited node no longer exists in the graph");
    auto parameter = std::find_if(node->parameters.begin(), node->parameters.end(), [&](const core::NodeParameter& candidate) {
        return candidate.key == request.key;
    });
    if (parameter == node->parameters.end()) {
        return reject_edit("edited parameter is not present in the node graph");
    }
    parameter->value = request.value;
    const auto validation = core::validate_graph(graph, core::particle_node_registry());
    if (!validation) return reject_edit("edited node parameter failed graph validation");

    PF_ArbitraryH replacement = nullptr;
    const PF_Err created = create_graph_parameter(in_data, graph, &replacement);
    if (created != PF_Err_NONE) return reject_edit("edited graph allocation failed", created);
    A_long revision = 0;
    const PF_Err snapshot_error = write_graph_snapshot(in_data, graph, &revision);
    if (snapshot_error != PF_Err_NONE) {
        in_data->utils->host_dispose_handle(replacement);
        return reject_edit("edited graph snapshot write failed", snapshot_error);
    }

    // The node's parameter edit is the user action. Changing these renderer
    // streams from this supervised callback keeps the compiled graph on the
    // same AE undo/cache path as the node control edit.
    params[kGraphParameterId]->u.arb_d.value = replacement;
    params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    params[kControlSourceId]->u.pd.value = kNodeControlSource;
    params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
    if (nonce != 0) set_receipt(params, nonce);
    return PF_Err_NONE;
}

} // namespace

PF_Err register_graph_carrier(PF_InData* in_data) noexcept {
    if (!in_data || !in_data->pica_basicP || in_data->appl_id == 'PrMr') return PF_Err_UNRECOGNIZED_PARAM_TYPE;
    if (g_registered.load(std::memory_order_acquire)) return PF_Err_NONE;
    std::lock_guard lock(g_registration_mutex);
    if (g_registered.load(std::memory_order_relaxed)) return PF_Err_NONE;
    const AEGP_UtilitySuite6* utility = nullptr;
    A_Err error = in_data->pica_basicP->AcquireSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6,
                                                      reinterpret_cast<const void**>(&utility));
    AEGP_PluginID plugin_id = 0;
    if (!error && utility) error = utility->AEGP_RegisterWithAEGP(nullptr, "Starfield Particle", &plugin_id);
    if (utility) in_data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite, kAEGPUtilitySuiteVersion6);
    if (!error && plugin_id != 0) {
        g_plugin_id.store(plugin_id, std::memory_order_release);
        g_registered.store(true, std::memory_order_release);
        return PF_Err_NONE;
    }
    return static_cast<PF_Err>(error ? error : PF_Err_BAD_CALLBACK_PARAM);
}

PF_Err write_graph_snapshot(PF_InData* in_data, const core::Graph& graph,
                            A_long* new_revision) noexcept {
    if (!in_data) return PF_Err_BAD_CALLBACK_PARAM;
    try {
        const auto encoded = core::serialize_graph(graph, core::particle_node_registry());
        if (!encoded.has_value()) return encoded.error().code == core::SequenceErrorCode::allocation_failed
            ? PF_Err_OUT_OF_MEMORY : PF_Err_BAD_CALLBACK_PARAM;
        if (encoded.value().size() > kMaxCarrierGraphBytes) return PF_Err_BAD_CALLBACK_PARAM;

        std::uint64_t revision = 0;
        core::OpaqueBytes previous;
        if (read_current_snapshot(in_data, revision, previous) != PF_Err_NONE) revision = 0;
        if (revision >= std::numeric_limits<std::uint32_t>::max()) return PF_Err_BAD_CALLBACK_PARAM;
        ++revision;
        const auto expression = encode_snapshot_expression(revision, encoded.value());
        if (expression.empty()) return PF_Err_BAD_CALLBACK_PARAM;
        const PF_Err error = write_parameter_expression(in_data, kGraphSnapshotId, expression);
        if (error == PF_Err_NONE && new_revision) *new_revision = static_cast<A_long>(revision);
        return error;
    } catch (const std::bad_alloc&) { return PF_Err_OUT_OF_MEMORY; }
    catch (...) { return PF_Err_INTERNAL_STRUCT_DAMAGED; }
}

PF_Err commit_graph_request(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                            PF_UserChangedParamExtra* extra) noexcept {
    if (!in_data || !params || !extra || extra->param_index != kGraphEditCommitId) return PF_Err_NONE;

    A_long nonce = 1;
    try {
        std::string request_expression;
        const PF_Err read_request = read_parameter_expression(in_data, kGraphEditRequestId, request_expression);
        NodeEditRequest node_request{};
        if (read_request == PF_Err_NONE && parse_node_edit_request(request_expression, node_request)) {
            // Native node edits use their own zero-nonce request envelope. The
            // graph transaction slider retains the previous CEP nonce, so it
            // must not gate a node-originated edit.
            return commit_node_parameter_edit(in_data, out_data, params, node_request, 0);
        }

        if (!params[kGraphEditCommitId] || params[kGraphEditCommitId]->param_type != PF_Param_FLOAT_SLIDER ||
            !std::isfinite(params[kGraphEditCommitId]->u.fs_d.value) ||
            params[kGraphEditCommitId]->u.fs_d.value < 0.0 ||
            params[kGraphEditCommitId]->u.fs_d.value > kMaxTransactionNonce ||
            std::floor(params[kGraphEditCommitId]->u.fs_d.value) != params[kGraphEditCommitId]->u.fs_d.value) {
            return reject_request(out_data, params, 1, "invalid transaction nonce");
        }
        nonce = static_cast<A_long>(params[kGraphEditCommitId]->u.fs_d.value);

        if (!params[kGraphSnapshotId] || !params[kGraphEditRequestId] || !params[kGraphEditReceiptId] ||
            params[kGraphSnapshotId]->param_type != PF_Param_FLOAT_SLIDER ||
            params[kGraphEditRequestId]->param_type != PF_Param_FLOAT_SLIDER ||
            params[kGraphEditReceiptId]->param_type != PF_Param_FLOAT_SLIDER ||
            !params[kGraphParameterId] || params[kGraphParameterId]->param_type != PF_Param_ARBITRARY_DATA ||
            !params[kGraphParameterId]->u.arb_d.value || !params[kControlSourceId] ||
            params[kControlSourceId]->param_type != PF_Param_POPUP) {
            return reject_request(out_data, params, nonce, "carrier parameters are unavailable");
        }

        if (read_request != PF_Err_NONE) return reject_request(out_data, params, nonce, "request expression is unavailable");
        if (nonce < 1) return reject_request(out_data, params, 1, "invalid transaction nonce");

        TransactionRequest request;
        if (parse_sync_request(request_expression, request)) {
            if (request.nonce != nonce) return reject_request(out_data, params, nonce, "request nonce does not match");
            auto graph = read_graph_parameter(in_data, params[kGraphParameterId]->u.arb_d.value);
            if (!graph.has_value()) return reject_request(out_data, params, nonce, "stored graph is invalid");
            std::uint64_t revision = 0;
            core::OpaqueBytes previous;
            if (read_current_snapshot(in_data, revision, previous) != PF_Err_NONE) revision = 0;
            if (revision >= std::numeric_limits<std::uint32_t>::max()) {
                return reject_request(out_data, params, nonce, "graph revision limit reached");
            }
            ++revision;
            const auto encoded = core::serialize_graph(graph.value(), core::particle_node_registry());
            if (!encoded.has_value() || encoded.value().size() > kMaxCarrierGraphBytes) {
                return reject_request(out_data, params, nonce, "graph exceeds the carrier size limit");
            }
            const auto snapshot = encode_snapshot_expression(revision, encoded.value());
            if (snapshot.empty() || write_parameter_expression(in_data, kGraphSnapshotId, snapshot) != PF_Err_NONE) {
                return reject_request(out_data, params, nonce, "graph snapshot could not be initialized");
            }
            set_receipt(params, nonce);
            return PF_Err_NONE;
        }

        if (!parse_transaction_request(request_expression, request) || request.sync || request.nonce != nonce) {
            return reject_request(out_data, params, nonce, "request envelope is malformed");
        }
        std::uint64_t current_revision = 0;
        core::OpaqueBytes current_bytes;
        if (read_current_snapshot(in_data, current_revision, current_bytes) != PF_Err_NONE ||
            request.base_revision != current_revision) {
            return reject_request(out_data, params, nonce, "graph revision is stale");
        }
        const auto decoded = core::deserialize_graph(request.bytes, core::particle_node_registry());
        if (!decoded.has_value()) return reject_request(out_data, params, nonce, "graph validation failed");
        if (current_revision >= std::numeric_limits<std::uint32_t>::max()) {
            return reject_request(out_data, params, nonce, "graph revision limit reached");
        }
        PF_ArbitraryH replacement = nullptr;
        const PF_Err created = create_graph_parameter(in_data, decoded.value(), &replacement);
        if (created != PF_Err_NONE) return reject_request(out_data, params, nonce, "graph allocation failed", created);
        const std::uint64_t next_revision = current_revision + 1;
        const auto snapshot = encode_snapshot_expression(next_revision, request.bytes);
        if (snapshot.empty()) {
            in_data->utils->host_dispose_handle(replacement);
            return reject_request(out_data, params, nonce, "graph exceeds the carrier size limit");
        }
        const PF_Err snapshot_error = write_parameter_expression(in_data, kGraphSnapshotId, snapshot);
        if (snapshot_error != PF_Err_NONE) {
            in_data->utils->host_dispose_handle(replacement);
            return reject_request(out_data, params, nonce, "graph snapshot write failed");
        }
        // All fallible validation/allocation and the expression write precede these
        // host-owned parameter mutations. AE records both values in the supervised
        // commit transaction; the replaced arbitrary handle remains host-owned.
        params[kGraphParameterId]->u.arb_d.value = replacement;
        params[kGraphParameterId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        params[kControlSourceId]->u.pd.value = kNodeControlSource;
        params[kControlSourceId]->uu.change_flags |= PF_ChangeFlag_CHANGED_VALUE;
        set_receipt(params, nonce);
        return PF_Err_NONE;
    } catch (const std::bad_alloc&) {
        return reject_request(out_data, params, nonce, "transaction allocation failed", PF_Err_OUT_OF_MEMORY);
    } catch (...) {
        return reject_request(out_data, params, nonce, "unexpected transaction failure", PF_Err_INTERNAL_STRUCT_DAMAGED);
    }
}

} // namespace starfield::adapter
