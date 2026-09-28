#include "starfield/core/Geometry.hpp"

#include <cmath>

namespace starfield::core {

double host_point_component_to_layer_pixels(double pixel_component) noexcept {
    if (!std::isfinite(pixel_component)) {
        return 0.0;
    }
    return pixel_component;
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
