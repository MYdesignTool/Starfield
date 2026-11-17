#include "NodeEffects.hpp"
#include "NativeNodeGraph.hpp"
#include "NativeClearRequest.hpp"
#include "NativeRetiredAlias.hpp"
#include "NativeSourceState.hpp"
#include "NativeTemporalCache.hpp"
#include "NativeTemporalUI.hpp"
#include "UiExclusionClient.hpp"
#include <string>
#include <fstream>
#include "NodeRecord.hpp"
#include "EffectGraphBackup.hpp"
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
#include "ParticleGradientUI.hpp"
#if defined(STARFIELD_OVERLIFE_TEST)
#include "../artifacts/generated/EditorPresetCatalog.hpp"
namespace starfield::adapter { extern int overlife_preset_index; }
#endif
#include "TransformBinding.hpp"
#include "MotionBlur.hpp"
#include "starfield/core/CpuRenderer.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <array>
#include <bit>
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <limits>
#include <unordered_map>
#include <vector>
#include <thread>
#include <functional>
#include <new>

#if defined(STARFIELD_TEST_NATIVE_ALLOCATIONS)
namespace {thread_local long long native_fail_allocation_after=-1;thread_local bool native_allocation_failed{};}
_Use_decl_annotations_ void* operator new(std::size_t size) {
    if(native_fail_allocation_after>=0 && native_fail_allocation_after--==0){native_allocation_failed=true;throw std::bad_alloc{};}
    if(auto* p=std::malloc(size?size:1))return p;throw std::bad_alloc{};
}
_Use_decl_annotations_ void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
#endif

