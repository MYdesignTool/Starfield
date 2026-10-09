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
    std::uint64_t max_model_input_triangles{16'000'000};
    std::uint64_t max_model_sample_visits{512'000'000};
};

// Deterministic CPU reference backend: analytic Circle/Rectangle/Cloud and
// bilinear Texture sprites and solid Model triangles, premultiplied transfer modes, camera depth ordering
// and shared Transform bases. The adapter invokes this for each shutter sample.
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
