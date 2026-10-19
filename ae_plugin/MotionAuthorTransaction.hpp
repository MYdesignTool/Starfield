#pragma once
#include "MotionLayout.hpp"
#include "NodeGraphSync.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <array>
#include <vector>
#include <memory>
#include <cstring>
#include <cmath>

namespace starfield::adapter {
// Only values survive a phase boundary. No source stream may survive its
// effect owner, graph publication, or another parameter write.
class MotionAuthorTransaction final {
    using Uuid=std::array<A_long,8>;
    static constexpr auto kind=native_nodes::Kind::motion;
    static constexpr auto mode_index=native_nodes::motion_layout::mode;
    static constexpr auto guard_index=native_nodes::sync_guard_index(kind);
    struct PrefixChange {motion_prefix::Bank before{},after{};};
    struct Change {
        Uuid uuid{};
        double previous{},previous_guard{},wanted{};
        A_long index{-1};
        bool changed{};
        std::unique_ptr<PrefixChange> prefix;
    };
    struct Effect {
        const AEGP_EffectSuite4* suite{};AEGP_EffectRefH value{};
        ~Effect(){if(value)suite->AEGP_DisposeEffect(value);}
    };
    struct Stream {
        const AEGP_StreamSuite6* suite{};AEGP_StreamRefH value{};
        ~Stream(){if(value)suite->AEGP_DisposeStream(value);}
    };
    PF_InData* data_{};AEGP_PluginID id_{};AEGP_LayerH layer_{};
    const AEGP_EffectSuite4* effects_{};const AEGP_StreamSuite6* streams_{};
    std::vector<Change> changes_;bool accepted_{};

