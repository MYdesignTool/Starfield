#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectCBSuites.h"
#include "NodeEffects.hpp"
#include "Param_Utils.h"
#include "NodeEffectFlags.h"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include "EditorPresetPicker.hpp"
#include "TransformNullUI.hpp"

#include <array>
#include <cstdio>
#include <cstring>
#include <vector>

// Actual node EffectMain; only the independent graph-sync callback is replaced.
PF_Err register_node_graph_sync(PF_InData*) noexcept { return PF_Err_NONE; }
PF_Err update_native_particle_visibility(PF_InData*,PF_ParamDef*[]) noexcept {return PF_Err_NONE;}
PF_Err sync_node_graph_parameter(PF_InData*, PF_OutData*, PF_ParamDef*[],
                                 const PF_UserChangedParamExtra*,bool) noexcept { return PF_Err_NONE; }
namespace starfield::adapter {
PF_Err transform_null_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept {return PF_Err_NONE;}
bool choose_curve_preset(PF_InData*,starfield::core::AgeCurve&) noexcept {return false;}
bool choose_gradient_preset(PF_InData*,starfield::core::ColorGradient&) noexcept {return false;}
}

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
#elif defined(STARFIELD_NODE_KIND_TRANSFORM)
    constexpr Kind kind = Kind::transform;
#else
    constexpr Kind kind = Kind::force;
