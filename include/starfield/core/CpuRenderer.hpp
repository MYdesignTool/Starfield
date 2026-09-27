#pragma once

#include "starfield/core/Render.hpp"

#include <cstdint>

namespace starfield::core {

struct RenderLimits {
    // Upper bound on sprite pixels a single render may touch, summed over particles
    // and counted after clipping to the region of interest. Exceeding the budget
    // fails with work_limit_exceeded instead of blocking the host indefinitely.
    std::uint64_t max_sprite_pixel_ops{1ull << 28}; // 268M sprite pixels
};

// Deterministic CPU reference backend for the M2 vertical slice: the point emitter
// is rasterized as soft-edged discs and composited over the optional source with
// premultiplied "over" (ADR 0005). M2 renders no shape distributions; every
// particle in a frame shares the emitter position and velocity.
class CpuParticleRenderer final : public Renderer {
public:
    CpuParticleRenderer() = default;
    explicit CpuParticleRenderer(RenderLimits limits) noexcept : limits_(limits) {}

    [[nodiscard]] Result<RenderOutput> render(const RenderRequest& request,
                                             const Cancellation& cancellation) const override;

private:
    RenderLimits limits_{};
};

} // namespace starfield::core
