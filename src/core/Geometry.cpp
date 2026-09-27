#include "starfield/core/Geometry.hpp"

#include <algorithm>
#include <cmath>

namespace starfield::core {
namespace {

// Above this magnitude a raw component cannot be a pixel position in any real
// comp, so it is treated as a fixed-point (percent x 65536) delivery.
constexpr double kFixedPointThreshold = 1.0e5;
constexpr double kFixedPointScale = 65536.0;

// A value this far outside the layer cannot be a pixel position either; it is a
// legacy percentage of the layer extent (50 meaning halfway).
constexpr double kPlausiblePixelMargin = 4.0;

} // namespace

double host_point_component_to_layer_pixels(double raw_component, double layer_extent) noexcept {
    if (!std::isfinite(raw_component)) {
        return 0.0;
    }

    const double extent = (std::isfinite(layer_extent) && layer_extent > 0.0) ? layer_extent : 1.0;
    const double magnitude = std::abs(raw_component);

    if (magnitude > kFixedPointThreshold) {
        return raw_component / kFixedPointScale;
    }
    if (magnitude > kPlausiblePixelMargin * extent) {
        return raw_component / 100.0 * extent;
    }
    return raw_component;
}

Vec3 layer_point_to_world(double x_pixels, double y_pixels, double z_pixels, const LayerUnits& units) noexcept {
    const double layer_width = units.layer_width > 0.0 ? units.layer_width : 1.0;
    const double layer_height = units.layer_height > 0.0 ? units.layer_height : 1.0;
    const double pixel_aspect = units.pixel_aspect_ratio > 0.0 ? units.pixel_aspect_ratio : 1.0;

    // One world unit is one layer height. Horizontally that is layer_height/pixel_aspect
    // layer pixels because the host's horizontal pixels are stretched by the aspect
    // ratio, which is why the horizontal term scales by (width * aspect / height).
    const double horizontal_units_per_full_width = layer_width * pixel_aspect / layer_height;

    Vec3 world;
    world.x = (x_pixels / layer_width - 0.5) * horizontal_units_per_full_width;
    world.y = 0.5 - y_pixels / layer_height;
    world.z = z_pixels / layer_height - 0.5;
    return world;
}

} // namespace starfield::core
