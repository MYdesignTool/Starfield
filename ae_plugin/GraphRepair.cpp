#include "GraphRepairRequest.hpp"
#include "NativeGraphCommit.hpp"
#include "NodeRecord.hpp"
#include "Parameters.hpp"
#include "UiExclusionClient.hpp"
#include "SPBasic.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <utility>

namespace starfield::adapter {
namespace {
constexpr A_Err bad=PF_Err_BAD_CALLBACK_PARAM;
template<class T>struct Suite {
    SPBasicSuite* basic;const char* name;A_long version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v){basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
struct Repair {
    AEGP_PluginID id;const AEGP_EffectSuite4* effects;const AEGP_StreamSuite6* streams;
    AEGP_EffectRefH renderer{};
    ~Repair(){close();}
    void close(){if(renderer)effects->AEGP_DisposeEffect(renderer);renderer=nullptr;}
    A_Err scalar(AEGP_EffectRefH effect,A_long index,double& value,bool write=false){
        AEGP_StreamRefH raw{};auto error=streams->AEGP_GetNewEffectStreamByIndex(id,effect,index,&raw);
        struct Ref{const AEGP_StreamSuite6* s;AEGP_StreamRefH r;~Ref(){if(r)s->AEGP_DisposeStream(r);}} ref{streams,raw};
        if(error || !raw)return error?error:bad;
        AEGP_StreamType type{};error=streams->AEGP_GetStreamType(raw,&type);if(error || type!=AEGP_StreamType_OneD)return error?error:bad;
        if(write){AEGP_StreamValue2 v{};v.streamH=raw;v.val.one_d=value;error=streams->AEGP_SetStreamValue(id,raw,&v);if(error)return error;}
        AEGP_StreamValue2 read{};const A_Time zero{0,1};error=streams->AEGP_GetNewStreamValue(id,raw,AEGP_LTimeMode_LayerTime,&zero,TRUE,&read);
        if(error)return error;const double observed=read.val.one_d;streams->AEGP_DisposeStreamValue(&read);
        if(!std::isfinite(observed) || (write && observed!=value))return bad;value=observed;return 0;
    }
    A_Err put(A_long index,double value){return scalar(renderer,index,value,true);}
    A_Err locate(AEGP_LayerH layer,bool idle_authors){
        close();A_long count{};auto error=effects->AEGP_GetLayerNumEffects(layer,&count);
        if(error || count<1 || count>4224)return error?error:bad;
        for(A_long i=0;i<count;++i){AEGP_EffectRefH fx{};error=effects->AEGP_GetLayerEffectByIndex(id,layer,i,&fx);
            struct Ref{const AEGP_EffectSuite4* s;AEGP_EffectRefH& r;~Ref(){if(r)s->AEGP_DisposeEffect(r);}} ref{effects,fx};
            if(error || !fx)return error?error:bad;
            AEGP_InstalledEffectKey key{};char match[AEGP_MAX_EFFECT_MATCH_NAME_SIZE]{};
            error=effects->AEGP_GetInstalledKeyFromLayerEffect(fx,&key);if(!error)error=effects->AEGP_GetEffectMatchName(key,match);if(error)return error;
            if(!std::strcmp(match,"org.starfieldfx.particle")){if(renderer)return bad;renderer=fx;fx=nullptr;continue;}
            if(!idle_authors)continue;
            // Unknown Starfield node layouts are never repaired by guessing.
            if(std::strncmp(match,"org.starfieldfx.node.",21))continue;
            A_long guard=-1;
            for(auto kind:{native_nodes::Kind::emitter,native_nodes::Kind::particle,native_nodes::Kind::force,native_nodes::Kind::transform,
                native_nodes::Kind::model,native_nodes::Kind::material,native_nodes::Kind::motion,native_nodes::Kind::turbulence}) {
                const char* suffix=kind==native_nodes::Kind::emitter?"emitter":kind==native_nodes::Kind::particle?"particle":
                    kind==native_nodes::Kind::force?"force":kind==native_nodes::Kind::transform?"transform":kind==native_nodes::Kind::model?"model":
                    kind==native_nodes::Kind::material?"material":kind==native_nodes::Kind::motion?"motion":"turbulence";
                if(!std::strcmp(match+21,suffix))guard=native_nodes::sync_guard_index(kind);
            }
            double value{};if(guard<0)return bad;error=scalar(fx,guard,value);if(error || value!=0)return error?error:bad;
        }
        return renderer?0:bad;
    }
};
}
PF_Err repair_selected_graph(PF_InData* data,GraphRepairRequest& request,PF_OutData* output) noexcept try {
    const char* stage="preflight";char detail[256]{};
    const auto report=[&](A_Err error){std::snprintf(request.message,sizeof(request.message),
        error?"Starfield repair failed: %s (error %d). %.120s Use Undo to revert if a backup was created.":
              "Starfield graph repaired on the selected layer. Disabled full-layer backup retained. Refresh the panel. Undo once to revert.",stage,int(error),detail);
        if(error && output)std::snprintf(output->return_msg,sizeof(output->return_msg),"%s",request.message);return static_cast<PF_Err>(error);};
    if(!data || !data->utils || !data->pica_basicP || !request.plugin || !request.layer || !ui_transaction_active())return report(bad);
    auto* basic=data->pica_basicP;
    Suite<AEGP_EffectSuite4> effects(basic,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_StreamSuite6> streams(basic,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_LayerSuite9> layers(basic,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_UtilitySuite6> utility(basic,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!effects.value || !streams.value || !layers.value || !utility.value)return report(bad);
    Repair tx{request.plugin,effects.value,streams.value};
    auto error=tx.locate(request.layer,true);if(error)return report(error);
    double guard{},nonce{},receipt{},ready{},revision{};
    if((error=tx.scalar(tx.renderer,kGraphSyncGuardId,guard)) || (error=tx.scalar(tx.renderer,kGraphEditCommitId,nonce)) ||
       (error=tx.scalar(tx.renderer,kGraphEditReceiptId,receipt)) || (error=tx.scalar(tx.renderer,kNodeEffectsReadyId,ready)) ||
       (error=tx.scalar(tx.renderer,kGraphRevisionId,revision)))return report(error);
    if(guard!=1 || ready!=1 || nonce<1 || nonce>1000000 || std::floor(nonce)!=nonce || receipt!=-nonce)return report(bad);
    AEGP_EffectFlags flags{};error=effects.value->AEGP_GetEffectFlags(tx.renderer,&flags);if(error)return report(error);
    tx.close();stage="start undo";error=utility.value->AEGP_StartUndoGroup("Starfield: repair selected graph");if(error)return report(error);
    struct Undo{const AEGP_UtilitySuite6* s;bool open{true};~Undo(){if(open)s->AEGP_EndUndoGroup();}} undo{utility.value};
    stage="backup original layer";AEGP_LayerH backup{};error=layers.value->AEGP_DuplicateLayer(request.layer,&backup);
    if(error || !backup)return report(error?error:bad);
    error=layers.value->AEGP_SetLayerFlag(backup,AEGP_LayerFlag_VIDEO_ACTIVE,FALSE);
    const auto mute=layers.value->AEGP_SetLayerFlag(backup,AEGP_LayerFlag_AUDIO_ACTIVE,FALSE);
    if(!error)error=mute;
    if(!error)error=layers.value->AEGP_SetLayerName(backup,reinterpret_cast<const A_UTF16Char*>(u"Starfield repair backup"));
    AEGP_LayerFlags backup_flags{};if(!error)error=layers.value->AEGP_GetLayerFlags(backup,&backup_flags);
    if(!error && (backup_flags&(AEGP_LayerFlag_VIDEO_ACTIVE|AEGP_LayerFlag_AUDIO_ACTIVE)))error=bad;
    if(!error)error=tx.locate(backup,false);if(!error)error=tx.put(kGraphSyncGuardId,2);tx.close();
    if(error)return report(error);
    stage="revalidate original";error=tx.locate(request.layer,true);
    double observed{};
    for(const auto [index,expected]:{std::pair<A_long,double>{kGraphSyncGuardId,1},{kGraphEditCommitId,nonce},
        {kGraphEditReceiptId,receipt},{kNodeEffectsReadyId,1},{kGraphRevisionId,revision}}){
        if(!error)error=tx.scalar(tx.renderer,index,observed);if(!error && observed!=expected)error=bad;
    }
    if(error)return report(error);
    stage="disable original renderer";
    error=effects.value->AEGP_SetEffectFlags(tx.renderer,AEGP_EffectFlags_ACTIVE,AEGP_EffectFlags_NONE);
    AEGP_EffectFlags disabled{};if(!error)error=effects.value->AEGP_GetEffectFlags(tx.renderer,&disabled);
    if(!error && (disabled&AEGP_EffectFlags_ACTIVE))error=bad;
    const auto next=static_cast<A_long>(nonce==1000000?1:nonce+1);
    if(!error){stage="publish derived graph";error=tx.put(kGraphEditCommitId,next);}
    if(!error)error=commit_panel_graph(data,tx.renderer,request.layer,next,request.plugin,output,true);
    if(error && output)std::snprintf(detail,sizeof(detail),"%s",output->return_msg);
    if(!error)error=tx.scalar(tx.renderer,kGraphEditReceiptId,observed);if(!error && observed!=next)error=bad;
    if(!error)error=tx.scalar(tx.renderer,kGraphRevisionId,observed);if(!error && observed<=revision)error=bad;
    if(error){
        // Keep a terminal rejected request, including a partially written nonce.
        // The publisher owns derived-state rollback; never declare it committed here.
        const auto restored=tx.put(kGraphEditCommitId,nonce);
        const auto rejected=tx.put(kGraphEditReceiptId,receipt);
        if(restored || rejected)std::snprintf(detail,sizeof(detail),"Request rollback failed (%d / %d).",int(restored),int(rejected));
    }
    if(!error)stage="restore renderer flags";
    const auto enabled=effects.value->AEGP_SetEffectFlags(tx.renderer,AEGP_EffectFlags_ACTIVE,flags&AEGP_EffectFlags_ACTIVE);
    if(!error)error=enabled;
    AEGP_EffectFlags restored_flags{};const auto read_flags=effects.value->AEGP_GetEffectFlags(tx.renderer,&restored_flags);
    if(!error && (read_flags || restored_flags!=flags))error=read_flags?read_flags:bad;
    if(!error){stage="release original guard";error=tx.put(kGraphSyncGuardId,0);}
    if(error){(void)tx.put(kGraphSyncGuardId,1);}
    tx.close();
    if(!error){stage="verify completed repair";error=tx.locate(request.layer,true);
        if(!error)error=tx.scalar(tx.renderer,kGraphSyncGuardId,observed);if(!error && observed!=0)error=bad;
        if(!error)error=tx.scalar(tx.renderer,kGraphEditReceiptId,observed);if(!error && observed!=next)error=bad;}
    if(error && tx.renderer)(void)tx.put(kGraphSyncGuardId,1);
    tx.close();if(!error)stage="close undo";
    const auto closed=utility.value->AEGP_EndUndoGroup();undo.open=false;
    if(!error)error=closed;
    // No writes outside this single undo group. A rejected host undo closure is
    // reported without pretending the already verified publication was rolled back.
    return report(error);
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
