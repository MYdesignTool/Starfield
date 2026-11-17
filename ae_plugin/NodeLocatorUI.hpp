#pragma once
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "AE_EffectCBSuites.h"
#include "adobesdk/DrawbotSuite.h"

namespace starfield::adapter {
// Cosmetic topic area only. No saved parameter, undo entry or render state.
inline PF_Err node_locator_event(PF_InData* data,PF_ParamDef* params[],PF_EventExtra* event,
    native_nodes::Kind kind,A_long index) noexcept {
    if(!data || data->num_params!=native_nodes::parameter_count(kind) || !params || !event || !event->contextH || event->e_type!=PF_Event_DRAW || event->effect_win.index!=index ||
       event->effect_win.area!=PF_EA_PARAM_TITLE || !params[index] || !data->pica_basicP)return PF_Err_NONE;
    const auto module=GetModuleHandleW(L"StarfieldHost.aex");
    using Query=bool(*)(PF_ProgPtr,const std::uint16_t*) noexcept;
    const auto query=module?std::bit_cast<Query>(GetProcAddress(module,"SFLD_IsLocatedNodeV1")):nullptr;
    std::array<std::uint16_t,8> uuid{};
    for(A_long i=0;i<8;++i) {
        const auto p=params[native_nodes::uuid_first_index(kind)+i];
        if(!p || p->param_type!=PF_Param_FLOAT_SLIDER)return PF_Err_NONE;
        const auto value=p->u.fs_d.value;
        if(!std::isfinite(value) || value<0 || value>65535 || std::floor(value)!=value)return PF_Err_NONE;
        uuid[i]=static_cast<std::uint16_t>(value);
    }
    const bool selected=query && query(data->effect_ref,uuid.data());
    struct Suites {
        SPBasicSuite* b;
        const PF_EffectCustomUISuite2* ui{};const DRAWBOT_DrawbotSuite1* bot{};
        const DRAWBOT_SupplierSuite1* supplier{};const DRAWBOT_SurfaceSuite1* surface{};const DRAWBOT_PathSuite1* path{};
        ~Suites(){if(path)b->ReleaseSuite(kDRAWBOT_PathSuite,kDRAWBOT_PathSuite_Version1);
            if(surface)b->ReleaseSuite(kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1);
            if(supplier)b->ReleaseSuite(kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1);
            if(bot)b->ReleaseSuite(kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1);
            if(ui)b->ReleaseSuite(kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2);}
    } s{data->pica_basicP};
    auto get=[&](const char* name,A_long version,auto& suite){return !s.b->AcquireSuite(name,version,reinterpret_cast<const void**>(&suite)) && suite;};
    if(!get(kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2,s.ui) || !get(kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1,s.bot) ||
       !get(kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1,s.supplier) || !get(kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1,s.surface) ||
       !get(kDRAWBOT_PathSuite,kDRAWBOT_PathSuite_Version1,s.path))return PF_Err_NONE;
    DRAWBOT_DrawRef draw{};DRAWBOT_SupplierRef supplier{};DRAWBOT_SurfaceRef surface{};
    if(s.ui->PF_GetDrawingReference(event->contextH,&draw) || !draw)return PF_Err_NONE;
    if(s.bot->GetSupplier(draw,&supplier) || !supplier)return PF_Err_NONE;
    if(s.bot->GetSurface(draw,&surface) || !surface)return PF_Err_NONE;
    const auto& frame=event->effect_win.current_frame;
    DRAWBOT_BrushRef brush{};DRAWBOT_FontRef font{};DRAWBOT_PathRef path{};
    const DRAWBOT_ColorRGBA color=selected?DRAWBOT_ColorRGBA{.2f,.75f,1,1}:DRAWBOT_ColorRGBA{.18f,.18f,.18f,1};
    const DRAWBOT_RectF32 line{float(frame.left),float(frame.top),float(frame.right-frame.left),2};
    auto error=s.supplier->NewBrush(supplier,&color,&brush);
    if(!error)error=s.supplier->NewPath(supplier,&path);
    if(!error)error=s.path->AddRect(path,&line);
    if(!error)error=s.surface->FillPath(surface,brush,path,kDRAWBOT_FillType_EvenOdd);
    if(path)s.supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(path));
    if(brush)s.supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));brush=nullptr;
    const DRAWBOT_ColorRGBA text{.82f,.82f,.82f,1};char16_t label[32]{};
    for(unsigned i=0;i<31 && params[index]->name[i];++i)label[i]=static_cast<unsigned char>(params[index]->name[i]);
    DRAWBOT_Boolean supports_text=false;
    if(!error)error=s.supplier->SupportsText(supplier,&supports_text);
    if(error || !supports_text) {event->evt_out_flags|=PF_EO_HANDLED_EVENT;return static_cast<PF_Err>(error);}
    if(!error)error=s.supplier->NewBrush(supplier,&text,&brush);
    if(!error)error=s.supplier->NewDefaultFont(supplier,11,&font);
    const DRAWBOT_PointF32 at{float(frame.left+2),float(frame.top+16)};
    if(!error)error=s.surface->DrawString(surface,brush,font,reinterpret_cast<const DRAWBOT_UTF16Char*>(label),&at,
        kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_End,float(frame.right-frame.left-4));
    if(font)s.supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));
    if(brush)s.supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
    event->evt_out_flags|=PF_EO_HANDLED_EVENT;return static_cast<PF_Err>(error);
}
}
