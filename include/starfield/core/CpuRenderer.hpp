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

// Deterministic CPU reference backend for the current particle slice: particles
// are rasterized as white soft-edged discs and composited over the optional source
// with premultiplied "over" (ADR 0005). Shape sampling and velocity variation are
// in the simulation; this renderer still ignores depth, color, and age curves.
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
