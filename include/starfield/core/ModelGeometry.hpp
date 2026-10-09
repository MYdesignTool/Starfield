#pragma once
#include "starfield/core/Error.hpp"
#include "starfield/core/Settings.hpp"
#include <cstddef>
#include <string_view>

namespace starfield::core {
class Cancellation;
struct ModelParseLimits {
    std::size_t source_bytes{8*1024*1024};
    std::size_t logical_line_bytes{64*1024};
    std::uint32_t positions{kMaxModelVertices};
    std::uint32_t attributes{kMaxModelVertices};
    std::uint32_t triangles{kMaxModelTriangles};
    std::uint32_t face_corners{256};
    std::uint64_t triangulation_work{16'000'000};
};
struct ModelParseLocation { std::size_t line{}; };

// Bounds cover referenced geometry; unreferenced positions are still validated.
[[nodiscard]] Result<ModelBounds> validate_model_geometry(const ModelGeometry&,const Cancellation&) noexcept;
[[nodiscard]] Result<ModelGeometry> make_unit_cube() noexcept;
// Never performs filesystem access or executes OBJ statements. A failed parse
// returns no partial mesh. Optional weights/UVW and corner indices are retained.
[[nodiscard]] Result<ModelGeometry> parse_model_obj(std::string_view,const Cancellation&,
    ModelParseLimits = {},ModelParseLocation* = nullptr) noexcept;
} // namespace starfield::core
