// Graph-parameter/registration tests have no camera or animation-history host.
// Exercise real MotionBlur registration/UI code; reject capture if a test
// accidentally enters the separate host-render path. NativeSync covers capture.
#include "Camera.hpp"
#include "EmitterHistory.hpp"

namespace starfield::adapter {
#if !defined(STARFIELD_TEST_GPU)
PF_Err capture_camera(PF_InData*, SfCoreRenderRequest&, const PF_InData*) noexcept {
    return PF_Err_BAD_CALLBACK_PARAM;
}
#endif
PF_Err capture_motion_particles(PF_InData*, PF_OutData*, const core::Graph&,
    A_long, A_long, std::span<const MotionLookAtTargetResources>,std::span<const core::RationalTime>, const core::Cancellation&,
    std::vector<CapturedParticleFrame>&) noexcept {
    return PF_Err_BAD_CALLBACK_PARAM;
}
}
