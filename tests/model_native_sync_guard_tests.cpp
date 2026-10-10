#include "NodeEffects.hpp"
#include "NativeGraphCommit.hpp"
#include "NodeGraphSync.hpp"
#include "Parameters.hpp"
#include "EffectGraphBackup.hpp"
#include "SPBasic.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
using namespace starfield::adapter;
namespace records = native_nodes;
int checks{}, failures{}, commits{}, refs{}, acquisitions{}, effects_read{};
std::array<double, 4> guards{};
int effect_count = 3;
A_long fail_guard = -1;
AEGP_PFInterfaceSuite1 pf{};
AEGP_EffectSuite4 effects{};
AEGP_StreamSuite6 streams{};
AEGP_UtilitySuite6 utility{};
struct Stream { unsigned effect; A_long index; };
void check(bool value, const char* message) {
    ++checks;
    if (!value) { ++failures; std::printf("FAILED: %s\n", message); }
}
unsigned row(AEGP_EffectRefH ref) { return static_cast<unsigned>(reinterpret_cast<std::uintptr_t>(ref) - 1); }
AEGP_EffectRefH ref(unsigned index) { return reinterpret_cast<AEGP_EffectRefH>(std::uintptr_t(index + 1)); }
A_Err acquire(const char* name, int32, const void** out) {
    if (!std::strcmp(name, kAEGPPFInterfaceSuite)) *out = &pf;
    else if (!std::strcmp(name, kAEGPEffectSuite)) *out = &effects;
    else if (!std::strcmp(name, kAEGPStreamSuite)) *out = &streams;
    else if (!std::strcmp(name, kAEGPUtilitySuite)) *out = &utility;
    else return 516;
    ++acquisitions; return 0;
}
A_Err release(const char*, int32) { --acquisitions; return 0; }
void initialize() {
    utility.AEGP_RegisterWithAEGP = [](AEGP_GlobalRefcon, const A_char*, AEGP_PluginID* id)->A_Err { *id = 701; return 0; };
    pf.AEGP_GetEffectLayer = [](PF_ProgPtr, AEGP_LayerH* layer)->A_Err { *layer = reinterpret_cast<AEGP_LayerH>(1); return 0; };
    pf.AEGP_GetNewEffectForEffect = [](AEGP_PluginID, PF_ProgPtr, AEGP_EffectRefH* out)->A_Err { *out = ref(1); ++refs; return 0; };
    effects.AEGP_GetLayerNumEffects = [](AEGP_LayerH, A_long* count)->A_Err { *count = effect_count; return 0; };
    effects.AEGP_GetLayerEffectByIndex = [](AEGP_PluginID, AEGP_LayerH, A_long index, AEGP_EffectRefH* out)->A_Err {
        ++effects_read; *out = ref(static_cast<unsigned>(index)); ++refs; return 0;
    };
    effects.AEGP_GetInstalledKeyFromLayerEffect = [](AEGP_EffectRefH value, AEGP_InstalledEffectKey* key)->A_Err { *key = row(value); return 0; };
    effects.AEGP_GetEffectMatchName = [](AEGP_InstalledEffectKey key, A_char* name)->A_Err {
        std::strcpy(name, key == 1 ? "org.starfieldfx.node.model" : "org.starfieldfx.particle"); return 0;
    };
    effects.AEGP_DisposeEffect = [](AEGP_EffectRefH)->A_Err { --refs; return 0; };
    streams.AEGP_GetNewEffectStreamByIndex = [](AEGP_PluginID, AEGP_EffectRefH value, A_long index, AEGP_StreamRefH* out)->A_Err {
        if (index == fail_guard) return 516;
        *out = reinterpret_cast<AEGP_StreamRefH>(new Stream{row(value), index}); ++refs; return 0;
    };
    streams.AEGP_GetStreamType = [](AEGP_StreamRefH, AEGP_StreamType* type)->A_Err { *type = AEGP_StreamType_OneD; return 0; };
    streams.AEGP_GetNewStreamValue = [](AEGP_PluginID, AEGP_StreamRefH value, AEGP_LTimeMode, const A_Time*, A_Boolean, AEGP_StreamValue2* out)->A_Err {
        const auto& stream = *reinterpret_cast<Stream*>(value);
        check(stream.index == (stream.effect == 1 ? records::sync_guard_index(records::Kind::model) : kGraphSyncGuardId), "only live guard controls read before publication");
        out->streamH = value; out->val.one_d = guards[stream.effect]; return 0;
    };
    streams.AEGP_DisposeStreamValue = [](AEGP_StreamValue2*)->A_Err { return 0; };
    streams.AEGP_DisposeStream = [](AEGP_StreamRefH value)->A_Err { delete reinterpret_cast<Stream*>(value); --refs; return 0; };
}
}
namespace starfield::adapter {
PF_Err commit_native_graph_edit(node_sync::NativeEdit* edit, AEGP_PluginID plugin, PF_OutData*) noexcept {
    ++commits;
    check(plugin == 701 && edit->node_kind == 5 && edit->parameter_index == 1 && edit->value[0] == 2 && edit->uuid[7] == 4,
          "unguarded Source edit reaches existing compiler with captured value and identity");
    edit->accepted = true; edit->revision = 1; edit->status = 0; return 0;
}
}
int main() {
    initialize();
    SPBasicSuite basic{}; basic.AcquireSuite = acquire; basic.ReleaseSuite = release;
    PF_UtilCallbacks handles{};
    PF_InData data{}; data.pica_basicP = &basic; data.utils = &handles;
    data.effect_ref = reinterpret_cast<PF_ProgPtr>(2); data.width = 1920; data.height = 1080;
    data.current_time = 24; data.time_scale = 24; data.pixel_aspect_ratio = {1,1};
    std::array<PF_ParamDef, records::parameter_count(records::Kind::model)> params{};
    std::array<PF_ParamDef*, records::parameter_count(records::Kind::model)> pointers{};
    for (unsigned i = 0; i < params.size(); ++i) { params[i].param_type = PF_Param_FLOAT_SLIDER; pointers[i] = &params[i]; }
    params[1].param_type = PF_Param_POPUP; params[1].u.pd.value = 2;
    params[records::uuid_first_index(records::Kind::model) + 7].u.fs_d.value = 4;
    PF_UserChangedParamExtra changed{}; changed.param_index = 1;
    check(register_node_graph_sync(&data) == 0, "Model callback registration");
    const auto run = [&] { PF_OutData output{}; const auto error = sync_node_graph_parameter(&data, &output, pointers.data(), &changed);
        check(refs == 0 && acquisitions == 0, "all callback suites and refs released"); return error; };
    guards = {1, 0, transaction_backup_guard, 0};
    check(run() == 0 && commits == 0, "live renderer transaction guard blocks stale delivered Model guard");
    params[records::uuid_first_index(records::Kind::model) + 7].u.fs_d.value = 0;
    check(run() == 0 && commits == 0, "transaction blocks transient unassigned identity before capture");
    params[records::uuid_first_index(records::Kind::model) + 7].u.fs_d.value = 4;
    guards = {0, 1, transaction_backup_guard, 0};
    check(run() == 0 && commits == 0, "live Model writer guard blocks stale delivered guard outside whole graph transaction");
    guards[1] = transaction_backup_guard;
    check(run() == 0 && commits == 0, "backup Model callback cannot publish");
    guards[1] = 0;
    const auto before = commits;
    check(run() == 0 && commits == before + 1, "real unguarded native Source edit still publishes; backup renderer excluded");
    guards[1] = std::nan("");
    const auto invalid_before = commits;
    check(run() != 0 && commits == invalid_before, "invalid live Model guard fails without publication");
    guards[1] = 0; guards[0] = 3;
    check(run() != 0 && commits == invalid_before, "invalid renderer guard fails without publication");
    guards[0] = 0; effect_count = 4;
    check(run() != 0 && commits == invalid_before, "two live renderers remain rejected");
    effect_count = 3; fail_guard = records::sync_guard_index(records::Kind::model);
    check(run() == 516 && commits == invalid_before, "failed live guard read propagates without publication");
    fail_guard = -1; params[records::sync_guard_index(records::Kind::model)].u.fs_d.value = 1;
    const auto reads = effects_read;
    check(run() == 0 && commits == invalid_before && effects_read == reads, "delivered guard retains zero-query fast path");
    std::printf("Model native sync guard: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
