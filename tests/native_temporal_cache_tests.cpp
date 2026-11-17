#include "AEConfig.h"
#include "NativeTemporalCache.hpp"
#include "Parameters.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>
#include <new>
#include <thread>
using namespace starfield;
namespace {
std::atomic<bool> count_allocations{};
std::atomic<std::uint64_t> allocations{},allocated_bytes{};
std::atomic<long long> fail_after{-1};
}
_Use_decl_annotations_ void* operator new(std::size_t size) {
    const auto remaining=fail_after.load();
    if(remaining>=0 && fail_after.fetch_sub(1)==0)throw std::bad_alloc{};
    if(count_allocations){++allocations;allocated_bytes+=size;}
    if(auto* p=std::malloc(size?size:1))return p;throw std::bad_alloc{};
}
_Use_decl_annotations_ void* operator new[](std::size_t size){return ::operator new(size);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
namespace {
using namespace adapter;
unsigned checks{};
std::atomic<unsigned> held_suites{};
bool suite_available=true;
PF_ParamUtilsSuite3 param_utils{};
struct Context {
    std::map<A_long,std::uint64_t> versions;
    unsigned states{},compares{};
    A_long failed_stream{-999};bool fail_compare{},fail_whole_compare{},throw_state{};
    std::function<void()> on_state;
    bool trace{};std::vector<A_long> events;
};
void check(bool ok,const char* message){++checks;if(!ok)throw message;}
core::NodeId node(unsigned n){core::NodeId id;id.value.bytes[12]=static_cast<std::uint8_t>(n>>24);id.value.bytes[13]=static_cast<std::uint8_t>(n>>16);id.value.bytes[14]=static_cast<std::uint8_t>(n>>8);id.value.bytes[15]=static_cast<std::uint8_t>(n);return id;}
PF_State state(std::uint64_t version,A_long stream){PF_State result{};static_assert(sizeof(result)>=sizeof(version));const auto token=(version<<32)|std::uint32_t(stream);std::memcpy(&result,&token,sizeof(token));return result;}
SPErr acquire(const char* name,int32,const void** out){*out=nullptr;if(suite_available && !std::strcmp(name,kPFParamUtilsSuite)){*out=&param_utils;++held_suites;return 0;}return kSPBadParameterError;}
SPErr release(const char*,int32){--held_suites;return 0;}
PF_InData data(Context& context){static SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;PF_InData d{};d.pica_basicP=&basic;d.effect_ref=reinterpret_cast<PF_ProgPtr>(&context);return d;}
PF_InData publisher(unsigned id){PF_InData d{};d.effect_ref=reinterpret_cast<PF_ProgPtr>(std::uintptr_t(id)+1);return d;}
NativeControlProof proof(unsigned id,A_long stream,std::uint64_t version,double marker) {
    NativeControlProof p{node(id),stream,state(version,stream),true,{},marker};core::EmissionRateProfile rate;rate.constant=marker;p.rate=rate;return p;
}
void clear_registry(){for(unsigned n=0;n<32;++n){auto p=publisher(0x100000+n);remember_native_control_proofs(&p,{});}}
void publication_failure() {
    // Must run first, with an empty publication registry. An allocation failure
    // publishing metadata must not discard a live, potentially expensive prefix.
    Context c;c.versions={{98,1}};auto d=data(c),p=publisher(600);
    auto retained=native_emission_timeline(&d,node(0x4242),98,30);check(bool(retained),"startup prefix available before publication");
    core::EmissionRateProfile rate;rate.constant=10;check(retained->configure(30,&rate).has_value(),"startup prefix configured");
    std::vector<NativeControlProof> values{proof(0x4242,98,1,10)};
    fail_after=0;remember_native_control_proofs(&p,std::move(values));fail_after=-1;
    check(!native_control_proof_publication(&p).id,"failed publication has no scheduling token");
    auto parallel=native_emission_timeline(&d,node(0x4242),98,30);
    check(parallel && !parallel->initialized() && retained->initialized(),
        "publication allocation failure preserves owned history and does not expose an uncommitted frame");
}
void behavior() {
    clear_registry();Context c;c.versions={{98,1},{99,1},{100,1}};auto d=data(c),a=publisher(1),b=publisher(2);
    remember_native_control_proofs(&a,{proof(1,98,1,3),proof(1,99,1,4),proof(2,100,1,5)});
    const auto old=native_control_proof_publication(&a);check(old.count==3 && old.id,"publication identity");
    remember_native_control_proofs(&b,{proof(1,98,2,8),proof(1,99,1,9)});
    c.trace=true;auto selected=validated_native_control_proofs(&d);c.trace=false;
    check(selected.size()==3 && selected[0].stream==99 && selected[0].life_bound==9 && selected[1].stream==98 && selected[1].life_bound==3 && selected[2].stream==100,
        "newest valid proof wins, older valid proof survives a newer mismatch, ordered result retained");
    check(c.states==3 && c.compares==4 && !held_suites,"one state read per stream, mandatory comparisons and balanced suites");
    check(c.events==std::vector<A_long>{98,-98,99,-99,-98,100,-100},"SDK state/comparison order matches full newest-first traversal including old fallback");
    const std::array wanted{node(2),node(2)};c.states=c.compares=0;
    selected=validated_native_control_proofs(&d,wanted);
    check(selected.size()==1 && selected[0].node==node(2) && c.states==1 && c.compares==1,"scope filter and duplicate requested UUIDs");
    c.failed_stream=99;selected=validated_native_control_proofs(&d);check(selected.size()==2,"state failure omits only that stream");c.failed_stream=-999;
    c.fail_compare=true;check(validated_native_control_proofs(&d).empty(),"comparison failure cannot certify");c.fail_compare=false;
    Context copy;copy.versions={{98,7},{99,7},{100,7}};auto copied=data(copy);check(validated_native_control_proofs(&copied).empty(),"copied-layer UUIDs never bypass host state");
    suite_available=false;check(validated_native_control_proofs(&d).empty(),"unavailable suite is safe miss");suite_available=true;
    c.throw_state=true;check(validated_native_control_proofs(&d).empty() && !held_suites,"bad_alloc during SDK lookup releases suite and returns miss");c.throw_state=false;
    // Reentrant replacement during a host callback must neither deadlock nor
    // invalidate the owned snapshot currently being validated.
    clear_registry();remember_native_control_proofs(&a,{proof(1,98,1,3),proof(1,99,1,4)});
    c.on_state=[&]{remember_native_control_proofs(&a,{proof(1,98,2,8)});};
    selected=validated_native_control_proofs(&d);
    check(selected.size()==2 && selected[0].life_bound==3 && selected[1].life_bound==4,"publication replacement during host callback retains snapshot");
    check(validated_native_control_proofs(&d).empty(),"next request observes replacement and revalidates");
    check(!native_control_proof_publication_available(old.id),"removed publication expires scheduling token");
    remember_native_control_proofs(&a,{proof(1,98,1,3)});const auto evicted=native_control_proof_publication(&a);
    clear_registry();check(!native_control_proof_publication_available(evicted.id),"registry remains bounded to32 publications");
}
void prefixes() {
    clear_registry();Context c;c.versions={{98,1}};auto d=data(c),p=publisher(11);
    auto first=native_emission_timeline(&d,node(5),98,30);check(bool(first),"prefix lease available");
    auto nested=native_emission_timeline(&d,node(5),98,30);
    check(nested && nested.get()!=first.get(),"nested same-prefix request owns independent data");nested.reset();
    core::EmissionRateProfile rate;rate.constant=10;
    check(first->configure(30,&rate).has_value(),"prefix configured");
    core::NeverCancelled cancel;std::uint64_t work=0;
    check(first->extend(1,[](double){return core::Result<double>::success(10);},cancel,work).has_value(),"prefix extended");
    remember_native_control_proofs(&p,{proof(5,98,1,10)});
    auto replacement=native_emission_timeline(&d,node(5),98,30);
    check(replacement && replacement.get()!=first.get() && first->initialized(),"invalidation removes registry entry without destroying an outstanding lease");
    c.versions[98]=2;auto changed=native_emission_timeline(&d,node(5),98,30);
    check(changed && changed.get()!=replacement.get(),"changed host dependency owns separate prefix");
    check(!native_emission_timeline(&d,node(5),98,24),"unsupported timeline frequency is rejected");
    for(unsigned n=0;n<12;++n)check(bool(native_emission_timeline(&d,node(100+n),98,30)),"bounded prefix churn");
    check(first->initialized(),"old lease survives prefix eviction");
}
void concurrency() {
    clear_registry();auto p=publisher(25);Context c;c.versions={{98,1}};auto d=data(c);
    remember_native_control_proofs(&p,{proof(1,98,1,3)});
    std::atomic<bool> start{},failed{};
    std::thread writer([&]{while(!start.load())std::this_thread::yield();for(unsigned i=0;i<1000;++i)remember_native_control_proofs(&p,{proof(1,98,1,3)});});
    start=true;for(unsigned i=0;i<1000;++i){auto r=validated_native_control_proofs(&d);if(r.size()!=1 || r[0].life_bound!=3)failed=true;}
    writer.join();check(!failed && !held_suites,"concurrent identical publication replacement has no missing/partial proofs");
}
void concurrent_prefixes() {
    clear_registry();Context context;context.versions={{98,1}};auto d=data(context);
    const auto id=node(0x7000);
    auto a=native_emission_timeline(&d,id,98,30),b=native_emission_timeline(&d,id,98,30);
    check(a && b && a.get()!=b.get(),"overlapping frames own independent timelines for the same dependency");
    core::NeverCancelled cancel;std::uint64_t work{};
    const auto rate=[](double t){return core::Result<double>::success(100+10*t);};
    check(a->configure(30).has_value() && b->configure(30).has_value(),"parallel prefixes configure independently");
    check(a->extend(2,rate,cancel,work).has_value(),"first frame extends its own history");
    check(b->segment_count()==0,"another in-flight frame never sees partially appended history");
    check(b->extend(4,rate,cancel,work).has_value(),"second frame extends independently");
    a.reset();b.reset();
    auto accepted=native_emission_timeline(&d,id,98,30);
    check(accepted && accepted->segment_count()==60,"stale overlapping completion cannot replace a published cache generation");
    accepted.reset();
    std::array<Context,4> contexts;std::array<PF_InData,4> copies;
    for(unsigned i=0;i<copies.size();++i){contexts[i].versions={{98,1}};copies[i]=data(contexts[i]);}
    std::array<std::thread,4> workers;std::atomic<bool> start{},failed{};
    for(unsigned i=0;i<workers.size();++i)workers[i]=std::thread([&,i]{
        while(!start.load())std::this_thread::yield();
        for(unsigned frame=0;frame<200;++frame){
            const double time=double((frame*11+i*7)%180)/30+.03;
            auto cached=native_emission_timeline(&copies[i],id,98,30);core::EmissionTimeline reference;
            std::uint64_t cached_work{},reference_work{};
            if(!cached || (!cached->initialized() && !cached->configure(30).has_value()) ||
                !reference.configure(30).has_value() || !cached->extend(time,rate,cancel,cached_work).has_value() ||
                !reference.extend(time,rate,cancel,reference_work).has_value() ||
                cached->integral(time)!=reference.integral(time) || cached->birth(1)!=reference.birth(1))failed=true;
        }
    });
    start=true;for(auto& worker:workers)worker.join();
    check(!failed && !held_suites,"four workers /800 out-of-order frames match uncached birth times and integrals exactly");
}
void allocation_faults() {
    clear_registry();Context c;c.versions={{98,1}};auto d=data(c),p=publisher(31);
    remember_native_control_proofs(&p,{proof(1,98,1,3)});
    for(long long n=0;n<32;++n){fail_after=n;auto r=validated_native_control_proofs(&d);fail_after=-1;
        check((r.empty() || (r.size()==1 && r[0].life_bound==3)) && !held_suites,"allocation failure returns complete valid result or safe miss");}
}
void whole_receipts() {
    clear_registry();Context c;c.versions={{PF_ParamIndex_CHECK_ALL,10},{98,1},{99,1},{100,1}};
    auto d=data(c),a=publisher(41),b=publisher(42);
    auto values=std::vector{proof(1,98,1,3),proof(1,99,1,4),proof(2,100,1,5)};
    values[1].constant=false;values[1].constant_value.reset();values[1].rate->keys.push_back({1,7,core::RateInterpolation::linear});
    values[0].constant_value=3;
    remember_native_control_proofs(&a,values,state(10,PF_ParamIndex_CHECK_ALL));
    auto r=validated_native_control_proofs(&d);
    check(r.size()==3 && r[0].constant_value==3 && !r[1].constant && !r[1].constant_value && r[1].rate->keys.size()==1,
        "whole match retains certified numeric and dynamic profile payloads without inventing constants");
    check(c.states==1 && c.compares==1 && !held_suites,"one complete all-time receipt validates one publication");
    const std::array wanted{node(2),node(2)};c.states=c.compares=0;
    r=validated_native_control_proofs(&d,wanted);
    check(r.size()==1 && r[0].node==node(2) && c.states==1 && c.compares==1,"whole path still filters UUIDs and duplicate requested nodes");
    // A broader state can change for an unrelated input. This is a fast-path
    // miss, not grounds to discard individually valid certificates.
    c.versions[PF_ParamIndex_CHECK_ALL]=11;c.states=c.compares=0;
    r=validated_native_control_proofs(&d);
    check(r.size()==3 && c.states==4 && c.compares==4,"whole mismatch falls back to individually unchanged inputs");
    c.versions[98]=2;r=validated_native_control_proofs(&d);
    check(r.size()==2 && r[0].stream==99,"changed source cannot reuse an old certified number");
    c.versions[98]=1;c.versions[PF_ParamIndex_CHECK_ALL]=10;
    c.failed_stream=PF_ParamIndex_CHECK_ALL;c.states=c.compares=0;r=validated_native_control_proofs(&d);
    check(r.size()==3 && c.states==4 && c.compares==3,"unavailable complete state uses exact per-stream validation");c.failed_stream=-999;
    c.fail_whole_compare=true;r=validated_native_control_proofs(&d);
    check(r.size()==3,"whole comparison error cannot discard valid individual proofs");c.fail_whole_compare=false;
    c.fail_compare=true;check(validated_native_control_proofs(&d).empty(),"comparison failures cannot certify whole or individual proofs");c.fail_compare=false;
    Context copied;copied.versions={{PF_ParamIndex_CHECK_ALL,12},{98,2},{99,2},{100,2}};auto copy=data(copied);
    check(validated_native_control_proofs(&copy).empty(),"copied-layer UUIDs require host equivalence even with complete receipts");
    remember_native_control_proofs(&b,{proof(1,98,2,8)},state(12,PF_ParamIndex_CHECK_ALL));c.states=c.compares=0;
    r=validated_native_control_proofs(&d);
    check(r.size()==3 && r[0].stream==98 && r[0].constant_value==3 && r[1].stream==99 && r[2].stream==100,
        "newer invalid scoped candidate retains ordered older valid scoped fallback");
    check(c.states==2 && c.compares==3,"whole state queried once and compared at most once per publication including fallback");
    clear_registry();remember_native_control_proofs(&a,values,state(10,PF_ParamIndex_CHECK_ALL));
    c.on_state=[&]{clear_registry();};r=validated_native_control_proofs(&d);
    check(r.size()==3 && r[0].constant_value==3 && validated_native_control_proofs(&d).empty(),
        "reentrant eviction retains complete receipt owner and proof payloads until request ends");
    remember_native_control_proofs(&a,values,state(10,PF_ParamIndex_CHECK_ALL));
    for(long long n=0;n<40;++n){fail_after=n;r=validated_native_control_proofs(&d);fail_after=-1;
        check((r.empty() || (r.size()==3 && r[0].constant_value==3 && !r[1].constant)) && !held_suites,
            "scoped allocation failure returns complete certified results or safe miss");}
    std::atomic<bool> start{},failed{};
    std::thread writer([&]{while(!start.load())std::this_thread::yield();for(unsigned i=0;i<1000;++i)remember_native_control_proofs(&a,values,state(10,PF_ParamIndex_CHECK_ALL));});
    start=true;for(unsigned i=0;i<1000;++i){r=validated_native_control_proofs(&d);if(r.size()!=3 || r[0].constant_value!=3 || r[1].constant)failed=true;}
    writer.join();check(!failed && !held_suites,"concurrent scoped publications retain whole receipt and certified payload together");
}
std::vector<NativeControlProof> workload(unsigned count,unsigned rate_keys) {
    std::vector<NativeControlProof> result;result.reserve(count);
    for(unsigned n=0;n<count;++n){auto p=proof(0x5000+n/16,98+static_cast<A_long>(n),42,100);
        if(n%32==0){p.rate->keys.reserve(rate_keys);for(unsigned k=0;k<rate_keys;++k)p.rate->keys.push_back({k/30.,100,core::RateInterpolation::linear});}else p.rate.reset();result.push_back(std::move(p));}
    return result;
}
void benchmark() {
    std::puts("whole_receipt,rate_keys,inputs,publications,median_ms,allocations,bytes_allocated,state_queries,comparisons");
    for(unsigned pass=0;pass<2;++pass)for(bool scoped:{false,true})for(unsigned rate_keys:{0u,1024u})for(unsigned count:{128u,256u,512u})for(unsigned publications:{1u,8u,32u}) {
        clear_registry();auto values=workload(count,rate_keys);Context c;std::vector<core::NodeId> wanted;
        for(unsigned n=0;n<count;++n){c.versions[98+static_cast<A_long>(n)]=42;if(n%16==0)wanted.push_back(node(0x5000+n/16));}
        c.versions[PF_ParamIndex_CHECK_ALL]=7;
        for(unsigned n=0;n<publications;++n){auto p=publisher(1000+n);if(scoped)remember_native_control_proofs(&p,values,state(7,PF_ParamIndex_CHECK_ALL));else remember_native_control_proofs(&p,values);}auto d=data(c);
        double ms[25]{};std::uint64_t heap=0,bytes=0;unsigned states=0,compares=0;
        for(unsigned repeat=0;repeat<25;++repeat){allocations=0;allocated_bytes=0;c.states=c.compares=0;count_allocations=true;
            const auto begin=std::chrono::steady_clock::now();auto selected=validated_native_control_proofs(&d,wanted);
            ms[repeat]=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();count_allocations=false;
            check(selected.size()==count,"benchmark result count");heap=allocations;bytes=allocated_bytes;states=c.states;compares=c.compares;
            check(states==(scoped?1:count) && compares==(scoped?1:count),"valid newest publication queries once per stream or once per certified whole scope");
        }
        std::sort(ms,ms+25);if(pass)std::printf("%u,%u,%u,%u,%.6f,%llu,%llu,%u,%u\n",unsigned(scoped),rate_keys,count,publications,ms[12],static_cast<unsigned long long>(heap),static_cast<unsigned long long>(bytes),states,compares);
    }
}
}
int main(int argc,char** argv)try {
    param_utils.PF_GetCurrentState=[](PF_ProgPtr ref,PF_ParamIndex stream,const A_Time* start,const A_Time* duration,PF_State* out)->PF_Err {
        auto& c=*reinterpret_cast<Context*>(ref);++c.states;if(c.trace)c.events.push_back(stream);if(start || duration)return PF_Err_BAD_CALLBACK_PARAM;
        if(c.on_state){auto callback=std::move(c.on_state);callback();}if(c.throw_state)throw std::bad_alloc{};
        const auto found=c.versions.find(stream);if(stream==c.failed_stream || found==c.versions.end())return PF_Err_BAD_CALLBACK_PARAM;
        *out=state(found->second,stream);return 0;};
    param_utils.PF_AreStatesIdentical=[](PF_ProgPtr ref,const PF_State* a,const PF_State* b,A_Boolean* same)->PF_Err {
        auto& c=*reinterpret_cast<Context*>(ref);++c.compares;if(c.trace){std::uint64_t token{};std::memcpy(&token,b,sizeof(token));c.events.push_back(-A_long(std::uint32_t(token)));}if(c.fail_compare)return PF_Err_BAD_CALLBACK_PARAM;
        std::uint64_t token{};std::memcpy(&token,a,sizeof(token));
        if(c.fail_whole_compare && std::uint32_t(token)==std::uint32_t(PF_ParamIndex_CHECK_ALL))return PF_Err_BAD_CALLBACK_PARAM;
        *same=std::memcmp(a,b,sizeof(*a))==0;return 0;};
    const bool only_benchmark=argc>1 && !std::strcmp(argv[1],"--benchmark-only");
    if(!only_benchmark){publication_failure();behavior();prefixes();concurrency();concurrent_prefixes();allocation_faults();whole_receipts();}
    if(only_benchmark || (argc>1 && !std::strcmp(argv[1],"--benchmark")))benchmark();
    std::printf("Native temporal cache: %u checks, 0 failures\n",checks);return 0;
}catch(const char* message){fail_after=-1;count_allocations=false;std::fprintf(stderr,"FAIL: %s\n",message);return 1;}
catch(...){fail_after=-1;count_allocations=false;std::fputs("Unexpected fixture exception\n",stderr);return 1;}
