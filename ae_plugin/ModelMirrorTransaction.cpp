#include "ModelMirrorTransaction.hpp"
#include "NodeRecord.hpp"
#include "SPBasic.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>

namespace starfield::adapter {
namespace {
PF_Err host_error(core::CoreError error) noexcept{return error.code==core::ErrorCode::allocation_failed?PF_Err_OUT_OF_MEMORY:
    error.code==core::ErrorCode::cancelled?PF_Interrupt_CANCEL:PF_Err_BAD_CALLBACK_PARAM;}
bool same_bounds(const core::ModelBounds& a,const core::ModelBounds& b) noexcept {
    return a.minimum.x==b.minimum.x&&a.minimum.y==b.minimum.y&&a.minimum.z==b.minimum.z&&
        a.maximum.x==b.maximum.x&&a.maximum.y==b.maximum.y&&a.maximum.z==b.maximum.z;
}
core::NeverCancelled never;
}
struct ModelMirrorTransaction::Impl {
    struct Change {AEGP_StreamRefH ref{};AEGP_StreamValue2 previous{};PF_ArbitraryH replacement{};bool read{},changed{};};
    PF_InData* data;AEGP_PluginID id;AEGP_EffectRefH renderer;AEGP_LayerH layer;
    const AEGP_StreamSuite6* streams{};const AEGP_EffectSuite4* effects{};const AEGP_PFInterfaceSuite1* pf{};
    std::vector<Change> changes;bool accepted{},installed{};
    Impl(PF_InData* d,AEGP_PluginID p,AEGP_EffectRefH r,AEGP_LayerH l):data(d),id(p),renderer(r),layer(l){}
    ~Impl(){if(!accepted)(void)rollback();for(auto& c:changes){if(c.read)streams->AEGP_DisposeStreamValue(&c.previous);
        if(c.ref)streams->AEGP_DisposeStream(c.ref);if(c.replacement)data->utils->host_dispose_handle(c.replacement);}
        if(!data||!data->pica_basicP)return;
        if(pf)data->pica_basicP->ReleaseSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
        if(effects)data->pica_basicP->ReleaseSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
        if(streams)data->pica_basicP->ReleaseSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6);}
    PF_Err rollback() noexcept {
        PF_Err error{};if(accepted)return error;
        for(auto it=changes.rbegin();it!=changes.rend();++it)if(it->changed) {
            const auto e=streams->AEGP_SetStreamValue(id,it->ref,&it->previous);
            if(e){error=static_cast<PF_Err>(e);continue;}
            AEGP_StreamValue2 check{};const A_Time time{data->current_time,data->time_scale};
            auto result=streams->AEGP_GetNewStreamValue(id,it->ref,AEGP_LTimeMode_LayerTime,&time,1,&check);
            if(!result){
                if(it->replacement) {
                    PF_ArbParamsExtra compare{};compare.id=kModelMirrorFirstDiskId;compare.which_function=PF_Arbitrary_COMPARE_FUNC;
                    PF_ArbCompareResult equal{};compare.u.compare_func_params.a_arbH=reinterpret_cast<PF_ArbitraryH>(it->previous.val.arbH);
                    compare.u.compare_func_params.b_arbH=reinterpret_cast<PF_ArbitraryH>(check.val.arbH);compare.u.compare_func_params.compareP=&equal;
                    result=model_mirror_arbitrary_callback(data,&compare);if(!result&&equal!=PF_ArbCompare_EQUAL)result=PF_Err_INTERNAL_STRUCT_DAMAGED;
                } else if(check.val.one_d!=it->previous.val.one_d)result=PF_Err_INTERNAL_STRUCT_DAMAGED;
                const auto disposed=streams->AEGP_DisposeStreamValue(&check);if(!result)result=disposed;
            }
            if(result)error=static_cast<PF_Err>(result);else it->changed=false;
        }
        return error;
    }
    PF_Err open(A_long index,AEGP_StreamType expected,Change*& output) {
        changes.emplace_back();output=&changes.back();auto& c=*output;
        auto e=streams->AEGP_GetNewEffectStreamByIndex(id,renderer,index,&c.ref);
        AEGP_StreamType type{};
        if(!e&&!c.ref)e=PF_Err_BAD_CALLBACK_PARAM;
        if(!e)e=streams->AEGP_GetStreamType(c.ref,&type);
        if(!e&&type!=expected)e=PF_Err_BAD_CALLBACK_PARAM;
        const A_Time time{data->current_time,data->time_scale};
        if(!e)e=streams->AEGP_GetNewStreamValue(id,c.ref,AEGP_LTimeMode_LayerTime,&time,1,&c.previous);
        if(!e)c.read=true;return static_cast<PF_Err>(e);
    }
    PF_Err source(const ModelMirrorRecipe& recipe,ModelMirrorValue& value) {
        if(!layer){auto e=pf->AEGP_GetEffectLayer(data->effect_ref,&layer);if(e||!layer)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);}
        A_long count{};auto e=effects->AEGP_GetLayerNumEffects(layer,&count);if(e)return static_cast<PF_Err>(e);
        bool found=false;const A_Time time{data->current_time,data->time_scale};
        for(A_long i=0;i<count;++i) {
            AEGP_EffectRefH effect{};e=effects->AEGP_GetLayerEffectByIndex(id,layer,i,&effect);if(e||!effect)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
            struct DisposeEffect{const AEGP_EffectSuite4* suite;AEGP_EffectRefH ref;~DisposeEffect(){suite->AEGP_DisposeEffect(ref);}} effect_scope{effects,effect};
            AEGP_InstalledEffectKey key{};char name[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            e=effects->AEGP_GetInstalledKeyFromLayerEffect(effect,&key);if(!e)e=effects->AEGP_GetEffectMatchName(key,name);if(e)return static_cast<PF_Err>(e);
            if(std::strcmp(name,"org.starfieldfx.node.model"))continue;
            auto read=[&](A_long index,AEGP_StreamType expected,AEGP_StreamValue2& result)->PF_Err {
                AEGP_StreamRefH ref{};auto err=streams->AEGP_GetNewEffectStreamByIndex(id,effect,index,&ref);
                if(err||!ref)return static_cast<PF_Err>(err?err:PF_Err_BAD_CALLBACK_PARAM);
                AEGP_StreamType type{};err=streams->AEGP_GetStreamType(ref,&type);
                if(!err&&type!=expected)err=PF_Err_BAD_CALLBACK_PARAM;
                if(!err)err=streams->AEGP_GetNewStreamValue(id,ref,AEGP_LTimeMode_LayerTime,&time,1,&result);
                if(err)streams->AEGP_DisposeStream(ref);return static_cast<PF_Err>(err);
            };
            const auto release_value=[&](AEGP_StreamValue2& value)->A_Err {
                const auto ref=value.streamH;const auto disposed=streams->AEGP_DisposeStreamValue(&value);
                const auto closed=streams->AEGP_DisposeStream(ref);return disposed?disposed:closed;
            };
            core::NodeId node;
            for(A_long chunk=0;chunk<8;++chunk){AEGP_StreamValue2 chunk_value{};e=read(native_nodes::uuid_first_index(native_nodes::Kind::model)+chunk,AEGP_StreamType_OneD,chunk_value);
                if(e)return static_cast<PF_Err>(e);const auto number=chunk_value.val.one_d;const auto disposed=release_value(chunk_value);
                if(disposed)return static_cast<PF_Err>(disposed);
                if(!std::isfinite(number)||number<0||number>65535||std::floor(number)!=number)return PF_Err_BAD_CALLBACK_PARAM;
                const auto bits=static_cast<std::uint16_t>(number);node.value.bytes[chunk*2]=static_cast<std::uint8_t>(bits>>8);node.value.bytes[chunk*2+1]=static_cast<std::uint8_t>(bits);}
            if(node!=recipe.node)continue;if(found)return PF_Err_BAD_CALLBACK_PARAM;found=true;
            for(auto [index,desired]:{std::pair{native_nodes::model_layout::source,2.0},std::pair{native_nodes::model_layout::revision,double(recipe.revision)}}) {
                AEGP_StreamValue2 control{};e=read(index,AEGP_StreamType_OneD,control);if(e)return static_cast<PF_Err>(e);
                const auto number=control.val.one_d;const auto disposed=release_value(control);
                if(disposed||number!=desired)return static_cast<PF_Err>(disposed?disposed:PF_Err_BAD_CALLBACK_PARAM);
            }
            AEGP_StreamValue2 mesh{};e=read(native_nodes::model_layout::mesh,AEGP_StreamType_ARB,mesh);if(e)return static_cast<PF_Err>(e);
            auto decoded=read_model_geometry_parameter(data,reinterpret_cast<PF_ArbitraryH>(mesh.val.arbH),never);
            const auto disposed=release_value(mesh);if(!decoded.has_value())return host_error(decoded.error());
            if(disposed)return static_cast<PF_Err>(disposed);
            if(!same_bounds(decoded.value().bounds,recipe.bounds))return PF_Err_BAD_CALLBACK_PARAM;
            value={node.value.bytes,recipe.revision,decoded.take_value()};
        }
        return found?PF_Err_NONE:PF_Err_BAD_CALLBACK_PARAM;
    }
};
ModelMirrorTransaction::ModelMirrorTransaction(PF_InData* d,AEGP_PluginID id,AEGP_EffectRefH r,AEGP_LayerH l):impl_(std::make_unique<Impl>(d,id,r,l)){}
ModelMirrorTransaction::~ModelMirrorTransaction()=default;
void ModelMirrorTransaction::accept() noexcept{impl_->accepted=true;}
PF_Err ModelMirrorTransaction::rollback() noexcept{return impl_->rollback();}
PF_Err ModelMirrorTransaction::install(const core::Graph& graph,A_long* failed_stream,const char** failed_stage) noexcept {
    const auto stage=[&](const char* text,A_long index=-1){if(failed_stage)*failed_stage=text;if(failed_stream)*failed_stream=index;};
    auto& tx=*impl_;if(tx.installed)return PF_Err_BAD_CALLBACK_PARAM;tx.installed=true;
    const auto perform=[&]()->PF_Err {
        stage("decode Model mirror recipes");auto recipes=model_mirror_recipes(graph);if(!recipes.has_value())return host_error(recipes.error());
        if(!tx.data||!tx.data->pica_basicP||!tx.id||!tx.renderer||!tx.data->time_scale)return PF_Err_BAD_CALLBACK_PARAM;
        auto* basic=tx.data->pica_basicP;stage("acquire Model mirror suites");
        auto e=basic->AcquireSuite(kAEGPStreamSuite,kAEGPStreamSuiteVersion6,reinterpret_cast<const void**>(&tx.streams));if(e||!tx.streams)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
        tx.changes.reserve(kModelMirrorCapacity+1);Impl::Change* count{};stage("read Model mirror count",kModelMirrorCountIndex);
        auto error=tx.open(kModelMirrorCountIndex,AEGP_StreamType_OneD,count);if(error)return error;
        const auto old=count->previous.val.one_d;
        if(!std::isfinite(old)||old<0||old>kModelMirrorCapacity||std::floor(old)!=old)return PF_Err_BAD_CALLBACK_PARAM;
        if(!recipes.value().empty()) {
            e=basic->AcquireSuite(kAEGPEffectSuite,kAEGPEffectSuiteVersion4,reinterpret_cast<const void**>(&tx.effects));if(e||!tx.effects)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);
            if(!tx.layer){e=basic->AcquireSuite(kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1,reinterpret_cast<const void**>(&tx.pf));if(e||!tx.pf)return static_cast<PF_Err>(e?e:PF_Err_BAD_CALLBACK_PARAM);}
        }
        for(A_long slot=0;slot<std::max<A_long>(static_cast<A_long>(old),static_cast<A_long>(recipes.value().size()));++slot) {
            Impl::Change* change{};const A_long index=kModelMirrorFirstIndex+slot;stage("read Model mirror slot",index);
            error=tx.open(index,AEGP_StreamType_ARB,change);if(error)return error;
            auto previous=read_model_mirror_parameter(tx.data,reinterpret_cast<PF_ArbitraryH>(change->previous.val.arbH),never);
            if(!previous.has_value())return host_error(previous.error());ModelMirrorValue desired;
            if(slot<static_cast<A_long>(recipes.value().size())) {
                const auto& recipe=recipes.value()[slot];
                if(previous.value().id==recipe.node.value.bytes&&previous.value().revision==recipe.revision&&same_bounds(previous.value().geometry.bounds,recipe.bounds))continue;
                stage("read Model author mesh",native_nodes::model_layout::mesh);error=tx.source(recipe,desired);if(error)return error;
            } else if(previous.value().id==core::ModelResourceId{})continue;
            stage("prepare Model mirror slot",index);error=create_model_mirror_parameter(tx.data,desired,&change->replacement,never);if(error)return error;
            AEGP_StreamValue2 value{};value.streamH=change->ref;value.val.arbH=reinterpret_cast<AEGP_ArbBlockVal>(change->replacement);
            change->changed=true;stage("write Model mirror slot",index);e=tx.streams->AEGP_SetStreamValue(tx.id,change->ref,&value);if(e)return static_cast<PF_Err>(e);
            AEGP_StreamValue2 written{};const A_Time time{tx.data->current_time,tx.data->time_scale};stage("verify Model mirror slot",index);
            e=tx.streams->AEGP_GetNewStreamValue(tx.id,change->ref,AEGP_LTimeMode_LayerTime,&time,1,&written);if(e)return static_cast<PF_Err>(e);
            PF_ArbParamsExtra compare{};compare.id=kModelMirrorFirstDiskId;compare.which_function=PF_Arbitrary_COMPARE_FUNC;
            PF_ArbCompareResult equal{};compare.u.compare_func_params.a_arbH=change->replacement;
            compare.u.compare_func_params.b_arbH=reinterpret_cast<PF_ArbitraryH>(written.val.arbH);compare.u.compare_func_params.compareP=&equal;
            error=model_mirror_arbitrary_callback(tx.data,&compare);const auto disposed=tx.streams->AEGP_DisposeStreamValue(&written);
            if(error||disposed||equal!=PF_ArbCompare_EQUAL)return error?error:static_cast<PF_Err>(disposed?disposed:PF_Err_INTERNAL_STRUCT_DAMAGED);
        }
        count=&tx.changes.front();if(old!=recipes.value().size()) {
            AEGP_StreamValue2 value{};value.streamH=count->ref;value.val.one_d=double(recipes.value().size());count->changed=true;
            stage("write Model mirror count",kModelMirrorCountIndex);e=tx.streams->AEGP_SetStreamValue(tx.id,count->ref,&value);if(e)return static_cast<PF_Err>(e);
            AEGP_StreamValue2 written{};const A_Time time{tx.data->current_time,tx.data->time_scale};stage("verify Model mirror count",kModelMirrorCountIndex);
            e=tx.streams->AEGP_GetNewStreamValue(tx.id,count->ref,AEGP_LTimeMode_LayerTime,&time,1,&written);if(e)return static_cast<PF_Err>(e);
            const bool equal=written.val.one_d==value.val.one_d;const auto disposed=tx.streams->AEGP_DisposeStreamValue(&written);
            if(disposed||!equal)return static_cast<PF_Err>(disposed?disposed:PF_Err_INTERNAL_STRUCT_DAMAGED);
        }
        return PF_Err_NONE;
    };
    PF_Err error{};try{error=perform();}catch(const std::bad_alloc&){error=PF_Err_OUT_OF_MEMORY;}catch(...){error=PF_Err_INTERNAL_STRUCT_DAMAGED;}
    if(error){const auto restored=tx.rollback();if(restored){stage("Model mirror rollback failed");return PF_Err_INTERNAL_STRUCT_DAMAGED;}}
    return error;
}
}
