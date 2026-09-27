#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <utility>
#include <vector>

namespace starfield::core {
namespace {

constexpr std::array<std::byte, 8> kMagic{
    std::byte{'S'}, std::byte{'F'}, std::byte{'L'}, std::byte{'D'},
    std::byte{'S'}, std::byte{'E'}, std::byte{'Q'}, std::byte{0},
};
constexpr std::uint16_t kNodeRecordKind = 1;
constexpr std::uint16_t kEdgeRecordKind = 2;
constexpr std::uint16_t kRecordVersion = 1;

constexpr std::array<std::uint32_t, 256> make_crc_table() noexcept {
    std::array<std::uint32_t, 256> table{};
    for (std::uint32_t index = 0; index < table.size(); ++index) {
        std::uint32_t value = index;
        for (unsigned bit = 0; bit < 8; ++bit) {
            value = (value >> 1u) ^ ((0u - (value & 1u)) & 0xedb88320u);
        }
        table[index] = value;
    }
    return table;
}

constexpr auto kCrcTable = make_crc_table();

SequenceResult<OpaqueBytes> encode_failure(SequenceErrorCode code, const char* detail,
                                           GraphError graph_error = {}) noexcept {
    return SequenceResult<OpaqueBytes>::failure(SequenceError{code, detail, graph_error});
}

SequenceResult<Graph> decode_failure(SequenceErrorCode code, const char* detail,
                                     GraphError graph_error = {}) noexcept {
    return SequenceResult<Graph>::failure(SequenceError{code, detail, graph_error});
}

class Writer {
public:
    explicit Writer(OpaqueBytes& bytes) : bytes_(bytes) {}

    void u8(std::uint8_t value) { bytes_.push_back(static_cast<std::byte>(value)); }
    void u16(std::uint16_t value) {
        u8(static_cast<std::uint8_t>(value & 0xffu));
        u8(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
    }
    void u32(std::uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) u8(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
    void u64(std::uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) u8(static_cast<std::uint8_t>((value >> shift) & 0xffu));
    }
    void raw(std::span<const std::byte> bytes) { bytes_.insert(bytes_.end(), bytes.begin(), bytes.end()); }
    void text(const std::string& value) {
        for (const unsigned char character : value) u8(character);
    }
    void uuid(const Uuid128& value) {
        for (const auto byte : value.bytes) u8(byte);
    }
    void patch_u32(std::size_t offset, std::uint32_t value) noexcept {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            bytes_[offset + shift / 8] = static_cast<std::byte>((value >> shift) & 0xffu);
        }
    }

private:
    OpaqueBytes& bytes_;
};

class Reader {
public:
    explicit Reader(std::span<const std::byte> bytes) : bytes_(bytes) {}

    [[nodiscard]] std::size_t remaining() const noexcept { return bytes_.size() - offset_; }
    [[nodiscard]] bool empty() const noexcept { return offset_ == bytes_.size(); }

    bool u8(std::uint8_t& value) noexcept {
        if (remaining() < 1) return false;
        value = std::to_integer<std::uint8_t>(bytes_[offset_++]);
        return true;
    }
    bool u16(std::uint16_t& value) noexcept {
        if (remaining() < 2) return false;
        value = static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes_[offset_])) |
                static_cast<std::uint16_t>(static_cast<std::uint16_t>(std::to_integer<std::uint8_t>(bytes_[offset_ + 1])) << 8u);
        offset_ += 2;
        return true;
    }
    bool u32(std::uint32_t& value) noexcept {
        if (remaining() < 4) return false;
        value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8) {
            value |= static_cast<std::uint32_t>(std::to_integer<std::uint8_t>(bytes_[offset_++])) << shift;
        }
        return true;
    }
    bool u64(std::uint64_t& value) noexcept {
        if (remaining() < 8) return false;
        value = 0;
        for (unsigned shift = 0; shift < 64; shift += 8) {
            value |= static_cast<std::uint64_t>(std::to_integer<std::uint8_t>(bytes_[offset_++])) << shift;
        }
        return true;
    }
    bool take(std::size_t count, std::span<const std::byte>& result) noexcept {
        if (count > remaining()) return false;
        result = bytes_.subspan(offset_, count);
        offset_ += count;
        return true;
    }
    bool uuid(Uuid128& value) noexcept {
        std::span<const std::byte> raw_bytes;
        if (!take(value.bytes.size(), raw_bytes)) return false;
        for (std::size_t index = 0; index < value.bytes.size(); ++index) {
            value.bytes[index] = std::to_integer<std::uint8_t>(raw_bytes[index]);
        }
        return true;
    }
    bool text(std::size_t count, std::string& value) {
        std::span<const std::byte> raw_bytes;
        if (!take(count, raw_bytes)) return false;
        value.clear();
        value.reserve(count);
        for (const auto byte : raw_bytes) value.push_back(static_cast<char>(std::to_integer<std::uint8_t>(byte)));
        return true;
    }

