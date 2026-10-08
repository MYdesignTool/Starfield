#pragma once

#include "AEConfig.h"
#include "AE_Effect.h"

#include "ParticleLayout.hpp"
#include "TransformLayout.hpp"
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

enum class Kind : A_long { emitter=0, particle=1, force=3, transform=4 }; // 2 is not a node kind.

[[nodiscard]] constexpr A_long base_parameter_count(Kind kind) noexcept {
    switch (kind) {
        case Kind::emitter: return 34;
        case Kind::particle: return particle_layout::last;
        case Kind::force: return 11+particle_layout::curve_span-1;
        case Kind::transform: return transform_layout::last;
    }
    return 0;
}
[[nodiscard]] constexpr A_long binding_field_count(Kind kind) noexcept {
    return kind==Kind::transform?transform_layout::compensation_last:
        kind==Kind::particle?particle_layout::cloud_enabled:base_parameter_count(kind);
}
[[nodiscard]] constexpr bool authored_parameter(Kind kind,A_long index) noexcept {
    return index>0 && (index<=base_parameter_count(kind) || (kind==Kind::particle &&
        (index==particle_layout::transfer || (index>=particle_layout::texture_front && index<=particle_layout::texture_perspective) ||
         (index>=particle_layout::cloud_circles && index<=particle_layout::cloud_density) || index==particle_layout::cloud_enabled)));
}

