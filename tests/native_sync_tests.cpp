#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "GraphCarrier.hpp"
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/AgeCurve.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>

int run_camera_capture_tests();
namespace {
using namespace starfield::adapter;
namespace core = starfield::core;
namespace records = starfield::adapter::native_nodes;
int checks{}, failures{}, calls{}, sets{}, live_refs{}, acquisitions{};
void check(bool condition, const char* message) {
    ++checks; if (!condition) { ++failures; std::printf("FAILED: %s\n", message); }
}
bool ignore_call{}, ignore_graph{};
A_long fail_set = -1;
A_long fail_read = -1;
A_long wrong_type = -1;
bool corrupt_receipt{};
struct Memory { std::vector<std::byte> bytes; };
std::unordered_map<PF_Handle, std::unique_ptr<Memory>> handles;
PF_Handle allocate(A_u_longlong count) {
    auto memory = std::make_unique<Memory>(); memory->bytes.resize(static_cast<std::size_t>(count));
    auto handle = new void*(memory->bytes.data()); handles.emplace(handle, std::move(memory)); return handle;
}
void* lock_handle(PF_Handle handle) { return handles.at(handle)->bytes.data(); }
void unlock_handle(PF_Handle) {}
void dispose(PF_Handle handle) { if (handle) { check(handles.contains(handle), "disposed handle exists"); handles.erase(handle); delete handle; } }
A_u_longlong size_handle(PF_Handle handle) { return handles.at(handle)->bytes.size(); }
PF_Handle clone(PF_Handle handle) {
    if (!handle) return nullptr;
    auto result = allocate(size_handle(handle)); handles.at(result)->bytes = handles.at(handle)->bytes; return result;
}
struct Fixture {
    const char* name{};
    std::vector<AEGP_StreamVal2> values;
    std::vector<PF_ParamDef> params;
};
std::array<Fixture, 4> fixtures;
struct Ref { std::size_t effect{}; A_long index{}; };
std::size_t effect_index(AEGP_EffectRefH value) { return reinterpret_cast<std::size_t>(value) - 1; }
AEGP_EffectRefH effect_ref(std::size_t i) { return reinterpret_cast<AEGP_EffectRefH>(i + 1); }
AEGP_PFInterfaceSuite1 pf{};
AEGP_EffectSuite4 effect{};
AEGP_StreamSuite6 stream{};
AEGP_UtilitySuite6 utility{};
AEGP_LayerSuite9 layers{};
AEGP_ItemSuite9 items{};
PF_InData renderer_data{};
A_Err acquire(const char* name, int32, const void** out) {
    if (!std::strcmp(name, kAEGPPFInterfaceSuite)) *out = &pf;
    else if (!std::strcmp(name, kAEGPEffectSuite)) *out = &effect;
    else if (!std::strcmp(name, kAEGPStreamSuite)) *out = &stream;
    else if (!std::strcmp(name, kAEGPUtilitySuite)) *out = &utility;
    else if (!std::strcmp(name, kAEGPLayerSuite)) *out = &layers;
    else if (!std::strcmp(name, kAEGPItemSuite)) *out = &items;
    else return 1;
    ++acquisitions; return 0;
}
A_Err release(const char*, int32) { --acquisitions; return 0; }
PF_Err add_param(PF_ProgPtr, PF_ParamIndex, PF_ParamDef* value) {
    fixtures[0].params.push_back(*value); return 0;
}
core::Graph saved_graph() {
    auto graph = read_graph_parameter(&renderer_data, reinterpret_cast<PF_ArbitraryH>(fixtures[0].values[kGraphParameterId].arbH));
    check(graph.has_value(), "saved arbitrary graph decodes"); return graph.has_value() ? graph.take_value() : core::Graph{};
}
const core::ParameterValue& parameter(const core::Graph& graph, const char* kind, core::ParameterKey key) {
    for (const auto& node : graph.nodes) if (node.type_key == kind)
        for (const auto& param : node.parameters) if (param.key == key) return param.value;
    std::printf("Missing graph parameter %s:%u\n", kind, static_cast<unsigned>(key.value)); std::abort();
}
void uuid(std::size_t effect_id, int first, unsigned last_word) {
    fixtures[effect_id].values[first + 7].one_d = last_word;
    fixtures[effect_id].params[first + 7].u.fs_d.value = last_word;
}
void connection(std::size_t id, records::Kind kind, unsigned destination, unsigned edge) {
    auto& node = fixtures[id]; node.values[records::connection_count_index(kind)].one_d = 1;
    uuid(id, records::connection_uuid_index(kind, 0, 0), destination);
    uuid(id, records::connection_edge_uuid_index(kind, 0, 0), edge);
}
node_sync::NativeEdit edit(std::uint32_t kind, A_long index, double value) {
    node_sync::NativeEdit request; request.node_kind = kind; request.parameter_index = index;
    request.uuid[7] = static_cast<std::uint16_t>(kind + 1); request.value[0] = value;
    request.renderer = effect_ref(0); request.layer = reinterpret_cast<AEGP_LayerH>(1);
    request.width = 1920; request.height = 1080; request.time_scale = 24;
    request.handles = renderer_data.utils; request.basic = renderer_data.pica_basicP;
    return request;
}
PF_Err direct_edit(node_sync::NativeEdit& request) {
    // Model the generic selector, not a fully populated native UI callback.
    PF_InData generic = renderer_data;
    generic.num_params = 0; generic.effect_ref = nullptr;
    generic.width = generic.height = generic.current_time = 0;
    generic.time_scale = 0; generic.pixel_aspect_ratio = {0, 0};
    generic.utils = nullptr; generic.pica_basicP = nullptr;
    PF_OutData output{}; return commit_native_graph_edit(&generic, &output, nullptr, &request);
}
}
int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SPBasicSuite basic{}; basic.AcquireSuite = acquire; basic.ReleaseSuite = release;
    PF_UtilCallbacks utils{}; utils.host_new_handle = allocate; utils.host_lock_handle = lock_handle;
    utils.host_unlock_handle = unlock_handle; utils.host_dispose_handle = dispose; utils.host_get_handle_size = size_handle;
    renderer_data.pica_basicP = &basic; renderer_data.effect_ref = reinterpret_cast<PF_ProgPtr>(1);
    renderer_data.utils = &utils; renderer_data.width = 1920; renderer_data.height = 1080;
    renderer_data.pixel_aspect_ratio = {1, 1}; renderer_data.time_scale = 24;
    renderer_data.num_params = kTotalEffectParameterCount + 1;
    renderer_data.inter.add_param = add_param;
    fixtures[0].name = "org.starfieldfx.particle"; fixtures[0].params.resize(1);
    PF_OutData setup{}; check(setup_parameters(&renderer_data, &setup) == 0, "renderer setup succeeds");
    fixtures[0].values.resize(fixtures[0].params.size());
    auto& main = fixtures[0];
    main.values[kGraphParameterId].arbH = reinterpret_cast<AEGP_ArbBlockVal>(main.params[kGraphParameterId].u.arb_d.value);
    for (const A_long i : {kMaxParticlesId, kLayoutOutputXId, kLayoutOutputYId, kGraphRevisionId,
                           kTimeRemapSecondsId, kPreviewChanceId}) main.values[i].one_d = main.params[i].u.fs_d.value;
    for (const A_long i : {kTimeRemapEnabledId, kPreviewEnabledId}) main.values[i].one_d = main.params[i].u.bd.value;
    main.values[kControlSourceId].one_d = kNodeControlSource;
    for (const auto kind : {records::Kind::emitter, records::Kind::particle, records::Kind::force}) {
        const std::size_t id = kind == records::Kind::force ? 3 : static_cast<std::size_t>(kind) + 1;
        fixtures[id].values.resize(records::parameter_count(kind));
        fixtures[id].params.resize(records::parameter_count(kind));
        for (auto& param : fixtures[id].params) param.param_type = PF_Param_FLOAT_SLIDER;
        uuid(id, records::uuid_first_index(kind), static_cast<unsigned>(kind) + 1);
    }
    fixtures[1].name = "org.starfieldfx.node.emitter";
    fixtures[2].name = "org.starfieldfx.node.particle";
    fixtures[3].name = "org.starfieldfx.node.force";
    auto& emitter = fixtures[1]; auto& particle = fixtures[2];
    emitter.values[1].one_d = 1; emitter.values[2].one_d = 1; emitter.values[3].one_d = 30;
    emitter.values[4].two_d = {960, 540}; emitter.values[6].one_d = 100;
    for (int i : {8, 9, 10, 17, 19, 25, 26}) emitter.values[i].one_d = 100;
    emitter.values[15].one_d = 1; emitter.values[16].one_d = 60;
    particle.values[1].one_d = 2; particle.values[2].one_d = 10; particle.values[4].one_d = 100;
    particle.values[6].color = {1, 1, 1, 1}; particle.values[7].color = {1, 1, 1, 1};
    particle.values[8].one_d = 100; particle.values[9].one_d = 100;
    connection(1, records::Kind::emitter, 2, 11);
    connection(2, records::Kind::particle, 4, 12);
    connection(3, records::Kind::force, 255, 13);

    utility.AEGP_RegisterWithAEGP = [](AEGP_GlobalRefcon, const A_char*, AEGP_PluginID* id)->A_Err { *id = 1; return 0; };
    layers.AEGP_GetLayerSourceItem = [](AEGP_LayerH, AEGP_ItemH* item)->A_Err { *item = reinterpret_cast<AEGP_ItemH>(1); return 0; };
    layers.AEGP_GetLayerCurrentTime = [](AEGP_LayerH, AEGP_LTimeMode, A_Time* time)->A_Err { *time = {0, 24}; return 0; };
    items.AEGP_GetItemDimensions = [](AEGP_ItemH, A_long* width, A_long* height)->A_Err { *width = 1920; *height = 1080; return 0; };
    items.AEGP_GetItemPixelAspectRatio = [](AEGP_ItemH, A_Ratio* aspect)->A_Err { *aspect = {1, 1}; return 0; };
    pf.AEGP_GetEffectLayer = [](PF_ProgPtr, AEGP_LayerH* layer)->A_Err { *layer = reinterpret_cast<AEGP_LayerH>(1); return 0; };
    pf.AEGP_GetNewEffectForEffect = [](AEGP_PluginID, PF_ProgPtr, AEGP_EffectRefH* ref)->A_Err { *ref = effect_ref(0); return 0; };
    effect.AEGP_GetLayerNumEffects = [](AEGP_LayerH, A_long* count)->A_Err { *count = 4; return 0; };
    effect.AEGP_GetLayerEffectByIndex = [](AEGP_PluginID, AEGP_LayerH, A_long i, AEGP_EffectRefH* ref)->A_Err { *ref = effect_ref(i); return 0; };
    effect.AEGP_GetInstalledKeyFromLayerEffect = [](AEGP_EffectRefH ref, AEGP_InstalledEffectKey* key)->A_Err { *key = static_cast<AEGP_InstalledEffectKey>(effect_index(ref)); return 0; };
    effect.AEGP_GetEffectMatchName = [](AEGP_InstalledEffectKey key, A_char* name)->A_Err { std::strcpy(name, fixtures[key].name); return 0; };
    effect.AEGP_DisposeEffect = [](AEGP_EffectRefH)->A_Err { return 0; };
    effect.AEGP_EffectCallGeneric = [](AEGP_PluginID, AEGP_EffectRefH, const A_Time*, PF_Cmd cmd, void* extra)->A_Err {
        ++calls; check(cmd == PF_Cmd_COMPLETELY_GENERAL, "SDK generic command transports native edit");
        if (ignore_call) return 0;
        return direct_edit(*static_cast<node_sync::NativeEdit*>(extra));
    };
    stream.AEGP_GetNewEffectStreamByIndex = [](AEGP_PluginID, AEGP_EffectRefH ref, A_long index, AEGP_StreamRefH* out)->A_Err {
        ++live_refs; *out = reinterpret_cast<AEGP_StreamRefH>(new Ref{effect_index(ref), index}); return 0;
    };
    stream.AEGP_GetStreamType = [](AEGP_StreamRefH ref, AEGP_StreamType* type)->A_Err {
        const auto& key = *reinterpret_cast<Ref*>(ref);
        if (key.index == wrong_type) { wrong_type = -1; *type = AEGP_StreamType_COLOR; }
        else *type = key.index == kGraphParameterId ? AEGP_StreamType_ARB : AEGP_StreamType_OneD;
        return 0;
    };
    stream.AEGP_GetNewStreamValue = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_LTimeMode, const A_Time*, A_Boolean, AEGP_StreamValue2* out)->A_Err {
        auto& key = *reinterpret_cast<Ref*>(ref);
        if (!key.effect && key.index == fail_read) { fail_read = -1; return PF_Err_BAD_CALLBACK_PARAM; }
        out->streamH = ref; out->val = fixtures[key.effect].values[key.index];
        if (!key.effect && key.index == kGraphParameterId) out->val.arbH = reinterpret_cast<AEGP_ArbBlockVal>(clone(reinterpret_cast<PF_Handle>(out->val.arbH)));
        return 0;
    };
    stream.AEGP_SetStreamValue = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_StreamValue2* value)->A_Err {
        ++sets; auto& key = *reinterpret_cast<Ref*>(ref);
        check(key.effect == 0, "native compile writes renderer only");
        check(key.index != kGraphEditCommitId && key.index != kGraphEditReceiptId, "native edit preserves CEP nonce and receipt");
        if (key.index == fail_set) { fail_set = -1; return PF_Err_OUT_OF_MEMORY; }
        if (key.index == kGraphParameterId) {
            if (ignore_graph) { ignore_graph = false; return 0; }
            auto copy = clone(reinterpret_cast<PF_Handle>(value->val.arbH));
            dispose(reinterpret_cast<PF_Handle>(fixtures[0].values[key.index].arbH));
            fixtures[0].values[key.index].arbH = reinterpret_cast<AEGP_ArbBlockVal>(copy);
        } else {
            // Model host float storage; integer receipts must remain exact.
            fixtures[0].values[key.index].one_d = static_cast<PF_FpShort>(value->val.one_d);
            if (corrupt_receipt && key.index == kGraphChecksumHighId) {
                corrupt_receipt = false; fixtures[0].values[key.index].one_d += 1;
            }
        }
        return 0;
    };
    stream.AEGP_DisposeStreamValue = [](AEGP_StreamValue2* value)->A_Err {
        auto& key = *reinterpret_cast<Ref*>(value->streamH);
        if (!key.effect && key.index == kGraphParameterId) dispose(reinterpret_cast<PF_Handle>(value->val.arbH)); return 0;
    };
    stream.AEGP_DisposeStream = [](AEGP_StreamRefH ref)->A_Err { --live_refs; delete reinterpret_cast<Ref*>(ref); return 0; };
    check(register_graph_carrier(&renderer_data) == 0 && register_node_graph_sync(&renderer_data) == 0, "both adapters register");
    PF_InData node_data = renderer_data; node_data.effect_ref = reinterpret_cast<PF_ProgPtr>(2);
    node_data.num_params = static_cast<A_long>(emitter.params.size()); node_data.downsample_x = {1, 4}; node_data.downsample_y = {1, 4};
    std::vector<PF_ParamDef*> pointers; for (auto& param : emitter.params) pointers.push_back(&param);
    emitter.params[4].param_type = PF_Param_POINT; emitter.params[4].u.td.x_value = 400 << 16; emitter.params[4].u.td.y_value = 600 << 16;
    emitter.params[4].uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    PF_UserChangedParamExtra changed{}; changed.param_index = 4; PF_OutData output{};
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0, "native Origin commits with saved node value still old");
    auto graph = saved_graph(); auto origin = std::get<core::Vec3>(parameter(graph, core::graph_keys::kEmitterNode, core::graph_keys::kEmitterOrigin));
    check(std::abs(origin.x - 640.0 / 1080) < 1e-9 && std::abs(origin.y + 1860.0 / 1080) < 1e-9, "callback Origin beats delayed old stream with quarter normalization");
    check(emitter.values[4].two_d.x == 960 && emitter.values[4].two_d.y == 540, "source remains owned by host edit");
    check((output.out_flags & PF_OutFlag_FORCE_RERENDER) != 0, "native callback requests rerender");
    check(std::get<std::uint32_t>(parameter(graph, core::graph_keys::kOutputNode, core::graph_keys::kParticleCount)) == 1000000, "main cap comes from AEGP despite null generic params");
    check(std::get<double>(parameter(graph, core::graph_keys::kOutputNode, core::graph_keys::kPreviewChance)) == 100, "main preview comes from AEGP despite generic zero geometry/count");
    node_data.width = node_data.height = 0; node_data.time_scale = 0; node_data.pixel_aspect_ratio = {0, 0};
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0,
          "native UI without dimensions/time uses source item and layer time");
    node_data.width = 1920; node_data.height = 1080; node_data.time_scale = 24; node_data.pixel_aspect_ratio = {1, 1};
    emitter.values[4].two_d = {1600, 2400}; // host saves original control after callback.
    changed.param_index = 3; emitter.params[3].u.fs_d.value = 45;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0, "scalar native value commits without CEP");
    graph = saved_graph(); check(std::get<double>(parameter(graph, core::graph_keys::kEmitterNode, core::graph_keys::kBirthRate)) == 45, "saved graph contains accepted scalar instead of old 30");
    emitter.values[3].one_d = 45;
    changed.param_index = 12; emitter.params[12].param_type = PF_Param_ANGLE;
    emitter.params[12].u.ad.value = 90 << 16;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0, "native angle commits");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kEmitterNode, core::graph_keys::kEmissionAngleX)) == 90, "fixed-point angle converted to degrees");
    emitter.values[12].one_d = 90;
    changed.param_index = 1; emitter.params[1].param_type = PF_Param_POPUP; emitter.params[1].u.pd.value = 2;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0, "native Type popup commits");
    emitter.values[1].one_d = 2;
    changed.param_index = 5; emitter.params[5].u.fs_d.value = -200;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0, "native Origin Z commits");
    emitter.values[5].one_d = -200;
    const int before_calls = calls;
    emitter.params[records::sync_guard_index(records::Kind::emitter)].u.fs_d.value = 1;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0 && calls == before_calls, "CEP guard skips native publication");
    emitter.params[records::sync_guard_index(records::Kind::emitter)].u.fs_d.value = 0;
    changed.param_index = records::layout_x_index(records::Kind::emitter);
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0 && calls == before_calls, "metadata awaits CEP transaction");
    auto life = edit(1, 1, 9); check(direct_edit(life) == 0 && life.accepted, "Particle Life publishes independently");
    particle.values[1].one_d = 9; // Host saves Life after its native callback returns.
    auto null_generic = edit(1, 2, 15); PF_OutData null_output{};
    check(commit_native_graph_edit(nullptr, &null_output, nullptr, &null_generic) == 0 && null_generic.accepted,
          "generic in_data may be absent when request context is complete");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kParticleLifetimeSeconds)) == 9, "Particle lifetime saved");
    auto opacity = edit(1, 4, 25); check(direct_edit(opacity) == 0, "Particle opacity publishes");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kOpacityStart)) == .25, "percent opacity normalized");
    auto color = edit(1, 6, 0); color.value_kind = node_sync::ValueKind::color; color.value = {.2, .3, .4, 1};
    check(direct_edit(color) == 0, "color publishes with delayed stream");
    check(std::get<core::Vec3>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kColorStart)).x == .2, "color value replaces only selected color");
    auto gravity = edit(3, 1, 1080); check(direct_edit(gravity) == 0, "Force Gravity publishes");
    check(std::get<core::Vec3>(parameter(saved_graph(), core::graph_keys::kForceNode, core::graph_keys::kGravity)).y == -1, "Gravity units remain intact");
    for (int count : {10, 27}) {
        particle.values[count].one_d = 2; particle.values[count + 1].one_d = 0;
        particle.values[count + 2].one_d = 100; particle.values[count + 3].one_d = 1; particle.values[count + 4].one_d = 0;
    }
    auto curve = edit(1, 14, 60); check(direct_edit(curve) == 0, "single curve bank value commits");
    graph = saved_graph();
    core::AgeCurve size_curve{}, opacity_curve{};
    const bool size_ok = core::decode_age_curve(std::get<core::OpaqueBytes>(parameter(graph, core::graph_keys::kParticleNode, core::graph_keys::kSizeOverLifeCurve)), size_curve, 0, 100);
    const bool opacity_ok = core::decode_age_curve(std::get<core::OpaqueBytes>(parameter(graph, core::graph_keys::kParticleNode, core::graph_keys::kOpacityOverLifeCurve)), opacity_curve, 0, 100);
    check(size_ok && size_curve.points[1].value == 60 && opacity_ok && opacity_curve.points[1].value == 0, "other Over Life curve preserved");
    const double old_revision = main.values[kGraphRevisionId].one_d;
    const auto old_bytes = handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
    const auto old_handle_count = handles.size();
    auto missing_control = edit(1, 2, 20); fail_read = kMaxParticlesId;
    check(direct_edit(missing_control) != 0 && !missing_control.accepted &&
          missing_control.stage == node_sync::Stage::controls && missing_control.stream_index == kMaxParticlesId,
          "missing main control reports stage and stream index");
    auto missing_context = edit(1, 2, 20); missing_context.layer = nullptr;
    check(direct_edit(missing_context) != 0 && missing_context.stage == node_sync::Stage::context,
          "missing borrowed context rejected with context diagnostic");
    auto bad_override = edit(1, 2, 20); bad_override.value_kind = node_sync::ValueKind::color;
    check(direct_edit(bad_override) != 0 && bad_override.stage == node_sync::Stage::compile &&
          bad_override.stream_index == 2, "node compiler identifies wrong edited stream type");
    auto typed_receipt = edit(1, 2, 20); wrong_type = kGraphChecksumHighId;
    const int sets_before_type_check = sets;
    check(direct_edit(typed_receipt) != 0 && typed_receipt.stage == node_sync::Stage::capture &&
          typed_receipt.stream_index == kGraphChecksumHighId && sets == sets_before_type_check,
          "non-scalar receipt rejected before reading union or writing streams");
    auto invalid = edit(1, 2, 20); invalid.uuid[7] = 500;
    check(direct_edit(invalid) != 0 && !invalid.accepted && main.values[kGraphRevisionId].one_d == old_revision, "missing UUID rejected before publish");
    auto failed = edit(1, 2, 20); fail_set = kGraphParameterId;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::publish_graph && failed.stream_index == kGraphParameterId, "failed graph write restores revision and reports graph phase");
    check(handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes == old_bytes, "failed write retains old graph");
    failed = edit(1, 2, 20); fail_set = kGraphChecksumLowId;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::publish_scalars && failed.stream_index == kGraphChecksumLowId,
          "failed metadata write restores revision and reports scalar phase");
    failed = edit(1, 2, 20); corrupt_receipt = true;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::verify_scalars && failed.stream_index == kGraphChecksumHighId,
          "changed integer receipt fails exact readback and restores graph");
    failed = edit(1, 2, 20); ignore_graph = true;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::verify_graph && failed.stream_index == kGraphParameterId,
          "silent ignored graph write reports graph readback and rolls back");
    check(handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes == old_bytes, "readback rejection restores graph");
    changed.param_index = 3; ignore_call = true; output = {};
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) != 0 && !(output.out_flags & PF_OutFlag_FORCE_RERENDER), "ignored generic call cannot acknowledge native edit");
    ignore_call = false;
    check(handles.size() == old_handle_count && live_refs == 0 && acquisitions == 0, "all publication handles/suites balanced");
    main.values[kGraphRevisionId].one_d = 16777214;
    auto max_receipt = edit(1, 2, 20);
    check(direct_edit(max_receipt) == 0 && max_receipt.accepted && max_receipt.revision == 16777215 &&
          main.values[kGraphRevisionId].one_d == 16777215,
          "largest integer revision survives host float storage and exact verification");
    dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));
    // PARAMS_SETUP creates its own default arbitrary value.
    for (auto& param : main.params) if (param.param_type == PF_Param_ARBITRARY_DATA) {
        if (handles.contains(param.u.arb_d.value)) dispose(param.u.arb_d.value);
        if (handles.contains(param.u.arb_d.dephault)) dispose(param.u.arb_d.dephault);
    }
    check(handles.empty(), "all retained graph handles released");
    std::printf("Native sync: %d checks, %d failures\n", checks, failures);
    failures += run_camera_capture_tests(); return failures ? 1 : 0;
}
