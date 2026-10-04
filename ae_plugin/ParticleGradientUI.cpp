#include "ParticleGradientUI.hpp"
#include "GradientEditorModel.hpp"
#include "ParticleLayout.hpp"
#include "NodeEffects.hpp"
#include "AE_EffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "SPBasic.h"
#include <adobesdk/DrawbotSuite.h>
#include <array>
#include <map>
#include <optional>
#include <cstdio>

namespace starfield::adapter {
namespace {
namespace model=gradient_editor;
namespace layout=native_nodes::particle_layout;
struct UIState {int selected{};bool presets{};bool bitmap_disabled{};};
// Opaque context keys are never dereferenced or passed to a later SDK callback.
// CLOSE_CONTEXT erases them; selection has no effect on authored values.
std::map<PF_ContextH,UIState> contexts;
std::optional<core::ColorGradient> clipboard;
template<class T> struct Suite {
    SPBasicSuite* basic;const char* name;A_long version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v) {
        if(basic)(void)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));
    }
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->() const{return value;}
    explicit operator bool() const{return value!=nullptr;}
};
bool read(PF_ParamDef* params[],core::ColorGradient& value) noexcept {
    if(!params || !params[layout::gradient] || params[layout::gradient]->param_type!=PF_Param_FLOAT_SLIDER)return false;
    const auto count=params[layout::gradient]->u.fs_d.value;
    if(!std::isfinite(count) || count<2 || count>8 || std::floor(count)!=count)return false;
    value.count=static_cast<std::uint8_t>(count);
    for(unsigned i=0;i<value.count;++i) {
        const auto* position=params[layout::gradient_first+2*i];const auto* color=params[layout::gradient_first+2*i+1];
        if(!position || !color || position->param_type!=PF_Param_FLOAT_SLIDER || color->param_type!=PF_Param_COLOR)return false;
        const auto rgb=color->u.cd.value;
        value.stops[i]={position->u.fs_d.value/100,{rgb.red/255.0,rgb.green/255.0,rgb.blue/255.0}};
    }
    return model::valid(value);
}
PF_Err publish(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],const core::ColorGradient& value) noexcept {
    if(!model::valid(value))return PF_Err_BAD_CALLBACK_PARAM;
    std::array<PF_ParamDef,17> previous{};
    for(A_long i=0;i<17;++i) {
        if(!params[layout::gradient+i])return PF_Err_BAD_CALLBACK_PARAM;
        previous[i]=*params[layout::gradient+i];
    }
    params[layout::gradient]->u.fs_d.value=value.count;
    params[layout::gradient]->uu.change_flags=PF_ChangeFlag_CHANGED_VALUE;
    for(unsigned i=0;i<value.count;++i) {
        auto* position=params[layout::gradient_first+2*i];auto* color=params[layout::gradient_first+2*i+1];
        position->u.fs_d.value=value.stops[i].position*100;
        const auto channel=[](double c){return static_cast<A_u_char>(std::lround(c*255));};
        const auto& rgb=value.stops[i].color;
        color->u.cd.value={255,channel(rgb.x),channel(rgb.y),channel(rgb.z)};
        position->uu.change_flags=color->uu.change_flags=PF_ChangeFlag_CHANGED_VALUE;
    }
    PF_UserChangedParamExtra changed{};changed.param_index=layout::gradient;
    // All bank components belong to one direct publication. The renderer must
    // not read a new count with old AEGP colors before this PF event returns.
    const auto error=sync_node_graph_parameter(data,out,params,&changed,true);
    if(error) {for(A_long i=0;i<17;++i)*params[layout::gradient+i]=previous[i];return error;}
    out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;return PF_Err_NONE;
}
struct Canvas {
    Suite<PF_EffectCustomUISuite2> ui;
    Suite<DRAWBOT_DrawbotSuite1> bot;
    Suite<DRAWBOT_SupplierSuite1> supplier;
    Suite<DRAWBOT_SurfaceSuite1> surface;
    Suite<DRAWBOT_PathSuite1> paths;
    DRAWBOT_SupplierRef source{};DRAWBOT_SurfaceRef target{};DRAWBOT_FontRef font{};
    explicit Canvas(PF_InData* data,PF_ContextH context):
        ui(data->pica_basicP,kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2),
        bot(data->pica_basicP,kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1),
        supplier(data->pica_basicP,kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1),
        surface(data->pica_basicP,kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1),
        paths(data->pica_basicP,kDRAWBOT_PathSuite,kDRAWBOT_PathSuite_Version1) {
        DRAWBOT_DrawRef ref{};
        if(ui && bot && supplier && surface && paths && !ui->PF_GetDrawingReference(context,&ref) && ref &&
            !bot->GetSupplier(ref,&source) && !bot->GetSurface(ref,&target) && source && target)
            (void)supplier->NewDefaultFont(source,11,&font);
    }
    ~Canvas(){if(font)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));}
    explicit operator bool() const{return source && target && font;}
    void rect(float x,float y,float width,float height,const DRAWBOT_ColorRGBA& color) const {
        DRAWBOT_BrushRef brush{};DRAWBOT_PathRef path{};
        if(!supplier->NewBrush(source,&color,&brush) && brush && !supplier->NewPath(source,&path) && path) {
            const DRAWBOT_RectF32 bounds{x,y,width,height};
            if(!paths->AddRect(path,&bounds))(void)surface->FillPath(target,brush,path,kDRAWBOT_FillType_EvenOdd);
        }
        if(path)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(path));
        if(brush)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
    }
    void text(float x,float y,const char* label,bool enabled=true) const {
        std::array<DRAWBOT_UTF16Char,96> string{};
        for(unsigned i=0;label[i] && i+1<string.size();++i)string[i]=static_cast<DRAWBOT_UTF16Char>(label[i]);
        const DRAWBOT_ColorRGBA color{enabled?0.9f:0.5f,enabled?0.9f:0.5f,enabled?0.9f:0.5f,1};
        DRAWBOT_BrushRef brush{};const DRAWBOT_PointF32 origin{x,y};
        if(!supplier->NewBrush(source,&color,&brush) && brush) {
            (void)surface->DrawString(target,brush,font,string.data(),&origin,
                kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_None,260);
            supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
        }
    }
    void button(float x,float y,float width,const char* label,bool enabled=true) const {
        rect(x,y,width,21,{0.43f,0.43f,0.45f,1});rect(x+1,y+1,width-2,19,{0.22f,0.22f,0.23f,1});
        text(x+6,y+14,label,enabled);
    }
    std::optional<model::PixelOrder> pixel_order() const {
        DRAWBOT_Boolean bgra{},argb{},prefer_bgra{},prefer_argb{};
        // A successful support query is required before creating an image. Do
        // not probe layouts with NewImageFromBuffer: AE can show a modal warning.
        const bool has_bgra=supplier->SupportsPixelLayoutBGRA &&
            !supplier->SupportsPixelLayoutBGRA(source,&bgra) && bgra;
        const bool has_argb=supplier->SupportsPixelLayoutARGB &&
            !supplier->SupportsPixelLayoutARGB(source,&argb) && argb;
        if(has_bgra && supplier->PrefersPixelLayoutBGRA &&
           supplier->PrefersPixelLayoutBGRA(source,&prefer_bgra))prefer_bgra=false;
        if(has_argb && supplier->PrefersPixelLayoutARGB &&
           supplier->PrefersPixelLayoutARGB(source,&prefer_argb))prefer_argb=false;
        if(has_bgra && (prefer_bgra || !has_argb || !prefer_argb))return model::PixelOrder::bgra;
        if(has_argb)return model::PixelOrder::argb;
        return std::nullopt;
    }
    bool bitmap(float x,float y,unsigned width,const core::ColorGradient& value,UIState& state) const {
        if(state.bitmap_disabled)return false;
        const auto order=pixel_order();
        if(!order || !supplier->NewImageFromBuffer || !surface->DrawImage) {
            state.bitmap_disabled=true;return false;
        }
        std::array<std::uint8_t,220*62*4> pixels{};
        const unsigned stride=width*4;
        const auto size=std::size_t(stride)*62;
        if(!model::rasterize_opaque32(value,width,62,*order,std::span{pixels.data(),size},stride))return false;
        const auto format=*order==model::PixelOrder::bgra?kDRAWBOT_PixelLayout_32BGRA_Premul:kDRAWBOT_PixelLayout_32ARGB_Premul;
        DRAWBOT_ImageRef image{};const DRAWBOT_PointF32 origin{x,y};
        // Set the guard before calling the host: even a reentrant repaint while
        // an unexpected modal warning is open must not attempt another image.
        state.bitmap_disabled=true;
        const auto created=supplier->NewImageFromBuffer(source,width,62,stride,format,pixels.data(),&image);
        const bool drawn=!created && image && !surface->DrawImage(target,image,&origin,1);
        if(image)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(image));
        // An unexpected driver failure is attempted once per UI context, so a
        // warning cannot become an unbounded loop of repaint/dialog/repaint.
        state.bitmap_disabled=!drawn;
        return drawn;
    }
    void gradient(float x,float y,unsigned width,const core::ColorGradient& value,UIState& state) const {
        if(bitmap(x,y,width,value,state))return;
        // Optional bitmap support must not remove the editor. Overlap integer
        // bands on an opaque base to avoid uncovered antialiased strip edges.
        const auto first=value.stops[0].color;
        rect(x,y,static_cast<float>(width),62,{static_cast<float>(first.x),static_cast<float>(first.y),static_cast<float>(first.z),1});
        for(unsigned column=0;column<width;column+=2) {
            const auto rgb=core::evaluate_color_gradient(value,double(column)/(width-1));
            rect(x+column,y,static_cast<float>(std::min(3U,width-column)),62,
                {static_cast<float>(rgb.x),static_cast<float>(rgb.y),static_cast<float>(rgb.z),1});
        }
        const auto last=value.stops[value.count-1].color;
        rect(x+width-1,y,1,62,{static_cast<float>(last.x),static_cast<float>(last.y),static_cast<float>(last.z),1});
    }
};
struct Bounds {
    float x,y,width;
    explicit Bounds(const PF_EffectWindowInfo& info):x(static_cast<float>(info.current_frame.left)+8),
        y(static_cast<float>(info.current_frame.top)+5),
        width(std::clamp(static_cast<float>(info.current_frame.right-info.current_frame.left)-82,100.0f,220.0f)){}
    double position(A_long horizontal) const {return (horizontal-x)/width;}
    bool inside(A_long h,A_long v,float dx,float dy,float w,float height) const {
        return h>=x+dx && h<x+dx+w && v>=y+dy && v<y+dy+height;
    }
};
void draw(PF_InData* data,PF_EventExtra* event,const core::ColorGradient& value,UIState& state) {
    Canvas canvas(data,event->contextH);if(!canvas)return;
    const Bounds b(event->effect_win);
    canvas.rect(b.x,b.y,b.width+69,171,{0.18f,0.18f,0.19f,1});
    canvas.gradient(b.x,b.y,static_cast<unsigned>(b.width),value,state);
    for(unsigned i=0;i<value.count;++i) {
        const float x=b.x+static_cast<float>(value.stops[i].position)*b.width;
        const auto& c=value.stops[i].color;
        canvas.rect(x-5,b.y+61,10,21,state.selected==static_cast<int>(i)?DRAWBOT_ColorRGBA{0.9f,0.92f,1,1}:DRAWBOT_ColorRGBA{0.6f,0.6f,0.6f,1});
        canvas.rect(x-3,b.y+63,6,17,{static_cast<float>(c.x),static_cast<float>(c.y),static_cast<float>(c.z),1});
    }
    canvas.text(b.x+b.width+10,b.y+14,"Linear");
    canvas.button(b.x+b.width+8,b.y+29,59,"Flip");
    canvas.button(b.x,b.y+92,58,"Copy");canvas.button(b.x+65,b.y+92,58,"Paste",clipboard.has_value());
    canvas.button(b.x+130,b.y+92,70,"Presets");
    if(state.presets) {canvas.button(b.x,b.y+119,58,"White");canvas.button(b.x+65,b.y+119,58,"Fire");canvas.button(b.x+130,b.y+119,70,"Spectrum");}
    char label[80]{};std::snprintf(label,sizeof(label),"Stop %d: %.1f%%",state.selected+1,value.stops[state.selected].position*100);
    canvas.text(b.x,b.y+154,label);canvas.text(b.x,b.y+169,"Double-click: color; Alt-click: delete");
}
void invalidate(PF_InData* data,PF_EventExtra* event) {
    Suite<PFAppSuite6> app(data->pica_basicP,kPFAppSuite,kPFAppSuiteVersion6);
    if(app && app->PF_InvalidateRect)(void)app->PF_InvalidateRect(event->contextH,nullptr);
    event->evt_out_flags|=PF_EO_UPDATE_NOW|PF_EO_HANDLED_EVENT;
}
bool pick(PF_InData* data,core::Vec3& rgb) {
    Suite<PFAppSuite6> app(data->pica_basicP,kPFAppSuite,kPFAppSuiteVersion6);
    if(!app || !app->PF_AppColorPickerDialog)return false;
    PF_PixelFloat sample{1,static_cast<float>(rgb.x),static_cast<float>(rgb.y),static_cast<float>(rgb.z)},chosen{};
    if(app->PF_AppColorPickerDialog("Particle gradient color",&sample,TRUE,&chosen))return false;
    for(float c:{chosen.red,chosen.green,chosen.blue})if(!std::isfinite(c))return false;
    rgb={std::clamp(double(chosen.red),0.0,1.0),std::clamp(double(chosen.green),0.0,1.0),std::clamp(double(chosen.blue),0.0,1.0)};
    return true;
}
}
void clear_particle_gradient_ui() noexcept {contexts.clear();clipboard.reset();}
PF_Err particle_gradient_param_ui(PF_InData* data,PF_ParamDef* params[]) noexcept {
    return update_native_particle_visibility(data,params);
}
PF_Err particle_gradient_event(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],PF_EventExtra* event) noexcept try {
    if(!event || !data || !out)return PF_Err_NONE;
    if(event->e_type==PF_Event_CLOSE_CONTEXT){contexts.erase(event->contextH);return PF_Err_NONE;}
    if(!event->contextH || event->effect_win.index!=layout::gradient || event->effect_win.area!=PF_EA_CONTROL)return PF_Err_NONE;
    core::ColorGradient value;if(!read(params,value))return PF_Err_NONE;
    if(contexts.size()>=64 && !contexts.contains(event->contextH))contexts.erase(contexts.begin());
    auto& state=contexts[event->contextH];state.selected=std::clamp(state.selected,0,int(value.count)-1);
    if(event->e_type==PF_Event_DRAW){draw(data,event,value,state);event->evt_out_flags|=PF_EO_HANDLED_EVENT;return PF_Err_NONE;}
    bool changed=false;const Bounds b(event->effect_win);
    if(event->e_type==PF_Event_DO_CLICK) {
        auto& click=event->u.do_click;const auto h=click.screen_point.h,v=click.screen_point.v;
        if(b.inside(h,v,b.width+8,29,59,21)){model::flip(value);state.selected=int(value.count)-1-state.selected;changed=true;}
        else if(b.inside(h,v,0,92,58,21))clipboard=value;
        else if(b.inside(h,v,65,92,58,21) && clipboard){value=*clipboard;state.selected=0;changed=true;}
        else if(b.inside(h,v,130,92,70,21))state.presets=!state.presets;
        else if(state.presets && b.inside(h,v,0,119,200,21)) {
            value=model::preset(h<b.x+65?0:h<b.x+130?1:2);state.selected=0;state.presets=false;changed=true;
        } else if(b.inside(h,v,-6,0,b.width+12,84)) {
            int nearest=-1;double distance=9;
            for(unsigned i=0;i<value.count;++i) {
                const double delta=std::abs(h-(b.x+b.width*value.stops[i].position));
                if(delta<distance){distance=delta;nearest=static_cast<int>(i);}
            }
            if(nearest>=0 && v>=b.y+58) {
                state.selected=nearest;
                if(click.modifiers&PF_Mod_OPT_ALT_KEY){changed=model::erase(value,nearest);state.selected=0;}
                else if(click.num_clicks>=2)changed=pick(data,value.stops[nearest].color);
                else {click.send_drag=TRUE;click.continue_refcon[0]=nearest+1;}
            } else if(v<b.y+62){const auto inserted=model::insert(value,b.position(h));if(inserted>=0){state.selected=inserted;changed=true;}}
        } else return PF_Err_NONE;
    } else if(event->e_type==PF_Event_DRAG) {
        const auto index=event->u.do_click.continue_refcon[0]-1;
        if(index<0 || index>=value.count)return PF_Err_NONE;
        const auto next=model::move(value,static_cast<int>(index),b.position(event->u.do_click.screen_point.h));
        if(next>=0){state.selected=next;event->u.do_click.continue_refcon[0]=next+1;changed=true;}
    } else if(event->e_type==PF_Event_KEYDOWN) {
        const auto code=PF_KEYCODE_GET_CONTROL_CODE(event->u.key_down.keycode);
        if(code!=PF_ControlCode_Delete && code!=PF_ControlCode_Backspace)return PF_Err_NONE;
        changed=model::erase(value,state.selected);state.selected=0;
    } else return PF_Err_NONE;
    if(changed) {const auto error=publish(data,out,params,value);if(error)return error;}
    invalidate(data,event);return PF_Err_NONE;
} catch(...) {return PF_Err_NONE;} // Optional UI cannot crash or reject host loading.
}
