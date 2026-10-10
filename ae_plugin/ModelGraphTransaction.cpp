#include "ModelGraphTransaction.hpp"
#include "NodeRecord.hpp"
#include "starfield/core/ModelResources.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace starfield::adapter {
namespace {
constexpr A_Err bad=PF_Err_BAD_CALLBACK_PARAM;
namespace layout=native_nodes::model_layout;
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){if(basic)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct Ref {const AEGP_EffectSuite4* suite;AEGP_EffectRefH value;~Ref(){if(value)suite->AEGP_DisposeEffect(value);}};
struct Stream {const AEGP_StreamSuite6* suite;AEGP_StreamRefH value;~Stream(){if(value)suite->AEGP_DisposeStream(value);}};
struct Cancellation final:core::Cancellation {
    const ModelGraphTransactionPlan& plan;
    explicit Cancellation(const ModelGraphTransactionPlan& p):plan(p){}
    bool is_cancelled()const noexcept override{return plan.is_cancelled && plan.is_cancelled(plan.context)!=0;}
};
bool valid_id(const ModelTransactionId& id) noexcept {
    ModelTransactionId output{};output.back()=0xff;
    return id!=output && std::any_of(id.begin(),id.end(),[](auto b){return b!=0;});
}
A_Err scalar(const AEGP_StreamSuite6* streams,AEGP_PluginID plugin,AEGP_EffectRefH effect,A_long index,const A_Time& time,double& out) noexcept {
    AEGP_StreamRefH raw{};auto e=streams->AEGP_GetNewEffectStreamByIndex(plugin,effect,index,&raw);Stream ref{streams,raw};
    if(e || !raw)return e?e:bad;
    AEGP_StreamType type{};e=streams->AEGP_GetStreamType(raw,&type);if(e || type!=AEGP_StreamType_OneD)return e?e:bad;
    AEGP_StreamValue2 value{};e=streams->AEGP_GetNewStreamValue(plugin,raw,AEGP_LTimeMode_LayerTime,&time,TRUE,&value);
    if(e)return e;out=value.val.one_d;e=streams->AEGP_DisposeStreamValue(&value);return e?e:std::isfinite(out)?0:bad;
}
A_Err find_model(const ModelGraphTransactionPlan& plan,const ModelAssetRestore& asset,
    const AEGP_EffectSuite4* effects,const AEGP_StreamSuite6* streams,AEGP_EffectRefH& out) noexcept {
    out=nullptr;A_long count{};auto e=effects->AEGP_GetLayerNumEffects(plan.layer,&count);
    if(e || count<1 || count>4224)return e?e:bad;
    Ref found{effects,nullptr};
    for(A_long i=0;i<count;++i){AEGP_EffectRefH raw{};e=effects->AEGP_GetLayerEffectByIndex(plan.plugin,plan.layer,i,&raw);Ref candidate{effects,raw};
        if(e || !raw)return e?e:bad;
        AEGP_InstalledEffectKey key{};char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
        e=effects->AEGP_GetInstalledKeyFromLayerEffect(raw,&key);if(!e)e=effects->AEGP_GetEffectMatchName(key,match);if(e)return e;
        if(std::strcmp(match,"org.starfieldfx.node.model"))continue;
        double guard{};e=scalar(streams,plan.plugin,raw,native_nodes::sync_guard_index(native_nodes::Kind::model),plan.time,guard);if(e)return e;
        if(guard==transaction_backup_guard)continue;
        ModelTransactionId id{};
        for(A_long part=0;part<8;++part){double word{};
            e=scalar(streams,plan.plugin,raw,native_nodes::uuid_first_index(native_nodes::Kind::model)+part,plan.time,word);
            if(e || word<0 || word>65535 || std::floor(word)!=word)return e?e:bad;
            const auto value=static_cast<unsigned>(word);id[part*2]=static_cast<std::uint8_t>(value>>8);id[part*2+1]=static_cast<std::uint8_t>(value);}
        if(id!=asset.node_id)continue;
        if(guard!=0 || found.value)return bad;
        found.value=raw;candidate.value=nullptr;
    }
    if(!found.value)return bad;out=found.value;found.value=nullptr;return 0;
}
A_Err write_asset(const ModelGraphTransactionPlan& plan,const ModelAssetRestore& asset,
    const AEGP_EffectSuite4* effects,const AEGP_StreamSuite6* streams,ModelGraphTransactionResult& result) noexcept {
    AEGP_EffectRefH raw{};auto e=find_model(plan,asset,effects,streams,raw);Ref ref{effects,raw};if(e)return e;
    double source{},revision{};e=scalar(streams,plan.plugin,raw,layout::source,plan.time,source);
    if(!e)e=scalar(streams,plan.plugin,raw,layout::revision,plan.time,revision);
    if(e || (source!=1 && source!=2) || revision<0 || revision>2147483647 || std::floor(revision)!=revision)return e?e:bad;
    ModelAssetWriteRequest request;
    std::copy(asset.node_id.begin(),asset.node_id.end(),std::begin(request.expected_uuid));
    request.expected_source=static_cast<std::uint32_t>(source);request.expected_revision=static_cast<std::uint32_t>(revision);
    request.desired_source=asset.source;request.desired_revision=asset.revision;
    request.mesh_bytes=asset.mesh.empty()?nullptr:asset.mesh.data();request.mesh_length=static_cast<std::uint32_t>(asset.mesh.size());
    std::copy(asset.bounds.begin(),asset.bounds.end(),std::begin(request.desired_bounds));
    request.cancellation_context=plan.context;request.is_cancelled=plan.is_cancelled;
    e=effects->AEGP_EffectCallGeneric(plan.plugin,raw,&plan.time,PF_Cmd_COMPLETELY_GENERAL,&request);
    result.asset_error=e?ModelAssetError::host_error:request.acknowledged!=1?ModelAssetError::unavailable:request.error;
    if(request.rollback_error)result.asset_rollback_error=request.rollback_error;
    if(e)return e;
    if(result.asset_error!=ModelAssetError::none)return request.host_error?request.host_error:
        result.asset_error==ModelAssetError::cancelled?PF_Interrupt_CANCEL:bad;
    // The private native writer readbacks mesh/bounds and all metadata itself;
    // verify the two constant controls again before advancing to another asset.
    e=scalar(streams,plan.plugin,raw,layout::source,plan.time,source);
    if(!e)e=scalar(streams,plan.plugin,raw,layout::revision,plan.time,revision);
    return e?e:source==asset.source && revision==asset.revision?0:PF_Err_INTERNAL_STRUCT_DAMAGED;
}
A_Err preflight(const ModelGraphTransactionPlan& plan,ModelGraphTransactionResult& result,const Cancellation& cancel) noexcept {
    if(!plan.basic || !plan.plugin || !plan.layer || !plan.time.scale || !plan.prepare || !plan.commit || !valid_id(plan.id) ||
       plan.desired_ids.size()>64 || plan.assets.size()>63)return bad;
    for(std::size_t i=0;i<plan.desired_ids.size();++i){
        if(std::none_of(plan.desired_ids[i].begin(),plan.desired_ids[i].end(),[](auto b){return b!=0;}))return bad;
        for(std::size_t j=0;j<i;++j)if(plan.desired_ids[i]==plan.desired_ids[j])return bad;}
    std::size_t total{};
    for(std::size_t i=0;i<plan.assets.size();++i){result.asset_index=static_cast<std::int32_t>(i);const auto& asset=plan.assets[i];
        result.asset_error=ModelAssetError::invalid_request;
        if(!valid_id(asset.node_id) || std::find(plan.desired_ids.begin(),plan.desired_ids.end(),asset.node_id)==plan.desired_ids.end() ||
           (asset.source!=1 && asset.source!=2) || asset.revision>2147483647u || asset.mesh.size()>core::kMaxModelEncodedBytes ||
           (asset.revision==0?!asset.mesh.empty():asset.mesh.size()<32) || asset.mesh.size()>64u*1024u*1024u-total)return bad;
        for(std::size_t j=0;j<i;++j)if(plan.assets[j].node_id==asset.node_id)return bad;
        for(unsigned axis=0;axis<6;++axis)if(!std::isfinite(asset.bounds[axis]) || std::abs(asset.bounds[axis])>1e9 ||
            (axis>=3 && asset.bounds[axis]<asset.bounds[axis-3]))return bad;
        total+=asset.mesh.size();
    }
    for(std::size_t i=0;i<plan.assets.size();++i){result.asset_index=static_cast<std::int32_t>(i);const auto& asset=plan.assets[i];
        if(cancel.is_cancelled()){result.asset_error=ModelAssetError::cancelled;return PF_Interrupt_CANCEL;}
        auto mesh=asset.revision?core::decode_model_geometry({reinterpret_cast<const std::byte*>(asset.mesh.data()),asset.mesh.size()},cancel):core::make_unit_cube();
        if(!mesh.has_value()){result.asset_error=mesh.error().code==core::ErrorCode::cancelled?ModelAssetError::cancelled:
            mesh.error().code==core::ErrorCode::allocation_failed?ModelAssetError::allocation_failed:ModelAssetError::invalid_geometry;
            return result.asset_error==ModelAssetError::cancelled?PF_Interrupt_CANCEL:result.asset_error==ModelAssetError::allocation_failed?PF_Err_OUT_OF_MEMORY:bad;}
        const auto& box=mesh.value().bounds;const std::array<double,6> bounds{box.minimum.x,box.minimum.y,box.minimum.z,box.maximum.x,box.maximum.y,box.maximum.z};
        if(bounds!=asset.bounds){result.asset_error=ModelAssetError::invalid_geometry;return bad;}
    }
    result.asset_index=-1;result.asset_error=ModelAssetError::none;
    return cancel.is_cancelled()?PF_Interrupt_CANCEL:0;
}
}
ModelGraphTransactionResult apply_model_graph_transaction(const ModelGraphTransactionPlan& plan) noexcept {
    ModelGraphTransactionResult result;Cancellation cancel(plan);
    result.error=preflight(plan,result,cancel);if(result.error)return result;
    Suite<AEGP_UtilitySuite6> utility(plan.basic,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    Suite<AEGP_EffectSuite4> effects(plan.basic,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(plan.basic,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    if(!utility.value || !effects.value || !streams.value){result.error=bad;return result;}
    result.stage=ModelTransactionStage::undo;result.error=utility.value->AEGP_StartUndoGroup("Starfield: apply graph and Model assets");if(result.error)return result;
    {
        EffectGraphBackup backup(plan.basic,plan.plugin,plan.layer);
        result.stage=ModelTransactionStage::backup;result.error=backup.prepare(plan.id,plan.desired_ids);
        if(result.error)result.rollback_error=backup.rollback_error();
        else {
            try {
                result.stage=ModelTransactionStage::prepare;
                result.error=cancel.is_cancelled()?PF_Interrupt_CANCEL:plan.prepare(plan.context);
                if(!result.error){result.stage=ModelTransactionStage::assets;
                    for(std::size_t i=0;i<plan.assets.size();++i){result.asset_index=static_cast<std::int32_t>(i);
                        result.error=cancel.is_cancelled()?PF_Interrupt_CANCEL:write_asset(plan,plan.assets[i],effects.value,streams.value,result);
                        if(result.error)break;}}
                if(!result.error){result.stage=ModelTransactionStage::commit;
                    result.error=cancel.is_cancelled()?PF_Interrupt_CANCEL:plan.commit(plan.context);}
            } catch(const std::bad_alloc&){result.error=PF_Err_OUT_OF_MEMORY;}catch(...){result.error=PF_Err_INTERNAL_STRUCT_DAMAGED;}
            if(result.error){const auto restored=backup.restore();if(restored)result.rollback_error=restored;}
            else {result.committed=true;result.stage=ModelTransactionStage::complete;result.asset_index=-1;result.cleanup_error=backup.discard();}
        }
    }
    result.undo_error=utility.value->AEGP_EndUndoGroup();return result;
}
}
