#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCBSuites.h"
#include "NodeEffects.hpp"
#include "NodeEffectFlags.h"
#include "NodeRecord.hpp"
#include "SPBasic.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

// Actual node EffectMain; only the independent graph-sync callback is replaced.
PF_Err register_node_graph_sync(PF_InData*) noexcept { return PF_Err_NONE; }
PF_Err sync_node_graph_parameter(PF_InData*, PF_OutData*, PF_ParamDef*[],
                                 const PF_UserChangedParamExtra*) noexcept { return PF_Err_NONE; }

namespace {
int checks = 0, failures = 0;
void check(bool result, const char* description) {
    ++checks;
    if (!result) { ++failures; std::printf("FAILED: %s\n", description); }
}
std::vector<PF_ParamDef> registered;
PF_Err add_parameter(PF_ProgPtr, PF_ParamIndex, PF_ParamDef* value) {
    registered.push_back(*value); return PF_Err_NONE;
}
PF_WorldTransformSuite1 transform{};
PF_EffectWorld input_world{}, output_world{};
PF_CheckoutResult input_result{};
PF_RenderRequest requested{};
A_long requested_time = 0, requested_step = 0;
A_u_long requested_scale = 0;
int checkouts = 0, outputs = 0, checkins = 0, acquisitions = 0, releases = 0, copies = 0;
int bytes_per_pixel = 4;
PF_Err input_error = 0, output_error = 0, copy_error = 0, checkin_error = 0, acquire_error = 0;
PF_Err checkout_metadata(PF_ProgPtr, PF_ParamIndex index, A_long id, const PF_RenderRequest* request,
                         A_long time, A_long step, A_u_long scale, PF_CheckoutResult* result) {
    check(index == 0 && id == 0, "pre-render input and checkout IDs agree");
    requested = *request; requested_time = time; requested_step = step; requested_scale = scale;
    *result = input_result; return input_error;
}
PF_Err checkout_pixels(PF_ProgPtr, A_long id, PF_EffectWorld** world) {
    ++checkouts; check(id == 0, "smart render uses pre-render checkout ID");
    *world = &input_world; return input_error;
}
PF_Err checkout_output(PF_ProgPtr, PF_EffectWorld** world) {
    ++outputs; *world = &output_world; return output_error;
}
PF_Err checkin_pixels(PF_ProgPtr, A_long id) {
    ++checkins; check(id == 0, "input checkin uses checkout ID"); return checkin_error;
}
PF_Err copy_world(PF_ProgPtr, PF_EffectWorld* src, PF_EffectWorld* dst, PF_Rect* src_rect, PF_Rect* dst_rect) {
    ++copies; check(!src_rect && !dst_rect, "passthrough delegates whole ROI worlds to host copy");
    if (copy_error) return copy_error;
    for (A_long y = 0; y < src->height; ++y) {
        std::memcpy(reinterpret_cast<char*>(dst->data) + y * dst->rowbytes,
                    reinterpret_cast<const char*>(src->data) + y * src->rowbytes,
                    static_cast<std::size_t>(src->width * bytes_per_pixel));
    }
    return PF_Err_NONE;
}
SPErr acquire(const char* name, int32 version, const void** suite) {
    ++acquisitions;
    check(std::strcmp(name, kPFWorldTransformSuite) == 0 && version == kPFWorldTransformSuiteVersion1,
          "uses float-capable world transform suite");
    *suite = &transform; return acquire_error;
}
SPErr release(const char*, int32) { ++releases; return kSPNoError; }
}

