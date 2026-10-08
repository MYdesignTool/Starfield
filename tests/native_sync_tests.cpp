#include "NodeEffects.hpp"
#include "NativeNodeGraph.hpp"
#include "NativeTemporalCache.hpp"
#include "NativeTemporalUI.hpp"
#include <string>
#include <fstream>
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "GraphCarrier.hpp"
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "EmitterHistory.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include "starfield/core/AgeCurve.hpp"
#include "starfield/core/ColorGradient.hpp"
#include "GradientEditorModel.hpp"
#include "ParticleLayout.hpp"
#include "TransformBinding.hpp"
#include "MotionBlur.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <limits>
#include <unordered_map>
#include <vector>
#include <thread>

int run_camera_capture_tests();
namespace {
using namespace starfield::adapter;
namespace core = starfield::core;
namespace records = starfield::adapter::native_nodes;
int checks{}, failures{}, calls{}, sets{}, live_refs{}, acquisitions{}, suite_requests{}, aegp_suite_requests{};
void check(bool condition, const char* message) {
    ++checks; if (!condition) { ++failures; std::printf("FAILED: %s\n", message); }
}
bool ignore_graph{};
A_long fail_set = -1;
A_long fail_read = -1;
A_long fail_evaluated_read = -1;
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
std::array<Fixture, 5> fixtures;
bool transform_present{};
bool cloud_animation_test{};
using Matrix=transform_binding::Matrix;
Matrix null_world{1,0,0,910,0,1,0,490,0,0,1,0,0,0,0,1};
Matrix owner_world{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
core::Vec3 null_anchor{50,50,0};
int affine_reads{},synthetic_native_reads{};
struct Ref { std::size_t effect{}; A_long index{}; };
std::size_t effect_index(AEGP_EffectRefH value) { return reinterpret_cast<std::size_t>(value) - 1; }
AEGP_EffectRefH effect_ref(std::size_t i) { return reinterpret_cast<AEGP_EffectRefH>(i + 1); }
AEGP_PFInterfaceSuite1 pf{};
AEGP_EffectSuite4 effect{};
AEGP_StreamSuite6 stream{};
AEGP_DynamicStreamSuite4 dynamic{};
bool dynamic_enabled{};
std::array<AEGP_DynStreamFlags,records::parameter_count(records::Kind::particle)> visibility_flags{};
int visibility_sets{};
AEGP_UtilitySuite6 utility{};
AEGP_MemorySuite1 memory{};
PF_ParamUtilsSuite3 param_utils{};
AEGP_KeyframeSuite5 keyframes{};
bool temporal_metadata_enabled{}, metadata_rate_keys{}, metadata_life_keys{}, metadata_bezier{}, metadata_life_expression{};unsigned metadata_epoch{};
std::array<std::u16string, kNativeBindingCapacity> expressions;
std::array<A_Boolean, kNativeBindingCapacity> expression_enabled{};
bool lazy_dependencies{};
std::array<bool,kNativeBindingCapacity> dependencies_evaluated{};
std::array<unsigned,kNativeBindingCapacity> dependency_generations{};
A_long nonfinite_alias=-1;
A_long fail_expression = -1;
bool disable_next_expression{};
AEGP_LayerSuite9 layers{};
AEGP_ItemSuite9 items{};
PF_InData renderer_data{};
A_Err acquire(const char* name, int32, const void** out) {
    ++suite_requests;
    if (std::strstr(name,"AEGP")) ++aegp_suite_requests;
    if (!std::strcmp(name, kAEGPPFInterfaceSuite)) *out = &pf;
    else if (!std::strcmp(name, kAEGPEffectSuite)) *out = &effect;
    else if (!std::strcmp(name, kAEGPStreamSuite)) *out = &stream;
    else if(dynamic_enabled && !std::strcmp(name,kAEGPDynamicStreamSuite))*out=&dynamic;
    else if (!std::strcmp(name, kAEGPUtilitySuite)) *out = &utility;
    else if (!std::strcmp(name, kAEGPLayerSuite)) *out = &layers;
    else if (!std::strcmp(name, kAEGPItemSuite)) *out = &items;
    else if (!std::strcmp(name, kAEGPMemorySuite)) *out = &memory;
    else if (temporal_metadata_enabled && !std::strcmp(name,kPFParamUtilsSuite)) *out=&param_utils;
    else if (temporal_metadata_enabled && !std::strcmp(name,kAEGPKeyframeSuite)) *out=&keyframes;
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
    PF_OutData output{}; return commit_native_graph_edit(&request, 701, &output);
}
void save_transform_expressions(const char* path) {
    std::ofstream output(path);output<<"[";bool first=true;unsigned count=0;
    for(std::size_t slot=0;slot<expressions.size();++slot) {
        const auto& expression=expressions[slot];
        if(expression.find(u"// Starfield Transform affine entry ")==std::u16string::npos)continue;
        if(!first)output<<",";first=false;++count;
        output<<"{\"stream\":"<<kNativeBindingFirstIndex+slot<<",\"expression\":\"";
        for(auto c:expression) {
            if(c==u'\n')output<<"\\n";
            else {if(c==u'\\' || c==u'\"')output<<"\\";output<<static_cast<char>(c);}
        }
        output<<"\"}";
    }
    output<<"]";
    check(bool(output) && count==12,"complete Transform expressions recorded for real UUID lookup execution");
}
}
double evaluated_binding(A_long index, A_long time) {
    const auto slot=index-kNativeBindingFirstIndex;
    if(lazy_dependencies && !dependencies_evaluated[slot]) {
        dependencies_evaluated[slot]=true;++dependency_generations[slot];
    }
    if(index==nonfinite_alias)return std::numeric_limits<double>::quiet_NaN();
    const auto& expr = expressions[index-kNativeBindingFirstIndex];
    if (expr.empty() || !expression_enabled[index-kNativeBindingFirstIndex]) return fixtures[0].values[index].one_d;
    const auto identity=[&](records::Kind kind,unsigned value) {
        const auto number=[](unsigned n){const auto s=std::to_string(n);return std::u16string(s.begin(),s.end());};
        return expr.find(u"fx.param("+number(records::uuid_first_index(kind)+7)+u").value === "+number(value))!=std::u16string::npos;
    };
    std::size_t id = identity(records::Kind::transform,5) ? 4 : identity(records::Kind::emitter,1) ? 1 : identity(records::Kind::particle,2) ? 2 : 3;
    if(id==4 && expr.find(u"// Starfield Transform affine entry ")!=std::u16string::npos) {
        const std::u16string marker=u"// Starfield Transform affine entry ";
        const auto first=expr.find(marker)+marker.size(),last=expr.find(u"\n",first);
        std::string digits;for(auto c:expr.substr(first,last-first))digits+=char(c);
        const auto entry=std::stoi(digits);
        if(fixtures[4].values[1].layer_id==0) {
            const transform_binding::PixelAffine value{1,0,0,960,0,1,0,540,0,0,1,0};return value[entry];
        }
        auto sampled=null_world;sampled[3]+=time;
        auto value=transform_binding::relative_anchor_affine(sampled,owner_world,null_anchor);
        return value.has_value()?value.value()[entry]:kNativeBindingUnavailable;
    }
    const std::u16string needle=u"result = fx.param(";
    auto from=expr.find(needle)+needle.size(); auto to=expr.find(u")",from);
    const auto digits=expr.substr(from,to-from);
    std::string source_digits;for(auto digit:digits) source_digits.push_back(static_cast<char>(digit));
    int source=std::stoi(source_digits); auto& value=fixtures[id].values[source];
    auto component_marker=expr.find(u").value[",to);
    if(component_marker!=std::u16string::npos) {
        int component=expr[component_marker+8]-u'0';
        if(id==1 && source==4) return component ? value.two_d.y : value.two_d.x + time;
        if(id==4 && source==2)return component ? value.two_d.y : value.two_d.x;
        return component==0 ? value.color.redF*(1.0-time/48.0) : component==1 ? value.color.greenF : value.color.blueF;
    }
    if(cloud_animation_test && id==2 && source>=records::particle_layout::cloud_circles && source<=records::particle_layout::cloud_density)
        return value.one_d+time/24.0;
    return (id==2 && source==5) || (id==3 && source==1) ? value.one_d+time :
        (id==2 && source==8) ? 100.0-time : (id==2 && source==2) ? 2.0+time/24.0 : value.one_d;
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
    for(std::size_t i=0;i<kMotionParameterIds.size();++i) {
        const auto index=kMotionParameterIds[i];
        main.values[index].one_d=motion_popup(i)?main.params[index].u.pd.value:main.params[index].u.fs_d.value;
    }
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
    particle.values[records::particle_layout::cloud_circles].one_d=10;
    particle.values[records::particle_layout::cloud_aspect].one_d=150;
    particle.values[records::particle_layout::cloud_density].one_d=66;
    particle.values[records::particle_layout::transfer].one_d=1;
    particle.values[records::particle_layout::texture_time].one_d=1;
    particle.values[records::particle_layout::texture_color].one_d=1;
    particle.values[records::particle_layout::texture_ratio].one_d=1;
    emitter.values[1].one_d = 1; emitter.values[2].one_d = 1; emitter.values[3].one_d = 30;
    emitter.values[4].two_d = {960, 540}; emitter.values[6].one_d = 100;
    for (int i : {8, 9, 10, 20, 22, 28, 29}) emitter.values[i].one_d = 100;
    emitter.values[15].one_d = 1; emitter.values[19].one_d = 60;
    particle.values[1].one_d=1;particle.values[2].one_d=2;
    particle.values[5].one_d=10;particle.values[6].one_d=10;
    particle.values[8].one_d=100;particle.values[10].one_d=1;
    particle.values[11].color={1,1,1,1};particle.values[records::particle_layout::up_axis].one_d=3;
    particle.values[records::particle_layout::size_over_life].one_d=100;particle.values[records::particle_layout::opacity_over_life].one_d=100;
    particle.values[12].one_d=2;particle.values[13].one_d=0;particle.values[15].one_d=100;
    particle.values[14].color={1,1,1,1};particle.values[16].color={1,1,1,1};
    particle.values[records::particle_layout::orient].one_d=1;particle.values[records::particle_layout::random_limit].one_d=1;
    particle.values[records::particle_layout::anchor_x].one_d=50;particle.values[records::particle_layout::anchor_y].one_d=50;
    main.values[kTimeSamplingHzId].one_d=1;main.values[kAccelerationId].one_d=1;main.values[kAccelerationId].one_d=1;
    connection(1, records::Kind::emitter, 2, 11);
    connection(2, records::Kind::particle, 4, 12);
    connection(3, records::Kind::force, 255, 13);

    utility.AEGP_RegisterWithAEGP = [](AEGP_GlobalRefcon, const A_char*, AEGP_PluginID* id)->A_Err { *id = 701; return 0; };
    layers.AEGP_GetLayerSourceItem = [](AEGP_LayerH, AEGP_ItemH* item)->A_Err { *item = reinterpret_cast<AEGP_ItemH>(1); return 0; };
    layers.AEGP_GetLayerCurrentTime = [](AEGP_LayerH, AEGP_LTimeMode, A_Time* time)->A_Err { *time = {0, 24}; return 0; };
    items.AEGP_GetItemDimensions = [](AEGP_ItemH, A_long* width, A_long* height)->A_Err { *width = 1920; *height = 1080; return 0; };
    items.AEGP_GetItemPixelAspectRatio = [](AEGP_ItemH, A_Ratio* aspect)->A_Err { *aspect = {1, 1}; return 0; };
    pf.AEGP_GetEffectLayer = [](PF_ProgPtr, AEGP_LayerH* layer)->A_Err { *layer = reinterpret_cast<AEGP_LayerH>(1); return 0; };
    pf.AEGP_GetNewEffectForEffect = [](AEGP_PluginID, PF_ProgPtr, AEGP_EffectRefH* ref)->A_Err { *ref = effect_ref(0); return 0; };
    effect.AEGP_GetLayerNumEffects = [](AEGP_LayerH, A_long* count)->A_Err { *count = transform_present?5:4; return 0; };
    effect.AEGP_GetLayerEffectByIndex = [](AEGP_PluginID, AEGP_LayerH, A_long i, AEGP_EffectRefH* ref)->A_Err { *ref = effect_ref(i); return 0; };
    effect.AEGP_GetInstalledKeyFromLayerEffect = [](AEGP_EffectRefH ref, AEGP_InstalledEffectKey* key)->A_Err { *key = static_cast<AEGP_InstalledEffectKey>(effect_index(ref)); return 0; };
    effect.AEGP_GetEffectMatchName = [](AEGP_InstalledEffectKey key, A_char* name)->A_Err { std::strcpy(name, fixtures[key].name); return 0; };
    effect.AEGP_DisposeEffect = [](AEGP_EffectRefH)->A_Err { return 0; };
    effect.AEGP_EffectCallGeneric = [](AEGP_PluginID, AEGP_EffectRefH, const A_Time*, PF_Cmd, void*)->A_Err {
        ++calls; check(false, "cross-effect generic calls are forbidden in native edits");
        return PF_Err_BAD_CALLBACK_PARAM;
    };
    stream.AEGP_GetNewEffectStreamByIndex = [](AEGP_PluginID, AEGP_EffectRefH ref, A_long index, AEGP_StreamRefH* out)->A_Err {
        if(effect_index(ref)==4 && index>=15 && index<=26)++synthetic_native_reads;
        ++live_refs; *out = reinterpret_cast<AEGP_StreamRefH>(new Ref{effect_index(ref), index}); return 0;
    };
    stream.AEGP_GetStreamType = [](AEGP_StreamRefH ref, AEGP_StreamType* type)->A_Err {
        const auto& key = *reinterpret_cast<Ref*>(ref);
        if (key.index == wrong_type) { wrong_type = -1; *type = AEGP_StreamType_COLOR; }
        else if((key.effect==4 && key.index==1) || (key.effect==2 &&
                (key.index==records::particle_layout::texture_front || key.index==records::particle_layout::texture_back)))
            *type=AEGP_StreamType_LAYER_ID;
        else if(key.effect==5)*type=AEGP_StreamType_ThreeD_SPATIAL;
        else *type = key.index == kGraphParameterId ? AEGP_StreamType_ARB : AEGP_StreamType_OneD;
        return 0;
    };
    stream.AEGP_GetNewStreamValue = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_LTimeMode, const A_Time* time, A_Boolean pre_expression, AEGP_StreamValue2* out)->A_Err {
        auto& key = *reinterpret_cast<Ref*>(ref);
        if (!key.effect && key.index == fail_read) { fail_read = -1; return PF_Err_BAD_CALLBACK_PARAM; }
        if (!key.effect && !pre_expression && key.index == fail_evaluated_read) {
            fail_evaluated_read=-1;return PF_Err_BAD_CALLBACK_PARAM;
        }
        out->streamH = ref;
        if(key.effect==5) {out->val.three_d={null_anchor.x,null_anchor.y,null_anchor.z};return 0;}
        out->val = fixtures[key.effect].values[key.index];
        if (!key.effect && key.index>=kNativeBindingFirstIndex && !pre_expression)
            out->val.one_d=evaluated_binding(key.index,time->value);
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
    stream.AEGP_GetExpressionState = [](AEGP_PluginID, AEGP_StreamRefH ref, A_Boolean* enabled)->A_Err {
        auto* key=reinterpret_cast<Ref*>(ref);
        *enabled = key->effect ? FALSE : expression_enabled[key->index - kNativeBindingFirstIndex]; return 0;
    };
    stream.AEGP_GetExpression = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_MemHandle* result)->A_Err {
        *result = reinterpret_cast<AEGP_MemHandle>(new std::u16string(expressions[reinterpret_cast<Ref*>(ref)->index - kNativeBindingFirstIndex])); return 0;
    };
    memory.AEGP_LockMemHandle = [](AEGP_MemHandle ref, void** result)->A_Err {
        *result = const_cast<char16_t*>(reinterpret_cast<std::u16string*>(ref)->c_str()); return 0;
    };
    memory.AEGP_UnlockMemHandle = [](AEGP_MemHandle)->A_Err {return 0;};
    memory.AEGP_FreeMemHandle = [](AEGP_MemHandle ref)->A_Err {delete reinterpret_cast<std::u16string*>(ref);return 0;};
    stream.AEGP_SetExpression = [](AEGP_PluginID, AEGP_StreamRefH ref, const A_UTF16Char* text)->A_Err {
        auto index = reinterpret_cast<Ref*>(ref)->index - kNativeBindingFirstIndex;
        if (index == fail_expression) {fail_expression = -1; return PF_Err_BAD_CALLBACK_PARAM;}
        expressions[index] = reinterpret_cast<const char16_t*>(text); return 0;
    };
    stream.AEGP_SetExpressionState = [](AEGP_PluginID, AEGP_StreamRefH ref, A_Boolean enabled)->A_Err {
        if(enabled && disable_next_expression) { enabled=FALSE;disable_next_expression=false; }
        expression_enabled[reinterpret_cast<Ref*>(ref)->index - kNativeBindingFirstIndex] = enabled;return 0;
    };
    check(register_node_graph_sync(&renderer_data) == 0, "node adapter registers its own AEGP ID");
    {
        namespace layout=records::particle_layout;
        dynamic_enabled=true;
        dynamic.AEGP_GetDynamicStreamFlags=[](AEGP_StreamRefH ref,AEGP_DynStreamFlags* flags)->A_Err {
            const auto& key=*reinterpret_cast<Ref*>(ref);check(key.effect==2,"visibility operates on the current Particle effect only");
            *flags=visibility_flags[key.index];return 0;
        };
        dynamic.AEGP_SetDynamicStreamFlag=[](AEGP_StreamRefH ref,AEGP_DynStreamFlags flag,A_Boolean undoable,A_Boolean hidden)->A_Err {
            check(flag==AEGP_DynStreamFlag_HIDDEN && !undoable,"UI visibility changes only non-undoable HIDDEN flags");
            auto& flags=visibility_flags[reinterpret_cast<Ref*>(ref)->index];
            flags=hidden?flags|flag:flags&~flag;++visibility_sets;return 0;
        };
        const auto original_pf=pf.AEGP_GetNewEffectForEffect;
        pf.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr,AEGP_EffectRefH* ref)->A_Err{*ref=effect_ref(2);return 0;};
        std::array<PF_ParamDef,records::parameter_count(records::Kind::particle)> ui_values{};
        std::array<PF_ParamDef*,ui_values.size()> ui_params{};
        for(unsigned i=0;i<ui_values.size();++i)ui_params[i]=&ui_values[i];
        ui_values[layout::shape].param_type=ui_values[layout::color_mode].param_type=PF_Param_POPUP;
        ui_values[layout::shape].u.pd.value=ui_values[layout::color_mode].u.pd.value=1;
        const int before_writes=sets;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 &&
              (visibility_flags[layout::size_y]&AEGP_DynStreamFlag_HIDDEN) &&
              (visibility_flags[layout::gradient]&AEGP_DynStreamFlag_HIDDEN),"Circle and solid mode hide Size Y and gradient through actual stream flags");
        const int previous_sets=visibility_sets;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 && visibility_sets==previous_sets,"unchanged visibility performs no setter calls");
        ui_values[layout::shape].u.pd.value=2;ui_values[layout::color_mode].u.pd.value=4;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 &&
              !(visibility_flags[layout::size_y]&AEGP_DynStreamFlag_HIDDEN) &&
              !(visibility_flags[layout::gradient]&AEGP_DynStreamFlag_HIDDEN),"rectangle and gradient modes reveal controls through actual stream flags");
        check(sets==before_writes && acquisitions==0 && live_refs==0,"UI visibility authors no parameter values and releases suites and stream references");
        ui_values[layout::shape].u.pd.value=3;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 &&
              !(visibility_flags[layout::cloud]&AEGP_DynStreamFlag_HIDDEN) &&
              !(visibility_flags[layout::cloud_density]&AEGP_DynStreamFlag_HIDDEN) &&
              (visibility_flags[layout::size_y]&AEGP_DynStreamFlag_HIDDEN),"Cloud reveals its group and round-member controls only");
        ui_values[layout::shape].u.pd.value=1;
        ui_values[records::sync_guard_index(records::Kind::particle)].param_type=PF_Param_FLOAT_SLIDER;
        ui_values[records::sync_guard_index(records::Kind::particle)].u.fs_d.value=1;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 &&
              !(visibility_flags[layout::cloud_density]&AEGP_DynStreamFlag_HIDDEN),"CEP sync guard temporarily exposes conditional Cloud controls");
        dynamic_enabled=false;pf.AEGP_GetNewEffectForEffect=original_pf;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 && acquisitions==0,"unavailable optional visibility suite cannot reject effect loading");
    }
    check(graph_carrier_plugin_id() == 0, "renderer registration is absent in native edit fixture");