    A_Err read(AEGP_EffectRefH effect,A_long index,double& value) const noexcept {
        Stream ref{streams_};
        auto error=streams_->AEGP_GetNewEffectStreamByIndex(id_,effect,index,&ref.value);
        if(!error && !ref.value)error=PF_Err_BAD_CALLBACK_PARAM;
        AEGP_StreamType type{};
        if(!error)error=streams_->AEGP_GetStreamType(ref.value,&type);
        if(!error && type!=AEGP_StreamType_OneD)error=PF_Err_BAD_CALLBACK_PARAM;
        if(error)return error;
        AEGP_StreamValue2 saved{};const A_Time time{data_->current_time,data_->time_scale};
        error=streams_->AEGP_GetNewStreamValue(id_,ref.value,AEGP_LTimeMode_LayerTime,&time,TRUE,&saved);
        if(!error){value=saved.val.one_d;error=streams_->AEGP_DisposeStreamValue(&saved);
            if(!error && !std::isfinite(value))error=PF_Err_BAD_CALLBACK_PARAM;}
        return error;
    }
    A_Err write(AEGP_EffectRefH effect,A_long index,double value) const noexcept {
        Stream ref{streams_};
        auto error=streams_->AEGP_GetNewEffectStreamByIndex(id_,effect,index,&ref.value);
        if(!error && !ref.value)error=PF_Err_BAD_CALLBACK_PARAM;
        if(error)return error;
        AEGP_StreamValue2 saved{};saved.streamH=ref.value;saved.val.one_d=value;
        return streams_->AEGP_SetStreamValue(id_,ref.value,&saved);
    }
    A_Err verify(AEGP_EffectRefH effect,A_long index,double wanted) const noexcept {
        double observed{};auto error=read(effect,index,observed);
        return error?error:observed==wanted?0:PF_Err_BAD_CALLBACK_PARAM;
    }
    A_Err prefix_values(AEGP_EffectRefH effect,const motion_prefix::Bank& values,bool writing) const noexcept {
        for(unsigned i=0;i<values.size();++i){const auto index=native_nodes::motion_layout::path_prefix_length+int(i);
            auto error=writing?write(effect,index,values[i]):0;
            if(!error)error=verify(effect,index,values[i]);if(error)return error;}
        return 0;
    }
    A_Err identity(AEGP_EffectRefH effect,Uuid& uuid) const noexcept {
        for(A_long chunk=0;chunk<8;++chunk){
            double value{};auto error=read(effect,native_nodes::uuid_first_index(kind)+chunk,value);
            if(error)return error;
            if(value<0 || value>65535 || std::floor(value)!=value)return PF_Err_BAD_CALLBACK_PARAM;
            uuid[chunk]=static_cast<A_long>(value);
        }
        return 0;
    }
    A_Err is_motion(AEGP_EffectRefH effect,bool& matches) const noexcept {
        AEGP_InstalledEffectKey key{};char name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        auto error=effects_->AEGP_GetInstalledKeyFromLayerEffect(effect,&key);
        if(!error)error=effects_->AEGP_GetEffectMatchName(key,name);
        matches=!error && !std::strcmp(name,"org.starfieldfx.node.motion");return error;
    }
    A_Err count(A_long& value) const noexcept {
        auto error=effects_->AEGP_GetLayerNumEffects(layer_,&value);
        return error?error:value<0 || value>4224?PF_Err_BAD_CALLBACK_PARAM:0;
    }
    A_Err open(A_long index,Effect& ref) const noexcept {
        auto error=effects_->AEGP_GetLayerEffectByIndex(id_,layer_,index,&ref.value);
        return error?error:ref.value?0:PF_Err_BAD_CALLBACK_PARAM;
    }
    // O(N log M) inventory per phase, instead of a full scan per changed node.
    // Indices are phase-local hints; reopened effects are checked by UUID again.
    A_Err resolve() noexcept {
        for(auto& change:changes_)change.index=-1;
        A_long length{};auto error=count(length);if(error)return error;
        for(A_long index=0;index<length;++index){
            Effect ref{effects_};error=open(index,ref);if(error)return error;
            bool matches{};error=is_motion(ref.value,matches);if(error)return error;
            if(!matches)continue;
            Uuid uuid{};error=identity(ref.value,uuid);if(error)return error;
            auto found=std::lower_bound(changes_.begin(),changes_.end(),uuid,
                [](const Change& a,const Uuid& b){return a.uuid<b;});
            if(found==changes_.end() || found->uuid!=uuid)continue;
            found->index=found->index==-1?index:-2;
        }
        for(const auto& change:changes_)if(change.index<0)return PF_Err_BAD_CALLBACK_PARAM;
        return 0;
    }
    A_Err open_change(const Change& change,Effect& ref) const noexcept {
        if(change.index<0)return PF_Err_BAD_CALLBACK_PARAM;
        auto error=open(change.index,ref);bool matches{};
        if(!error)error=is_motion(ref.value,matches);
        if(!error && !matches)error=PF_Err_BAD_CALLBACK_PARAM;
        Uuid uuid{};if(!error)error=identity(ref.value,uuid);
        if(!error && uuid!=change.uuid)error=PF_Err_BAD_CALLBACK_PARAM;
        return error;
    }
public:
    MotionAuthorTransaction(PF_InData* data,AEGP_PluginID id):data_(data),id_(id){}
    MotionAuthorTransaction(const MotionAuthorTransaction&)=delete;
    ~MotionAuthorTransaction(){
        rollback(); // Exception safety; normal errors use the explicit result.
        if(effects_)data_->pica_basicP->ReleaseSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
        if(streams_)data_->pica_basicP->ReleaseSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    }
    PF_Err prepare(AEGP_LayerH layer,const node_sync::NativeEdit* edit){
        const bool prefix=edit && !edit->records_only && edit->node_kind==7 && edit->parameter_index==native_nodes::motion_layout::path_prefix_length;
        if(prefix && !node_sync::valid_edit(*edit))return PF_Err_BAD_CALLBACK_PARAM;
        const bool direct=prefix || (edit && !edit->records_only && edit->node_kind==7 && edit->parameter_index==mode_index);
        if(edit && !edit->records_only && !direct)return PF_Err_NONE;
        layer_=layer;
        auto error=data_->pica_basicP->AcquireSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4,reinterpret_cast<const void**>(&effects_));
        if(!error)error=data_->pica_basicP->AcquireSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,reinterpret_cast<const void**>(&streams_));
        if(error || !effects_ || !streams_)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
        A_long length{};error=count(length);if(error)return static_cast<PF_Err>(error);
        for(A_long index=0;index<length;++index){
            Effect ref{effects_};error=open(index,ref);if(error)return static_cast<PF_Err>(error);
            bool matches{};error=is_motion(ref.value,matches);if(error)return static_cast<PF_Err>(error);
            if(!matches)continue;
            Change change;
            if(prefix){
                error=identity(ref.value,change.uuid);if(error)return static_cast<PF_Err>(error);
                if(!std::equal(change.uuid.begin(),change.uuid.end(),edit->uuid.begin()))continue;
            }
            error=read(ref.value,guard_index,change.previous_guard);
            if(error)return static_cast<PF_Err>(error);
            if(change.previous_guard<0 || change.previous_guard>6 || std::floor(change.previous_guard)!=change.previous_guard)
                return PF_Err_BAD_CALLBACK_PARAM;
            const bool intent=change.previous_guard>=4;
            if(prefix && change.previous_guard!=0)return PF_Err_BAD_CALLBACK_PARAM;
            if(!intent && !(direct && change.previous_guard==0))continue;
            error=identity(ref.value,change.uuid);if(error)return static_cast<PF_Err>(error);
            if(!intent && !std::equal(change.uuid.begin(),change.uuid.end(),edit->uuid.begin()))continue;
            if(std::all_of(change.uuid.begin(),change.uuid.end(),[](A_long v){return v==0;}))return PF_Err_BAD_CALLBACK_PARAM;
            error=read(ref.value,mode_index,change.previous);
            if(error || change.previous<1 || change.previous>3 || std::floor(change.previous)!=change.previous)
                return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
            change.wanted=prefix?change.previous:intent?(change.previous_guard==5?1:change.previous_guard==4?2:3):edit->value[0];
            if(change.wanted<1 || change.wanted>3 || std::floor(change.wanted)!=change.wanted)return PF_Err_BAD_CALLBACK_PARAM;
            if(prefix){
                if(change.previous==1)return PF_Err_BAD_CALLBACK_PARAM;
                change.prefix=std::make_unique<PrefixChange>();change.prefix->before=edit->motion_prefix_previous;
                for(unsigned i=0;i<change.prefix->after.size();++i)change.prefix->after[i]=edit->additional_fields[i].value[0];
                error=prefix_values(ref.value,change.prefix->before,false);if(error)return static_cast<PF_Err>(error);
            }
            changes_.push_back(std::move(change));
        }
        if(prefix && changes_.size()!=1)return PF_Err_BAD_CALLBACK_PARAM;
        if(changes_.empty())return PF_Err_NONE;
        std::sort(changes_.begin(),changes_.end(),[](const Change& a,const Change& b){return a.uuid<b.uuid;});
        for(std::size_t i=1;i<changes_.size();++i)if(changes_[i-1].uuid==changes_[i].uuid)return PF_Err_BAD_CALLBACK_PARAM;
        error=resolve();if(error)return static_cast<PF_Err>(error);
        for(auto& change:changes_){
            Effect ref{effects_};error=open_change(change,ref);
            if(!error)error=verify(ref.value,guard_index,change.previous_guard);
            if(!error)error=verify(ref.value,mode_index,change.previous);
            if(!error && change.prefix)error=prefix_values(ref.value,change.prefix->before,false);
            if(error)return static_cast<PF_Err>(error);
            change.changed=true; // Includes partially applied failed writes.
            if(change.previous_guard==0)error=write(ref.value,guard_index,1);
            if(!error)error=change.prefix?prefix_values(ref.value,change.prefix->after,true):write(ref.value,mode_index,change.wanted);
            if(!error)error=verify(ref.value,mode_index,change.wanted);
            if(error)return static_cast<PF_Err>(error);
        }
        return PF_Err_NONE;
    }
    PF_Err accept() noexcept {
        if(!changes_.empty()){
            auto error=resolve();if(error)return static_cast<PF_Err>(error);
            for(auto& change:changes_){
                Effect ref{effects_};error=open_change(change,ref);
                if(!error)error=verify(ref.value,guard_index,change.previous_guard==0?1:change.previous_guard);
                if(!error)error=verify(ref.value,mode_index,change.wanted);
                if(!error && change.prefix)error=prefix_values(ref.value,change.prefix->after,false);
                if(!error)error=write(ref.value,guard_index,0);
                if(!error)error=verify(ref.value,guard_index,0);
                if(!error)error=verify(ref.value,mode_index,change.wanted);
                if(!error && change.prefix)error=prefix_values(ref.value,change.prefix->after,false);
                if(error)return static_cast<PF_Err>(error);
            }
        }
        accepted_=true;return PF_Err_NONE;
    }
    PF_Err rollback() noexcept {
        if(accepted_ || std::none_of(changes_.begin(),changes_.end(),[](const Change& c){return c.changed;}))return PF_Err_NONE;
        bool failed=resolve()!=0;
        // An incomplete inventory cannot authorize any cached-index write.
        if(failed){accepted_=true;return PF_Err_INTERNAL_STRUCT_DAMAGED;}
        for(auto it=changes_.rbegin();it!=changes_.rend();++it)if(it->changed){
            Effect ref{effects_};auto error=open_change(*it,ref);
            const bool verified=!error;
            if(!error)error=write(ref.value,guard_index,1);
            if(!error)error=verify(ref.value,guard_index,1);
            if(!error)error=it->prefix?prefix_values(ref.value,it->prefix->before,true):write(ref.value,mode_index,it->previous);
            if(!error)error=verify(ref.value,mode_index,it->previous);
            if(!error)error=write(ref.value,guard_index,it->previous_guard);
            if(!error)error=verify(ref.value,guard_index,it->previous_guard);
            if(error){failed=true;if(verified)write(ref.value,guard_index,1);}
        }
        accepted_=true; // Do not silently retry failed restoration in a destructor.
        return failed?PF_Err_INTERNAL_STRUCT_DAMAGED:PF_Err_NONE;
    }
};
}
