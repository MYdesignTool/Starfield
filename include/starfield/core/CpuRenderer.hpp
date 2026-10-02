#pragma once

#include "starfield/core/Render.hpp"

#include <cstdint>

namespace starfield::core {

struct RenderLimits {
    // Optional upper bound on clipped sprite pixels, summed over particles.
    // Zero disables the artificial cutoff; graph, population and buffer bounds
    // still apply and every scan row checks cancellation. Explicit finite limits
    // fail with work_limit_exceeded instead of truncating the output.
    std::uint64_t max_sprite_pixel_ops{0};
};

// Deterministic CPU reference backend for the current particle slice: particles
// are rasterized as soft-edged discs using evaluated per-particle RGB/opacity/size
// and composited over the optional source with premultiplied "over" (ADR 0005).
// Depth and motion blur remain future work.
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