int run_camera_capture_tests();
namespace {
using namespace starfield::adapter;
namespace core = starfield::core;
namespace records = starfield::adapter::native_nodes;
int checks{}, failures{}, calls{}, sets{}, live_refs{}, acquisitions{}, suite_requests{}, aegp_suite_requests{},opened_streams{};
int null_pf_layer_queries{};
void check(bool condition, const char* message) {
    ++checks; if (!condition) { ++failures; std::printf("FAILED: %s\n", message); }
}
bool ignore_graph{};
A_long fail_set = -1;
A_long fail_read = -1;
A_long fail_evaluated_read = -1;
A_long wrong_type = -1;
bool corrupt_receipt{};
bool panel_publish{};
bool panel_output_only{};
AEGP_EffectFlags renderer_flags=AEGP_EffectFlags_ACTIVE;
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
bool silent_motion_write{};
bool silent_motion_guard_release{};
bool fail_motion_restore{};
bool motion_lifetime_checks{};
unsigned motion_stream_epoch{}, stale_motion_accesses{};
#endif
std::function<void()> alias_mutation_hook;
void sample_after_alias_mutation() {if(alias_mutation_hook)alias_mutation_hook();}
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
    std::u16string display_name;
    std::vector<AEGP_StreamVal2> values;
    std::vector<PF_ParamDef> params;
};
std::array<Fixture, 9> fixtures;
bool all_kinds_present{};
bool transform_present{};
bool model_present{};
bool cloud_animation_test{};
bool birth_animation_test{};
using Matrix=transform_binding::Matrix;
Matrix null_world{1,0,0,910,0,1,0,490,0,0,1,0,0,0,0,1};
Matrix owner_world{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
core::Vec3 null_anchor{50,50,0};
int affine_reads{},synthetic_native_reads{};
struct Ref { std::size_t effect{}; A_long index{}; unsigned motion_epoch{}; };
bool stale_motion_ref(AEGP_StreamRefH ref) {
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
    const auto& key=*reinterpret_cast<Ref*>(ref);
    if(motion_lifetime_checks && key.effect==8 && key.motion_epoch!=motion_stream_epoch) {
        ++stale_motion_accesses;return true;
    }
#endif
    return false;
}
std::size_t effect_index(AEGP_EffectRefH value) { return reinterpret_cast<std::size_t>(value) - 1; }
AEGP_EffectRefH effect_ref(std::size_t i) { return reinterpret_cast<AEGP_EffectRefH>(i + 1); }
AEGP_PFInterfaceSuite1 pf{};
AEGP_EffectSuite4 effect{};
AEGP_StreamSuite6 stream{};
AEGP_DynamicStreamSuite4 dynamic{};
bool dynamic_enabled{};
bool dynamic_unavailable{};
std::array<AEGP_DynStreamFlags,records::parameter_count(records::Kind::particle)> visibility_flags{};
int visibility_sets{};
AEGP_UtilitySuite6 utility{};
AEGP_MemorySuite1 memory{};
PF_ParamUtilsSuite3 param_utils{};
AEGP_KeyframeSuite5 keyframes{};
bool temporal_metadata_enabled{}, metadata_rate_keys{}, metadata_life_keys{}, metadata_bezier{}, metadata_life_expression{}, metadata_chance_expression{};unsigned metadata_epoch{};
unsigned metadata_whole_queries{},metadata_whole_fail_at{};bool metadata_whole_change_at_finish{};
std::uint64_t metadata_state_queries{};
std::array<std::u16string, kNativeBindingCapacity> expressions;
std::array<A_Boolean, kNativeBindingCapacity> expression_enabled{};
bool lazy_dependencies{};
std::array<bool,kNativeBindingCapacity> dependencies_evaluated{};
std::array<unsigned,kNativeBindingCapacity> dependency_generations{};
A_long nonfinite_alias=-1, unresolved_alias=-1, stale_dependency_alias=-1,stale_retirement_alias=-1;
int ignore_retired_text=-1,partial_retired_text=-1,enable_retired_text=-1,corrupt_retired_base=-1,ignore_retired_disable=-1;
int retired_readback_slot=-1,retired_readback_mode{};bool retired_readback_armed{};
AEGP_MemHandle null_retired_lock{};
unsigned absent_source_uuid{};
bool external_force_removed{};
A_long fail_expression = -1;
unsigned expression_clear_count{};
bool disable_next_expression{};
AEGP_LayerSuite9 layers{};
AEGP_ItemSuite9 items{};
PF_InData renderer_data{};
AEGP_ProjSuite6 source_projects{};
bool source_probe_environment{},source_probe_supported=true,source_probe_null{},source_probe_wrong_context{},source_probe_state_error{},source_probe_compare_error{};
bool source_probe_empty_project{};
bool source_probe_sdk_different{};
bool source_probe_delay{},source_probe_delay_read{};
bool source_probe_reorder{};
std::u16string source_probe_project=u"D:\\test.aep";
std::array<unsigned,9> source_identity_epochs{};
unsigned source_probe_calls{},source_probe_nonce{},source_probe_comparisons{};
std::array<unsigned,9> source_epochs{},source_reads{};
int source_change_during_read=-1;
A_Err acquire(const char* name, int32, const void** out) {
    ++suite_requests;
    if (std::strstr(name,"AEGP")) ++aegp_suite_requests;
    if (!std::strcmp(name, kAEGPPFInterfaceSuite)) *out = &pf;
    else if (!std::strcmp(name, kAEGPEffectSuite)) *out = &effect;
    else if (!std::strcmp(name, kAEGPStreamSuite)) *out = &stream;
    else if (!dynamic_unavailable && !std::strcmp(name,kAEGPDynamicStreamSuite))*out=&dynamic;
    else if (!std::strcmp(name, kAEGPUtilitySuite)) *out = &utility;
    else if (!std::strcmp(name, kAEGPLayerSuite)) *out = &layers;
    else if (!std::strcmp(name, kAEGPItemSuite)) *out = &items;
    else if (!std::strcmp(name, kAEGPMemorySuite)) *out = &memory;
    else if (source_probe_environment && !std::strcmp(name,kAEGPProjSuite)) *out=&source_projects;
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
PF_Err source_probe_generic(AEGP_PluginID plugin,AEGP_EffectRefH ref,const A_Time*,PF_Cmd command,void* message) {
    if(!source_probe_environment || !message || command!=PF_Cmd_COMPLETELY_GENERAL ||
       *static_cast<const std::uint32_t*>(message)!=0x53535031 || !effect_index(ref)) {
        ++calls;check(false,"renderer/unknown cross-effect generic calls remain forbidden");return PF_Err_BAD_CALLBACK_PARAM;
    }
    ++source_probe_calls;check(plugin==701,"source probe uses registered adapter ID");
    if(source_probe_delay)std::this_thread::sleep_for(std::chrono::milliseconds(2));
    auto& request=*static_cast<NativeSourceStateRequest*>(message);
    if(!source_probe_supported)return PF_Err_NONE;
    PF_ParamUtilsSuite3 source_utils{};
    source_utils.PF_GetCurrentState=[](PF_ProgPtr context,PF_ParamIndex index,const A_Time* start,const A_Time* duration,PF_State* state)->PF_Err {
        check(index==PF_ParamIndex_CHECK_ALL_EXCEPT_LAYER_PARAMS && !start && !duration,"source state covers all numeric author times");
        if(source_probe_state_error)return PF_Err_BAD_CALLBACK_PARAM;
        const auto id=effect_index(reinterpret_cast<AEGP_EffectRefH>(context));
        *state={};std::memcpy(state,&source_epochs[id],sizeof(unsigned));
        // Opaque tokens can differ bytewise while the SDK considers them equal.
        const auto noise=source_probe_sdk_different?0:++source_probe_nonce;std::memcpy(reinterpret_cast<char*>(state)+sizeof(unsigned),&noise,sizeof(noise));return PF_Err_NONE;
    };
    source_utils.PF_AreStatesIdentical=[](PF_ProgPtr,const PF_State* a,const PF_State* b,A_Boolean* same)->PF_Err {
        ++source_probe_comparisons;if(source_probe_compare_error)return PF_Err_BAD_CALLBACK_PARAM;
        unsigned av{},bv{};std::memcpy(&av,a,sizeof(av));std::memcpy(&bv,b,sizeof(bv));*same=!source_probe_sdk_different && av==bv;return PF_Err_NONE;
    };
    // The real source handler must verify its own PF context through PFInterface.
    SPBasicSuite proxy=*renderer_data.pica_basicP;
    static const PF_ParamUtilsSuite3* active_source_utils{};active_source_utils=&source_utils;
    proxy.AcquireSuite=[](const char* name,int32 version,const void** out)->SPErr {
        if(!std::strcmp(name,kPFParamUtilsSuite)){*out=active_source_utils;++acquisitions;return 0;}
        return acquire(name,version,out);
    };
    auto data=renderer_data;data.pica_basicP=&proxy;
    data.effect_ref=source_probe_wrong_context?renderer_data.effect_ref:reinterpret_cast<PF_ProgPtr>(ref);
    const auto result=read_native_source_state(source_probe_null?nullptr:&data,request.kind,request);
    check(result==PF_Err_NONE,"optional source proof failure stays silent");return result;
}
void compiler_reuse_tests() {
    const auto before_handles=handles.size();
    auto saved=std::make_unique<decltype(fixtures)>();*saved=fixtures;
    (*saved)[0].values[kGraphParameterId].arbH=reinterpret_cast<AEGP_ArbBlockVal>(clone(reinterpret_cast<PF_Handle>(fixtures[0].values[kGraphParameterId].arbH)));
    const auto old_expressions=std::make_unique<decltype(expressions)>(expressions);
    const auto old_enabled=expression_enabled;
    fixtures[0].values[kGraphRevisionId].one_d=1000;
    source_probe_environment=true;invalidate_native_compile_reuse();
    struct ProbeResult {std::uint32_t reused_nodes{},state_queries{};std::uint64_t query_us{},full_read_us{};};
    const auto run=[](std::uint32_t kind=1,int index=5,double value=21) {
        EffectUiExclusion owner(true);check(bool(owner),"compiler proof owns actual resident source exclusion");
        // NativeEdit contains a large delivered curve bank. Keep this matrix's
        // many temporary results off the already instrumented main stack.
        auto request=std::unique_ptr<node_sync::NativeEdit>(new node_sync::NativeEdit(edit(kind,index,value)));
        const auto result=direct_edit(*request);
        check(request->compile_us>=request->query_us && request->compile_us>=request->full_read_us,
            "query and full-node read timing are contained within compilation");
        check(result==0 && request->accepted,"proof optimization preserves successful native edit");return ProbeResult{request->reused_nodes,request->state_queries,request->query_us,request->full_read_us};
    };
    auto cold=run();check(cold.reused_nodes==0 && cold.state_queries==4,"cold edit brackets two unchanged authors");
    source_reads={};auto warm=run();
    check(warm.reused_nodes==2 && warm.state_queries==2,"warm Particle edit reuses Emitter and Force by SDK state");
    const auto warm_reads=source_reads;
    source_probe_delay=source_probe_delay_read=true;const auto observed=run();source_probe_delay=source_probe_delay_read=false;
    check(observed.query_us>=2000 && observed.full_read_us>=1000,"timers independently capture expensive source queries and full target reads");
    invalidate_native_compile_reuse();source_reads={};cold=run();
    check(cold.reused_nodes==0 && source_reads[1]>warm_reads[1] && source_reads[3]>warm_reads[3] && source_reads[2]==warm_reads[2],
        "reuse removes unrelated parameter reads and retains target reads");
    std::printf("compiler reuse reads: cold %u/%u/%u/%u, warm %u/%u/%u/%u; queries cold %u warm %u\n",
        source_reads[0],source_reads[1],source_reads[2],source_reads[3],warm_reads[0],warm_reads[1],warm_reads[2],warm_reads[3],cold.state_queries,warm.state_queries);
    ++source_epochs[1];fixtures[1].values[3].one_d=177;
    auto changed=run();check(changed.reused_nodes==1,"external Emitter parameter change forces only that author's raw read");
    check(std::get<double>(parameter(saved_graph(),core::graph_keys::kEmitterNode,core::graph_keys::kBirthRate))==177,
        "external source edit reaches the published graph");
    check(run().reused_nodes==2,"fresh external source proof is retained after publication");
    ++source_identity_epochs[1];check(run().reused_nodes==1,"fresh stream identity prevents proof reuse on replacement");
    check(run().reused_nodes==2,"replacement identity is proven by a new complete read");
    source_probe_reorder=true;check(run().reused_nodes==2,"effect reorder uses UUID/unique stream rather than parade index");
    source_probe_reorder=false;check(run().reused_nodes==2,"reordered raw records stay paired with source UUID");
    {
        EffectUiExclusion owner(true);uuid(3,records::uuid_first_index(records::Kind::force),1);
        auto duplicate=edit(1,5,21);check(direct_edit(duplicate)!=0 && !duplicate.accepted,"duplicate source identity is still rejected before publication with a warm cache");
        uuid(3,records::uuid_first_index(records::Kind::force),4);
    }
    check(run().reused_nodes==2,"rejected duplicate candidate does not replace accepted proof");
    check(run(3,1,1080).reused_nodes==1,"switching edit target fully reads the previously edited Particle");
    check(run(3,1,1080).reused_nodes==2,"repeated Force edit can reuse proven zero-resource Particle");
    fixtures[2].values[records::particle_layout::texture_front].layer_id=77;
    check(run(3,1,1080).reused_nodes==1,"new texture layer prevents Particle numeric-only reuse even with identical SDK state");
    check(run(3,1,1080).reused_nodes==1,"nonzero texture reference remains on complete resource path");
    fixtures[2].values[records::particle_layout::texture_front].layer_id=0;
    check(run(3,1,1080).reused_nodes==1,"texture removal cannot reuse a record containing an old resource");
    check(run(3,1,1080).reused_nodes==2,"zero-resource Particle can reenter reuse only after fresh publication");
    for(int mode=0;mode<4;++mode) {
        {EffectUiExclusion owner(true);auto request=edit(3,1,1080);
        if(mode==0)request.time=1;if(mode==1)request.width=1921;if(mode==2)request.pixel_aspect={2,1};if(mode==3)request.time_scale=25;
        check(direct_edit(request)==0 && request.accepted && request.reused_nodes==0,"frame/geometry/PAR/scale change falls back without reusing prior raw data");}
        check(run(3,1,1080).reused_nodes==0,"returning context cannot resurrect historical cache entry");
    }
    std::thread invalidator([]{invalidate_native_compile_reuse();});invalidator.join();
    check(run().reused_nodes==0,"worker lifecycle invalidation is atomic and drops UI proofs");
    source_change_during_read=1;invalidate_native_compile_reuse();run();
    check(run().reused_nodes==1,"state mutation during full source read cannot acquire a proof");
    check(run().reused_nodes==2,"stable second read establishes missing proof");
    source_probe_sdk_different=true;
    check(run().reused_nodes==0 && run().reused_nodes==0,"byte-identical PF tokens cannot override SDK different result");
    source_probe_sdk_different=false;check(run().reused_nodes==0 && run().reused_nodes==2,"SDK stable result must reestablish bracketing proofs after unstable reads");
    source_probe_project=u"D:\\other.aep";check(run().reused_nodes==0,"new project path cannot use previous project receipts");
    source_probe_project=u"D:\\test.aep";check(run().reused_nodes==0,"project return cannot resurrect earlier project receipts");
    check(run().reused_nodes==2,"stable current project can warm again");
    {
        const auto module=GetModuleHandleW(L"StarfieldHost.aex");
        const auto begin=std::bit_cast<std::uint64_t(*)() noexcept>(GetProcAddress(module,"SFLD_BeginGraphMutationV1"));
        const auto end=std::bit_cast<void(*)(std::uint64_t) noexcept>(GetProcAddress(module,"SFLD_EndGraphMutationV1"));
        EffectUiExclusion owner;const auto mutation=begin();check(mutation!=0,"physical mutation advances resident epoch");end(mutation);
    }
    check(run().reused_nodes==0 && run().reused_nodes==2,"physical mutation invalidates prior raw receipts before warming again");
    {
        EffectUiExclusion owner(true);auto rejected=edit(1,5,31);fail_set=kGraphParameterId;
        check(direct_edit(rejected)!=0 && !rejected.accepted,"failed publication remains failed with reuse enabled");
    }
    check(run().reused_nodes==2,"failed candidate does not replace the last accepted proof/bytes");
    auto historic=clone(reinterpret_cast<PF_Handle>(fixtures[0].values[kGraphParameterId].arbH));
    run(3,1,1080);
    dispose(reinterpret_cast<PF_Handle>(fixtures[0].values[kGraphParameterId].arbH));
    fixtures[0].values[kGraphParameterId].arbH=reinterpret_cast<AEGP_ArbBlockVal>(historic);
    check(run().reused_nodes==0,"restored historical graph bytes do not match latest accepted publication");
    for(int failure=0;failure<5;++failure) {
        invalidate_native_compile_reuse();source_probe_supported=failure!=0;source_probe_null=failure==1;
        source_probe_wrong_context=failure==2;source_probe_state_error=failure==3;source_probe_compare_error=failure==4;
        auto fallback=run();check(fallback.reused_nodes==0,"unsupported/null/wrong-PF/error probe retains full compiler");
        const auto queries=source_probe_calls;check(run().state_queries==0 && source_probe_calls==queries,"unavailable source proof is negatively cached until lifecycle reset");
        source_probe_supported=true;source_probe_null=source_probe_wrong_context=source_probe_state_error=source_probe_compare_error=false;
    }
    invalidate_native_compile_reuse();source_probe_empty_project=true;
    check(run().state_queries==0 && run().reused_nodes==0,"unsaved project takes original compiler without source probes");source_probe_empty_project=false;
    invalidate_native_compile_reuse();check(run().reused_nodes==0 && run().reused_nodes==2,"valid context resumes after lifecycle reset");
    {
        auto data=renderer_data;data.effect_ref=reinterpret_cast<PF_ProgPtr>(effect_ref(1));
        NativeSourceStateRequest request;request.plugin=701;request.kind=0;request.identity_stream=-1;
        EffectUiExclusion owner(true);
        check(read_native_source_state(&data,0,request)==0 && request.acknowledged && !request.supported,"source stream identity mismatch is silent and unsupported");
        request={};request.plugin=701;request.kind=1;
        check(read_native_source_state(&data,0,request)==0 && !request.supported,"source-kind mismatch cannot prove another node");
    }
    source_probe_environment=false;invalidate_native_compile_reuse();
    dispose(reinterpret_cast<PF_Handle>(fixtures[0].values[kGraphParameterId].arbH));fixtures=std::move(*saved);
    expressions=*old_expressions;expression_enabled=old_enabled;
    check(source_probe_comparisons>0 && live_refs==0 && acquisitions==0 && handles.size()==before_handles,"proof contexts retain no SDK references or PF handles");
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
    if(index==unresolved_alias)return kNativeBindingUnavailable;
    if(index==stale_dependency_alias)return kNativeBindingUnavailable;
    if(index==stale_retirement_alias)return kNativeBindingUnavailable;
    const auto& expr = expressions[index-kNativeBindingFirstIndex];
    if (expr.empty() || !expression_enabled[index-kNativeBindingFirstIndex] || native_retired_alias::owns(expr)) return fixtures[0].values[index].one_d;
    if(expr==u"0")return 0; // A valid authored constant, including handoff reads.
    const auto identity=[&](unsigned value) {
        const auto number=[](unsigned n){const auto s=std::to_string(n);return std::u16string(s.begin(),s.end());};
        return expr.find(u"fx.param(\"Node UUID 7\").value === "+number(value))!=std::u16string::npos;
    };
    if(absent_source_uuid && identity(absent_source_uuid))return kNativeBindingUnavailable;
    std::size_t id = identity(8) ? 8 : identity(7) ? 7 : identity(6) ? 6 : identity(5) ? 4 : identity(1) ? 1 : identity(2) ? 2 : 3;
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
        if(id==8 && source==records::motion_layout::origin_xy)return component?value.two_d.y:value.two_d.x;
        return component==0 ? value.color.redF*(1.0-time/48.0) : component==1 ? value.color.greenF : value.color.blueF;
    }
    if(cloud_animation_test && id==2 && source>=records::particle_layout::cloud_circles && source<=records::particle_layout::cloud_density)
        return value.one_d+time/24.0;
    if(birth_animation_test && id==2 && source==records::particle_layout::seed_shift)return value.one_d+time/24.0;
    if(birth_animation_test && id==2 && source==records::particle_layout::birth_chance)return value.one_d+time;
    return (id==2 && source==5) || (id==3 && source==1) ? value.one_d+time :
        (id==2 && source==8) ? 100.0-time : (id==2 && source==2) ? 2.0+time/24.0 : value.one_d;
}
#if defined(_MSC_VER)
__declspec(noinline)
#endif
void final_refresh_test(const core::Graph& prior,PF_OutData& output) {
    auto& renderer=fixtures[0];
    // Adversarial dependency cache, not an AE emulator. Explicit reinstallation
    // is required even when the expression text still names the same live source.
    stale_dependency_alias=kNativeBindingFirstIndex;
    auto unavailable=prior;auto checked=renderer_data;
    checked.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex i,A_long t,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
        *out={};out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=evaluated_binding(i,t);return 0;};
    checked.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
    check(sample_native_node_animation(&checked,unavailable,1920,1080)==PF_Err_BAD_CALLBACK_PARAM,"stale final dependency remains a strict render failure");
    const auto revision=renderer.values[kGraphRevisionId].one_d,receipt=renderer.values[kGraphEditReceiptId].one_d;
    const auto bytes=core::serialize_graph(prior,core::particle_node_registry()).value();
    const auto spare=kNativeBindingCapacity-1;const auto spare_text=expressions[spare];
    expressions[spare]=u"user text";expression_enabled[spare]=FALSE;
    check(refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output)==0 && stale_dependency_alias==-1,"final refresh rebuilds unchanged dependencies");
    check(expressions[spare]==u"user text" && !expression_enabled[spare],"final retirement sweep preserves non-native expression text");expressions[spare]=spare_text;
    auto refreshed=saved_graph();check(sample_native_node_animation(&checked,refreshed,1920,1080)==0,"refreshed seven-kind graph renders without fallback");
    check(renderer.values[kGraphRevisionId].one_d==revision && renderer.values[kGraphEditReceiptId].one_d==receipt && core::serialize_graph(saved_graph(),core::particle_node_registry()).value()==bytes,"final refresh preserves graph/revision/receipt");
    renderer.values[kGraphSyncGuardId].one_d=0;
    check(refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output)==PF_Err_BAD_CALLBACK_PARAM,"unguarded final refresh rejects");renderer.values[kGraphSyncGuardId].one_d=1;
}
#if defined(_MSC_VER)
__declspec(noinline)
#endif
void final_retirement_test(const core::Graph& prior,PF_InData& render_data,PF_OutData& output) {
    auto& renderer=fixtures[0];
    stale_retirement_alias=kNativeBindingFirstIndex;auto old=prior;
    check(sample_native_node_animation(&render_data,old,1920,1080)==PF_Err_BAD_CALLBACK_PARAM,"adversarial old frame retains retired dependency");
    const auto base=renderer.values[kNativeBindingFirstIndex].one_d;
    check(refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output)==0 && stale_retirement_alias==-1 &&
        renderer.values[kNativeBindingFirstIndex].one_d==base && native_retired_alias::owns(expressions[0]) && !expression_enabled[0],"final refresh restamps retirement with exact handoff");
    old=prior;check(sample_native_node_animation(&render_data,old,1920,1080)==0,"old frame reads finite final retirement handoffs");
}
void retirement_marker_matrix_tests() {
    const auto original_text=std::make_unique<decltype(expressions)>(expressions);
    const auto original_enabled=expression_enabled;
    const auto original_values=std::make_unique<decltype(fixtures[0].values)>(fixtures[0].values);
    const A_long slot=kNativeBindingCapacity-1,index=kNativeBindingFirstIndex+slot;
    auto restore=[&]{expressions=*original_text;expression_enabled=original_enabled;fixtures[0].values=*original_values;
        ignore_retired_text=partial_retired_text=enable_retired_text=corrupt_retired_base=ignore_retired_disable=-1;
        retired_readback_slot=-1;retired_readback_mode=0;retired_readback_armed=false;null_retired_lock=nullptr;};
    auto run=[&]{fixtures[0].values[kGraphSyncGuardId].one_d=1;PF_OutData output{};
        return refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output);};
    for(double base:{0.0,7.25})for(auto before:{std::u16string_view{},native_retired_alias::a,native_retired_alias::b}) {
        restore();expressions[slot]=before;expression_enabled[slot]=FALSE;fixtures[0].values[index].one_d=base;
        check(run()==0 && expressions[slot]==native_retired_alias::next(before) && !expression_enabled[slot] &&
            fixtures[0].values[index].one_d==base,"zero/nonzero retirement uses a different checked source-free marker");
        const auto first=expressions[slot];
        check(run()==0 && expressions[slot]!=first && native_retired_alias::owns(expressions[slot]) &&
            fixtures[0].values[index].one_d==base,"repeated final refresh changes text without changing exact handoff");
    }
    for(int failure=0;failure<9;++failure) {
        restore();expressions[slot]=native_retired_alias::a;expression_enabled[slot]=FALSE;fixtures[0].values[index].one_d=0;
        const auto expected=std::make_unique<decltype(expressions)>(expressions);
        if(failure==0)ignore_retired_text=slot;
        if(failure==1)partial_retired_text=slot;
        if(failure==2)corrupt_retired_base=slot;
        if(failure==3){enable_retired_text=slot;ignore_retired_disable=slot;}
        if(failure==4)fail_expression=slot;
        if(failure>=5){retired_readback_slot=slot;retired_readback_mode=failure-4;retired_readback_armed=false;}
        check(run()!=0,"silent/partial/base corruption/ignored disabling/reported marker failure rejects");
        check(expressions==*expected && expression_enabled==original_enabled && fixtures[0].values[index].one_d==0,
            "failed retirement restores exact earlier text, state and zero base");
    }
    restore();enable_retired_text=slot;
    check(run()==0 && native_retired_alias::owns(expressions[slot]) && !expression_enabled[slot],
        "host auto-enabling marker is explicitly disabled and checked");
    restore();expressions[slot]=u"user text";fixtures[0].values[index].one_d=4;
    check(run()==0 && expressions[slot]==u"user text" && fixtures[0].values[index].one_d==4,
        "final refresh does not claim foreign nonempty expression text");
    restore();{
        NativeBindingTransaction tx(&renderer_data,701,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1));
        const auto graph=saved_graph();check(tx.install(graph,nullptr,nullptr,nullptr,nullptr,true)==0,"capability fixture installs first");
        const auto prior=std::make_unique<decltype(expressions)>(expressions);
        const auto get_size=memory.AEGP_GetMemHandleSize;memory.AEGP_GetMemHandleSize=nullptr;
        const auto rejected=tx.retire_after_publication();memory.AEGP_GetMemHandleSize=get_size;
        check(rejected!=0 && expressions==*prior,"lost required readback capability rejects retirement before mutation");
    }
    restore();check(live_refs==0 && acquisitions==0,"retirement matrix releases all borrowed suites and streams");
}
void saved_output_scope_test(const char* path) {
    // Optional owner fixture stays in ignored artifacts; no project data ships.
    std::ifstream file(path,std::ios::binary);
    std::vector<char> bytes((std::istreambuf_iterator<char>(file)),{});
    auto decoded=core::deserialize_graph(std::span<const std::byte>(reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()),
        core::particle_node_registry());
    check(decoded.has_value(),"saved owner graph fixture decodes");if(!decoded.has_value())return;
    auto graph=decoded.take_value();
    struct Values {
        std::array<double,kNativeBindingCapacity> aliases{};
        std::array<bool,kNativeBindingCapacity> missing{},visited{};
        core::NodeId missing_node{};A_long first_missing{-1};
    } values;
    for(const auto& record:graph.optional_records) {
        if(record.size()<12 || record[0]!=std::byte{2} || record[1]!=std::byte{0x80})continue;
        std::size_t at=8;
        const auto read=[&](unsigned count){std::uint64_t value{};for(unsigned i=0;i<count;++i)
            value|=std::uint64_t(std::to_integer<unsigned char>(record.at(at++)))<<(8*i);return value;};
        const auto count=read(4);
        for(std::uint64_t n=0;n<count;++n) {
            core::NodeId id;for(auto& byte:id.value.bytes)byte=static_cast<std::uint8_t>(read(1));
            const auto kind=read(2),fields=read(2);
            const bool absent=n==count-1; // Last saved Turbulence source is absent in this owner fixture.
            if(absent){values.missing_node=id;check(kind==8,"saved absent source is Turbulence");}
            for(std::uint64_t f=0;f<fields;++f) {
                (void)read(2);const auto type=read(2),slot=read(2);(void)read(2);
                const unsigned components=type==0?1:type==1?2:3;
                for(unsigned c=0;c<3;++c) {
                    const auto number=std::bit_cast<double>(read(8));
                    if(slot!=0xffff && c<components) {
                        values.aliases.at(slot+c)=number;values.missing.at(slot+c)=absent;
                        if(absent && values.first_missing<0)values.first_missing=kNativeBindingFirstIndex+static_cast<A_long>(slot+c);
                    }
                }
            }
        }
    }
    auto data=renderer_data;data.effect_ref=reinterpret_cast<PF_ProgPtr>(&values);data.num_params=0;
    data.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
        auto& v=*reinterpret_cast<Values*>(ref);const auto slot=index-kNativeBindingFirstIndex;
        v.visited.at(slot)=true;*out={};out->param_type=PF_Param_FLOAT_SLIDER;
        out->u.fs_d.value=v.missing.at(slot)?kNativeBindingUnavailable:v.aliases.at(slot);return 0;
    };
    data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
    auto sampled=graph;A_long stream_index=-1;NativeBindingFailure failure;
    check(sample_native_node_animation(&data,sampled,1920,1080,&stream_index,nullptr,nullptr,&failure)==0,
        "actual saved orphan graph remains previewable without its disconnected source");
    check(std::count(values.visited.begin(),values.visited.end(),true)==69,
        "saved Output uses only Emitter/Particle's 69 inputs");
    core::RenderRequest request{};request.settings=core::validate_settings(core::Settings{});
    request.graph=std::make_shared<const core::Graph>(graph);
    request.frame={1920,1080,512,288,{0,0,512,288},{1,1},{1,24},core::PixelFormat::rgba8,
        core::ColorSpace::ae_working_space,core::AlphaMode::premultiplied,1,core::Quality::full};
    const auto rendered=core::CpuParticleRenderer{}.render(request,core::NeverCancelled{});
    check(rendered.has_value(),"actual saved Output graph renders through the CPU backend");
    if(rendered.has_value()) {
        const auto& pixels=rendered.value().pixels;bool visible=false;
        for(std::size_t i=3;i<pixels.size();i+=4)visible |= pixels[i]!=std::byte{0};
        check(visible,"saved Output graph produces visible particle alpha");
    }
    std::vector<NativeOriginBinding> origins;std::vector<NativeLifetimeBinding> lifetimes;
    check(read_native_origin_bindings(graph,origins)==0 && origins.size()==1 &&
        read_native_lifetime_bindings(graph,lifetimes)==0 && lifetimes.size()==1,"saved history uses the same Output scope");
    NativeAnimationPlan plan(graph,1920,1080,1);core::GraphNode unused;
    check(plan.valid() && plan.sample(&data,values.missing_node,unused)==PF_Err_BAD_CALLBACK_PARAM,
        "history plan excludes saved orphan bindings");
    auto parked=graph;parked.edges.clear();origins.clear();lifetimes.clear();
    check(read_native_origin_bindings(parked,origins)==0 && origins.empty() &&
        read_native_lifetime_bindings(parked,lifetimes)==0 && lifetimes.empty(),"parked birth/lifetime nodes do not enter history");
    auto no_callbacks=data;no_callbacks.inter.checkout_param=nullptr;no_callbacks.inter.checkin_param=nullptr;
    check(sample_native_node_animation(&no_callbacks,parked,1920,1080)==0,"empty Output scope needs no host callbacks");
    core::NodeId particle{},output{};
    for(const auto& node:graph.nodes) {
        if(node.type_key==core::graph_keys::kParticleNode)particle=node.id;
        if(node.type_key==core::graph_keys::kOutputNode)output=node.id;
    }
    auto required=graph;std::erase_if(required.edges,[&](const auto& edge){return edge.destination_node==output;});
    core::EdgeId edge1{},edge2{};edge1.value.bytes[0]=250;edge2.value.bytes[0]=251;
    required.edges.push_back({edge1,particle,core::graph_keys::kParticleParticlesOut,values.missing_node,core::graph_keys::kTurbulenceParticlesIn});
    required.edges.push_back({edge2,values.missing_node,core::graph_keys::kTurbulenceParticlesOut,output,core::graph_keys::kOutputParticles});
    check(sample_native_node_animation(&data,required,1920,1080,&stream_index,nullptr,nullptr,&failure)==PF_Err_BAD_CALLBACK_PARAM &&
        stream_index==values.first_missing && failure.node==values.missing_node && failure.parameter==5 && failure.component==0,
        "reachable missing source still fails with exact node/parameter/component context");
    auto explicit_node=graph;
    check(sample_native_node_animation(&data,explicit_node,1920,1080,&stream_index,nullptr,&values.missing_node)==PF_Err_BAD_CALLBACK_PARAM,
        "explicit requested-node sampling still reports missing bindings");
}
void saved_missing_model_test(const char* path) {
    std::ifstream input(path,std::ios::binary);
    const std::vector<char> raw((std::istreambuf_iterator<char>(input)),{});
    std::vector<std::byte> bytes;for(auto c:raw)bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    auto decoded=core::deserialize_graph(bytes,core::particle_node_registry());
    check(decoded.has_value(),"saved active-Model failure graph decodes");if(!decoded.has_value())return;
    auto graph=decoded.take_value();
    struct Samples {std::array<double,kNativeBindingCapacity> values{};std::array<bool,kNativeBindingCapacity> missing{};core::NodeId model{};} samples;
    for(const auto& record:graph.optional_records) {
        if(record.size()<12 || record[0]!=std::byte{2} || record[1]!=std::byte{0x80})continue;
        std::size_t at=8;
        const auto read=[&](unsigned count){std::uint64_t value{};for(unsigned i=0;i<count;++i)
            value|=std::uint64_t(std::to_integer<unsigned char>(record.at(at++)))<<(8*i);return value;};
        const auto count=read(4);
        for(std::uint64_t n=0;n<count;++n){core::NodeId id;for(auto& byte:id.value.bytes)byte=static_cast<std::uint8_t>(read(1));
            const auto kind=read(2),fields=read(2);if(kind==5)samples.model=id;
            for(std::uint64_t f=0;f<fields;++f){(void)read(2);const auto type=read(2),slot=read(2);(void)read(2);
                const unsigned components=type==0?1:type==1?2:3;
                for(unsigned c=0;c<3;++c){const auto value=std::bit_cast<double>(read(8));
                    if(slot!=0xffff && c<components){samples.values.at(slot+c)=value;samples.missing.at(slot+c)=kind==5;}}
            }
        }
        check(at==record.size(),"saved active-Model record is consumed exactly");
    }
    auto data=renderer_data;data.effect_ref=reinterpret_cast<PF_ProgPtr>(&samples);data.num_params=0;
    data.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
        const auto& values=*reinterpret_cast<Samples*>(ref);const auto slot=index-kNativeBindingFirstIndex;
        *out={};out->param_type=PF_Param_FLOAT_SLIDER;
        out->u.fs_d.value=values.missing.at(slot)?kNativeBindingUnavailable:values.values.at(slot);return 0;};
    data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
    A_long stream=-1;NativeBindingFailure failure;
    check(sample_native_node_animation(&data,graph,1920,1080,&stream,nullptr,nullptr,&failure)==PF_Err_BAD_CALLBACK_PARAM &&
        stream==237 && failure.node==samples.model && failure.parameter==5 && failure.component==0,
        "actual saved active Model reproduces stream237 instead of supplying a numeric fallback");
}
void saved_missing_auxiliary_test(const char* path) {
    std::ifstream input(path,std::ios::binary);const std::vector<char> raw((std::istreambuf_iterator<char>(input)),{});
    core::OpaqueBytes bytes;for(auto c:raw)bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c)));
    auto decoded=core::deserialize_graph(bytes,core::particle_node_registry());
    check(decoded.has_value(),"current saved Auxiliary startup graph decodes with CRC");if(!decoded.has_value())return;
    auto graph=decoded.take_value();core::NodeId absent{};
    constexpr std::array<std::uint8_t,16> identity{0x7f,0x86,0xfd,0x58,0xba,0x9f,0x49,0x4e,0x80,0x72,0x5f,0x51,0x8b,0xcf,0x16,0xec};absent.value.bytes=identity;
    struct Samples {std::array<double,kNativeBindingCapacity> values{};std::array<bool,kNativeBindingCapacity> missing{};unsigned missing_reads{};} samples;
    for(const auto& record:graph.optional_records) {
        if(record.size()<12 || record[0]!=std::byte{2} || record[1]!=std::byte{0x80})continue;
        std::size_t at=8;const auto read=[&](unsigned n){std::uint64_t v{};for(unsigned i=0;i<n;++i)v|=std::uint64_t(std::to_integer<unsigned char>(record.at(at++)))<<(8*i);return v;};
        const auto count=read(4);
        for(std::uint64_t n=0;n<count;++n){core::NodeId id;for(auto& b:id.value.bytes)b=static_cast<std::uint8_t>(read(1));
            (void)read(2);const auto fields=read(2);
            for(std::uint64_t f=0;f<fields;++f){(void)read(2);const auto type=read(2),slot=read(2);(void)read(2);
                const unsigned components=type==0?1:type==1?2:3;
                for(unsigned c=0;c<3;++c){const double value=std::bit_cast<double>(read(8));
                    if(slot!=0xffff && c<components){samples.values.at(slot+c)=value;samples.missing.at(slot+c)=id==absent;}}
            }
        }
        check(at==record.size(),"current saved Auxiliary binding record consumed exactly");
    }
    auto data=renderer_data;data.effect_ref=reinterpret_cast<PF_ProgPtr>(&samples);data.num_params=0;
    data.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
        auto& values=*reinterpret_cast<Samples*>(ref);const auto slot=index-kNativeBindingFirstIndex;
        *out={};out->param_type=PF_Param_FLOAT_SLIDER;if(values.missing.at(slot))++values.missing_reads;
        out->u.fs_d.value=values.missing.at(slot)?kNativeBindingUnavailable:values.values.at(slot);return 0;};
    data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
    A_long stream=-1;NativeBindingFailure failure;
    check(sample_native_node_animation(&data,graph,1920,1080,&stream,nullptr,nullptr,&failure)==PF_Err_BAD_CALLBACK_PARAM &&
        stream==208 && failure.node==absent && failure.parameter==1 && failure.component==0,
        "actual current saved graph reproduces exact startup stream208 rather than inventing values");
    const auto original_nodes=graph.nodes.size(),original_edges=graph.edges.size();
    std::erase_if(graph.nodes,[&](const auto& n){return n.id==absent;});
    std::erase_if(graph.edges,[&](const auto& e){return e.source_node==absent || e.destination_node==absent;});
    check(graph.nodes.size()+1==original_nodes && graph.edges.size()+2==original_edges && core::validate_graph(graph,core::particle_node_registry()).ok(),
        "actual inventory reconstruction removes only the missing Auxiliary and two dangling links");
    samples.missing_reads=0;
    check(sample_native_node_animation(&data,graph,1920,1080)==0 && !samples.missing_reads,
        "reconstructed current saved graph samples every surviving source and never reads absent stream208");
}
__declspec(noinline) void repair_publication_failure_test(PF_OutData& output) {
    auto& main=fixtures[0];
    const auto baseline_text_storage=std::make_unique<decltype(expressions)>(expressions);const auto& baseline_text=*baseline_text_storage;const auto baseline_enabled=expression_enabled;
        // Repair still uses the actual publisher's rollback before alias retirement.
        panel_output_only=true;renderer_flags=AEGP_EffectFlags_NONE;
        main.values[kGraphEditCommitId].one_d=987658;unresolved_alias=kNativeBindingFirstIndex;
        fail_set=kGraphEditReceiptId;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987658,701,&output,true)!=0,
            "disabled repair rejects partially failed saved publication");
        check(expressions==baseline_text && expression_enabled==baseline_enabled && saved_graph().nodes.size()==8,
            "disabled repair restores original graph and aliases after publication failure");
        panel_output_only=false;renderer_flags=AEGP_EffectFlags_ACTIVE;unresolved_alias=-1;
}
__declspec(noinline) void repair_bindings_test(const core::Graph& prior) {
    const auto before_invalid=sets;const char* stage{};
        {
            NativeBindingTransaction active_repair(&renderer_data,701);
            check(active_repair.install(saved_graph(),nullptr,&stage,nullptr,&prior,false,true)!=0 && sets==before_invalid,
                "repair cannot bypass old handoff while renderer is active");
        }
        renderer_flags=AEGP_EffectFlags_NONE;unresolved_alias=kNativeBindingFirstIndex;
        {
            NativeBindingTransaction disabled_repair(&renderer_data,701);
            check(disabled_repair.install(saved_graph(),nullptr,&stage,nullptr,&prior,false,true)==0,
                "explicit disabled renderer can rebuild after obsolete source disappeared");
        }
        {
            NativeBindingTransaction invalid_new(&renderer_data,701);
            check(invalid_new.install(prior,nullptr,&stage,nullptr,&prior,false,true)!=0,
                "disabled repair still rejects an unresolved newly active binding");
        }
        renderer_flags=AEGP_EffectFlags_ACTIVE;unresolved_alias=-1;
}
__declspec(noinline) void seven_kind_publication_tests() {
    auto& main=fixtures[0];
    {
        // Include real Material/Motion records as well as Emitter, Particle,
        // Force, Transform and Model in a single saved alias publication.
        using namespace core::graph_keys;
        model_present=transform_present=all_kinds_present=panel_publish=true;
        for(auto [index,kind]:{std::pair{7,records::Kind::material},std::pair{8,records::Kind::motion}}) {
            auto& f=fixtures[index];f.values.resize(records::parameter_count(kind));f.params.resize(f.values.size());
            for(auto& p:f.params)p.param_type=PF_Param_FLOAT_SLIDER;
            uuid(index,records::uuid_first_index(kind),index);
        }
        fixtures[7].name="org.starfieldfx.node.material";
        fixtures[7].values[1].one_d=1;fixtures[7].values[2].color={1,1,1,1};
        fixtures[7].values[3].one_d=50;fixtures[7].values[5].one_d=100;fixtures[7].values[6].one_d=1;
        fixtures[8].name="org.starfieldfx.node.motion";
        auto& m=fixtures[8].values;namespace ml=records::motion_layout;
        m[ml::mode].one_d=1;m[ml::origin_type].one_d=1;m[ml::origin_xy].two_d={960,540};
        m[ml::speed].one_d=100;m[ml::curve].one_d=2;
        m[ml::path_max_delay].one_d=.5;m[ml::path_end_behavior].one_d=1; // Current native popup defaults.
        m[ml::curve+2].one_d=100;m[ml::curve+3].one_d=1;m[ml::curve+4].one_d=100;
        connection(4,records::Kind::transform,8,14);connection(8,records::Kind::motion,255,18);
        main.values[kGraphSyncGuardId].one_d=1;main.values[kGraphEditCommitId].one_d=987657;
        PF_OutData output{};
        const auto seven_error=commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987657,701,&output);
        if(seven_error)std::printf("Seven-kind diagnostic: %d %s\n",seven_error,output.return_msg);
        check(seven_error==0,
            "seven-kind native graph publishes actual bindings");
        auto prior=saved_graph();check(prior.nodes.size()==8,"all seven author kinds and Output saved");
        final_refresh_test(prior,output);
        // Owner topology: Motion -> Force -> Output. Delete just Force,
        // preserving parked Motion and its upstream authors, then sample both
        // a previously checked-out graph and the newly published graph.
        connection(2,records::Kind::particle,8,12);
        fixtures[4].values[records::connection_count_index(records::Kind::transform)].one_d=0;
        connection(8,records::Kind::motion,4,18);
        connection(3,records::Kind::force,255,19);
        main.values[kGraphEditCommitId].one_d=987660;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987660,701,&output)==0,
            "Motion Force Output baseline publishes");
        const auto force_prior=saved_graph();
        // The former fixture inherited Force -> Transform with parked Transform,
        // so its alleged Motion -> Force -> Output removal was not Output-active.
        check(std::any_of(force_prior.edges.begin(),force_prior.edges.end(),[](const auto& e){return e.source_node.value.bytes.back()==4 && e.destination_node.value.bytes.back()==255;}),
            "Force deletion baseline really reaches Output");
        {
            auto clear=std::make_unique<NativeClearSnapshot>();
            clear->revision=static_cast<A_long>(main.values[kGraphRevisionId].one_d);
            clear->high=static_cast<A_long>(main.values[kGraphChecksumHighId].one_d);clear->low=static_cast<A_long>(main.values[kGraphChecksumLowId].one_d);
            clear->selected_count=1;clear->selected[0].value.bytes.back()=4;
            const auto saved_time=renderer_data.current_time;renderer_data.current_time=0;main.values[kGraphSyncGuardId].one_d=0;
            check(capture_panel_clear(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,*clear)==0 && clear->captured,
                "Clear captures complete old active finite handoffs while all authors are alive");
            check(std::count(clear->present.begin(),clear->present.end(),true)>10,"Clear capture includes surviving old aliases, not just Force");
            main.values[kGraphSyncGuardId].one_d=1;external_force_removed=true;absent_source_uuid=4;
            fixtures[8].values[records::connection_count_index(records::Kind::motion)].one_d=0;
            clear->removed_count=1;clear->removed[0]=clear->selected[0];main.values[kGraphEditCommitId].one_d=987670;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987670,701,&output)!=0,
                "ordinary post-physical-delete publication reproduces unavailable old handoff");
            const auto first_slot=static_cast<std::size_t>(std::find(clear->present.begin(),clear->present.end(),true)-clear->present.begin());
            for(unsigned invalid=0;invalid<7;++invalid){auto rejected=std::make_unique<NativeClearSnapshot>(*clear);
                if(invalid==0)++rejected->revision;
                if(invalid==1)rejected->owners[first_slot]={};
                if(invalid==2)rejected->values[first_slot]=kNativeBindingUnavailable;
                if(invalid==3)rejected->present[first_slot]=false;
                if(invalid==4)++rejected->time.value;
                if(invalid==5)rejected->removed[0]={};
                if(invalid==6)nonfinite_alias=kNativeBindingFirstIndex;
                check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987670,701,&output,false,rejected.get())!=0,
                    "Clear rejects stale stamp, wrong slot owner, unavailable value, missing slot, wrong time/UUID or invalid NEW live binding");
                nonfinite_alias=-1;
            }
            external_force_removed=false;fixtures[3].values[records::sync_guard_index(records::Kind::force)].one_d=transaction_removal_guard;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987670,701,&output,false,clear.get())!=0,
                "Clear proof rejects a physically present guard3 source");
            fixtures[3].values[records::sync_guard_index(records::Kind::force)].one_d=0;external_force_removed=true;
            stale_dependency_alias=kNativeBindingFirstIndex;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987670,701,&output,false,clear.get())==0 && stale_dependency_alias==-1,
                "same-operation capture permits certified Clear publication and rebuilds stale surviving dependencies");
            check(saved_graph().nodes.size()+1==force_prior.nodes.size(),"Clear publication removes exactly the observed Force");
            check(refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output)==0,
                "physical Clear final refresh validates all new live bindings");
            external_force_removed=false;absent_source_uuid=0;connection(8,records::Kind::motion,4,18);
            main.values[kGraphEditCommitId].one_d=987671;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987671,701,&output)==0,
                "source restoration rebuilds original topology without reusing the Clear proof");
            renderer_data.current_time=saved_time;
        }
        fixtures[8].values[records::connection_count_index(records::Kind::motion)].one_d=0;
        fixtures[3].values[records::sync_guard_index(records::Kind::force)].one_d=transaction_removal_guard;
        main.values[kGraphEditCommitId].one_d=987661;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987661,701,&output)==0,
            "Force deletion publishes before physical removal");
        absent_source_uuid=4;
        auto force_render=renderer_data;force_render.num_params=0;
        force_render.inter.checkout_param=[](PF_ProgPtr,A_long index,A_long time,A_long,A_u_long,PF_ParamDef* p)->PF_Err {
            p->param_type=PF_Param_FLOAT_SLIDER;p->u.fs_d.value=evaluated_binding(index,time);return 0;
        };
        force_render.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
        auto checked_out=force_prior,after_force=saved_graph();
        check(sample_native_node_animation(&force_render,checked_out,1920,1080)==0,
            "deleted Force leaves finite handoffs for in-flight old graph");
        check(sample_native_node_animation(&force_render,after_force,1920,1080)==0,
            "disconnected Motion graph needs no deleted Force binding");
        check(refresh_panel_graph_bindings(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,&output)==0,
            "surviving Motion bindings refresh after Force removal");
        absent_source_uuid=0;
        fixtures[3].values[records::sync_guard_index(records::Kind::force)].one_d=0;
        connection(2,records::Kind::particle,4,12);
        connection(4,records::Kind::transform,8,14);
        connection(8,records::Kind::motion,255,18);
        main.values[kGraphEditCommitId].one_d=987662;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987662,701,&output)==0,
            "Force regression restores independent baseline for remaining tests");
        prior=saved_graph();
        const auto lifecycle_bytes=core::serialize_graph(prior,core::particle_node_registry());
        check(lifecycle_bytes.has_value(),"seven-kind lifecycle graph serializes");
        std::ofstream lifecycle_file("artifacts/seven-kind-native-graph.bin",std::ios::binary);
        lifecycle_file.write(reinterpret_cast<const char*>(lifecycle_bytes.value().data()),lifecycle_bytes.value().size());
        check(bool(lifecycle_file),"actual compiler graph saved for full backup/render lifecycle test");lifecycle_file.close();
        const auto baseline_text_storage=std::make_unique<decltype(expressions)>(expressions);const auto& baseline_text=*baseline_text_storage;const auto baseline_enabled=expression_enabled;
        repair_publication_failure_test(output);
        auto render_data=renderer_data;render_data.num_params=0;
        render_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex i,A_long t,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
            *out={};out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=evaluated_binding(i,t);return 0;};
        render_data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
        unsigned samples{};
        alias_mutation_hook=[&]{++samples;auto old=prior;
            check(sample_native_node_animation(&render_data,old,1920,1080)==0,"seven-kind in-flight graph never sees a transient sentinel");
            auto live=saved_graph();check(sample_native_node_animation(&render_data,live,1920,1080)==0,
                "seven-kind saved graph remains valid at each host mutation");};
        // A receipt-write failure before retirement must leave every expression.
        panel_output_only=true;main.values[kGraphEditCommitId].one_d=987658;fail_set=kGraphEditReceiptId;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987658,701,&output)!=0,
            "seven-kind deletion publication failure rejects");
        check(expressions==baseline_text && expression_enabled==baseline_enabled && saved_graph().nodes.size()==8,
            "seven-kind failed publication keeps original aliases and graph");
        // A detach failure AFTER new graph verification restores graph+aliases.
        fail_expression=0;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987658,701,&output)!=0,
            "seven-kind post-publication retirement failure rejects");
        check(expressions==baseline_text && expression_enabled==baseline_enabled && saved_graph().nodes.size()==8,
            "seven-kind failed retirement restores graph and exact expressions");
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987658,701,&output)==0 && saved_graph().nodes.size()==1,
            "seven-kind deletion completes post-publication retirement");
        check(samples>0,"seven-kind reentrant mutation callbacks executed");alias_mutation_hook={};
        final_retirement_test(prior,render_data,output);
        // Invalid evaluated source values are still errors before any handoff.
        const auto before_invalid=sets;unresolved_alias=kNativeBindingFirstIndex;
        NativeBindingTransaction invalid(&renderer_data,701);const char* stage{};
        check(invalid.install(saved_graph(),nullptr,&stage,nullptr,&prior)==PF_Err_BAD_CALLBACK_PARAM && sets==before_invalid &&
            stage && !std::strcmp(stage,"verify finite alias handoff"),"missing old binding cannot become a numeric fallback");
        unresolved_alias=-1;
        repair_bindings_test(prior);
        panel_output_only=all_kinds_present=model_present=transform_present=panel_publish=false;main.values[kGraphSyncGuardId].one_d=0;
    }
}
int main(int argc,char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    SPBasicSuite basic{}; basic.AcquireSuite = acquire; basic.ReleaseSuite = release;
    PF_UtilCallbacks utils{}; utils.host_new_handle = allocate; utils.host_lock_handle = lock_handle;
    utils.host_unlock_handle = unlock_handle; utils.host_dispose_handle = dispose; utils.host_get_handle_size = size_handle;
    renderer_data.pica_basicP = &basic; renderer_data.effect_ref = reinterpret_cast<PF_ProgPtr>(1);
    renderer_data.utils = &utils; renderer_data.width = 1920; renderer_data.height = 1080;
    renderer_data.pixel_aspect_ratio = {1, 1}; renderer_data.time_scale = 24;
    renderer_data.num_params = kTotalEffectParameterCount + 1;
    if(argc>2 && !std::strcmp(argv[1],"--missing-auxiliary"))saved_missing_auxiliary_test(argv[2]);
    else {if(argc>1)saved_output_scope_test(argv[1]);if(argc>2)saved_missing_model_test(argv[2]);}
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
    particle.values[records::particle_layout::birth_chance].one_d=100;
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
    pf.AEGP_GetEffectLayer = [](PF_ProgPtr ref, AEGP_LayerH* layer)->A_Err {
        if(!ref){++null_pf_layer_queries;*layer=nullptr;return PF_Err_INTERNAL_STRUCT_DAMAGED;}
        *layer = reinterpret_cast<AEGP_LayerH>(1); return 0;
    };
    layers.AEGP_GetLayerParentComp=[](AEGP_LayerH layer,AEGP_CompH* comp)->A_Err {
        if(layer!=reinterpret_cast<AEGP_LayerH>(1))return PF_Err_BAD_CALLBACK_PARAM;
        *comp=reinterpret_cast<AEGP_CompH>(10);return 0;
    };
    layers.AEGP_GetLayerFromLayerID=[](AEGP_CompH comp,AEGP_LayerIDVal id,AEGP_LayerH* source)->A_Err {
        if(comp!=reinterpret_cast<AEGP_CompH>(10))return PF_Err_BAD_CALLBACK_PARAM;
        *source=id==1?reinterpret_cast<AEGP_LayerH>(1):id>=77 && id<=79?reinterpret_cast<AEGP_LayerH>(id):nullptr;
        return 0;
    };
    pf.AEGP_GetNewEffectForEffect = [](AEGP_PluginID, PF_ProgPtr source, AEGP_EffectRefH* ref)->A_Err {
        *ref = reinterpret_cast<AEGP_EffectRefH>(source); return 0;
    };
    effect.AEGP_GetLayerNumEffects = [](AEGP_LayerH, A_long* count)->A_Err { *count = (panel_output_only?1:all_kinds_present?9:model_present?7:transform_present?5:4)-(external_force_removed?1:0); return 0; };
    effect.AEGP_GetEffectFlags=[](AEGP_EffectRefH,AEGP_EffectFlags* flags)->A_Err{*flags=renderer_flags;return 0;};
    effect.AEGP_GetLayerEffectByIndex = [](AEGP_PluginID, AEGP_LayerH, A_long i, AEGP_EffectRefH* ref)->A_Err {if(source_probe_reorder && (i==1 || i==3))i=4-i;*ref = effect_ref(external_force_removed && i>=3?i+1:i); return 0; };
    effect.AEGP_GetInstalledKeyFromLayerEffect = [](AEGP_EffectRefH ref, AEGP_InstalledEffectKey* key)->A_Err { *key = static_cast<AEGP_InstalledEffectKey>(effect_index(ref)); return 0; };
    effect.AEGP_GetEffectMatchName = [](AEGP_InstalledEffectKey key, A_char* name)->A_Err { std::strcpy(name, fixtures[key].name); return 0; };
    effect.AEGP_DisposeEffect = [](AEGP_EffectRefH ref)->A_Err {
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
        if(motion_lifetime_checks && effect_index(ref)==8)++motion_stream_epoch;
#endif
        return 0;
    };
    dynamic.AEGP_GetNewStreamRefForLayer=[](AEGP_PluginID,AEGP_LayerH,AEGP_StreamRefH* out)->A_Err {
        ++live_refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{0,-3});return 0;};
    dynamic.AEGP_GetNewStreamRefByMatchname=[](AEGP_PluginID,AEGP_StreamRefH,const A_char* match,AEGP_StreamRefH* out)->A_Err {
        check(!std::strcmp(match,"ADBE Effect Parade"),"literal binding opens effect parade");
        ++live_refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{0,-2});return 0;};
    dynamic.AEGP_GetNewStreamRefByIndex=[](AEGP_PluginID,AEGP_StreamRefH,A_long index,AEGP_StreamRefH* out)->A_Err {
        if(source_probe_reorder && (index==1 || index==3))index=4-index;
        ++live_refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{static_cast<std::size_t>(external_force_removed && index>=3?index+1:index),-1});return 0;};
    stream.AEGP_GetStreamName=[](AEGP_PluginID,AEGP_StreamRefH ref,A_Boolean,AEGP_MemHandle* out)->A_Err {
        const auto& fixture=fixtures[reinterpret_cast<Ref*>(ref)->effect];std::u16string text=fixture.display_name;
        if(text.empty())for(const auto* name=fixture.name;*name;++name)text+=static_cast<char16_t>(*name);
        *out=reinterpret_cast<AEGP_MemHandle>(new std::u16string(text));return 0;};
    effect.AEGP_EffectCallGeneric = source_probe_generic;
    source_projects.AEGP_GetProjectByIndex=[](A_long index,AEGP_ProjectH* out)->A_Err {
        check(index==0,"cache scopes the current project");*out=reinterpret_cast<AEGP_ProjectH>(1);return 0;};
    source_projects.AEGP_GetProjectPath=[](AEGP_ProjectH,AEGP_MemHandle* out)->A_Err {
        *out=reinterpret_cast<AEGP_MemHandle>(new std::u16string(source_probe_empty_project?u"":source_probe_project));return 0;};
    stream.AEGP_GetUniqueStreamID=[](AEGP_StreamRefH ref,A_long* out)->A_Err {
        const auto& key=*reinterpret_cast<Ref*>(ref);*out=static_cast<A_long>(source_identity_epochs[key.effect]*100000+key.effect*10000+key.index+1);return 0;};
    stream.AEGP_GetNewEffectStreamByIndex = [](AEGP_PluginID, AEGP_EffectRefH ref, A_long index, AEGP_StreamRefH* out)->A_Err {
        ++opened_streams;
        if(effect_index(ref)==4 && index>=15 && index<=26)++synthetic_native_reads;
        ++live_refs;auto* owned=new Ref{effect_index(ref), index};
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
        owned->motion_epoch=motion_stream_epoch;
#endif
        *out=reinterpret_cast<AEGP_StreamRefH>(owned);return 0;
    };
    stream.AEGP_GetStreamType = [](AEGP_StreamRefH ref, AEGP_StreamType* type)->A_Err {
        if(stale_motion_ref(ref))return PF_Err_BAD_CALLBACK_PARAM;
        const auto& key = *reinterpret_cast<Ref*>(ref);
        if (key.index == wrong_type) { wrong_type = -1; *type = AEGP_StreamType_COLOR; }
        else if((!key.effect && key.index>=kTextureResourceFirstIndex && key.index<kTextureResourceFirstIndex+kTextureResourceCapacity) ||
                (key.effect==4 && key.index==1) || (key.effect==2 &&
                (key.index==records::particle_layout::texture_front || key.index==records::particle_layout::texture_back)))
            *type=AEGP_StreamType_LAYER_ID;
        else if(key.effect==5)*type=AEGP_StreamType_ThreeD_SPATIAL;
        else *type = key.index == kGraphParameterId ? AEGP_StreamType_ARB : AEGP_StreamType_OneD;
        return 0;
    };
    stream.AEGP_GetNewStreamValue = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_LTimeMode, const A_Time* time, A_Boolean pre_expression, AEGP_StreamValue2* out)->A_Err {
        if(stale_motion_ref(ref))return PF_Err_BAD_CALLBACK_PARAM;
        auto& key = *reinterpret_cast<Ref*>(ref);
        if(source_probe_delay_read && key.effect==2 && key.index==2 && !pre_expression)std::this_thread::sleep_for(std::chrono::milliseconds(2));
        if(source_probe_environment){++source_reads[key.effect];if(static_cast<int>(key.effect)==source_change_during_read && key.index==3){++source_epochs[key.effect];source_change_during_read=-1;}}
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
        if(stale_motion_ref(ref))return PF_Err_BAD_CALLBACK_PARAM;
        ++sets; auto& key = *reinterpret_cast<Ref*>(ref);
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
        if(key.effect==8) {
            check(key.index==records::motion_layout::mode || key.index==records::sync_guard_index(records::Kind::motion) ||
                (key.index>=143 && key.index<=186),"native Motion author writes only its mode, prefix bank and private guard");
            if(key.index==fail_set){fail_set=-1;return PF_Err_BAD_CALLBACK_PARAM;}
            if(key.index==records::motion_layout::mode && value->val.one_d==3 && fail_motion_restore)return PF_Err_BAD_CALLBACK_PARAM;
            if(key.index==records::motion_layout::mode && silent_motion_write){silent_motion_write=false;return 0;}
            if(key.index==records::sync_guard_index(records::Kind::motion) && value->val.one_d==0 && silent_motion_guard_release){silent_motion_guard_release=false;return 0;}
            fixtures[8].values[key.index].one_d=value->val.one_d;return 0;
        }
#endif
        check(key.effect == 0, "native compile writes renderer only");
        check(key.index != kGraphEditCommitId && (panel_publish || key.index != kGraphEditReceiptId), "native edit preserves CEP nonce and receipt");
        if (key.index == fail_set) { fail_set = -1; return PF_Err_OUT_OF_MEMORY; }
        if (key.index == kGraphParameterId) {
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
            if(motion_lifetime_checks)++motion_stream_epoch;
#endif
            if (ignore_graph) { ignore_graph = false; return 0; }
            auto copy = clone(reinterpret_cast<PF_Handle>(value->val.arbH));
            dispose(reinterpret_cast<PF_Handle>(fixtures[0].values[key.index].arbH));
            fixtures[0].values[key.index].arbH = reinterpret_cast<AEGP_ArbBlockVal>(copy);
        } else if(key.index>=kTextureResourceFirstIndex && key.index<kTextureResourceFirstIndex+kTextureResourceCapacity) {
            fixtures[0].values[key.index].layer_id=value->val.layer_id;
        } else {
            // Model host float storage; integer receipts must remain exact.
            fixtures[0].values[key.index].one_d = static_cast<PF_FpShort>(value->val.one_d);
            if (corrupt_receipt && key.index == kGraphChecksumHighId) {
                corrupt_receipt = false; fixtures[0].values[key.index].one_d += 1;
            }
        }
        if(key.index==kGraphParameterId || key.index>=kNativeBindingFirstIndex)sample_after_alias_mutation();
        return 0;
    };
    stream.AEGP_DisposeStreamValue = [](AEGP_StreamValue2* value)->A_Err {
        auto& key = *reinterpret_cast<Ref*>(value->streamH);
        if (!key.effect && key.index == kGraphParameterId) dispose(reinterpret_cast<PF_Handle>(value->val.arbH)); return 0;
    };
    stream.AEGP_DisposeStream = [](AEGP_StreamRefH ref)->A_Err { --live_refs; delete reinterpret_cast<Ref*>(ref); return 0; };
    stream.AEGP_GetExpressionState = [](AEGP_PluginID, AEGP_StreamRefH ref, A_Boolean* enabled)->A_Err {
        auto* key=reinterpret_cast<Ref*>(ref);
        if(!key->effect && key->index-kNativeBindingFirstIndex==retired_readback_slot && retired_readback_armed && retired_readback_mode==4) {
            retired_readback_armed=false;retired_readback_slot=-1;return PF_Err_OUT_OF_MEMORY;
        }
        *enabled = key->effect ? FALSE : expression_enabled[key->index - kNativeBindingFirstIndex]; return 0;
    };
    stream.AEGP_GetExpression = [](AEGP_PluginID, AEGP_StreamRefH ref, AEGP_MemHandle* result)->A_Err {
        const auto slot=reinterpret_cast<Ref*>(ref)->index-kNativeBindingFirstIndex;
        const bool injected=slot==retired_readback_slot && retired_readback_armed && retired_readback_mode!=4;
        if(injected){retired_readback_armed=false;retired_readback_slot=-1;
            if(retired_readback_mode==1){*result=nullptr;return PF_Err_OUT_OF_MEMORY;}
            if(retired_readback_mode==2){*result=reinterpret_cast<AEGP_MemHandle>(new std::u16string(u"x"));return 0;}
        }
        *result = reinterpret_cast<AEGP_MemHandle>(new std::u16string(expressions[slot]));
        if(injected && retired_readback_mode==3)null_retired_lock=*result;
        return 0;
    };
    memory.AEGP_LockMemHandle = [](AEGP_MemHandle ref, void** result)->A_Err {
        if(ref==null_retired_lock && ref){null_retired_lock=nullptr;*result=nullptr;return 0;}
        *result = const_cast<char16_t*>(reinterpret_cast<std::u16string*>(ref)->c_str()); return 0;
    };
    memory.AEGP_GetMemHandleSize=[](AEGP_MemHandle ref,AEGP_MemSize* size)->A_Err {
        *size=(reinterpret_cast<std::u16string*>(ref)->size()+1)*sizeof(char16_t);return 0;};
    memory.AEGP_UnlockMemHandle = [](AEGP_MemHandle)->A_Err {return 0;};
    memory.AEGP_FreeMemHandle = [](AEGP_MemHandle ref)->A_Err {delete reinterpret_cast<std::u16string*>(ref);return 0;};
    stream.AEGP_SetExpression = [](AEGP_PluginID, AEGP_StreamRefH ref, const A_UTF16Char* text)->A_Err {
        auto index = reinterpret_cast<Ref*>(ref)->index - kNativeBindingFirstIndex;
        if (index == fail_expression) {fail_expression = -1; return PF_Err_BAD_CALLBACK_PARAM;}
        const std::u16string_view incoming(reinterpret_cast<const char16_t*>(text));
        if(native_retired_alias::owns(incoming) && index==ignore_retired_text){ignore_retired_text=-1;return 0;}
        if(!*text)++expression_clear_count;
        const bool text_changed=expressions[index]!=reinterpret_cast<const char16_t*>(text);
        expressions[index] = reinterpret_cast<const char16_t*>(text);
        if(native_retired_alias::owns(incoming)) {
            if(index==retired_readback_slot)retired_readback_armed=true;
            if(index==partial_retired_text){partial_retired_text=-1;return PF_Err_OUT_OF_MEMORY;}
            if(index==enable_retired_text){enable_retired_text=-1;expression_enabled[index]=TRUE;}
            if(index==corrupt_retired_base){corrupt_retired_base=-1;fixtures[0].values[kNativeBindingFirstIndex+index].one_d+=1;}
        }
        if(*text && index+kNativeBindingFirstIndex==stale_dependency_alias)stale_dependency_alias=-1;
        if(text_changed && index+kNativeBindingFirstIndex==stale_retirement_alias)stale_retirement_alias=-1;
        sample_after_alias_mutation(); return 0;
    };
    stream.AEGP_SetExpressionState = [](AEGP_PluginID, AEGP_StreamRefH ref, A_Boolean enabled)->A_Err {
        if(enabled && disable_next_expression) { enabled=FALSE;disable_next_expression=false; }
        const auto index=reinterpret_cast<Ref*>(ref)->index-kNativeBindingFirstIndex;
        if(!enabled && index==ignore_retired_disable && native_retired_alias::owns(expressions[index])){ignore_retired_disable=-1;return 0;}
        expression_enabled[index] = enabled;
        sample_after_alias_mutation();return 0;
    };
    check(register_node_graph_sync(&renderer_data) == 0, "node adapter registers its own AEGP ID");
    const auto source_scope_module=LoadLibraryW(L"artifacts\\model-ui-exclusion-tests\\paired\\StarfieldHost.aex");
    check(source_scope_module!=nullptr,"source callback fixture loads the paired resident exclusion");
    const auto source_scope_initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(source_scope_module,"SFLD_TestInitialize"));
    check(source_scope_initialize!=nullptr,"source callback fixture initializes the UI thread explicitly");source_scope_initialize();
    {
        // Compile actual native records, then compare every encoded particle with
        // the render-only startup projection. This catches parameter AND UUID drift.
        const auto saved_emitter=emitter, saved_particle=particle;
        const auto saved_cap=main.params[kMaxParticlesId].u.fs_d.value;
        external_force_removed=true;
        emitter.values[3].one_d=100;emitter.values[27].one_d=1000;
        emitter.values[28].one_d=10;emitter.values[29].one_d=100;
        emitter.values[11].one_d=.05;emitter.values[15].one_d=2;
        const auto write_uuid=[](Fixture& fixture,A_long first,core::Uuid128 id){
            for(unsigned i=0;i<8;++i)fixture.values[first+i].one_d=(unsigned(id.bytes[2*i])<<8)|id.bytes[2*i+1];
        };
        std::vector<PF_ParamDef*> pointers(main.params.size());
        for(std::size_t i=0;i<pointers.size();++i)pointers[i]=&main.params[i];
        for(const A_long height:{720,1080,2160})for(const std::uint32_t cap:{0u,7u,1000000u}){
            renderer_data.height=height;emitter.values[4].two_d={960,double(height)/2};
            main.params[kMaxParticlesId].u.fs_d.value=cap;
            auto initial=initial_node_graph(height,cap);check(initial.has_value(),"startup projection builds");
            if(!initial.has_value())continue;
            const auto& a=initial.value();
            write_uuid(emitter,records::uuid_first_index(records::Kind::emitter),a.nodes[0].id.value);
            write_uuid(particle,records::uuid_first_index(records::Kind::particle),a.nodes[1].id.value);
            write_uuid(emitter,records::connection_uuid_index(records::Kind::emitter,0,0),a.nodes[1].id.value);
            write_uuid(particle,records::connection_uuid_index(records::Kind::particle,0,0),a.nodes[2].id.value);
            write_uuid(emitter,records::connection_edge_uuid_index(records::Kind::emitter,0,0),a.edges[0].id.value);
            write_uuid(particle,records::connection_edge_uuid_index(records::Kind::particle,0,0),a.edges[1].id.value);
            core::Graph published;bool found=false;
            const auto result=compile_native_node_graph(&renderer_data,pointers.data(),published,found,701);
            check(result==0 && found && published.nodes.size()==3,"actual compiler produces the initial three-node graph");
            if(result)continue;
            for(const auto frame:{0,1,12,24,48,144}){
                const core::RationalTime time{frame,24};
                const auto before=core::evaluate_particle_graph(a,time,core::NeverCancelled{},{double(height),1});
                const auto after=core::evaluate_particle_graph(published,time,core::NeverCancelled{},{double(height),1});
                check(before.has_value() && after.has_value(),"both startup and native graphs evaluate");
                if(before.has_value() && after.has_value()){
                    const auto lhs=core::encode_evaluated_particles(before.value(),time);
                    const auto rhs=core::encode_evaluated_particles(after.value(),time);
                    check(lhs.has_value() && rhs.has_value() && lhs.value()==rhs.value(),
                        "first render and actual native compilation preserve every particle identity/position/appearance byte");
                }
            }
        }
        emitter=saved_emitter;particle=saved_particle;renderer_data.height=1080;
        main.params[kMaxParticlesId].u.fs_d.value=saved_cap;external_force_removed=false;
        check(!initial_node_graph(0,100).has_value(),"invalid startup geometry is rejected");
        check(!initial_node_graph(1080,core::kMaxParticleCount+1).has_value(),"invalid startup cap is rejected");
    }
