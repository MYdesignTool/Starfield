// Reuse the actual complete-backup fake SDK, extending its mutation callbacks
// with real checkout_render_graph. Its old contract tests remain independently run.
#define main backup_contract_fixture_main
#include "effect_graph_backup_tests.cpp"
#undef main
#include "GraphParameter.hpp"
#include "MotionBlur.hpp"
#include "NativeNodeGraph.hpp"
#include "starfield/core/GraphEvaluation.hpp"
#include "starfield/core/SequenceCodec.hpp"
#include <bit>
#include <cmath>
#include <fstream>
#include <functional>
#include <optional>

namespace {
std::map<PF_Handle,std::vector<std::byte>> render_handles;
std::map<A_long,std::pair<UUID,double>> alias_sources;
std::vector<std::pair<int,UUID>> source_nodes;
Ptr actor;
unsigned frames{},canceled{},normal{},disabled_frames{},alias_reads{},graph_reads{},param_checkouts{},param_checkins{},render_errors{};
unsigned guarded_bad_bindings{},backup_bad_bindings{};
std::vector<std::uint8_t> full_graph,empty_graph;
bool render_enabled{};
bool stale_expression{},split_receipt{};
std::uint32_t bytes_checksum(const std::vector<std::uint8_t>& bytes) {
    std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[24+i])<<(8u*i);return value;
}
PF_Handle allocate_render(A_u_longlong size) {
    auto h=reinterpret_cast<PF_Handle>(new int);render_handles[h].resize(static_cast<std::size_t>(size));return h;
}
void* lock_render(PF_Handle h){return render_handles.at(h).data();}
void unlock_render(PF_Handle){}
void dispose_render(PF_Handle h){CHECK(render_handles.erase(h)==1);delete reinterpret_cast<int*>(h);}
A_u_longlong size_render(PF_Handle h){return render_handles.at(h).size();}
bool alive(const UUID& id) {
    return std::any_of(parade.begin(),parade.end(),[&](const auto& p){return p->kind>=0 && uuid(*p)==id;});
}
PF_Err render_checkout(PF_ProgPtr,PF_ParamIndex index,A_long,A_long,A_u_long,PF_ParamDef* out) {
    ++param_checkouts;*out={};
    if(index==kControlSourceId){out->param_type=PF_Param_POPUP;out->u.pd.value=kNodeControlSource;}
    else if(index==kGraphSyncGuardId){out->param_type=PF_Param_FLOAT_SLIDER;out->u.fs_d.value=actor->numbers.at(index);}
    else if(index==kGraphChecksumHighId || index==kGraphChecksumLowId) {
        out->param_type=PF_Param_FLOAT_SLIDER;
        const auto checksum=bytes_checksum(actor->arbitrary.at(kGraphParameterId))^(split_receipt?1u:0u);
        out->u.fs_d.value=index==kGraphChecksumHighId?checksum>>16u:checksum&65535u;
    }
    else if(index==kGraphParameterId) {
        ++graph_reads;const auto& bytes=actor->arbitrary.at(index);auto h=allocate_render(bytes.size());
        std::memcpy(lock_render(h),bytes.data(),bytes.size());out->param_type=PF_Param_ARBITRARY_DATA;out->u.arb_d.value=h;
    } else if(index>=kNativeBindingFirstIndex && index<kNativeBindingFirstIndex+kNativeBindingCapacity) {
        ++alias_reads;const auto& source=alias_sources.at(index);out->param_type=PF_Param_FLOAT_SLIDER;
        out->u.fs_d.value=stale_expression?kNativeBindingStaleGraph:alive(source.first)?source.second:kNativeBindingUnavailable;
    } else if(index==kTimeRemapEnabledId || index==kPreviewEnabledId){out->param_type=PF_Param_CHECKBOX;}
    else if(index==kTimeSamplingHzId || index==kAccelerationId){out->param_type=PF_Param_POPUP;out->u.pd.value=1;}
    else {
        out->param_type=PF_Param_FLOAT_SLIDER;
        if(index==kPreviewChanceId)out->u.fs_d.value=100;
        for(std::size_t i=0;i<kMotionParameterIds.size();++i)if(index==kMotionParameterIds[i]) {
            if(motion_popup(i)){out->param_type=PF_Param_POPUP;out->u.pd.value=static_cast<A_long>(kMotionDefaults[i])+1;}
            else out->u.fs_d.value=kMotionDefaults[i];
        }
    }
    return 0;
}
PF_Err render_checkin(PF_ProgPtr,PF_ParamDef* value) {
    ++param_checkins;if(value->param_type==PF_Param_ARBITRARY_DATA)dispose_render(value->u.arb_d.value);return 0;
}
void render_actor() {
    ++frames;const auto g=actor->numbers.at(kGraphSyncGuardId);
    if(!(actor->flags&AEGP_EffectFlags_ACTIVE))++disabled_frames;
    PF_UtilCallbacks util{};util.host_new_handle=allocate_render;util.host_lock_handle=lock_render;
    util.host_unlock_handle=unlock_render;util.host_dispose_handle=dispose_render;util.host_get_handle_size=size_render;
    PF_InData data{};data.utils=&util;data.width=1920;data.height=1080;data.time_scale=24;data.pixel_aspect_ratio={1,1};
    data.effect_ref=reinterpret_cast<PF_ProgPtr>(actor.get());data.inter.checkout_param=render_checkout;data.inter.checkin_param=render_checkin;
    PF_OutData out{};std::shared_ptr<const starfield::core::Graph> graph;
    const auto aliases_before=alias_reads,graphs_before=graph_reads;
    const auto error=checkout_render_graph(&data,&out,graph);
    if((g==1 || g==transaction_backup_guard) && error==PF_Err_BAD_CALLBACK_PARAM && out.return_msg[0]) {
        ++guarded_bad_bindings;if(g==transaction_backup_guard)++backup_bad_bindings;
        if(guarded_bad_bindings<=3)std::cerr<<"guarded binding failure guard="<<g<<" "<<out.return_msg<<'\n';
    }
    if(g==1 || g==transaction_backup_guard) {
        if(error!=PF_Interrupt_CANCEL || graph || out.return_msg[0] || alias_reads!=aliases_before || graph_reads!=graphs_before) {
            ++render_errors;if(render_errors<5)std::cerr<<"guard="<<g<<" render="<<error<<" "<<out.return_msg<<'\n';
        } else ++canceled;
    } else if(error || !graph) {
        ++render_errors;if(render_errors<5)std::cerr<<"normal render="<<error<<" "<<out.return_msg<<'\n';
    } else ++normal;
    CHECK(param_checkouts==param_checkins);CHECK(render_handles.empty());
}
void render_parade() {
    if(!render_enabled)return;
    const auto current=parade;
    for(const auto& p:current)if(p->kind==-1){actor=p;render_actor();}
    actor.reset();
}
void install_render_hooks() {
    const auto write=streams.AEGP_SetStreamValue;
    // SDK callbacks are function pointers; keep originals in static bindings.
    static auto scalar_write=write;
    streams.AEGP_SetStreamValue=[](AEGP_PluginID p,AEGP_StreamRefH h,AEGP_StreamValue2* v)->A_Err {
        const auto e=scalar_write(p,h,v);render_parade();return e;};
    static auto duplicate=effects.AEGP_DuplicateEffect;
    effects.AEGP_DuplicateEffect=[](AEGP_EffectRefH h,AEGP_EffectRefH* out)->A_Err{const auto e=duplicate(h,out);render_parade();return e;};
    static auto erase=effects.AEGP_DeleteLayerEffect;
    effects.AEGP_DeleteLayerEffect=[](AEGP_EffectRefH h)->A_Err{const auto e=erase(h);render_parade();return e;};
    static auto flags=effects.AEGP_SetEffectFlags;
    effects.AEGP_SetEffectFlags=[](AEGP_EffectRefH h,AEGP_EffectFlags m,AEGP_EffectFlags f)->A_Err{const auto e=flags(h,m,f);render_parade();return e;};
    static auto rename=dynamic.AEGP_SetStreamName;
    dynamic.AEGP_SetStreamName=[](AEGP_StreamRefH h,const A_UTF16Char* n)->A_Err{const auto e=rename(h,n);render_parade();return e;};
    static auto reorder=effects.AEGP_ReorderEffect;
    effects.AEGP_ReorderEffect=[](AEGP_EffectRefH h,A_long i)->A_Err{const auto e=reorder(h,i);render_parade();return e;};
}
void load_graph(const char* path) {
    std::ifstream input(path,std::ios::binary);CHECK(bool(input));std::vector<char> bytes{std::istreambuf_iterator<char>(input),{}};
    full_graph.assign(bytes.begin(),bytes.end());
    const auto graph=starfield::core::deserialize_graph({reinterpret_cast<const std::byte*>(bytes.data()),bytes.size()},starfield::core::particle_node_registry());
    CHECK(graph.has_value());CHECK(graph.value().nodes.size()==8);
    const auto read=[](const auto& b,std::size_t at,unsigned count){std::uint64_t v{};CHECK(at+count<=b.size());
        for(unsigned i=0;i<count;++i)v|=std::uint64_t(std::to_integer<unsigned>(b[at+i]))<<(i*8);return v;};
    for(const auto& record:graph.value().optional_records)if(record.size()>=12 && read(record,0,2)==0x8002) {
        std::size_t at=12;CHECK(read(record,8,4)==7);
        for(unsigned n=0;n<7;++n) {
            UUID id{};for(auto& byte:id)byte=static_cast<std::uint8_t>(read(record,at++,1));
            const auto kind=static_cast<int>(read(record,at,2));const auto fields=static_cast<unsigned>(read(record,at+2,2));at+=4;
            source_nodes.push_back({kind,id});
            for(unsigned f=0;f<fields;++f) {
                const auto type=read(record,at+2,2),slot=read(record,at+4,2);
                if(slot!=65535)for(unsigned c=0;c<(type==0?1:type==1?2:3);++c) {
                    const auto value=std::bit_cast<double>(read(record,at+8+c*8,8));
                    CHECK(std::isfinite(value));alias_sources.emplace(kNativeBindingFirstIndex+static_cast<A_long>(slot+c),std::pair{id,value});
                }at+=32;
            }
        }CHECK(at==record.size());
    }
    CHECK(source_nodes.size()==7 && !alias_sources.empty());
    auto empty=graph.value();empty.nodes.erase(std::remove_if(empty.nodes.begin(),empty.nodes.end(),[](const auto& n){return n.type_key!=starfield::core::graph_keys::kOutputNode;}),empty.nodes.end());
    empty.edges.clear();empty.optional_records.clear();
    const auto encoded=starfield::core::serialize_graph(empty,starfield::core::particle_node_registry());CHECK(encoded.has_value());
    for(auto b:encoded.value())empty_graph.push_back(static_cast<std::uint8_t>(b));
}
void begin_case() {
    render_enabled=false;reset();parade.clear();
    auto main=make(-1,1);main->arbitrary[kGraphParameterId]=full_graph;parade.push_back(main);
    for(const auto& [kind,id]:source_nodes){auto node=make(kind,static_cast<unsigned>(parade.size()+1));uuid(*node,id);parade.push_back(node);}
    render_enabled=true;render_parade();
}
void remove_authors(const std::optional<UUID>& only={}) {
    auto current=parade;
    for(const auto& p:current)if(p->kind>=0 && p->numbers.at(guard(p->kind))!=transaction_backup_guard && (!only || uuid(*p)==*only)) {
        auto h=effect_ref(p);CHECK(effects.AEGP_DeleteLayerEffect(h)==0);effects.AEGP_DisposeEffect(h);
    }
}
}
int main(int argc,char** argv) {
    CHECK(argc==2);initialize();
    // The reused fixture also understands Material/Motion match names.
    static auto original_match=effects.AEGP_GetEffectMatchName;
    effects.AEGP_GetEffectMatchName=[](AEGP_InstalledEffectKey key,A_char* out)->A_Err {
        if(static_cast<int>(key)-3==6){std::strcpy(out,"org.starfieldfx.node.material");return 0;}
        if(static_cast<int>(key)-3==7){std::strcpy(out,"org.starfieldfx.node.motion");return 0;}
        return original_match(key,out);};
    load_graph(argv[1]);install_render_hooks();
    // Real render entry, independently mixed PF graph/receipt and expression
    // versions. These are protocol fixtures, not a claim to emulate AE caches.
    begin_case();render_enabled=false;actor=parade.front();
    PF_UtilCallbacks stamp_utils{};stamp_utils.host_new_handle=allocate_render;stamp_utils.host_lock_handle=lock_render;
    stamp_utils.host_unlock_handle=unlock_render;stamp_utils.host_dispose_handle=dispose_render;stamp_utils.host_get_handle_size=size_render;
    PF_InData stamp_data{};stamp_data.utils=&stamp_utils;stamp_data.width=1920;stamp_data.height=1080;stamp_data.time_scale=24;
    stamp_data.inter.checkout_param=render_checkout;stamp_data.inter.checkin_param=render_checkin;
    PF_OutData stamp_out{};std::shared_ptr<const starfield::core::Graph> stamp_graph;
    auto stamp_aliases=alias_reads;split_receipt=true;
    CHECK(checkout_render_graph(&stamp_data,&stamp_out,stamp_graph)==PF_Interrupt_CANCEL && !stamp_graph && !stamp_out.return_msg[0] && alias_reads==stamp_aliases);
    split_receipt=false;stale_expression=true;
    CHECK(checkout_render_graph(&stamp_data,&stamp_out,stamp_graph)==PF_Interrupt_CANCEL && !stamp_graph && !stamp_out.return_msg[0]);
    stale_expression=false;
    CHECK(checkout_render_graph(&stamp_data,&stamp_out,stamp_graph)==0 && stamp_graph && !stamp_out.return_msg[0]);
    CHECK(param_checkouts==param_checkins && render_handles.empty());actor.reset();
    for(bool appended:{false,true}) {
        begin_case();duplicate_at_end=appended;const auto before=snapshot();
        {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);
            const auto original=parade.front();original->arbitrary[kGraphParameterId]=empty_graph;render_parade();
            remove_authors();CHECK(backup.discard()==0);CHECK(original->numbers.at(kGraphSyncGuardId)==0);}
        CHECK(parade.size()==1);render_parade();no_owned();
        for(const auto& [kind,id]:source_nodes) {
            begin_case();duplicate_at_end=appended;
            {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);
                parade.front()->arbitrary[kGraphParameterId]=empty_graph;render_parade();remove_authors(id);
                CHECK(backup.discard()==0);}
            CHECK(parade.size()==source_nodes.size());render_parade();no_owned();
        }
        for(unsigned count=0;count<=source_nodes.size();++count) {
            begin_case();duplicate_at_end=appended;const auto old=snapshot();
            {EffectGraphBackup backup(&basic,7,layer());CHECK(backup.prepare(operation)==0);
                for(unsigned n=0;n<count;++n)remove_authors(source_nodes[n].second);
                CHECK(backup.restore()==0);equals(old);render_parade();}
            no_owned();
        }
    }
    // Normal guard0 must still reject genuinely missing sources.
    begin_case();render_enabled=false;auto original=parade.front();remove_authors(source_nodes.front().second);actor=original;
    const auto expected_before=render_errors;render_actor();CHECK(render_errors==expected_before+1);--render_errors;actor.reset();
    CHECK(frames>0 && normal>0 && disabled_frames>0);if(!render_errors)CHECK(canceled>0);CHECK(render_handles.empty());no_owned();
    std::cout<<"Deletion render lifecycle: "<<checks<<" checks, "<<frames<<" frames, "<<canceled<<" canceled, "<<normal
        <<" normal, "<<disabled_frames<<" disabled-backup callbacks, "<<render_errors<<" unexpected errors\n";
    std::cout<<"Guarded bad bindings: "<<guarded_bad_bindings<<", backup bad bindings: "<<backup_bad_bindings<<'\n';
    return render_errors?1:0;
}
