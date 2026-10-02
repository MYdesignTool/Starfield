#include "Camera.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
int checks{}, failures{}, acquisitions{}, releases{};
AEGP_PFInterfaceSuite1 pf{};
AEGP_LayerSuite9 layers{};
A_Matrix4 camera{}, layer{};
AEGP_LayerFlags flags{};
bool has_camera = true, valid_geometry = true;
A_Err matrix_error{};
void check(bool v, const char* name) { ++checks; if (!v) { ++failures; std::printf("FAILED: %s\n", name); } }
void identity(A_Matrix4& m) { m = {}; for (int i = 0; i < 4; ++i) m.mat[i][i] = 1; }
A_Err acquire(const char* name, int32, const void** out) {
    if (!std::strcmp(name, kAEGPPFInterfaceSuite)) *out = &pf;
    else if (!std::strcmp(name, kAEGPLayerSuite)) *out = &layers;
    else return 1;
    ++acquisitions; return 0;
}
A_Err release(const char*, int32) { ++releases; return 0; }
}

int run_camera_capture_tests() {
    SPBasicSuite basic{}; basic.AcquireSuite = acquire; basic.ReleaseSuite = release;
    pf.AEGP_ConvertEffectToCompTime = [](PF_ProgPtr, A_long value, A_u_long scale, A_Time* time)->A_Err {
        *time = {value, scale}; return 0;
    };
    pf.AEGP_GetEffectCamera = [](PF_ProgPtr, const A_Time*, AEGP_LayerH* out)->A_Err {
        *out = has_camera ? reinterpret_cast<AEGP_LayerH>(1) : nullptr; return 0;
    };
    pf.AEGP_GetEffectCameraMatrix = [](PF_ProgPtr, const A_Time*, A_Matrix4* out, A_FpLong* zoom, A_short* w, A_short* h)->A_Err {
        *out = camera; *zoom = valid_geometry ? 100 : 0; *w = *h = valid_geometry ? 100 : 0; return matrix_error;
    };
    pf.AEGP_GetEffectLayer = [](PF_ProgPtr, AEGP_LayerH* out)->A_Err { *out = reinterpret_cast<AEGP_LayerH>(2); return 0; };
    layers.AEGP_GetLayerToWorldXform = [](AEGP_LayerH, const A_Time*, A_Matrix4* out)->A_Err { *out = layer; return 0; };
    layers.AEGP_GetLayerFlags = [](AEGP_LayerH, AEGP_LayerFlags* out)->A_Err { *out = flags; return 0; };
    PF_InData data{}; data.pica_basicP = &basic; data.time_scale = 24;
    identity(camera); identity(layer); camera.mat[3][0] = camera.mat[3][1] = 50; camera.mat[3][2] = -100;
    SfCoreRenderRequest request{};
    check(starfield::adapter::capture_camera(&data, request) == 0 && request.camera_enabled == 1, "capture camera numbers");
    check(request.layer_to_view[12] == -50 && request.layer_to_view[14] == 100, "SDK row camera inverse maps layer into view");
    check(request.image_to_layer[0] == 1 && request.image_to_layer[8] == 1, "2D layer plane stays unchanged");
    layer.mat[0][0] = 2; layer.mat[1][1] = 3; layer.mat[3][0] = 10;
    check(starfield::adapter::capture_camera(&data, request) == 0, "capture transformed 2D layer");
    check(request.image_to_layer[0] == .5 && std::abs(request.image_to_layer[4] - 1.0/3) < 1e-12 && request.image_to_layer[2] == -5, "undo later 2D affine transform");
    flags = AEGP_LayerFlag_LAYER_IS_3D;
    check(starfield::adapter::capture_camera(&data, request) == 0 &&
        std::abs(request.image_to_layer[2] / request.image_to_layer[8] + 5) < 1e-12,
        "3D layer inverse homography avoids double projection");
    camera = {};
    check(starfield::adapter::capture_camera(&data, request) != 0 && request.camera_enabled == 0, "singular explicit camera rejects without stale enable");
    has_camera = false; valid_geometry = false;
    check(starfield::adapter::capture_camera(&data, request) == 0 && request.camera_enabled == 0, "absent default geometry retains flat output");
    identity(camera); camera.mat[3][0] = camera.mat[3][1] = 50; camera.mat[3][2] = -100; valid_geometry = true;
    check(starfield::adapter::capture_camera(&data, request) == 0 && request.camera_enabled == 1, "host default view geometry projects without active camera layer");
    has_camera = true; matrix_error = 123;
    check(starfield::adapter::capture_camera(&data, request) == 123 && request.camera_enabled == 0, "camera API failure propagates");
    check(acquisitions == releases, "camera suites released on success and error");
    check(starfield::adapter::capture_camera(nullptr, request) != 0, "invalid host request rejects");
    std::printf("Camera capture: %d checks, %d failures\n", checks, failures);
    return failures;
}