#if defined(STARFIELD_CLOUD_CALLBACK_TEST)
    {
        namespace layout=records::particle_layout;using namespace core::graph_keys;
        PF_InData particle_data=renderer_data;particle_data.effect_ref=reinterpret_cast<PF_ProgPtr>(3);
        particle_data.num_params=static_cast<A_long>(particle.params.size());
        std::vector<PF_ParamDef*> pp;for(auto& p:particle.params)pp.push_back(&p);
        particle.params[layout::cloud_circles].u.fs_d.value=10;
        particle.params[layout::cloud_aspect].u.fs_d.value=150;
        particle.params[layout::cloud_density].u.fs_d.value=1000;
        particle.params[layout::shape].param_type=PF_Param_POPUP;particle.params[layout::shape].u.pd.value=3;
        PF_UserChangedParamExtra cloud_changed{};cloud_changed.param_index=layout::cloud_density;PF_OutData cloud_out{};
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&cloud_changed)==0 &&
              particle.params[layout::cloud_enabled].u.fs_d.value==1 &&
              (particle.params[layout::cloud_enabled].uu.change_flags&PF_ChangeFlag_CHANGED_VALUE) &&
              std::get<double>(parameter(saved_graph(),kParticleNode,kCloudDensity))==1000,
              "actual Particle USER_CHANGED persists activation only after the atomic Cloud graph commit");
        particle.params[layout::cloud_enabled].u.fs_d.value=0;
        particle.params[layout::cloud_enabled].uu.change_flags=0;
        fail_set=kGraphParameterId;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&cloud_changed)!=0 &&
              particle.params[layout::cloud_enabled].u.fs_d.value==0 && particle.params[layout::cloud_enabled].uu.change_flags==0,
              "failed actual native callback leaves the legacy activation field untouched");
        cloud_changed.param_index=layout::shape;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&cloud_changed)==0 &&
              particle.params[layout::cloud_enabled].u.fs_d.value==1 &&
              std::get<std::uint32_t>(parameter(saved_graph(),kParticleNode,kParticleShape))==2,
              "explicit native Shape=Cloud activates the configurable group");
        particle.params[records::sync_guard_index(records::Kind::particle)].u.fs_d.value=1;
        const auto before=sets;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&cloud_changed)==0 && sets==before,
              "actual Particle CEP guard suppresses implicit Cloud activation publication");
        check(live_refs==0 && acquisitions==0,"actual Cloud callbacks release every acquired host reference");
        dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));
        for(auto& p:main.params)if(p.param_type==PF_Param_ARBITRARY_DATA) {
            if(handles.contains(p.u.arb_d.value))dispose(p.u.arb_d.value);
            if(handles.contains(p.u.arb_d.dephault))dispose(p.u.arb_d.dephault);
        }
        check(handles.empty(),"Cloud callback graph handles released");
        std::printf("Cloud callbacks: %d checks, %d failures\n",checks,failures);
        return failures?1:0;
    }
