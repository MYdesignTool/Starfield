#include "AEConfig.h"
#include "AE_EffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "SPBasic.h"
#include "NodeEffects.hpp"
#include "ParticleGradientUI.hpp"
#include "ParticleLayout.hpp"
#include "GradientEditorModel.hpp"
#include <adobesdk/DrawbotSuite.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <limits>
#include <fstream>
#include <vector>

namespace {
namespace model=starfield::adapter::gradient_editor;
namespace layout=starfield::adapter::native_nodes::particle_layout;
int checks{},failures{},publications{},invalidations{},gets{},releases{},objects{},rectangles{},texts{},ui_updates{},bitmap_draws{};
bool reject{},cancel{},drawing_fail{},bitmap_fail{};
std::vector<std::uint8_t> bitmap_pixels;
int bitmap_width{},bitmap_height{};
std::ofstream preview;
DRAWBOT_ColorRGBA preview_color{};
DRAWBOT_RectF32 preview_rect{};
void check(bool value,const char* message){++checks;if(!value){++failures;std::printf("FAILED: %s\n",message);}}
SPBasicSuite basic{};PFAppSuite6 app{};PF_EffectCustomUISuite2 custom{};PF_ParamUtilsSuite3 utility{};
DRAWBOT_DrawbotSuite1 bot{};DRAWBOT_SupplierSuite1 supplier{};DRAWBOT_SurfaceSuite1 surface{};DRAWBOT_PathSuite1 path{};
template<class T> T reference(){return reinterpret_cast<T>(std::uintptr_t{16});}
void configure() {
    basic.AcquireSuite=[](const char* name,int32,const void** value)->SPErr {
        ++gets;*value=nullptr;
        if(!std::strcmp(name,kPFAppSuite))*value=&app;
        else if(!std::strcmp(name,kPFEffectCustomUISuite))*value=&custom;
        else if(!std::strcmp(name,kDRAWBOT_DrawSuite))*value=&bot;
        else if(!std::strcmp(name,kDRAWBOT_SupplierSuite))*value=&supplier;
        else if(!std::strcmp(name,kDRAWBOT_SurfaceSuite))*value=&surface;
        else if(!std::strcmp(name,kDRAWBOT_PathSuite))*value=&path;
        else if(!std::strcmp(name,kPFParamUtilsSuite))*value=&utility;
        return *value?0:1;
    };
    basic.ReleaseSuite=[](const char*,int32)->SPErr{++releases;return 0;};
    app.PF_InvalidateRect=[](PF_ContextH,const PF_Rect*)->PF_Err{++invalidations;return 0;};
    app.PF_AppColorPickerDialog=[](const A_char*,const PF_PixelFloat*,PF_Boolean monitor,PF_PixelFloat* output)->PF_Err {
        check(monitor!=0,"native picker uses working-space display transform");*output={1,0.2f,0.4f,0.8f};return cancel?PF_Err_BAD_CALLBACK_PARAM:0;
    };
    custom.PF_GetDrawingReference=[](PF_ContextH,DRAWBOT_DrawRef* output)->PF_Err{*output=reference<DRAWBOT_DrawRef>();return 0;};
    bot.GetSupplier=[](DRAWBOT_DrawRef,DRAWBOT_SupplierRef* output)->SPErr{*output=reference<DRAWBOT_SupplierRef>();return 0;};
    bot.GetSurface=[](DRAWBOT_DrawRef,DRAWBOT_SurfaceRef* output)->SPErr{*output=reference<DRAWBOT_SurfaceRef>();return 0;};
    supplier.NewDefaultFont=[](DRAWBOT_SupplierRef,float,DRAWBOT_FontRef* output)->SPErr{++objects;*output=reference<DRAWBOT_FontRef>();return 0;};
    supplier.NewBrush=[](DRAWBOT_SupplierRef,const DRAWBOT_ColorRGBA* color,DRAWBOT_BrushRef* output)->SPErr{preview_color=*color;++objects;*output=reference<DRAWBOT_BrushRef>();return 0;};
    supplier.NewPath=[](DRAWBOT_SupplierRef,DRAWBOT_PathRef* output)->SPErr{if(drawing_fail)return 1;++objects;*output=reference<DRAWBOT_PathRef>();return 0;};
    supplier.NewImageFromBuffer=[](DRAWBOT_SupplierRef,int width,int height,int stride,DRAWBOT_PixelLayout format,const void* data,DRAWBOT_ImageRef* output)->SPErr {
        check(format==kDRAWBOT_PixelLayout_24RGB && stride>=width*3 && stride%4==0 && height==62,"gradient creates one opaque RGB bitmap with aligned rows");
        const auto* pixels=static_cast<const std::uint8_t*>(data);bitmap_pixels.assign(pixels,pixels+stride*height);
        bitmap_width=width;bitmap_height=height;++objects;*output=reference<DRAWBOT_ImageRef>();return bitmap_fail?1:0;
    };
    surface.DrawImage=[](DRAWBOT_SurfaceRef,DRAWBOT_ImageRef,const DRAWBOT_PointF32* origin,float alpha)->SPErr {
        ++bitmap_draws;check(alpha==1,"bitmap draws opaquely without strip edge blending");
        if(preview.is_open())for(int x=0;x<bitmap_width;++x)preview<<"<rect x='"<<origin->x+x<<"' y='"<<origin->y<<"' width='1' height='"<<bitmap_height
            <<"' fill='rgb("<<int(bitmap_pixels[x*3])<<","<<int(bitmap_pixels[x*3+1])<<","<<int(bitmap_pixels[x*3+2])<<")'/>\n";
        return 0;
    };
    supplier.ReleaseObject=[](DRAWBOT_ObjectRef)->SPErr{--objects;return 0;};
    path.AddRect=[](DRAWBOT_PathRef,const DRAWBOT_RectF32* bounds)->SPErr{preview_rect=*bounds;++rectangles;check(bounds->width>0 && bounds->height>0,"positive drawing rectangles");return 0;};
    surface.FillPath=[](DRAWBOT_SurfaceRef,DRAWBOT_BrushRef,DRAWBOT_PathRef,DRAWBOT_FillType)->SPErr{
        if(preview.is_open())preview<<"<rect x='"<<preview_rect.left<<"' y='"<<preview_rect.top<<"' width='"<<preview_rect.width<<"' height='"<<preview_rect.height
            <<"' fill='rgb("<<int(preview_color.red*255)<<","<<int(preview_color.green*255)<<","<<int(preview_color.blue*255)<<")'/>\n";
        return 0;};
    surface.DrawString=[](DRAWBOT_SurfaceRef,DRAWBOT_BrushRef,DRAWBOT_FontRef,const DRAWBOT_UTF16Char* string,
        const DRAWBOT_PointF32* origin,DRAWBOT_TextAlignment,DRAWBOT_TextTruncation,float)->SPErr{
        ++texts;if(preview.is_open()) {
            preview<<"<text x='"<<origin->x<<"' y='"<<origin->y<<"' fill='#e5e5e5' font-family='Arial' font-size='11'>";
            for(unsigned i=0;string[i];++i)preview<<char(string[i]);preview<<"</text>\n";
        }return 0;};
    utility.PF_UpdateParamUI=[](PF_ProgPtr,PF_ParamIndex,const PF_ParamDef*)->PF_Err{++ui_updates;return 0;};
}
}
PF_Err update_native_particle_visibility(PF_InData*,PF_ParamDef* params[]) noexcept {
    ++ui_updates;
    check(params[layout::shape]->u.pd.value==1 && params[layout::color_mode]->u.pd.value==1,"UI forwards mode/shape visibility without authoring values");
    return PF_Err_NONE;
}
PF_Err sync_node_graph_parameter(PF_InData*,PF_OutData*,PF_ParamDef* params[],const PF_UserChangedParamExtra* extra,bool bank) noexcept {
    ++publications;check(bank && extra->param_index==layout::gradient,"entire gradient bank publishes atomically");
    check(params[layout::gradient]->uu.change_flags==PF_ChangeFlag_CHANGED_VALUE,"count carries native undo/change flag");
    return reject?PF_Err_BAD_CALLBACK_PARAM:PF_Err_NONE;
}
int main() {
    using starfield::adapter::particle_gradient_event;
    auto gradient=model::preset(2);check(model::valid(gradient),"independent preset validates");
    const auto original=gradient;model::flip(gradient);model::flip(gradient);
    std::array<std::uint8_t,220*62*3> pixels{};
    check(model::rasterize_rgb8(gradient,220,62,pixels),"entire gradient rasterizes into a bounded bitmap");
    bool equal_rows=true;for(unsigned y=1;y<62;++y)for(unsigned i=0;i<220*3;++i)equal_rows &= pixels[y*220*3+i]==pixels[i];
    check(equal_rows && pixels[0]==255 && pixels[219*3]==std::lround(gradient.stops[4].color.x*255),"all rows have complete identical coverage and exact endpoints");
    check(!model::rasterize_rgb8(gradient,221,62,pixels) && !model::rasterize_rgb8(gradient,220,0,pixels),"invalid bitmap dimensions rejected");
    std::array<std::uint8_t,304*62> narrow{};
    check(model::rasterize_rgb8(gradient,101,62,narrow,304) && narrow[303]==0 && narrow[304]==narrow[0],"narrow panel bitmap has aligned padding and continuous complete rows");
    check(starfield::core::encode_color_gradient(original)==starfield::core::encode_color_gradient(gradient),"double flip retains complete gradient");
    check(model::insert(gradient,std::numeric_limits<double>::quiet_NaN())<0,"nonfinite add rejected");
    check(!model::move(gradient,0,.3) && !model::erase(gradient,0),"endpoints cannot move/delete");
    const auto added=model::insert(gradient,.125);check(added==1 && gradient.count==6,"insert preserves interpolated color");
    check(model::move(gradient,added,5) && gradient.stops[added].position<gradient.stops[added+1].position,"drag clamps before adjacent stop");
    check(model::erase(gradient,added) && gradient.count==5,"interior delete retains valid gradient");
    while(gradient.count<8) {double widest=0;unsigned slot=1;for(unsigned i=1;i<gradient.count;++i)if(gradient.stops[i].position-gradient.stops[i-1].position>widest){widest=gradient.stops[i].position-gradient.stops[i-1].position;slot=i;}
        check(model::insert(gradient,(gradient.stops[slot].position+gradient.stops[slot-1].position)/2)>=0,"bounded add");}
    check(model::insert(gradient,.1)<0,"ninth stop rejected");
    configure();PF_InData data{};data.pica_basicP=&basic;PF_OutData out{};
    std::array<PF_ParamDef,160> values{};std::array<PF_ParamDef*,160> params{};
    for(unsigned i=0;i<params.size();++i)params[i]=&values[i];
    values[layout::gradient].param_type=PF_Param_FLOAT_SLIDER;values[layout::gradient].u.fs_d.value=2;
    for(int i=0;i<8;++i){values[layout::gradient_first+2*i].param_type=PF_Param_FLOAT_SLIDER;
        values[layout::gradient_first+2*i+1].param_type=PF_Param_COLOR;values[layout::gradient_first+2*i+1].u.cd.value={255,255,255,255};}
    values[layout::gradient_first+2].u.fs_d.value=100;
    PF_EventExtra event{};event.contextH=reference<PF_ContextH>();event.effect_win.index=layout::gradient;event.effect_win.area=PF_EA_CONTROL;
    event.effect_win.current_frame.left=0;event.effect_win.current_frame.top=0;
    event.effect_win.current_frame.right=310;event.effect_win.current_frame.bottom=180;
    auto click=[&](A_short x,A_short y,A_long count=1,PF_Modifiers modifiers=0){event.e_type=PF_Event_DO_CLICK;event.u.do_click={};event.u.do_click.screen_point.h=x;event.u.do_click.screen_point.v=y;event.u.do_click.num_clicks=count;event.u.do_click.modifiers=modifiers;
        return particle_gradient_event(&data,&out,params.data(),&event);};
    check(click(118,30)==0 && values[layout::gradient].u.fs_d.value==3,"bar click adds actual native stop");
    check(click(118,70)==0 && event.u.do_click.send_drag,"interior marker starts native drag");
    event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=200;event.u.do_click.screen_point.v=70;event.u.do_click.last_time=TRUE;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && values[layout::gradient_first+2].u.fs_d.value>80,"drag changes authored position");
    const int before_cancel=publications;cancel=true;check(click(200,70,2)==0 && publications==before_cancel,"cancelled picker does not author");cancel=false;
    check(click(200,70,2)==0 && values[layout::gradient_first+3].u.cd.value.blue==204,"native picker authors selected stop color");
    check(click(200,70,1,PF_Mod_OPT_ALT_KEY)==0 && values[layout::gradient].u.fs_d.value==2,"Alt-click deletes only interior stop");
    check(click(8,70,1,PF_Mod_OPT_ALT_KEY)==0 && values[layout::gradient].u.fs_d.value==2,"endpoint delete is harmless");
    check(click(145,105)==0,"presets menu opens");check(click(145,130)==0 && values[layout::gradient].u.fs_d.value==5,"preset authors all saved stops");
    check(click(15,105)==0,"copy changes no source values");
    const auto before=values;reject=true;check(click(248,45)==PF_Err_BAD_CALLBACK_PARAM,"publication failure returned");reject=false;
    bool restored=true;for(int i=0;i<17;++i)restored &= std::memcmp(&values[layout::gradient+i],&before[layout::gradient+i],sizeof(PF_ParamDef))==0;
    check(restored,"failed edit restores every count/position/color/change flag");
    check(click(248,45)==0,"flip authors complete bank");check(click(80,105)==0 && values[layout::gradient_first+1].u.cd.value.red==255,"paste restores copied first stop");
    const int prior=publications;event.e_type=PF_Event_DRAW;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_draws==1 && rectangles<30 && texts>=7,"native editor draws gradient once with markers and actions");
    check(publications==prior && objects==0,"draw writes no authored values and releases Drawbot objects");
    drawing_fail=true;check(particle_gradient_event(&data,&out,params.data(),&event)==0 && objects==0,"drawing failure releases brushes and font");drawing_fail=false;
    const int previous_bitmaps=bitmap_draws;bitmap_fail=true;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && objects==0 && bitmap_draws==previous_bitmaps,"failed bitmap creation releases its object without drawing");bitmap_fail=false;
    values[layout::color_mode].u.pd.value=1;values[layout::shape].u.pd.value=1;
    check(starfield::adapter::particle_gradient_param_ui(&data,params.data())==0 && ui_updates==1,"particle UI forwards visibility through the native stream helper");
    check(publications==prior,"UPDATE_PARAMS_UI never changes authored values");
    preview.open("artifacts/gradient-editor-preview.svg");
    preview<<"<svg xmlns='http://www.w3.org/2000/svg' width='310' height='180'>\n<rect width='310' height='180' fill='#29292c'/>\n";
    event.e_type=PF_Event_DRAW;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0,"native Drawbot commands exported for visual inspection");
    preview<<"</svg>\n";check(bool(preview),"visual evidence saved only under artifacts");preview.close();
    event.e_type=PF_Event_CLOSE_CONTEXT;check(particle_gradient_event(&data,&out,params.data(),&event)==0,"UI context closes safely");
    starfield::adapter::clear_particle_gradient_ui();check(gets==releases,"SDK suites balanced across all events");
    std::printf("Gradient editor: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
