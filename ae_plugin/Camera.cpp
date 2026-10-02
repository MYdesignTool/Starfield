#include "Camera.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <algorithm>
#include <cmath>

namespace starfield::adapter {
namespace {
template <int N> bool inverse(const double* input, double* output) noexcept {
    double a[N][N * 2]{};
    for (int r = 0; r < N; ++r) for (int c = 0; c < N; ++c) {
        a[r][c] = input[r * N + c]; a[r][c + N] = r == c ? 1.0 : 0.0;
    }
    for (int c = 0; c < N; ++c) {
        int pivot = c;
        for (int r = c + 1; r < N; ++r) if (std::abs(a[r][c]) > std::abs(a[pivot][c])) pivot = r;
        if (!std::isfinite(a[pivot][c]) || std::abs(a[pivot][c]) < 1e-12) return false;
        for (int j = 0; j < N * 2; ++j) std::swap(a[c][j], a[pivot][j]);
        const double scale = a[c][c];
        for (int j = 0; j < N * 2; ++j) a[c][j] /= scale;
        for (int r = 0; r < N; ++r) {
            if (r == c) continue;
            const double factor = a[r][c];
            for (int j = 0; j < N * 2; ++j) a[r][j] -= factor * a[c][j];
        }
    }
    for (int r = 0; r < N; ++r) for (int c = 0; c < N; ++c) {
        output[r * N + c] = a[r][c + N];
        if (!std::isfinite(output[r * N + c])) return false;
    }
    return true;
}
struct Suites {
    SPBasicSuite* basic;
    const AEGP_PFInterfaceSuite1* pf{};
    const AEGP_LayerSuite9* layer{};
    ~Suites() {
        if (layer) basic->ReleaseSuite(kAEGPLayerSuite, kAEGPLayerSuiteVersion9);
        if (pf) basic->ReleaseSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    }
};
}
PF_Err capture_camera(PF_InData* data, SfCoreRenderRequest& request) noexcept {
    request.camera_enabled = 0;
    if (!data || !data->pica_basicP || !data->pica_basicP->AcquireSuite ||
        !data->pica_basicP->ReleaseSuite) return PF_Err_BAD_CALLBACK_PARAM;
    Suites suites{data->pica_basicP};
    A_Err error = suites.basic->AcquireSuite(kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1,
                                             reinterpret_cast<const void**>(&suites.pf));
    if (!error) error = suites.basic->AcquireSuite(kAEGPLayerSuite, kAEGPLayerSuiteVersion9,
                                                  reinterpret_cast<const void**>(&suites.layer));
    if (error) return static_cast<PF_Err>(error);
    A_Time time{};
    A_Matrix4 camera{}, layer{}, view{};
    A_FpLong zoom{};
    A_short width{}, height{};
    AEGP_LayerH effect_layer{}, active_camera{};
    AEGP_LayerFlags flags{};
    error = suites.pf->AEGP_ConvertEffectToCompTime(data->effect_ref, data->current_time, data->time_scale, &time);
    if (!error) error = suites.pf->AEGP_GetEffectCamera(data->effect_ref, &time, &active_camera);
    if (error) return static_cast<PF_Err>(error);
    if (!error) error = suites.pf->AEGP_GetEffectCameraMatrix(data->effect_ref, &time, &camera, &zoom, &width, &height);
    // Some host views supply no default camera geometry. Preserve the ordinary
    // layer-space output in that case; an invalid explicit camera remains an error.
    if (!active_camera && (error || !(zoom > 0) || width <= 0 || height <= 0)) return PF_Err_NONE;
    if (!error) error = suites.pf->AEGP_GetEffectLayer(data->effect_ref, &effect_layer);
    if (!error) error = suites.layer->AEGP_GetLayerToWorldXform(effect_layer, &time, &layer);
    if (!error) error = suites.layer->AEGP_GetLayerFlags(effect_layer, &flags);
    if (error) return static_cast<PF_Err>(error);
    if (!(zoom > 0) || !std::isfinite(zoom) || width <= 0 || height <= 0 ||
        !inverse<4>(&camera.mat[0][0], &view.mat[0][0])) return PF_Err_BAD_CALLBACK_PARAM;
    // Row-vector SDK matrices: layer-to-view = layer-to-world * world-to-view.
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) {
        double value = 0;
        for (int k = 0; k < 4; ++k) value += layer.mat[r][k] * view.mat[k][c];
        request.layer_to_view[r * 4 + c] = value;
    }
    request.focal_x = request.focal_y = zoom;
    request.center_x = width * 0.5; request.center_y = height * 0.5;
    request.near_clip = 0.01;
    double plane[9]{};
    if (flags & AEGP_LayerFlag_LAYER_IS_3D) {
        // Undo AE's later projection of the layer's Z=0 plane.
        for (int c = 0; c < 3; ++c) {
            const int r = c == 2 ? 3 : c;
            const auto* v = request.layer_to_view + r * 4;
            plane[c] = zoom * v[0] + request.center_x * v[2];
            plane[3 + c] = zoom * v[1] + request.center_y * v[2];
            plane[6 + c] = v[2];
        }
    } else {
        plane[0] = layer.mat[0][0]; plane[1] = layer.mat[1][0]; plane[2] = layer.mat[3][0];
        plane[3] = layer.mat[0][1]; plane[4] = layer.mat[1][1]; plane[5] = layer.mat[3][1]; plane[8] = 1;
    }
    if (!inverse<3>(plane, request.image_to_layer)) return PF_Err_BAD_CALLBACK_PARAM;
    request.camera_enabled = 1;
    return PF_Err_NONE;
}
}