#endif
    PF_InData node_data = renderer_data; node_data.effect_ref = reinterpret_cast<PF_ProgPtr>(2);
    node_data.num_params = static_cast<A_long>(emitter.params.size()); node_data.downsample_x = {1, 4}; node_data.downsample_y = {1, 4};
    std::vector<PF_ParamDef*> pointers; for (auto& param : emitter.params) pointers.push_back(&param);
    emitter.params[4].param_type = PF_Param_POINT; emitter.params[4].u.td.x_value = 400 << 16; emitter.params[4].u.td.y_value = 600 << 16;
    emitter.params[4].uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    PF_UserChangedParamExtra changed{}; changed.param_index = 4; PF_OutData output{};
    const auto first_edit_error=sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed);
    if(first_edit_error)std::printf("First native edit: %d %s\n",first_edit_error,output.return_msg);
    check(first_edit_error == 0, "native Origin commits with saved node value still old");
    if(first_edit_error) {
        std::vector<PF_ParamDef*> renderer_params;for(auto& p:main.params)renderer_params.push_back(&p);
        core::Graph candidate;bool found{};const auto error=compile_native_node_graph(&renderer_data,renderer_params.data(),candidate,found,701);
        const auto validation=core::validate_graph(candidate,core::particle_node_registry());
        std::printf("Compiler diagnostic: error %d nodes %zu edges %zu registry %s\n",error,candidate.nodes.size(),candidate.edges.size(),core::describe(validation.error.code));
        for(const auto& node:candidate.nodes)std::printf("  %s parameters %zu\n",node.type_key.c_str(),node.parameters.size());
        return 1;
    }
    auto graph = saved_graph(); auto origin = std::get<core::Vec3>(parameter(graph, core::graph_keys::kEmitterNode, core::graph_keys::kEmitterOrigin));
    check(std::abs(origin.x - 640.0 / 1080) < 1e-9 && std::abs(origin.y + 1860.0 / 1080) < 1e-9, "callback Origin beats delayed old stream with quarter normalization");
    check(emitter.values[4].two_d.x == 960 && emitter.values[4].two_d.y == 540, "source remains owned by host edit");
    check((output.out_flags & PF_OutFlag_FORCE_RERENDER) != 0, "native callback requests rerender");
    check(std::get<std::uint32_t>(parameter(graph, core::graph_keys::kOutputNode, core::graph_keys::kParticleCount)) == 1000000, "main cap comes from AEGP on node-owned publication path");
    check(std::get<double>(parameter(graph, core::graph_keys::kOutputNode, core::graph_keys::kPreviewChance)) == 100, "main preview comes from AEGP on node-owned publication path");
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
    const int before_sets = sets;
    emitter.params[records::sync_guard_index(records::Kind::emitter)].u.fs_d.value = 1;
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0 && sets == before_sets, "CEP guard skips native publication");
    emitter.params[records::sync_guard_index(records::Kind::emitter)].u.fs_d.value = 0;
    changed.param_index = records::layout_x_index(records::Kind::emitter);
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0 && sets == before_sets, "metadata awaits CEP transaction");
    auto life = edit(1, 2, 9); check(direct_edit(life) == 0 && life.accepted, "Particle Life publishes independently");
    particle.values[2].one_d = 9; // Host saves Life after its native callback returns.
    auto local_edit = edit(1, 5, 15); PF_OutData local_output{};
    check(commit_native_graph_edit(&local_edit, 701, &local_output) == 0 && local_edit.accepted,
          "publication needs no renderer callback or registered main AEGP ID");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kParticleLifetimeSeconds)) == 9, "Particle lifetime saved");
    auto opacity = edit(1, 8, 25); check(direct_edit(opacity) == 0, "Particle opacity publishes");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kOpacityStart)) == .25, "percent opacity normalized");
    auto color = edit(1, 11, 0); color.value_kind = node_sync::ValueKind::color; color.value = {.2, .3, .4, 1};
    check(direct_edit(color) == 0, "color publishes with delayed stream");
    check(std::get<core::Vec3>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kColorStart)).x == .2, "color value replaces only selected color");
    {
        namespace layout=records::particle_layout;
        auto palette=gradient_editor::preset(2);
        palette.stops[0].position=.1;palette.stops[palette.count-1].position=.9;
        auto bank=edit(1,layout::gradient,palette.count);
        bank.additional_fields[bank.additional_count++]={layout::gradient,node_sync::ValueKind::scalar,{double(palette.count),0,0,0}};
        for(unsigned i=0;i<8;++i) {
            const auto stop=i<palette.count?palette.stops[i]:core::ColorStop{1,{1,1,1}};
            bank.additional_fields[bank.additional_count++]={layout::gradient_first+2*static_cast<A_long>(i),
                node_sync::ValueKind::scalar,{stop.position*100,0,0,0}};
            bank.additional_fields[bank.additional_count++]={layout::gradient_first+2*static_cast<A_long>(i)+1,
                node_sync::ValueKind::color,{stop.color.x,stop.color.y,stop.color.z,1}};
        }
        bank.additional_fields[bank.additional_count++]={layout::gradient_interpolation,node_sync::ValueKind::scalar,{0,0,0,0}};
        const auto previous_bytes=handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
        const double previous_revision=main.values[kGraphRevisionId].one_d;
        auto rejected=bank;fail_set=kGraphParameterId;
        check(direct_edit(rejected)!=0 && !rejected.accepted &&
              main.values[kGraphRevisionId].one_d==previous_revision &&
              handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes==previous_bytes,
              "whole-gradient failed publication preserves previous graph and revision");
        auto duplicate=bank;duplicate.additional_fields[1].index=layout::gradient;
        check(!node_sync::valid_edit(duplicate),"duplicate gradient override rejected before publication");
        auto nonfinite=bank;nonfinite.additional_fields[3].value[0]=std::numeric_limits<double>::quiet_NaN();
        check(!node_sync::valid_edit(nonfinite),"nonfinite bank override rejected before publication");
        auto wrong_bank=bank;wrong_bank.additional_fields[2].kind=node_sync::ValueKind::scalar;
        check(direct_edit(wrong_bank)!=0 && !wrong_bank.accepted,"wrong typed gradient bank cannot publish");
        check(particle.values[layout::gradient].one_d==2 && direct_edit(bank)==0 && bank.accepted,
              "whole-gradient publication uses callback bank while AEGP still holds two old stops");
        core::ColorGradient decoded{};
        const auto published=saved_graph();
        const auto& bytes=std::get<core::OpaqueBytes>(parameter(published,core::graph_keys::kParticleNode,core::graph_keys::kColorGradient));
        check(core::decode_color_gradient(bytes,decoded) && decoded.count==palette.count,
              "whole-gradient graph contains all five new stops atomically");
        for(unsigned i=0;i<palette.count;++i)check(
            decoded.stops[i].position==palette.stops[i].position &&
            decoded.stops[i].color.x==palette.stops[i].color.x &&
            decoded.stops[i].color.y==palette.stops[i].color.y &&
            decoded.stops[i].color.z==palette.stops[i].color.z,"published bank retains new position and RGB together");
        std::array<PF_ParamDef,82> callback{};std::array<PF_ParamDef*,82> callback_params{};
        for(unsigned i=0;i<82;++i)callback_params[i]=&callback[i];
        for(const auto& field:bank.additional_fields) {
            auto& def=callback[field.index];
            if(field.kind==node_sync::ValueKind::color) {
                def.param_type=PF_Param_COLOR;
                def.u.cd.value={255,static_cast<A_u_char>(std::lround(field.value[0]*255)),
                    static_cast<A_u_char>(std::lround(field.value[1]*255)),static_cast<A_u_char>(std::lround(field.value[2]*255))};
            } else {def.param_type=PF_Param_FLOAT_SLIDER;def.u.fs_d.value=field.value[0];}
        }
        auto partial_count=edit(1,layout::gradient,5);
        check(direct_edit(partial_count)!=0 && partial_count.stream_index==layout::gradient_first+4,
              "single changed count reproduces mixed old-bank rejection with exact bad position index");
        for(A_long index=layout::gradient;index<layout::gradient_first+16;++index) {
            auto followup=edit(1,index,index==layout::gradient?5:0);
            check(node_sync::capture_gradient_bank(callback_params.data(),followup) && followup.additional_count==18,
                  "every gradient callback captures all stops and interpolation atomically");
            check(direct_edit(followup)==0 && followup.accepted,"full callback bank publishes while AEGP still exposes the old two-stop bank");
        }
        auto incomplete=edit(1,layout::gradient,5);callback_params[layout::gradient_first+1]=nullptr;
        check(!node_sync::capture_gradient_bank(callback_params.data(),incomplete) && incomplete.additional_count==0,
              "incomplete callback bank cannot partially modify the edit request");
        check(!node_sync::gradient_bank_parameter(0,layout::gradient) && !node_sync::gradient_bank_parameter(3,layout::gradient),
              "Emitter and Force controls are never classified as Particle gradient fields");
        auto reset=edit(1,layout::gradient,2);
        check(direct_edit(reset)==0,"fixture restores previous bank without callback overrides");
    }
    auto gravity = edit(3, 1, 1080); check(direct_edit(gravity) == 0, "Force Gravity publishes");
    check(std::get<core::Vec3>(parameter(saved_graph(), core::graph_keys::kForceNode, core::graph_keys::kGravity)).y == -1, "Gravity units remain intact");
    for (int count : {records::particle_layout::size_curve, records::particle_layout::opacity_curve}) {
        particle.values[count].one_d = 2; particle.values[count + 1].one_d = 0;
        particle.values[count + 2].one_d = 100; particle.values[count + 3].one_d = 1; particle.values[count + 4].one_d = 0;
    }
    auto curve = edit(1, records::particle_layout::size_curve+4, 60); check(direct_edit(curve) == 0, "single curve bank value commits");
    graph = saved_graph();
    core::AgeCurve size_curve{}, opacity_curve{};
    const bool size_ok = core::decode_age_curve(std::get<core::OpaqueBytes>(parameter(graph, core::graph_keys::kParticleNode, core::graph_keys::kSizeOverLifeCurve)), size_curve, 0, 100);
    const bool opacity_ok = core::decode_age_curve(std::get<core::OpaqueBytes>(parameter(graph, core::graph_keys::kParticleNode, core::graph_keys::kOpacityOverLifeCurve)), opacity_curve, 0, 100);
    check(size_ok && size_curve.points[1].value == 60 && opacity_ok && opacity_curve.points[1].value == 0, "other Over Life curve preserved");
    const double old_revision = main.values[kGraphRevisionId].one_d;
    const auto old_bytes = handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
    const auto old_handle_count = handles.size();
    auto missing_control = edit(1, 5, 20); fail_read = kMaxParticlesId;
    check(direct_edit(missing_control) != 0 && !missing_control.accepted &&
          missing_control.stage == node_sync::Stage::controls && missing_control.stream_index == kMaxParticlesId,
          "missing main control reports stage and stream index");
    auto missing_context = edit(1, 5, 20); missing_context.layer = nullptr;
    check(direct_edit(missing_context) != 0 && missing_context.stage == node_sync::Stage::context,
          "missing borrowed context rejected with context diagnostic");
    auto bad_override = edit(1, 5, 20); bad_override.value_kind = node_sync::ValueKind::color;
    check(direct_edit(bad_override) != 0 && bad_override.stage == node_sync::Stage::compile &&
          bad_override.stream_index == 5, "node compiler identifies wrong edited stream type");
    auto typed_receipt = edit(1, 5, 20); wrong_type = kGraphChecksumHighId;
    const int sets_before_type_check = sets;
    check(direct_edit(typed_receipt) != 0 && typed_receipt.stage == node_sync::Stage::capture &&
          typed_receipt.stream_index == kGraphChecksumHighId && sets == sets_before_type_check,
          "non-scalar receipt rejected before reading union or writing streams");
    auto invalid = edit(1, 5, 20); invalid.uuid[7] = 500;
    check(direct_edit(invalid) != 0 && !invalid.accepted && main.values[kGraphRevisionId].one_d == old_revision, "missing UUID rejected before publish");
    auto failed = edit(1, 5, 20); fail_set = kGraphParameterId;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::publish_graph && failed.stream_index == kGraphParameterId, "failed graph write restores revision and reports graph phase");
    check(handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes == old_bytes, "failed write retains old graph");
    failed = edit(1, 5, 20); fail_set = kGraphChecksumLowId;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::publish_scalars && failed.stream_index == kGraphChecksumLowId,
          "failed metadata write restores revision and reports scalar phase");
    failed = edit(1, 5, 20); corrupt_receipt = true;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::verify_scalars && failed.stream_index == kGraphChecksumHighId,
          "changed integer receipt fails exact readback and restores graph");
    failed = edit(1, 5, 20); ignore_graph = true;
    check(direct_edit(failed) != 0 && !failed.accepted && main.values[kGraphRevisionId].one_d == old_revision &&
          failed.stage == node_sync::Stage::verify_graph && failed.stream_index == kGraphParameterId,
          "silent ignored graph write reports graph readback and rolls back");
    check(handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes == old_bytes, "readback rejection restores graph");
    changed.param_index = 3; fail_set = kGraphParameterId; output = {};
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) != 0 &&
          !(output.out_flags & PF_OutFlag_FORCE_RERENDER) && std::strstr(output.return_msg, "publish graph"),
          "failed native publication reports actual write stage and cannot acknowledge edit");
    check(calls == 0, "all native edits work without entering the renderer effect selector");
    check(handles.size() == old_handle_count && live_refs == 0 && acquisitions == 0, "all publication handles/suites balanced");
    main.values[kGraphRevisionId].one_d = 16777214;
    auto max_receipt = edit(1, 5, 20);
    check(direct_edit(max_receipt) == 0 && max_receipt.accepted && max_receipt.revision == 16777215 &&
          main.values[kGraphRevisionId].one_d == 16777215,
          "largest integer revision survives host float storage and exact verification");
    auto animated_graph = saved_graph();
    check(std::count_if(expressions.begin(), expressions.end(), [](const auto& text) { return !text.empty(); }) == 79,
          "all emitter particle force scalar and vector/color components have bindings");
    const auto expression_baseline = expressions;
    {
        std::ofstream generated("artifacts/native-animation-expressions.json");
        generated << "[";
        bool first=true;
        for(const auto& expression:expressions) if(!expression.empty()) {
            if(!first) generated << ","; first=false;
            generated << "\"";
            for(auto c:expression) {
                if(c==u'\n') generated << "\\n";
                else {if(c==u'\\' || c==u'\"') generated << "\\"; generated << static_cast<char>(c);}
            }
            generated << "\"";
        }
        generated << "]";
        check(bool(generated),"actual generated expressions recorded under artifacts for syntax/identity checks");
    }
    for (A_long i=0; i<67; ++i) expressions[i] = u"0"; // Force a binding rewrite, then fail graph publication.
    const auto before_failed_binding = expressions;
    fail_set = kGraphParameterId;
    auto binding_failure = edit(1, 5, 22);
    main.values[kGraphRevisionId].one_d = 100; // Avoid revision exhaustion for this transaction.
    check(direct_edit(binding_failure) != 0 && expressions == before_failed_binding,
          "graph publication failure restores all changed binding expressions");
    expressions = expression_baseline;
    expressions[0]=u"0";
    const auto disabled_expression_baseline=expressions;
    disable_next_expression=true;
    auto rejected_binding=edit(1,2,23);
    check(direct_edit(rejected_binding)==PF_Err_BAD_CALLBACK_PARAM &&
          rejected_binding.stage==node_sync::Stage::animation_bindings && rejected_binding.stream_index==kNativeBindingFirstIndex,
          "AE parser disabling a binding is rejected at the exact stream before graph publication");
    check(rejected_binding.binding_stage && std::strcmp(rejected_binding.binding_stage,"verify expression enabled")==0 &&
          rejected_binding.binding_parameter>0,
          "direct native edit retains the failing binding operation and source parameter");
    check(expressions==disabled_expression_baseline && expression_enabled[0],
          "disabled-binding rejection restores old expressions and enabled states");
    expressions=expression_baseline;
    // Earlier publication checks intentionally place the emitter outside the frame.
    // Keep the pixel regression visible while sampling the same native bindings.
    emitter.values[4].two_d = {960, 540};
    emitter.values[1].one_d = 1;
    emitter.values[6].one_d = 0;
    renderer_data.inter.checkout_param = [](PF_ProgPtr, PF_ParamIndex index, A_long time, A_long, A_u_long, PF_ParamDef* output)->PF_Err {
        if(index<kNativeBindingFirstIndex || index>=kNativeBindingFirstIndex+kNativeBindingCapacity) {
            *output=fixtures[0].params.at(index);
            if(index==kGraphParameterId) output->u.arb_d.value=reinterpret_cast<PF_ArbitraryH>(fixtures[0].values[index].arbH);
            if(index==kControlSourceId) output->u.pd.value=static_cast<A_long>(fixtures[0].values[index].one_d);
            return 0;
        }
        *output = {}; output->param_type=PF_Param_FLOAT_SLIDER;
        output->u.fs_d.value=evaluated_binding(index,time);
        return 0;
    };
    renderer_data.inter.checkin_param = [](PF_ProgPtr, PF_ParamDef*)->PF_Err {return 0;};
    // SmartFX has no delivered params[]; callbacks still expose registered streams.
    renderer_data.num_params = 0;
    auto smartfx_graph = animated_graph;
    check(sample_native_node_animation(&renderer_data,smartfx_graph,1920,1080)==0,
          "SmartFX samples registered animation streams without a delivered parameter array");
    const auto suites_before=aegp_suite_requests;
    NativeAnimationPlan animation_plan(animated_graph,1920,1080,1);
    check(animation_plan.valid(),"prepared animation plan decodes bindings once");
    std::vector<std::byte> first_animated_pixels;
    for (auto time : {0,24,12,0}) {
        auto sampled=animated_graph; renderer_data.current_time=time;
        check(sample_native_node_animation(&renderer_data,sampled,1920,1080)==0,"owned inputs sample requested animation time");
        auto prepared=animated_graph;
        for(auto& node:prepared.nodes) if(node.type_key!=core::graph_keys::kOutputNode) {
            core::GraphNode value;A_long failed_stream=-1;
            check(animation_plan.sample(&renderer_data,node.id,value,&failed_stream)==0,"prepared plan samples one native node");
            node=std::move(value);
        }
        const auto expected_bytes=core::serialize_graph(sampled,core::particle_node_registry());
        const auto prepared_bytes=core::serialize_graph(prepared,core::particle_node_registry());
        check(expected_bytes.has_value() && prepared_bytes.has_value() && expected_bytes.value()==prepared_bytes.value(),
              "prepared node sampling matches full graph sampling exactly in both time directions");
        check(std::get<double>(parameter(sampled,core::graph_keys::kParticleNode,core::graph_keys::kSizeStart))==particle.values[5].one_d+time,
              "size samples forward intermediate and reverse times");
        const auto sampled_origin=std::get<core::Vec3>(parameter(sampled,core::graph_keys::kEmitterNode,core::graph_keys::kEmitterOrigin));
        check(std::abs(sampled_origin.x-(emitter.values[4].two_d.x+time-960)/1080)<1e-12,"animated point converts full-resolution pixels");
        check(std::abs(std::get<double>(parameter(sampled,core::graph_keys::kParticleNode,core::graph_keys::kOpacityStart))-(100.0-time)/100.0)<1e-12,
              "animated opacity converts percentage to multiplier");
        check(std::get<double>(parameter(sampled,core::graph_keys::kParticleNode,core::graph_keys::kParticleLifetimeSeconds))==2.0+time/24.0,
              "animated lifetime samples seconds");
        const auto animated_color=std::get<core::Vec3>(parameter(sampled,core::graph_keys::kParticleNode,core::graph_keys::kColorStart));
        check(animated_color.x==particle.values[11].color.redF*(1.0-time/48.0),"animated color samples RGB components");
        const auto animated_gravity=std::get<core::Vec3>(parameter(sampled,core::graph_keys::kForceNode,core::graph_keys::kGravity));
        check(std::abs(animated_gravity.y+(fixtures[3].values[1].one_d+time)/1080.0)<1e-12,"animated Force converts pixels to world units");
        std::shared_ptr<const core::Graph> owned_graph;
        PF_OutData render_output{};
        check(checkout_render_graph(&renderer_data,&render_output,owned_graph,nullptr,1920,1080)==0 && owned_graph,
              "SmartFX full graph checkout succeeds with zero delivered parameters");
        if(!owned_graph) continue;
        const auto checked_origin=std::get<core::Vec3>(parameter(*owned_graph,core::graph_keys::kEmitterNode,core::graph_keys::kEmitterOrigin));
        check(checked_origin.x==sampled_origin.x && checked_origin.y==sampled_origin.y && checked_origin.z==sampled_origin.z,
              "full SmartFX checkout retains the requested-time Origin XY value");
        core::RenderRequest request{};
        request.settings=core::validate_settings(core::Settings{});
        request.graph=owned_graph;
        request.frame={1920,1080,512,288,{0,0,512,288},{1,1},{1,24},core::PixelFormat::rgba8,
            core::ColorSpace::ae_working_space,core::AlphaMode::premultiplied,1,core::Quality::full};
        auto rendered=core::CpuParticleRenderer{}.render(request,core::NeverCancelled{});
        check(rendered.has_value(),"sampled native animation graph renders through the actual CPU backend");
        if(rendered.has_value()) {
            const auto& pixels=rendered.value().pixels;
            bool visible=false; for(std::size_t i=3;i<pixels.size();i+=4) visible |= pixels[i]!=std::byte{0};
            check(visible,"Origin XY animation does not zero output alpha");
            if(first_animated_pixels.empty()) first_animated_pixels=pixels;
            else check(time==0 ? pixels==first_animated_pixels : pixels!=first_animated_pixels,
                "animation changes actual pixels and reverse-order sampling repeats exactly");
        } else std::printf("Animated render detail: %s\n",rendered.error().detail);
    }
    check(aegp_suite_requests==suites_before && live_refs==0,"render sampling acquires no AEGP suites or references");
    auto invalid_record=animated_graph;
    for(auto& record:invalid_record.optional_records)
        if(record.size()>=12 && record[0]==std::byte{2} && record[1]==std::byte{0x80}) record[4]=std::byte{0};
    A_long failed_stream=-1;
    const char* failed_stage="";
    check(sample_native_node_animation(&renderer_data,invalid_record,1920,1080,&failed_stream,&failed_stage)==PF_Err_BAD_CALLBACK_PARAM &&
          failed_stream==-1 && std::strcmp(failed_stage,"binding record")==0,"malformed record is distinguished from a stream failure");
    auto absent_callbacks=renderer_data; absent_callbacks.inter.checkout_param=nullptr;
    auto callback_graph=animated_graph;
    check(sample_native_node_animation(&absent_callbacks,callback_graph,1920,1080,&failed_stream,&failed_stage)==PF_Err_BAD_CALLBACK_PARAM &&
          failed_stream==-1 && std::strcmp(failed_stage,"host callbacks")==0,"missing SmartFX callbacks report their actual phase");
    const auto successful_checkout=renderer_data.inter.checkout_param;
    renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex,A_long,A_long,A_u_long,PF_ParamDef*)->PF_Err {return PF_Err_INVALID_INDEX;};
    auto checkout_failure=animated_graph;
    check(sample_native_node_animation(&renderer_data,checkout_failure,1920,1080,&failed_stream,&failed_stage)==PF_Err_INVALID_INDEX &&
          failed_stream==kNativeBindingFirstIndex && std::strcmp(failed_stage,"parameter checkout")==0,
          "actual checkout errors survive independently of delivered parameter count");
    renderer_data.inter.checkout_param=successful_checkout;
    for(auto count:{1,static_cast<int>(kTotalEffectParameterCount+1)}) {
        renderer_data.num_params=count;
        auto count_graph=animated_graph;
        check(sample_native_node_animation(&renderer_data,count_graph,1920,1080)==0,"stream sampling also works with nonzero delivered counts");
    }
    renderer_data.num_params=0;
    std::vector<NativeOriginBinding> birth_bindings;
    check(read_native_origin_bindings(animated_graph,birth_bindings)==0 && birth_bindings.size()==1,
          "native emitter identity maps to its three owned Origin dependency streams");
    const auto birth_binding=birth_bindings.front();
    static NativeOriginBinding fixture_birth_binding;
    static A_long fixture_life_stream;
    static bool fixture_animated_rate=false;
    fixture_birth_binding=birth_binding;
    std::vector<NativeLifetimeBinding> life_bindings;
    check(read_native_lifetime_bindings(animated_graph,life_bindings)==0 && life_bindings.size()==1,
          "native Particle maps to its owned Life dependency stream");
    fixture_life_stream=life_bindings.front().stream;
    emitter.values[3].one_d=4;emitter.values[1].one_d=1;emitter.values[6].one_d=0;emitter.values[7].one_d=0;
    for(int index:{12,13,14,16,17,18,21,22,23,24}) emitter.values[index].one_d=0;
    particle.values[2].one_d=5;fixtures[3].values[1].one_d=0;
    renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex index,A_long time,A_long,A_u_long scale,PF_ParamDef* output)->PF_Err {
        const double seconds=double(time)/scale;
        *output={};output->param_type=PF_Param_FLOAT_SLIDER;
        if(index==fixture_birth_binding.x) output->u.fs_d.value=960+240*seconds*seconds;
        else if(index==fixture_birth_binding.y) output->u.fs_d.value=540;
        else if(index==fixture_birth_binding.z) output->u.fs_d.value=0;
        else if(index==fixture_birth_binding.rate && fixture_animated_rate) output->u.fs_d.value=seconds<1?4:0;
        else if(index==fixture_life_stream) output->u.fs_d.value=5;
        else output->u.fs_d.value=evaluated_binding(index,0);
        return 0;
    };
    auto birth_graph=animated_graph;
    for(auto& node:birth_graph.nodes) for(auto& p:node.parameters) {
        if(node.type_key==core::graph_keys::kEmitterNode && p.key==core::graph_keys::kBirthRate) p.value=4.0;
        if(node.type_key==core::graph_keys::kEmitterNode && p.key==core::graph_keys::kEmitterShape) p.value=std::uint32_t{0};
        if(node.type_key==core::graph_keys::kEmitterNode && p.key==core::graph_keys::kVelocity) p.value=core::Vec3{};
        if(node.type_key==core::graph_keys::kEmitterNode &&
           (p.key==core::graph_keys::kEmissionSpeed || p.key==core::graph_keys::kEmissionSpeedRandom || p.key==core::graph_keys::kVelocitySpread)) p.value=0.0;
        if(node.type_key==core::graph_keys::kParticleNode && p.key==core::graph_keys::kParticleLifetimeSeconds) p.value=5.0;
        if(node.type_key==core::graph_keys::kForceNode && p.key==core::graph_keys::kGravity) p.value=core::Vec3{};
    }
    const auto saved_before_history=handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
    std::vector<std::byte> history_pixels;
    for(auto time:{24,36,24}) {
        auto snapshot=birth_graph;renderer_data.current_time=time;PF_OutData birth_output{};
        check(capture_emitter_origin_history(&renderer_data,&birth_output,snapshot,1920,1080,core::NeverCancelled{})==0,
              "pre-render samples nonlinear native Origin XY at historical birth times");
        const auto population=core::evaluate_particle_graph(snapshot,{time,24},core::NeverCancelled{});
        check(population.has_value() && !population.value().particles.empty(),"frozen birth graph evaluates without AE callbacks");
        if(population.has_value()) for(const auto& p:population.value().particles)
            check(std::abs(p.position.x-(240*std::pow(double(p.id)/4,2))/1080)<1e-10,
                  "already emitted particles retain birth positions as the emitter moves");
        core::RenderRequest request{};request.settings=core::validate_settings(core::Settings{});
        request.graph=std::make_shared<const core::Graph>(snapshot);
        request.frame={1920,1080,512,288,{0,0,512,288},{time,24},{1,24},core::PixelFormat::rgba8,
            core::ColorSpace::ae_working_space,core::AlphaMode::premultiplied,1,core::Quality::full};
        const auto pixels=core::CpuParticleRenderer{}.render(request,core::NeverCancelled{});
        check(pixels.has_value(),"CPU backend renders native historical Origin graph");
        if(pixels.has_value()) {
            if(history_pixels.empty()) history_pixels=pixels.value().pixels;
            else if(time==24) check(history_pixels==pixels.value().pixels,"history pixels repeat exactly in reverse frame order");
        }
    }
    check(handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes==saved_before_history && aegp_suite_requests==suites_before,
          "birth history neither mutates saved project graph nor acquires AEGP suites");
    fixture_animated_rate=true;
    auto rate_history=birth_graph;renderer_data.current_time=48;PF_OutData rate_output{};
    check(capture_emitter_origin_history(&renderer_data,&rate_output,rate_history,1920,1080,core::NeverCancelled{})==0,
          "real owned PF checkout captures a rate that stops emitting");
    auto historical_rate=core::evaluate_particle_graph(rate_history,{2,1},core::NeverCancelled{},{1080,1});
    check(historical_rate.has_value() && historical_rate.value().particles.size()==5,
          "DLL renders historical survivors when current PPS is zero");
    PF_ArbitraryH forbidden_project_history=nullptr;
    check(create_graph_parameter(&renderer_data,rate_history,&forbidden_project_history)==PF_Err_BAD_CALLBACK_PARAM &&
          forbidden_project_history==nullptr,"transient temporal record cannot be persisted into AE project data");
    fixture_animated_rate=false;
    renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex,A_long,A_long,A_u_long,PF_ParamDef*)->PF_Err {return PF_Err_INVALID_INDEX;};
    auto failed_history=birth_graph;PF_OutData history_error{};
    check(capture_emitter_origin_history(&renderer_data,&history_error,failed_history,1920,1080,core::NeverCancelled{})==PF_Err_INVALID_INDEX &&
          std::strstr(history_error.return_msg,"checkout failed") && failed_history.optional_records.size()==birth_graph.optional_records.size(),
          "historical checkout rejection reports its cause and cannot publish partial history");
    struct CancelHistory final:core::Cancellation {bool is_cancelled() const noexcept override {return true;}} cancel_history;
    auto cancelled_history=birth_graph;PF_OutData cancel_output{};
    check(capture_emitter_origin_history(&renderer_data,&cancel_output,cancelled_history,1920,1080,cancel_history)==PF_Interrupt_CANCEL && cancel_output.return_msg[0]=='\0',
          "cancelled birth sampling returns the normal interrupt without an error dialog");
    renderer_data.inter.checkout_param=successful_checkout;
    renderer_data.inter.checkout_param=[](PF_ProgPtr, PF_ParamIndex, A_long, A_long, A_u_long, PF_ParamDef* output)->PF_Err {
        *output={};output->param_type=PF_Param_FLOAT_SLIDER;output->u.fs_d.value=kNativeBindingUnavailable;return 0;
    };
    A_long missing_stream=-1;
    auto unresolved_graph=animated_graph;
    check(sample_native_node_animation(&renderer_data,unresolved_graph,1920,1080,&missing_stream,&failed_stage)==PF_Err_BAD_CALLBACK_PARAM &&
          missing_stream==kNativeBindingFirstIndex && std::strcmp(failed_stage,"parameter value")==0,
          "unresolved expression output cannot silently zero particle settings");

    // Real UI metadata reader and native static preparation, using ParamUtils
    // tokens for the owned aliases. No AEGP suite is queried in render preparation.
    temporal_metadata_enabled=true;
    param_utils.PF_GetCurrentState=[](PF_ProgPtr,PF_ParamIndex index,const A_Time* start,const A_Time* duration,PF_State* state)->PF_Err {
        check(!start && !duration,"native proof covers all time");*state={};std::memcpy(state,&metadata_epoch,sizeof(metadata_epoch));
        unsigned generation{};
        if(index>=kNativeBindingFirstIndex && index<kNativeBindingFirstIndex+kNativeBindingCapacity)
            generation=dependency_generations[index-kNativeBindingFirstIndex];
        else for(const auto n:dependency_generations)generation+=n;
        std::memcpy(reinterpret_cast<char*>(state)+sizeof(metadata_epoch),&generation,sizeof(generation));return 0;
    };
    param_utils.PF_AreStatesIdentical=[](PF_ProgPtr,const PF_State* a,const PF_State* b,A_Boolean* same)->PF_Err {*same=std::memcmp(a,b,sizeof(*a))==0;return 0;};
    stream.AEGP_CanVaryOverTime=[](AEGP_StreamRefH ref,A_Boolean* vary)->A_Err {auto* r=reinterpret_cast<Ref*>(ref);*vary=(metadata_rate_keys && r->effect==1 && r->index==3) || (metadata_life_keys && r->effect==2 && r->index==2);return 0;};
    stream.AEGP_GetExpressionState=[](AEGP_PluginID,AEGP_StreamRefH ref,A_Boolean* enabled)->A_Err {
        auto* r=reinterpret_cast<Ref*>(ref);
        *enabled=!r->effect?expression_enabled[r->index-kNativeBindingFirstIndex]:metadata_life_expression && r->effect==2 && r->index==2;return 0;
    };
    keyframes.AEGP_GetStreamNumKFs=[](AEGP_StreamRefH ref,A_long* count)->A_Err {auto* r=reinterpret_cast<Ref*>(ref);*count=((metadata_rate_keys && r->effect==1 && r->index==3) || (metadata_life_keys && r->effect==2 && r->index==2))?2:0;return 0;};
    keyframes.AEGP_GetKeyframeTime=[](AEGP_StreamRefH,AEGP_KeyframeIndex k,AEGP_LTimeMode,A_Time* t)->A_Err {*t={k*10,1};return 0;};
    keyframes.AEGP_GetKeyframeInterpolation=[](AEGP_StreamRefH,AEGP_KeyframeIndex,AEGP_KeyframeInterpolationType* in,AEGP_KeyframeInterpolationType* out)->A_Err {*in=*out=metadata_bezier?AEGP_KeyInterp_BEZIER:AEGP_KeyInterp_LINEAR;return 0;};
    keyframes.AEGP_GetNewKeyframeValue=[](AEGP_PluginID,AEGP_StreamRefH ref,AEGP_KeyframeIndex k,AEGP_StreamValue2* v)->A_Err {auto* r=reinterpret_cast<Ref*>(ref);*v={};v->streamH=ref;v->val.one_d=r->effect==2?3-k*2:10000+k*10000;return 0;};
    emitter.values[3].one_d=10000;particle.values[2].one_d=2;
    renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* output)->PF_Err {
        *output={};output->param_type=PF_Param_FLOAT_SLIDER;output->u.fs_d.value=evaluated_binding(index,0);return 0;
    };
    auto static_graph=animated_graph;
    // Drop the Force from this reference fixture: owner reports Emitter/Particle only.
    core::NodeId particle_id{},output_id{};
    for(const auto& n:static_graph.nodes) {if(n.type_key==core::graph_keys::kParticleNode)particle_id=n.id;if(n.type_key==core::graph_keys::kOutputNode)output_id=n.id;}
    std::erase_if(static_graph.nodes,[](const auto& n){return n.type_key==core::graph_keys::kForceNode;});
    std::erase_if(static_graph.edges,[&](const auto& e){return !std::any_of(static_graph.nodes.begin(),static_graph.nodes.end(),[&](const auto& n){return n.id==e.source_node;}) || !std::any_of(static_graph.nodes.begin(),static_graph.nodes.end(),[&](const auto& n){return n.id==e.destination_node;});});
    static_graph.edges.push_back({core::EdgeId{core::Uuid128{{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,99}}},particle_id,core::graph_keys::kParticleParticlesOut,output_id,core::graph_keys::kOutputParticles});
    for(auto& n:static_graph.nodes)for(auto& p:n.parameters) {
        if(n.type_key==core::graph_keys::kEmitterNode && p.key==core::graph_keys::kBirthRate)p.value=10000.0;
        if(n.type_key==core::graph_keys::kParticleNode && p.key==core::graph_keys::kParticleLifetimeSeconds)p.value=2.0;
    }
    capture_native_temporal_metadata(&renderer_data,static_graph,1);
    auto proofs=validated_native_control_proofs(&renderer_data);
    check(!proofs.empty() && std::all_of(proofs.begin(),proofs.end(),[](const auto& p){return p.constant;}),"source metadata certifies genuinely unanimated aliases");
    NativeAnimationPlan constant_plan(static_graph,1920,1080,1);constant_plan.prepare_constants(&renderer_data);
    check(constant_plan.fully_constant(),"native plan verifies static controls once per frame");
    check(constant_plan.constant_node(particle_id)!=nullptr,"constant particle node is converted once per frame");
    const auto constant_checkouts=constant_plan.checkout_count();
    for(A_long t:{1,2,2400}) {
        auto sample_data=renderer_data;sample_data.current_time=t;core::GraphNode sample;
        const auto error=constant_plan.sample(&sample_data,particle_id,sample);
        const auto* expected=constant_plan.constant_node(particle_id);
        const auto* a=sample.parameters.empty()?nullptr:std::get_if<core::Vec3>(&sample.parameters[0].value);
        const auto* b=!expected || expected->parameters.empty()?nullptr:std::get_if<core::Vec3>(&expected->parameters[0].value);
        check(error==0 && expected && a && b &&
            sample.parameters.size()==constant_plan.constant_node(particle_id)->parameters.size() &&
            sample.parameters[0].key==constant_plan.constant_node(particle_id)->parameters[0].key &&
            a->x==b->x && a->y==b->y && a->z==b->z,
            "distinct birth times reuse certified node without changing values");
    }
    check(constant_plan.checkout_count()==constant_checkouts,"constant node reuse performs zero historical PF checkouts");
    renderer_data.current_time=2400;
    const auto certified_render_suites=aegp_suite_requests;
    const auto record_count=static_graph.optional_records.size();PF_OutData fast_output{};
    check(capture_emitter_origin_history(&renderer_data,&fast_output,static_graph,1920,1080,core::NeverCancelled{})==0 && static_graph.optional_records.size()==record_count,
        "100-second static scene skips historical traversal and particle snapshot encoding");
    const auto actual=last_native_history_trace();
    check(actual.path==NativeHistoryPath::static_graph && actual.seconds==100 && actual.inputs>0 &&
        actual.constants==actual.inputs && actual.checkouts==0 && actual.rate_queries==0 && actual.life_queries==0 && actual.node_queries==0,
        "static current-frame graph performs no duplicate hoisting or birth history reads");
    check(aegp_suite_requests==certified_render_suites,"certified render preparation still acquires no AEGP suites");
    auto complex_graph=animated_graph;renderer_data.current_time=48;
    capture_native_temporal_metadata(&renderer_data,complex_graph,1);
    PF_OutData complex_output{};
    check(capture_emitter_origin_history(&renderer_data,&complex_output,complex_graph,1920,1080,core::NeverCancelled{})==0,
        "constant nodes work inside a temporal graph containing Force");
    const auto complex_trace=last_native_history_trace();
    check(complex_trace.path==NativeHistoryPath::temporal && complex_trace.node_queries>0 &&
        complex_trace.node_samples==0 && complex_trace.constant_node_hits==complex_trace.node_queries &&
        complex_trace.checkouts==complex_trace.inputs,
        "distinct births reuse constant Emitter/Particle/Force without node conversions or historical checkouts");
    ++metadata_epoch;check(validated_native_control_proofs(&renderer_data).empty(),"edited alias cannot reuse constant metadata");
    constant_plan.prepare_constants(&renderer_data);
    check(!constant_plan.fully_constant() && !constant_plan.constant_node(particle_id),
        "changed dependencies clear converted constant nodes before resampling");
    metadata_rate_keys=true;capture_native_temporal_metadata(&renderer_data,static_graph,1);
    proofs=validated_native_control_proofs(&renderer_data);
    auto keyed=std::find_if(proofs.begin(),proofs.end(),[](const auto& p){return p.rate && p.rate->keys.size()==2;});
    check(keyed!=proofs.end() && !keyed->constant,"UI keyframe reader distinguishes linear PPS from constant");
    if(keyed!=proofs.end()) {core::EmissionTimeline timeline;check(timeline.configure(30,&*keyed->rate).has_value() && timeline.integral(10)==150000,"native linear PPS uses exact key area");}
    metadata_life_keys=true;capture_native_temporal_metadata(&renderer_data,static_graph,1);
    proofs=validated_native_control_proofs(&renderer_data);
    check(std::any_of(proofs.begin(),proofs.end(),[](const auto& p){return !p.constant && p.life_bound && *p.life_bound==3;}),
          "linear Life uses its complete key envelope instead of current Life");
    metadata_bezier=true;capture_native_temporal_metadata(&renderer_data,static_graph,1);
    proofs=validated_native_control_proofs(&renderer_data);
    check(std::none_of(proofs.begin(),proofs.end(),[](const auto& p){return p.life_bound.has_value();}),
          "Bezier Life cannot certify a key-value-only bound");
    metadata_bezier=false;metadata_life_expression=true;capture_native_temporal_metadata(&renderer_data,static_graph,1);
    proofs=validated_native_control_proofs(&renderer_data);
    check(std::none_of(proofs.begin(),proofs.end(),[](const auto& p){return p.life_bound.has_value();}),
          "expression Life cannot certify a key envelope");
    // Actual UI DRAW path: automatic recapture, unchanged dependency throttling,
    // worker rejection, and no project/event mutation.
    metadata_rate_keys=metadata_life_keys=metadata_life_expression=false;
    remember_native_control_proofs(&renderer_data,{});
    auto ui_data=renderer_data;ui_data.num_params=kGraphParameterId+1;
    ui_data.inter.register_ui=[](PF_ProgPtr,PF_CustomUIInfo* info)->PF_Err {
        check(info && info->events==(PF_CustomEFlag_COMP|PF_CustomEFlag_EFFECT) && !info->comp_ui_width && !info->comp_ui_height,
            "metadata UI registers only zero-sized composition events");return 0;
    };
    check(register_native_temporal_ui(&ui_data)==0,"UI metadata callback registration succeeds");
    PF_ArbitraryH ui_graph_handle{};
    check(create_graph_parameter(&ui_data,static_graph,&ui_graph_handle)==0,"UI metadata graph fixture created");
    PF_ParamDef ui_graph_param{};ui_graph_param.param_type=PF_Param_ARBITRARY_DATA;ui_graph_param.u.arb_d.value=ui_graph_handle;
    std::array<PF_ParamDef*,kGraphParameterId+1> ui_params{};ui_params[kGraphParameterId]=&ui_graph_param;
    PF_EventExtra ui_event{};ui_event.e_type=PF_Event_DO_CLICK;ui_event.evt_out_flags=PF_EO_NEVER_UPDATE;
    initialize_native_temporal_ui();const auto event_suites=suite_requests;
    refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
    check(suite_requests==event_suites,"non-DRAW event performs no metadata SDK access");
    ui_event.e_type=PF_Event_DRAW;const auto ui_writes=sets;
    refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
    check(!validated_native_control_proofs(&ui_data).empty(),"UI DRAW automatically captures native animation metadata");
    const auto stable_aegp=aegp_suite_requests;
    refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
    check(aegp_suite_requests==stable_aegp,"unchanged UI dependencies skip source stream reads");
    ++metadata_epoch;
    std::thread worker([&]{refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);});worker.join();
    check(aegp_suite_requests==stable_aegp,"worker DRAW cannot access AEGP metadata");
    refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
    check(aegp_suite_requests>stable_aegp && !validated_native_control_proofs(&ui_data).empty(),
        "authored dependency edit triggers automatic UI recapture");
    check(sets==ui_writes && ui_event.evt_out_flags==PF_EO_NEVER_UPDATE,
        "UI refresh creates no undo writes, rerender or handled-event flags");
    auto render_copy=renderer_data;render_copy.effect_ref=reinterpret_cast<PF_ProgPtr>(2);
    NativeAnimationPlan copied_plan(static_graph,1920,1080,1);copied_plan.prepare_constants(&render_copy);
    check(copied_plan.fully_constant(),"render callback copy uses UI-certified static graph");
    // Cold project lifecycle without Options, DRAW or CEP. The flat marker is
    // retained by the host across save/reopen, not the process-global PF states.
    auto legacy_save=ui_data;legacy_save.sequence_data=nullptr;PF_OutData legacy_flat{};
    const auto legacy_suite_reads=aegp_suite_requests;
    check(flatten_native_temporal_sequence(&legacy_save,&legacy_flat)==0 && legacy_flat.sequence_data &&
        size_handle(legacy_flat.sequence_data)==4 && std::memcmp(lock_handle(legacy_flat.sequence_data),"SFU1",4)==0,
        "legacy null save provisions byte-ordered schema-1 marker");
    check(aegp_suite_requests==legacy_suite_reads,"flattening never reads AEGP metadata");
    legacy_save.sequence_data=legacy_flat.sequence_data;
    check(flatten_native_temporal_sequence(&legacy_save,&legacy_flat)==0 && legacy_flat.sequence_data==legacy_save.sequence_data,
        "already flat marker is preserved without another allocation");
    static_cast<char*>(lock_handle(legacy_save.sequence_data))[3]='2';
    check(flatten_native_temporal_sequence(&legacy_save,&legacy_flat)==PF_Err_INTERNAL_STRUCT_DAMAGED,
        "unknown lifecycle schema is rejected without replacing its data");
    static_cast<char*>(lock_handle(legacy_save.sequence_data))[3]='1';setdown_native_temporal_sequence(&legacy_save);
    const auto sequence_old_graph=main.values[kGraphParameterId].arbH;
    main.values[kGraphParameterId].arbH=reinterpret_cast<AEGP_ArbBlockVal>(clone(ui_graph_handle));
    auto sequence_data=ui_data;sequence_data.sequence_data=nullptr;
    // AE forbids PF checkout/checkin in SEQUENCE_RESETUP. Missing callback
    // functions and a deliberately undersized array must both be harmless.
    const auto sequence_inter=sequence_data.inter;
    static unsigned forbidden_sequence_checkouts{},forbidden_sequence_checkins{};
    sequence_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex,A_long,A_long,A_u_long,PF_ParamDef*)->PF_Err {
        ++forbidden_sequence_checkouts;return PF_Err_BAD_CALLBACK_PARAM;
    };
    sequence_data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err {
        ++forbidden_sequence_checkins;return PF_Err_BAD_CALLBACK_PARAM;
    };
    PF_ParamDef* forbidden_sequence_params[1]{nullptr};
    PF_OutData sequence_output{};
    const auto idle_bootstrap=[&] {NativeBootstrapRequest request;
        refresh_native_temporal_idle(&sequence_data,1,request);return request;};
    remember_native_control_proofs(&sequence_data,{});
    const auto sequence_writes=sets;
    lazy_dependencies=true;dependencies_evaluated.fill(false);
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,forbidden_sequence_params,1,false)==0 &&
        sequence_output.sequence_data && size_handle(sequence_output.sequence_data)==sizeof(std::uint32_t),
        "legacy null sequence receives a flat lifecycle marker");
    check(validated_native_control_proofs(&sequence_data).empty(),"sequence setup never certifies unfinished project state");
    ++metadata_epoch; // AE finishes restoring dependencies AFTER sequence setup.
    check(idle_bootstrap().proofs>0,"post-load idle certifies metadata without Options or DRAW");
    const auto warmed=last_native_metadata_trace();
    check(warmed.inputs>0 && warmed.evaluated==warmed.inputs && warmed.proofs>0 && !warmed.error,
        "cold setup evaluates all active aliases before publishing dependency proofs");
    auto cold_render=sequence_data;cold_render.inter=sequence_inter;
    NativeAnimationPlan cold_plan(static_graph,1920,1080,1);cold_plan.prepare_constants(&cold_render);
    check(cold_plan.fully_constant() && !validated_native_control_proofs(&cold_render).empty(),
        "first render retains warmed proofs after evaluating restored expressions");
    // Negative control: a pre-evaluation token becomes obsolete when the host
    // lazily establishes expression dependencies, exactly the missing old step.
    dependencies_evaluated.fill(false);
    const auto cold_proofs=validated_native_control_proofs(&cold_render);
    for(const auto& proof:cold_proofs)(void)evaluated_binding(proof.stream,0);
    check(!cold_proofs.empty() && validated_native_control_proofs(&cold_render).empty(),
        "fixture proves pre-evaluation states fail after lazy dependency discovery");
    sequence_data.sequence_data=sequence_output.sequence_data;
    const auto saved_marker=clone(sequence_data.sequence_data);
    setdown_native_temporal_sequence(&sequence_data);sequence_data.sequence_data=saved_marker;
    remember_native_control_proofs(&sequence_data,{});++metadata_epoch;dependencies_evaluated.fill(false);
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,forbidden_sequence_params,1,true)==0 &&
        sequence_output.sequence_data==saved_marker && validated_native_control_proofs(&sequence_data).empty(),
        "save/reopen resetup only restores the flat marker");
    ++metadata_epoch;
    check(idle_bootstrap().proofs>0,"post-load idle rebuilds process metadata after reopen");
    check(last_native_ui_timing().idle_refreshes>=2 && last_native_ui_timing().sequence_refreshes==0 && sets==sequence_writes,
        "idle bootstrap is timed and writes no project streams");
    NativeAnimationPlan reopened_constant(static_graph,1920,1080,1);reopened_constant.prepare_constants(&cold_render);
    check(reopened_constant.fully_constant(),"save/reopen warms process-local dependencies again without Options");
    const auto active_alias=validated_native_control_proofs(&sequence_data).front().stream;
    const auto assert_missing_alias=[&] {
        const auto available=validated_native_control_proofs(&sequence_data);
        check(last_native_metadata_trace().error!=0 &&
            std::none_of(available.begin(),available.end(),[&](const auto& p){return p.stream==active_alias;}),
            "an alias that cannot be evaluated cannot certify a constant or analytical profile");
    };
    const auto failure_refs=live_refs;const auto failure_handles=handles.size();
    fail_read=active_alias;capture_native_temporal_metadata(&sequence_data,static_graph,1);assert_missing_alias();
    wrong_type=active_alias;capture_native_temporal_metadata(&sequence_data,static_graph,1);assert_missing_alias();
    nonfinite_alias=active_alias;capture_native_temporal_metadata(&sequence_data,static_graph,1);assert_missing_alias();nonfinite_alias=-1;
    expression_enabled[active_alias-kNativeBindingFirstIndex]=FALSE;
    capture_native_temporal_metadata(&sequence_data,static_graph,1);assert_missing_alias();
    expression_enabled[active_alias-kNativeBindingFirstIndex]=TRUE;
    check(live_refs==failure_refs && handles.size()==failure_handles,"failed alias warmups release every value and stream");
    metadata_rate_keys=true;++metadata_epoch;
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,forbidden_sequence_params,1,true)==0,
        "reopen reads current keyframe metadata");
    (void)idle_bootstrap();
    auto keyed_render=sequence_data;keyed_render.inter=sequence_inter;
    NativeAnimationPlan reopened_keyed(static_graph,1920,1080,1);reopened_keyed.prepare_constants(&keyed_render);
    check(!reopened_keyed.fully_constant(),"animated PPS on reopen cannot reuse a previous constant proof");
    metadata_rate_keys=false;
    const auto sequence_suites=aegp_suite_requests;
    sequence_data.in_flags=PF_InFlag_PROJECT_IS_RENDER_ONLY;
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,forbidden_sequence_params,1,true)==0 &&
        aegp_suite_requests==sequence_suites,"render-only resetup performs no AEGP metadata queries");
    sequence_data.in_flags=PF_InFlag_NONE;
    std::thread sequence_worker([&]{check(setup_native_temporal_sequence(&sequence_data,&sequence_output,forbidden_sequence_params,1,true)==0,
        "worker sequence resetup retains flat marker");});sequence_worker.join();
    check(aegp_suite_requests==sequence_suites,"worker sequence callback never reads AEGP source controls");
    check(forbidden_sequence_checkouts==0 && forbidden_sequence_checkins==0,
        "UI and worker sequence callbacks never invoke AE's forbidden PF checkout/checkin");
    sequence_data.num_params=0;sequence_data.inter.checkout_param=nullptr;sequence_data.inter.checkin_param=nullptr;
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,nullptr,1,true)==0 &&
        idle_bootstrap().proofs>0,
        "idle does not require generic params or PF checkout callbacks");
    const auto sequence_handles=handles.size();const auto sequence_refs=live_refs;
    fail_read=kGraphParameterId;remember_native_control_proofs(&sequence_data,{});
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,nullptr,1,true)==0 &&
        idle_bootstrap().error!=0 && validated_native_control_proofs(&sequence_data).empty(),"unavailable idle graph is an optional miss");
    check(handles.size()==sequence_handles && live_refs==sequence_refs,"failed startup read balances handles and streams");
    wrong_type=kGraphParameterId;
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,nullptr,1,true)==0 &&
        idle_bootstrap().error!=0 && validated_native_control_proofs(&sequence_data).empty(),"wrong idle graph type is rejected before union access");
    check(handles.size()==sequence_handles && live_refs==sequence_refs,"wrong-type startup read releases owned references");
    check(setup_native_temporal_sequence(&sequence_data,&sequence_output,nullptr,1,true)==0 &&
        idle_bootstrap().proofs>0,"subsequent valid idle read can recover without Options");
    const auto idle_suites=aegp_suite_requests;
    NativeBootstrapRequest invalid_idle;invalid_idle.version=2;refresh_native_temporal_idle(&sequence_data,1,invalid_idle);
    check(!invalid_idle.acknowledged && aegp_suite_requests==idle_suites,"unknown idle protocol is ignored before host access");
    NativeBootstrapRequest absent;refresh_native_temporal_idle(nullptr,1,absent);
    check(absent.acknowledged && absent.error!=0 && aegp_suite_requests==idle_suites,"weak generic PF context is an acknowledged optional miss");
    std::thread idle_worker([&]{NativeBootstrapRequest request;refresh_native_temporal_idle(&sequence_data,1,request);
        check(request.acknowledged && request.error!=0,"worker idle message cannot access UI metadata");});idle_worker.join();
    check(aegp_suite_requests==idle_suites,"idle worker guard runs before any AEGP call");
    dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));main.values[kGraphParameterId].arbH=sequence_old_graph;
    setdown_native_temporal_sequence(&sequence_data);
    dispose(ui_graph_handle);
    lazy_dependencies=false;
    temporal_metadata_enabled=false;remember_native_control_proofs(&renderer_data,{});

    // Append the new kind without changing the legacy fixture or its expression
    // artifact. These checks execute the real UI compiler and numeric playback.
    {
        using namespace core::graph_keys;
        auto& transform=fixtures[4];transform_present=true;
        transform.name="org.starfieldfx.node.transform";
        transform.values.resize(records::parameter_count(records::Kind::transform));
        transform.params.resize(transform.values.size());
        for(auto& p:transform.params)p.param_type=PF_Param_FLOAT_SLIDER;
        uuid(4,records::uuid_first_index(records::Kind::transform),5);
        transform.values[2].two_d={960,540};
        for(A_long i=10;i<=14;++i)transform.values[i].one_d=100;
        connection(3,records::Kind::force,5,13);connection(4,records::Kind::transform,255,14);
        std::vector<PF_ParamDef*> renderer_params;for(auto& p:main.params)renderer_params.push_back(&p);
        core::Graph transformed;bool found{};
        check(compile_native_node_graph(&renderer_data,renderer_params.data(),transformed,found,701)==0 && found,
            "Transform None compiles with native schema and synthetic fields");
        auto tf=std::find_if(transformed.nodes.begin(),transformed.nodes.end(),[](const auto& n){return n.type_key==kTransformNode;});
        check(tf!=transformed.nodes.end() && tf->schema_version==1,"compiler appends the independent Transform kind");
        const auto transform_id=tf->id;
        check(std::get<std::uint32_t>(parameter(transformed,kTransformNode,kTransformInheritLayer))==0,
            "None resource is project-local zero");
        const auto& inherited=std::get<core::OpaqueBytes>(parameter(transformed,kTransformNode,kTransformInheritedMatrix));
        check(inherited.size()==132 && inherited[0]==std::byte{1},"UI capture writes the bounded affine payload");
        const auto anchor=std::get<core::Vec3>(parameter(transformed,kTransformNode,kTransformAnchor));
        check(anchor.x==0 && anchor.y==0 && anchor.z==0,
            "native centred Anchor XY and Z0 map to canonical zero");
        check(core::validate_graph(transformed,core::particle_node_registry()).ok(),"new native graph passes current registry");
        {NativeBindingTransaction bindings(&renderer_data,701);check(bindings.install(transformed)==0,"new synthetic expressions install");bindings.accept();}
        save_transform_expressions("artifacts/transform-none-animation-expressions.json");
        const auto matrix_alias=std::find_if(expressions.begin(),expressions.end(),[](const auto& text) {
            return text.find(u"// Starfield Transform affine entry 0\n")!=std::u16string::npos;
        });
        check(matrix_alias!=expressions.end(),"first Transform matrix alias exists");
        const auto matrix_stream=kNativeBindingFirstIndex+static_cast<A_long>(matrix_alias-expressions.begin());
        const auto before_expression_failure=expressions;
        expressions[matrix_stream-kNativeBindingFirstIndex]=u"0";
        {
            NativeBindingTransaction bindings(&renderer_data,701);A_long failed_stream=-1,failed_parameter=-1;
            const char* failed_stage=nullptr;fail_evaluated_read=matrix_stream;
            check(bindings.install(transformed,&failed_stream,&failed_stage,&failed_parameter)==PF_Err_BAD_CALLBACK_PARAM &&
                failed_stream==matrix_stream && failed_parameter==15 &&
                failed_stage && std::strcmp(failed_stage,"read evaluated value")==0,
                "binding failure identifies the synthetic parameter and precise host operation");
        }
        check(expressions[matrix_stream-kNativeBindingFirstIndex]==u"0" && live_refs==0 && acquisitions==0,
            "failed matrix evaluation restores expression text and releases host references");
        {
            NativeBindingTransaction bindings(&renderer_data,701);const char* failed_stage=nullptr;
            nonfinite_alias=matrix_stream;
            check(bindings.install(transformed,nullptr,&failed_stage)==PF_Err_BAD_CALLBACK_PARAM &&
                failed_stage && std::strcmp(failed_stage,"verify finite bound value")==0,
                "invalid evaluated results are distinguished from host suite failures");
        }
        nonfinite_alias=-1;
        expressions=before_expression_failure;
        temporal_metadata_enabled=true;synthetic_native_reads=0;
        capture_native_temporal_metadata(&renderer_data,transformed,701);
        check(synthetic_native_reads==0,"constancy proof never queries synthetic fields as native metadata streams");
        renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex i,A_long t,A_long,A_u_long,PF_ParamDef* output)->PF_Err {
            *output={};output->param_type=PF_Param_FLOAT_SLIDER;output->u.fs_d.value=evaluated_binding(i,t);return 0;
        };
        NativeAnimationPlan none_plan(transformed,1920,1080,1);none_plan.prepare_constants(&renderer_data);
        check(none_plan.constant_node(transform_id)!=nullptr,"None affine can certify identity through alias states");
        // Native numbers still use callback values; pixel and angle signs convert once.
        auto move=edit(4,4,108);check(direct_edit(move)==0 && move.accepted,"native Transform Position edit publishes atomically");
        auto synthetic_edit=edit(4,15,0);check(!node_sync::valid_edit(synthetic_edit),"synthetic binding fields cannot be authored as native controls");
        auto typed_marker=edit(4,1,0);typed_marker.value_kind=node_sync::ValueKind::point2;
        check(!node_sync::valid_edit(typed_marker),"resource selection marker requires its declared type");
        check(std::abs(std::get<core::Vec3>(parameter(saved_graph(),kTransformNode,kTransformPosition)).x-.1)<1e-12,
            "native Position pixels convert to canonical units");
        transform.values[4].one_d=108;transform.values[7].one_d=10;transform.values[8].one_d=20;transform.values[9].one_d=30;
        layers.AEGP_GetLayerParentComp=[](AEGP_LayerH,AEGP_CompH* comp)->A_Err {*comp=reinterpret_cast<AEGP_CompH>(1);return 0;};
        layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH,AEGP_LayerIDVal id,AEGP_LayerH* out)->A_Err {
            *out=id==77?reinterpret_cast<AEGP_LayerH>(2):nullptr;return 0;
        };
        layers.AEGP_ConvertLayerToCompTime=[](AEGP_LayerH,const A_Time* time,A_Time* out)->A_Err {*out=*time;return 0;};
        layers.AEGP_GetLayerToWorldXform=[](AEGP_LayerH layer,const A_Time* time,A_Matrix4* out)->A_Err {
            ++affine_reads;auto matrix=layer==reinterpret_cast<AEGP_LayerH>(2)?null_world:owner_world;
            if(layer==reinterpret_cast<AEGP_LayerH>(2))matrix[3]+=time->value;
            for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c)out->mat[r][c]=matrix[c*4+r];return 0;
        };
        stream.AEGP_GetNewLayerStream=[](AEGP_PluginID,AEGP_LayerH,AEGP_LayerStream which,AEGP_StreamRefH* out)->A_Err {
            check(which==AEGP_LayerStream_ANCHORPOINT,"Null capture samples its anchor stream");
            ++live_refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{5,0});return 0;
        };
        transform.values[1].layer_id=77;renderer_data.current_time=0;
        auto select=edit(4,1,0);
        check(direct_edit(select)==0 && select.accepted,"PF layer marker resolves the authoritative resource ID");
        save_transform_expressions("artifacts/transform-selected-animation-expressions.json");
        transformed=saved_graph();check(std::get<std::uint32_t>(parameter(transformed,kTransformNode,kTransformInheritLayer))==77,
            "selected resource uses ID instead of layer index");
        const auto rotation=std::get<core::Vec3>(parameter(transformed,kTransformNode,kTransformRotation));
        check(rotation.x==-10 && rotation.y==20 && rotation.z==-30,"native Euler signs map to the core frame");
        synthetic_native_reads=0;capture_native_temporal_metadata(&renderer_data,transformed,701);
        check(synthetic_native_reads==0,"selected Null synthetic fields skip native constancy queries");
        NativeAnimationPlan dynamic_plan(transformed,1920,1080,1);dynamic_plan.prepare_constants(&renderer_data);
        check(!dynamic_plan.constant_node(transform_id),"unkeyed node controls cannot freeze Null or parent motion");
        const auto before_render_suites=aegp_suite_requests,before_affine_reads=affine_reads;
        renderer_data.current_time=24;core::GraphNode sampled;
        check(dynamic_plan.sample(&renderer_data,transform_id,sampled)==0,"animated Null coefficients sample through numeric aliases");
        auto sampled_graph=transformed;for(auto& n:sampled_graph.nodes)if(n.id==transform_id)n=sampled;
        const auto& bytes=std::get<core::OpaqueBytes>(parameter(sampled_graph,kTransformNode,kTransformInheritedMatrix));
        std::uint64_t bits{};for(unsigned b=0;b<8;++b)bits|=std::uint64_t(std::to_integer<unsigned char>(bytes[4+3*8+b]))<<(8*b);
        check(std::abs(std::bit_cast<double>(bits)-24./1080)<1e-12,"Null animation moves the canonical pose at exact sample time");
        check(aegp_suite_requests==before_render_suites && affine_reads==before_affine_reads,
            "render playback invokes no AEGP or layer-transform callback");
        static const auto transform_frame_checkout=renderer_data.inter.checkout_param;
        renderer_data.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* value)->PF_Err {
            if(index==kTimeRemapEnabledId){*value={};value->param_type=PF_Param_CHECKBOX;return 0;}
            return transform_frame_checkout(ref,index,time,step,scale,value);
        };
        std::vector<CapturedParticleFrame> opening_frames;
        const std::array<core::RationalTime,1> opening_times{{{-1,48}}};PF_OutData opening_output{};
        check(capture_motion_particles(&renderer_data,&opening_output,transformed,1920,1080,opening_times,core::NeverCancelled{},opening_frames)==0 &&
              opening_frames.size()==1 && opening_frames[0].particles.particles.empty(),
              "selected Null accepts an empty negative opening-shutter sample instead of AE range error");
        check(last_native_history_trace().node_samples==0 && !opening_output.return_msg[0],
              "opening-shutter sample does not request historical Transform aliases");
        renderer_data.inter.checkout_param=transform_frame_checkout;
        transform.values[1].layer_id=0;
        auto deselect=edit(4,1,0);
        check(direct_edit(deselect)==0 && deselect.accepted,"selecting None regenerates identity bindings");
        unsigned identity_bindings=0;
        for(const auto& expression:expressions) if(expression.find(u"// Starfield Transform affine entry ")!=std::u16string::npos) {
            ++identity_bindings;
            check(expression.find(u"fx.param(1)")==std::u16string::npos,
                "deselecting the source removes every layer-property dependency");
        }
        check(identity_bindings==12 && std::get<std::uint32_t>(parameter(saved_graph(),kTransformNode,kTransformInheritLayer))==0,
            "None preserves the resource and matrix record contract");
        transform.values[1].layer_id=77;
        auto reselect=edit(4,1,0);
        check(direct_edit(reselect)==0 && reselect.accepted,"reselecting a Null restores dynamic bindings");
        const auto before_bad_writes=sets;transform.values[1].layer_id=88;
        auto missing=edit(4,1,0);check(direct_edit(missing)!=0 && !missing.accepted && sets==before_bad_writes,
            "unresolved resource rejects before any publication");
        transform.values[1].layer_id=77;owner_world[0]=0;
        auto singular=edit(4,4,200);check(direct_edit(singular)!=0 && !singular.accepted && sets==before_bad_writes,
            "singular owner frame rejects atomically");
        owner_world[0]=1;renderer_data.current_time=0;
        check(live_refs==0 && acquisitions==0,"Transform capture and failure paths balance host references");
        transform_present=false;temporal_metadata_enabled=false;remember_native_control_proofs(&renderer_data,{});
    }

    {
        namespace layout=records::particle_layout;using namespace core::graph_keys;
        const auto has_cloud=[](const core::Graph& g) {
            for(const auto& n:g.nodes)if(n.type_key==kParticleNode)
                for(const auto& p:n.parameters)if(p.key==kCloudCircles || p.key==kCloudAspect || p.key==kCloudDensity)return true;
            return false;
        };
        connection(2,records::Kind::particle,4,12);
        connection(3,records::Kind::force,255,13);
        particle.values[layout::shape].one_d=3;
        particle.values[layout::cloud_enabled].one_d=0;
        auto legacy_edit=edit(1,layout::size,10);
        const auto legacy_error=direct_edit(legacy_edit);
        if(legacy_error)std::printf("Cloud legacy fixture error %d stage %s stream %ld binding %ld\n",legacy_error,
            node_sync::stage_name(legacy_edit.stage),long(legacy_edit.stream_index),long(legacy_edit.binding_parameter));
        if(legacy_error) {
            std::vector<PF_ParamDef*> mp;for(auto& p:main.params)mp.push_back(&p);
            core::Graph diagnostic;bool found{};compile_native_node_graph(&renderer_data,mp.data(),diagnostic,found,701,&legacy_edit);
            const auto v=core::validate_graph(diagnostic,core::particle_node_registry());
            std::printf("Cloud topology diagnostic: %s: %s nodes %zu edges %zu\n",core::describe(v.error.code),v.error.detail,diagnostic.nodes.size(),diagnostic.edges.size());
            for(const auto& e:diagnostic.edges)std::printf("Cloud edge %u to %u\n",unsigned(e.source_node.value.bytes[15]),unsigned(e.destination_node.value.bytes[15]));
        }
        check(legacy_error==0 && !has_cloud(saved_graph()),"appended defaults preserve old native Cloud on unrelated edits");
        const auto legacy=saved_graph();
        // Rebuild the real private record at each historical Particle field bound.
        for(unsigned version=1;version<=4;++version) {
            auto old=legacy;
            for(auto& bytes:old.optional_records)if(bytes.size()>12 && bytes[0]==std::byte{2} && bytes[1]==std::byte{0x80}) {
                const auto get16=[&](std::size_t at){return std::to_integer<unsigned>(bytes[at])|(std::to_integer<unsigned>(bytes[at+1])<<8);};
                core::OpaqueBytes trimmed(bytes.begin(),bytes.begin()+12);trimmed[2]=std::byte(version);trimmed[3]=std::byte{0};
                std::size_t at=12;
                while(at<bytes.size()) {
                    if(at+20>bytes.size()){check(false,"binding fixture node header bounded");return 1;}
                    const auto kind=get16(at+16),count=get16(at+18);const auto start=trimmed.size();
                    trimmed.insert(trimmed.end(),bytes.begin()+at,bytes.begin()+at+20);at+=20;
                    if(at+32*count>bytes.size()){check(false,"binding fixture scalar records bounded");return 1;}
                    unsigned kept=0;
                    for(unsigned f=0;f<count;++f,at+=32) {
                        const auto limit=kind==1?(version<3?442u:version==3?519u:526u):533u;
                        if(get16(at)<=limit) {trimmed.insert(trimmed.end(),bytes.begin()+at,bytes.begin()+at+32);++kept;}
                    }
                    trimmed[start+18]=std::byte(kept&255);trimmed[start+19]=std::byte(kept>>8);
                }
                for(unsigned i=0;i<4;++i)trimmed[4+i]=std::byte((trimmed.size()>>(8*i))&255);
                bytes=std::move(trimmed);
            }
            NativeAnimationPlan old_plan(old,1920,1080,1);
            check(old_plan.valid() && sample_native_node_animation(&renderer_data,old,1920,1080)==0 && !has_cloud(old),
                  "historical binding versions retain legacy Cloud and their original field limits");
        }
        auto bad_old=legacy;
        for(auto& r:bad_old.optional_records)if(r.size()>12 && r[0]==std::byte{2} && r[1]==std::byte{0x80})r[2]=std::byte{4};
        check(!NativeAnimationPlan(bad_old,1920,1080,1).valid(),"v4 rejects an appended activation field without migration");
        auto activate=edit(1,layout::cloud_density,1000);
        activate.additional_fields[activate.additional_count++]={layout::cloud_enabled,node_sync::ValueKind::scalar,{1,0,0,0}};
        const auto activation_error=direct_edit(activate);
        if(activation_error)std::printf("Cloud activation fixture error %d stage %s stream %ld binding %ld\n",activation_error,
            node_sync::stage_name(activate.stage),long(activate.stream_index),long(activate.binding_parameter));
        check(node_sync::valid_edit(activate) && activation_error==0 && activate.accepted,"Cloud activation and Density commit atomically before the host saves streams");
        if(activation_error)return 1;
        auto configured=saved_graph();
        check(std::get<std::uint32_t>(parameter(configured,kParticleNode,kCloudCircles))==10 &&
              std::get<double>(parameter(configured,kParticleNode,kCloudAspect))==150 &&
              std::get<double>(parameter(configured,kParticleNode,kCloudDensity))==1000,"native graph receives explicit Cloud values and defaults");
        auto forged=activate;forged.parameter_index=layout::size;
        check(!node_sync::valid_edit(forged),"unrelated edits cannot inject the Cloud activation override");
        forged=activate;forged.additional_fields[0].value[0]=0;
        check(!node_sync::valid_edit(forged),"activation override only admits the constant enabled value");
        particle.values[layout::cloud_enabled].one_d=1;
        for(auto [index,value]:{std::pair{layout::cloud_circles,0.0},std::pair{layout::cloud_circles,1001.0},
                               std::pair{layout::cloud_aspect,0.0},std::pair{layout::cloud_density,-1.0},std::pair{layout::cloud_density,1001.0}}) {
            const auto previous=saved_graph();auto rejected=edit(1,index,value);
            check(direct_edit(rejected)!=0 && !rejected.accepted &&
                  core::serialize_graph(previous,core::particle_node_registry()).value()==core::serialize_graph(saved_graph(),core::particle_node_registry()).value(),
                  "invalid Cloud range rejects without replacing the saved graph");
        }
        particle.values[layout::cloud_circles].one_d=10.4;
        auto round=edit(1,layout::cloud_density,66);check(direct_edit(round)==0,"native animated Circles captures a rounded integer");
        auto animated=saved_graph();cloud_animation_test=true;renderer_data.current_time=6;
        check(sample_native_node_animation(&renderer_data,animated,1920,1080)==0 &&
              std::get<std::uint32_t>(parameter(animated,kParticleNode,kCloudCircles))==11 &&
              std::get<double>(parameter(animated,kParticleNode,kCloudAspect))==150.25 &&
              std::get<double>(parameter(animated,kParticleNode,kCloudDensity))==66.25,"Cloud aliases sample all three controls at render time and round Circles");
        cloud_animation_test=false;renderer_data.current_time=0;
        check(live_refs==0 && acquisitions==0,"Cloud conversion and compatibility sampling release all host references");
    }
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
