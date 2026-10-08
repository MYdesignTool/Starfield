#include "TransformNullUI.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "AE_EffectCBSuites.h"
#include "adobesdk/DrawbotSuite.h"
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

namespace starfield::adapter {
namespace {
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;A_long version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v) {
        if(basic)(void)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));
    }
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->()const{return value;}
    explicit operator bool()const{return value!=nullptr;}
};
constexpr auto kind=native_nodes::Kind::transform;
constexpr A_long guard_index=native_nodes::sync_guard_index(kind);
struct Button {
    A_long x,y,width,height{21};
    explicit Button(const PF_EffectWindowInfo& info):x(info.current_frame.left+2),
        y(info.current_frame.top+3),width(std::clamp<A_long>(info.current_frame.right-x-2,0,114)){}
    bool hit(A_long h,A_long v)const {return width>=40 && h>=x && h<x+width && v>=y && v<y+height;}
};
PF_Err draw_button(PF_InData* data,PF_EventExtra* event) {
    Suite<PF_EffectCustomUISuite2> ui(data->pica_basicP,kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2);
    Suite<DRAWBOT_DrawbotSuite1> bot(data->pica_basicP,kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1);
    Suite<DRAWBOT_SupplierSuite1> supplier(data->pica_basicP,kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1);
    Suite<DRAWBOT_SurfaceSuite1> surface(data->pica_basicP,kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1);
    Suite<DRAWBOT_PathSuite1> paths(data->pica_basicP,kDRAWBOT_PathSuite,kDRAWBOT_PathSuite_Version1);
    if(!ui || !bot || !supplier || !surface || !paths)return PF_Err_BAD_CALLBACK_PARAM;
    DRAWBOT_DrawRef ref{};DRAWBOT_SupplierRef source{};DRAWBOT_SurfaceRef target{};
    auto error=ui->PF_GetDrawingReference(event->contextH,&ref);
    if(!error)error=bot->GetSupplier(ref,&source);
    if(!error)error=bot->GetSurface(ref,&target);
    if(error || !source || !target)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    const Button b(event->effect_win);
    if(b.width<40)return PF_Err_NONE;
    for(unsigned border=0;border<2 && !error;++border) {
        const DRAWBOT_ColorRGBA color=border?DRAWBOT_ColorRGBA{.22f,.22f,.23f,1}:DRAWBOT_ColorRGBA{.43f,.43f,.45f,1};
        const DRAWBOT_RectF32 bounds{float(b.x+border),float(b.y+border),float(b.width-2*border),float(b.height-2*border)};
        DRAWBOT_BrushRef brush{};DRAWBOT_PathRef path{};
        error=supplier->NewBrush(source,&color,&brush);
        if(!error)error=supplier->NewPath(source,&path);
        if(!error)error=paths->AddRect(path,&bounds);
        if(!error)error=surface->FillPath(target,brush,path,kDRAWBOT_FillType_EvenOdd);
        if(path)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(path));
        if(brush)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
    }
    DRAWBOT_FontRef font{};DRAWBOT_BrushRef brush{};const DRAWBOT_ColorRGBA color{.9f,.9f,.9f,1};
    if(!error)error=supplier->NewDefaultFont(source,11,&font);
    if(!error)error=supplier->NewBrush(source,&color,&brush);
    const DRAWBOT_PointF32 at{float(b.x+7),float(b.y+14)};
    if(!error)error=surface->DrawString(target,brush,font,reinterpret_cast<const DRAWBOT_UTF16Char*>(u"Create Null"),
        &at,kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_None,float(b.width-12));
    if(brush)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
    if(font)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));
    return static_cast<PF_Err>(error);
}
}