[[nodiscard]] constexpr A_long layout_x_index(Kind kind) noexcept { return base_parameter_count(kind) + 1; }
[[nodiscard]] constexpr A_long layout_y_index(Kind kind) noexcept { return base_parameter_count(kind) + 2; }
[[nodiscard]] constexpr A_long connection_count_index(Kind kind) noexcept { return base_parameter_count(kind) + 3; }
[[nodiscard]] constexpr A_long connection_first_index(Kind kind) noexcept { return base_parameter_count(kind) + 4; }
[[nodiscard]] constexpr A_long uuid_first_index(Kind kind) noexcept {
    return connection_first_index(kind) + kMaxOutgoingEdges * kConnectionRecordChunks;
}
[[nodiscard]] constexpr A_long sync_guard_index(Kind kind) noexcept { return uuid_first_index(kind) + 8; }
[[nodiscard]] constexpr A_long last_parameter_index(Kind kind) noexcept {
    return kind==Kind::particle?particle_layout::cloud_enabled:sync_guard_index(kind);
}
static_assert(sync_guard_index(Kind::particle)+1==particle_layout::transfer);
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
    kTransformInheritId=1401,kTransformAnchorXYId=1402,kTransformAnchorZId=1403,
    kTransformPositionXId=1404,kTransformPositionYId=1405,kTransformPositionZId=1406,
    kTransformRotationXId=1407,kTransformRotationYId=1408,kTransformRotationZId=1409,
    kTransformScaleXId=1410,kTransformScaleYId=1411,kTransformScaleZId=1412,
    kTransformParticlesScaleId=1413,kTransformParticlesOpacityId=1414,
    kAuxiliarySourceId = 136,
    kLifeRandomId = 212, kParticleShapeId = 213, kSizeYId = 214,
    kFeatherId = 216, kUpAxisId = 217, kOrientToId = 218,
    kParticleAngleXId = 219, kParticleAngleYId = 220, kParticleAngleZId = 221,
    kParticleAngleRandomId = 222, kRotationSpeedXId = 223, kRotationSpeedYId = 224,
    kRotationSpeedZId = 225, kRotationSpeedRandomId = 226, kLimitTo2DId = 227,
    kRandomLimitId=228,kLimitAngleId=229,kAnchorXId=230,kAnchorYId=231,kParticleTransferId=232,
    kTextureFrontId=233,kTextureBackId=234,kTextureTimeId=235,kTextureColorId=236,
    kTextureRatioId=237,kTexturePerspectiveId=238,kTextureTopicId=2920,kTextureEndId=2921,
    kCloudCirclesId=239,kCloudAspectId=240,kCloudDensityId=241,kCloudEnabledId=242,
    kCloudTopicId=2922,kCloudEndId=2923,
    kRotationCurveCountId=960,kRotationCurveAgeFirstId=970,kRotationCurveValueFirstId=980,
    kEmitterOrientXId=137,kEmitterOrientYId=138,kEmitterOrientZId=139,
    kParticlePropertiesId=2910, kParticlePropertiesEndId=2911,
    kParticleOverLifeId=2912, kParticleOverLifeEndId=2913,
    kParticleRotationId=2914, kParticleRotationEndId=2915,
    kParticleColorModeId = 211, kColorGradientCountId = 930,
    kColorGradientPositionFirstId = 940, kColorGradientColorFirstId = 950,
    kEmitterTypeId = 101, kBirthRateId = 102,
    kSeedId = 103, kEmitterParticleSizeId = 104,
    kVelocityXId = 107, kVelocityYId = 108, kVelocityZId = 109,
    kDiscSizeId = 110, kSpeedRandomId = 111,
    kEmitterSizeXId = 112, kEmitterSizeYId = 113, kEmitterSizeZId = 114,
    kEmissionSpeedId = 115, kEmissionSpeedRandomId = 116,
    kEmissionAngleXId = 133, kEmissionAngleYId = 134, kEmissionAngleZId = 135,
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
    kOpacityCurveCountId = 800, kOpacityCurveAgeFirstId = 810, kOpacityCurveValueFirstId = 820,
    kSizeCurveInterpolationId=3610, kOpacityCurveInterpolationId=3611,
    kRotationCurveInterpolationId=3612, kColorGradientInterpolationId=3613,
    kWindSpinCurveInterpolationId=3614
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
    return bank == 's' ? disk_ids::kSizeCurveCountId : bank == 'o' ? disk_ids::kOpacityCurveCountId : bank == 'w' ? disk_ids::kWindSpinCurveCountId : bank=='r'?disk_ids::kRotationCurveCountId:0;
}
[[nodiscard]] constexpr A_long curve_age_id(char bank, A_long point) noexcept {
    if((bank!='s' && bank!='o' && bank!='r' && bank!='w') || point<0 || point>=particle_layout::curve_points)return 0;
    if(point<8)return (bank=='s'?disk_ids::kSizeCurveAgeFirstId:bank=='o'?disk_ids::kOpacityCurveAgeFirstId:bank=='w'?disk_ids::kWindSpinCurveAgeFirstId:bank=='r'?disk_ids::kRotationCurveAgeFirstId:0)+point;
    return (bank=='s'?3000:bank=='o'?3200:bank=='r'?3400:bank=='w'?3700:0)+point;
}
[[nodiscard]] constexpr A_long curve_value_id(char bank, A_long point) noexcept {
    if((bank!='s' && bank!='o' && bank!='r' && bank!='w') || point<0 || point>=particle_layout::curve_points)return 0;
    if(point<8)return (bank=='s'?disk_ids::kSizeCurveValueFirstId:bank=='o'?disk_ids::kOpacityCurveValueFirstId:bank=='w'?disk_ids::kWindSpinCurveValueFirstId:bank=='r'?disk_ids::kRotationCurveValueFirstId:0)+point;
    return (bank=='s'?3100:bank=='o'?3300:bank=='r'?3500:bank=='w'?3800:0)+point;
}
[[nodiscard]] constexpr A_long curve_interpolation_id(char bank) noexcept {
    return bank=='s'?disk_ids::kSizeCurveInterpolationId:bank=='o'?disk_ids::kOpacityCurveInterpolationId:
        bank=='r'?disk_ids::kRotationCurveInterpolationId:bank=='w'?disk_ids::kWindSpinCurveInterpolationId:0;
}

