#ifndef STARFIELD_CORE_PLUGIN_API_H
#define STARFIELD_CORE_PLUGIN_API_H

/* ABI between StarfieldParticle.aex and a runtime-loaded core. No AE or C++ type
 * crosses this boundary. All pointers remain valid only for the duration stated. */
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SF_CORE_ABI_VERSION 6u
#if defined(_WIN32)
#define SF_CORE_CALL __cdecl
#if defined(SF_CORE_BUILD_DLL)
#define SF_CORE_EXPORT __declspec(dllexport)
#else
#define SF_CORE_EXPORT
#endif
#else
#define SF_CORE_CALL
#define SF_CORE_EXPORT
#endif

typedef enum SfCoreStatus {
    SF_CORE_OK = 0,
    SF_CORE_INVALID_REQUEST = 1,
    SF_CORE_INVALID_TIME = 2,
    SF_CORE_UNSUPPORTED_FORMAT = 3,
    SF_CORE_ALLOCATION_FAILED = 4,
    SF_CORE_WORK_LIMIT = 5,
    SF_CORE_CANCELLED = 6,
    SF_CORE_INTERNAL_FAILURE = 7
} SfCoreStatus;

typedef struct SfCoreRect {
    int32_t left, top, right, bottom;
} SfCoreRect;

typedef struct SfCoreFrame {
    uint32_t layer_width, layer_height;
    uint32_t frame_width, frame_height;
    SfCoreRect roi;
    int64_t time_value, time_scale;
    int64_t duration_value, duration_scale;
    uint32_t pixel_format, color_space, alpha_mode, quality;
    double pixel_aspect_ratio;
} SfCoreFrame;

typedef struct SfTextureSource {
    uint32_t struct_size, resource_id;
    double start_seconds, end_seconds, frame_seconds;
    uint32_t width, height;
    double pixel_aspect_ratio;
} SfTextureSource;
typedef struct SfTextureFrame {
    uint32_t struct_size, resource_id, frame_index, width, height, row_floats;
    const float* pixels; /* caller-owned numeric staging, never an AE world */
    uint64_t pixel_float_count;
} SfTextureFrame;

/* The graph bytes and callback context are caller-owned during this call only.
 * pixel_format: 0=RGBA8, 1=RGBA16, 2=RGBA32F. Other enum fields follow the
 * values in Render.hpp; unknown values are rejected by the DLL. */
typedef struct SfCoreRenderRequest {
    uint32_t struct_size;
    SfCoreFrame frame;
    const void* graph_bytes;
    uint64_t graph_byte_count;
    uint64_t graph_revision;
    int32_t (SF_CORE_CALL *is_cancelled)(void* context);
    void* cancel_context;
    uint32_t camera_enabled;
    double layer_to_view[16];
    double image_to_layer[9];
    double focal_x, focal_y, center_x, center_y, near_clip;
    uint32_t texture_source_count, texture_frame_count;
    const SfTextureSource* texture_sources;
    const SfTextureFrame* texture_frames;
} SfCoreRenderRequest;

/* pixels and opaque_handle are owned by this DLL generation. The adapter copies
 * pixels while its generation lease is alive, then calls release_render_result
 * from the SAME generation on every success and failure path. */
typedef struct SfCoreRenderResult {
    uint32_t struct_size;
    SfCoreStatus status;
    char detail[128];
    SfCoreRect region;
    uint32_t row_bytes, pixel_format;
    const void* pixels;
    uint64_t pixel_byte_count;
    void* opaque_handle;
} SfCoreRenderResult;

typedef struct SfCoreInspectRequest {
    uint32_t struct_size;
    const void* graph_bytes;
    uint64_t graph_byte_count;
    int64_t time_value, time_scale;
} SfCoreInspectRequest;

typedef struct SfCoreInspectResult {
    uint32_t struct_size;
    SfCoreStatus status;
    char detail[128];
    uint64_t node_count, edge_count, live_particle_count;
} SfCoreInspectResult;

/* Numeric, immutable, ROI-relative scene. No host pointers cross this ABI.
 * All arrays belong to opaque_handle and are released by the same DLL generation.
 * Tiles are 16x16; indices in each tile preserve back-to-front sprite order. */
typedef struct SfGpuSprite {
    float x, y, inverse_ax, inverse_ay, inverse_bx, inverse_by;
    float edge_scale, feather, red, green, blue, opacity;
    int32_t left, top, right, bottom;
    uint32_t shape, reserved[3]; /* ABI5: reserved[0] is ParticleTransferMode (0..3); remaining words zero. */
} SfGpuSprite;
typedef struct SfCoreGpuSceneResult {
    uint32_t struct_size;
    SfCoreStatus status;
    char detail[128];
    SfCoreRect region;
    uint32_t tile_size, tiles_x, tiles_y, sprite_count, index_count;
    const SfGpuSprite* sprites;
    const uint32_t* tile_offsets; /* tiles_x * tiles_y + 1 entries */
    const uint32_t* tile_indices;
    void* opaque_handle;
} SfCoreGpuSceneResult;

typedef struct SfCoreApi {
    uint32_t struct_size;
    uint32_t abi_version;
    SfCoreStatus (SF_CORE_CALL *render)(const SfCoreRenderRequest*, SfCoreRenderResult*);
    void (SF_CORE_CALL *release_render_result)(SfCoreRenderResult*);
    SfCoreStatus (SF_CORE_CALL *inspect)(const SfCoreInspectRequest*, SfCoreInspectResult*);
    SfCoreStatus (SF_CORE_CALL *prepare_gpu_scene)(const SfCoreRenderRequest*, SfCoreGpuSceneResult*);
    void (SF_CORE_CALL *release_gpu_scene)(SfCoreGpuSceneResult*);
} SfCoreApi;

/* Returns 1 only when the complete requested ABI is available. The caller must
 * pass sizeof(SfCoreApi), and keep the containing DLL loaded while using it. */
typedef int32_t (SF_CORE_CALL *SfCoreGetApiFn)(uint32_t, uint32_t, SfCoreApi*);
SF_CORE_EXPORT int32_t SF_CORE_CALL StarfieldCore_GetApi(uint32_t abi_version,
                                                        uint32_t api_struct_size,
                                                        SfCoreApi* out_api);

#ifdef __cplusplus
}
#endif
#endif