private:
    std::span<const std::byte> bytes_;
    std::size_t offset_{0};
};

std::uint32_t crc32(std::span<const std::byte> bytes) noexcept {
    std::uint32_t crc = 0xffffffffu;
    for (const auto byte : bytes) {
        const auto index = static_cast<std::uint8_t>(crc ^ std::to_integer<std::uint8_t>(byte));
        crc = (crc >> 8u) ^ kCrcTable[index];
    }
    return crc ^ 0xffffffffu;
}

bool add_size(std::uint64_t amount, std::uint64_t& total) noexcept {
    if (total > kMaxGraphPayloadBytes || amount > kMaxGraphPayloadBytes - total) return false;
    total += amount;
    return true;
}

std::uint16_t value_type_id(ParameterKind kind) noexcept {
    return static_cast<std::uint16_t>(static_cast<std::uint8_t>(kind) + 1u);
}

std::uint32_t parameter_value_size(const ParameterValue& value) noexcept {
    switch (static_cast<ParameterKind>(value.index())) {
        case ParameterKind::boolean: return 1;
        case ParameterKind::int32:
        case ParameterKind::uint32: return 4;
        case ParameterKind::float64: return 8;
        case ParameterKind::vector3_float64: return 24;
        case ParameterKind::utf8: return static_cast<std::uint32_t>(std::get<std::string>(value).size());
        case ParameterKind::opaque_bytes: return static_cast<std::uint32_t>(std::get<OpaqueBytes>(value).size());
    }
    return 0;
}

void write_value(Writer& writer, const ParameterValue& value) {
    switch (static_cast<ParameterKind>(value.index())) {
        case ParameterKind::boolean:
            writer.u8(std::get<bool>(value) ? 1 : 0);
            break;
        case ParameterKind::int32:
            writer.u32(std::bit_cast<std::uint32_t>(std::get<std::int32_t>(value)));
            break;
        case ParameterKind::uint32:
            writer.u32(std::get<std::uint32_t>(value));
            break;
        case ParameterKind::float64:
            writer.u64(std::bit_cast<std::uint64_t>(std::get<double>(value)));
            break;
        case ParameterKind::vector3_float64: {
            const Vec3 vector = std::get<Vec3>(value);
            writer.u64(std::bit_cast<std::uint64_t>(vector.x));
            writer.u64(std::bit_cast<std::uint64_t>(vector.y));
            writer.u64(std::bit_cast<std::uint64_t>(vector.z));
            break;
        }
        case ParameterKind::utf8:
            writer.text(std::get<std::string>(value));
            break;
        case ParameterKind::opaque_bytes:
            writer.raw(std::get<OpaqueBytes>(value));
            break;
    }
}

std::uint64_t value_wire_size(const ParameterValue& value) noexcept {
    return static_cast<std::uint64_t>(parameter_value_size(value));
}

