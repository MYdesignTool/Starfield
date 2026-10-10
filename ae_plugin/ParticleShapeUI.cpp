#include "ParticleShapeUI.hpp"
#include "ParticleShape.hpp"
#include "ParticleLayout.hpp"
#include "NodeRecord.hpp"
#include "NodeEffects.hpp"
#include "UiExclusionClient.hpp"
#include "AE_EffectCBSuites.h"
#include "adobesdk/DrawbotSuite.h"
#include <algorithm>

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
struct Button {
    A_long x,y,width,height{21};
    explicit Button(const PF_EffectWindowInfo& info):x(info.current_frame.left+2),y(info.current_frame.top+3),
        width(std::clamp<A_long>(info.current_frame.right-x-2,0,230)){}
    bool hit(A_long h,A_long v)const{return width>=40 && h>=x && h<x+width && v>=y && v<y+height;}
};
PF_Err draw(PF_InData* data,PF_EventExtra* event,A_long selected) {
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
    const Button b(event->effect_win);if(b.width<40)return PF_Err_NONE;
    const char16_t* label=u"Unsupported Shape";
    for(const auto& choice:particle_shapes::choices)if(static_cast<A_long>(choice.native)==selected){label=choice.label;break;}
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
    const DRAWBOT_PointF32 at{float(b.x+7),float(b.y+14)},arrow{float(b.x+b.width-18),float(b.y+14)};
    if(!error)error=surface->DrawString(target,brush,font,reinterpret_cast<const DRAWBOT_UTF16Char*>(label),
        &at,kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_End,float(b.width-28));
    if(!error)error=surface->DrawString(target,brush,font,reinterpret_cast<const DRAWBOT_UTF16Char*>(u"\u25be"),
        &arrow,kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_None,14);
    if(brush)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
    if(font)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));
    return static_cast<PF_Err>(error);
}
}
PF_Err particle_shape_event(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],PF_EventExtra* event) noexcept try {
    namespace layout=native_nodes::particle_layout;
    constexpr auto guard=native_nodes::sync_guard_index(native_nodes::Kind::particle);
    if(!data || !out || !params || !event || !event->contextH || event->effect_win.index!=layout::shape ||
       event->effect_win.area!=PF_EA_CONTROL || !params[layout::shape] ||
       params[layout::shape]->param_type!=PF_Param_POPUP)return PF_Err_NONE;
    if(event->e_type==PF_Event_DRAW) {
        const auto error=draw(data,event,params[layout::shape]->u.pd.value);
        event->evt_out_flags|=PF_EO_HANDLED_EVENT;return error;
    }
    if(event->e_type!=PF_Event_DO_CLICK && event->e_type!=PF_Event_DRAG)return PF_Err_NONE;
    if(!params[guard] || params[guard]->param_type!=PF_Param_FLOAT_SLIDER ||
       params[guard]->u.fs_d.value!=0)return PF_Err_NONE;
    EffectUiExclusion exclusion;if(!exclusion)return PF_Err_NONE;
    auto& click=event->u.do_click;
    if(event->e_type==PF_Event_DO_CLICK) {
        click.continue_refcon[0]=0;
        if(!Button(event->effect_win).hit(click.screen_point.h,click.screen_point.v))return PF_Err_NONE;
        std::uint32_t selected{};
        if(choose_particle_shape(data,params[layout::shape]->u.pd.value,selected) &&
           selected!=static_cast<std::uint32_t>(params[layout::shape]->u.pd.value)) {
            std::uint32_t core{};if(!particle_shapes::native_to_core(selected,core))return PF_Err_BAD_CALLBACK_PARAM;
            // SDK change_flags is valid in DRAG. Keep only numbers between events.
            click.continue_refcon[0]=selected;click.continue_refcon[1]=params[layout::shape]->u.pd.value;
            click.send_drag=TRUE;
        }
    } else {
        const auto selected=click.continue_refcon[0];if(!selected)return PF_Err_NONE;
        click.continue_refcon[0]=0;
        std::uint32_t core{};
        if(!particle_shapes::native_to_core(static_cast<double>(selected),core) ||
           params[layout::shape]->u.pd.value!=click.continue_refcon[1])return PF_Err_BAD_CALLBACK_PARAM;
        const auto previous=*params[layout::shape];
        params[layout::shape]->u.pd.value=static_cast<A_long>(selected);
        params[layout::shape]->uu.change_flags|=PF_ChangeFlag_CHANGED_VALUE;
        PF_UserChangedParamExtra changed{};changed.param_index=layout::shape;
        const auto error=sync_node_graph_parameter(data,out,params,&changed);
        if(error){*params[layout::shape]=previous;return error;}
        out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;
    }
    event->evt_out_flags|=PF_EO_HANDLED_EVENT|PF_EO_UPDATE_NOW;return PF_Err_NONE;
}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
