#include "EditorPresetPicker.hpp"
#include "EditorPresetCatalog.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#define NOMINMAX
#include <Windows.h>
#include <algorithm>
#include <iterator>
#include <vector>

namespace starfield::adapter {
namespace {
struct Picker {bool gradients{};int selected{};HBRUSH background{};};
constexpr int first_card=100;
int count(const Picker& p) noexcept {return static_cast<int>(p.gradients?std::size(editor_presets::gradients):std::size(editor_presets::curves));}
void fill(HDC dc,RECT r,COLORREF color) noexcept {
    auto brush=CreateSolidBrush(color);if(brush){FillRect(dc,&r,brush);DeleteObject(brush);}
}
void card(const DRAWITEMSTRUCT& item,const Picker& p) noexcept {
    const int index=static_cast<int>(item.CtlID)-first_card;
    if(index<0 || index>=count(p))return;
    auto r=item.rcItem;const auto dc=item.hDC;
    fill(dc,r,RGB(25,25,25));
    auto frame=CreateSolidBrush(index==p.selected?RGB(142,203,230):RGB(70,70,70));
    if(frame){FrameRect(dc,&r,frame);DeleteObject(frame);}
    RECT plot{r.left+9,r.top+9,r.right-9,r.bottom-31};
    fill(dc,plot,RGB(45,45,45));
    const int width=plot.right-plot.left,height=plot.bottom-plot.top;
    if(p.gradients) {
        const auto& value=editor_presets::gradients[index].value;
        for(int x=0;x<width;++x){
            const auto color=core::evaluate_color_gradient(value,double(x)/std::max(1,width-1));
            fill(dc,{plot.left+x,plot.top,plot.left+x+1,plot.bottom},RGB(int(color.x*255+.5),int(color.y*255+.5),int(color.z*255+.5)));
        }
    } else {
        const auto& value=editor_presets::curves[index].value;
        auto pen=CreatePen(PS_SOLID,1,RGB(207,207,207));
        if(pen){
            const auto old=SelectObject(dc,pen);
            for(int x=0;x<width;++x){
                const auto y=core::evaluate_age_curve(value,double(x)/std::max(1,width-1),100,100);
                const int py=plot.bottom-2-static_cast<int>(y/100*(height-4));
                if(x==0)MoveToEx(dc,plot.left+x,py,nullptr);else LineTo(dc,plot.left+x,py);
            }
            // Dense Draw presets keep the plot readable without 64 marker boxes.
            if(value.count<=8)for(unsigned i=0;i<value.count;++i){
                const auto& point=value.points[i];
                const int x=plot.left+static_cast<int>(point.age*(width-1));
                const int y=plot.bottom-2-static_cast<int>(point.value/100*(height-4));
                const auto brush=SelectObject(dc,GetStockObject(HOLLOW_BRUSH));
                Ellipse(dc,x-3,y-3,x+4,y+4);SelectObject(dc,brush);
            }
            SelectObject(dc,old);DeleteObject(pen);
        }
    }
    const char* name=p.gradients?editor_presets::gradients[index].name:editor_presets::curves[index].name;
    RECT label{r.left+4,r.bottom-28,r.right-4,r.bottom-3};
    SetTextColor(dc,RGB(221,221,221));SetBkMode(dc,TRANSPARENT);
    const auto oldFont=SelectObject(dc,GetStockObject(DEFAULT_GUI_FONT));
    DrawTextA(dc,name,-1,&label,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_END_ELLIPSIS);
    SelectObject(dc,oldFont);
    if(item.itemState&ODS_FOCUS){auto focus=r;InflateRect(&focus,-3,-3);DrawFocusRect(dc,&focus);}
}
INT_PTR CALLBACK dialog(HWND window,UINT message,WPARAM wp,LPARAM lp) noexcept {
    auto* picker=reinterpret_cast<Picker*>(GetWindowLongPtrW(window,DWLP_USER));
    if(message==WM_INITDIALOG) {
        picker=reinterpret_cast<Picker*>(lp);SetWindowLongPtrW(window,DWLP_USER,lp);
        SetWindowTextW(window,picker->gradients?L"Starfield Color Gradient Presets":L"Starfield Over Life Presets");
        MONITORINFO available{sizeof(MONITORINFO)};
        if(GetMonitorInfoW(MonitorFromWindow(GetParent(window),MONITOR_DEFAULTTONEAREST),&available)) {
            RECT frame{};GetWindowRect(window,&frame);
            const auto width=std::min(frame.right-frame.left,available.rcWork.right-available.rcWork.left-24);
            const auto height=std::min(frame.bottom-frame.top,available.rcWork.bottom-available.rcWork.top-24);
            SetWindowPos(window,nullptr,0,0,width,height,SWP_NOMOVE|SWP_NOZORDER);
        }
        RECT r{};GetClientRect(window,&r);
        const int gap=10,columns=5,rows=(count(*picker)+columns-1)/columns;
        const int width=(r.right-gap*(columns+1))/columns,height=(r.bottom-54-gap*(rows+1))/rows;
        for(int i=0;i<count(*picker);++i){
            const auto control=CreateWindowExW(0,L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW|BS_NOTIFY,
                gap+(i%columns)*(width+gap),gap+(i/columns)*(height+gap),width,height,window,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(first_card+i)),GetModuleHandleW(nullptr),nullptr);
            if(control)SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE);
        }
        for(int id:{IDCANCEL,IDOK}){
            auto control=CreateWindowExW(0,L"BUTTON",id==IDOK?L"Apply":L"Cancel",WS_CHILD|WS_VISIBLE|WS_TABSTOP|(id==IDOK?BS_DEFPUSHBUTTON:0),
                r.right-(id==IDOK?100:200),r.bottom-40,90,28,window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
            if(control)SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),FALSE);
        }
        RECT owner{},self{};GetWindowRect(GetParent(window),&owner);GetWindowRect(window,&self);
        MONITORINFO monitor{sizeof(MONITORINFO)};GetMonitorInfoW(MonitorFromWindow(GetParent(window),MONITOR_DEFAULTTONEAREST),&monitor);
        const int x=std::clamp<LONG>(owner.left+(owner.right-owner.left-(self.right-self.left))/2,
            monitor.rcWork.left,std::max(monitor.rcWork.left,monitor.rcWork.right-(self.right-self.left)));
        const int y=std::clamp<LONG>(owner.top+(owner.bottom-owner.top-(self.bottom-self.top))/2,
            monitor.rcWork.top,std::max(monitor.rcWork.top,monitor.rcWork.bottom-(self.bottom-self.top)));
        SetWindowPos(window,nullptr,x,y,0,0,SWP_NOSIZE|SWP_NOZORDER);
        SetFocus(GetDlgItem(window,first_card));return FALSE;
    }
    if(!picker)return FALSE;
    if(message==WM_DRAWITEM){card(*reinterpret_cast<DRAWITEMSTRUCT*>(lp),*picker);return TRUE;}
    if(message==WM_CTLCOLORDLG)return reinterpret_cast<INT_PTR>(picker->background);
    if(message==WM_COMMAND){
        const int id=LOWORD(wp),notification=HIWORD(wp);
        if(id==IDCANCEL){EndDialog(window,-1);return TRUE;}
        if(id==IDOK){EndDialog(window,picker->selected+1);return TRUE;}
        if(id>=first_card && id<first_card+count(*picker) && (notification==BN_CLICKED || notification==BN_DOUBLECLICKED || notification==BN_SETFOCUS)){
            const auto previous=picker->selected;picker->selected=id-first_card;
            InvalidateRect(GetDlgItem(window,first_card+previous),nullptr,FALSE);
            InvalidateRect(GetDlgItem(window,id),nullptr,FALSE);
            if(notification==BN_DOUBLECLICKED)EndDialog(window,picker->selected+1);
            return TRUE;
        }
    }
    if(message==WM_CLOSE){EndDialog(window,-1);return TRUE;}
    return FALSE;
}
int choose(PF_InData* data,bool gradients) {
    if(!data || !data->pica_basicP)return -1;
    const AEGP_UtilitySuite6* utility{};
    if(data->pica_basicP->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,reinterpret_cast<const void**>(&utility)) || !utility)return -1;
    HWND owner{};const auto error=utility->AEGP_GetMainHWND(&owner);
    data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(error || !owner)return -1;
    // Runtime DLGTEMPLATE: no resource identities or external executable.
    std::vector<WORD> words;
    const auto dword=[&](DWORD v){words.push_back(static_cast<WORD>(v));words.push_back(static_cast<WORD>(v>>16));};
    dword(WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_SETFONT);dword(0);
    for(int v:{0,0,0,570,410})words.push_back(static_cast<WORD>(v)); // controls, x, y, width, height
    words.push_back(0);words.push_back(0);
    const auto string=[&](const wchar_t* text){do{words.push_back(static_cast<WORD>(*text));}while(*text++);};
    string(L"Starfield Editor Presets");words.push_back(9);string(L"Segoe UI");
    Picker picker{gradients,0,CreateSolidBrush(RGB(25,25,25))};
    const auto result=DialogBoxIndirectParamW(GetModuleHandleW(nullptr),reinterpret_cast<const DLGTEMPLATE*>(words.data()),owner,dialog,reinterpret_cast<LPARAM>(&picker));
    if(picker.background)DeleteObject(picker.background);
    return result>0 && result<=count(picker)?static_cast<int>(result)-1:-1;
}
}
bool choose_gradient_preset(PF_InData* data,core::ColorGradient& value) noexcept try {
    const int index=choose(data,true);if(index<0)return false;
    value=editor_presets::gradients[index].value;return true;
} catch(...){return false;}
bool choose_curve_preset(PF_InData* data,core::AgeCurve& value) noexcept try {
    const int index=choose(data,false);if(index<0)return false;
    value=editor_presets::curves[index].value;return true;
} catch(...){return false;}
}
