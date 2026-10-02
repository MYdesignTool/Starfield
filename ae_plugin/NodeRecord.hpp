#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"

#include <array>
#include <iterator>

namespace starfield::adapter::native_nodes {

// Node-module project record v1. Keep this layout shared by the node AEX and
// renderer-side graph compiler. This native record currently supports four
// outgoing edges per node. It is a stricter authoring limit than the portable
// graph's total edge bound; a wider fan-out requires extending this record.
inline constexpr A_long kMaxOutgoingEdges = 4;
inline constexpr A_long kConnectionUuidChunks = 8;
inline constexpr A_long kConnectionRecordChunks = 16; // destination UUID + edge UUID

enum class Kind : A_long { emitter, particle, appearance, force };

[[nodiscard]] constexpr A_long base_parameter_count(Kind kind) noexcept {
    switch (kind) {
        case Kind::emitter: return 30;
        case Kind::particle: return 43;
        case Kind::appearance: return 42;
        case Kind::force: return 27;
    }
    return 0;
}

[[nodiscard]] constexpr A_long layout_x_index(Kind kind) noexcept { return base_parameter_count(kind) + 1; }
[[nodiscard]] constexpr A_long layout_y_index(Kind kind) noexcept { return base_parameter_count(kind) + 2; }
[[nodiscard]] constexpr A_long connection_count_index(Kind kind) noexcept { return base_parameter_count(kind) + 3; }
[[nodiscard]] constexpr A_long connection_first_index(Kind kind) noexcept { return base_parameter_count(kind) + 4; }
[[nodiscard]] constexpr A_long uuid_first_index(Kind kind) noexcept {
    return connection_first_index(kind) + kMaxOutgoingEdges * kConnectionRecordChunks;
}
[[nodiscard]] constexpr A_long sync_guard_index(Kind kind) noexcept { return uuid_first_index(kind) + 8; }
[[nodiscard]] constexpr A_long last_parameter_index(Kind kind) noexcept { return sync_guard_index(kind); }
// PF_OutData::num_params includes parameter 0 (the input layer), while registered
// node controls occupy indices 1..last_parameter_index.
[[nodiscard]] constexpr A_long parameter_count(Kind kind) noexcept { return last_parameter_index(kind) + 1; }

[[nodiscard]] constexpr A_long connection_uuid_index(Kind kind, A_long slot, A_long chunk) noexcept {
    return connection_first_index(kind) + slot * kConnectionRecordChunks + chunk;
}
[[nodiscard]] constexpr A_long connection_edge_uuid_index(Kind kind, A_long slot, A_long chunk) noexcept {
    return connection_uuid_index(kind, slot, kConnectionUuidChunks + chunk);
}

// AE parameter disk IDs must be 1..9999, not FourCCs. Long decimal IDs can
// overflow the generated effect-parameter match name. Keep these explicit
// identities stable independently of stream indices (schema/node-parameters.json).
namespace disk_ids {
enum : A_long {
    kEmitterTypeId = 101, kBirthRateId = 102,
    kSeedId = 103, kEmitterParticleSizeId = 104,
    kVelocityXId = 107, kVelocityYId = 108, kVelocityZId = 109,
    kDiscSizeId = 110, kSpeedRandomId = 111,
    kEmitterSizeXId = 112, kEmitterSizeYId = 113, kEmitterSizeZId = 114,
    kEmissionSpeedId = 115, kEmissionSpeedRandomId = 116,
    kEmissionAngleXId = 117, kEmissionAngleYId = 118, kEmissionAngleZId = 119,
    kDirectionId = 120, kDirectionSpanId = 121,
    kOriginXYId = 123, kOriginZId = 124,
    kEmittingModeId = 125, kEmitChanceId = 126, kEmitLifeStartId = 127, kEmitLifeEndId = 128,
    kInheritVelocityId = 129, kInheritSizeId = 130, kInheritOpacityId = 131, kInheritColorId = 132,
    kLifetimeId = 201, kSizeId = 202,
    kSizeOverLifeId = 203, kOpacityId = 204, kOpacityOverLifeId = 205,
    kColorStartId = 206, kColorEndId = 207, kSizeRandomId = 208,
    kOpacityRandomId = 209,
    kGravityId = 301, kDragId = 302, // reserved development vector/drag controls
    kForceGravityId = 304, kAirDensityId = 305, kGravityRandomId = 306,
    kWindXId = 307, kWindYId = 308, kWindZId = 309,
    kSpinId = 310, kSpinFrequencyId = 311, kSpinResistId = 312, kSpinDelayId = 313,
    kWindSpinCurveCountId = 900, kWindSpinCurveAgeFirstId = 910, kWindSpinCurveValueFirstId = 920,
    kLayoutXId = 400, kLayoutYId = 401, kConnectionCountId = 402,
    kConnectionFirstId = 500, kUuidFirstId = 600, kSyncGuardId = 608,
    kSizeCurveCountId = 700, kSizeCurveAgeFirstId = 710, kSizeCurveValueFirstId = 720,
    kOpacityCurveCountId = 800, kOpacityCurveAgeFirstId = 810, kOpacityCurveValueFirstId = 820
};
} // namespace disk_ids

[[nodiscard]] constexpr bool valid_disk_id(A_long id) noexcept { return id >= 1 && id <= 9999; }
[[nodiscard]] constexpr A_long connection_count_id() noexcept { return disk_ids::kConnectionCountId; }
[[nodiscard]] constexpr A_long layout_x_id() noexcept { return disk_ids::kLayoutXId; }
[[nodiscard]] constexpr A_long layout_y_id() noexcept { return disk_ids::kLayoutYId; }
[[nodiscard]] constexpr A_long connection_uuid_id(A_long slot, A_long chunk) noexcept {
    return slot >= 0 && slot < kMaxOutgoingEdges && chunk >= 0 && chunk < kConnectionUuidChunks
        ? disk_ids::kConnectionFirstId + slot * kConnectionRecordChunks + chunk : 0;
}
[[nodiscard]] constexpr A_long connection_edge_uuid_id(A_long slot, A_long chunk) noexcept {
    return slot >= 0 && slot < kMaxOutgoingEdges && chunk >= 0 && chunk < kConnectionUuidChunks
        ? disk_ids::kConnectionFirstId + slot * kConnectionRecordChunks + kConnectionUuidChunks + chunk : 0;
}
[[nodiscard]] constexpr A_long uuid_id(A_long chunk) noexcept {
    return chunk >= 0 && chunk < 8 ? disk_ids::kUuidFirstId + chunk : 0;
}
[[nodiscard]] constexpr A_long curve_count_id(char bank) noexcept {
    return bank == 's' ? disk_ids::kSizeCurveCountId : bank == 'o' ? disk_ids::kOpacityCurveCountId : bank == 'w' ? disk_ids::kWindSpinCurveCountId : 0;
}
[[nodiscard]] constexpr A_long curve_age_id(char bank, A_long point) noexcept {
    return point >= 0 && point < 8 && (bank == 's' || bank == 'o' || bank == 'w')
        ? (bank == 's' ? disk_ids::kSizeCurveAgeFirstId : bank == 'o' ? disk_ids::kOpacityCurveAgeFirstId : disk_ids::kWindSpinCurveAgeFirstId) + point : 0;
}
[[nodiscard]] constexpr A_long curve_value_id(char bank, A_long point) noexcept {
    return point >= 0 && point < 8 && (bank == 's' || bank == 'o' || bank == 'w')
        ? (bank == 's' ? disk_ids::kSizeCurveValueFirstId : bank == 'o' ? disk_ids::kOpacityCurveValueFirstId : disk_ids::kWindSpinCurveValueFirstId) + point : 0;
}

// Check the whole identity allocation, including every generated connection,
// UUID and curve ID. A new collision or out-of-range ID must fail compilation.
[[nodiscard]] constexpr bool disk_ids_are_unique_and_bounded() noexcept {
    using namespace disk_ids;
    constexpr A_long fixed[] = {
        kEmitterTypeId, kBirthRateId, kSeedId, kEmitterParticleSizeId,
        kOriginXYId, kOriginZId, kVelocityXId, kVelocityYId, kVelocityZId, kDiscSizeId, kSpeedRandomId,
        kEmitterSizeXId, kEmitterSizeYId, kEmitterSizeZId, kEmissionSpeedId,
        kEmissionSpeedRandomId, kEmissionAngleXId, kEmissionAngleYId, kEmissionAngleZId,
        kDirectionId, kDirectionSpanId,
        kLifetimeId, kSizeId, kSizeOverLifeId, kOpacityId, kOpacityOverLifeId,
        kColorStartId, kColorEndId, kSizeRandomId, kOpacityRandomId,
        kGravityId, kDragId, kForceGravityId, kAirDensityId, kGravityRandomId,
        kWindXId, kWindYId, kWindZId, kSpinId, kSpinFrequencyId, kSpinResistId, kSpinDelayId,
        kWindSpinCurveCountId,
        kLayoutXId, kLayoutYId, kConnectionCountId, kSyncGuardId,
        kSizeCurveCountId, kOpacityCurveCountId
    };
    std::array<A_long, std::size(fixed) + kMaxOutgoingEdges * kConnectionRecordChunks + 8 + 48> ids{};
    std::size_t count = 0;
    for (auto id : fixed) ids[count++] = id;
    for (A_long slot = 0; slot < kMaxOutgoingEdges; ++slot) {
        for (A_long chunk = 0; chunk < kConnectionUuidChunks; ++chunk) {
            ids[count++] = connection_uuid_id(slot, chunk);
            ids[count++] = connection_edge_uuid_id(slot, chunk);
        }
    }
    for (A_long point = 0; point < 8; ++point) {
        ids[count++] = uuid_id(point);
        for (char bank : {'s', 'o', 'w'}) {
            ids[count++] = curve_age_id(bank, point);
            ids[count++] = curve_value_id(bank, point);
        }
    }
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (!valid_disk_id(ids[i])) return false;
        for (std::size_t j = 0; j < i; ++j) if (ids[i] == ids[j]) return false;
    }
    return count == ids.size();
}
static_assert(disk_ids_are_unique_and_bounded(), "Node parameter disk IDs must be unique and within 1..9999");

} // namespace starfield::adapter::native_nodes