PF_Err create_transform_null(PF_InData* data,PF_OutData* out,PF_ParamDef* params[]) noexcept try {
    const auto plugin=node_graph_sync_plugin_id();
    if(!data || !out || !data->pica_basicP || !data->effect_ref || !plugin || !params || !params[1] ||
       params[1]->param_type!=PF_Param_LAYER || !params[guard_index] ||
       params[guard_index]->param_type!=PF_Param_FLOAT_SLIDER || params[guard_index]->u.fs_d.value!=0 || !data->time_scale)
        return PF_Err_BAD_CALLBACK_PARAM;
    Suite<AEGP_PFInterfaceSuite1> pf(data->pica_basicP,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_LayerSuite9> layers(data->pica_basicP,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(data->pica_basicP,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_StreamSuite6> streams(data->pica_basicP,kAEGPStreamSuite,kAEGPStreamSuiteVersion6);
    Suite<AEGP_EffectSuite4> effects(data->pica_basicP,kAEGPEffectSuite,kAEGPEffectSuiteVersion4);
    Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!pf || !layers || !comps || !streams || !effects || !utility)return PF_Err_BAD_CALLBACK_PARAM;
    struct Refs {
        const AEGP_StreamSuite6* streams;const AEGP_EffectSuite4* effects;
        AEGP_EffectRefH effect{};AEGP_StreamRefH source{},guard{};
        ~Refs(){if(source)streams->AEGP_DisposeStream(source);if(guard)streams->AEGP_DisposeStream(guard);
            if(effect)effects->AEGP_DisposeEffect(effect);}
    } refs{streams.value,effects.value};
    AEGP_LayerH owner{};AEGP_CompH comp{};A_long layer_count{};
    A_Err error=pf->AEGP_GetEffectLayer(data->effect_ref,&owner);
    if(!error)error=layers->AEGP_GetLayerParentComp(owner,&comp);
    if(!error)error=layers->AEGP_GetCompNumLayers(comp,&layer_count);
    if(error || !owner || !comp || layer_count<0 || layer_count>=4096)return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    error=pf->AEGP_GetNewEffectForEffect(plugin,data->effect_ref,&refs.effect);
    if(!error)error=streams->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,1,&refs.source);
    if(!error)error=streams->AEGP_GetNewEffectStreamByIndex(plugin,refs.effect,guard_index,&refs.guard);
    AEGP_StreamType source_type{},guard_type{};
    if(!error)error=streams->AEGP_GetStreamType(refs.source,&source_type);
    if(!error)error=streams->AEGP_GetStreamType(refs.guard,&guard_type);
    if(error || source_type!=AEGP_StreamType_LAYER_ID || guard_type!=AEGP_StreamType_OneD)
        return error?static_cast<PF_Err>(error):PF_Err_BAD_CALLBACK_PARAM;
    const A_Time time{data->current_time,data->time_scale};AEGP_StreamValue2 previous{};
    error=streams->AEGP_GetNewStreamValue(plugin,refs.source,AEGP_LTimeMode_LayerTime,&time,TRUE,&previous);
    if(error)return static_cast<PF_Err>(error);
    const auto previous_id=previous.val.layer_id;streams->AEGP_DisposeStreamValue(&previous);
    error=streams->AEGP_GetNewStreamValue(plugin,refs.guard,AEGP_LTimeMode_LayerTime,&time,TRUE,&previous);
    if(error)return static_cast<PF_Err>(error);
    const auto previous_guard=previous.val.one_d;streams->AEGP_DisposeStreamValue(&previous);
    if(previous_guard!=0)return PF_Err_BAD_CALLBACK_PARAM;
    error=utility->AEGP_StartUndoGroup("Starfield: Create Transform Null");
    if(error)return static_cast<PF_Err>(error);
    struct Undo {const AEGP_UtilitySuite6* utility;~Undo(){utility->AEGP_EndUndoGroup();}} undo{utility.value};
    const auto guard=[&](double value) {
        AEGP_StreamValue2 v{};v.streamH=refs.guard;v.val.one_d=value;
        const auto e=streams->AEGP_SetStreamValue(plugin,refs.guard,&v);
        if(!e)params[guard_index]->u.fs_d.value=value;
        return e;
    };
    const auto select=[&](AEGP_LayerIDVal id) {
        AEGP_StreamValue2 v{};v.streamH=refs.source;v.val.layer_id=id;
        return streams->AEGP_SetStreamValue(plugin,refs.source,&v);
    };
    AEGP_LayerH created{};const char* stage="create Null";
    error=comps->AEGP_CreateNullInComp(reinterpret_cast<const A_UTF16Char*>(u"Starfield Transform Null"),comp,nullptr,&created);
    if(!error && !created)error=PF_Err_BAD_CALLBACK_PARAM;
    if(!error){stage="enable 3D";error=layers->AEGP_SetLayerFlag(created,AEGP_LayerFlag_LAYER_IS_3D,TRUE);}
    AEGP_LayerIDVal id{};
    if(!error){stage="read Null ID";error=layers->AEGP_GetLayerID(created,&id);if(!error && id<=0)error=PF_Err_BAD_CALLBACK_PARAM;}
    if(!error){
        stage="name Null";
        std::array<char,96> name{};std::array<A_UTF16Char,96> utf{};
        std::snprintf(name.data(),name.size(),"Starfield Transform Null %ld",static_cast<long>(id));
        for(std::size_t i=0;name[i] && i+1<utf.size();++i)utf[i]=static_cast<A_UTF16Char>(name[i]);
        error=layers->AEGP_SetLayerName(created,utf.data());
    }
    if(!error){stage="guard selection";error=guard(1);}
    if(!error){stage="reference Null";error=select(id);}
    if(!error){stage="release selection guard";error=guard(0);}
    if(!error){
        stage="publish Null binding";PF_UserChangedParamExtra changed{};changed.param_index=1;
        error=sync_node_graph_parameter(data,out,params,&changed);
    }
    if(error) {
        A_Err rollback=guard(1);const auto restore=select(previous_id);if(!rollback)rollback=restore;
        if(created){const auto remove=layers->AEGP_DeleteLayer(created);if(!rollback)rollback=remove;}
        const auto reset=guard(previous_guard);if(!rollback)rollback=reset;
        if(!out->return_msg[0] || rollback)std::snprintf(out->return_msg,sizeof(out->return_msg),
            "Starfield Create Null failed: %s (error %ld, rollback %ld).",stage,static_cast<long>(error),static_cast<long>(rollback));
        return static_cast<PF_Err>(error);
    }
    out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;return PF_Err_NONE;
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err transform_null_event(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],PF_EventExtra* event) noexcept try {
    if(!event || !data || !out || !event->contextH || event->effect_win.index!=1 || event->effect_win.area!=PF_EA_CONTROL)
        return PF_Err_NONE;
    PF_Err error=PF_Err_NONE;
    if(event->e_type==PF_Event_DRAW)error=draw_button(data,event);
    else if(event->e_type==PF_Event_DO_CLICK && Button(event->effect_win).hit(event->u.do_click.screen_point.h,event->u.do_click.screen_point.v))
        error=create_transform_null(data,out,params);
    else return PF_Err_NONE;
    event->evt_out_flags|=PF_EO_HANDLED_EVENT;return error;
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
