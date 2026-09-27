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

// Interprets one raw point component that a host delivered, tolerating the three
// unit conventions seen in AE documentation and samples:
//   * layer pixels (documented behaviour, e.g. 960 on a 1920-wide layer),
//   * a fixed-point scaled value (percent x 65536, i.e. far above any pixel value),
//   * a legacy percentage (a small value that cannot be a plausible pixel position).
// The result is layer pixels. The branch that fired is reported by the diagnostics
// readout, so the actual host behaviour can be recorded instead of guessed.
[[nodiscard]] double host_point_component_to_layer_pixels(double raw_component, double layer_extent) noexcept;

} // namespace starfield::core
