#pragma once

#include "starfield/core/Settings.hpp"

namespace starfield::core {

// Layer reference used when converting host-space positions into world space.
struct LayerUnits {
    double layer_width{1.0};          // full-resolution layer width in layer pixels
    double layer_height{1.0};         // full-resolution layer height in layer pixels
    double pixel_aspect_ratio{1.0};   // host pixel width/height ratio
};

// Converts a host point control into world space.
//
// AE point controls deliver *absolute pixels in destination-layer space* with the
// origin at the layer's top-left, x growing right and y growing down (see
// docs/parameter-mapping.md). The canonical world space instead has its origin at
// the layer centre with +X right, +Y up, and one unit equal to one layer height
// (ADR 0003), so the horizontal axis also folds in the pixel aspect ratio.
[[nodiscard]] Vec3 layer_point_to_world(double x_pixels, double y_pixels, double z_pixels,
                                       const LayerUnits& units) noexcept;

// Validates one already-normalized point component from the host adapter. The adapter
// removes AE's preview scaling first; for AE 2023.5 Build 52, observed values are pixel
// coordinates, including valid off-layer positions. Non-finite values become zero.
[[nodiscard]] double host_point_component_to_layer_pixels(double pixel_component) noexcept;

} // namespace starfield::core
