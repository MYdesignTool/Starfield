# Sequence data format, schema 1

This is the plug-in-owned graph/preset state. Animated AE controls remain normal AE parameters and are not duplicated in sequence data.

## Header

All integers are little-endian. Header size is exactly 32 bytes; reject a different size unless a future reader explicitly supports it.

| Offset | Size | Field | Schema 1 rule |
|---:|---:|---|---|
| 0 | 8 | Magic | ASCII `SFLDSEQ` followed by NUL |
| 8 | 2 | Format version | `1` |
| 10 | 2 | Header size | `32` |
| 12 | 4 | Payload size | Bytes following this header |
| 16 | 4 | Node count | Maximum `4096` |
| 20 | 4 | Edge count | Maximum `16384` |
| 24 | 4 | Payload CRC-32 | IEEE CRC-32 over payload bytes only |
| 28 | 4 | Flags | Must be zero in schema 1 |

Maximum total blob size is 64 MiB. Reject truncated data, integer overflow, inconsistent counts, bad checksums, invalid UTF-8, unsupported node versions, duplicate IDs, invalid edge endpoints, and trailing unparsed bytes. Parsing must never allocate based only on untrusted lengths; validate each count/length before allocation.

## Payload records

Payload is a sequence of length-delimited records. Each record starts with `kind:u16`, `record_version:u16`, and `record_size:u32` (including the 8-byte record prefix). Unknown kinds with the optional bit set (`kind & 0x8000`) may be skipped; unknown required kinds fail cleanly.

### Node record (`kind = 1`, `record_version = 1`)

Fields after the 8-byte record prefix:

1. `node_id`: 16 bytes, UUID byte order in network order; all-zero is invalid.
2. `type_key_length`: `u16`, 1..128 bytes follow as lowercase ASCII reverse-DNS key.
3. `node_schema_version`: `u16`, non-zero.
4. `parameter_count`: `u16`, at most 512.
5. `parameters`: each entry is `param_key:u64`, `value_type:u16`, `value_size:u32`, then exactly `value_size` bytes. Keys are stable node-type parameter IDs, not AE UI parameter IDs. Duplicate keys are invalid.

Value type IDs are fixed: `1=bool` (one byte, only 0/1), `2=i32`, `3=u32`, `4=f64` (IEEE-754 little-endian, finite only), `5=vec3_f64` (three finite f64 values), `6=utf8` (no NUL terminator), `7=opaque_bytes` (only for a node type that explicitly owns this payload). Unknown required value types fail migration; opaque bytes remain subject to the 64 MiB total limit.

### Edge record (`kind = 2`, `record_version = 1`)

Fields after the 8-byte record prefix:

1. `edge_id`: 16 bytes, UUID byte order in network order; all-zero is invalid.
2. `source_node_id`: 16 bytes.
3. `source_port_key`: `u64`.
4. `destination_node_id`: 16 bytes.
5. `destination_port_key`: `u64`.

Reject duplicate edge IDs, missing endpoint nodes, duplicate input connections where the destination port is single-input, and cycles unless the node schema explicitly declares a delayed-feedback boundary.

## Migration and AE lifecycle

Migrations operate on parsed owned values (`vN -> vN+1`) and never mutate raw input bytes. Unknown future versions return a controlled compatibility error; do not reset user data to defaults silently. The core schema-1 codec rejects unsupported format versions; there is no earlier graph schema to migrate. **Storage decision superseded:** ADR 0008 stores graph bytes in an AE arbitrary-data parameter, not `sequence_data`; the codec contract and schema remain unchanged. G-04 implements host parameter callbacks, legacy-control selection and explicit current-time capture. AE lifecycle behavior remains to be qualified in AE 2023.
