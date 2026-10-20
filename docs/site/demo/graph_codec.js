// Structural schema-1 codec for the CEP graph editor. The native registry remains
// authoritative for node types, parameter schemas, ports, cardinality, and cycles.
(function (root, factory) {
    var codec = factory();
    if (typeof module === "object" && module.exports) module.exports = codec;
    else root.StarfieldGraphCodec = codec;
}(typeof window !== "undefined" ? window : this, function () {
    "use strict";

    var FORMAT_VERSION = 1;
    var HEADER_SIZE = 32;
    var MAX_BYTES = 64 * 1024 * 1024;
    var MAX_NODES = 4096;
    var MAX_EDGES = 16384;
    var MAX_OPTIONAL_RECORDS = 4096;
    var MAX_TYPE_KEY_BYTES = 128;
    var MAX_PARAMETERS = 512;
    var MAX_VALUE_BYTES = 64 * 1024 * 1024;
    var U64_MAX = "18446744073709551615";
    var MAX_SAFE_INTEGER = 9007199254740991;
    var MAGIC = [83, 70, 76, 68, 83, 69, 81, 0];

    function error(code, message) {
        var result = new Error(message);
        result.code = code;
        return result;
    }

    function requireCondition(condition, code, message) {
        if (!condition) throw error(code, message);
    }

    function bytesOf(value) {
        if (value instanceof Uint8Array) return value;
        if (typeof ArrayBuffer !== "undefined" && value instanceof ArrayBuffer) return new Uint8Array(value);
        if (Object.prototype.toString.call(value) === "[object Array]") {
            for (var i = 0; i < value.length; i++) {
                requireCondition(value[i] >= 0 && value[i] <= 255 && Math.floor(value[i]) === value[i],
                                 "invalid_value", "byte array contains a non-byte value");
            }
            return new Uint8Array(value);
        }
        throw error("invalid_value", "expected byte data");
    }

    function bytesToHex(bytes) {
        var result = "";
        for (var i = 0; i < bytes.length; i++) {
            var value = bytes[i].toString(16);
            result += value.length === 1 ? "0" + value : value;
        }
        return result;
    }

    function hexToBytes(hex) {
        requireCondition(typeof hex === "string" && hex.length % 2 === 0 && /^[0-9a-fA-F]*$/.test(hex),
                         "invalid_value", "hex graph payload is malformed");
        requireCondition(hex.length <= MAX_BYTES * 2, "size_limit_exceeded", "graph exceeds the 64 MiB limit");
        var result = new Uint8Array(hex.length / 2);
        for (var i = 0; i < result.length; i++) result[i] = parseInt(hex.substr(i * 2, 2), 16);
        return result;
    }

    function crc32(bytes) {
        var crc = 0xffffffff;
        for (var i = 0; i < bytes.length; i++) {
            crc ^= bytes[i];
            for (var bit = 0; bit < 8; bit++) crc = (crc >>> 1) ^ ((crc & 1) ? 0xedb88320 : 0);
        }
        return (crc ^ 0xffffffff) >>> 0;
    }

    function decimalNormalize(value) {
        if (typeof value === "number") {
            requireCondition(isFinite(value) && value >= 0 && Math.floor(value) === value && value <= MAX_SAFE_INTEGER,
                             "invalid_value", "64-bit key as Number must be a safe non-negative integer");
            value = String(value);
        }
        requireCondition(typeof value === "string" && /^\d+$/.test(value), "invalid_value", "64-bit key must be decimal digits");
        value = value.replace(/^0+(?=\d)/, "");
        requireCondition(value.length < U64_MAX.length ||
                         (value.length === U64_MAX.length && value <= U64_MAX),
                         "invalid_value", "64-bit key exceeds its unsigned range");
        return value;
    }

    function decimalMultiplyAdd(decimal, multiplier, addend) {
        var carry = addend;
        var output = "";
        for (var i = decimal.length - 1; i >= 0; i--) {
            var value = (decimal.charCodeAt(i) - 48) * multiplier + carry;
            output = String(value % 10) + output;
            carry = Math.floor(value / 10);
        }
        while (carry > 0) {
            output = String(carry % 10) + output;
            carry = Math.floor(carry / 10);
        }
        return output.replace(/^0+(?=\d)/, "");
    }

    function decimalDivide(decimal, divisor) {
        var quotient = "";
        var remainder = 0;
        for (var i = 0; i < decimal.length; i++) {
            var value = remainder * 10 + decimal.charCodeAt(i) - 48;
            var digit = Math.floor(value / divisor);
            remainder = value % divisor;
            if (quotient.length || digit) quotient += String(digit);
        }
        return { quotient: quotient || "0", remainder: remainder };
    }

    function compareU64(left, right) {
        left = decimalNormalize(left);
        right = decimalNormalize(right);
        if (left.length !== right.length) return left.length < right.length ? -1 : 1;
        return left === right ? 0 : left < right ? -1 : 1;
    }

    function Reader(bytes) {
        this.bytes = bytes;
        this.offset = 0;
    }
    Reader.prototype.remaining = function () { return this.bytes.length - this.offset; };
    Reader.prototype.need = function (count) {
        requireCondition(count >= 0 && count <= this.remaining(), "malformed_record", "record is truncated");
    };
    Reader.prototype.u8 = function () {
        this.need(1);
        return this.bytes[this.offset++];
    };
    Reader.prototype.u16 = function () {
        this.need(2);
        var value = this.bytes[this.offset] | (this.bytes[this.offset + 1] << 8);
        this.offset += 2;
        return value;
    };
    Reader.prototype.u32 = function () {
        this.need(4);
        var value = (this.bytes[this.offset] | (this.bytes[this.offset + 1] << 8) |
                     (this.bytes[this.offset + 2] << 16) | (this.bytes[this.offset + 3] << 24)) >>> 0;
        this.offset += 4;
        return value;
    };
    Reader.prototype.u64 = function () {
        this.need(8);
        var value = "0";
        for (var i = this.offset + 7; i >= this.offset; i--) value = decimalMultiplyAdd(value, 256, this.bytes[i]);
        this.offset += 8;
        return value;
    };
    Reader.prototype.f64 = function () {
        this.need(8);
        var view = new DataView(this.bytes.buffer, this.bytes.byteOffset + this.offset, 8);
        var value = view.getFloat64(0, true);
        this.offset += 8;
        requireCondition(isFinite(value), "invalid_value", "floating-point value must be finite");
        return value;
    };
    Reader.prototype.take = function (count) {
        this.need(count);
        var bytes = this.bytes.subarray(this.offset, this.offset + count);
        this.offset += count;
        return bytes;
    };

    function Writer(capacity) {
        this.bytes = new Uint8Array(Math.max(64, capacity || 64));
        this.offset = 0;
    }
    Writer.prototype.ensure = function (count) {
        var needed = this.offset + count;
        requireCondition(needed <= MAX_BYTES, "size_limit_exceeded", "graph exceeds the 64 MiB limit");
        if (needed <= this.bytes.length) return;
        var nextSize = this.bytes.length;
        while (nextSize < needed) nextSize = Math.min(MAX_BYTES, nextSize * 2);
        var next = new Uint8Array(nextSize);
        next.set(this.bytes.subarray(0, this.offset));
        this.bytes = next;
    };
    Writer.prototype.u8 = function (value) {
        this.ensure(1);
        this.bytes[this.offset++] = value & 255;
    };
    Writer.prototype.u16 = function (value) {
        this.ensure(2);
        this.u8(value);
        this.u8(value >>> 8);
    };
    Writer.prototype.u32 = function (value) {
        this.ensure(4);
        this.u8(value);
        this.u8(value >>> 8);
        this.u8(value >>> 16);
        this.u8(value >>> 24);
    };
    Writer.prototype.u64 = function (value) {
        var decimal = decimalNormalize(value);
        var output = [];
        for (var i = 0; i < 8; i++) {
            var division = decimalDivide(decimal, 256);
            output.push(division.remainder);
            decimal = division.quotient;
        }
        requireCondition(decimal === "0", "invalid_value", "64-bit key exceeds its unsigned range");
        this.raw(output);
    };
    Writer.prototype.f64 = function (value) {
        requireCondition(typeof value === "number" && isFinite(value), "invalid_value", "floating-point value must be finite");
        this.ensure(8);
        new DataView(this.bytes.buffer).setFloat64(this.offset, value, true);
        this.offset += 8;
    };
    Writer.prototype.raw = function (bytes) {
        if (Object.prototype.toString.call(bytes) === "[object Array]") bytes = new Uint8Array(bytes);
        else bytes = bytesOf(bytes);
        this.ensure(bytes.length);
        this.bytes.set(bytes, this.offset);
        this.offset += bytes.length;
    };
    Writer.prototype.finish = function () { return this.bytes.slice(0, this.offset); };

    function encodeUtf8(text, allowNul) {
        requireCondition(typeof text === "string", "invalid_value", "UTF-8 value must be text");
        var bytes = [];
        for (var i = 0; i < text.length; i++) {
            var code = text.charCodeAt(i);
            if (code >= 0xd800 && code <= 0xdbff) {
                requireCondition(i + 1 < text.length, "invalid_value", "UTF-8 value contains an unpaired surrogate");
                var low = text.charCodeAt(++i);
                requireCondition(low >= 0xdc00 && low <= 0xdfff, "invalid_value", "UTF-8 value contains an unpaired surrogate");
                code = 0x10000 + ((code - 0xd800) << 10) + (low - 0xdc00);
            } else {
                requireCondition(code < 0xdc00 || code > 0xdfff, "invalid_value", "UTF-8 value contains an unpaired surrogate");
            }
            requireCondition(allowNul || code !== 0, "invalid_value", "UTF-8 graph text must not contain NUL");
            if (code < 0x80) bytes.push(code);
            else if (code < 0x800) bytes.push(0xc0 | (code >> 6), 0x80 | (code & 63));
            else if (code < 0x10000) bytes.push(0xe0 | (code >> 12), 0x80 | ((code >> 6) & 63), 0x80 | (code & 63));
            else bytes.push(0xf0 | (code >> 18), 0x80 | ((code >> 12) & 63),
                            0x80 | ((code >> 6) & 63), 0x80 | (code & 63));
        }
        return new Uint8Array(bytes);
    }

    function decodeUtf8(bytes, allowNul) {
        var output = "";
        for (var i = 0; i < bytes.length;) {
            var first = bytes[i++];
            var code;
            var needed;
            var minimum;
            if (first <= 0x7f) { code = first; needed = 0; minimum = 0; }
            else if (first >= 0xc2 && first <= 0xdf) { code = first & 31; needed = 1; minimum = 0x80; }
            else if (first >= 0xe0 && first <= 0xef) { code = first & 15; needed = 2; minimum = 0x800; }
            else if (first >= 0xf0 && first <= 0xf4) { code = first & 7; needed = 3; minimum = 0x10000; }
            else throw error("invalid_value", "text contains invalid UTF-8");
            requireCondition(i + needed <= bytes.length, "invalid_value", "text contains truncated UTF-8");
            for (var n = 0; n < needed; n++) {
                var next = bytes[i++];
                requireCondition((next & 0xc0) === 0x80, "invalid_value", "text contains invalid UTF-8 continuation bytes");
                code = (code << 6) | (next & 63);
            }
            requireCondition(code >= minimum && code <= 0x10ffff && !(code >= 0xd800 && code <= 0xdfff),
                             "invalid_value", "text contains a non-canonical UTF-8 scalar");
            requireCondition(allowNul || code !== 0, "invalid_value", "UTF-8 graph text must not contain NUL");
            if (code <= 0xffff) output += String.fromCharCode(code);
            else {
                code -= 0x10000;
                output += String.fromCharCode(0xd800 + (code >> 10), 0xdc00 + (code & 1023));
            }
        }
        return output;
    }

    function decodeValue(type, bytes) {
        var reader = new Reader(bytes);
        var value;
        if (type === 1) {
            requireCondition(bytes.length === 1 && bytes[0] <= 1, "invalid_value", "bool must be one byte containing 0 or 1");
            value = bytes[0] === 1;
        } else if (type === 2) {
            requireCondition(bytes.length === 4, "invalid_value", "i32 must be four bytes");
            value = new DataView(bytes.buffer, bytes.byteOffset, 4).getInt32(0, true);
        } else if (type === 3) {
            requireCondition(bytes.length === 4, "invalid_value", "u32 must be four bytes");
            value = new DataView(bytes.buffer, bytes.byteOffset, 4).getUint32(0, true);
        } else if (type === 4) {
            requireCondition(bytes.length === 8, "invalid_value", "f64 must be eight bytes");
            value = reader.f64();
        } else if (type === 5) {
            requireCondition(bytes.length === 24, "invalid_value", "vec3_f64 must be 24 bytes");
            value = [reader.f64(), reader.f64(), reader.f64()];
        } else if (type === 6) {
            value = decodeUtf8(bytes, false);
        } else if (type === 7) {
            value = new Uint8Array(bytes);
        } else throw error("unsupported_value_type", "unknown required parameter value type");
        return value;
    }

    function encodeValue(parameter) {
        var type = parameter.type;
        var writer = new Writer(32);
        var value = parameter.value;
        if (type === 1) {
            requireCondition(typeof value === "boolean", "invalid_value", "bool parameter needs a boolean");
            writer.u8(value ? 1 : 0);
        } else if (type === 2) {
            requireCondition(typeof value === "number" && isFinite(value) && Math.floor(value) === value &&
                             value >= -2147483648 && value <= 2147483647,
                             "invalid_value", "i32 parameter is outside its range");
            writer.u32(value >>> 0);
        } else if (type === 3) {
            requireCondition(typeof value === "number" && isFinite(value) && Math.floor(value) === value &&
                             value >= 0 && value <= 4294967295,
                             "invalid_value", "u32 parameter is outside its range");
            writer.u32(value);
        } else if (type === 4) {
            writer.f64(value);
        } else if (type === 5) {
            requireCondition(Object.prototype.toString.call(value) === "[object Array]" && value.length === 3,
                             "invalid_value", "vec3_f64 parameter needs three values");
            writer.f64(value[0]); writer.f64(value[1]); writer.f64(value[2]);
        } else if (type === 6) {
            writer.raw(encodeUtf8(value, false));
        } else if (type === 7) {
            var opaque = bytesOf(value);
            requireCondition(opaque.length <= MAX_VALUE_BYTES, "size_limit_exceeded", "opaque parameter exceeds the graph size limit");
            writer.raw(opaque);
        } else throw error("unsupported_value_type", "unknown required parameter value type");
        return writer.finish();
    }

    function validTypeKey(bytes) {
        var key = "";
        for (var i = 0; i < bytes.length; i++) {
            requireCondition(bytes[i] >= 0x20 && bytes[i] <= 0x7e, "invalid_value", "node type key must be lowercase ASCII");
            key += String.fromCharCode(bytes[i]);
        }
        requireCondition(/^[a-z0-9]+(?:[.-][a-z0-9]+)*$/.test(key) && key.indexOf(".") >= 0,
                         "invalid_value", "node type key must be a lowercase reverse-DNS key");
        return key;
    }

    function normalizeTypeKey(key) {
        requireCondition(typeof key === "string", "invalid_value", "node type key must be text");
        var bytes = encodeUtf8(key, false);
        requireCondition(bytes.length >= 1 && bytes.length <= MAX_TYPE_KEY_BYTES,
                         "size_limit_exceeded", "node type key length is outside 1..128 bytes");
        requireCondition(validTypeKey(bytes) === key, "invalid_value", "node type key must be lowercase ASCII");
        return bytes;
    }

    function normalizeUuid(value, allowZero) {
        requireCondition(typeof value === "string" && /^[0-9a-fA-F]{32}$/.test(value),
                         "invalid_value", "UUID must be exactly 16 bytes in hexadecimal");
        value = value.toLowerCase();
        requireCondition(allowZero || !/^0{32}$/.test(value), "invalid_value", "zero UUID is reserved");
        return value;
    }

    function parseNode(reader) {
        var id = bytesToHex(reader.take(16));
        normalizeUuid(id, false);
        var typeLength = reader.u16();
        requireCondition(typeLength >= 1 && typeLength <= MAX_TYPE_KEY_BYTES, "malformed_record", "node type key length is invalid");
        var type = validTypeKey(reader.take(typeLength));
        var schemaVersion = reader.u16();
        requireCondition(schemaVersion !== 0, "malformed_record", "node schema version cannot be zero");
        var count = reader.u16();
        requireCondition(count <= MAX_PARAMETERS, "size_limit_exceeded", "node has too many parameters");
        var parameters = [];
        var seen = {};
        for (var i = 0; i < count; i++) {
            var key = reader.u64();
            var typeId = reader.u16();
            var valueSize = reader.u32();
            requireCondition(valueSize <= MAX_VALUE_BYTES, "size_limit_exceeded", "parameter value exceeds the graph size limit");
            requireCondition(!seen["$" + key], "invalid_graph", "node has duplicate parameter keys");
            seen["$" + key] = true;
            parameters.push({ key: key, type: typeId, value: decodeValue(typeId, reader.take(valueSize)) });
        }
        requireCondition(reader.remaining() === 0, "malformed_record", "node record contains trailing bytes");
        return { id: id, type: type, schemaVersion: schemaVersion, parameters: parameters };
    }

    function parseEdge(reader) {
        requireCondition(reader.remaining() === 64, "malformed_record", "edge record must contain exactly 64 bytes");
        var edge = {
            id: bytesToHex(reader.take(16)),
            sourceNode: bytesToHex(reader.take(16)),
            sourcePort: reader.u64(),
            destinationNode: bytesToHex(reader.take(16)),
            destinationPort: reader.u64()
        };
        normalizeUuid(edge.id, false);
        normalizeUuid(edge.sourceNode, false);
        normalizeUuid(edge.destinationNode, false);
        return edge;
    }

    function validateGraphShape(graph) {
        requireCondition(graph && Object.prototype.toString.call(graph.nodes) === "[object Array]" &&
                         Object.prototype.toString.call(graph.edges) === "[object Array]",
                         "invalid_graph", "graph must contain node and edge arrays");
        requireCondition(graph.version === FORMAT_VERSION, "unsupported_format_version", "graph format version is unsupported");
        requireCondition(graph.optionalRecords === undefined ||
                         Object.prototype.toString.call(graph.optionalRecords) === "[object Array]",
                         "invalid_graph", "optional records must be an array");
        requireCondition((graph.optionalRecords || []).length <= MAX_OPTIONAL_RECORDS,
                         "size_limit_exceeded", "optional record count exceeds the schema-1 limit");
        requireCondition(graph.nodes.length <= MAX_NODES && graph.edges.length <= MAX_EDGES,
                         "size_limit_exceeded", "node or edge count exceeds the schema-1 limit");
        var nodes = {};
        var edges = {};
        for (var i = 0; i < graph.nodes.length; i++) {
            var node = graph.nodes[i];
            node.id = normalizeUuid(node.id, false);
            nodes["$" + node.id] = nodes["$" + node.id] || 0;
            requireCondition(nodes["$" + node.id]++ === 0, "invalid_graph", "graph has duplicate node IDs");
            normalizeTypeKey(node.type);
            requireCondition(typeof node.schemaVersion === "number" && node.schemaVersion >= 1 &&
                             node.schemaVersion <= 65535 && Math.floor(node.schemaVersion) === node.schemaVersion,
                             "invalid_graph", "node schema version is invalid");
            requireCondition(Object.prototype.toString.call(node.parameters) === "[object Array]" &&
                             node.parameters.length <= MAX_PARAMETERS,
                             "size_limit_exceeded", "node parameter count exceeds its limit");
            var params = {};
            for (var p = 0; p < node.parameters.length; p++) {
                var parameter = node.parameters[p];
                parameter.key = decimalNormalize(parameter.key);
                requireCondition(!params["$" + parameter.key], "invalid_graph", "node has duplicate parameter keys");
                params["$" + parameter.key] = true;
                encodeValue(parameter);
            }
        }
        for (var e = 0; e < graph.edges.length; e++) {
            var edge = graph.edges[e];
            edge.id = normalizeUuid(edge.id, false);
            requireCondition(!edges["$" + edge.id], "invalid_graph", "graph has duplicate edge IDs");
            edges["$" + edge.id] = true;
            edge.sourceNode = normalizeUuid(edge.sourceNode, false);
            edge.destinationNode = normalizeUuid(edge.destinationNode, false);
            requireCondition(nodes["$" + edge.sourceNode] && nodes["$" + edge.destinationNode],
                             "invalid_graph", "edge references a missing node");
            edge.sourcePort = decimalNormalize(edge.sourcePort);
            edge.destinationPort = decimalNormalize(edge.destinationPort);
        }
    }

    function makeNodeRecord(node) {
        var typeBytes = normalizeTypeKey(node.type);
        var parameters = node.parameters.slice().sort(function (left, right) { return compareU64(left.key, right.key); });
        var encoded = [];
        var recordSize = 30 + typeBytes.length;
        for (var i = 0; i < parameters.length; i++) {
            var value = encodeValue(parameters[i]);
            encoded.push(value);
            recordSize += 14 + value.length;
        }
        requireCondition(recordSize <= MAX_BYTES, "size_limit_exceeded", "node record exceeds the graph size limit");
        var writer = new Writer(recordSize);
        writer.u16(1); writer.u16(1); writer.u32(recordSize);
        writer.raw(hexToBytes(node.id));
        writer.u16(typeBytes.length); writer.raw(typeBytes);
        writer.u16(node.schemaVersion); writer.u16(parameters.length);
        for (var p = 0; p < parameters.length; p++) {
            writer.u64(parameters[p].key);
            writer.u16(parameters[p].type);
            writer.u32(encoded[p].length);
            writer.raw(encoded[p]);
        }
        return writer.finish();
    }

    function makeEdgeRecord(edge) {
        var writer = new Writer(72);
        writer.u16(2); writer.u16(1); writer.u32(72);
        writer.raw(hexToBytes(edge.id));
        writer.raw(hexToBytes(edge.sourceNode));
        writer.u64(edge.sourcePort);
        writer.raw(hexToBytes(edge.destinationNode));
        writer.u64(edge.destinationPort);
        return writer.finish();
    }

    function normalizeGraphForWrite(graph) {
        requireCondition(graph && Object.prototype.toString.call(graph.nodes) === "[object Array]" &&
                         Object.prototype.toString.call(graph.edges) === "[object Array]",
                         "invalid_graph", "graph must contain node and edge arrays");
        var nodes = [];
        var edges = [];
        for (var i = 0; i < graph.nodes.length; i++) {
            var node = graph.nodes[i];
            requireCondition(node && Object.prototype.toString.call(node.parameters) === "[object Array]",
                             "invalid_graph", "node parameters must be an array");
            var parameters = [];
            for (var p = 0; p < node.parameters.length; p++) {
                var parameter = node.parameters[p];
                parameters.push({ key: decimalNormalize(parameter.key), type: parameter.type, value: parameter.value });
            }
            nodes.push({ id: normalizeUuid(node.id, false), type: node.type,
                         schemaVersion: node.schemaVersion, parameters: parameters });
        }
        for (var e = 0; e < graph.edges.length; e++) {
            var edge = graph.edges[e];
            edges.push({ id: normalizeUuid(edge.id, false),
                         sourceNode: normalizeUuid(edge.sourceNode, false),
                         sourcePort: decimalNormalize(edge.sourcePort),
                         destinationNode: normalizeUuid(edge.destinationNode, false),
                         destinationPort: decimalNormalize(edge.destinationPort) });
        }
        var optional = graph.optionalRecords === undefined ? [] : graph.optionalRecords.slice();
        return { version: graph.version, nodes: nodes, edges: edges, optionalRecords: optional };
    }

    function parse(bytes) {
        bytes = bytesOf(bytes);
        requireCondition(bytes.length <= MAX_BYTES, "size_limit_exceeded", "graph exceeds the 64 MiB limit");
        requireCondition(bytes.length >= HEADER_SIZE, "invalid_header", "graph header is truncated");
        var header = new Reader(bytes.subarray(0, HEADER_SIZE));
        for (var i = 0; i < MAGIC.length; i++) requireCondition(header.u8() === MAGIC[i], "invalid_header", "graph magic is invalid");
        var version = header.u16();
        var headerSize = header.u16();
        var payloadSize = header.u32();
        var nodeCount = header.u32();
        var edgeCount = header.u32();
        var checksum = header.u32();
        var flags = header.u32();
        requireCondition(version === FORMAT_VERSION, "unsupported_format_version", "graph format version is unsupported");
        requireCondition(headerSize === HEADER_SIZE, "invalid_header", "schema-1 header must be 32 bytes");
        requireCondition(flags === 0, "unsupported_flags", "schema-1 graph flags must be zero");
        requireCondition(nodeCount <= MAX_NODES && edgeCount <= MAX_EDGES,
                         "size_limit_exceeded", "graph node or edge count exceeds its limit");
        requireCondition(payloadSize === bytes.length - HEADER_SIZE, "length_mismatch", "graph payload size does not match the header");
        var payload = bytes.subarray(HEADER_SIZE);
        requireCondition(crc32(payload) === checksum, "checksum_mismatch", "graph payload CRC-32 does not match");
        var records = new Reader(payload);
        var graph = { version: FORMAT_VERSION, nodes: [], edges: [], optionalRecords: [] };
        while (records.remaining()) {
            requireCondition(records.remaining() >= 8, "malformed_record", "record header is truncated");
            var start = records.offset;
            var kind = records.u16();
            var recordVersion = records.u16();
            var recordSize = records.u32();
            requireCondition(recordSize >= 8 && recordSize - 8 <= records.remaining(),
                             "malformed_record", "record size is invalid");
            var body = records.take(recordSize - 8);
            var bodyReader = new Reader(body);
            if (kind === 1 || kind === 2) {
                requireCondition(recordVersion === 1, "unsupported_record_version", "schema-1 record version is unsupported");
                if (kind === 1) {
                    requireCondition(graph.nodes.length < nodeCount, "malformed_record", "more nodes than declared in header");
                    graph.nodes.push(parseNode(bodyReader));
                } else {
                    requireCondition(graph.edges.length < edgeCount, "malformed_record", "more edges than declared in header");
                    graph.edges.push(parseEdge(bodyReader));
                }
            } else if (kind & 0x8000) {
                requireCondition(graph.optionalRecords.length < MAX_OPTIONAL_RECORDS,
                                 "size_limit_exceeded", "optional record count exceeds the schema-1 limit");
                graph.optionalRecords.push(bytesToHex(payload.subarray(start, records.offset)));
            } else throw error("unsupported_record", "graph contains an unknown required record");
        }
        requireCondition(graph.nodes.length === nodeCount && graph.edges.length === edgeCount,
                         "length_mismatch", "record counts do not match the graph header");
        validateGraphShape(graph);
        return graph;
    }

    function serialize(graph) {
        graph = normalizeGraphForWrite(graph);
        validateGraphShape(graph);
        var nodes = graph.nodes.slice().sort(function (left, right) { return left.id < right.id ? -1 : left.id > right.id ? 1 : 0; });
        var edges = graph.edges.slice().sort(function (left, right) { return left.id < right.id ? -1 : left.id > right.id ? 1 : 0; });
        var records = new Writer(1024);
        for (var n = 0; n < nodes.length; n++) records.raw(makeNodeRecord(nodes[n]));
        for (var e = 0; e < edges.length; e++) records.raw(makeEdgeRecord(edges[e]));
        var optional = graph.optionalRecords;
        for (var o = 0; o < optional.length; o++) {
            var raw = hexToBytes(optional[o]);
            requireCondition(raw.length >= 8, "malformed_record", "optional record is truncated");
            var optionalReader = new Reader(raw);
            var kind = optionalReader.u16(); optionalReader.u16();
            var declaredSize = optionalReader.u32();
            requireCondition((kind & 0x8000) !== 0 && declaredSize === raw.length,
                             "invalid_graph", "preserved optional record is malformed");
            records.raw(raw);
        }
        var payload = records.finish();
        requireCondition(payload.length + HEADER_SIZE <= MAX_BYTES, "size_limit_exceeded", "graph exceeds the 64 MiB limit");
        var output = new Writer(payload.length + HEADER_SIZE);
        output.raw(MAGIC);
        output.u16(FORMAT_VERSION); output.u16(HEADER_SIZE);
        output.u32(payload.length); output.u32(nodes.length); output.u32(edges.length);
        output.u32(crc32(payload)); output.u32(0);
        output.raw(payload);
        return output.finish();
    }

    function fromHex(hex) { return parse(hexToBytes(hex)); }
    function toHex(graph) { return bytesToHex(serialize(graph)); }

    return {
        formatVersion: FORMAT_VERSION,
        headerSize: HEADER_SIZE,
        limits: { bytes: MAX_BYTES, nodes: MAX_NODES, edges: MAX_EDGES,
                  optionalRecords: MAX_OPTIONAL_RECORDS,
                  typeKeyBytes: MAX_TYPE_KEY_BYTES, parameters: MAX_PARAMETERS },
        crc32: crc32,
        parse: parse,
        fromHex: fromHex,
        serialize: serialize,
        toHex: toHex,
        bytesToHex: bytesToHex,
        hexToBytes: hexToBytes
    };
}));