#if defined(STARFIELD_MODEL_CALLBACK_TEST)
    {
        namespace ml=records::model_layout;
        auto& model=fixtures[6];model.name="org.starfieldfx.node.model";
        model.values.resize(records::parameter_count(records::Kind::model));model.params.resize(model.values.size());
        uuid(6,records::uuid_first_index(records::Kind::model),6);model.values[ml::source].one_d=1;
        for(A_long i=ml::scale;i<ml::scale+3;++i)model.values[i].one_d=100;
        connection(2,records::Kind::particle,255,12);connection(6,records::Kind::model,2,15);
        effect.AEGP_GetLayerNumEffects=[](AEGP_LayerH,A_long* count)->A_Err{*count=5;return 0;};
        effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long i,AEGP_EffectRefH* out)->A_Err{*out=effect_ref(i==4?6:i);return 0;};
        dynamic.AEGP_GetNewStreamRefByIndex=[](AEGP_PluginID,AEGP_StreamRefH,A_long i,AEGP_StreamRefH* out)->A_Err {
            ++live_refs;*out=reinterpret_cast<AEGP_StreamRefH>(new Ref{static_cast<std::size_t>(i==4?6:i),-1});return 0;
        };
        std::vector<PF_ParamDef*> delivered(model.params.size());
        for(std::size_t i=0;i<delivered.size();++i){model.params[i].param_type=PF_Param_FLOAT_SLIDER;
            model.params[i].u.fs_d.value=model.values[i].one_d;delivered[i]=&model.params[i];}
        model.params[ml::source].param_type=PF_Param_POPUP;model.params[ml::source].u.pd.value=1;
        PF_InData source=renderer_data;source.effect_ref=reinterpret_cast<PF_ProgPtr>(effect_ref(3));source.num_params=static_cast<A_long>(delivered.size());
        PF_UserChangedParamExtra changed{};changed.param_index=ml::source;PF_OutData output{};
        {
            EffectUiExclusion owner;check(bool(owner),"actual OBJ modal scope acquires resident Host exclusion");
            const auto before=suite_requests;
            check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0 && suite_requests==before,
                "automatic callback still defers under the OBJ modal scope");
            check(sync_model_import_graph_parameter(&source,&output,delivered.data(),&changed,owner)==0 &&
                main.values[kGraphRevisionId].one_d>0,"explicit OBJ owner publishes through real compiler and publisher under its own scope");
            check(std::get<std::uint32_t>(parameter(saved_graph(),core::graph_keys::kModelNode,core::graph_keys::kModelSource))==0,
                "owner publication resolves Model UUID despite stale PF binding pointing at Force");
            const auto revision=main.values[kGraphRevisionId].one_d;
            auto partial=source;partial.num_params=0;
            check(sync_model_import_graph_parameter(&partial,&output,reinterpret_cast<PF_ParamDef**>(1),&changed,owner)!=0,
                "explicit import partial delivery fails before reading the array");
            model.params[records::uuid_first_index(records::Kind::model)+7].u.fs_d.value=99;
            check(sync_model_import_graph_parameter(&source,&output,delivered.data(),&changed,owner)!=0 && main.values[kGraphRevisionId].one_d==revision,
                "missing import UUID fails instead of reporting empty success");
            model.params[records::uuid_first_index(records::Kind::model)+7].u.fs_d.value=6;
            EffectUiExclusion nested;check(!nested && sync_model_import_graph_parameter(&source,&output,delivered.data(),&changed,nested)!=0,
                "an unowned nested scope cannot authorize import publication");
            auto foreign=changed;foreign.param_index=ml::origin;
            check(sync_model_import_graph_parameter(&source,&output,delivered.data(),&foreign,owner)!=0,
                "the import capability cannot bypass other source callbacks");
        }
        check(live_refs==0 && acquisitions==0 && !ui_transaction_active(),"actual Model owner path releases every SDK reference and exclusion");
        std::printf("Model owned callback: %u checks, %u failures (actual NodeGraphSync/compiler/publisher; no AE qualification)\n",checks,failures);
        return failures?1:0;
    }
