#include "EffectGraphBackup.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace starfield::adapter {
namespace {
constexpr A_Err bad=PF_Err_BAD_CALLBACK_PARAM;
struct Ref {const AEGP_EffectSuite4* suite;AEGP_EffectRefH ref;~Ref(){if(ref)suite->AEGP_DisposeEffect(ref);}};
struct Stream {const AEGP_StreamSuite6* suite;AEGP_StreamRefH ref;~Stream(){if(ref)suite->AEGP_DisposeStream(ref);}};
}
EffectGraphBackup::EffectGraphBackup(SPBasicSuite* b,AEGP_PluginID p,AEGP_LayerH l) noexcept:basic_(b),plugin_(p),layer_(l){
    if(!basic_)return;
    basic_->AcquireSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4,reinterpret_cast<const void**>(&effects_));
    basic_->AcquireSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,reinterpret_cast<const void**>(&streams_));
    basic_->AcquireSuite(kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4,reinterpret_cast<const void**>(&dynamic_));
    basic_->AcquireSuite(kAEGPMemorySuite,kAEGPMemorySuiteVersion1,reinterpret_cast<const void**>(&memory_));
}
EffectGraphBackup::~EffectGraphBackup(){
    if(prepared_)(void)restore();
    if(!basic_)return;
    if(memory_)basic_->ReleaseSuite(kAEGPMemorySuite,kAEGPMemorySuiteVersion1);
    if(dynamic_)basic_->ReleaseSuite(kAEGPDynamicStreamSuite,kAEGPDynamicStreamSuiteVersion4);
    if(streams_)basic_->ReleaseSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(effects_)basic_->ReleaseSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
}
A_Err EffectGraphBackup::scalar(AEGP_EffectRefH effect,A_long index,double& number) noexcept {
    AEGP_StreamRefH raw{};auto e=streams_->AEGP_GetNewEffectStreamByIndex(plugin_,effect,index,&raw);
    Stream ref{streams_,raw};if(e || !raw)return e?e:bad;
    AEGP_StreamType type{};e=streams_->AEGP_GetStreamType(raw,&type);if(e || type!=AEGP_StreamType_OneD)return e?e:bad;
    AEGP_StreamValue2 value{};const A_Time zero{0,1};e=streams_->AEGP_GetNewStreamValue(plugin_,raw,AEGP_LTimeMode_LayerTime,&zero,TRUE,&value);
    if(e)return e;number=value.val.one_d;return streams_->AEGP_DisposeStreamValue(&value);
}
A_Err EffectGraphBackup::write_scalar(AEGP_EffectRefH effect,A_long index,double number) noexcept {
    AEGP_StreamRefH raw{};auto e=streams_->AEGP_GetNewEffectStreamByIndex(plugin_,effect,index,&raw);
    Stream ref{streams_,raw};if(e || !raw)return e?e:bad;
    AEGP_StreamValue2 value{};value.streamH=raw;value.val.one_d=number;e=streams_->AEGP_SetStreamValue(plugin_,raw,&value);
    if(e)return e;double observed{};e=scalar(effect,index,observed);return e?e:observed==number?0:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
A_Err EffectGraphBackup::describe(AEGP_EffectRefH effect,Record& record,bool& supported) noexcept {
    supported=true;AEGP_InstalledEffectKey key{};char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
    auto e=effects_->AEGP_GetInstalledKeyFromLayerEffect(effect,&key);if(!e)e=effects_->AEGP_GetEffectMatchName(key,match);if(e)return e;
    if(!std::strcmp(match,"org.starfieldfx.particle")){record.kind=-1;record.guard=kGraphSyncGuardId;return 0;}
    for(auto [label,kind]:{std::pair{"org.starfieldfx.node.emitter",native_nodes::Kind::emitter},
        std::pair{"org.starfieldfx.node.particle",native_nodes::Kind::particle},std::pair{"org.starfieldfx.node.force",native_nodes::Kind::force},
        std::pair{"org.starfieldfx.node.transform",native_nodes::Kind::transform},std::pair{"org.starfieldfx.node.model",native_nodes::Kind::model}}){
        if(std::strcmp(label,match))continue;record.kind=static_cast<A_long>(kind);record.guard=native_nodes::sync_guard_index(kind);
        record.uuid_first=native_nodes::uuid_first_index(kind);return 0;}
    supported=false;return 0;
}
A_Err EffectGraphBackup::identity(AEGP_EffectRefH effect,const Record& r,std::array<std::uint8_t,16>& uuid) noexcept {
    if(r.kind<0)return 0;
    for(A_long chunk=0;chunk<8;++chunk){double value{};const auto e=scalar(effect,r.uuid_first+chunk,value);
        if(e)return e;if(!std::isfinite(value) || value<0 || value>65535 || std::floor(value)!=value)return bad;
        const auto word=static_cast<unsigned>(value);uuid[chunk*2]=static_cast<std::uint8_t>(word>>8);uuid[chunk*2+1]=static_cast<std::uint8_t>(word);}
    return std::any_of(uuid.begin(),uuid.end(),[](auto b){return b!=0;})?0:bad;
}
A_Err EffectGraphBackup::write_identity(AEGP_EffectRefH effect,const Record& r,const std::array<std::uint8_t,16>& uuid) noexcept {
    if(r.kind<0)return 0;
    for(A_long chunk=0;chunk<8;++chunk){const auto e=write_scalar(effect,r.uuid_first+chunk,(unsigned(uuid[chunk*2])<<8)|uuid[chunk*2+1]);if(e)return e;}return 0;
}
A_Err EffectGraphBackup::find(const Record& expected,bool backup,AEGP_EffectRefH& out,A_long& slot) noexcept {
    out=nullptr;A_long count{};auto e=effects_->AEGP_GetLayerNumEffects(layer_,&count);if(e || count<0 || count>4224)return e?e:bad;
    for(A_long i=0;i<count;++i){AEGP_EffectRefH raw{};e=effects_->AEGP_GetLayerEffectByIndex(plugin_,layer_,i,&raw);Ref ref{effects_,raw};if(e || !raw){if(out)effects_->AEGP_DisposeEffect(out);out=nullptr;return e?e:bad;}
        Record r;bool supported{};e=describe(raw,r,supported);double guard{};if(!e && supported)e=scalar(raw,r.guard,guard);
        if(e){if(out)effects_->AEGP_DisposeEffect(out);out=nullptr;return e;}
        if(!supported || r.kind!=expected.kind || (guard==transaction_backup_guard)!=backup)continue;
        std::array<std::uint8_t,16> uuid{};e=identity(raw,r,uuid);if(e){if(out)effects_->AEGP_DisposeEffect(out);out=nullptr;return e;}
        if(r.kind>=0 && uuid!=(backup?expected.backup:expected.original))continue;
        if(out){effects_->AEGP_DisposeEffect(out);out=nullptr;return bad;}out=raw;slot=i;ref.ref=nullptr;
    }return out?0:bad;
}
A_Err EffectGraphBackup::group(A_long slot,AEGP_StreamRefH& out) noexcept {
    out=nullptr;AEGP_StreamRefH root{},parade{};auto e=dynamic_->AEGP_GetNewStreamRefForLayer(plugin_,layer_,&root);Stream r{streams_,root};
    if(e || !root)return e?e:bad;e=dynamic_->AEGP_GetNewStreamRefByMatchname(plugin_,root,"ADBE Effect Parade",&parade);Stream p{streams_,parade};
    if(e || !parade)return e?e:bad;e=dynamic_->AEGP_GetNewStreamRefByIndex(plugin_,parade,slot,&out);return e?e:out?0:bad;
}
A_Err EffectGraphBackup::name(A_long slot,std::array<A_UTF16Char,512>& out) noexcept {
    AEGP_StreamRefH raw{};auto e=group(slot,raw);Stream s{streams_,raw};if(e)return e;
    AEGP_MemHandle handle{};e=streams_->AEGP_GetStreamName(plugin_,raw,FALSE,&handle);
    struct Memory{const AEGP_MemorySuite1* suite;AEGP_MemHandle handle;~Memory(){if(handle)suite->AEGP_FreeMemHandle(handle);}} owned{memory_,handle};
    if(e || !handle)return e?e:bad;AEGP_MemSize bytes{};e=memory_->AEGP_GetMemHandleSize(handle,&bytes);
    if(e || bytes<sizeof(A_UTF16Char) || bytes>out.size()*sizeof(A_UTF16Char) || bytes%sizeof(A_UTF16Char))return e?e:bad;
    void* data{};e=memory_->AEGP_LockMemHandle(handle,&data);if(e || !data)return e?e:bad;
    const auto* text=static_cast<const A_UTF16Char*>(data);const auto units=bytes/sizeof(A_UTF16Char);
    const auto end=std::find(text,text+units,0);const bool terminated=end!=text+units;
    if(terminated){out.fill(0);std::copy(text,end,out.begin());}
    e=memory_->AEGP_UnlockMemHandle(handle);return e?e:terminated?0:bad;
}
A_Err EffectGraphBackup::set_name(A_long slot,const std::array<A_UTF16Char,512>& text) noexcept {
    AEGP_StreamRefH raw{};auto e=group(slot,raw);Stream s{streams_,raw};if(e)return e;
    e=dynamic_->AEGP_SetStreamName(raw,text.data());if(e)return e;
    std::array<A_UTF16Char,512> observed{};e=name(slot,observed);return e?e:observed==text?0:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
A_Err EffectGraphBackup::release_guard() noexcept {
    for(std::size_t i=0;i<count_;++i)if(records_[i].kind<0){AEGP_EffectRefH raw{};A_long slot{};const auto e=find(records_[i],false,raw,slot);
        Ref ref{effects_,raw};return e?e:write_scalar(raw,records_[i].guard,0);}return bad;
}
A_Err EffectGraphBackup::erase(AEGP_EffectRefH effect) noexcept {
    A_long before{},after{};auto e=effects_->AEGP_GetLayerNumEffects(layer_,&before);
    if(e || before<1)return e?e:bad;
    e=effects_->AEGP_DeleteLayerEffect(effect);if(e)return e;
    e=effects_->AEGP_GetLayerNumEffects(layer_,&after);
    return e?e:after==before-1?0:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
A_Err EffectGraphBackup::delete_backups() noexcept {
    A_Err first{};
    for(std::size_t i=count_;i-->0;)if(records_[i].created){AEGP_EffectRefH raw{};A_long slot{};auto e=find(records_[i],true,raw,slot);Ref ref{effects_,raw};
        if(!e)e=erase(raw);if(e){if(!first)first=e;}else records_[i].created=false;}return first;
}
A_Err EffectGraphBackup::prepare(const std::array<std::uint8_t,16>& operation,std::span<const std::array<std::uint8_t,16>> desired_ids) noexcept {
    if(prepared_ || count_ || !basic_ || !plugin_ || !layer_ || !effects_ || !streams_ || !dynamic_ || !memory_ ||
       desired_ids.size()>64 || std::none_of(operation.begin(),operation.end(),[](auto b){return b!=0;}))return bad;
    A_long count{};auto e=effects_->AEGP_GetLayerNumEffects(layer_,&count);if(e || count<1 || count>4096)return e?e:bad;
    unsigned renderers{};
    for(A_long i=0;i<count;++i){AEGP_EffectRefH raw{};e=effects_->AEGP_GetLayerEffectByIndex(plugin_,layer_,i,&raw);Ref ref{effects_,raw};if(e || !raw)return e?e:bad;
        Record record;bool supported{};e=describe(raw,record,supported);if(e)return e;if(!supported)continue;
        if(count_==records_.size())return bad;double guard{};e=scalar(raw,record.guard,guard);if(e || guard!=0)return e?e:bad;
        e=identity(raw,record,record.original);if(e)return e;record.index=i;
        if(record.kind<0)++renderers;else for(std::size_t previous=0;previous<count_;++previous)if(records_[previous].kind>=0 && records_[previous].original==record.original)return bad;
        e=effects_->AEGP_GetEffectFlags(raw,&record.flags);if(e)return e;e=name(i,record.name);if(e)return e;records_[count_++]=record;
    }
    if(renderers!=1)return bad;
    unsigned serial{};
    for(std::size_t i=0;i<count_;++i)if(records_[i].kind>=0){bool unique=false;
        for(unsigned attempts=0;attempts<256 && !unique;++attempts){auto candidate=operation;candidate[0]^=0x80;const auto number=++serial;
            candidate[12]^=static_cast<std::uint8_t>(number>>24);candidate[13]^=static_cast<std::uint8_t>(number>>16);
            candidate[14]^=static_cast<std::uint8_t>(number>>8);candidate[15]^=static_cast<std::uint8_t>(number);
            unique=std::any_of(candidate.begin(),candidate.end(),[](auto b){return b!=0;});
            std::array<std::uint8_t,16> output{};output.back()=0xff;
            if(candidate==output)unique=false;
            for(std::size_t j=0;j<count_;++j)if(records_[j].kind>=0 && (candidate==records_[j].original || candidate==records_[j].backup))unique=false;
            for(const auto& desired:desired_ids)if(candidate==desired)unique=false;
            if(unique)records_[i].backup=candidate;}if(!unique)return bad;}
    // Hold the original renderer before any duplicate can trigger supervision.
    for(std::size_t i=0;i<count_;++i)if(records_[i].kind<0){AEGP_EffectRefH raw{};A_long slot{};e=find(records_[i],false,raw,slot);Ref ref{effects_,raw};
        if(!e)e=write_scalar(raw,records_[i].guard,1);break;}
    if(e){rollback_error_=release_guard();return e;}
    prepared_=true;
    for(std::size_t i=0;i<count_;++i){auto& record=records_[i];AEGP_EffectRefH original{};A_long slot{};e=find(record,false,original,slot);
        AEGP_EffectRefH duplicate{};
        {Ref ref{effects_,original};if(!e)e=effects_->AEGP_DuplicateEffect(original,&duplicate);}
        {Ref ref{effects_,duplicate};if(!e && !duplicate)e=bad;
            Record observed;bool supported{};std::array<std::uint8_t,16> copied{};double copied_guard{};
            if(!e)e=describe(duplicate,observed,supported);
            if(!e && (!supported || observed.kind!=record.kind))e=bad;
            if(!e)e=identity(duplicate,record,copied);
            if(!e && copied!=record.original)e=bad;
            if(!e)e=scalar(duplicate,record.guard,copied_guard);
            if(!e && copied_guard!=(record.kind<0?1:0))e=bad;
            if(!e)e=write_scalar(duplicate,record.guard,transaction_backup_guard);
            if(!e)e=write_identity(duplicate,record,record.backup);
            if(!e)e=effects_->AEGP_SetEffectFlags(duplicate,AEGP_EffectFlags_ACTIVE,0);
            AEGP_EffectFlags flags{};if(!e)e=effects_->AEGP_GetEffectFlags(duplicate,&flags);
            if(!e && flags!=(record.flags&~AEGP_EffectFlags_ACTIVE))e=PF_Err_INTERNAL_STRUCT_DAMAGED;
            if(e && duplicate){const auto removed=erase(duplicate);if(removed)rollback_error_=removed;}
            if(!e)record.created=true;}
        if(e){const auto cleanup=delete_backups();if(cleanup)rollback_error_=cleanup;const auto reset=release_guard();if(reset)rollback_error_=reset;prepared_=false;return e;}
    }return 0;
}
A_Err EffectGraphBackup::discard() noexcept {
    if(!prepared_)return bad;prepared_=false;const auto cleanup=delete_backups();const auto reset=release_guard();return cleanup?cleanup:reset;
}
A_Err EffectGraphBackup::restore() noexcept {
    if(!prepared_)return bad;prepared_=false;
    // Validate every required backup before deleting any surviving author effect.
    for(std::size_t i=0;i<count_;++i){AEGP_EffectRefH raw{};A_long slot{};auto e=find(records_[i],true,raw,slot);Ref ref{effects_,raw};if(e)return rollback_error_=e;}
    A_long count{};auto e=effects_->AEGP_GetLayerNumEffects(layer_,&count);if(e || count<1 || count>4224)return rollback_error_=e?e:bad;
    for(A_long i=count;i-->0;){AEGP_EffectRefH raw{};e=effects_->AEGP_GetLayerEffectByIndex(plugin_,layer_,i,&raw);Ref ref{effects_,raw};if(e || !raw)return rollback_error_=e?e:bad;
        Record record;bool supported{};e=describe(raw,record,supported);if(e)return rollback_error_=e;if(!supported)continue;
        double guard{};e=scalar(raw,record.guard,guard);if(e)return rollback_error_=e;if(guard==transaction_backup_guard)continue;
        e=erase(raw);if(e)return rollback_error_=e;}
    for(std::size_t i=0;i<count_;++i){auto& record=records_[i];AEGP_EffectRefH raw{};A_long slot{};e=find(record,true,raw,slot);Ref ref{effects_,raw};if(e)return rollback_error_=e;
        e=effects_->AEGP_ReorderEffect(raw,record.index);if(e)return rollback_error_=e;
        // Structural edits may invalidate refs; reacquire for position readback.
        effects_->AEGP_DisposeEffect(ref.ref);ref.ref=nullptr;
        AEGP_EffectRefH moved{};A_long observed{};e=find(record,true,moved,observed);Ref position{effects_,moved};
        if(e || observed!=record.index)return rollback_error_=e?e:PF_Err_INTERNAL_STRUCT_DAMAGED;}
    // Rename/retag before activating any node; the renderer is activated last.
    for(std::size_t i=0;i<count_;++i){auto& record=records_[i];AEGP_EffectRefH raw{};A_long slot{};e=find(record,true,raw,slot);Ref ref{effects_,raw};if(e)return rollback_error_=e;
        e=set_name(slot,record.name);if(!e)e=write_identity(raw,record,record.original);
        if(!e)e=effects_->AEGP_SetEffectFlags(raw,AEGP_EffectFlags_ACTIVE,record.flags&AEGP_EffectFlags_ACTIVE);
        AEGP_EffectFlags flags{};if(!e)e=effects_->AEGP_GetEffectFlags(raw,&flags);
        if(!e && flags!=record.flags)e=PF_Err_INTERNAL_STRUCT_DAMAGED;
        if(e)return rollback_error_=e;}
    for(std::size_t i=0;i<count_;++i)if(records_[i].kind>=0){auto& record=records_[i];record.backup=record.original;
        AEGP_EffectRefH raw{};A_long slot{};e=find(record,true,raw,slot);Ref ref{effects_,raw};if(!e)e=write_scalar(raw,record.guard,0);if(e)return rollback_error_=e;record.created=false;}
    for(std::size_t i=0;i<count_;++i)if(records_[i].kind<0){auto& record=records_[i];AEGP_EffectRefH raw{};A_long slot{};e=find(record,true,raw,slot);Ref ref{effects_,raw};
        if(!e)e=write_scalar(raw,record.guard,0);if(e)return rollback_error_=e;record.created=false;}
    return 0;
}
}
