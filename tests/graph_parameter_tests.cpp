// Uses the local Adobe SDK declarations and fake host callbacks, not an AE process.
#include "GraphParameter.hpp"
#include "GraphCarrier.hpp"
#include "Parameters.hpp"
#include "WorldBridge.hpp"
#include "AE_EffectCB.h"
#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace starfield;
using namespace starfield::adapter;

namespace starfield::adapter {
// The adapter fake-host suite exercises parameter checkout and graph construction;
// AEGP expression access is covered by the separate host carrier gate, so capture
// tests provide a successful snapshot sink instead of linking real AEGP suites.
PF_Err write_graph_snapshot(PF_InData*, const core::Graph&, A_long* new_revision) noexcept {
    if (new_revision) *new_revision = 1;
    return PF_Err_NONE;
}
}

namespace {
int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
struct Memory { core::OpaqueBytes bytes; unsigned locks{0}; };
std::unordered_map<PF_Handle, std::unique_ptr<Memory>> handles;
bool fail_allocation = false, fail_lock = false;
std::array<PF_ParamDef, kTotalEffectParameterCount + 1> parameters{};
std::vector<PF_ParamDef> registered;
std::unordered_map<PF_ParamDef*, A_long> checked_out;
std::vector<A_long> checked_indices;
A_long fail_checkout = -1;
A_long last_time = 0;

class CancelAtPoll final : public core::Cancellation {
public:
    explicit CancelAtPoll(unsigned poll) : cancel_at_(poll) {}
    [[nodiscard]] bool is_cancelled() const noexcept override { return ++polls_ >= cancel_at_; }
private:
    unsigned cancel_at_{0};
    mutable unsigned polls_{0};
};

PF_Handle allocate(A_u_longlong size) {
    if (fail_allocation) { fail_allocation = false; return nullptr; }
    auto memory = std::make_unique<Memory>();
    memory->bytes.resize(static_cast<std::size_t>(size));
    auto handle = new void*(memory->bytes.data());
    handles.emplace(handle, std::move(memory));
    return handle;
}
void* lock(PF_Handle handle) {
    if (fail_lock) { fail_lock = false; return nullptr; }
    if (!handles.contains(handle)) return nullptr;
    ++handles.at(handle)->locks;
    return handles.at(handle)->bytes.data();
}
void unlock(PF_Handle handle) { CHECK(handles.at(handle)->locks > 0); --handles.at(handle)->locks; }
void dispose(PF_Handle handle) {
    if (!handle) return;
    CHECK(handles.contains(handle));
    if (!handles.contains(handle)) return;
    CHECK(handles.at(handle)->locks == 0);
    handles.erase(handle); delete handle;
}
A_u_longlong size_of(PF_Handle handle) { return handles.at(handle)->bytes.size(); }
PF_Err checkout(PF_ProgPtr, PF_ParamIndex index, A_long time, A_long, A_u_long, PF_ParamDef* value) {
    last_time = time;
    checked_indices.push_back(index);
    if (index == fail_checkout) return PF_Err_OUT_OF_MEMORY;
    if (index < 1 || index > static_cast<PF_ParamIndex>(kTotalEffectParameterCount) || !value) {
        return PF_Err_BAD_CALLBACK_PARAM;
    }
    *value = parameters[static_cast<std::size_t>(index)];
    checked_out[value] = index;
    return PF_Err_NONE;
}
PF_Err checkin(PF_ProgPtr, PF_ParamDef* value) {
    CHECK(checked_out.contains(value)); checked_out.erase(value); return PF_Err_NONE;
}
PF_Err add_param(PF_ProgPtr, PF_ParamIndex, PF_ParamDef* value) { registered.push_back(*value); return PF_Err_NONE; }

PF_ArbParamsExtra call_for(PF_FunctionSelector function) {
    PF_ArbParamsExtra call{}; call.id = kGraphParameterId; call.which_function = function; return call;
}
PF_Err invoke(PF_InData& host, PF_ArbParamsExtra& call) { return graph_arbitrary_callback(&host, &call); }
PF_ArbitraryH new_value(PF_InData& host) {
    PF_ArbitraryH value = nullptr;
    auto call = call_for(PF_Arbitrary_NEW_FUNC); call.u.new_func_params.arbPH = &value;
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(value != nullptr); return value;
}

void test_callbacks(PF_InData& host) {
    const auto original = new_value(host);
    const auto bytes = handles.at(original)->bytes;
    CHECK(read_graph_parameter(&host, original).has_value());
    PF_ArbitraryH copied = nullptr;
    auto call = call_for(PF_Arbitrary_COPY_FUNC);
    call.u.copy_func_params = {nullptr, original, &copied};
    CHECK(invoke(host, call) == PF_Err_NONE);
    CHECK(original != copied && handles.at(copied)->bytes == bytes);

    A_u_long flat_size = 0;
    call = call_for(PF_Arbitrary_FLAT_SIZE_FUNC);
    call.u.flat_size_func_params = {nullptr, original, &flat_size};
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(flat_size == bytes.size());
    core::OpaqueBytes buffer(flat_size + 8, std::byte{0xcc});
    call = call_for(PF_Arbitrary_FLATTEN_FUNC);
    call.u.flatten_func_params = {nullptr, original, flat_size - 1, buffer.data()};
    CHECK(invoke(host, call) != PF_Err_NONE); CHECK(buffer[0] == std::byte{0xcc});
    call.u.flatten_func_params.buf_sizeLu = flat_size;
    CHECK(invoke(host, call) == PF_Err_NONE);
    CHECK(std::memcmp(buffer.data(), bytes.data(), flat_size) == 0); CHECK(buffer[flat_size] == std::byte{0xcc});

    PF_ArbitraryH restored = nullptr;
    call = call_for(PF_Arbitrary_UNFLATTEN_FUNC);
    call.u.unflatten_func_params = {nullptr, flat_size, buffer.data(), &restored};
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(handles.at(restored)->bytes == bytes);
    dispose(restored); restored = nullptr;
    buffer[flat_size - 1] ^= std::byte{1};
    CHECK(invoke(host, call) != PF_Err_NONE); CHECK(restored == nullptr);
    buffer[flat_size - 1] ^= std::byte{1};
    buffer[8] = std::byte{2}; // unsupported version cannot silently reset
    CHECK(invoke(host, call) != PF_Err_NONE); CHECK(restored == nullptr);

    A_u_long text_size = 0;
    call = call_for(PF_Arbitrary_PRINT_SIZE_FUNC);
    call.u.print_size_func_params = {nullptr, original, &text_size};
    CHECK(invoke(host, call) == PF_Err_NONE);
    std::vector<char> text(text_size + 1, '?');
    call = call_for(PF_Arbitrary_PRINT_FUNC);
    call.u.print_func_params = {nullptr, PF_ArbPrint_NONE, original, text_size - 1, text.data()};
    CHECK(invoke(host, call) != PF_Err_NONE); CHECK(text[0] == '?');
    call.u.print_func_params.print_sizeLu = text_size;
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(text[text_size - 1] == '\0' && text[text_size] == '?');
    call = call_for(PF_Arbitrary_SCAN_FUNC);
    call.u.scan_func_params = {nullptr, text.data(), text_size, &restored};
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(handles.at(restored)->bytes == bytes);
    dispose(restored); restored = nullptr;
    call.u.scan_func_params.bytes_to_scanLu = text_size - 1; // also accept no terminating NUL
    CHECK(invoke(host, call) == PF_Err_NONE); dispose(restored); restored = nullptr;
    text[10] = '!';
    CHECK(invoke(host, call) == PF_Err_CANNOT_PARSE_KEYFRAME_TEXT); CHECK(restored == nullptr);

    PF_ArbCompareResult comparison = PF_ArbCompare_NOT_EQUAL;
    call = call_for(PF_Arbitrary_COMPARE_FUNC);
    call.u.compare_func_params = {nullptr, original, copied, &comparison};
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(comparison == PF_ArbCompare_EQUAL);
    auto settings = core::Settings{}; settings.seed = 19;
    auto graph = graph_from_controls(settings);
    CHECK(graph.has_value());
    PF_ArbitraryH different = nullptr;
    CHECK(create_graph_parameter(&host, graph.value(), &different) == PF_Err_NONE);
    call.u.compare_func_params.b_arbH = different;
    CHECK(invoke(host, call) == PF_Err_NONE); CHECK(comparison == PF_ArbCompare_NOT_EQUAL);
    for (const double time : {0.0, 0.5, 1.0}) {
        call = call_for(PF_Arbitrary_INTERP_FUNC);
        call.u.interp_func_params = {nullptr, original, different, time, &restored};
        CHECK(invoke(host, call) == PF_Err_NONE);
        CHECK(handles.at(restored)->bytes == handles.at(time < 1.0 ? original : different)->bytes);
        dispose(restored); restored = nullptr;
    }

    const auto live = handles.size();
    call = call_for(PF_Arbitrary_COPY_FUNC);
    call.u.copy_func_params = {nullptr, original, &restored};
    fail_allocation = true;
    CHECK(invoke(host, call) == PF_Err_OUT_OF_MEMORY); CHECK(restored == nullptr && handles.size() == live);
    fail_lock = true;
    CHECK(invoke(host, call) != PF_Err_NONE); CHECK(restored == nullptr && handles.size() == live);
    // Failure to lock a newly allocated handle must dispose it.
    fail_lock = true;
    CHECK(create_graph_parameter(&host, graph.value(), &restored) == PF_Err_OUT_OF_MEMORY);
    CHECK(restored == nullptr && handles.size() == live);
    call.id = 99; CHECK(invoke(host, call) == PF_Err_BAD_CALLBACK_PARAM);
    CHECK(!read_graph_parameter(&host, nullptr).has_value());
    CHECK(graph_arbitrary_callback(nullptr, &call) == PF_Err_BAD_CALLBACK_PARAM);
    for (auto value : {original, copied, different}) {
        call = call_for(PF_Arbitrary_DISPOSE_FUNC); call.u.dispose_func_params = {nullptr, value};
        CHECK(invoke(host, call) == PF_Err_NONE);
    }
}

void test_parameters(PF_InData& host) {
    PF_OutData output{};
    CHECK(setup_parameters(&host, &output) == PF_Err_NONE);
    CHECK(output.num_params == static_cast<A_long>(kTotalEffectParameterCount + 1) &&
          registered.size() == kTotalEffectParameterCount);
    const auto& source = registered[kControlSourceId - 1];
    // Manifest revision 6: a fresh effect must drive the visible controls, so both the
    // default and the old-project value select AE Controls. Node Graph is opt-in.
    CHECK(source.u.pd.dephault == kLegacyControlSource && source.u.pd.value == kLegacyControlSource);
    CHECK((source.flags & PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS) != 0);
    // The SDK's own PF_ADD_ARBITRARY2 passes no PF_ParamFlags; arbitrary data cannot be
    // animated, and the two flags this used to carry were never part of the contract.
    CHECK(registered[kGraphParameterId - 1].flags == 0);
    CHECK(registered[kGraphParameterId - 1].u.arb_d.value == nullptr);
    for (std::size_t i = 1; i <= kTotalEffectParameterCount; ++i) {
        parameters[i] = registered[i - 1];
        parameters[i].uu.change_flags = 0;
    }
    parameters[kGraphParameterId].u.arb_d.value = new_value(host);
    parameters[kControlSourceId].u.pd.value = kNodeControlSource;
    std::shared_ptr<const core::Graph> snapshot;
    A_long active_source = -1;
    checked_indices.clear();
    CHECK(checkout_render_graph(&host, &output, snapshot, &active_source) == PF_Err_NONE);
    CHECK(snapshot && checked_out.empty() && active_source == kNodeControlSource);
    CHECK(checked_indices == std::vector<A_long>({kControlSourceId, kGraphParameterId}));
    const auto frozen = core::serialize_graph(*snapshot, core::particle_node_registry());
    CHECK(frozen.has_value());
    const auto previous = parameters[kGraphParameterId].u.arb_d.value;
    // Set actual host-style slider values, independently of registration defaults.
    parameters[kMaxParticlesId].u.fs_d.value = 20;
    parameters[kParticlesPerSecondId].u.fs_d.value = 12;
    parameters[kSeedId].u.fs_d.value = 99;
    parameters[kLifetimeId].u.fs_d.value = 3;
    parameters[kTypeId].u.pd.value = 3;
    parameters[kOriginId].u.point3d_d.x_value = 32;
    parameters[kOriginId].u.point3d_d.y_value = 32;
    parameters[kOriginId].u.point3d_d.z_value = 32;
    parameters[kSizeId].u.fs_d.value = 4;
    parameters[kOpacityId].u.fs_d.value = 0.5;
    // Force and appearance controls (IDs 17..24) feed the same legacy snapshot.
    parameters[kGravityYId].u.fs_d.value = -0.5;
    parameters[kLinearDragId].u.fs_d.value = 0.25;
    parameters[kColorStartId].u.cd.value.red = 255;
    parameters[kColorStartId].u.cd.value.green = 0;
    parameters[kColorStartId].u.cd.value.blue = 0;
    parameters[kParticleSizeEndId].u.fs_d.value = 2.0;
    parameters[kOpacityEndId].u.fs_d.value = 0.25;
    parameters[kEmitterSizeXId].u.fs_d.value = 250.0;
    parameters[kEmitterSizeYId].u.fs_d.value = 75.0;
    parameters[kEmitterSizeZId].u.fs_d.value = 150.0;
    parameters[kParticleSizeRandomId].u.fs_d.value = 65.0;
    parameters[kOpacityRandomId].u.fs_d.value = 35.0;
    parameters[kControlSourceId].u.pd.value = kLegacyControlSource;
    CHECK(checkout_render_graph(&host, &output, snapshot, &active_source) == PF_Err_NONE);
    CHECK(checked_out.empty() && last_time == host.current_time && active_source == kLegacyControlSource);
    const auto legacy = core::serialize_graph(*snapshot, core::particle_node_registry());
    CHECK(legacy.has_value() && legacy.value() != frozen.value());

    // The current control projection follows Emitter -> Particle -> Force -> Output.
    // Appearance controls belong to Particle and force values stay on the Force node.
    const auto find_node = [](const core::Graph& graph, const char* key) {
        return std::find_if(graph.nodes.begin(), graph.nodes.end(),
                            [key](const core::GraphNode& node) { return node.type_key == key; });
    };
    const auto find_value = [](const core::GraphNode& node, core::ParameterKey key) -> const core::ParameterValue* {
        for (const auto& parameter : node.parameters) {
            if (parameter.key == key) return &parameter.value;
        }
        return nullptr;
    };
    CHECK(snapshot->nodes.size() == 4 && snapshot->edges.size() == 3);
    const auto force_node = find_node(*snapshot, core::graph_keys::kForceNode);
    const auto particle_node = find_node(*snapshot, core::graph_keys::kParticleNode);
    const auto emitter_node = find_node(*snapshot, core::graph_keys::kEmitterNode);
    const auto output_node = find_node(*snapshot, core::graph_keys::kOutputNode);
    CHECK(force_node != snapshot->nodes.end() && particle_node != snapshot->nodes.end() &&
          output_node != snapshot->nodes.end() &&
          emitter_node != snapshot->nodes.end());
    if (snapshot->nodes.size() == 4 && snapshot->edges.size() == 3) {
        CHECK(snapshot->edges[0].source_node == emitter_node->id &&
              snapshot->edges[0].destination_node == particle_node->id);
        CHECK(snapshot->edges[1].source_node == particle_node->id &&
              snapshot->edges[1].destination_node == force_node->id);
        CHECK(snapshot->edges[2].source_node == force_node->id &&
              snapshot->edges[2].destination_node == output_node->id);
    }
    if (emitter_node != snapshot->nodes.end()) {
        const auto* size_x = find_value(*emitter_node, core::graph_keys::kEmitterSizePercentX);
        const auto* size_y = find_value(*emitter_node, core::graph_keys::kEmitterSizePercentY);
        const auto* size_z = find_value(*emitter_node, core::graph_keys::kEmitterSizePercentZ);
        CHECK(size_x != nullptr && std::get<double>(*size_x) == 250.0);
        CHECK(size_y != nullptr && std::get<double>(*size_y) == 75.0);
        CHECK(size_z != nullptr && std::get<double>(*size_z) == 150.0);
    }
    if (force_node != snapshot->nodes.end()) {
        const auto* gravity = find_value(*force_node, core::graph_keys::kGravity);
        const auto* drag = find_value(*force_node, core::graph_keys::kLinearDrag);
        CHECK(gravity != nullptr && std::get<core::Vec3>(*gravity).y == -0.5);
        CHECK(drag != nullptr && std::get<double>(*drag) == 0.25);
    }
    if (particle_node != snapshot->nodes.end()) {
        const auto* color_start = find_value(*particle_node, core::graph_keys::kColorStart);
        const auto* size_end = find_value(*particle_node, core::graph_keys::kSizeEnd);
        const auto* opacity_end = find_value(*particle_node, core::graph_keys::kOpacityEnd);
        const auto* size_random = find_value(*particle_node, core::graph_keys::kSizeRandom);
        const auto* opacity_random = find_value(*particle_node, core::graph_keys::kOpacityRandom);
        CHECK(color_start != nullptr && std::get<core::Vec3>(*color_start).x == 1.0);
        CHECK(color_start != nullptr && std::get<core::Vec3>(*color_start).y == 0.0);
        CHECK(size_end != nullptr && std::get<double>(*size_end) == 2.0);
        CHECK(opacity_end != nullptr && std::get<double>(*opacity_end) == 0.25);
        CHECK(size_random != nullptr && std::get<double>(*size_random) == 65.0);
        CHECK(opacity_random != nullptr && std::get<double>(*opacity_random) == 35.0);
    }
    std::array<PF_ParamDef*, kTotalEffectParameterCount + 1> pointers{};
    for (std::size_t i = 0; i < pointers.size(); ++i) pointers[i] = &parameters[i];
    PF_UserChangedParamExtra extra{}; extra.param_index = kCaptureControlsId;
    fail_allocation = true;
    CHECK(capture_controls(&host, &output, pointers.data(), &extra) == PF_Err_OUT_OF_MEMORY);
    CHECK(parameters[kGraphParameterId].u.arb_d.value == previous && parameters[kControlSourceId].u.pd.value == kLegacyControlSource);
    CHECK(checked_out.empty());
    CHECK(capture_controls(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    // The replaced handle must stay alive: it belongs to the host, which disposes the
    // value it replaced once the parameter change is committed. Disposing it here as
    // well frees a host-owned handle and makes AE abort later.
    CHECK(handles.contains(previous)); CHECK(parameters[kControlSourceId].u.pd.value == kNodeControlSource);
    CHECK((parameters[kGraphParameterId].uu.change_flags & PF_ChangeFlag_CHANGED_VALUE) != 0);
    CHECK((parameters[kControlSourceId].uu.change_flags & PF_ChangeFlag_CHANGED_VALUE) != 0);
    CHECK(handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes == legacy.value());
    CHECK(checkout_render_graph(&host, &output, snapshot) == PF_Err_NONE);
    CHECK(core::serialize_graph(*snapshot, core::particle_node_registry()).value() == legacy.value());
    fail_checkout = kGraphParameterId;
    CHECK(checkout_render_graph(&host, &output, snapshot) != PF_Err_NONE);
    CHECK(!snapshot && checked_out.empty()); fail_checkout = -1;
    parameters[kControlSourceId].u.pd.value = kLegacyControlSource;
    fail_checkout = 8;
    CHECK(checkout_render_graph(&host, &output, snapshot) != PF_Err_NONE);
    CHECK(!snapshot && checked_out.empty()); fail_checkout = -1;
    dispose(parameters[kGraphParameterId].u.arb_d.value);
    dispose(previous); // replaced by capture; in AE the host owns and frees it
    dispose(registered[kGraphParameterId - 1].u.arb_d.dephault);
}

void test_point_control_preview_scale(PF_InData& host) {
    const A_long saved_width = host.width;
    const A_long saved_height = host.height;
    const PF_RationalScale saved_x = host.downsample_x;
    const PF_RationalScale saved_y = host.downsample_y;
    const PF_Point3DDef saved_origin = parameters[kOriginId].u.point3d_d;
    host.width = 3840;
    host.height = 2160;

    const auto check_center = [&host](PF_RationalScale downsample_x, PF_RationalScale downsample_y,
                                      double x, double y, double z) {
        host.downsample_x = downsample_x;
        host.downsample_y = downsample_y;
        parameters[kOriginId].u.point3d_d.x_value = static_cast<PF_FpLong>(x);
        parameters[kOriginId].u.point3d_d.y_value = static_cast<PF_FpLong>(y);
        parameters[kOriginId].u.point3d_d.z_value = static_cast<PF_FpLong>(z);
        ParameterSnapshot snapshot;
        CHECK(snapshot.checkout(&host, 3840, 2160) == PF_Err_NONE);
        CHECK(snapshot.valid());
        CHECK(std::abs(snapshot.settings().emitter_origin.x) < 1.0e-12);
        CHECK(std::abs(snapshot.settings().emitter_origin.y) < 1.0e-12);
        CHECK(std::abs(snapshot.settings().emitter_origin.z) < 1.0e-12);
        snapshot.checkin(&host);
        CHECK(checked_out.empty());
    };

    // AE 2023.5 Build 52 readouts: Full delivers 1920/1080/1080; Quarter delivers
    // 480/270/270 with downsample 1/4. Both values denote the same layer centre.
    check_center(PF_RationalScale{1, 1}, PF_RationalScale{1, 1}, 1920, 1080, 1080);
    check_center(PF_RationalScale{1, 4}, PF_RationalScale{1, 4}, 480, 270, 270);
    // Horizontal and vertical factors are independent; Z follows the vertical axis.
    check_center(PF_RationalScale{1, 2}, PF_RationalScale{1, 4}, 960, 270, 270);

    parameters[kOriginId].u.point3d_d = saved_origin;
    host.width = saved_width;
    host.height = saved_height;
    host.downsample_x = saved_x;
    host.downsample_y = saved_y;
}

void test_capture_scales_reduced_preview(PF_InData& host) {
    PF_OutData output{};
    parameters[kGraphParameterId].u.arb_d.value = new_value(host);
    parameters[kGraphParameterId].uu.change_flags = 0;
    parameters[kControlSourceId].u.pd.value = kLegacyControlSource;
    const PF_Point3DDef saved_origin = parameters[kOriginId].u.point3d_d;
    const PF_RationalScale saved_x = host.downsample_x;
    const PF_RationalScale saved_y = host.downsample_y;
    const PF_FpLong saved_gravity_y = parameters[kGravityYId].u.fs_d.value;
    std::array<PF_ParamDef*, kTotalEffectParameterCount + 1> pointers{};
    for (std::size_t i = 0; i < pointers.size(); ++i) pointers[i] = &parameters[i];
    PF_UserChangedParamExtra extra{}; extra.param_index = kCaptureControlsId;

    const auto previous = parameters[kGraphParameterId].u.arb_d.value;
    host.downsample_x = PF_RationalScale{1, 4};
    host.downsample_y = PF_RationalScale{1, 4};
    parameters[kOriginId].u.point3d_d.x_value = 8;
    parameters[kOriginId].u.point3d_d.y_value = 8;
    parameters[kOriginId].u.point3d_d.z_value = 8;
    CHECK(capture_controls(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    CHECK(parameters[kControlSourceId].u.pd.value == kNodeControlSource);
    CHECK(checked_out.empty());
    const auto captured = parameters[kGraphParameterId].u.arb_d.value;
    const auto captured_graph = read_graph_parameter(&host, captured);
    CHECK(captured_graph.has_value());
    if (captured_graph.has_value()) {
        const auto emitter = std::find_if(captured_graph.value().nodes.begin(), captured_graph.value().nodes.end(),
            [](const core::GraphNode& node) { return node.type_key == core::graph_keys::kEmitterNode; });
        CHECK(emitter != captured_graph.value().nodes.end());
        if (emitter != captured_graph.value().nodes.end()) {
            const auto origin = std::find_if(emitter->parameters.begin(), emitter->parameters.end(),
                [](const core::NodeParameter& parameter) { return parameter.key == core::graph_keys::kEmitterOrigin; });
            CHECK(origin != emitter->parameters.end());
            if (origin != emitter->parameters.end()) {
                const auto point = std::get<core::Vec3>(origin->value);
                CHECK(std::abs(point.x) < 1.0e-12 && std::abs(point.y) < 1.0e-12 && std::abs(point.z) < 1.0e-12);
            }
        }
    }

    // A supervised parameter edit while preview is reduced uses the same conversion
    // before replacing the stored graph; it must not bake the quarter-sized point.
    parameters[kGravityYId].u.fs_d.value = -0.5;
    extra.param_index = kGravityYId;
    CHECK(user_changed_param(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    const auto synced = parameters[kGraphParameterId].u.arb_d.value;
    const auto synced_graph = read_graph_parameter(&host, synced);
    CHECK(synced_graph.has_value());
    if (synced_graph.has_value()) {
        const auto emitter = std::find_if(synced_graph.value().nodes.begin(), synced_graph.value().nodes.end(),
            [](const core::GraphNode& node) { return node.type_key == core::graph_keys::kEmitterNode; });
        CHECK(emitter != synced_graph.value().nodes.end());
        if (emitter != synced_graph.value().nodes.end()) {
            const auto origin = std::find_if(emitter->parameters.begin(), emitter->parameters.end(),
                [](const core::NodeParameter& parameter) { return parameter.key == core::graph_keys::kEmitterOrigin; });
            CHECK(origin != emitter->parameters.end());
            if (origin != emitter->parameters.end()) {
                const auto point = std::get<core::Vec3>(origin->value);
                CHECK(std::abs(point.x) < 1.0e-12 && std::abs(point.y) < 1.0e-12 && std::abs(point.z) < 1.0e-12);
            }
        }
    }

    dispose(parameters[kGraphParameterId].u.arb_d.value);
    dispose(captured); // replaced by the supervised edit; AE owns and frees it
    dispose(previous); // replaced by capture; AE owns and frees it
    parameters[kGraphParameterId].u.arb_d.value = nullptr;
    parameters[kOriginId].u.point3d_d = saved_origin;
    parameters[kGravityYId].u.fs_d.value = saved_gravity_y;
    host.downsample_x = saved_x;
    host.downsample_y = saved_y;
}

// The supervised edit surface the dockable panel drives (ADR 0009): in Node Graph
// mode a control change rewrites the canonical graph from the delivered values; in
// AE Controls mode the stored graph is never touched.
void test_supervision(PF_InData& host) {
    PF_OutData output{};
    parameters[kGraphParameterId].u.arb_d.value = new_value(host);
    parameters[kGraphParameterId].uu.change_flags = 0;
    parameters[kControlSourceId].u.pd.value = kNodeControlSource;
    parameters[kGravityYId].u.fs_d.value = 0.0;
    parameters[kGravityYId].uu.change_flags = 0;
    std::array<PF_ParamDef*, kTotalEffectParameterCount + 1> pointers{};
    for (std::size_t i = 0; i < pointers.size(); ++i) pointers[i] = &parameters[i];
    PF_UserChangedParamExtra extra{};
    extra.param_index = kGravityYId;

    const auto previous = parameters[kGraphParameterId].u.arb_d.value;
    const auto before = handles.at(previous)->bytes;
    // The delivered array carries the accepted new value; the graph must follow it.
    parameters[kGravityYId].u.fs_d.value = -0.5;
    CHECK(user_changed_param(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    CHECK(parameters[kGraphParameterId].u.arb_d.value != previous);
    CHECK(handles.contains(previous)); // host-owned replaced value, not ours to free
    CHECK((parameters[kGraphParameterId].uu.change_flags & PF_ChangeFlag_CHANGED_VALUE) != 0);
    const auto after = handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes;
    CHECK(after != before);
    const auto graph = read_graph_parameter(&host, parameters[kGraphParameterId].u.arb_d.value);
    CHECK(graph.has_value());
    if (graph.has_value()) {
        const auto force_node = std::find_if(graph.value().nodes.begin(), graph.value().nodes.end(),
            [](const core::GraphNode& node) { return node.type_key == core::graph_keys::kForceNode; });
        CHECK(force_node != graph.value().nodes.end());
        if (force_node != graph.value().nodes.end()) {
            bool found = false;
            for (const auto& parameter : force_node->parameters) {
                if (parameter.key != core::graph_keys::kGravity) continue;
                found = true;
                CHECK(std::get<core::Vec3>(parameter.value).y == -0.5);
            }
            CHECK(found);
        }
    }

    // AE Controls mode: the stored graph is left alone.
    parameters[kControlSourceId].u.pd.value = kLegacyControlSource;
    parameters[kGravityYId].u.fs_d.value = 0.9;
    parameters[kGraphParameterId].uu.change_flags = 0;
    const auto kept = handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes;
    CHECK(user_changed_param(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    CHECK(handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes == kept);
    CHECK((parameters[kGraphParameterId].uu.change_flags & PF_ChangeFlag_CHANGED_VALUE) == 0);

    // Bookkeeping parameters never rewrite the graph.
    parameters[kControlSourceId].u.pd.value = kNodeControlSource;
    extra.param_index = kControlSourceId;
    CHECK(user_changed_param(&host, &output, pointers.data(), &extra) == PF_Err_NONE);
    CHECK(handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes == kept);

    // A failed host allocation leaves the stored graph untouched.
    extra.param_index = kGravityYId;
    fail_allocation = true;
    CHECK(user_changed_param(&host, &output, pointers.data(), &extra) != PF_Err_NONE);
    CHECK(handles.at(parameters[kGraphParameterId].u.arb_d.value)->bytes == kept);

    dispose(parameters[kGraphParameterId].u.arb_d.value);
    dispose(previous); // replaced by the supervised rewrite
    parameters[kGraphParameterId].u.arb_d.value = nullptr;
}

void test_world_copy_cancellation() {
    PF_Pixel source_pixels[4]{};
    PF_EffectWorld source_world{};
    source_world.data = source_pixels;
    source_world.width = 2;
    source_world.height = 2;
    source_world.rowbytes = static_cast<A_long>(sizeof(source_pixels[0]) * 2);

    const CancelAtPoll cancel_read_after_one_row(3); // preflight, row 0, then row 1
    const auto source = read_world(source_world, HostBitDepth::bpc8, cancel_read_after_one_row);
    CHECK(!source.has_value());
    CHECK(source.error().code == core::ErrorCode::cancelled);

    core::RenderOutput output;
    output.region = core::RectI{0, 0, 2, 2};
    output.format = core::PixelFormat::rgba8;
    output.row_bytes = 8;
    output.pixels.assign(16, std::byte{0});
    output.pixels[0] = std::byte{1};
    output.pixels[1] = std::byte{2};
    output.pixels[2] = std::byte{3};
    output.pixels[3] = std::byte{4};

    PF_Pixel destination_pixels[4]{};
    for (auto& pixel : destination_pixels) {
        pixel.red = 40; pixel.green = 80; pixel.blue = 120; pixel.alpha = 255;
    }
    PF_EffectWorld destination_world{};
    destination_world.data = destination_pixels;
    destination_world.width = 2;
    destination_world.height = 2;
    destination_world.rowbytes = static_cast<A_long>(sizeof(destination_pixels[0]) * 2);
    const WorldLayout layout{0, 0, 2, 2, static_cast<std::uint32_t>(destination_world.rowbytes)};
    // Two world-clear rows run before output-copy row 0; cancel before copy row 1.
    const CancelAtPoll cancel_write_after_one_row(4);
    CHECK(!write_output(output, layout, destination_world, HostBitDepth::bpc8, cancel_write_after_one_row));
    CHECK(destination_pixels[0].red == 1 && destination_pixels[0].green == 2 &&
          destination_pixels[0].blue == 3 && destination_pixels[0].alpha == 4);
    CHECK(destination_pixels[2].alpha == 0); // the second row was not copied after cancellation
}

void test_world_output_is_transparent_outside_particles() {
    core::RenderOutput output;
    output.region = core::RectI{0, 0, 1, 1};
    output.format = core::PixelFormat::rgba8;
    output.row_bytes = 4;
    output.pixels = {std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};

    PF_Pixel destination_pixels[4]{};
    for (auto& pixel : destination_pixels) {
        pixel.red = 40; pixel.green = 80; pixel.blue = 120; pixel.alpha = 255;
    }
    PF_EffectWorld destination_world{};
    destination_world.data = destination_pixels;
    destination_world.width = 2;
    destination_world.height = 2;
    destination_world.rowbytes = static_cast<A_long>(sizeof(destination_pixels[0]) * 2);
    const WorldLayout layout{0, 0, 2, 2, static_cast<std::uint32_t>(destination_world.rowbytes)};
    const CancelAtPoll no_cancel(100);

    CHECK(write_output(output, layout, destination_world, HostBitDepth::bpc8, no_cancel));
    CHECK(destination_pixels[0].red == 1 && destination_pixels[0].green == 2 &&
          destination_pixels[0].blue == 3 && destination_pixels[0].alpha == 4);
    for (std::size_t i = 1; i < std::size(destination_pixels); ++i) {
        CHECK(destination_pixels[i].red == 0 && destination_pixels[i].green == 0 &&
              destination_pixels[i].blue == 0 && destination_pixels[i].alpha == 0);
    }

    output.region = core::RectI{0, 0, 0, 0};
    output.pixels.clear();
    for (auto& pixel : destination_pixels) {
        pixel.red = 40; pixel.green = 80; pixel.blue = 120; pixel.alpha = 255;
    }
    CHECK(write_output(output, layout, destination_world, HostBitDepth::bpc8, no_cancel));
    for (const auto& pixel : destination_pixels) {
        CHECK(pixel.red == 0 && pixel.green == 0 && pixel.blue == 0 && pixel.alpha == 0);
    }

    for (auto& pixel : destination_pixels) {
        pixel.red = 40; pixel.green = 80; pixel.blue = 120; pixel.alpha = 255;
    }
    PF_EffectWorld invalid_stride_world = destination_world;
    invalid_stride_world.rowbytes = -1;
    const WorldLayout invalid_stride_layout{0, 0, 2, 2,
                                            static_cast<std::uint32_t>(invalid_stride_world.rowbytes)};
    CHECK(!write_output(output, invalid_stride_layout, invalid_stride_world, HostBitDepth::bpc8, no_cancel));
    CHECK(destination_pixels[0].red == 40 && destination_pixels[0].alpha == 255);
    CHECK(destination_pixels[3].blue == 120 && destination_pixels[3].alpha == 255);
}
} // namespace

int main() {
    PF_UtilCallbacks utils{};
    utils.host_new_handle = allocate; utils.host_lock_handle = lock;
    utils.host_unlock_handle = unlock; utils.host_dispose_handle = dispose; utils.host_get_handle_size = size_of;
    PF_InData host{}; host.utils = &utils;
    host.inter.checkout_param = checkout; host.inter.checkin_param = checkin; host.inter.add_param = add_param;
    host.width = 64; host.height = 64; host.current_time = 24; host.time_step = 1; host.time_scale = 24;
    host.pixel_aspect_ratio = {1, 1};
    // effect_ref intentionally null: arbitrary callbacks must work without one.
    test_callbacks(host);
    test_parameters(host);
    test_point_control_preview_scale(host);
    test_supervision(host);
    test_capture_scales_reduced_preview(host);
    test_world_copy_cancellation();
    test_world_output_is_transparent_outside_particles();
    CHECK(handles.empty() && checked_out.empty());
    std::printf("%d adapter checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