#endif
#if defined(STARFIELD_MOTION_CALLBACK_TEST)
    {
        namespace ml=records::motion_layout;
        using namespace core::graph_keys;
        const auto module=LoadLibraryW(L"artifacts\\model-ui-exclusion-tests\\paired\\StarfieldHost.aex");
        check(module!=nullptr,"Motion callback fixture loads its private Host exclusion fixture");
        const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));
        check(initialize!=nullptr,"Motion callback fixture has explicit UI-thread initialization");initialize();
        // Exercise the actual Motion-compiled NodeGraphSync, not a fabricated
        // NativeEdit or the registration fixture's synchronization stub.
        auto& motion=fixtures[8];motion.name="org.starfieldfx.node.motion";
        motion.values.resize(records::parameter_count(records::Kind::motion));
        motion.params.resize(motion.values.size());
        uuid(8,records::uuid_first_index(records::Kind::motion),8);
        motion.values[ml::mode].one_d=3;motion.values[ml::origin_type].one_d=1;
        motion.values[ml::origin_xy].two_d={960,540};motion.values[ml::speed].one_d=100;
        motion.values[ml::path_max_delay].one_d=.5;motion.values[ml::path_end_behavior].one_d=1;
        connection(2,records::Kind::particle,8,12);connection(8,records::Kind::motion,255,18);
        effect.AEGP_GetLayerEffectByIndex=[](AEGP_PluginID,AEGP_LayerH,A_long index,AEGP_EffectRefH* ref)->A_Err {
            *ref=effect_ref(index==3?8:index);return 0;
        };
        std::vector<PF_ParamDef*> delivered(motion.params.size());
        for(std::size_t i=0;i<delivered.size();++i) {
            motion.params[i].param_type=PF_Param_FLOAT_SLIDER;
            motion.params[i].u.fs_d.value=motion.values[i].one_d;
            delivered[i]=&motion.params[i];
        }
        motion.params[ml::mode].param_type=motion.params[ml::origin_type].param_type=
            motion.params[ml::path_end_behavior].param_type=PF_Param_POPUP;
        motion.params[ml::origin_xy].param_type=PF_Param_POINT;
        motion.params[ml::path_orient].param_type=PF_Param_CHECKBOX;
        PF_InData source=renderer_data;source.effect_ref=reinterpret_cast<PF_ProgPtr>(effect_ref(8));
        source.num_params=static_cast<A_long>(delivered.size());
        PF_UserChangedParamExtra changed{};changed.param_index=ml::mode;
        PF_OutData output{};
        for(const auto count:{0,records::parameter_count(records::Kind::force),static_cast<A_long>(delivered.size()-1),static_cast<A_long>(delivered.size()+1)}) {
            auto invalid=source;invalid.num_params=count;const auto before=suite_requests;
            check(sync_node_graph_parameter(&invalid,&output,reinterpret_cast<PF_ParamDef**>(1),&changed)==0 && suite_requests==before,
                "zero, Force, truncated and oversized Motion deliveries defer before touching array or SDK");
        }
        auto foreign_source=source;foreign_source.effect_ref=reinterpret_cast<PF_ProgPtr>(effect_ref(3));
        const auto before_foreign_sets=sets;
        const auto get_borrowed=pf.AEGP_GetNewEffectForEffect;
        pf.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr,AEGP_EffectRefH*)->A_Err {
            check(false,"Motion supervision must never open its stale PF effect parameter binding");return PF_Err_BAD_CALLBACK_PARAM;
        };
        motion.params[ml::mode].u.pd.value=1;
        check(sync_node_graph_parameter(&foreign_source,&output,delivered.data(),&changed)==0 && sets>before_foreign_sets,
            "Motion edit resolves a fresh kind+UUID even when its PF effect binding points at a former Force slot");
        pf.AEGP_GetNewEffectForEffect=get_borrowed;
        const auto identity_last=records::uuid_first_index(records::Kind::motion)+7;
        motion.params[identity_last].u.fs_d.value=7;
        const auto before_stale=sets;
        check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0 && sets==before_stale,
            "same-kind stale Motion UUID cannot publish or write a different node");
        motion.params[identity_last].u.fs_d.value=8;
        unsigned nested_callbacks{};
        alias_mutation_hook=[&] {
            const auto before_sets=sets,before_suites=suite_requests;
            check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0 && sets==before_sets && suite_requests==before_suites,
                "nested Motion supervision defers before touching SDK or publishing stale values");
            PF_UserChangedParamExtra stale_main{};stale_main.param_index=kMaxParticlesId;
            check(user_changed_param(&renderer_data,&output,nullptr,&stale_main)==0 && suite_requests==before_suites,
                "nested main supervision defers before reading a stale parameter delivery");
            check(update_motion_ui(&renderer_data,nullptr)==0 && suite_requests==before_suites,
                "nested main Motion UI refresh defers before SDK access");
            ++nested_callbacks;
        };
        const auto saved_mode=[] {
            for(const auto& node:saved_graph().nodes)if(node.type_key==kMotionNode)
                for(const auto& p:node.parameters)if(p.key==kMotionMode)return std::get<std::uint32_t>(p.value);
            throw std::runtime_error("saved Motion is missing");
        };
        for(int cycle=0;cycle<8;++cycle)for(const auto popup:{1,2,3,1}) {
            const auto before=motion.values[ml::mode].one_d;
            motion.params[ml::mode].u.pd.value=popup;
            check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0,
                "actual Motion callback accepts each delivered mode with old saved streams");
            std::uint32_t wanted{};check(ml::core_mode_from_popup(popup,wanted),"valid popup maps to Core");
            check(saved_mode()==wanted,"actual Motion callback publishes delivered mode rather than previous saved mode");
            (void)before;
            check(motion.values[ml::mode].one_d==popup,"native Motion author persists the delivered selector before publication");
            check(motion.params[ml::mode].uu.change_flags&PF_ChangeFlag_CHANGED_VALUE,
                "delivered Motion selector is explicitly retained when AE completes the callback");
            // Simulated AE saves the user's own edit after USER_CHANGED_PARAM.
            motion.values[ml::mode].one_d=popup;
        }
        {
            motion.params[ml::mode].u.pd.value=2;
            check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0,"prepare Light Path prefix edit");
            motion_prefix::Bank previous{},wanted{};
            check(motion_prefix::encode(L"Light_",wanted),"encode native prefix fixture");
            for(unsigned i=0;i<wanted.size();++i)motion.params[143+i].u.fs_d.value=wanted[i];
            {
                EffectUiExclusion owner;check(bool(owner),"prefix owns existing modal scope");
                check(sync_motion_prefix_graph_parameter(&source,&output,delivered.data(),previous,owner)==0,
                    "actual Motion owned prefix callback publishes through compiler and transaction");
            }
            const auto saved_prefix=[] {return std::get<std::string>(parameter(saved_graph(),kMotionNode,kMotionPathStartingWith));};
            check(saved_prefix()=="Light_" && saved_mode()==0,"saved graph contains authored prefix and Light Path mode");
            for(unsigned i=0;i<wanted.size();++i)check(motion.values[143+i].one_d==wanted[i],"saved source bank matches published prefix");
            previous=wanted;check(motion_prefix::encode(L"Other_",wanted),"encode replacement prefix");
            for(unsigned i=0;i<wanted.size();++i)motion.params[143+i].u.fs_d.value=wanted[i];
            const auto revision=main.values[kGraphRevisionId].one_d;
            {
                EffectUiExclusion owner;fail_set=kGraphParameterId;
                check(sync_motion_prefix_graph_parameter(&source,&output,delivered.data(),previous,owner)!=0,
                    "prefix graph publication error propagates");
            }
            check(saved_prefix()=="Light_" && main.values[kGraphRevisionId].one_d==revision,"failed prefix publication restores graph and revision");
            for(unsigned i=0;i<previous.size();++i)check(motion.values[143+i].one_d==previous[i],"failed prefix publication restores source bank");
            check(motion.values[records::sync_guard_index(records::Kind::motion)].one_d==0,"failed prefix publication restores source guard");
            for(unsigned i=0;i<previous.size();++i)motion.params[143+i].u.fs_d.value=0;
            {EffectUiExclusion owner;check(sync_motion_prefix_graph_parameter(&source,&output,delivered.data(),previous,owner)==0,"clear prefix via owned callback");}
            check(saved_prefix().empty(),"empty prefix preserves match-all semantics");
            motion.params[ml::mode].u.pd.value=1;
            check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0,"restore Circle fixture");
        }
        main.values[kGraphSyncGuardId].one_d=1;
        alias_mutation_hook={};check(nested_callbacks>0,"actual Motion publication exercises nested callback exclusion");
        motion.params[ml::mode].u.pd.value=1;
        const auto before_guarded=sets;
        check(sync_node_graph_parameter(&source,&output,delivered.data(),&changed)==0 && sets==before_guarded,
            "saved renderer guard suppresses a delayed Motion callback with delivered guard0");
        main.values[kNodeEffectsReadyId].one_d=1;
        panel_publish=true;
        for(const auto popup:{3,1,2,1}) {
            motion.values[ml::mode].one_d=popup;
            const auto nonce=400+popup+static_cast<A_long>(sets%100);
            main.values[kGraphEditCommitId].one_d=nonce;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),nonce,701,&output)==0,
                "actual panel publisher accepts each saved Motion mode");
            std::uint32_t wanted{};ml::core_mode_from_popup(popup,wanted);
            check(saved_mode()==wanted && motion.values[ml::mode].one_d==popup,
                "actual panel publisher preserves source mode and publishes the same Core mode");
        }
        // The observed owner failure has two authorities: the script can read
        // authored Circle while a native saved getter still reports Look At.
        // Float guard5 carries the requested Circle independently of that getter.
        motion.values[ml::mode].one_d=3;
        for(const auto silent:{false,true}) {
            motion.values[ml::mode].one_d=3;
            motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
            main.values[kGraphEditCommitId].one_d=704;
            const auto revision=main.values[kGraphRevisionId].one_d;
            if(silent)silent_motion_guard_release=true;
            else fail_set=records::sync_guard_index(records::Kind::motion);
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),704,701,&output)!=0 &&
                main.values[kGraphRevisionId].one_d==revision && saved_mode()==1 &&
                motion.values[ml::mode].one_d==3 && motion.values[records::sync_guard_index(records::Kind::motion)].one_d==5,
                "failed or silently discarded source guard release rejects and restores graph/revision/source intent");
        }
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
        main.values[kGraphEditCommitId].one_d=700;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),700,701,&output)==0 &&
            saved_mode()==1 && motion.values[ml::mode].one_d==1 &&
            motion.values[records::sync_guard_index(records::Kind::motion)].one_d==0,
            "explicit native Circle intent replaces a disagreeing native saved selector and clears after verification");
        for(const auto silent:{false,true}) {
            motion.values[ml::mode].one_d=3;
            motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
            main.values[kGraphEditCommitId].one_d=702;
            const auto revision=main.values[kGraphRevisionId].one_d;
            if(silent)silent_motion_write=true;else fail_set=ml::mode;
            check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),702,701,&output)!=0 &&
                main.values[kGraphRevisionId].one_d==revision && saved_mode()==1 &&
                motion.values[ml::mode].one_d==3 && motion.values[records::sync_guard_index(records::Kind::motion)].one_d==5,
                "failed or silently discarded native mode write preserves graph/revision and restores source intent");
        }
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
        main.values[kGraphEditCommitId].one_d=703;
        const auto restore_revision=main.values[kGraphRevisionId].one_d;fail_set=kGraphParameterId;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),703,701,&output)!=0 &&
            main.values[kGraphRevisionId].one_d==restore_revision && saved_mode()==1 && motion.values[ml::mode].one_d==3,
            "later native graph publication failure restores the already-written source selector");
        // Existing physical source is retired before the Output-only publication.
        motion_lifetime_checks=true;
        motion.values[ml::mode].one_d=3;
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
        main.values[kGraphEditCommitId].one_d=705;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),705,701,&output)==0 &&
            motion.values[ml::mode].one_d==1 && motion.values[records::sync_guard_index(records::Kind::motion)].one_d==0,
            "Motion publication reacquires source streams after owner release and graph invalidation");
        check(stale_motion_accesses==0,"Motion transaction never accesses an expired stream");
        motion.values[ml::mode].one_d=3;
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=5;
        main.values[kGraphEditCommitId].one_d=706;
        fail_motion_restore=true;fail_set=kGraphParameterId;
        const auto failed_restore=commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),706,701,&output);
        check(failed_restore==PF_Err_INTERNAL_STRUCT_DAMAGED &&
            motion.values[records::sync_guard_index(records::Kind::motion)].one_d==1 &&
            std::strstr(output.return_msg,"rollback"),
            "source restore failure propagates to panel command and remains guarded");
        fail_motion_restore=false;
        motion.values[ml::mode].one_d=1;
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=0;
        check(stale_motion_accesses==0,"publication and rollback never touch expired Motion streams");
        motion_lifetime_checks=false;
        motion.values[records::sync_guard_index(records::Kind::motion)].one_d=transaction_removal_guard;
        fixtures[1].values[records::sync_guard_index(records::Kind::emitter)].one_d=transaction_removal_guard;
        fixtures[2].values[records::sync_guard_index(records::Kind::particle)].one_d=transaction_removal_guard;
        main.values[kGraphEditCommitId].one_d=701;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),701,701,&output)==0 &&
            saved_graph().nodes.size()==1,"Motion retirement removes its record and aliases before physical deletion");
        check(std::none_of(expression_enabled.begin(),expression_enabled.end(),[](auto enabled){return enabled!=0;}),
            "Motion deletion leaves no enabled source dependency");
        dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));
        for(auto& p:main.params)if(p.param_type==PF_Param_ARBITRARY_DATA) {
            if(handles.contains(p.u.arb_d.value))dispose(p.u.arb_d.value);
            if(handles.contains(p.u.arb_d.dephault))dispose(p.u.arb_d.dephault);
        }
        check(FreeLibrary(module)!=0,"Motion callback fixture unloads its private Host module");
        check(FreeLibrary(source_scope_module)!=0,"source callback fixture unloads its paired resident exclusion");
        check(live_refs==0 && acquisitions==0 && handles.empty(),"actual Motion callback and publisher release all owned resources");
        std::printf("Motion native callbacks: %d checks, %d failures. Fake SDK; no AE qualification.\n",checks,failures);
        return failures?1:0;
    }
