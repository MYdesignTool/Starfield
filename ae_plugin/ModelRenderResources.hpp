#pragma once
#include "ModelMirrorParameter.hpp"
#include "MotionBlur.hpp"

namespace starfield::adapter {
struct ModelAbiStorage {
    std::vector<SfModelPosition> positions;
    std::vector<SfModelAttribute> texture_coordinates,normals;
    std::vector<SfModelTriangle> triangles;
};
struct ModelRenderResources {
    std::vector<ModelAbiStorage> buffers;
    std::vector<SfModelSource> sources;
};
// PF checkout/checkin only; no sibling effects, AEGP resource queries or file IO.
[[nodiscard]] PF_Err prepare_model_render_resources(PF_InData*,PF_OutData*,PF_PreRenderExtra*,
    const core::Graph&,const MotionExposure&,A_long height,double par,const core::Cancellation&,ModelRenderResources&) noexcept;
}