SequenceError parse_value(std::uint16_t type_id, std::uint32_t value_size, Reader& reader,
                          ParameterValue& value) {
    std::span<const std::byte> raw;
    if (value_size > reader.remaining() || !reader.take(value_size, raw)) {
        return SequenceError{SequenceErrorCode::malformed_record, "parameter value exceeds its node record"};
    }
    Reader value_reader(raw);
    std::uint8_t byte = 0;
    std::uint32_t u32 = 0;
    std::uint64_t u64 = 0;
    switch (type_id) {
        case 1:
            if (value_size != 1 || !value_reader.u8(byte) || byte > 1) {
                return SequenceError{SequenceErrorCode::invalid_value, "bool parameter must be one byte with value 0 or 1"};
            }
            value = (byte != 0);
            break;
        case 2:
            if (value_size != 4 || !value_reader.u32(u32)) {
                return SequenceError{SequenceErrorCode::invalid_value, "i32 parameter must be exactly four bytes"};
            }
            value = std::bit_cast<std::int32_t>(u32);
            break;
        case 3:
            if (value_size != 4 || !value_reader.u32(u32)) {
                return SequenceError{SequenceErrorCode::invalid_value, "u32 parameter must be exactly four bytes"};
            }
            value = u32;
            break;
        case 4:
            if (value_size != 8 || !value_reader.u64(u64)) {
                return SequenceError{SequenceErrorCode::invalid_value, "f64 parameter must be exactly eight bytes"};
            }
            value = std::bit_cast<double>(u64);
            if (!std::isfinite(std::get<double>(value))) {
                return SequenceError{SequenceErrorCode::invalid_value, "f64 parameter must be finite"};
            }
            break;
        case 5: {
            if (value_size != 24) {
                return SequenceError{SequenceErrorCode::invalid_value, "vec3 parameter must be exactly 24 bytes"};
            }
            Vec3 vector;
            if (!value_reader.u64(u64)) return SequenceError{SequenceErrorCode::malformed_record, "truncated vec3 value"};
            vector.x = std::bit_cast<double>(u64);
            if (!value_reader.u64(u64)) return SequenceError{SequenceErrorCode::malformed_record, "truncated vec3 value"};
            vector.y = std::bit_cast<double>(u64);
            if (!value_reader.u64(u64)) return SequenceError{SequenceErrorCode::malformed_record, "truncated vec3 value"};
            vector.z = std::bit_cast<double>(u64);
            if (!std::isfinite(vector.x) || !std::isfinite(vector.y) || !std::isfinite(vector.z)) {
                return SequenceError{SequenceErrorCode::invalid_value, "vec3 values must be finite"};
            }
            value = vector;
            break;
        }
        case 6: {
            std::string text;
            if (!value_reader.text(value_size, text)) {
                return SequenceError{SequenceErrorCode::malformed_record, "UTF-8 parameter is truncated"};
            }
            value = std::move(text);
            break;
        }
        case 7: {
            std::span<const std::byte> opaque;
            if (!value_reader.take(value_size, opaque)) {
                return SequenceError{SequenceErrorCode::malformed_record, "opaque parameter is truncated"};
            }
            value = OpaqueBytes(opaque.begin(), opaque.end());
            break;
        }
        default:
            return SequenceError{SequenceErrorCode::unsupported_value_type,
                                 "unknown required parameter value type"};
    }
    if (!value_reader.empty()) {
        return SequenceError{SequenceErrorCode::invalid_value, "parameter value contains trailing bytes"};
    }
    return {};
}

std::uint64_t node_record_size(const GraphNode& node) noexcept {
    std::uint64_t size = 30u + static_cast<std::uint64_t>(node.type_key.size());
    for (const auto& parameter : node.parameters) {
        size += 14u + value_wire_size(parameter.value);
    }
    return size;
}

void write_node_record(Writer& writer, const GraphNode& node) {
    const auto record_size = static_cast<std::uint32_t>(node_record_size(node));
    writer.u16(kNodeRecordKind);
    writer.u16(kRecordVersion);
    writer.u32(record_size);
    writer.uuid(node.id.value);
    writer.u16(static_cast<std::uint16_t>(node.type_key.size()));
    writer.text(node.type_key);
    writer.u16(node.schema_version);
    writer.u16(static_cast<std::uint16_t>(node.parameters.size()));

    std::array<const NodeParameter*, kMaxNodeParameters> parameters{};
    for (std::size_t i = 0; i < node.parameters.size(); ++i) parameters[i] = &node.parameters[i];
    std::sort(parameters.begin(), parameters.begin() + static_cast<std::ptrdiff_t>(node.parameters.size()),
              [](const auto* left, const auto* right) { return left->key < right->key; });
    for (std::size_t i = 0; i < node.parameters.size(); ++i) {
        const NodeParameter& parameter = *parameters[i];
        const auto kind = static_cast<ParameterKind>(parameter.value.index());
        writer.u64(parameter.key.value);
        writer.u16(value_type_id(kind));
        writer.u32(parameter_value_size(parameter.value));
        write_value(writer, parameter.value);
    }
}

