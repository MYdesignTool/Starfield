#include "PresetsUI.hpp"
#include "GraphCarrier.hpp"
#include "AE_EffectSuites.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <adobesdk/DrawbotSuite.h>
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <algorithm>
#include <array>
#include <map>
#include <mutex>
#include <vector>

namespace starfield::adapter {
namespace {
using Microsoft::WRL::ComPtr;
template<class T> struct Suite {
    SPBasicSuite* basic{};const char* name{};A_long version{};const T* value{};
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v){if(basic)(void)basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
    const T* operator->()const{return value;} explicit operator bool()const{return value!=nullptr;}
};
std::map<PF_ContextH,bool> disabled;
std::vector<std::uint8_t> pixels;
std::once_flag decoded;
void decode_picture() noexcept try {
    const auto initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    struct ComScope{HRESULT status;~ComScope(){if(SUCCEEDED(status))CoUninitialize();}} scope{initialized};
    HMODULE module{};if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&main_presets_event),&module))return;
    const auto resource=FindResourceW(module,MAKEINTRESOURCEW(16100),RT_RCDATA);if(!resource)return;
    const auto bytes=SizeofResource(module,resource);const auto loaded=LoadResource(module,resource);const auto memory=LockResource(loaded);
    if(!memory || !bytes)return;
    ComPtr<IWICImagingFactory> factory;ComPtr<IWICStream> stream;ComPtr<IWICBitmapDecoder> decoder;ComPtr<IWICBitmapFrameDecode> frame;
    ComPtr<IWICBitmapScaler> scaled;ComPtr<IWICFormatConverter> converted;
    if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory))) ||
       FAILED(factory->CreateStream(&stream)) || FAILED(stream->InitializeFromMemory(static_cast<BYTE*>(memory),bytes)) ||
       FAILED(factory->CreateDecoderFromStream(stream.Get(),nullptr,WICDecodeMetadataCacheOnLoad,&decoder)) ||
       FAILED(decoder->GetFrame(0,&frame)) || FAILED(factory->CreateBitmapScaler(&scaled)) ||
       FAILED(scaled->Initialize(frame.Get(),300,100,WICBitmapInterpolationModeFant)) || FAILED(factory->CreateFormatConverter(&converted)) ||
       FAILED(converted->Initialize(scaled.Get(),GUID_WICPixelFormat32bppPBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom)))return;
    std::vector<std::uint8_t> image(300*100*4);if(FAILED(converted->CopyPixels(nullptr,1200,static_cast<UINT>(image.size()),image.data())))return;
    pixels=std::move(image); // Owned CPU bytes only; no COM/Adobe objects survive.
} catch(...) {}
void launch(PF_InData* data) noexcept {
    Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!utility || !utility->AEGP_ExecuteScript)return;
    constexpr auto script="(function(){var id=app.findMenuCommandId('Starfield Presets');if(id)app.executeCommand(id);else alert('Open Starfield Particle Controls from Window > Extensions, then click Presets.');}())";
    (void)utility->AEGP_ExecuteScript(graph_carrier_plugin_id(),script,FALSE,nullptr,nullptr);
}
void draw(PF_InData* data,PF_EventExtra* event) {
    Suite<PF_EffectCustomUISuite2> ui(data->pica_basicP,kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2);
    Suite<DRAWBOT_DrawbotSuite1> bot(data->pica_basicP,kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1);
    Suite<DRAWBOT_SupplierSuite1> supplier(data->pica_basicP,kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1);
    Suite<DRAWBOT_SurfaceSuite1> surface(data->pica_basicP,kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1);
    if(!ui || !bot || !supplier || !surface)return;
    DRAWBOT_DrawRef ref{};DRAWBOT_SupplierRef source{};DRAWBOT_SurfaceRef target{};
    if(ui->PF_GetDrawingReference(event->contextH,&ref) || !ref || bot->GetSupplier(ref,&source) || bot->GetSurface(ref,&target) || !source || !target)return;
    const auto& frame=event->effect_win.current_frame;
    const int width=std::clamp(int(frame.right-frame.left)-4,30,300),height=width/3;
    const DRAWBOT_PointF32 origin{float(frame.left+2),float(frame.top+2)};
    if(disabled.size()>=64 && !disabled.contains(event->contextH))disabled.erase(disabled.begin());
    auto& failed=disabled[event->contextH];std::call_once(decoded,decode_picture);
    DRAWBOT_Boolean bgra{},argb{};
    const bool supports_bgra=supplier->SupportsPixelLayoutBGRA && !supplier->SupportsPixelLayoutBGRA(source,&bgra) && bgra;
    const bool supports_argb=supplier->SupportsPixelLayoutARGB && !supplier->SupportsPixelLayoutARGB(source,&argb) && argb;
    if(!failed && !pixels.empty() && (supports_bgra || supports_argb) && supplier->NewImageFromBuffer && surface->DrawImage) {
        failed=true; // Reentrant/failing host image calls cannot repeat a modal warning.
        std::vector<std::uint8_t> display(width*height*4);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            const auto* p=pixels.data()+((y*100/height)*300+x*300/width)*4;auto* q=display.data()+(y*width+x)*4;
            if(supports_bgra)std::copy_n(p,4,q);else {q[0]=p[3];q[1]=p[2];q[2]=p[1];q[3]=p[0];}
        }
        DRAWBOT_ImageRef image{};const auto error=supplier->NewImageFromBuffer(source,width,height,width*4,
            supports_bgra?kDRAWBOT_PixelLayout_32BGRA_Premul:kDRAWBOT_PixelLayout_32ARGB_Premul,display.data(),&image);
        if(!error && image && !surface->DrawImage(target,image,&origin,1))failed=false;
        if(image)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(image));
        if(!failed)return;
    }
    DRAWBOT_FontRef font{};DRAWBOT_BrushRef brush{};const DRAWBOT_ColorRGBA color{.85f,.9f,1,1};
    const std::array<DRAWBOT_UTF16Char,26> text{u'S',u'T',u'A',u'R',u'F',u'I',u'E',u'L',u'D',u' ',u'/',u' ',u'P',u'R',u'E',u'S',u'E',u'T',u'S',u' ',u' ',u'O',u'p',u'e',u'n',0};
    if(!supplier->NewDefaultFont(source,12,&font) && font && !supplier->NewBrush(source,&color,&brush) && brush) {
        const DRAWBOT_PointF32 at{origin.x+8,origin.y+30};(void)surface->DrawString(target,brush,font,text.data(),&at,kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_None,float(width));
    }
    if(brush)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));if(font)supplier->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));
}
}
void clear_main_presets_ui() noexcept {disabled.clear();}
PF_Err main_presets_event(PF_InData* data,PF_OutData*,PF_EventExtra* event) noexcept try {
    if(!data || !event)return PF_Err_NONE;
    if(event->e_type==PF_Event_CLOSE_CONTEXT){disabled.erase(event->contextH);return PF_Err_NONE;}
    // PF_Context's public window type is borrowed only during this callback.
    if(!event->contextH || !*event->contextH || (**event->contextH).w_type!=PF_Window_EFFECT ||
        event->effect_win.index!=1 || event->effect_win.area!=PF_EA_CONTROL)return PF_Err_NONE;
    if(event->e_type==PF_Event_DRAW)draw(data,event);
    else if(event->e_type==PF_Event_DO_CLICK)launch(data);
    else return PF_Err_NONE;
    event->evt_out_flags|=PF_EO_HANDLED_EVENT;return PF_Err_NONE;
} catch(...) {return PF_Err_NONE;}
}