#endif
    {
        // Use the real Emitter visibility implementation and AEGP callbacks.
        // Hidden controls must remain writable during guarded preset/rollback batches.
        dynamic_enabled=true;
        visibility_flags.fill(0);
        dynamic.AEGP_GetDynamicStreamFlags=[](AEGP_StreamRefH ref,AEGP_DynStreamFlags* flags)->A_Err {
            const auto& key=*reinterpret_cast<Ref*>(ref);
            check(key.effect==1 && (key.index==1 || (key.index>=8 && key.index<=14) || (key.index>=20 && key.index<=26)),"Emitter visibility touches only its own source controls");
            *flags=visibility_flags[key.index];return 0;
        };
        dynamic.AEGP_SetDynamicStreamFlag=[](AEGP_StreamRefH ref,AEGP_DynStreamFlags flag,A_Boolean undoable,A_Boolean hidden)->A_Err {
            check(flag==AEGP_DynStreamFlag_HIDDEN && !undoable,"Emitter visibility changes only non-undoable HIDDEN flags");
            auto& flags=visibility_flags[reinterpret_cast<Ref*>(ref)->index];
            flags=hidden?flags|flag:flags&~flag;++visibility_sets;return 0;
        };
        const auto original_pf=pf.AEGP_GetNewEffectForEffect;
        pf.AEGP_GetNewEffectForEffect=[](AEGP_PluginID,PF_ProgPtr,AEGP_EffectRefH* ref)->A_Err{*ref=effect_ref(1);return 0;};
        std::array<PF_ParamDef,records::parameter_count(records::Kind::emitter)> ui_values{};
        std::array<PF_ParamDef*,ui_values.size()> ui_params{};
        for(unsigned i=0;i<ui_values.size();++i) {ui_params[i]=&ui_values[i];ui_values[i].param_type=PF_Param_FLOAT_SLIDER;}
        constexpr auto guard=records::sync_guard_index(records::Kind::emitter);
        auto emitter_ui_data=renderer_data;emitter_ui_data.num_params=static_cast<A_long>(ui_params.size());
        const auto all_hidden=[](bool hidden) {
            for(A_long i=20;i<=26;++i)if(bool(visibility_flags[i]&AEGP_DynStreamFlag_HIDDEN)!=hidden)return false;
            return true;
        };
        const int before_writes=sets;
        const auto original_values=ui_values;
        check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && all_hidden(true),
            "ordinary Emitter hides chance, parent-life window and all four inheritance controls");
        check(std::memcmp(ui_values.data(),original_values.data(),sizeof(ui_values))==0,"visibility preserves all authored parameter bytes");
        const int previous_sets=visibility_sets;
        check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && visibility_sets==previous_sets,
            "repeated Emitter UI refresh performs no redundant setter calls");
        for(A_long timing=1;timing<=4;++timing) {
            ui_values[2].param_type=PF_Param_POPUP;ui_values[2].u.pd.value=timing;
            ui_values[34].u.fs_d.value=1;
            check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && all_hidden(false),
                "Auxiliary exposes the whole bank independently of Emitting timing");
            ui_values[34].u.fs_d.value=0;
            check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && all_hidden(true),
                "ordinary Emitter stays distinct for every Emitting timing mode");
        }
#if defined(STARFIELD_NODE_KIND_EMITTER)
        PF_UserChangedParamExtra changed{};changed.param_index=guard;
        PF_OutData ui_output{};
        ui_values[guard].u.fs_d.value=1;
        check(sync_node_graph_parameter(&emitter_ui_data,&ui_output,ui_params.data(),&changed)==0 && all_hidden(false) &&
            (ui_output.out_flags&PF_OutFlag_REFRESH_UI),"actual supervised guard opens hidden controls before preset writes");
        ui_values[guard].u.fs_d.value=0;
        check(sync_node_graph_parameter(&emitter_ui_data,&ui_output,ui_params.data(),&changed)==0 && all_hidden(true),
            "actual supervised guard release restores Emitter visibility without graph publication");
#endif
        ui_values[guard].u.fs_d.value=starfield::adapter::transaction_backup_guard;
        check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && all_hidden(false),
            "backup rollback guard also exposes the Auxiliary bank");
        ui_values[guard].u.fs_d.value=0;
        check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && all_hidden(true),
            "rollback finally release restores Emitter visibility");
        auto partial=emitter_ui_data;partial.num_params=0;
        const auto before_requests=suite_requests;
        check(update_native_emitter_visibility(&partial,ui_params.data())==0 && suite_requests==before_requests,
            "partial delivery never indexes Emitter source or guard");
        dynamic_unavailable=true;
        check(update_native_emitter_visibility(&emitter_ui_data,ui_params.data())==0 && acquisitions==0,
            "optional Emitter visibility suite failure releases acquired suites and permits effect loading");
        check(sets==before_writes && acquisitions==0 && live_refs==0,"Emitter UI visibility writes no values or graph and releases every resource");
        dynamic_unavailable=false;dynamic_enabled=false;pf.AEGP_GetNewEffectForEffect=original_pf;
        visibility_flags.fill(0);
    }
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
        dynamic_enabled=false;dynamic_unavailable=true;pf.AEGP_GetNewEffectForEffect=original_pf;
        check(update_native_particle_visibility(&renderer_data,ui_params.data())==0 && acquisitions==0,"unavailable optional visibility suite cannot reject effect loading");
        dynamic_unavailable=false;
    }
    check(graph_carrier_plugin_id() == 0, "renderer registration is absent in native edit fixture");
#if defined(STARFIELD_OVERLIFE_TEST)
    {
        namespace layout=records::particle_layout;using namespace core::graph_keys;
        PF_InData particle_data=renderer_data;particle_data.effect_ref=reinterpret_cast<PF_ProgPtr>(3);
        particle_data.num_params=static_cast<A_long>(particle.params.size());
        std::vector<PF_ParamDef*> pp;for(auto& p:particle.params)pp.push_back(&p);
        particle.params[layout::size_over_life].u.fs_d.value=particle.params[layout::opacity_over_life].u.fs_d.value=100;
        for(const auto base:{layout::size_curve,layout::opacity_curve,layout::rotation_curve}) {
            PF_EventExtra event{};PF_OutData curve_out{};
            event.contextH=reinterpret_cast<PF_ContextH>(1);event.effect_win.index=base;event.effect_win.area=PF_EA_CONTROL;
            event.effect_win.current_frame={0,0,310,180};
            event.e_type=PF_Event_DO_CLICK;event.u.do_click.screen_point.h=228;
            event.u.do_click.screen_point.v=base==layout::rotation_curve?45:5;
            check(particle_rotation_curve_event(&particle_data,&curve_out,pp.data(),&event)==0 && event.u.do_click.send_drag,
                  "real Over Life editor begins a default endpoint gesture");
            event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.v=65;event.u.do_click.last_time=TRUE;
            const auto before=main.values[kGraphRevisionId].one_d;
            check(particle_rotation_curve_event(&particle_data,&curve_out,pp.data(),&event)==0 && main.values[kGraphRevisionId].one_d==before+1,
                  "real editor and native sync publish one acknowledged revision");
            auto graph=saved_graph();
            const auto key=base==layout::size_curve?kSizeOverLifeCurve:base==layout::opacity_curve?kOpacityOverLifeCurve:kRotationOverLife;
            core::AgeCurve curve{};
            check(core::decode_age_curve(std::get<core::OpaqueBytes>(parameter(graph,kParticleNode,key)),curve,
                  base==layout::rotation_curve?-32768:0,base==layout::rotation_curve?32768:100) && curve.count==2 &&
                  std::abs(curve.points[1].value-(base==layout::rotation_curve?-180:25))<1e-9,
                  "published graph contains exact complete edited curve before source streams settle");
            // AE owns saving the source bank after the PF event returns.
            for(int i=0;i<layout::curve_span;++i)particle.values[base+i].one_d=particle.params[base+i].u.fs_d.value;
            auto frame_data=renderer_data;frame_data.num_params=0;frame_data.current_time=0;
            frame_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex index,A_long time,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
                *out={};out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=evaluated_binding(index,time);return 0;};
            frame_data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
            const auto curve_bytes=std::get<core::OpaqueBytes>(parameter(graph,kParticleNode,key));
            check(sample_native_node_animation(&frame_data,graph,1920,1080)==0 &&
                  std::get<core::OpaqueBytes>(parameter(graph,kParticleNode,key))==curve_bytes,
                  "SmartFX-style current-frame sampling retains the exact constant Over Life bank");
            core::NeverCancelled never;
            auto scene=core::evaluate_particle_graph(graph,{1,1},never);
            check(scene.has_value() && !scene.value().particles.empty(),"published Over Life graph evaluates surviving particles");
            if(scene.has_value())for(const auto& p:scene.value().particles) {
                const double age=p.age_seconds/p.lifetime_seconds;
                const auto expected=core::evaluate_age_curve(curve,age,100,100);
                check(std::abs((base==layout::size_curve?p.size_pixels:base==layout::opacity_curve?p.opacity:p.rotation_degrees.z)-
                      (base==layout::size_curve?10*expected/100:base==layout::opacity_curve?expected/100:expected))<1e-8,
                      "edited curve reaches actual particle size, opacity or rotation");
            }
        }
        for(const auto delivered:{0,1,static_cast<int>(particle.params.size())})
        for(const auto base:{layout::size_curve,layout::opacity_curve,layout::rotation_curve}) {
            particle_data.num_params=delivered;
            for(unsigned preset=0;preset<std::size(editor_presets::curves);++preset) {
                overlife_preset_index=static_cast<int>(preset);
                PF_EventExtra event{};PF_OutData preset_out{};
                event.contextH=reinterpret_cast<PF_ContextH>(1);event.effect_win.index=base;event.effect_win.area=PF_EA_CONTROL;
                event.effect_win.current_frame={0,0,310,180};event.e_type=PF_Event_DO_CLICK;
                event.u.do_click.screen_point.h=145;event.u.do_click.screen_point.v=100;
                const auto error=particle_rotation_curve_event(&particle_data,&preset_out,pp.data(),&event);
                check(error==0,"every production Over Life preset applies through actual UI/native boundary for each delivered count");
                auto preset_graph=saved_graph();
                const auto key=base==layout::size_curve?kSizeOverLifeCurve:base==layout::opacity_curve?kOpacityOverLifeCurve:kRotationOverLife;
                check(std::get<core::OpaqueBytes>(parameter(preset_graph,kParticleNode,key))==core::encode_age_curve(editor_presets::curves[preset].value),
                      "production preset reaches render graph exactly, including dense Draw and interpolation");
                check(particle_data.num_params==delivered,"custom editor does not fabricate PF delivered array count");
                for(int i=0;i<layout::curve_span;++i)particle.values[base+i].one_d=particle.params[base+i].u.fs_d.value;
            }
        }
        overlife_preset_index=-1;particle_data.num_params=static_cast<A_long>(particle.params.size());
        {
            auto bank=edit(1,layout::size_curve,particle.params[layout::size_curve].u.fs_d.value);
            check(node_sync::capture_age_bank(pp.data(),bank) && node_sync::valid_custom_bank(bank),"complete custom bank captures its own identity and typed values");
            const auto revision=main.values[kGraphRevisionId].one_d;const auto writes=sets;
            const auto suite_before=suite_requests;
            for(unsigned variant=0;variant<4;++variant) {
                auto invalid=bank;
                if(variant==0)--invalid.additional_count;
                if(variant==1)invalid.uuid={};
                if(variant==2)invalid.node_kind=3;
                if(variant==3)invalid.additional_fields[1].index=layout::opacity_curve;
                PF_OutData invalid_out{};
                check(sync_custom_node_graph_bank(&particle_data,&invalid_out,invalid)!=0 && std::strstr(invalid_out.return_msg,"invalid owned bank"),
                      "truncated, identity-less, foreign-kind and mixed-bank custom requests fail closed");
            }
            check(suite_requests==suite_before && sets==writes,"malformed owned banks reject before host reads or writes");
            auto missing=bank;missing.uuid[7]=333;PF_OutData missing_out{};
            check(sync_custom_node_graph_bank(&particle_data,&missing_out,missing)!=0 && std::strstr(missing_out.return_msg,"identity changed"),
                  "owned bank must match a live host source identity");
            {
                EffectUiExclusion busy;PF_OutData busy_out{};
                check(bool(busy) && sync_custom_node_graph_bank(&particle_data,&busy_out,bank)!=0 && std::strstr(busy_out.return_msg,"UI transaction active"),
                      "custom bank cannot bypass an existing UI transaction");
            }
            main.values[kGraphSyncGuardId].one_d=1;PF_OutData guarded_out{};
            check(sync_custom_node_graph_bank(&particle_data,&guarded_out,bank)!=0 && std::strstr(guarded_out.return_msg,"renderer missing or guarded"),
                  "custom bank honors authoritative renderer guard");
            main.values[kGraphSyncGuardId].one_d=0;
            check(main.values[kGraphRevisionId].one_d==revision && sets==writes,"all rejected owned requests preserve exact graph revision and host values");
        }
        // A custom editor must never save PF changes when the graph was not
        // published. Reproduce the old successful-no-op under the source guard.
        for(const auto base:{layout::size_curve,layout::opacity_curve,layout::rotation_curve}) {
            PF_EventExtra event{};PF_OutData curve_out{};
            event.contextH=reinterpret_cast<PF_ContextH>(1);event.effect_win.index=base;event.effect_win.area=PF_EA_CONTROL;
            event.effect_win.current_frame={0,0,310,180};event.e_type=PF_Event_DO_CLICK;
            event.u.do_click.screen_point.h=145;event.u.do_click.screen_point.v=100;overlife_preset_index=0;
            const auto before=particle.params;const auto revision=main.values[kGraphRevisionId].one_d;
            particle.params[records::sync_guard_index(records::Kind::particle)].u.fs_d.value=1;
            particle.values[records::sync_guard_index(records::Kind::particle)].one_d=1;
            check(particle_rotation_curve_event(&particle_data,&curve_out,pp.data(),&event)!=0 &&
                  main.values[kGraphRevisionId].one_d==revision,"guarded curve edit is rejected rather than acknowledged without publication");
            bool restored=true;for(int i=0;i<layout::curve_span;++i)restored&=
                std::memcmp(&particle.params[base+i],&before[base+i],sizeof(PF_ParamDef))==0;
            check(restored && std::strstr(curve_out.return_msg,"SF-T203"),"unpublished curve restores full PF bank and reports a concrete error");
            particle.params[records::sync_guard_index(records::Kind::particle)].u.fs_d.value=0;
            particle.values[records::sync_guard_index(records::Kind::particle)].one_d=0;
        }
        check(acquisitions==0 && live_refs==0,"Over Life pipeline releases SDK suites and references");
        clear_particle_gradient_ui();
        dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));
        for(auto& p:main.params)if(p.param_type==PF_Param_ARBITRARY_DATA) {
            if(handles.contains(p.u.arb_d.value))dispose(p.u.arb_d.value);
            if(handles.contains(p.u.arb_d.dephault))dispose(p.u.arb_d.dephault);
        }
        check(handles.empty(),"Over Life pipeline releases every graph handle");
        std::printf("Over Life pipeline: %d checks, %d failures\n",checks,failures);return failures?1:0;
    }