void write_edge_record(Writer& writer, const GraphEdge& edge) {
    writer.u16(kEdgeRecordKind);
    writer.u16(kRecordVersion);
    writer.u32(72);
    writer.uuid(edge.id.value);
    writer.uuid(edge.source_node.value);
    writer.u64(edge.source_port.value);
    writer.uuid(edge.destination_node.value);
    writer.u64(edge.destination_port.value);
}

SequenceError read_node_record(Reader& reader, GraphNode& node) {
    std::uint16_t type_key_length = 0;
    std::uint16_t parameter_count = 0;
    if (!reader.uuid(node.id.value) || !reader.u16(type_key_length) || type_key_length == 0 ||
        type_key_length > kMaxGraphTypeKeyBytes || !reader.text(type_key_length, node.type_key) ||
        !reader.u16(node.schema_version) || !reader.u16(parameter_count) ||
        parameter_count > kMaxNodeParameters) {
        return SequenceError{SequenceErrorCode::malformed_record, "node record header is truncated or out of range"};
    }

    node.parameters.reserve(parameter_count);
    for (std::uint16_t index = 0; index < parameter_count; ++index) {
        std::uint64_t key = 0;
        std::uint16_t type_id = 0;
        std::uint32_t value_size = 0;
        if (!reader.u64(key) || !reader.u16(type_id) || !reader.u32(value_size)) {
            return SequenceError{SequenceErrorCode::malformed_record, "parameter record header is truncated"};
        }
        ParameterValue value;
        const SequenceError value_error = parse_value(type_id, value_size, reader, value);
        if (value_error.code != SequenceErrorCode::none) return value_error;
        node.parameters.push_back(NodeParameter{ParameterKey{key}, std::move(value)});
    }
    if (!reader.empty()) {
        return SequenceError{SequenceErrorCode::malformed_record, "node record contains unparsed trailing bytes"};
    }
    return {};
}

SequenceError read_edge_record(Reader& reader, GraphEdge& edge) noexcept {
    if (!reader.uuid(edge.id.value) || !reader.uuid(edge.source_node.value) || !reader.u64(edge.source_port.value) ||
        !reader.uuid(edge.destination_node.value) || !reader.u64(edge.destination_port.value) || !reader.empty()) {
        return SequenceError{SequenceErrorCode::malformed_record, "edge record has an invalid size or truncated fields"};
    }
    return {};
}

bool is_error(const SequenceError& error) noexcept {
    return error.code != SequenceErrorCode::none;
}

} // namespace

const char* describe(SequenceErrorCode code) noexcept {
    switch (code) {
        case SequenceErrorCode::none: return "no sequence codec error";
        case SequenceErrorCode::invalid_header: return "sequence header is invalid";
        case SequenceErrorCode::unsupported_format_version: return "sequence format version is unsupported";
        case SequenceErrorCode::unsupported_flags: return "sequence format flags are unsupported";
        case SequenceErrorCode::length_mismatch: return "sequence payload length does not match the blob";
        case SequenceErrorCode::size_limit_exceeded: return "sequence blob exceeds a bounded size";
        case SequenceErrorCode::checksum_mismatch: return "sequence payload CRC-32 does not match";
        case SequenceErrorCode::malformed_record: return "sequence record is malformed";
        case SequenceErrorCode::unsupported_record: return "sequence contains an unknown required record";
        case SequenceErrorCode::unsupported_record_version: return "record schema version is unsupported";
        case SequenceErrorCode::unsupported_value_type: return "parameter value type is unsupported";
        case SequenceErrorCode::invalid_value: return "parameter value encoding is invalid";
        case SequenceErrorCode::invalid_graph: return "decoded graph does not match a supported node schema";
        case SequenceErrorCode::allocation_failed: return "sequence codec allocation failed";
        case SequenceErrorCode::internal_failure: return "sequence codec failed unexpectedly";
    }
    return "unknown sequence codec error";
}