// Check the whole identity allocation, including every generated connection,
// UUID and curve ID. A new collision or out-of-range ID must fail compilation.
[[nodiscard]] constexpr bool disk_ids_are_unique_and_bounded() noexcept {
    using namespace disk_ids;
    constexpr A_long fixed[] = {
        kTransformInheritId,kTransformAnchorXYId,kTransformAnchorZId,
        kTransformPositionXId,kTransformPositionYId,kTransformPositionZId,
        kTransformRotationXId,kTransformRotationYId,kTransformRotationZId,
        kTransformScaleXId,kTransformScaleYId,kTransformScaleZId,
        kTransformParticlesScaleId,kTransformParticlesOpacityId,
        kEmitterTypeId, kBirthRateId, kSeedId, kEmitterParticleSizeId,
        kOriginXYId, kOriginZId, kVelocityXId, kVelocityYId, kVelocityZId, kDiscSizeId, kSpeedRandomId,
        kEmitterSizeXId, kEmitterSizeYId, kEmitterSizeZId, kEmissionSpeedId,
        kEmissionSpeedRandomId, kEmissionAngleXId, kEmissionAngleYId, kEmissionAngleZId,
        kDirectionId, kDirectionSpanId,kEmitterOrientXId,kEmitterOrientYId,kEmitterOrientZId,
        kLifetimeId, kSizeId, kSizeOverLifeId, kOpacityId, kOpacityOverLifeId,
        kColorStartId, kColorEndId, kSizeRandomId, kOpacityRandomId,
        kAuxiliarySourceId,kLifeRandomId,kParticleShapeId,kSizeYId,kFeatherId,kUpAxisId,kOrientToId,
        kParticleAngleXId,kParticleAngleYId,kParticleAngleZId,kParticleAngleRandomId,kRotationSpeedXId,kRotationSpeedYId,kRotationSpeedZId,kRotationSpeedRandomId,kLimitTo2DId,
        kParticleColorModeId, kColorGradientCountId,
        kRandomLimitId,kLimitAngleId,kAnchorXId,kAnchorYId,kParticleTransferId,kRotationCurveCountId,
        kTextureFrontId,kTextureBackId,kTextureTimeId,kTextureColorId,kTextureRatioId,kTexturePerspectiveId,
        kTextureTopicId,kTextureEndId,
        kCloudCirclesId,kCloudAspectId,kCloudDensityId,kCloudEnabledId,kCloudTopicId,kCloudEndId,
        kParticlePropertiesId, kParticlePropertiesEndId, kParticleOverLifeId,
        kParticleOverLifeEndId, kParticleRotationId, kParticleRotationEndId,
        kGravityId, kDragId, kForceGravityId, kAirDensityId, kGravityRandomId,
        kWindXId, kWindYId, kWindZId, kSpinId, kSpinFrequencyId, kSpinResistId, kSpinDelayId,
        kWindSpinCurveCountId,
        kLayoutXId, kLayoutYId, kConnectionCountId, kSyncGuardId,
        kSizeCurveCountId, kOpacityCurveCountId, kSizeCurveInterpolationId, kOpacityCurveInterpolationId,
        kRotationCurveInterpolationId, kColorGradientInterpolationId, kWindSpinCurveInterpolationId
    };
    std::array<A_long, std::size(fixed) + kMaxOutgoingEdges * kConnectionRecordChunks + 24 + 8*particle_layout::curve_points> ids{};
    std::size_t count = 0;
    for (auto id : fixed) ids[count++] = id;
    for (A_long slot = 0; slot < kMaxOutgoingEdges; ++slot) {
        for (A_long chunk = 0; chunk < kConnectionUuidChunks; ++chunk) {
            ids[count++] = connection_uuid_id(slot, chunk);
            ids[count++] = connection_edge_uuid_id(slot, chunk);
        }
    }
    for (A_long point = 0; point < 8; ++point) {
        ids[count++]=kColorGradientPositionFirstId+point;
        ids[count++]=kColorGradientColorFirstId+point;
        ids[count++] = uuid_id(point);
    }
    for(A_long point=0;point<particle_layout::curve_points;++point) {
        for (char bank : {'s', 'o', 'w', 'r'}) {
            ids[count++] = curve_age_id(bank, point);
            ids[count++] = curve_value_id(bank, point);
        }
    }
    std::array<bool,10000> seen{};
    for(auto id:ids) {
        if(!valid_disk_id(id) || seen[id])return false;
        seen[id]=true;
    }
    return count == ids.size();
}
static_assert(disk_ids_are_unique_and_bounded(), "Node parameter disk IDs must be unique and within 1..9999");

} // namespace starfield::adapter::native_nodes
