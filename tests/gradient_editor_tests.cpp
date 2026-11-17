#include "AEConfig.h"
#include "AE_EffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "SPBasic.h"
#include "NodeEffects.hpp"
#include "ParticleGradientUI.hpp"
#include "ParticleLayout.hpp"
#include "NodeRecord.hpp"
#include "NodeGraphSync.hpp"
#include "GradientEditorModel.hpp"
#include "EditorPresetPicker.hpp"
#include <adobesdk/DrawbotSuite.h>
#include <array>
#include <cstring>
#include <cstdio>
#include <limits>
#include <fstream>
#include <functional>
#include <vector>

// Modal selection is a fake-host seam. Verify the selected payload is published
// by the real editor; this does not open a Windows dialog or qualify its UI.
namespace starfield::adapter {
bool choose_gradient_preset(PF_InData*, core::ColorGradient& value) noexcept {
    value = gradient_editor::preset(2);
    return true;
}
bool choose_curve_preset(PF_InData*, core::AgeCurve& value) noexcept {
    value = {};
    value.count = 2;
    value.points[0] = {0, 0};
    value.points[1] = {1, 360};
    return true;
}
}

namespace {
namespace model=starfield::adapter::gradient_editor;
namespace layout=starfield::adapter::native_nodes::particle_layout;
int checks{},failures{},publications{},invalidations{},gets{},releases{},objects{},rectangles{},texts{},ui_updates{},bitmap_draws{},bitmap_creates{},unsupported_formats{};
bool reject{},cancel{},drawing_fail{},bitmap_fail{},bitmap_null{},draw_image_fail{};
bool supports_bgra{true},supports_argb{},prefers_bgra{true},prefers_argb{},query_fail{},preference_fail{};
std::vector<std::uint8_t> bitmap_pixels;
int bitmap_width{},bitmap_height{};
DRAWBOT_PixelLayout bitmap_format{};
std::function<void()> image_hook;
std::function<void()> invalidate_hook;
std::function<void(const starfield::adapter::node_sync::NativeEdit&)> publish_hook;
std::vector<DRAWBOT_PointF32> curve_path;
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
    app.PF_InvalidateRect=[](PF_ContextH,const PF_Rect*)->PF_Err{++invalidations;if(invalidate_hook)invalidate_hook();return 0;};
    app.PF_AppColorPickerDialog=[](const A_char*,const PF_PixelFloat*,PF_Boolean monitor,PF_PixelFloat* output)->PF_Err {
        check(monitor!=0,"native picker uses working-space display transform");*output={1,0.2f,0.4f,0.8f};return cancel?PF_Err_BAD_CALLBACK_PARAM:0;
    };
    custom.PF_GetDrawingReference=[](PF_ContextH,DRAWBOT_DrawRef* output)->PF_Err{*output=reference<DRAWBOT_DrawRef>();return 0;};
    bot.GetSupplier=[](DRAWBOT_DrawRef,DRAWBOT_SupplierRef* output)->SPErr{*output=reference<DRAWBOT_SupplierRef>();return 0;};
    bot.GetSurface=[](DRAWBOT_DrawRef,DRAWBOT_SurfaceRef* output)->SPErr{*output=reference<DRAWBOT_SurfaceRef>();return 0;};
    supplier.NewDefaultFont=[](DRAWBOT_SupplierRef,float,DRAWBOT_FontRef* output)->SPErr{++objects;*output=reference<DRAWBOT_FontRef>();return 0;};
    supplier.NewBrush=[](DRAWBOT_SupplierRef,const DRAWBOT_ColorRGBA* color,DRAWBOT_BrushRef* output)->SPErr{preview_color=*color;++objects;*output=reference<DRAWBOT_BrushRef>();return 0;};
    supplier.NewPen=[](DRAWBOT_SupplierRef,const DRAWBOT_ColorRGBA*,float,DRAWBOT_PenRef* output)->SPErr{++objects;*output=reference<DRAWBOT_PenRef>();return 0;};
    supplier.NewPath=[](DRAWBOT_SupplierRef,DRAWBOT_PathRef* output)->SPErr{if(drawing_fail)return 1;++objects;*output=reference<DRAWBOT_PathRef>();return 0;};
    supplier.SupportsPixelLayoutBGRA=[](DRAWBOT_SupplierRef,DRAWBOT_Boolean* output)->SPErr{*output=supports_bgra;return query_fail?1:0;};
    supplier.SupportsPixelLayoutARGB=[](DRAWBOT_SupplierRef,DRAWBOT_Boolean* output)->SPErr{*output=supports_argb;return query_fail?1:0;};
    supplier.PrefersPixelLayoutBGRA=[](DRAWBOT_SupplierRef,DRAWBOT_Boolean* output)->SPErr{*output=prefers_bgra;return preference_fail?1:0;};
    supplier.PrefersPixelLayoutARGB=[](DRAWBOT_SupplierRef,DRAWBOT_Boolean* output)->SPErr{*output=prefers_argb;return preference_fail?1:0;};
    supplier.NewImageFromBuffer=[](DRAWBOT_SupplierRef,int width,int height,int stride,DRAWBOT_PixelLayout format,const void* data,DRAWBOT_ImageRef* output)->SPErr {
        ++bitmap_creates;
        if(image_hook){auto callback=std::move(image_hook);callback();}
        const bool supported=!query_fail && ((format==kDRAWBOT_PixelLayout_32BGRA_Premul && supports_bgra) ||
            (format==kDRAWBOT_PixelLayout_32ARGB_Premul && supports_argb));
        if(!supported){++unsupported_formats;*output=nullptr;return 1;}
        check(stride==width*4 && stride%4==0 && height==62,"gradient creates one supported 32-bit bitmap with aligned rows");
        const auto* pixels=static_cast<const std::uint8_t*>(data);bitmap_pixels.assign(pixels,pixels+stride*height);
        const auto alpha=format==kDRAWBOT_PixelLayout_32BGRA_Premul?3:0;
        bool opaque=true;for(int y=0;y<height;++y)for(int x=0;x<width;++x)opaque &= pixels[y*stride+x*4+alpha]==255;
        check(opaque,"every bitmap pixel is fully opaque in the negotiated channel order");
        bitmap_width=width;bitmap_height=height;bitmap_format=format;
        if(bitmap_null){*output=nullptr;return 0;}
        ++objects;*output=reference<DRAWBOT_ImageRef>();return bitmap_fail?1:0;
    };
    surface.DrawImage=[](DRAWBOT_SurfaceRef,DRAWBOT_ImageRef,const DRAWBOT_PointF32* origin,float alpha)->SPErr {
        ++bitmap_draws;check(alpha==1,"bitmap draws opaquely without strip edge blending");
        if(preview.is_open())for(int x=0;x<bitmap_width;++x) {
            const bool bgra=bitmap_format==kDRAWBOT_PixelLayout_32BGRA_Premul;
            preview<<"<rect x='"<<origin->x+x<<"' y='"<<origin->y<<"' width='1' height='"<<bitmap_height
                <<"' fill='rgb("<<int(bitmap_pixels[x*4+(bgra?2:1)])<<","<<int(bitmap_pixels[x*4+(bgra?1:2)])<<","<<int(bitmap_pixels[x*4+(bgra?0:3)])<<")'/>\n";
        }
        return draw_image_fail?1:0;
    };
    supplier.ReleaseObject=[](DRAWBOT_ObjectRef)->SPErr{--objects;return 0;};
    path.AddRect=[](DRAWBOT_PathRef,const DRAWBOT_RectF32* bounds)->SPErr{preview_rect=*bounds;++rectangles;check(bounds->width>0 && bounds->height>0,"positive drawing rectangles");return 0;};
    path.MoveTo=[](DRAWBOT_PathRef,float x,float y)->SPErr{curve_path.clear();curve_path.push_back({x,y});return 0;};
    path.LineTo=[](DRAWBOT_PathRef,float x,float y)->SPErr{curve_path.push_back({x,y});return 0;};
    surface.StrokePath=[](DRAWBOT_SurfaceRef,DRAWBOT_PenRef,DRAWBOT_PathRef)->SPErr{return 0;};
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
    ++publications;check(bank && (extra->param_index==layout::gradient || layout::curve_base(extra->param_index)),"entire visual bank publishes atomically");
    bool changed=false;
    const auto span=extra->param_index==layout::gradient?18:layout::curve_span;
    for(int i=0;i<span;++i)changed|=(params[extra->param_index+i]->uu.change_flags&PF_ChangeFlag_CHANGED_VALUE)!=0;
    check(changed,"visual bank carries native undo/change flags");
    return reject?PF_Err_BAD_CALLBACK_PARAM:PF_Err_NONE;
}
PF_Err sync_custom_node_graph_bank(PF_InData*,PF_OutData*,const starfield::adapter::node_sync::NativeEdit& bank) noexcept {
    ++publications;check(starfield::adapter::node_sync::valid_custom_bank(bank),"entire visual bank publishes as owned typed values");
    if(publish_hook)publish_hook(bank);
    return reject?PF_Err_BAD_CALLBACK_PARAM:PF_Err_NONE;
}
int main() {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    using starfield::adapter::particle_gradient_event;
    auto gradient=model::preset(2);check(model::valid(gradient),"independent preset validates");
    const auto original=gradient;model::flip(gradient);model::flip(gradient);
    std::array<std::uint8_t,220*62*4> pixels{};
    check(model::rasterize_opaque32(gradient,220,62,model::PixelOrder::bgra,pixels),"entire gradient rasterizes into a bounded BGRA bitmap");
    bool equal_rows=true;for(unsigned y=1;y<62;++y)for(unsigned i=0;i<220*4;++i)equal_rows &= pixels[y*220*4+i]==pixels[i];
    check(equal_rows && pixels[2]==255 && pixels[3]==255 && pixels[219*4+2]==std::lround(gradient.stops[4].color.x*255),"all BGRA rows have identical coverage and exact endpoints");
    check(model::rasterize_opaque32(gradient,220,62,model::PixelOrder::argb,pixels) && pixels[0]==255 && pixels[1]==255 && pixels[219*4+1]==std::lround(gradient.stops[4].color.x*255),"ARGB pixels have correct alpha and color byte order");
    check(!model::rasterize_opaque32(gradient,221,62,model::PixelOrder::bgra,pixels) && !model::rasterize_opaque32(gradient,220,0,model::PixelOrder::bgra,pixels),"invalid bitmap dimensions rejected");
    std::array<std::uint8_t,408*62> narrow{};
    check(model::rasterize_opaque32(gradient,101,62,model::PixelOrder::bgra,narrow,408) && narrow[407]==0 && narrow[408]==narrow[0],"narrow bitmap has zero padding and continuous complete rows");
    check(!model::rasterize_opaque32(gradient,101,62,model::PixelOrder::bgra,narrow,400) && !model::rasterize_opaque32(gradient,101,62,model::PixelOrder::bgra,std::span{narrow.data(),narrow.size()-1},408),"short strides and buffers rejected");
    check(!model::rasterize_opaque32(gradient,std::numeric_limits<unsigned>::max(),62,model::PixelOrder::bgra,pixels),"dimension overflow rejected before stride arithmetic");
    check(starfield::core::encode_color_gradient(original)==starfield::core::encode_color_gradient(gradient),"double flip retains complete gradient");
    check(model::insert(gradient,std::numeric_limits<double>::quiet_NaN())<0,"nonfinite add rejected");
    auto ends=gradient;
    const auto first_color=ends.stops[0].color,last_color=ends.stops[ends.count-1].color;
    check(model::move(ends,0,.1)==0 && model::move(ends,ends.count-1,.9)==ends.count-1 && model::valid(ends),"first and last markers move inside the gradient range");
    check(starfield::core::evaluate_color_gradient(ends,0).x==first_color.x && starfield::core::evaluate_color_gradient(ends,1).x==last_color.x,"moved end colors extend across the uncovered range");
    check(model::erase(ends,0) && model::erase(ends,ends.count-1) && ends.count==3,"any marker can be removed while retaining at least two stops");
    check(model::move(ends,-1,.3)<0 && model::move(ends,0,std::numeric_limits<double>::quiet_NaN())<0,"invalid and nonfinite moves rejected");
    const auto added=model::insert(gradient,.125);check(added==1 && gradient.count==6,"insert preserves interpolated color");
    const auto crossed=model::move(gradient,added,5);
    check(crossed>added && model::valid(gradient) && gradient.stops[crossed].position<1,"drag crosses other stops and avoids coincident positions");
    check(model::erase(gradient,crossed) && gradient.count==5,"delete follows the moved marker identity");
    while(gradient.count<8) {double widest=0;unsigned slot=1;for(unsigned i=1;i<gradient.count;++i)if(gradient.stops[i].position-gradient.stops[i-1].position>widest){widest=gradient.stops[i].position-gradient.stops[i-1].position;slot=i;}
        check(model::insert(gradient,(gradient.stops[slot].position+gradient.stops[slot-1].position)/2)>=0,"bounded add");}
    check(model::insert(gradient,.1)<0,"ninth stop rejected");
    configure();PF_InData data{};data.pica_basicP=&basic;PF_OutData out{};
    namespace records=starfield::adapter::native_nodes;
    constexpr auto parameter_count=records::last_parameter_index(records::Kind::particle)+1;
    std::array<PF_ParamDef,parameter_count> values{};std::array<PF_ParamDef*,parameter_count> params{};
    data.num_params=parameter_count;
    for(unsigned i=0;i<params.size();++i)params[i]=&values[i];
    for(int i=0;i<8;++i)values[records::uuid_first_index(records::Kind::particle)+i].param_type=PF_Param_FLOAT_SLIDER;
    values[records::uuid_first_index(records::Kind::particle)+7].u.fs_d.value=2;
    values[layout::gradient].param_type=PF_Param_FLOAT_SLIDER;values[layout::gradient].u.fs_d.value=2;
    values[layout::gradient_interpolation].param_type=PF_Param_FLOAT_SLIDER;
    for(int i=0;i<8;++i){values[layout::gradient_first+2*i].param_type=PF_Param_FLOAT_SLIDER;
        values[layout::gradient_first+2*i+1].param_type=PF_Param_COLOR;values[layout::gradient_first+2*i+1].u.cd.value={255,255,255,255};}
    values[layout::gradient_first+2].u.fs_d.value=100;
    PF_EventExtra event{};event.contextH=reference<PF_ContextH>();event.effect_win.index=layout::gradient;event.effect_win.area=PF_EA_CONTROL;
    event.effect_win.current_frame.left=0;event.effect_win.current_frame.top=0;
    event.effect_win.current_frame.right=310;event.effect_win.current_frame.bottom=180;
    auto click=[&](A_short x,A_short y,A_long count=1,PF_Modifiers modifiers=0){event.e_type=PF_Event_DO_CLICK;event.u.do_click={};event.u.do_click.screen_point.h=x;event.u.do_click.screen_point.v=y;event.u.do_click.num_clicks=count;event.u.do_click.modifiers=modifiers;
        return particle_gradient_event(&data,&out,params.data(),&event);};
    check(click(8,70)==0 && event.u.do_click.send_drag,"first native marker starts drag");
    event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=52;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && std::abs(values[layout::gradient_first].u.fs_d.value-20)<.001,"first native marker authors an inset position");
    check(click(228,70)==0 && event.u.do_click.send_drag,"last native marker starts drag");
    event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=184;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && std::abs(values[layout::gradient_first+2].u.fs_d.value-80)<.001,"last native marker authors an inset position");
    check(click(118,30)==0 && values[layout::gradient].u.fs_d.value==3,"bar click adds actual native stop");
    check(click(118,70)==0 && event.u.do_click.send_drag,"interior marker starts native drag");
    event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=200;event.u.do_click.screen_point.v=70;event.u.do_click.last_time=TRUE;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && event.u.do_click.continue_refcon[0]==3 && values[layout::gradient_first+4].u.fs_d.value>80,"drag reorders authored positions and updates the continuation marker");
    const int before_cancel=publications;cancel=true;check(click(200,70,2)==0 && publications==before_cancel,"cancelled picker does not author");cancel=false;
    check(click(200,70,2)==0 && values[layout::gradient_first+5].u.cd.value.blue==204,"native picker follows the reordered stop color");
    check(click(200,70,1,PF_Mod_OPT_ALT_KEY)==0 && values[layout::gradient].u.fs_d.value==2,"Alt-click deletes a moved last stop");
    check(click(52,70,1,PF_Mod_OPT_ALT_KEY)==0 && values[layout::gradient].u.fs_d.value==2,"two-stop gradient cannot lose another marker");
    check(click(145,105)==0 && values[layout::gradient].u.fs_d.value==5,"selected preset authors all saved stops");
    check(click(15,105)==0,"copy changes no source values");
    const auto before=values;reject=true;check(click(248,45)==PF_Err_BAD_CALLBACK_PARAM,"publication failure returned");reject=false;
    bool restored=true;for(int i=0;i<17;++i)restored &= std::memcmp(&values[layout::gradient+i],&before[layout::gradient+i],sizeof(PF_ParamDef))==0;
    check(restored,"failed edit restores every count/position/color/change flag");
    check(click(248,45)==0,"flip authors complete bank");check(click(80,105)==0 && values[layout::gradient_first+1].u.cd.value.red==255,"paste restores copied first stop");
    const int prior=publications;event.e_type=PF_Event_DRAW;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_draws==1 && rectangles<30 && texts>=7,"native editor draws gradient once with markers and actions");
    check(publications==prior && objects==0,"draw writes no authored values and releases Drawbot objects");
    drawing_fail=true;check(particle_gradient_event(&data,&out,params.data(),&event)==0 && objects==0,"drawing failure releases brushes and font");drawing_fail=false;
    auto close_context=[&](){event.e_type=PF_Event_CLOSE_CONTEXT;check(particle_gradient_event(&data,&out,params.data(),&event)==0,"UI context resets without SDK object retention");event.e_type=PF_Event_DRAW;};
    // The prior fake supplier accepted 24RGB unconditionally. Model the host's
    // support queries and reject unadvertised formats without allowing a probe.
    for(unsigned mask=0;mask<16;++mask) {
        close_context();supports_bgra=(mask&1)!=0;supports_argb=(mask&2)!=0;
        prefers_bgra=(mask&4)!=0;prefers_argb=(mask&8)!=0;
        const auto creates=bitmap_creates;const auto shapes=rectangles;
        check(particle_gradient_event(&data,&out,params.data(),&event)==0 && objects==0,"support/preference matrix draws and releases resources");
        if(!supports_bgra && !supports_argb)check(bitmap_creates==creates && rectangles>shapes+100,"no supported layout uses path fallback without image probes");
        else check(bitmap_creates==creates+1 && bitmap_format==((supports_bgra && (prefers_bgra || !supports_argb || !prefers_argb))?kDRAWBOT_PixelLayout_32BGRA_Premul:kDRAWBOT_PixelLayout_32ARGB_Premul),"image layout follows advertised support and preference");
    }
    supports_bgra=supports_argb=true;prefers_bgra=false;prefers_argb=true;
    close_context();query_fail=true;auto creates=bitmap_creates;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates,"failed support queries cannot authorize image creation");query_fail=false;
    close_context();preference_fail=true;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_format==kDRAWBOT_PixelLayout_32BGRA_Premul,"failed preferences still use successfully queried support");preference_fail=false;
    const auto saved_bgra=supplier.SupportsPixelLayoutBGRA,saved_argb=supplier.SupportsPixelLayoutARGB;
    supplier.SupportsPixelLayoutBGRA=supplier.SupportsPixelLayoutARGB=nullptr;close_context();creates=bitmap_creates;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates,"missing capability APIs use fallback without guessing");
    const auto authored=publications;check(click(70,30)==0 && publications==authored+1,"path fallback retains authored gradient interactions");event.e_type=PF_Event_DRAW;
    supplier.SupportsPixelLayoutBGRA=saved_bgra;supplier.SupportsPixelLayoutARGB=saved_argb;
    supports_bgra=true;supports_argb=false;prefers_bgra=true;prefers_argb=false;
    close_context();creates=bitmap_creates;
    image_hook=[&](){check(particle_gradient_event(&data,&out,params.data(),&event)==0,"reentrant image-creation repaint draws a safe fallback");};
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates+1 && objects==0,"image guard is set before a host call can repaint recursively");
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates+2,"successful image creation restores subsequent normal bitmap draws");
    const auto saved_prefer_bgra=supplier.PrefersPixelLayoutBGRA,saved_prefer_argb=supplier.PrefersPixelLayoutARGB;
    supplier.PrefersPixelLayoutBGRA=supplier.PrefersPixelLayoutARGB=nullptr;close_context();
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_format==kDRAWBOT_PixelLayout_32BGRA_Premul,"missing preferences use advertised supported layout");
    supplier.PrefersPixelLayoutBGRA=saved_prefer_bgra;supplier.PrefersPixelLayoutARGB=saved_prefer_argb;
    for(unsigned failure=0;failure<3;++failure) {
        close_context();bitmap_fail=failure==0;bitmap_null=failure==1;draw_image_fail=failure==2;
        creates=bitmap_creates;
        for(unsigned repeat=0;repeat<4;++repeat)check(particle_gradient_event(&data,&out,params.data(),&event)==0 && objects==0,"image failure repaint retains usable editor and releases any image");
        check(bitmap_creates==creates+1,"image creation/null/draw failure is not retried by subsequent repaints");
        bitmap_fail=bitmap_null=draw_image_fail=false;
        close_context();check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates+2,"a new UI context may attempt a supported image again");
    }
    const auto saved_create=supplier.NewImageFromBuffer;const auto saved_draw=surface.DrawImage;
    supplier.NewImageFromBuffer=nullptr;close_context();creates=bitmap_creates;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates,"missing image creation API uses safe fallback");supplier.NewImageFromBuffer=saved_create;
    surface.DrawImage=nullptr;close_context();
    check(particle_gradient_event(&data,&out,params.data(),&event)==0 && bitmap_creates==creates,"missing image draw API uses safe fallback");surface.DrawImage=saved_draw;
    check(unsupported_formats==0,"no unsupported image format was ever submitted to Drawbot");
    values[layout::color_mode].u.pd.value=1;values[layout::shape].u.pd.value=1;
    check(starfield::adapter::particle_gradient_param_ui(&data,params.data())==0 && ui_updates==1,"particle UI forwards visibility through the native stream helper");
    check(publications==authored+1,"draw and UPDATE_PARAMS_UI never change authored values");
    close_context();
    preview.open("artifacts/gradient-editor-preview.svg");
    preview<<"<svg xmlns='http://www.w3.org/2000/svg' width='310' height='180'>\n<rect width='310' height='180' fill='#29292c'/>\n";
    event.e_type=PF_Event_DRAW;
    check(particle_gradient_event(&data,&out,params.data(),&event)==0,"native Drawbot commands exported for visual inspection");
    preview<<"</svg>\n";check(bool(preview),"visual evidence saved only under artifacts");preview.close();
    event.e_type=PF_Event_CLOSE_CONTEXT;check(particle_gradient_event(&data,&out,params.data(),&event)==0,"UI context closes safely");
    starfield::adapter::clear_particle_gradient_ui();check(gets==releases,"SDK suites balanced across all events");
    using starfield::adapter::particle_rotation_curve_event;
    event.effect_win.index=layout::rotation_curve;
    for(int i=0;i<layout::curve_span;++i)values[layout::rotation_curve+i].param_type=PF_Param_FLOAT_SLIDER;
    event.e_type=PF_Event_DRAW;check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && objects==0,"default zero rotation curve draws without any bitmap");
    const auto curve_click=[&](int x,int y){event.e_type=PF_Event_DO_CLICK;event.u.do_click={};event.u.do_click.screen_point.h=x;event.u.do_click.screen_point.v=y;return particle_rotation_curve_event(&data,&out,params.data(),&event);};
    const auto curve_release=[&](){event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;return particle_rotation_curve_event(&data,&out,params.data(),&event);};
    auto edits=publications;check(curve_click(118,25)==0 && event.u.do_click.send_drag && publications==edits,"new point joins its initial gesture without publishing on mouse down");
    event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=140;event.u.do_click.screen_point.v=15;event.u.do_click.last_time=FALSE;
    check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==edits && values[layout::rotation_curve].u.fs_d.value==0,
          "newly inserted point moves on the same held gesture without writing host values");
    check(curve_release()==0 && publications==edits+1 && values[layout::rotation_curve].u.fs_d.value==3,"new point publishes once when its initial gesture finishes");
    check(std::abs(values[layout::rotation_curve+3].u.fs_d.value-.6)<1e-6 && std::abs(values[layout::rotation_curve+4].u.fs_d.value-270)<1e-6,
          "first gesture saves the moved new point rather than its original click location");
    check(values[layout::rotation_curve+1].u.fs_d.value==0 && values[layout::rotation_curve+5].u.fs_d.value==1,"rotation curve retains normalized life endpoints");
    auto rotation_bank=values;reject=true;check(curve_click(65,20)==0 && curve_release()!=0,"failed rotation publication is reported at gesture release");reject=false;
    bool curve_restored=true;for(int i=0;i<layout::curve_span;++i)curve_restored &= values[layout::rotation_curve+i].u.fs_d.value==rotation_bank[layout::rotation_curve+i].u.fs_d.value;
    check(curve_restored,"failed curve addition restores count and every old knot");
    check(curve_click(243,35)==0,"rotation curve Flip publishes atomically");
    check(curve_click(20,100)==0 && curve_click(80,100)==0,"rotation curve Copy and Paste remain usable");
    check(curve_click(145,100)==0 && values[layout::rotation_curve+4].u.fs_d.value==360,"selected rotation +360 preset is an authored degree curve");
    event.e_type=PF_Event_CLOSE_CONTEXT;check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && objects==0,"rotation close releases UI state");
    starfield::adapter::clear_particle_gradient_ui();check(gets==releases,"SDK suites balanced across curve events");
    for(const auto base:{layout::size_curve,layout::opacity_curve,layout::rotation_curve}) {
        starfield::adapter::clear_particle_gradient_ui();
        event.effect_win.index=base;
        for(int i=0;i<layout::curve_span;++i){values[base+i]={};values[base+i].param_type=PF_Param_FLOAT_SLIDER;}
        values[layout::size_over_life].u.fs_d.value=values[layout::opacity_over_life].u.fs_d.value=100;
        const auto snapshot=values;
        const auto start=publications;
        check(curve_click(228,base==layout::rotation_curve?45:5)==0 && event.u.do_click.send_drag,
              "all three default curves start an endpoint gesture");
        event.e_type=PF_Event_DRAG;
        for(int y=10;y<=70;y+=10){event.u.do_click.screen_point.v=y;event.u.do_click.last_time=FALSE;
            check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && (event.evt_out_flags&PF_EO_UPDATE_NOW),
                  "intermediate endpoint drag keeps immediate local preview");}
        check(publications==start,"endpoint drag performs no graph publication before release");
        bool untouched=true;for(int i=0;i<layout::curve_span;++i)untouched&=values[base+i].u.fs_d.value==snapshot[base+i].u.fs_d.value;
        check(untouched,"intermediate curve gesture leaves host parameter bank untouched");
        event.e_type=PF_Event_DRAW;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0,"local curve draft repaints");
        publish_hook=[&](const starfield::adapter::node_sync::NativeEdit&) {
            std::vector<PF_ParamDef> old_values(snapshot.begin(),snapshot.end());
            std::vector<PF_ParamDef*> old_params;for(auto& field:old_values)old_params.push_back(&field);
            for(unsigned delivery=0;delivery<2;++delivery) {
                if(delivery)old_values[base].u.fs_d.value=std::numeric_limits<double>::quiet_NaN();
                auto repaint=event;repaint.e_type=PF_Event_DRAW;PF_OutData repaint_out{};curve_path.clear();
                check(particle_rotation_curve_event(&data,&repaint_out,old_params.data(),&repaint)==0 && curve_path.size()==221 && std::abs(curve_path.back().y-70)<1e-5,
                      "publication-time repaint retains desired curve with old or partially updated host bank");
            }
        };
        invalidate_hook=[&](){
            std::vector<PF_ParamDef> old_values(snapshot.begin(),snapshot.end());
            std::vector<PF_ParamDef*> old_params;for(auto& field:old_values)old_params.push_back(&field);
            auto repaint=event;repaint.e_type=PF_Event_DRAW;PF_OutData repaint_out{};curve_path.clear();
            check(particle_rotation_curve_event(&data,&repaint_out,old_params.data(),&repaint)==0 && curve_path.size()==221 && std::abs(curve_path.back().y-70)<1e-5,
                  "local invalidation reentry still sees the desired curve before event return");
        };
        out={};event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==start+1,
              "release publishes exactly one complete curve bank");
        publish_hook={};
        invalidate_hook={};
        check(!(event.evt_out_flags&PF_EO_UPDATE_NOW) && (event.evt_out_flags&PF_EO_HANDLED_EVENT),
              "successful release queues its invalidation until host PF value adoption rather than forcing an immediate draw");
        // Model the host's event-return order: UPDATE_NOW may paint using the
        // pre-event array before the changed PF bank is adopted. A normal queued
        // repaint sees the adopted bank. Execute the real DRAW path in both.
        if(event.evt_out_flags&PF_EO_UPDATE_NOW) {
            std::vector<PF_ParamDef> old_values(snapshot.begin(),snapshot.end());
            std::vector<PF_ParamDef*> old_params;for(auto& field:old_values)old_params.push_back(&field);
            auto early=event;early.e_type=PF_Event_DRAW;PF_OutData early_out{};curve_path.clear();
            check(particle_rotation_curve_event(&data,&early_out,old_params.data(),&early)==0,"early host redraw executes the real control painter");
        }
        check(curve_path.size()==221 && std::abs(curve_path.back().y-70)<1e-5,
              "event return does not flash the pre-edit curve before PF adoption");
        auto settled=event;settled.e_type=PF_Event_DRAW;PF_OutData settled_out{};curve_path.clear();
        check(particle_rotation_curve_event(&data,&settled_out,params.data(),&settled)==0 && curve_path.size()==221 && std::abs(curve_path.back().y-70)<1e-5,
              "settled queued redraw uses adopted authored values without a persistent overlay");
        check(!(out.out_flags&PF_OutFlag_REFRESH_UI) && (out.out_flags&PF_OutFlag_FORCE_RERENDER),
              "curve commit invalidates its local control without refreshing every host window");
        const auto expected=base==layout::rotation_curve?-225.0:18.75;
        check(values[base].u.fs_d.value==2 && std::abs(values[base+4].u.fs_d.value-expected)<1e-9,
              "release retains exact endpoint value for size, opacity and rotation");
        auto committed=values;auto commits=publications;
        check(curve_click(228,70)==0,"committed endpoint can start another gesture");
        event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==commits,
              "click and release without movement performs no graph write");
        check(curve_click(228,70)==0,"cancellable endpoint gesture starts");
        event.e_type=PF_Event_DRAG;event.u.do_click.last_time=FALSE;event.u.do_click.screen_point.v=20;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0,"draft moves before cancellation");
        event.e_type=PF_Event_KEYDOWN;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0,"keyboard cancels owned curve draft");
        event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==commits,
              "late mouse release after cancellation cannot save the discarded draft");
        check(curve_click(228,70)==0,"external-edit guard gesture starts");
        values[base+4].u.fs_d.value=40;
        event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;event.u.do_click.screen_point.v=20;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==commits && values[base+4].u.fs_d.value==40,
              "external host edit cancels draft and is not overwritten at release");
        values=committed;
        check(curve_click(228,70)==0,"failure rollback gesture starts");
        event.e_type=PF_Event_DRAG;event.u.do_click.last_time=TRUE;event.u.do_click.screen_point.v=20;reject=true;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)!=0,"failed final curve publication is reported");reject=false;
        check(event.evt_out_flags&PF_EO_UPDATE_NOW,"failed publication immediately redraws restored authored data");
        bool bank_restored=true;for(int i=0;i<layout::curve_span;++i)bank_restored&=std::memcmp(&values[base+i],&committed[base+i],sizeof(PF_ParamDef))==0;
        check(bank_restored,"failed final gesture restores values and original change flags");
        event.e_type=PF_Event_DRAW;curve_path.clear();
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && curve_path.size()==221 && std::abs(curve_path.back().y-70)<1e-5,
              "failed publication clears its display preview and immediately redraws the restored curve");
        for(unsigned mode=1;mode<=3;++mode) {
            values=committed;values[layout::curve_interpolation(base)].u.fs_d.value=mode;
            commits=publications;
            check(curve_click(228,70)==0 && event.u.do_click.send_drag,"Hold, Bezier and Draw each begin a gesture");
            event.e_type=PF_Event_DRAG;event.u.do_click.last_time=FALSE;event.u.do_click.screen_point.v=25;
            check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==commits,
                  "every interpolation mode keeps intermediate edits local");
            event.u.do_click.last_time=TRUE;
            check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==commits+1,
                  "each interpolation mode publishes its final bank exactly once");
        }
    }
    starfield::adapter::clear_particle_gradient_ui();
    for(const auto base:{layout::size_curve,layout::opacity_curve,layout::rotation_curve}) {
        event.effect_win.index=base;
        for(int i=0;i<layout::curve_span;++i){values[base+i]={};values[base+i].param_type=PF_Param_FLOAT_SLIDER;}
        const auto prior_publications=publications;
        check(curve_click(118,40)==0 && event.u.do_click.send_drag && publications==prior_publications,
              "all Particle curves insert a point into the initial held gesture");
        event.e_type=PF_Event_DRAG;event.u.do_click.screen_point.h=140;event.u.do_click.screen_point.v=25;event.u.do_click.last_time=FALSE;
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && publications==prior_publications && values[base].u.fs_d.value==0,
              "new Size/Opacity/Rotation point moves immediately without a second click or host write");
        check(curve_release()==0 && publications==prior_publications+1 && values[base].u.fs_d.value==3 &&
              std::abs(values[base+3].u.fs_d.value-.6)<1e-6 && std::abs(values[base+4].u.fs_d.value-(base==layout::rotation_curve?180:75))<1e-6,
              "each inserted-point gesture publishes its final position and value once");
        // Simulate Undo before another click; no accepted preview survives the
        // publication call to conceal current authored values.
        values[base].u.fs_d.value=0;event.e_type=PF_Event_DRAW;curve_path.clear();
        check(particle_rotation_curve_event(&data,&out,params.data(),&event)==0 && curve_path.size()==221 &&
              std::abs(curve_path[110].y-(base==layout::rotation_curve?45:5))<1e-5,"post-publication Undo is displayed without a stale overlay");
    }
    starfield::adapter::clear_particle_gradient_ui();
    for(const auto kind:{records::Kind::motion,records::Kind::turbulence}) {
        const auto base=kind==records::Kind::motion?records::motion_layout::curve:records::turbulence_layout::curve;
        std::vector<PF_ParamDef> other_values(records::parameter_count(kind));
        std::vector<PF_ParamDef*> other_params;
        for(auto& param:other_values){param.param_type=PF_Param_FLOAT_SLIDER;other_params.push_back(&param);}
        other_values[records::uuid_first_index(kind)+7].u.fs_d.value=static_cast<A_long>(kind)+1;
        auto other_data=data;other_data.num_params=0;PF_OutData other_out{};
        PF_EventExtra other_event{};other_event.contextH=reference<PF_ContextH>();other_event.effect_win.index=base;
        other_event.effect_win.area=PF_EA_CONTROL;other_event.effect_win.current_frame={0,0,310,180};
        other_event.e_type=PF_Event_DO_CLICK;other_event.u.do_click.screen_point.h=228;other_event.u.do_click.screen_point.v=5;
        const auto prior_publications=publications;
        check(particle_rotation_curve_event(&other_data,&other_out,other_params.data(),&other_event,kind==records::Kind::motion,kind==records::Kind::turbulence)==0 && other_event.u.do_click.send_drag,
              "shared Motion/Turbulence editor can start an owned bank gesture without a USER_CHANGED array count");
        other_event.e_type=PF_Event_DRAG;other_event.u.do_click.last_time=TRUE;other_event.u.do_click.screen_point.v=65;
        check(particle_rotation_curve_event(&other_data,&other_out,other_params.data(),&other_event,kind==records::Kind::motion,kind==records::Kind::turbulence)==0 && publications==prior_publications+1 &&
              other_values[base].u.fs_d.value==2 && other_values[base+4].u.fs_d.value==25,"shared custom editors preserve complete typed bank publication");
    }
    starfield::adapter::clear_particle_gradient_ui();
    std::printf("Gradient editor: %d checks, %d failures\n",checks,failures);return failures?1:0;
}