SequenceResult<OpaqueBytes> serialize_graph(const Graph& graph, const NodeRegistry& registry) noexcept {
    static_assert(sizeof(double) == sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559);
    try {
        const GraphValidationResult validation = validate_graph(graph, registry);
        if (!validation) {
            return encode_failure(SequenceErrorCode::invalid_graph, describe(SequenceErrorCode::invalid_graph),
                                 validation.error);
        }

        std::uint64_t total_size = kSequenceHeaderSize;
        for (const auto& node : graph.nodes) {
            if (!add_size(node_record_size(node), total_size)) {
                return encode_failure(SequenceErrorCode::size_limit_exceeded, describe(SequenceErrorCode::size_limit_exceeded));
            }
        }
        for (std::size_t index = 0; index < graph.edges.size(); ++index) {
            if (!add_size(72, total_size)) {
                return encode_failure(SequenceErrorCode::size_limit_exceeded, describe(SequenceErrorCode::size_limit_exceeded));
            }
        }
        const std::uint64_t payload_size = total_size - kSequenceHeaderSize;
        if (payload_size > std::numeric_limits<std::uint32_t>::max()) {
            return encode_failure(SequenceErrorCode::size_limit_exceeded, describe(SequenceErrorCode::size_limit_exceeded));
        }

        std::vector<const GraphNode*> nodes;
        std::vector<const GraphEdge*> edges;
        nodes.reserve(graph.nodes.size());
        edges.reserve(graph.edges.size());
        for (const auto& node : graph.nodes) nodes.push_back(&node);
        for (const auto& edge : graph.edges) edges.push_back(&edge);
        std::sort(nodes.begin(), nodes.end(), [](const auto* left, const auto* right) { return left->id < right->id; });
        std::sort(edges.begin(), edges.end(), [](const auto* left, const auto* right) { return left->id < right->id; });

        OpaqueBytes bytes;
        bytes.reserve(static_cast<std::size_t>(total_size));
        Writer writer(bytes);
        writer.raw(kMagic);
        writer.u16(kSequenceFormatVersion);
        writer.u16(kSequenceHeaderSize);
        writer.u32(static_cast<std::uint32_t>(payload_size));
        writer.u32(static_cast<std::uint32_t>(nodes.size()));
        writer.u32(static_cast<std::uint32_t>(edges.size()));
        writer.u32(0); // payload CRC, patched after writing records
        writer.u32(0); // schema 1 flags
        for (const GraphNode* node : nodes) write_node_record(writer, *node);
        for (const GraphEdge* edge : edges) write_edge_record(writer, *edge);
        const auto payload = std::span<const std::byte>(bytes).subspan(kSequenceHeaderSize);
        writer.patch_u32(24, crc32(payload));
        return SequenceResult<OpaqueBytes>::success(std::move(bytes));
    } catch (const std::bad_alloc&) {
        return encode_failure(SequenceErrorCode::allocation_failed, describe(SequenceErrorCode::allocation_failed));
    } catch (...) {
        return encode_failure(SequenceErrorCode::internal_failure, describe(SequenceErrorCode::internal_failure));
    }
}

