// Uses the local Adobe SDK declarations and fake host callbacks, not an AE process.
#include "GraphParameter.hpp"
#include "Parameters.hpp"
#include "AE_EffectCB.h"
#include "starfield/core/SequenceCodec.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <variant>
#include <vector>

using namespace starfield;
using namespace starfield::adapter;
namespace {
int checks = 0, failures = 0;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
struct Memory { core::OpaqueBytes bytes; unsigned locks{0}; };
std::unordered_map<PF_Handle, std::unique_ptr<Memory>> handles;
bool fail_allocation = false, fail_lock = false;
std::array<PF_ParamDef, 33> parameters{};
std::vector<PF_ParamDef> registered;
std::unordered_map<PF_ParamDef*, A_long> checked_out;
std::vector<A_long> checked_indices;
A_long fail_checkout = -1;
A_long last_time = 0;

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
    if (index < 1 || index > 32 || !value) return PF_Err_BAD_CALLBACK_PARAM;
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
    CHECK(output.num_params == 33 && registered.size() == 32);
    const auto& source = registered[kControlSourceId - 1];
    // Manifest revision 6: a fresh effect must drive the visible controls, so both the
    // default and the old-project value select AE Controls. Node Graph is opt-in.
    CHECK(source.u.pd.dephault == kLegacyControlSource && source.u.pd.value == kLegacyControlSource);
    CHECK((source.flags & PF_ParamFlag_USE_VALUE_FOR_OLD_PROJECTS) != 0);
    // The SDK's own PF_ADD_ARBITRARY2 passes no PF_ParamFlags; arbitrary data cannot be
    // animated, and the two flags this used to carry were never part of the contract.
    CHECK(registered[kGraphParameterId - 1].flags == 0);
    CHECK(registered[kGraphParameterId - 1].u.arb_d.value == nullptr);
    for (std::size_t i = 1; i <= 32; ++i) {
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
    parameters[kControlSourceId].u.pd.value = kLegacyControlSource;
    CHECK(checkout_render_graph(&host, &output, snapshot, &active_source) == PF_Err_NONE);
    CHECK(checked_out.empty() && last_time == host.current_time && active_source == kLegacyControlSource);
    const auto legacy = core::serialize_graph(*snapshot, core::particle_node_registry());
    CHECK(legacy.has_value() && legacy.value() != frozen.value());

    // The single-emitter chain is emitter -> force -> appearance -> output, and the
    // delivered control values reach the force and appearance nodes.
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
    const auto appearance_node = find_node(*snapshot, core::graph_keys::kAppearanceNode);
    CHECK(force_node != snapshot->nodes.end() && appearance_node != snapshot->nodes.end());
    if (force_node != snapshot->nodes.end()) {
        const auto* gravity = find_value(*force_node, core::graph_keys::kGravity);
        const auto* drag = find_value(*force_node, core::graph_keys::kLinearDrag);
        CHECK(gravity != nullptr && std::get<core::Vec3>(*gravity).y == -0.5);
        CHECK(drag != nullptr && std::get<double>(*drag) == 0.25);
    }
    if (appearance_node != snapshot->nodes.end()) {
        const auto* color_start = find_value(*appearance_node, core::graph_keys::kColorStart);
        const auto* size_end = find_value(*appearance_node, core::graph_keys::kSizeEnd);
        const auto* opacity_end = find_value(*appearance_node, core::graph_keys::kOpacityEnd);
        CHECK(color_start != nullptr && std::get<core::Vec3>(*color_start).x == 1.0);
        CHECK(color_start != nullptr && std::get<core::Vec3>(*color_start).y == 0.0);
        CHECK(size_end != nullptr && std::get<double>(*size_end) == 2.0);
        CHECK(opacity_end != nullptr && std::get<double>(*opacity_end) == 0.25);
    }
    std::array<PF_ParamDef*, 33> pointers{};
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
    std::array<PF_ParamDef*, 33> pointers{};
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
    test_supervision(host);
    CHECK(handles.empty() && checked_out.empty());
    std::printf("%d adapter checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
