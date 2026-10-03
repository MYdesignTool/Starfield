#pragma once
#include "starfield/core/Render.hpp"
#include "starfield/core/PluginApi.h"

namespace starfield::core {
// Portable scene preparation. The adapter owns GPU allocation and dispatch.
struct SpriteScene {
    RectI region;
    std::uint32_t tiles_x{}, tiles_y{};
    std::vector<SfGpuSprite> sprites;
    std::vector<std::uint32_t> offsets, indices;
};
[[nodiscard]] Result<SpriteScene> prepare_sprite_scene(const RenderRequest&, const Cancellation&);
}