SequenceResult<Graph> deserialize_graph(std::span<const std::byte> bytes, const NodeRegistry& registry) noexcept {
    static_assert(sizeof(double) == sizeof(std::uint64_t) && std::numeric_limits<double>::is_iec559);
    try {
        if (bytes.size() > kMaxGraphPayloadBytes) {
            return decode_failure(SequenceErrorCode::size_limit_exceeded, describe(SequenceErrorCode::size_limit_exceeded));
        }
        if (bytes.size() < kSequenceHeaderSize) {
            return decode_failure(SequenceErrorCode::invalid_header, "sequence header is truncated");
        }

        Reader header(bytes.first(kSequenceHeaderSize));
        std::span<const std::byte> magic;
        std::uint16_t version = 0;
        std::uint16_t header_size = 0;
        std::uint32_t payload_size = 0;
        std::uint32_t node_count = 0;
        std::uint32_t edge_count = 0;
        std::uint32_t checksum = 0;
        std::uint32_t flags = 0;
        if (!header.take(kMagic.size(), magic) || !std::equal(magic.begin(), magic.end(), kMagic.begin()) ||
            !header.u16(version) || !header.u16(header_size) || !header.u32(payload_size) ||
            !header.u32(node_count) || !header.u32(edge_count) || !header.u32(checksum) || !header.u32(flags) ||
            !header.empty()) {
            return decode_failure(SequenceErrorCode::invalid_header, describe(SequenceErrorCode::invalid_header));
        }
        if (version != kSequenceFormatVersion) {
            return decode_failure(SequenceErrorCode::unsupported_format_version,
                                  describe(SequenceErrorCode::unsupported_format_version));
        }
        if (header_size != kSequenceHeaderSize) {
            return decode_failure(SequenceErrorCode::invalid_header, "schema 1 header size must be exactly 32 bytes");
        }
        if (flags != 0) {
            return decode_failure(SequenceErrorCode::unsupported_flags, describe(SequenceErrorCode::unsupported_flags));
        }
        if (node_count > kMaxGraphNodes || edge_count > kMaxGraphEdges) {
            return decode_failure(SequenceErrorCode::size_limit_exceeded, "header node or edge count exceeds its limit");
        }
        if (payload_size != bytes.size() - kSequenceHeaderSize) {
            return decode_failure(SequenceErrorCode::length_mismatch, describe(SequenceErrorCode::length_mismatch));
        }
        const auto payload = bytes.subspan(kSequenceHeaderSize, payload_size);
        if (crc32(payload) != checksum) {
            return decode_failure(SequenceErrorCode::checksum_mismatch, describe(SequenceErrorCode::checksum_mismatch));
        }

        Graph graph;
        graph.nodes.reserve(node_count);
        graph.edges.reserve(edge_count);
        Reader records(payload);
        std::uint32_t parsed_nodes = 0;
        std::uint32_t parsed_edges = 0;
        while (!records.empty()) {
            std::uint16_t kind = 0;
            std::uint16_t record_version = 0;
            std::uint32_t record_size = 0;
            if (!records.u16(kind) || !records.u16(record_version) || !records.u32(record_size) || record_size < 8 ||
                record_size - 8u > records.remaining()) {
                return decode_failure(SequenceErrorCode::malformed_record, "record header or record size is invalid");
            }
            std::span<const std::byte> record_bytes;
            if (!records.take(record_size - 8u, record_bytes)) {
                return decode_failure(SequenceErrorCode::malformed_record, describe(SequenceErrorCode::malformed_record));
            }
            if (kind == kNodeRecordKind || kind == kEdgeRecordKind) {
                if (record_version != kRecordVersion) {
                    return decode_failure(SequenceErrorCode::unsupported_record_version,
                                          describe(SequenceErrorCode::unsupported_record_version));
                }
                Reader record(record_bytes);
                if (kind == kNodeRecordKind) {
                    if (parsed_nodes >= node_count) {
                        return decode_failure(SequenceErrorCode::malformed_record, "more node records than declared");
                    }
                    GraphNode node;
                    const SequenceError error = read_node_record(record, node);
                    if (is_error(error)) return decode_failure(error.code, error.detail);
                    graph.nodes.push_back(std::move(node));
                    ++parsed_nodes;
                } else {
                    if (parsed_edges >= edge_count) {
                        return decode_failure(SequenceErrorCode::malformed_record, "more edge records than declared");
                    }
                    GraphEdge edge;
                    const SequenceError error = read_edge_record(record, edge);
                    if (is_error(error)) return decode_failure(error.code, error.detail);
                    graph.edges.push_back(edge);
                    ++parsed_edges;
                }
            } else if ((kind & 0x8000u) == 0) {
                return decode_failure(SequenceErrorCode::unsupported_record,
                                      describe(SequenceErrorCode::unsupported_record));
            }
        }
        if (parsed_nodes != node_count || parsed_edges != edge_count) {
            return decode_failure(SequenceErrorCode::length_mismatch, "record counts do not match the sequence header");
        }

        const GraphValidationResult validation = validate_graph(graph, registry);
        if (!validation) {
            return decode_failure(SequenceErrorCode::invalid_graph, describe(SequenceErrorCode::invalid_graph),
                                  validation.error);
        }
        return SequenceResult<Graph>::success(std::move(graph));
    } catch (const std::bad_alloc&) {
        return decode_failure(SequenceErrorCode::allocation_failed, describe(SequenceErrorCode::allocation_failed));
    } catch (...) {
        return decode_failure(SequenceErrorCode::internal_failure, describe(SequenceErrorCode::internal_failure));
    }
}

} // namespace starfield::core