int main() {
    using namespace starfield::adapter::native_nodes;
#if defined(STARFIELD_NODE_KIND_EMITTER)
    constexpr Kind kind = Kind::emitter;
#elif defined(STARFIELD_NODE_KIND_PARTICLE)
    constexpr Kind kind = Kind::particle;
#elif defined(STARFIELD_NODE_KIND_APPEARANCE)
    constexpr Kind kind = Kind::appearance;
#else
    constexpr Kind kind = Kind::force;
#endif
    PF_InData host{}; PF_OutData out{};
    host.inter.add_param = add_parameter;
    check(EffectMain(PF_Cmd_GLOBAL_SETUP, &host, &out, nullptr, nullptr, nullptr) == 0, "global setup succeeds");
    check(out.out_flags2 == (PF_OutFlag2_FLOAT_COLOR_AWARE | PF_OutFlag2_SUPPORTS_SMART_RENDER),
          "float awareness always advertises implemented SmartFX");
    check(out.out_flags == STARFIELD_NODE_OUT_FLAGS, "node remains internal/menu-hidden");
    check(EffectMain(PF_Cmd_PARAMS_SETUP, &host, &out, nullptr, nullptr, nullptr) == 0, "node controls register");
    check(out.num_params == parameter_count(kind) && registered.size() + 1 == static_cast<std::size_t>(out.num_params),
          "registered node count matches shared native stream layout");
    check(std::strcmp(registered[uuid_first_index(kind)-1].name, "Node UUID 0") == 0, "UUID stream index matches compiler");
    check(std::strcmp(registered[sync_guard_index(kind)-1].name, "Panel Sync Guard") == 0, "guard stream index matches compiler");

    host.current_time = 33; host.time_step = 1; host.time_scale = 24;
    PF_PreRenderInput pre_input{}; PF_PreRenderOutput pre_output{};
    PF_PreRenderCallbacks pre_callbacks{}; pre_callbacks.checkout_layer = checkout_metadata;
    PF_PreRenderExtra pre{&pre_input, &pre_output, &pre_callbacks};
    pre_input.output_request.rect = {5, 7, 9, 11};
    pre_input.output_request.preserve_rgb_of_zero_alpha = TRUE;
    input_result.result_rect = pre_input.output_request.rect;
    input_result.max_result_rect = {0, 0, 1920, 1080}; input_result.solid = FALSE;
    check(EffectMain(PF_Cmd_SMART_PRE_RENDER, &host, &out, nullptr, nullptr, &pre) == 0, "pre-render selector executes");
    check(std::memcmp(&requested, &pre_input.output_request, sizeof(requested)) == 0, "request forwards ROI and zero-alpha RGB policy");
    check(requested_time == 33 && requested_step == 1 && requested_scale == 24, "request forwards host rational time");
    check(std::memcmp(&pre_output.result_rect, &input_result.result_rect, sizeof(PF_LRect)) == 0 &&
          std::memcmp(&pre_output.max_result_rect, &input_result.max_result_rect, sizeof(PF_LRect)) == 0,
          "passthrough retains input result/max bounds");
    check(!pre_output.pre_render_data && !pre_output.delete_pre_render_data_func, "no node pre-render state lifetime");
    input_error = PF_Err_OUT_OF_MEMORY;
    check(EffectMain(PF_Cmd_SMART_PRE_RENDER, &host, &out, nullptr, nullptr, &pre) == input_error, "metadata errors propagate");
    input_error = 0;

    SPBasicSuite basic{}; basic.AcquireSuite = acquire; basic.ReleaseSuite = release; host.pica_basicP = &basic;
    transform.copy = copy_world;
    PF_SmartRenderInput render_input{};
    PF_SmartRenderCallbacks callbacks{checkout_pixels, checkin_pixels, checkout_output};
    PF_SmartRenderExtra render{&render_input, &callbacks};
    std::array<unsigned char, 128> src{}, dst{};
    input_world.width = output_world.width = 2; input_world.height = output_world.height = 2;
    input_world.data = reinterpret_cast<PF_PixelPtr>(src.data());
    output_world.data = reinterpret_cast<PF_PixelPtr>(dst.data());
    for (int depth : {8, 16, 32}) {
        render_input.bitdepth = static_cast<short>(depth); bytes_per_pixel = depth / 2;
        input_world.rowbytes = 2 * bytes_per_pixel + 4; output_world.rowbytes = 2 * bytes_per_pixel + 8;
        for (std::size_t i = 0; i < src.size(); ++i) src[i] = static_cast<unsigned char>(i * 17);
        dst.fill(0xee);
        const int old_checkins = checkins;
        check(EffectMain(PF_Cmd_SMART_RENDER, &host, &out, nullptr, nullptr, &render) == 0, "8/16/32-bpc selector succeeds");
        check(checkins == old_checkins + 1, "each successful checkout is checked in once");
        for (int y = 0; y < 2; ++y) {
            check(std::memcmp(src.data() + y * input_world.rowbytes, dst.data() + y * output_world.rowbytes,
                              2 * bytes_per_pixel) == 0, "pixel bytes pass through without alpha/depth conversion");
            check(dst[y * output_world.rowbytes + 2 * bytes_per_pixel] == 0xee, "row padding is untouched");
        }
    }
    check(acquisitions == releases && copies == 3, "all world suites released");
    auto run = [&] { return EffectMain(PF_Cmd_SMART_RENDER, &host, &out, nullptr, nullptr, &render); };
    int old_checkins = checkins, old_outputs = outputs;
    input_error = PF_Err_OUT_OF_MEMORY;
    check(run() == input_error && checkins == old_checkins && outputs == old_outputs, "failed input checkout is not checked in");
    input_error = 0; output_error = PF_Err_BAD_CALLBACK_PARAM;
    check(run() == output_error && checkins == old_checkins + 1, "output failure still checks in input");
    output_error = 0; acquire_error = PF_Err_OUT_OF_MEMORY;
    check(run() == acquire_error && checkins == old_checkins + 2, "suite acquisition failure still checks in input");
    acquire_error = 0; copy_error = PF_Err_INTERNAL_STRUCT_DAMAGED; checkin_error = PF_Err_BAD_CALLBACK_PARAM;
    check(run() == copy_error && checkins == old_checkins + 3, "copy error takes priority over checkin error");
    copy_error = 0;
    check(run() == checkin_error, "checkin error propagates when copying succeeded");
    checkin_error = 0; output_world.width = 0; const int old_copies = copies;
    check(run() == 0 && copies == old_copies, "empty output does not request a copy");
    check(EffectMain(PF_Cmd_SMART_RENDER, &host, &out, nullptr, nullptr, nullptr) == PF_Err_BAD_CALLBACK_PARAM,
          "missing SmartFX callbacks rejected");
    std::printf("Node kind %d: %d checks, %d failures\n", static_cast<int>(kind), checks, failures);
    return failures ? 1 : 0;
}