#endif
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
        particle.params[records::sync_guard_index(records::Kind::particle)].u.fs_d.value=0;
        particle.params[layout::seed_shift].param_type=PF_Param_SLIDER;
        particle.params[layout::seed_shift].u.sd.value=-19;
        particle.params[layout::birth_chance].u.fs_d.value=100;
        PF_UserChangedParamExtra birth_changed{};birth_changed.param_index=layout::seed_shift;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&birth_changed)==0 &&
              particle.params[layout::birth_enabled].u.fs_d.value==1 &&
              (particle.params[layout::birth_enabled].uu.change_flags&PF_ChangeFlag_CHANGED_VALUE) &&
              std::get<std::int32_t>(parameter(saved_graph(),kParticleNode,kParticleSeedShift))==-19 &&
              std::get<double>(parameter(saved_graph(),kParticleNode,kParticleBirthChance))==100,
              "integer Shift Seed USER_CHANGED activates and persists both birth controls atomically");
        particle.params[layout::birth_enabled].u.fs_d.value=0;particle.params[layout::birth_enabled].uu.change_flags=0;
        fail_set=kGraphParameterId;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&birth_changed)!=0 &&
              particle.params[layout::birth_enabled].u.fs_d.value==0 && particle.params[layout::birth_enabled].uu.change_flags==0,
              "failed Birth callback retains the prior activation without marking a successful change");
        birth_changed.param_index=layout::birth_chance;particle.params[layout::birth_chance].u.fs_d.value=101;
        check(sync_node_graph_parameter(&particle_data,&cloud_out,pp.data(),&birth_changed)!=0 &&
              particle.params[layout::birth_enabled].u.fs_d.value==0,"invalid chance cannot activate the appended controls");
        check(live_refs==0 && acquisitions==0,"Birth callbacks release all host references");
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
    emitter.values[records::sync_guard_index(records::Kind::emitter)].one_d=transaction_removal_guard;
    check(sync_node_graph_parameter(&node_data,&output,pointers.data(),&changed)==0 && sets==before_sets,
        "stale guard0 delivery defers publication when saved source is retired under guard3");
    emitter.values[records::sync_guard_index(records::Kind::emitter)].one_d=0;
    changed.param_index = records::layout_x_index(records::Kind::emitter);
    check(sync_node_graph_parameter(&node_data, &output, pointers.data(), &changed) == 0 && sets == before_sets, "metadata awaits CEP transaction");
    auto life = edit(1, 2, 9); check(direct_edit(life) == 0 && life.accepted, "Particle Life publishes independently");
    particle.values[2].one_d = 9; // Host saves Life after its native callback returns.
    auto local_edit = edit(1, 5, 15); PF_OutData local_output{};
    check(commit_native_graph_edit(&local_edit, 701, &local_output) == 0 && local_edit.accepted,
          "publication needs no renderer callback or registered main AEGP ID");
    check(std::get<double>(parameter(saved_graph(), core::graph_keys::kParticleNode, core::graph_keys::kParticleLifetimeSeconds)) == 9, "Particle lifetime saved");
    {
        namespace layout=records::particle_layout;
        const auto select_texture=[&](A_long index,AEGP_LayerIDVal id) {
            // The custom selector saves its authoritative constant stream under
            // a guard before the native commit, matching the production path.
            particle.values[index].layer_id=id;
            auto request=edit(1,index,0);
            const auto result=direct_edit(request);
            return result==0 && request.accepted;
        };
        check(select_texture(layout::texture_front,77),"native front selection uses borrowed owner with absent local PF effect_ref");
        check(std::get<std::uint32_t>(parameter(saved_graph(),core::graph_keys::kParticleNode,core::graph_keys::kTextureFront))==77 &&
            main.values[kTextureResourceCountIndex].one_d==1 && main.values[kTextureResourceFirstIndex].layer_id==77,
            "front resource reaches graph and renderer-owned checkout slot");
        check(select_texture(layout::texture_back,78) && main.values[kTextureResourceCountIndex].one_d==2 &&
            main.values[kTextureResourceFirstIndex+1].layer_id==78,"native back selection publishes the second resource");
        auto subsequent=edit(1,2,9);
        check(direct_edit(subsequent)==0 && subsequent.accepted,"other Particle edits retain selected texture resources");
        const auto textured=saved_graph();
        const auto bytes=handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
        const auto before_revision=main.values[kGraphRevisionId].one_d;
        const auto before_expressions_storage=std::make_unique<decltype(expressions)>(expressions);const auto& before_expressions=*before_expressions_storage;
        particle.values[layout::texture_front].layer_id=79;
        auto failed_selection=edit(1,layout::texture_front,0);fail_set=kGraphParameterId;
        check(direct_edit(failed_selection)!=0 && !failed_selection.accepted &&
            main.values[kGraphRevisionId].one_d==before_revision &&
            handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes==bytes &&
            main.values[kTextureResourceCountIndex].one_d==2 && main.values[kTextureResourceFirstIndex].layer_id==77 &&
            main.values[kTextureResourceFirstIndex+1].layer_id==78 && expressions==before_expressions,
            "failed graph publication restores texture slots, graph, revision and animation bindings");
        particle.values[layout::texture_front].layer_id=77; // Selector restores its source stream after rejection.
        const auto before_rejected_sets=sets;
        for(auto id:{1,99}) {
            particle.values[layout::texture_front].layer_id=id;
            auto invalid_source=edit(1,layout::texture_front,0);
            check(direct_edit(invalid_source)!=0 && !invalid_source.accepted && sets==before_rejected_sets &&
                invalid_source.binding_stage && std::strcmp(invalid_source.binding_stage,"validate texture source layers")==0,
                "self and deleted texture sources reject before any writes");
        }
        particle.values[layout::texture_front].layer_id=77;
        auto local=renderer_data;local.effect_ref=nullptr;
        {
            NativeBindingTransaction bindings(&local,701,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1));
            check(bindings.install(textured)==0,"binding transaction accepts validated borrowed owner without PF context");bindings.accept();
        }
        {
            NativeBindingTransaction bindings(&local,701,effect_ref(0));const char* stage=nullptr;
            check(bindings.install(textured,nullptr,&stage)==PF_Err_BAD_CALLBACK_PARAM && stage &&
                std::strcmp(stage,"resolve binding owner layer")==0,"missing owner and PF context fails before a host query");
        }
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(textured)==0,"main-effect texture transaction still resolves its real PF effect");bindings.accept();
        }
        check(select_texture(layout::texture_front,0) && main.values[kTextureResourceCountIndex].one_d==1 &&
            main.values[kTextureResourceFirstIndex].layer_id==78 && main.values[kTextureResourceFirstIndex+1].layer_id==0,
            "front deselection retains back and clears unused resource tail");
        check(select_texture(layout::texture_back,0) && main.values[kTextureResourceCountIndex].one_d==0 &&
            main.values[kTextureResourceFirstIndex].layer_id==0,"last deselection clears all renderer resources");
        check(null_pf_layer_queries==0 && live_refs==0 && acquisitions==0,"texture transactions never query a null PF effect and release all suites and streams");
    }
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
    retirement_marker_matrix_tests();
    compiler_reuse_tests();
    const auto expression_baseline_storage=std::make_unique<decltype(expressions)>(expressions);const auto& expression_baseline=*expression_baseline_storage;
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
    const auto before_failed_binding_storage=std::make_unique<decltype(expressions)>(expressions);const auto& before_failed_binding=*before_failed_binding_storage;
    fail_set = kGraphParameterId;
    auto binding_failure = edit(1, 5, 22);
    main.values[kGraphRevisionId].one_d = 100; // Avoid revision exhaustion for this transaction.
    check(direct_edit(binding_failure) != 0 && expressions == before_failed_binding,
          "graph publication failure restores all changed binding expressions");
    expressions = expression_baseline;
    expressions[0]=u"0";
    const auto disabled_expression_baseline_storage=std::make_unique<decltype(expressions)>(expressions);const auto& disabled_expression_baseline=*disabled_expression_baseline_storage;
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
            if(index==kGraphRevisionId || index==kNodeEffectsReadyId) output->u.fs_d.value=fixtures[0].values[index].one_d;
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
    {
        // A saved UI guard cannot update an already delivered PF snapshot.
        // Exercise the full render entry, not just the alias sampler, while the
        // actual resident Host seam owns a structural transaction.
        auto module=LoadLibraryW(L"artifacts\\model-ui-exclusion-tests\\paired\\StarfieldHost.aex");
        check(module!=nullptr,"render snapshot fixture loads actual Host exclusion module");
        if(module) {
            const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));
            const auto begin=std::bit_cast<std::uint64_t(*)() noexcept>(GetProcAddress(module,"SFLD_BeginUiExclusionV1"));
            const auto end=std::bit_cast<void(*)(std::uint64_t) noexcept>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));
            check(initialize && begin && end,"render snapshot fixture resolves Host exclusion exports");
            if(initialize && begin && end) {
                initialize();
                PF_ArbitraryH old_graph{};
                check(create_graph_parameter(&renderer_data,animated_graph,&old_graph)==0,"old active graph fixture is owned");
                const auto saved_graph_value=main.values[kGraphParameterId].arbH;
                main.values[kGraphParameterId].arbH=reinterpret_cast<AEGP_ArbBlockVal>(old_graph);
                static auto snapshot_checkout=renderer_data.inter.checkout_param;
                static auto snapshot_checkin=renderer_data.inter.checkin_param;
                static unsigned outstanding{},alias_reads{};
                auto snapshot=renderer_data;
                snapshot.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* out)->PF_Err {
                    const auto error=snapshot_checkout(ref,index,time,step,scale,out);
                    if(!error) {
                        ++outstanding;
                        if(index==kGraphSyncGuardId) {out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=0;}
                        if(index>=kNativeBindingFirstIndex && index<kNativeBindingFirstIndex+kNativeBindingCapacity)++alias_reads;
                    }
                    return error;
                };
                snapshot.inter.checkin_param=[](PF_ProgPtr ref,PF_ParamDef* value)->PF_Err {
                    check(outstanding>0,"render snapshot checks in only owned parameters");--outstanding;
                    return snapshot_checkin(ref,value);
                };
                remember_native_control_proofs(&snapshot,{});
                absent_source_uuid=4;
                std::shared_ptr<const core::Graph> result;PF_OutData diagnostic{};A_long source=-1;
                check(checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080)==PF_Err_BAD_CALLBACK_PARAM &&
                    !result && std::strstr(diagnostic.return_msg,"binding unavailable") && alias_reads>0 && outstanding==0,
                    "idle full render preserves a real missing Force binding failure");
                const auto saved_guard=main.values[kGraphSyncGuardId].one_d;
                main.values[kGraphSyncGuardId].one_d=1; // Live saved state; snapshot callback still delivers0.
                const auto token=begin();check(token && ui_transaction_active(),"structural transaction owns actual resident exclusion");
                {
                    PF_Err worker_error{};
                    std::thread worker([&]{worker_error=checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080);});worker.join();
                    check(worker_error==PF_Err_BAD_CALLBACK_PARAM && std::strstr(diagnostic.return_msg,"binding unavailable"),
                        "ordinary UI exclusion alone does not fence worker renders or hide missing bindings");
                }
                const auto begin_graph=std::bit_cast<std::uint64_t(*)() noexcept>(GetProcAddress(module,"SFLD_BeginGraphMutationV1"));
                const auto end_graph=std::bit_cast<void(*)(std::uint64_t) noexcept>(GetProcAddress(module,"SFLD_EndGraphMutationV1"));
                const auto mutation=begin_graph?begin_graph():0;
                if(begin_graph)check(mutation && end_graph,"explicit graph mutation is owned by the UI executor");
                {
                    PF_Err worker_error{};bool worker_ui=true;
                    std::thread worker([&]{worker_ui=ui_transaction_active();
                        worker_error=checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080);});worker.join();
                    check(!worker_ui && worker_error==PF_Interrupt_CANCEL && !result && source==-1 &&
                        !diagnostic.return_msg[0] && outstanding==0,
                        "worker stale guard0 cancels during owned structural mutation without relying on UI thread identity");
                }
                if(mutation && end_graph)end_graph(mutation);
                alias_reads=0;std::strcpy(diagnostic.return_msg,"previous frame failure");
                check(checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080)==PF_Interrupt_CANCEL &&
                    !result && source==-1 && !diagnostic.return_msg[0] && alias_reads==0 && outstanding==0,
                    "stale guard0 full render cancels before aliases inside structural transaction");
                end(token);check(!ui_transaction_active(),"render cancellation does not retain transaction ownership");
                alias_reads=0;
                check(checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080)==PF_Err_BAD_CALLBACK_PARAM &&
                    !result && std::strstr(diagnostic.return_msg,"binding unavailable") && alias_reads>0 && outstanding==0,
                    "transaction release cannot hide a persistent missing binding");
                {
                    static auto late_checkout=snapshot.inter.checkout_param;
                    static auto late_checkin=snapshot.inter.checkin_param;
                    static auto late_begin=begin;
                    static std::uint64_t late_token{};
                    static PF_ParamIndex late_index{};
                    static bool enter_on_checkin{};
                    snapshot.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* out)->PF_Err {
                        const auto error=late_checkout(ref,index,time,step,scale,out);
                        if(!error && !enter_on_checkin && index==late_index && !late_token)late_token=late_begin();
                        return error;
                    };
                    snapshot.inter.checkin_param=[](PF_ProgPtr ref,PF_ParamDef* value)->PF_Err {
                        const auto error=late_checkin(ref,value);
                        if(enter_on_checkin && !late_token)late_token=late_begin();
                        return error;
                    };
                    for(bool missing:{true,false})for(bool checkin_entry:{false,true}) {
                        absent_source_uuid=missing?4:0;enter_on_checkin=checkin_entry;
                        for(A_long index:{A_long{kGraphParameterId},kNativeBindingFirstIndex}) {
                            late_index=index;alias_reads=0;source=999;
                            const auto error=checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080);
                            check(late_token && ui_transaction_active() && error==PF_Interrupt_CANCEL && !result &&
                                source==-1 && !diagnostic.return_msg[0] && outstanding==0,
                                "late checkout/checkin transaction cancels both complete and failed snapshots");
                            end(late_token);late_token=0;
                        }
                    }
                    snapshot.inter.checkout_param=late_checkout;snapshot.inter.checkin_param=late_checkin;
                }
                if(begin_graph && end_graph){
                    static auto settled_checkout=snapshot.inter.checkout_param;
                    static auto settled_begin=begin;
                    static auto settled_end=end;
                    static auto mutation_begin=begin_graph;
                    static auto mutation_end=end_graph;
                    static PF_ParamIndex mutation_index{};
                    static bool modified{};
                    snapshot.inter.checkout_param=[](PF_ProgPtr ref,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* out)->PF_Err {
                        const auto error=settled_checkout(ref,index,time,step,scale,out);
                        if(!error && !modified && index==mutation_index){
                            modified=true;const auto owner=settled_begin(),mutation=mutation_begin();
                            check(owner && mutation,"checkout fixture runs a complete owned structural mutation");
                            mutation_end(mutation);settled_end(owner);
                        }return error;
                    };
                    for(bool missing:{true,false})for(A_long index:{A_long{kGraphParameterId},kNativeBindingFirstIndex}){
                        absent_source_uuid=missing?4:0;mutation_index=index;modified=false;
                        remember_native_control_proofs(&snapshot,{});
                        const auto error=checkout_render_graph(&snapshot,&diagnostic,result,&source,1920,1080);
                        check(modified && !ui_transaction_active() && !(graph_mutation_epoch()&1) &&
                            error==PF_Interrupt_CANCEL && !result && source==-1 && !diagnostic.return_msg[0] && outstanding==0,
                            "a completed mutation during checkout cancels both valid and failed snapshots");
                    }
                    snapshot.inter.checkout_param=settled_checkout;
                }
                main.values[kGraphSyncGuardId].one_d=saved_guard;absent_source_uuid=0;
                main.values[kGraphParameterId].arbH=saved_graph_value;
                dispose(old_graph);
            }
            check(FreeLibrary(module)!=0,"render snapshot fixture releases its Host module");
        }
    }
    {
        using namespace core::graph_keys;
        auto orphan=animated_graph;core::NodeId particle_id{},output_id{};
        for(const auto& node:orphan.nodes) {
            if(node.type_key==kParticleNode)particle_id=node.id;
            if(node.type_key==kOutputNode)output_id=node.id;
        }
        std::erase_if(orphan.edges,[&](const auto& edge){return edge.destination_node==output_id || edge.source_node==particle_id;});
        core::EdgeId route{};route.value.bytes[0]=240;
        orphan.edges.push_back({route,particle_id,kParticleParticlesOut,output_id,kOutputParticles});
        const auto texts_storage=std::make_unique<decltype(expressions)>(expressions);const auto& texts=*texts_storage;const auto enabled=expression_enabled;const auto bases=main.values;
        absent_source_uuid=4;
        auto sampled=orphan;
        check(sample_native_node_animation(&renderer_data,sampled,1920,1080)==0,"disconnected deleted Force cannot block Output preview");
        auto active=animated_graph;
        check(sample_native_node_animation(&renderer_data,active,1920,1080)==PF_Err_BAD_CALLBACK_PARAM,"active deleted Force remains a strict failure");
        auto output_only=orphan;output_only.edges.clear();
        std::erase_if(output_only.nodes,[](const auto& node){return node.type_key!=kOutputNode;});
        output_only.optional_records.clear();
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(output_only,nullptr,nullptr,nullptr,&orphan)==0 && bindings.retire_after_publication()==0,
                "explicit edit can retire a disconnected missing source using its finite saved value");
        }
        check(expressions==texts && expression_enabled==enabled,"orphan retirement rollback preserves exact alias dependencies");
        for(std::size_t i=kNativeBindingFirstIndex;i<main.values.size();++i)
            check(main.values[i].one_d==bases[i].one_d,"orphan retirement rollback preserves exact bases");
        absent_source_uuid=0;
    }
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
          missing_stream==kNativeBindingFirstIndex && std::strcmp(failed_stage,"binding unavailable")==0,
          "unresolved expression output cannot silently zero particle settings");

    // Real UI metadata reader and native static preparation, using ParamUtils
    // tokens for the owned aliases. No AEGP suite is queried in render preparation.
    temporal_metadata_enabled=true;
    param_utils.PF_GetCurrentState=[](PF_ProgPtr,PF_ParamIndex index,const A_Time* start,const A_Time* duration,PF_State* state)->PF_Err {
        ++metadata_state_queries;
        if(index==PF_ParamIndex_CHECK_ALL) {
            ++metadata_whole_queries;
            if(metadata_whole_fail_at && metadata_whole_queries==metadata_whole_fail_at)return PF_Err_BAD_CALLBACK_PARAM;
            if(metadata_whole_change_at_finish && metadata_whole_queries==2)++metadata_epoch;
        }
        check(!start && !duration,"native proof covers all time");*state={};std::memcpy(state,&metadata_epoch,sizeof(metadata_epoch));
        unsigned generation{};
        if(index>=kNativeBindingFirstIndex && index<kNativeBindingFirstIndex+kNativeBindingCapacity)
            generation=dependency_generations[index-kNativeBindingFirstIndex];
        else for(const auto n:dependency_generations)generation+=n;
        std::memcpy(reinterpret_cast<char*>(state)+sizeof(metadata_epoch),&generation,sizeof(generation));return 0;
    };
    param_utils.PF_AreStatesIdentical=[](PF_ProgPtr,const PF_State* a,const PF_State* b,A_Boolean* same)->PF_Err {*same=std::memcmp(a,b,sizeof(*a))==0;return 0;};
    stream.AEGP_CanVaryOverTime=[](AEGP_StreamRefH ref,A_Boolean* vary)->A_Err {auto* r=reinterpret_cast<Ref*>(ref);*vary=(metadata_rate_keys && r->effect==1 && r->index==3) ||
        ((metadata_life_keys || metadata_life_expression) && r->effect==2 && r->index==2) ||
        (metadata_chance_expression && r->effect==2 && r->index==records::particle_layout::birth_chance);return 0;};
    stream.AEGP_GetExpressionState=[](AEGP_PluginID,AEGP_StreamRefH ref,A_Boolean* enabled)->A_Err {
        auto* r=reinterpret_cast<Ref*>(ref);
        *enabled=!r->effect?expression_enabled[r->index-kNativeBindingFirstIndex]:r->effect==2 &&
            ((metadata_life_expression && r->index==2) || (metadata_chance_expression && r->index==records::particle_layout::birth_chance));return 0;
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
    {
        const auto baseline_trace=last_native_metadata_trace();
        auto parked_force=animated_graph;parked_force.edges=static_graph.edges;
        absent_source_uuid=4;
        capture_native_temporal_metadata(&renderer_data,parked_force,1);
        const auto trace=last_native_metadata_trace();
        check(trace.error==0 && trace.inputs==baseline_trace.inputs && trace.evaluated==trace.inputs,
            "idle metadata ignores disconnected absent sources and completes instead of retrying forever");
        const auto parked_records=parked_force.optional_records.size();PF_OutData parked_output{};
        check(capture_emitter_origin_history(&renderer_data,&parked_output,parked_force,1920,1080,core::NeverCancelled{})==0 &&
            last_native_history_trace().path==NativeHistoryPath::static_graph &&
            last_native_history_trace().checkouts==0 && last_native_history_trace().node_queries==0 &&
            parked_force.optional_records.size()==parked_records,
            "PERF-02 parked absent Force does not force history traversal or frozen particle encoding");
        absent_source_uuid=0;
    }
    auto proofs=validated_native_control_proofs(&renderer_data);
    check(!proofs.empty() && std::all_of(proofs.begin(),proofs.end(),[](const auto& p){return p.constant;}),"source metadata certifies genuinely unanimated aliases");
    NativeAnimationPlan constant_plan(static_graph,1920,1080,1);constant_plan.prepare_constants(&renderer_data);
    check(constant_plan.fully_constant(),"native plan verifies static controls once per frame");
    check(constant_plan.constant_node(particle_id)!=nullptr,"constant particle node is converted once per frame");
    {
        const auto previous_checkout=renderer_data.inter.checkout_param;
        const auto previous_time=renderer_data.current_time;
        static unsigned value_cache_checkouts{};
        renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* output)->PF_Err {
            ++value_cache_checkouts;*output={};output->param_type=PF_Param_FLOAT_SLIDER;
            output->u.fs_d.value=evaluated_binding(index,0);return 0;
        };
        renderer_data.current_time=0;++metadata_epoch;
        auto reference=static_graph;
        check(sample_native_node_animation(&renderer_data,reference,1920,1080)==0 && value_cache_checkouts>0,
            "PERF-06 missing numeric publication retains actual alias checkout");
        capture_native_temporal_metadata(&renderer_data,static_graph,1);
        const auto published=validated_native_control_proofs(&renderer_data);
        check(!published.empty() && std::all_of(published.begin(),published.end(),[](const auto& proof){return proof.constant && proof.constant_value && std::isfinite(*proof.constant_value);}),
            "PERF-06 UI publishes actual finite alias values only for certified constants");
        metadata_state_queries=0;
        check(validated_native_control_proofs(&renderer_data).size()==published.size() && metadata_state_queries==1,
            "PERF-07 actual UI capture certifies a complete receipt and stable preparation queries it once");
        value_cache_checkouts=0;
        for(A_long frame:{0,24,120}) {
            auto callback=renderer_data;callback.current_time=frame;callback.effect_ref=reinterpret_cast<PF_ProgPtr>(0x667788);
            auto cached=static_graph;
            const auto a=core::serialize_graph(reference,core::particle_node_registry());
            const auto b_error=sample_native_node_animation(&callback,cached,1920,1080);
            const auto b=core::serialize_graph(cached,core::particle_node_registry());
            check(b_error==0 && a.has_value() && b.has_value() && a.value()==b.value(),
                "PERF-06 certified values preserve the complete current-frame graph across time and callback references");
        }
        check(value_cache_checkouts==0,"PERF-06 unchanged certified frames perform no numeric alias checkout");
        NativeAnimationPlan numeric_plan(static_graph,1920,1080,1);numeric_plan.prepare_constants(&renderer_data);
        check(numeric_plan.fully_constant() && numeric_plan.checkout_count()==0,
            "PERF-06 history hoists the same owned certified values without duplicate checkout");
        const auto life=std::find_if(published.begin(),published.end(),[](const auto& proof){return proof.stream==fixture_life_stream;});
        check(life!=published.end(),"PERF-06 fixture has a certified Life alias");
        if(life!=published.end()) {
            const auto life_offset=static_cast<std::size_t>(life-published.begin());
            for(const auto invalid_value:{std::optional<double>{},std::optional<double>{std::numeric_limits<double>::quiet_NaN()},std::optional<double>{kNativeBindingUnavailable}}) {
                auto altered=published;altered[life_offset].constant_value=invalid_value;
                remember_native_control_proofs(&renderer_data,std::move(altered));value_cache_checkouts=0;
                auto sampled=static_graph;
                check(sample_native_node_animation(&renderer_data,sampled,1920,1080)==0 && value_cache_checkouts==1,
                    "PERF-06 absent/nonfinite/unavailable cached number falls back to exactly its real alias");
            }
            auto altered=published;altered[life_offset].node.value.bytes[0]^=0x80;
            remember_native_control_proofs(&renderer_data,std::move(altered));value_cache_checkouts=0;auto sampled=static_graph;
            check(sample_native_node_animation(&renderer_data,sampled,1920,1080)==0 && value_cache_checkouts==1,
                "PERF-06 cached number from another UUID cannot fill the same slot");
        }
        remember_native_control_proofs(&renderer_data,published);++metadata_epoch;value_cache_checkouts=0;
        auto invalidated=static_graph;
        check(sample_native_node_animation(&renderer_data,invalidated,1920,1080)==0 && value_cache_checkouts==published.size(),
            "PERF-06 dependency change invalidates every old number and restores checkout");
        for(const auto fail_at:{1u,2u}) {
            metadata_whole_queries=0;metadata_whole_fail_at=fail_at;++metadata_epoch;
            capture_native_temporal_metadata(&renderer_data,static_graph,1);
            metadata_whole_fail_at=0;metadata_state_queries=0;
            check(validated_native_control_proofs(&renderer_data).size()==published.size() && metadata_state_queries==published.size(),
                "PERF-07 failed outer before/after query publishes only individually validated proofs");
        }
        metadata_whole_queries=0;metadata_whole_change_at_finish=true;++metadata_epoch;
        capture_native_temporal_metadata(&renderer_data,static_graph,1);metadata_whole_change_at_finish=false;
        check(validated_native_control_proofs(&renderer_data).empty(),
            "PERF-07 change after individual certification cannot attach a newer whole receipt to stale values");
        capture_native_temporal_metadata(&renderer_data,static_graph,1);
        static decltype(stream.AEGP_GetNewStreamValue) real_alias_read{};
        static unsigned numeric_read_mode{},life_reads{};
        real_alias_read=stream.AEGP_GetNewStreamValue;
        stream.AEGP_GetNewStreamValue=[](AEGP_PluginID id,AEGP_StreamRefH ref,AEGP_LTimeMode mode,const A_Time* time,A_Boolean pre,AEGP_StreamValue2* out)->A_Err {
            const auto& key=*reinterpret_cast<Ref*>(ref);
            const bool guarded=!key.effect && !pre && key.index==fixture_life_stream && ++life_reads==2;
            if(guarded && numeric_read_mode==2)return PF_Err_BAD_CALLBACK_PARAM;
            const auto result=real_alias_read(id,ref,mode,time,pre,out);
            if(!result && guarded) {
                if(numeric_read_mode==1)++metadata_epoch;
                if(numeric_read_mode==3)out->val.one_d=std::numeric_limits<double>::quiet_NaN();
                if(numeric_read_mode==4)out->val.one_d=kNativeBindingUnavailable;
            }
            return result;
        };
        for(unsigned mode:{1u,2u,3u,4u}) {
            numeric_read_mode=mode;life_reads=0;++metadata_epoch;
            capture_native_temporal_metadata(&renderer_data,static_graph,1);
            const auto guarded=validated_native_control_proofs(&renderer_data);
            check(life_reads==2 && std::none_of(guarded.begin(),guarded.end(),[](const auto& proof){return proof.stream==fixture_life_stream;}),
                "PERF-06 changed dependency/read failure/NaN/unavailable inside the state pair cannot publish the earlier warm value");
        }
        stream.AEGP_GetNewStreamValue=real_alias_read;
        auto unavailable=static_graph;A_long unavailable_stream=-1;const char* unavailable_stage=nullptr;
        unresolved_alias=fixture_life_stream;
        check(sample_native_node_animation(&renderer_data,unavailable,1920,1080,&unavailable_stream,&unavailable_stage)==PF_Err_BAD_CALLBACK_PARAM &&
            unavailable_stream==fixture_life_stream && unavailable_stage && std::strcmp(unavailable_stage,"binding unavailable")==0,
            "PERF-06 failed value publication retains the actual missing-binding error instead of serving the previous value");
        unresolved_alias=-1;
        renderer_data.inter.checkout_param=previous_checkout;renderer_data.current_time=previous_time;
        capture_native_temporal_metadata(&renderer_data,static_graph,1);constant_plan.prepare_constants(&renderer_data);
    }
