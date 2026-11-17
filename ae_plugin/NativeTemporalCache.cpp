#include "NativeTemporalCache.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <map>
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
// This token only deduplicates UI publications. It is never dereferenced or
// required to equal a render callback's opaque reference.
struct ProofSet {PF_ProgPtr publisher{};std::vector<NativeControlProof> proofs;std::uint64_t publication{};std::optional<PF_State> whole_state;};
using ProofSnapshot=std::shared_ptr<const ProofSet>;
struct ProofCandidate {const NativeControlProof* proof{};std::size_t order{};const ProofSet* owner{};};
struct ProofCatalog {
    std::vector<ProofSnapshot> publications; // oldest to newest, owns candidate pointers
    std::map<core::NodeId,std::map<A_long,std::vector<ProofCandidate>>> nodes;
};
struct Prefix {
    core::NodeId node{};A_long stream{};unsigned hz{};PF_State state{};
    std::mutex mutex;core::EmissionTimeline timeline;std::uint64_t revision{};
};
std::mutex registry_mutex;
std::mutex publication_mutex; // writers only; never held by render/SDK callbacks
NativeMetadataTrace metadata_trace;
std::shared_ptr<const ProofCatalog> proof_catalog;
std::uint64_t next_publication{};
std::vector<std::shared_ptr<Prefix>> prefixes;
constexpr std::size_t kMaxProofInstances=32,kMaxPrefixes=8,kMaxCachedSegments=131072;
std::shared_ptr<const ProofCatalog> current_catalog() {
    std::lock_guard lock(registry_mutex);return proof_catalog;
}
struct PrefixLease {
    std::shared_ptr<Prefix> entry;
    core::EmissionTimeline timeline;
    std::uint64_t revision{};
    explicit PrefixLease(std::shared_ptr<Prefix> p):entry(std::move(p)) {
        // Copy only owned numeric data. Release before any caller's SDK callback.
        std::lock_guard lock(entry->mutex);timeline=entry->timeline;revision=entry->revision;
    }
    ~PrefixLease() {
        if(timeline.segment_count()>kMaxCachedSegments)timeline=core::EmissionTimeline{};
        std::lock_guard lock(entry->mutex);
        if(entry->revision!=revision)return; // Another request already published.
        entry->timeline=std::move(timeline);
        ++entry->revision;
    }
};
}
void record_native_metadata_trace(const NativeMetadataTrace& trace) noexcept {
    std::lock_guard lock(registry_mutex);metadata_trace=trace;
}
NativeMetadataTrace last_native_metadata_trace() noexcept {
    std::lock_guard lock(registry_mutex);return metadata_trace;
}
NativeProofPublication native_control_proof_publication(PF_InData* data) noexcept {
    if(!data)return {};
    const auto catalog=current_catalog();if(!catalog)return {};
    for(auto entry=catalog->publications.rbegin();entry!=catalog->publications.rend();++entry)
        if((*entry)->publisher==data->effect_ref && !(*entry)->proofs.empty())return {(*entry)->publication,(*entry)->proofs.size()};
    return {};
}
bool native_control_proof_publication_available(std::uint64_t id) noexcept {
    if(!id)return false;
    const auto catalog=current_catalog();
    return catalog && std::any_of(catalog->publications.begin(),catalog->publications.end(),[&](const auto& p){return p->publication==id && !p->proofs.empty();});
}
static void remember_proofs(PF_InData* data,std::vector<NativeControlProof> values,std::optional<PF_State> whole_state) noexcept try {
    if(!data || !data->effect_ref) return;
    // Build payloads before taking the registry lock. Readers retain immutable
    // snapshots; replacement and eviction cannot invalidate an SDK callback.
    auto replacement=std::make_shared<ProofSet>(ProofSet{data->effect_ref,std::move(values),0,whole_state});
    std::shared_ptr<const ProofCatalog> previous;
    std::vector<std::shared_ptr<Prefix>> retired_prefixes;retired_prefixes.reserve(kMaxPrefixes);
    {std::lock_guard writer(publication_mutex);
    previous=current_catalog();
    auto next=std::make_shared<ProofCatalog>();next->publications.reserve(kMaxProofInstances);
    if(previous)next->publications=previous->publications;
    std::erase_if(next->publications,[&](const auto& p){return p->publisher==data->effect_ref;});
    if(next->publications.size()>=kMaxProofInstances)next->publications.erase(next->publications.begin());
    if(++next_publication==0)++next_publication;
    replacement->publication=next_publication;next->publications.push_back(replacement);
    // Build the immutable candidate index outside the registry lock. Each key
    // retains newest-first order, without copying any rate-profile payload.
    std::size_t order=0;
    for(auto set=next->publications.rbegin();set!=next->publications.rend();++set)
        for(const auto& proof:(*set)->proofs)next->nodes[proof.node][proof.stream].push_back({&proof,order++,set->get()});
    // Invalidate sampled prefixes when a UI publication supplies an analytic
    // profile, including prefixes created by a different render callback copy.
    const auto affected=[&](const auto& p) {
        const auto matches=[&](const auto& proof){return proof.node==p->node && proof.stream==p->stream;};
        if(std::any_of(replacement->proofs.begin(),replacement->proofs.end(),matches)) return true;
        if(previous)for(const auto& old:previous->publications) if(old->publisher==data->effect_ref &&
            std::any_of(old->proofs.begin(),old->proofs.end(),matches)) return true;
        return false;
    };
    {std::lock_guard lock(registry_mutex);
    std::erase_if(prefixes,[&](auto& p){if(!affected(p))return false;retired_prefixes.push_back(std::move(p));return true;});
    proof_catalog=std::move(next);}}
} catch(...) {} // Optimization failure never rejects an authored edit.
void remember_native_control_proofs(PF_InData* data,std::vector<NativeControlProof> values) noexcept {
    remember_proofs(data,std::move(values),{});
}
void remember_native_control_proofs(PF_InData* data,std::vector<NativeControlProof> values,const PF_State& whole_state) noexcept {
    remember_proofs(data,std::move(values),whole_state);
}
std::vector<NativeControlProof> validated_native_control_proofs(PF_InData* data,
    std::span<const core::NodeId> nodes) noexcept try {
    const auto catalog=current_catalog();
    std::vector<NativeControlProof> result;
    if(!catalog || catalog->nodes.empty())return result;
    std::vector<core::NodeId> wanted(nodes.begin(),nodes.end());
    std::sort(wanted.begin(),wanted.end());wanted.erase(std::unique(wanted.begin(),wanted.end()),wanted.end());
    // Merge only unresolved key heads. A valid newest proof eliminates all its
    // older copies without visiting them. The heap preserves the old complete
    // publication/proof traversal order, including SDK calls and old fallback.
    struct Cursor {const std::vector<ProofCandidate>* candidates{};std::size_t at{};};
    std::size_t keys=0;
    const auto visit=[&](const auto& callback) {
        if(wanted.empty()){for(const auto& [id,streams]:catalog->nodes){(void)id;callback(streams);}}
        else for(const auto id:wanted)if(const auto found=catalog->nodes.find(id);found!=catalog->nodes.end())callback(found->second);
    };
    visit([&](const auto& streams){keys+=streams.size();});
    if(!keys)return result;
    std::vector<Cursor> pending;pending.reserve(keys);result.reserve(keys);
    visit([&](const auto& streams){for(const auto& [stream,candidates]:streams){(void)stream;pending.push_back({&candidates,0});}});
    const auto later=[](const Cursor& a,const Cursor& b){return (*a.candidates)[a.at].order>(*b.candidates)[b.at].order;};
    std::make_heap(pending.begin(),pending.end(),later);
    std::optional<ParamUtils> utils;
    std::map<A_long,std::optional<PF_State>> current_states;
    bool whole_queried=false;std::optional<PF_State> whole_current;
    std::vector<std::pair<const ProofSet*,bool>> whole_matches;
    while(!pending.empty()) {
        std::pop_heap(pending.begin(),pending.end(),later);auto cursor=pending.back();pending.pop_back();
        const auto& candidate=(*cursor.candidates)[cursor.at];const auto& proof=*candidate.proof;
        if(!utils)utils.emplace(data);
        if(candidate.owner->whole_state) {
            if(!whole_queried) {
                whole_queried=true;PF_State state{};
                if(utils->state(PF_ParamIndex_CHECK_ALL,state))whole_current=state;
            }
            if(whole_current) {
                auto match=std::find_if(whole_matches.begin(),whole_matches.end(),[&](const auto& m){return m.first==candidate.owner;});
                if(match==whole_matches.end()) {
                    whole_matches.emplace_back(candidate.owner,utils->same(*candidate.owner->whole_state,*whole_current));
                    match=std::prev(whole_matches.end());
                }
                // This receipt encloses certification of every proof, including
                // owned numbers. Matching it does not turn dynamic inputs into
                // constants. On any miss retain exact individual validation.
                if(match->second){result.push_back(proof);continue;}
            }
        }
        auto [current,inserted]=current_states.try_emplace(proof.stream);
        if(inserted) {PF_State state{};if(utils->state(proof.stream,state))current->second=state;}
        if(current->second && utils->same(proof.state,*current->second)) {
            result.push_back(proof);
        } else if(current->second && ++cursor.at<cursor.candidates->size()) {
            pending.push_back(cursor);std::push_heap(pending.begin(),pending.end(),later);
        }
    }
    return result;
} catch(...) {return {};}
std::shared_ptr<core::EmissionTimeline> native_emission_timeline(PF_InData* data,
    core::NodeId node,A_long stream,unsigned hz) noexcept try {
    ParamUtils utils(data);PF_State state{};
    if(!utils.state(stream,state) || (hz!=30 && hz!=60 && hz!=120)) return {};
    std::vector<std::shared_ptr<Prefix>> candidates;
    {std::lock_guard lock(registry_mutex);
        for(const auto& p:prefixes) if(p->node==node && p->stream==stream && p->hz==hz) candidates.push_back(p);}
    std::shared_ptr<Prefix> chosen;
    for(const auto& p:candidates) if(utils.same(p->state,state)) {chosen=p;break;}
    if(!chosen) {
        chosen=std::make_shared<Prefix>();chosen->node=node;
        chosen->stream=stream;chosen->hz=hz;chosen->state=state;
        std::shared_ptr<Prefix> retired;
        {std::lock_guard lock(registry_mutex);
        prefixes.reserve(kMaxPrefixes);
        // Keep separate dependency versions for copied layers / concurrent frame
        // copies. Only equal host states can share data; the registry is bounded.
        if(prefixes.size()>=kMaxPrefixes){retired=std::move(prefixes.front());prefixes.erase(prefixes.begin());}
        prefixes.push_back(chosen);}
    }
    auto lease=std::make_shared<PrefixLease>(chosen);
    auto* timeline=&lease->timeline;
    return std::shared_ptr<core::EmissionTimeline>(std::move(lease),timeline);
} catch(...) {return {};}
}
