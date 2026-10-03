#include "NativeTemporalCache.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <mutex>
#include <utility>

namespace starfield::adapter {
namespace {
struct ParamUtils {
    PF_InData* data{}; const PF_ParamUtilsSuite3* suite{};
    explicit ParamUtils(PF_InData* d):data(d) {
        if(d && d->effect_ref && d->pica_basicP)
            (void)d->pica_basicP->AcquireSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3,reinterpret_cast<const void**>(&suite));
    }
    ~ParamUtils() {if(suite)data->pica_basicP->ReleaseSuite(kPFParamUtilsSuite,kPFParamUtilsSuiteVersion3);}
    bool state(A_long stream,PF_State& value) const {
        return suite && suite->PF_GetCurrentState && suite->PF_AreStatesIdentical &&
            suite->PF_GetCurrentState(data->effect_ref,stream,nullptr,nullptr,&value)==0;
    }
    bool same(const PF_State& a,const PF_State& b) const {
        A_Boolean equal=FALSE;
        return suite && suite->PF_AreStatesIdentical &&
            suite->PF_AreStatesIdentical(data->effect_ref,&a,&b,&equal)==0 && equal;
    }
};
struct ProofSet {PF_ProgPtr instance{};std::vector<NativeControlProof> proofs;};
struct Prefix {
    PF_ProgPtr instance{};core::NodeId node{};A_long stream{};unsigned hz{};PF_State state{};
    std::mutex mutex;core::EmissionTimeline timeline;
};
std::mutex registry_mutex;
std::vector<ProofSet> proofs;
std::vector<std::shared_ptr<Prefix>> prefixes;
constexpr std::size_t kMaxProofInstances=32,kMaxPrefixes=8,kMaxCachedSegments=131072;
struct PrefixLease {
    std::shared_ptr<Prefix> entry;
    std::unique_lock<std::mutex> lock;
    explicit PrefixLease(std::shared_ptr<Prefix> p):entry(std::move(p)),lock(entry->mutex,std::try_to_lock) {}
    ~PrefixLease() {
        // A retained long timeline cannot make memory grow without a bound.
        if(lock.owns_lock() && entry->timeline.segment_count()>kMaxCachedSegments)
            entry->timeline=core::EmissionTimeline{};
    }
};
}
void remember_native_control_proofs(PF_InData* data,std::vector<NativeControlProof> values) noexcept try {
    if(!data || !data->effect_ref) return;
    std::lock_guard lock(registry_mutex);
    std::erase_if(proofs,[&](const auto& p){return p.instance==data->effect_ref;});
    if(proofs.size()>=kMaxProofInstances) proofs.erase(proofs.begin());
    proofs.push_back({data->effect_ref,std::move(values)});
    // A newly certified analytic profile must replace a sampled prefix.
    std::erase_if(prefixes,[&](const auto& p){return p->instance==data->effect_ref;});
} catch(...) {} // Optimization failure never rejects an authored edit.
std::vector<NativeControlProof> validated_native_control_proofs(PF_InData* data) noexcept try {
    std::vector<NativeControlProof> candidates,result;
    {std::lock_guard lock(registry_mutex);
        if(data) for(const auto& p:proofs) if(p.instance==data->effect_ref) {candidates=p.proofs;break;}}
    if(candidates.empty())return result;
    ParamUtils utils(data);
    for(auto& proof:candidates) {
        PF_State current{};
        if(utils.state(proof.stream,current) && utils.same(proof.state,current)) result.push_back(std::move(proof));
    }
    return result;
} catch(...) {return {};}
std::shared_ptr<core::EmissionTimeline> native_emission_timeline(PF_InData* data,
    core::NodeId node,A_long stream,unsigned hz) noexcept try {
    ParamUtils utils(data);PF_State state{};
    if(!utils.state(stream,state) || (hz!=30 && hz!=60 && hz!=120)) return {};
    std::vector<std::shared_ptr<Prefix>> candidates;
    {std::lock_guard lock(registry_mutex);
        for(const auto& p:prefixes) if(p->instance==data->effect_ref && p->node==node && p->stream==stream && p->hz==hz) candidates.push_back(p);}
    std::shared_ptr<Prefix> chosen;
    for(const auto& p:candidates) if(utils.same(p->state,state)) {chosen=p;break;}
    if(!chosen) {
        chosen=std::make_shared<Prefix>();chosen->instance=data->effect_ref;chosen->node=node;
        chosen->stream=stream;chosen->hz=hz;chosen->state=state;
        std::lock_guard lock(registry_mutex);
        // Retire stale entries; active frame leases keep their own data alive.
        std::erase_if(prefixes,[&](const auto& p){return p->instance==data->effect_ref && p->node==node && p->stream==stream && p->hz==hz;});
        if(prefixes.size()>=kMaxPrefixes) prefixes.erase(prefixes.begin());
        prefixes.push_back(chosen);
    }
    auto lease=std::make_shared<PrefixLease>(chosen);
    if(!lease->lock.owns_lock()) return {}; // No nested cache-lock waits between emitters.
    return std::shared_ptr<core::EmissionTimeline>(std::move(lease),&chosen->timeline);
} catch(...) {return {};}
}
