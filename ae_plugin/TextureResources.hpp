#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "MotionBlur.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/ParticleTexture.hpp"
#include <vector>

namespace starfield::adapter {
struct TextureCheckout {
    core::TextureFrameRequest frame;
    A_long id{},slot{};
    PF_CheckoutResult result{};
};
struct TexturePreparation {
    std::vector<core::TextureSource> sources;
    std::vector<SfTextureSource> abi_sources;
    std::vector<TextureCheckout> checkouts;
};
struct TextureStaging {
    std::vector<std::vector<float>> pixels;
    std::vector<SfTextureFrame> frames;
};
// Only numeric metadata survives pre-render; all suite refs are callback-local.
// A scoped work graph borrows the complete authored layout to keep renderer
// checkout slots stable. Staging indices still address compact owned metadata.
[[nodiscard]] PF_Err prepare_texture_resources(PF_InData*,PF_OutData*,PF_PreRenderExtra*,
    const core::Graph&,const MotionExposure&,A_long height,double par,
    const core::Cancellation&,TexturePreparation&,const core::Graph* resource_layout=nullptr) noexcept;
// Every successful pixel checkout is checked in, including failed copies/cancel.
[[nodiscard]] PF_Err stage_texture_resources(PF_InData*,PF_OutData*,PF_SmartRenderExtra*,
    const TexturePreparation&,HostBitDepth,const core::Cancellation&,TextureStaging&) noexcept;
}
