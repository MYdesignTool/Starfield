#pragma once
#include "starfield/core/ModelGeometry.hpp"
#include "starfield/core/Render.hpp"
#include <span>

namespace starfield::core {
// Numeric staging only. No resource/Particle/wire fields are added by this API.
struct ModelSceneLimits {
    std::size_t projected_triangles{131072};
    std::size_t surface_pixels{4194304};
    std::uint64_t sample_visits{64000000};
};
struct ModelProjection {
    FrameSpec frame;
    RenderRequest::Camera camera;
    // Affine row-vector OBJ -> full-resolution layer pixels (+Y downward).
    std::array<double,16> model_to_layer{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
};
struct ModelProjectedVertex {
    double x{},y{},reciprocal_w{1},depth_over_w{};
    Vec3 texture_over_w{};
};
struct ModelProjectedTriangle {
    std::array<ModelProjectedVertex,3> vertices;
    std::uint32_t source_triangle{};
};
struct ModelScene {
    RectI region{};
    std::vector<ModelProjectedTriangle> triangles;
};
struct ModelSurfacePixel {
    double depth{}; // nearest covered sample; meaningful only when coverage>0
    float coverage{};
    std::uint32_t source_triangle{kMissingModelAttribute};
};
struct ModelSurface {
    RectI region{};
    std::vector<ModelSurfacePixel> pixels; // tightly packed region
};
[[nodiscard]] Result<ModelScene> project_model_scene(const ModelGeometry&,const ModelProjection&,
    const Cancellation&,ModelSceneLimits = {}) noexcept;
[[nodiscard]] Result<ModelSurface> rasterize_model_scene(const ModelScene&,const Cancellation&,
    ModelSceneLimits = {}) noexcept;
// The destination is caller-owned premultiplied RGBA. Discard it on failure.
[[nodiscard]] Result<std::size_t> composite_model_surface(const ModelSurface&,Vec3 color,double opacity,
    ParticleTransferMode,RectI destination_region,std::span<float> destination,
    const Cancellation&) noexcept;
}