#if defined(STARFIELD_TEST_NATIVE_ALLOCATIONS)
    {
        const auto old_life=particle.values[2].one_d;const auto frame_checkout=renderer_data.inter.checkout_param;
        static double aliased_life;aliased_life=old_life;
        // The older deterministic fixture hard-codes Life=2 at t0. This case
        // changes both the actual author value and its owned checkout value.
        renderer_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out)->PF_Err {
            *out={};out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=index==fixture_life_stream?aliased_life:evaluated_binding(index,0);return 0;
        };
        NativeAnimationPlan reused(static_graph,1920,1080,1);reused.prepare_constants(&renderer_data);
        check(reused.fully_constant(),"allocation regression starts with a fully hoisted plan");
        reused.prepare_constants(nullptr);
        check(!reused.fully_constant() && reused.proofs().empty() && !reused.constant_node(particle_id),
            "invalid new preparation context clears previous certificates before returning");
        reused.prepare_constants(&renderer_data);
        particle.values[2].one_d=old_life+1;aliased_life=old_life+1;++metadata_epoch;
        native_allocation_failed=false;native_fail_allocation_after=0;
        reused.prepare_constants(&renderer_data);native_fail_allocation_after=-1;
        check(native_allocation_failed && !reused.fully_constant() && reused.proofs().empty() && !reused.constant_node(particle_id),
            "allocation failure before new proof lookup cannot retain an old certificate");
        core::GraphNode changed;check(reused.sample(&renderer_data,particle_id,changed)==0,
            "failed optimization retains actual owned-alias historical sampling");
        const auto life=std::find_if(changed.parameters.begin(),changed.parameters.end(),[](const auto& p){return p.key==core::graph_keys::kParticleLifetimeSeconds;});
        check(life!=changed.parameters.end() && std::get<double>(life->value)==old_life+1,
            "changed Life after failed reprepare is read from AE rather than the previous hoisted raw value");
        particle.values[2].one_d=old_life;renderer_data.inter.checkout_param=frame_checkout;
        capture_native_temporal_metadata(&renderer_data,static_graph,1);
        constant_plan.prepare_constants(&renderer_data);
    }