#endif
    PF_InData host{}; PF_OutData out{};
    host.inter.add_param = add_parameter;
    check(EffectMain(PF_Cmd_GLOBAL_SETUP, &host, &out, nullptr, nullptr, nullptr) == 0, "global setup succeeds");
    check(out.out_flags2 == (kind==Kind::particle?STARFIELD_PARTICLE_OUT_FLAGS2:STARFIELD_NODE_OUT_FLAGS2),
          "float awareness always advertises implemented SmartFX");
    check(out.out_flags == (kind==Kind::particle?STARFIELD_PARTICLE_OUT_FLAGS:
        kind==Kind::transform?STARFIELD_TRANSFORM_OUT_FLAGS:STARFIELD_NODE_OUT_FLAGS), "node remains internal/menu-hidden with declared UI capabilities");
    check(EffectMain(PF_Cmd_PARAMS_SETUP, &host, &out, nullptr, nullptr, nullptr) == 0, "node controls register");
    check(out.num_params == parameter_count(kind) && registered.size() + 1 == static_cast<std::size_t>(out.num_params),
          "registered node count matches shared native stream layout");
    bool dimensions_valid=true,hidden_valid=true;A_long registered_index=0;
    for(const auto& def:registered) {
        ++registered_index;
        const bool custom=bool(def.ui_flags&(PF_PUI_CONTROL|PF_PUI_TOPIC));
        const bool null_button=kind==Kind::transform && registered_index==1;
        dimensions_valid &= null_button?(def.ui_width==160 && def.ui_height==28 && bool(def.ui_flags&PF_PUI_CONTROL)):
            custom?(def.ui_width==300 && def.ui_height==178):(def.ui_width==0 && def.ui_height==0);
        if(registered_index>base_parameter_count(kind))hidden_valid &=
            bool(def.ui_flags&PF_PUI_NO_ECW_UI) && bool(def.ui_flags&PF_PUI_INVISIBLE) && def.ui_width==0 && def.ui_height==0;
    }
    check(dimensions_valid,"only declared custom controls have nonstandard dimensions");
    check(hidden_valid,"all internal node metadata retains hidden flags and zero dimensions");
    if(kind==Kind::emitter) for(A_long index:{12,13,14})
        check(registered[index-1].param_type==PF_Param_ANGLE &&
            (registered[index-1].flags & PF_ParamFlag_CANNOT_TIME_VARY)==0 &&
            registered[index-1].u.ad.value==0,"rotation registers native AE Angle with zero turns and keyframes");
    check(std::strcmp(registered[uuid_first_index(kind)-1].name, "Node UUID 0") == 0, "UUID stream index matches compiler");
    check(std::strcmp(registered[sync_guard_index(kind)-1].name, "Panel Sync Guard") == 0, "guard stream index matches compiler");
    if constexpr (kind == Kind::emitter) {
        check(std::strcmp(registered[1].name, "Emitting") == 0 && registered[1].param_type == PF_Param_POPUP,
              "source mode is runtime stream 2");
        check(std::strcmp(registered[3].name, "Origin XY") == 0 && registered[3].param_type == PF_Param_POINT,
              "native XY is runtime point stream 4");
        check(std::strcmp(registered[19].name, "Emit Chance") == 0 &&
              registered[19].u.fs_d.display_flags == PF_ValueDisplayFlag_PERCENT,
              "auxiliary bank starts at runtime stream 20 in percent");
        check(std::strcmp(registered[26].name, "Random Seed") == 0, "seed ends source controls at runtime stream 27");
    }
    if constexpr(kind==Kind::particle) {
        bool bank_unsupervised=true;
        for(A_long i=particle_layout::gradient;i<particle_layout::gradient_first+16;++i)
            bank_unsupervised &= (registered[i-1].flags&PF_ParamFlag_CANNOT_TIME_VARY) && !(registered[i-1].flags&PF_ParamFlag_SUPERVISE);
        check(bank_unsupervised,"custom events author one bank without intermediate supervised leaf callbacks");
        check(std::strcmp(registered[0].name,"Shape")==0 && registered[0].u.pd.num_choices==3,"supported shape choices lead Particle controls");
        check(std::strcmp(registered[1].name,"Life (Seconds)")==0 && registered[1].u.fs_d.value==2,"Life is two seconds by default");
        check(std::strcmp(registered[2].name,"Life Random")==0 && registered[2].u.fs_d.display_flags==PF_ValueDisplayFlag_PERCENT,"Life Random is a percentage");
        for(A_long index:{particle_layout::angle,particle_layout::angle+1,particle_layout::angle+2,
            particle_layout::speed,particle_layout::speed+1,particle_layout::speed+2,particle_layout::limit_angle})
            check(registered[index-1].param_type==PF_Param_ANGLE,"particle angles and spin use native AE Angle controls");
        check(std::strcmp(registered[12].name,"Color Gradient 0 Position")==0 && (registered[12].flags&PF_ParamFlag_CANNOT_TIME_VARY),"gradient endpoint positions stay structural");
        check((registered[13].ui_flags&PF_PUI_INVISIBLE)!=0,"gradient colors are edited by the native visual control");
        check(std::strcmp(registered[11].name,"Color Gradient")==0 && (registered[11].ui_flags&PF_PUI_CONTROL) && registered[11].ui_height==178,"native gradient control replaces numerical stop banks");
        check(registered[particle_layout::properties-1].param_type==PF_Param_GROUP_START &&
            registered[particle_layout::properties_end-1].param_type==PF_Param_GROUP_END,"Particle Properties grouping preserves root Life controls");
    }
    if constexpr(kind==Kind::emitter) {
        check(registered[1].u.pd.num_choices==4,"Emitting exposes timing choices");
        check(registered[14].u.pd.value==2,"Direction defaults Uniform");
        check(std::strcmp(registered[33].name,"Auxiliary Source")==0,"Auxiliary source is a separate structural control");
    }
    if constexpr (kind == Kind::force) {
        check(registered[0].param_type==PF_Param_FLOAT_SLIDER && registered[0].uu.id==disk_ids::kForceGravityId,"Force uses scalar Gravity with a fresh bounded disk identity");
        check(std::strcmp(registered[1].name,"Gravity random")==0,"Force random control follows Gravity");
        check(std::strcmp(registered[2].name,"Wind X")==0 && std::strcmp(registered[9].name,"Air Density")==0,"reference wind and air control ordering");
        check(std::strcmp(registered[10].name,"Wind and Spin Curve Count")==0 && registered[10].uu.id==900,"Force curve bank starts at runtime 11");
        check(registered[0].u.fs_d.slider_max==100 && registered[0].u.fs_d.valid_max==100000,"Force typed range does not determine scrub sensitivity");
        check(registered[8].u.fs_d.slider_max==10 && registered[8].u.fs_d.precision==PF_Precision_TENTHS,"Spin delay uses seconds and a useful native range");
    }
    if constexpr(kind==Kind::transform) {
        const char* names[]{"Inherit Motion (Null Layer)","Anchor XY","Anchor Z","Position X","Position Y","Position Z",
            "Rotation X","Rotation Y","Rotation Z","Scale X","Scale Y","Scale Z","Particles Scale","Particles Opacity"};
        for(A_long i=0;i<14;++i)check(std::strcmp(registered[i].name,names[i])==0 && registered[i].uu.id==1401+i,
            "Transform visible ordering and disk IDs match the authoring contract");
        check(registered[0].param_type==PF_Param_LAYER && registered[0].u.ld.dephault==PF_LayerDefault_NONE &&
            (registered[0].flags & PF_ParamFlag_CANNOT_TIME_VARY),"inherited layer is a constant resource selector");
        check(registered[1].param_type==PF_Param_POINT,"Anchor XY uses a native point");
        for(A_long i=6;i<9;++i)check(registered[i].param_type==PF_Param_ANGLE &&
            (registered[i].flags & PF_ParamFlag_START_COLLAPSED),"Transform rotations animate and default collapsed");
        for(A_long i=9;i<14;++i)check(registered[i].u.fs_d.value==100,"Transform scale and opacity default to 100 percent");
        check(registered[9].u.fs_d.valid_min<0,"system Scale accepts reflection");
        check(binding_field_count(kind)==26 && base_parameter_count(kind)==14,
            "synthetic affine bindings have their own bound, separate from native indices");
    }
    int colors = 0;
    bool controls_constant = true, controls_animated = true, interpolation_unrestricted = true, colors_supervised = true;
    A_long control_index = 0;
    const A_long last_animated = kind == Kind::emitter ? 33 : kind == Kind::particle || kind==Kind::transform ? 14 : 10;
    for (const auto& control : registered) {
        ++control_index;
        if (control.param_type == PF_Param_GROUP_START || control.param_type == PF_Param_GROUP_END) continue;
        if ((kind==Kind::particle?particle_layout::animated(control_index):control_index<=last_animated) &&
            !(kind==Kind::emitter && control_index==2) && !(kind==Kind::transform && control_index==1))
            controls_animated &= (control.flags & PF_ParamFlag_CANNOT_TIME_VARY) == 0;
        else controls_constant &= (control.flags & PF_ParamFlag_CANNOT_TIME_VARY) != 0;
        interpolation_unrestricted &= (control.flags & PF_ParamFlag_CANNOT_INTERP) == 0;
        if (control.param_type == PF_Param_COLOR) {
            ++colors;
            if(!(control.ui_flags & PF_PUI_INVISIBLE)) colors_supervised &= (control.flags & PF_ParamFlag_SUPERVISE) != 0;
        }
    }
    check(controls_animated, "all public node controls permit keyframes");
    check(controls_constant, "topology identity and curve banks stay constant");
    check(interpolation_unrestricted, "constant streams do not request unnecessary interpolation restrictions");
    check(colors == (kind == Kind::particle ? 9 : 0), "Particle registers Color plus eight saved gradient stops");
    check(colors_supervised, "color edits retain the supervised synchronization callback");

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