#endif
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
    const auto complex_saved_spin=fixtures[3].values[6].one_d;
    fixtures[3].values[6].one_d=10;
    check(sample_native_node_animation(&renderer_data,complex_graph,1920,1080)==0,
        "constant Spin fixture samples current native values before temporal capture");
    capture_native_temporal_metadata(&renderer_data,complex_graph,1);
    PF_OutData complex_output{};
    check(capture_emitter_origin_history(&renderer_data,&complex_output,complex_graph,1920,1080,core::NeverCancelled{})==0,
        "constant nodes work inside a temporal graph containing Force");
    const auto complex_trace=last_native_history_trace();
    check(complex_trace.path==NativeHistoryPath::temporal && complex_trace.node_queries>0 &&
        complex_trace.node_samples==0 && complex_trace.constant_node_hits==complex_trace.node_queries &&
        complex_trace.checkouts==0 && complex_trace.node_queries<=complex_graph.nodes.size(),
        "certified temporal processors compile each constant once with no duplicate current/historical checkouts");
    fixtures[3].values[6].one_d=complex_saved_spin;
    {
        const auto saved_force=fixtures[3].values;const auto saved_rate=emitter.values[3].one_d;
        fixtures[3].values[1].one_d=540;fixtures[3].values[2].one_d=37;fixtures[3].values[10].one_d=.4;
        for(A_long i:{3,4,5,6})fixtures[3].values[i].one_d=0;
        emitter.values[3].one_d=100;
        // These direct fixture assignments represent an authored dependency
        // change. Real AE changes PF_State; the fake must do so as well.
        ++metadata_epoch;
        auto force_fast=animated_graph;
        check(sample_native_node_animation(&renderer_data,force_fast,1920,1080)==0,"PERF-02 current Force values sampled before proof reuse");
        check(native_static_history_eligible(force_fast),"PERF-02 constant gravity/random/drag has a closed-form history equivalent");
        auto branched_force=force_fast;
        auto extra_force=*std::find_if(branched_force.nodes.begin(),branched_force.nodes.end(),[](const auto& n){return n.type_key==core::graph_keys::kForceNode;});
        extra_force.id.value.bytes[0]^=0x80;branched_force.nodes.push_back(extra_force);
        auto branch_edge=branched_force.edges.front();branch_edge.id.value.bytes[0]^=0x80;
        branch_edge.source_node=particle_id;branch_edge.source_port=core::graph_keys::kParticleParticlesOut;
        branch_edge.destination_node=extra_force.id;branch_edge.destination_port=core::graph_keys::kForceParticlesIn;branched_force.edges.push_back(branch_edge);
        branch_edge.id.value.bytes[0]^=0x40;branch_edge.source_node=extra_force.id;branch_edge.source_port=core::graph_keys::kForceParticlesOut;
        branch_edge.destination_node=output_id;branch_edge.destination_port=core::graph_keys::kOutputParticles;branched_force.edges.push_back(branch_edge);
        check(!native_static_history_eligible(branched_force),"PERF-02 parallel Force routes keep their existing temporal identity semantics");
        const auto change_force=[](core::Graph& graph,core::ParameterKey key,core::ParameterValue value) {
            for(auto& n:graph.nodes)if(n.type_key==core::graph_keys::kForceNode)for(auto& p:n.parameters)if(p.key==key)p.value=value;
        };
        for(const auto wind:{core::Vec3{1,0,0},core::Vec3{0,-1,0},core::Vec3{0,0,1}}) {
            auto changed_force=force_fast;change_force(changed_force,core::graph_keys::kWind,wind);
            check(!native_static_history_eligible(changed_force),"PERF-02 active Wind on every axis retains temporal integration");
        }
        auto spinning=force_fast;change_force(spinning,core::graph_keys::kSpin,1.0);
        check(!native_static_history_eligible(spinning),"PERF-02 active Spin retains temporal integration");
        for(const char* type:{core::graph_keys::kTransformNode,core::graph_keys::kMotionNode,core::graph_keys::kTurbulenceNode,
                             core::graph_keys::kModelNode,core::graph_keys::kMaterialNode}) {
            auto unsupported=force_fast;
            for(auto& n:unsupported.nodes)if(n.type_key==core::graph_keys::kForceNode)n.type_key=type;
            check(!native_static_history_eligible(unsupported),"PERF-02 complex active processor retains historical path");
        }
        for(const auto mode:{1u,2u,3u}) {
            auto timed=force_fast;
            for(auto& n:timed.nodes)if(n.type_key==core::graph_keys::kEmitterNode)
                for(auto& p:n.parameters)if(p.key==core::graph_keys::kEmittingMode)p.value=mode;
            check(!native_static_history_eligible(timed),"PERF-02 nondefault emission retains historical birth semantics");
        }
        auto random_life=force_fast;
        for(auto& n:random_life.nodes)if(n.type_key==core::graph_keys::kParticleNode)
            for(auto& p:n.parameters)if(p.key==core::graph_keys::kLifeRandom)p.value=10.;
        check(!native_static_history_eligible(random_life),"PERF-02 random Life retains historical survivor selection");
        capture_native_temporal_metadata(&renderer_data,force_fast,1);
        const auto uncaptured=force_fast;
        const auto force_records=force_fast.optional_records.size();PF_OutData force_output{};
        check(capture_emitter_origin_history(&renderer_data,&force_output,force_fast,1920,1080,core::NeverCancelled{})==0 &&
            last_native_history_trace().path==NativeHistoryPath::static_graph &&
            last_native_history_trace().checkouts==0 && last_native_history_trace().node_queries==0 &&
            force_fast.optional_records.size()==force_records,
            "PERF-02 certified constant gravity/random/drag skips history and duplicate parameter reads");
        const core::RationalTime force_time{renderer_data.current_time,renderer_data.time_scale};
        const auto fast_particles=core::evaluate_particle_graph(force_fast,force_time,core::NeverCancelled{}, {1080,1});
        ++metadata_epoch;
        auto historical=uncaptured;PF_OutData historical_output{};
        check(capture_emitter_origin_history(&renderer_data,&historical_output,historical,1920,1080,core::NeverCancelled{})==0 &&
            last_native_history_trace().path==NativeHistoryPath::temporal && last_native_history_trace().checkouts>0,
            "PERF-02 invalidated proof uses actual historical sampling instead of assuming constancy");
        const auto historical_particles=core::evaluate_particle_graph(historical,force_time,core::NeverCancelled{}, {1080,1});
        check(fast_particles.has_value() && historical_particles.has_value(),"PERF-02 both captured routes evaluate successfully");
        if(fast_particles.has_value() && historical_particles.has_value()) {
            auto a=fast_particles.value().particles,b=historical_particles.value().particles;
            const auto order=[](const auto& x,const auto& y){return x.id<y.id;};
            std::sort(a.begin(),a.end(),order);std::sort(b.begin(),b.end(),order);
            check(a.size()==b.size() && !a.empty(),"PERF-02 gravity/drag paths preserve survivors");
            const auto close_value=[](double x,double y){return std::abs(x-y)<1e-10*std::max({1.,std::abs(x),std::abs(y)});};
            for(std::size_t i=0;i<std::min(a.size(),b.size());++i) {
                check(a[i].id==b[i].id && a[i].emitter_id==b[i].emitter_id &&
                    close_value(a[i].position.x,b[i].position.x) && close_value(a[i].position.y,b[i].position.y) && close_value(a[i].position.z,b[i].position.z) &&
                    close_value(a[i].velocity.x,b[i].velocity.x) && close_value(a[i].velocity.y,b[i].velocity.y) && close_value(a[i].velocity.z,b[i].velocity.z) &&
                    close_value(a[i].size_pixels,b[i].size_pixels) && close_value(a[i].opacity,b[i].opacity) &&
                    close_value(a[i].age_seconds,b[i].age_seconds) && close_value(a[i].lifetime_seconds,b[i].lifetime_seconds) &&
                    close_value(a[i].color.x,b[i].color.x) && close_value(a[i].color.y,b[i].color.y) && close_value(a[i].color.z,b[i].color.z) &&
                    close_value(a[i].rotation_degrees.x,b[i].rotation_degrees.x) && close_value(a[i].rotation_degrees.y,b[i].rotation_degrees.y) &&
                    close_value(a[i].rotation_degrees.z,b[i].rotation_degrees.z),
                    "PERF-02 actual fast/reference captures preserve identity, position, velocity and appearance");
            }
        }
        if(std::getenv("STARFIELD_TEST_PREP_BENCHMARK"))for(unsigned population:{30000u,50000u}) {
            emitter.values[3].one_d=population/2.;++metadata_epoch;
            auto prepared=uncaptured;
            check(sample_native_node_animation(&renderer_data,prepared,1920,1080)==0,"PERF-02 benchmark samples current native inputs");
            capture_native_temporal_metadata(&renderer_data,prepared,1);
            for(unsigned repeat=0;repeat<3;++repeat) {
                auto frame=prepared;PF_OutData frame_output{};
                const auto benchmark_start=std::chrono::steady_clock::now();
                check(capture_emitter_origin_history(&renderer_data,&frame_output,frame,1920,1080,core::NeverCancelled{})==0,
                    "PERF-02 benchmark capture succeeds");
                const auto trace=last_native_history_trace();
                const auto evaluated=core::evaluate_particle_graph(frame,force_time,core::NeverCancelled{}, {1080,1});
                const double capture_and_evaluate_ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-benchmark_start).count();
                check(evaluated.has_value() && evaluated.value().particles.size()==population,"PERF-02 benchmark preserves full population");
                std::printf("PERF02_PREP,%u,%u,%.6f,%s,%llu,%llu\n",population,repeat,trace.milliseconds,
                    trace.path==NativeHistoryPath::static_graph?"static":"temporal",
                    static_cast<unsigned long long>(trace.checkouts),static_cast<unsigned long long>(trace.node_queries));
                std::printf("PERF02_CAPTURE_EVAL,%u,%u,%.6f\n",population,repeat,capture_and_evaluate_ms);
            }
        }
        fixtures[3].values=saved_force;emitter.values[3].one_d=saved_rate;
    }
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
    auto ui_data=renderer_data;ui_data.num_params=kGraphSyncGuardId+1;
    ui_data.inter.register_ui=[](PF_ProgPtr,PF_CustomUIInfo* info)->PF_Err {
        check(info && info->events==(PF_CustomEFlag_COMP|PF_CustomEFlag_EFFECT) && !info->comp_ui_width && !info->comp_ui_height,
            "metadata UI registers only zero-sized composition events");return 0;
    };
    check(register_native_temporal_ui(&ui_data)==0,"UI metadata callback registration succeeds");
    PF_ArbitraryH ui_graph_handle{};
    check(create_graph_parameter(&ui_data,static_graph,&ui_graph_handle)==0,"UI metadata graph fixture created");
    PF_ParamDef ui_graph_param{};ui_graph_param.param_type=PF_Param_ARBITRARY_DATA;ui_graph_param.u.arb_d.value=ui_graph_handle;
    PF_ParamDef ui_guard{};ui_guard.param_type=PF_Param_FLOAT_SLIDER;
    std::array<PF_ParamDef*,kGraphSyncGuardId+1> ui_params{};ui_params[kGraphParameterId]=&ui_graph_param;ui_params[kGraphSyncGuardId]=&ui_guard;
    PF_EventExtra ui_event{};ui_event.e_type=PF_Event_DO_CLICK;ui_event.evt_out_flags=PF_EO_NEVER_UPDATE;
    initialize_native_temporal_ui();const auto event_suites=suite_requests;
    refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
    check(suite_requests==event_suites,"non-DRAW event performs no metadata SDK access");
    ui_event.e_type=PF_Event_DRAW;const auto ui_writes=sets;
      if(auto module=LoadLibraryW(L"artifacts\\model-ui-exclusion-tests\\paired\\StarfieldHost.aex")) {
          const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));
          const auto begin=std::bit_cast<std::uint64_t(*)() noexcept>(GetProcAddress(module,"SFLD_BeginUiExclusionV1"));
          const auto end=std::bit_cast<void(*)(std::uint64_t) noexcept>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));
          check(initialize && begin && end,"paired UI exclusion test module exports its private seam");
          if(initialize && begin && end) {
              initialize();const auto token=begin();check(token && ui_transaction_active(),"native callbacks observe resident Host mutation scope");
              const auto before=suite_requests;
              refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
              std::vector<PF_ParamDef*> source_params;for(auto& p:emitter.params)source_params.push_back(&p);
              PF_UserChangedParamExtra change{};change.param_index=4;PF_OutData changed{};
              check(sync_node_graph_parameter(&renderer_data,&changed,source_params.data(),&change)==0 && suite_requests==before,
                  "stale guard0 DRAW and source supervision perform no SDK access during native structural transaction");
              end(token);check(!ui_transaction_active(),"metadata resumes after the exact mutation token releases");
          }
          check(FreeLibrary(module)!=0,"test Host module releases after callback-fence regression");
      }
      for(auto guard:{1.0,2.0}) {
          ui_guard.u.fs_d.value=guard;
          refresh_native_temporal_ui(&ui_data,ui_params.data(),&ui_event,1);
          check(suite_requests==event_suites,"guarded DRAW performs no SDK access during effect parade mutation");
      }
      ui_guard.u.fs_d.value=0;
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
    const auto warmed_suites=aegp_suite_requests;const auto warmed_states=metadata_state_queries;
    for(unsigned repeat=0;repeat<8;++repeat) {
        const auto again=idle_bootstrap();
        check(!again.error && !again.refreshed && again.proofs>0,
            "first successful post-warm idle capture deduplicates every unchanged follow-up");
    }
    refresh_native_temporal_ui(&sequence_data,ui_params.data(),&ui_event,1);
    check(aegp_suite_requests==warmed_suites,"idle and DRAW share post-warm stamp without repeated alias evaluations");
    check(metadata_state_queries-warmed_states==9,"unchanged idle/DRAW needs one global state query each, no per-alias sweep");
    auto another_callback=sequence_data;another_callback.effect_ref=reinterpret_cast<PF_ProgPtr>(0x554433);
    NativeBootstrapRequest callback_request;refresh_native_temporal_idle(&another_callback,1,callback_request);
    check(callback_request.proofs>0 && !callback_request.refreshed && aegp_suite_requests==warmed_suites,
        "equivalent host dependency state deduplicates across nonpersistent callback references");
    const auto publication=native_control_proof_publication(&sequence_data);
    check(publication.id && native_control_proof_publication_available(publication.id),"successful capture has a bounded publication identity");
    remember_native_control_proofs(&sequence_data,{});
    check(!native_control_proof_publication_available(publication.id),"removed publication cannot keep a warm scheduling stamp valid");
    const auto recaptured=idle_bootstrap();
    check(recaptured.refreshed && recaptured.proofs>0 && !recaptured.error,
        "evicted proof publication rewarms even when authored dependency state is unchanged");
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
        const auto append_clears=expression_clear_count;
        {NativeBindingTransaction bindings(&renderer_data,701);check(bindings.install(transformed,nullptr,nullptr,nullptr,&animated_graph)==0,"new synthetic expressions install");bindings.accept();}
        const auto new_aliases=std::count_if(expressions.begin(),expressions.end(),[](const auto& text){
            return text.find(u"fx.param(\"Node UUID 7\").value === 5")!=std::u16string::npos;});
        check(expression_clear_count-append_clears==new_aliases,"pure addition compiles only new aliases and preserves existing dependencies");
        save_transform_expressions("artifacts/transform-none-animation-expressions.json");
        const auto matrix_alias=std::find_if(expressions.begin(),expressions.end(),[](const auto& text) {
            return text.find(u"// Starfield Transform affine entry 0\n")!=std::u16string::npos;
        });
        check(matrix_alias!=expressions.end(),"first Transform matrix alias exists");
        const auto matrix_stream=kNativeBindingFirstIndex+static_cast<A_long>(matrix_alias-expressions.begin());
        const auto reset_count=expression_clear_count;
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(transformed,nullptr,nullptr,nullptr,&transformed)==0,
                "unchanged identities retain their enabled bindings");bindings.accept();
        }
        check(expression_clear_count==reset_count,"ordinary control edits skip redundant expression recompilation");
        auto retired=transformed;
        retired.optional_records.erase(std::remove_if(retired.optional_records.begin(),retired.optional_records.end(),
            [](const auto& bytes){return bytes.size()>=2 && bytes[0]==std::byte{2} && bytes[1]==std::byte{0x80};}),retired.optional_records.end());
        const auto retirement_text_storage=std::make_unique<decltype(expressions)>(expressions);const auto& retirement_text=*retirement_text_storage;
        const auto retirement_enabled=expression_enabled;
        const auto retirement_base=main.values;
        // Execute actual numeric playback after EVERY host expression/value
        // mutation, retaining a graph checked out before publication. This
        // catches the disabled-expression sentinel gap missed by remove hooks.
        auto handoff_data=renderer_data;handoff_data.num_params=0;
        handoff_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex i,A_long t,A_long,A_u_long,PF_ParamDef* output)->PF_Err {
            *output={};output->param_type=PF_Param_FLOAT_SLIDER;output->u.fs_d.value=evaluated_binding(i,t);return 0;
        };
        handoff_data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err {return 0;};
        unsigned handoff_samples{};
        alias_mutation_hook=[&]{++handoff_samples;auto old=transformed;
            check(sample_native_node_animation(&handoff_data,old,1920,1080)==0,
                "in-flight published graph sees finite valid values after each alias mutation");};
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(retired,nullptr,nullptr,nullptr,&transformed)==0 &&
                expressions==retirement_text && expression_enabled==retirement_enabled,
                "removed node bindings remain live before graph publication");
            check(bindings.retire_after_publication()==0 && expressions[matrix_stream-kNativeBindingFirstIndex].empty() &&
                !expression_enabled[matrix_stream-kNativeBindingFirstIndex],"published graph permits retired alias detachment");
        }
        check(expressions==retirement_text && expression_enabled==retirement_enabled,
            "unaccepted alias retirement restores exact expression text and enabled states");
        for(A_long i=kNativeBindingFirstIndex;i<kTotalEffectParameterCount;++i)
            check(main.values[i].one_d==retirement_base[i].one_d,"rollback restores exact alias base after restoring expressions");
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(retired,nullptr,nullptr,nullptr,&transformed)==0,"retirement failure fixture prepared");
            fail_set=matrix_stream;
            check(bindings.retire_after_publication()==PF_Err_OUT_OF_MEMORY,"partial retirement rejects rather than accepting missing bindings");
        }
        check(expressions==retirement_text && expression_enabled==retirement_enabled,"partial retirement rollback restores original expressions");
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(retired,nullptr,nullptr,nullptr,&transformed)==0 && bindings.retire_after_publication()==0,
                "alias retirement can commit after publication");bindings.accept();
        }
        {
            NativeBindingTransaction bindings(&renderer_data,701);
            check(bindings.install(transformed,nullptr,nullptr,nullptr,&retired)==0,"restored node bindings are rebuilt after identity changes");bindings.accept();
        }
        check(expressions==retirement_text && expression_enabled==retirement_enabled,
            "retired alias spans can be reused without stale expression state");
        check(handoff_samples>0,"actual alias mutations exercised reentrant playback");alias_mutation_hook={};
        const auto before_expression_failure_storage=std::make_unique<decltype(expressions)>(expressions);const auto& before_expression_failure=*before_expression_failure_storage;
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
        {
            NativeBindingTransaction bindings(&renderer_data,701);const char* failed_stage=nullptr;
            unresolved_alias=matrix_stream;
            check(bindings.install(transformed,nullptr,&failed_stage)==PF_Err_BAD_CALLBACK_PARAM &&
                failed_stage && std::strcmp(failed_stage,"verify source binding resolved")==0,
                "missing source identity differs from a nonfinite source value");
        }
        unresolved_alias=-1;

        check(expressions[matrix_stream-kNativeBindingFirstIndex]==u"0" && live_refs==0 && acquisitions==0,
            "unresolved source restores expressions and releases host references");
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
        check(capture_motion_particles(&renderer_data,&opening_output,transformed,1920,1080,{},opening_times,core::NeverCancelled{},opening_frames)==0 &&
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
        const auto has_birth=[](const core::Graph& g) {
            for(const auto& n:g.nodes)if(n.type_key==kParticleNode)
                for(const auto& p:n.parameters)if(p.key==kParticleSeedShift || p.key==kParticleBirthChance)return true;
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
        for(unsigned version=1;version<=5;++version) {
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
                        const auto limit=kind==1?(version<3?442u:version==3?519u:version==4?526u:533u):533u;
                        if(get16(at)<=limit) {trimmed.insert(trimmed.end(),bytes.begin()+at,bytes.begin()+at+32);++kept;}
                    }
                    trimmed[start+18]=std::byte(kept&255);trimmed[start+19]=std::byte(kept>>8);
                }
                for(unsigned i=0;i<4;++i)trimmed[4+i]=std::byte((trimmed.size()>>(8*i))&255);
                bytes=std::move(trimmed);
            }
            NativeAnimationPlan old_plan(old,1920,1080,1);
            check(old_plan.valid() && sample_native_node_animation(&renderer_data,old,1920,1080)==0 && !has_cloud(old) && !has_birth(old),
                  "historical binding versions retain legacy Cloud/birth semantics and their original field limits");
        }
        auto bad_old=legacy;
        for(auto& r:bad_old.optional_records)if(r.size()>12 && r[0]==std::byte{2} && r[1]==std::byte{0x80})r[2]=std::byte{4};
        check(!NativeAnimationPlan(bad_old,1920,1080,1).valid(),"v4 rejects an appended activation field without migration");
        for(auto& r:bad_old.optional_records)if(r.size()>12 && r[0]==std::byte{2} && r[1]==std::byte{0x80})r[2]=std::byte{5};
        check(!NativeAnimationPlan(bad_old,1920,1080,1).valid(),"v5 rejects appended birth fields without migration");
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
        check(!has_birth(saved_graph()),"Cloud authoring does not activate appended birth defaults");
        NativeAnimationPlan legacy_birth(saved_graph(),1920,1080,1);
        check(legacy_birth.constant_birth_chance(particle_id)==100,"missing legacy chance is intrinsically constant100");
        auto birth=edit(1,layout::seed_shift,-19);
        birth.additional_fields[birth.additional_count++]={layout::birth_enabled,node_sync::ValueKind::scalar,{1,0,0,0}};
        check(node_sync::valid_edit(birth) && direct_edit(birth)==0 && birth.accepted,"signed seed and birth activation commit atomically");
        auto born=saved_graph();
        check(std::get<std::int32_t>(parameter(born,kParticleNode,kParticleSeedShift))==-19 &&
              std::get<double>(parameter(born,kParticleNode,kParticleBirthChance))==100,"native birth graph preserves int32 seed and default100 chance");
        particle.values[layout::birth_enabled].one_d=1;particle.values[layout::seed_shift].one_d=-19;
        forged=birth;forged.parameter_index=layout::size;
        check(!node_sync::valid_edit(forged),"unrelated controls cannot inject birth activation");
        forged=birth;forged.additional_fields[0].value[0]=0;
        check(!node_sync::valid_edit(forged),"birth activation accepts only constant1");
        for(double value:{-2147483648.0,2147483647.0}) {
            auto edge=edit(1,layout::seed_shift,value);
            check(node_sync::valid_edit(edge) && direct_edit(edge)==0 &&
                  std::get<std::int32_t>(parameter(saved_graph(),kParticleNode,kParticleSeedShift))==value,"exact signed32 seed bounds round-trip through native graph");
        }
        for(double value:{-2147483649.0,2147483648.0,0.5}) {
            auto invalid=edit(1,layout::seed_shift,value);check(!node_sync::valid_edit(invalid),"invalid signed seed rejects before native mutation");
        }
        for(double value:{0.0,25.5,100.0}) {
            auto chance=edit(1,layout::birth_chance,value);
            check(direct_edit(chance)==0 && std::get<double>(parameter(saved_graph(),kParticleNode,kParticleBirthChance))==value,"native birth chance endpoints and fraction round-trip");
        }
        for(double value:{-1.0,101.0}) {
            auto invalid=edit(1,layout::birth_chance,value);check(!node_sync::valid_edit(invalid),"invalid chance rejects before native mutation");
        }
        particle.values[layout::birth_chance].one_d=25;
        auto sampling=edit(1,layout::seed_shift,-19);check(direct_edit(sampling)==0,"birth aliases prepare for time sampling");
        born=saved_graph();birth_animation_test=true;renderer_data.current_time=6;
        check(sample_native_node_animation(&renderer_data,born,1920,1080)==0 &&
              std::get<std::int32_t>(parameter(born,kParticleNode,kParticleSeedShift))==-19 &&
              std::get<double>(parameter(born,kParticleNode,kParticleBirthChance))==31,"birth aliases sample animated chance and rounded integer seed at requested time");
        birth_animation_test=false;renderer_data.current_time=0;
        particle.values[layout::birth_chance].one_d=0;
        auto zero_edit=edit(1,layout::birth_chance,0);check(direct_edit(zero_edit)==0,"zero chance graph commits");
        auto zero=saved_graph();NativeAnimationPlan unproven_zero(zero,1920,1080,1);
        check(!unproven_zero.constant_birth_chance(particle_id),"current zero value alone cannot certify all-time zero chance");
        temporal_metadata_enabled=true;metadata_life_expression=true;++metadata_epoch;
        capture_native_temporal_metadata(&renderer_data,zero,1);
        NativeAnimationPlan proved_zero(zero,1920,1080,1);proved_zero.prepare_constants(&renderer_data);
        check(!proved_zero.constant_node(particle_id) && proved_zero.constant_birth_chance(particle_id)==0,
              "constant chance alone is certified even when Life prevents whole-node hoisting");
        metadata_chance_expression=true;++metadata_epoch;capture_native_temporal_metadata(&renderer_data,zero,1);
        NativeAnimationPlan expression_zero(zero,1920,1080,1);expression_zero.prepare_constants(&renderer_data);
        check(!expression_zero.constant_birth_chance(particle_id),"expression chance at zero cannot certify a skipped source stream");
        metadata_chance_expression=metadata_life_expression=temporal_metadata_enabled=false;
        remember_native_control_proofs(&renderer_data,{});
        check(live_refs==0 && acquisitions==0,"Cloud conversion and compatibility sampling release all host references");
    }
    {
        // Exercise actual installation in a mixed Model/Transform parade. The
        // separate JS regression runs these generated expressions, rather than
        // hand-written imitations of the UUID lookup or layout gate.
        using namespace core::graph_keys;
        model_present=transform_present=true;
        fixtures[5].name="ADBE Null Control Fixture";
        auto& model=fixtures[6];model.name="org.starfieldfx.node.model";
        model.values.resize(records::parameter_count(records::Kind::model));
        model.params.resize(model.values.size());
        for(auto& p:model.params)p.param_type=PF_Param_FLOAT_SLIDER;
        uuid(6,records::uuid_first_index(records::Kind::model),6);
        model.values[records::model_layout::source].one_d=1;
        for(A_long i=records::model_layout::scale;i<records::model_layout::scale+3;++i)model.values[i].one_d=100;
        model.values[records::model_layout::center].one_d=1;
        model.values[records::model_layout::normalize].one_d=1;
        fixtures[4].values[1].layer_id=0;
        connection(3,records::Kind::force,5,13);
        connection(4,records::Kind::transform,255,14);
        connection(6,records::Kind::model,2,15);
        auto change=edit(5,records::model_layout::origin,37);
        check(direct_edit(change)==0 && change.accepted,"Model edit installs bindings with a Transform in the same parade");
        check(std::get<core::Vec3>(parameter(saved_graph(),kModelNode,kModelOrigin)).x==37,
              "Model edit retains its own source parameter and UUID");
        std::ofstream output("artifacts/model-transform-animation-expressions.json");
        output<<"[";bool first=true;unsigned models=0,transforms=0;
        for(const auto& expression:expressions) {
            const bool is_model=expression.find(u"fx.param(\"Node UUID 7\").value === 6")!=std::u16string::npos;
            const bool is_transform=expression.find(u"fx.param(\"Node UUID 7\").value === 5")!=std::u16string::npos;
            if(!is_model && !is_transform)continue;
            models+=is_model;transforms+=is_transform;
            if(!first)output<<",";first=false;output<<"\"";
            for(auto c:expression) {
                if(c==u'\n')output<<"\\n";
                else {if(c==u'\\' || c==u'\"')output<<"\\";output<<static_cast<char>(c);}
            }
            output<<"\"";
        }
        output<<"]";
        check(bool(output) && models==14 && transforms>12,"actual mixed Model and Transform expressions recorded");
        check(live_refs==0 && acquisitions==0,"mixed Model/Transform edit releases host references");
        const auto old_expressions_storage=std::make_unique<decltype(expressions)>(expressions);const auto& old_expressions=*old_expressions_storage;
        fixtures[1].display_name=fixtures[2].display_name=u"Ambiguous author name";
        auto ambiguous=edit(0,1,fixtures[1].values[1].one_d);
        check(direct_edit(ambiguous)!=0 && !ambiguous.accepted,"duplicate live effect names reject the binding transaction");
        check(expressions==old_expressions,"ambiguous source rejection preserves all previous expressions");
        fixtures[2].display_name.clear();
        fixtures[1].display_name=u"Emitter \"路径\\\n\u2028\u2029\U0001f680";
        auto escaped=edit(0,1,fixtures[1].values[1].one_d);
        check(direct_edit(escaped)==0 && escaped.accepted,"literal binding accepts quotes/newlines/Unicode in custom names");
        std::ofstream escaped_output("artifacts/native-escaped-source-expression.json");
        auto json_string=[&](const std::u16string& text) {
            constexpr char hex[]="0123456789abcdef";escaped_output<<'"';
            for(auto unit:text){escaped_output<<"\\u";for(int shift=12;shift>=0;shift-=4)escaped_output<<hex[(unit>>shift)&15];}
            escaped_output<<'"';
        };
        escaped_output<<"{\"name\":";json_string(fixtures[1].display_name);escaped_output<<",\"expression\":";
        json_string(expressions[0]);escaped_output<<"}";escaped_output.close();
        check(bool(escaped_output),"actual escaped-name binding saved for independent JS execution");
        fixtures[1].display_name.clear();
        auto restore_name=edit(0,1,fixtures[1].values[1].one_d);
        check(direct_edit(restore_name)==0 && restore_name.accepted,"author rename rebinds literal source");
        check(live_refs==0 && acquisitions==0,"literal inventory and rejected names release all host references");
        model_present=transform_present=false;
    }
    {
        // Actual compiler, aliases, arbitrary serializer and stream publisher.
        // No USER_CHANGED_PARAM is dispatched for the script request write.
        panel_publish=true;
        model_present=transform_present=true;
        main.values[kGraphSyncGuardId].one_d=1;main.values[kNodeEffectsReadyId].one_d=1;
        main.values[kGraphEditCommitId].one_d=987654;
        PF_OutData output{};
        const auto initial_commit_error=commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987654,701,&output);
        if(initial_commit_error)std::printf("Explicit command diagnostic: %d %s\n",initial_commit_error,output.return_msg);
        check(initial_commit_error==0 &&
            main.values[kGraphEditReceiptId].one_d==987654,"explicit command publishes saved receipt without supervised callback");
        auto before_graph=handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes;
        const auto before_revision=main.values[kGraphRevisionId].one_d;
        const auto before_expressions_storage=std::make_unique<decltype(expressions)>(expressions);const auto& before_expressions=*before_expressions_storage;
        main.values[kGraphEditCommitId].one_d=987653;
        auto empty_context=renderer_data;empty_context.width=empty_context.height=empty_context.time_scale=0;
        empty_context.effect_ref=nullptr; // explicit borrowed owner replaces PF lookup
        empty_context.pixel_aspect_ratio={0,0};
        check(commit_panel_graph(&empty_context,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987653,701,&output)==0 &&
            main.values[kGraphEditReceiptId].one_d==987653,"generic callback recovers missing geometry and time from its owned layer");
        // Restore the first publication baseline for the failure comparison.
        main.values[kGraphEditReceiptId].one_d=987654;main.values[kGraphRevisionId].one_d=before_revision;
        main.values[kGraphEditCommitId].one_d=987655;fail_set=kGraphEditReceiptId;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987655,701,&output)!=0,
            "receipt partial-write failure rejects explicit publication");
        check(main.values[kGraphEditReceiptId].one_d==987654 && main.values[kGraphRevisionId].one_d==before_revision &&
            handles.at(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH))->bytes==before_graph && expressions==before_expressions,
            "failed explicit receipt restores exact graph, revision, receipt and aliases");
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987655,701,&output)==0 &&
            main.values[kGraphEditReceiptId].one_d==987655 && main.values[kGraphRevisionId].one_d==before_revision+1,
            "failed request can retry without stale callback state");
        const auto before_writes=sets;main.values[kGraphSyncGuardId].one_d=2;
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987655,701,&output)!=0 && sets==before_writes,
            "transaction backup guard cannot publish a graph command");
        main.values[kGraphSyncGuardId].one_d=1;main.values[kGraphEditCommitId].one_d=987656;panel_output_only=true;
        unsigned publication_samples{};
        alias_mutation_hook=[&]{++publication_samples;auto old=saved_graph();
            auto sampled_data=renderer_data;sampled_data.num_params=0;
            sampled_data.inter.checkout_param=[](PF_ProgPtr,PF_ParamIndex i,A_long t,A_long,A_u_long,PF_ParamDef* output)->PF_Err {
                *output={};output->param_type=PF_Param_FLOAT_SLIDER;output->u.fs_d.value=evaluated_binding(i,t);return 0;};
            sampled_data.inter.checkin_param=[](PF_ProgPtr,PF_ParamDef*)->PF_Err{return 0;};
            check(sample_native_node_animation(&sampled_data,old,1920,1080)==0,"saved graph remains sampleable across explicit deletion publication");};
        check(commit_panel_graph(&renderer_data,effect_ref(0),reinterpret_cast<AEGP_LayerH>(1),987656,701,&output)==0 &&
            main.values[kGraphEditReceiptId].one_d==987656 && saved_graph().nodes.size()==1,
            "explicit deletion of every node publishes Output-only without recreating defaults");
        check(publication_samples>0,"explicit publisher exercised render at graph and alias writes");alias_mutation_hook={};
        panel_output_only=false;main.values[kGraphSyncGuardId].one_d=0;panel_publish=false;
        model_present=transform_present=false;
        check(live_refs==0 && acquisitions==0,"explicit graph command releases borrowed stream values and suites");
    }
    seven_kind_publication_tests();
    dispose(reinterpret_cast<PF_Handle>(main.values[kGraphParameterId].arbH));
    // PARAMS_SETUP creates its own default arbitrary value.
    for (auto& param : main.params) if (param.param_type == PF_Param_ARBITRARY_DATA) {
        if (handles.contains(param.u.arb_d.value)) dispose(param.u.arb_d.value);
        if (handles.contains(param.u.arb_d.dephault)) dispose(param.u.arb_d.dephault);
    }
    check(handles.empty(), "all retained graph handles released");
    check(FreeLibrary(source_scope_module)!=0,"source callback fixture releases its paired resident exclusion");
    std::printf("Native sync: %d checks, %d failures\n", checks, failures);
    failures += run_camera_capture_tests(); return failures ? 1 : 0;
}
