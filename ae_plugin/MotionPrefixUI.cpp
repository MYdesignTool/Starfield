#include "MotionPrefixUI.hpp"
#include "MotionPrefix.hpp"
#include "NodeEffects.hpp"
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "SPBasic.h"
#include "AE_EffectSuites.h"
#include <adobesdk/DrawbotSuite.h>
#include <memory>
#include <vector>
#include <cstdio>

namespace starfield::adapter {
namespace {
namespace m=native_nodes::motion_layout;
constexpr auto kind=native_nodes::Kind::motion;
constexpr auto uuid_first=native_nodes::uuid_first_index(kind),guard_index=native_nodes::sync_guard_index(kind);
template<class T>struct Suite {
    SPBasicSuite* basic;const char* name;int version;const T* value{};
    Suite(SPBasicSuite* b,const char* n,int v):basic(b),name(n),version(v){basic->AcquireSuite(n,v,reinterpret_cast<const void**>(&value));}
    ~Suite(){if(value)basic->ReleaseSuite(name,version);}
};
bool checked_scalar(PF_InData* data,A_long index,double& out){
    PF_ParamDef p{};if(data->inter.checkout_param(data->effect_ref,index,data->current_time,0,data->time_scale,&p))return false;
    const bool scalar=p.param_type==PF_Param_FLOAT_SLIDER || p.param_type==PF_Param_POPUP;
    const double value=p.param_type==PF_Param_POPUP?p.u.pd.value:p.u.fs_d.value;
    const auto error=data->inter.checkin_param(data->effect_ref,&p);
    if(error || !scalar || !std::isfinite(value))return false;out=value;return true;
}
struct Snapshot {
    motion_prefix::Bank bank{};std::array<double,8> uuid{};double mode{},guard{};
    A_long project{},comp{},layer{},time{};A_u_long scale{};
    bool operator==(const Snapshot&) const=default;
};
bool capture(PF_InData* data,Snapshot& s){
    if(!checked_scalar(data,m::mode,s.mode) || (s.mode!=2 && s.mode!=3) ||
        !checked_scalar(data,guard_index,s.guard) || s.guard!=0)return false;
    bool any{};
    for(unsigned i=0;i<s.uuid.size();++i){auto& v=s.uuid[i];
        if(!checked_scalar(data,uuid_first+int(i),v) || std::floor(v)!=v || v<0 || v>65535)return false;any|=v!=0;}
    if(!any)return false;
    for(unsigned i=0;i<s.bank.size();++i)if(!checked_scalar(data,m::path_prefix_length+int(i),s.bank[i]))return false;
    std::wstring text;if(!motion_prefix::decode(s.bank,text))return false;
    auto* b=data->pica_basicP;
    Suite<AEGP_PFInterfaceSuite1> pf(b,kAEGPPFInterfaceSuite,kAEGPPFInterfaceSuiteVersion1);
    Suite<AEGP_LayerSuite9> layers(b,kAEGPLayerSuite,kAEGPLayerSuiteVersion9);
    Suite<AEGP_CompSuite11> comps(b,kAEGPCompSuite,kAEGPCompSuiteVersion11);
    Suite<AEGP_ItemSuite9> items(b,kAEGPItemSuite,kAEGPItemSuiteVersion9);
    Suite<AEGP_ProjSuite6> projects(b,kAEGPProjSuite,kAEGPProjSuiteVersion6);
    if(!pf.value || !layers.value || !comps.value || !items.value || !projects.value)return false;
    AEGP_ProjectH project{};AEGP_ItemH root{},item{};AEGP_LayerH layer{};AEGP_CompH comp{};A_Time time{};
    if(projects.value->AEGP_GetProjectByIndex(0,&project) || !project ||
        projects.value->AEGP_GetProjectRootFolder(project,&root) || !root ||
        items.value->AEGP_GetItemID(root,&s.project) || s.project<0 ||
        pf.value->AEGP_GetEffectLayer(data->effect_ref,&layer) || !layer ||
        layers.value->AEGP_GetLayerID(layer,&s.layer) || s.layer<=0 ||
        layers.value->AEGP_GetLayerParentComp(layer,&comp) || !comp ||
        comps.value->AEGP_GetItemFromComp(comp,&item) || !item ||
        items.value->AEGP_GetItemID(item,&s.comp) || s.comp<=0 ||
        layers.value->AEGP_GetLayerCurrentTime(layer,AEGP_LTimeMode_LayerTime,&time) || !time.scale ||
        std::int64_t(time.value)*data->time_scale!=std::int64_t(data->current_time)*time.scale)return false;
    s.time=time.value;s.scale=time.scale;return true;
}
struct Dialog {std::wstring text;};
INT_PTR CALLBACK dialog_proc(HWND window,UINT message,WPARAM wp,LPARAM lp) noexcept {
    auto* state=reinterpret_cast<Dialog*>(GetWindowLongPtrW(window,DWLP_USER));
    if(message==WM_INITDIALOG){
        state=reinterpret_cast<Dialog*>(lp);SetWindowLongPtrW(window,DWLP_USER,lp);
        RECT r{};GetClientRect(window,&r);
        const auto make=[&](const wchar_t* type,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h){
            auto control=CreateWindowExW(type==std::wstring_view(L"EDIT")?WS_EX_CLIENTEDGE:0,type,text,WS_CHILD|WS_VISIBLE|style,
                x,y,w,h,window,reinterpret_cast<HMENU>(INT_PTR(id)),GetModuleHandleW(nullptr),nullptr);
            if(control)SendMessageW(control,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);return control;};
        make(L"STATIC",L"Light name starts with (case-sensitive). Leave empty to match all.",0,102,12,10,r.right-24,32);
        const auto input=make(L"EDIT",state->text.c_str(),WS_TABSTOP|ES_AUTOHSCROLL,101,12,46,r.right-24,26);
        SendMessageW(input,EM_SETLIMITTEXT,m::path_prefix_bytes,0);
        make(L"STATIC",L"Light Path uses point lights with animated Position. Maximum: 128 UTF-8 bytes.",0,103,12,82,r.right-24,36);
        make(L"BUTTON",L"Apply",WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,r.right-180,128,78,26);
        make(L"BUTTON",L"Cancel",WS_TABSTOP,IDCANCEL,r.right-90,128,78,26);
        RECT owner{},self{};GetWindowRect(GetParent(window),&owner);GetWindowRect(window,&self);
        SetWindowPos(window,nullptr,owner.left+(owner.right-owner.left-self.right+self.left)/2,
            owner.top+(owner.bottom-owner.top-self.bottom+self.top)/2,0,0,SWP_NOSIZE|SWP_NOZORDER);
        SetFocus(input);SendMessageW(input,EM_SETSEL,0,-1);return FALSE;
    }
    if(!state)return FALSE;
    if(message==WM_CLOSE || (message==WM_COMMAND && LOWORD(wp)==IDCANCEL)){EndDialog(window,0);return TRUE;}
    if(message==WM_COMMAND && LOWORD(wp)==IDOK){
        std::array<wchar_t,m::path_prefix_bytes+1> text{};GetDlgItemTextW(window,101,text.data(),int(text.size()));
        motion_prefix::Bank bank{};if(!motion_prefix::encode(text.data(),bank)){
            SetDlgItemTextW(window,103,L"Enter valid text within 128 UTF-8 bytes. Shorten the prefix and try again.");return TRUE;}
        try{state->text=text.data();}catch(...){EndDialog(window,-1);return TRUE;}
        EndDialog(window,1);return TRUE;
    }
    return FALSE;
}
int choose(HWND owner,Dialog& state){
    std::vector<WORD> words;
    const auto dword=[&](DWORD v){words.push_back(WORD(v));words.push_back(WORD(v>>16));};
    const auto text=[&](const wchar_t* p){do{words.push_back(WORD(*p));}while(*p++);};
    dword(WS_POPUP|WS_CAPTION|WS_SYSMENU|DS_MODALFRAME|DS_SETFONT);dword(0);
    for(int n:{0,0,0,320,112})words.push_back(WORD(n));words.push_back(0);words.push_back(0);
    text(L"Starfield Motion - Starting With");words.push_back(9);text(L"Segoe UI");
    return int(DialogBoxIndirectParamW(GetModuleHandleW(nullptr),reinterpret_cast<DLGTEMPLATE*>(words.data()),owner,dialog_proc,reinterpret_cast<LPARAM>(&state)));
}
}
bool motion_prefix_preview(PF_InData* data,PF_ParamDef* params[],std::wstring& text,bool& enabled) noexcept try {
    if(!data)return false;motion_prefix::Bank bank{};double mode{};
    const auto read=[&](A_long index,double& value){
        if(params && data->num_params==native_nodes::parameter_count(kind)){
            const auto* p=params[index];if(!p)return false;
            if(index==m::mode && p->param_type==PF_Param_POPUP)value=p->u.pd.value;
            else if(p->param_type==PF_Param_FLOAT_SLIDER)value=p->u.fs_d.value;else return false;
            return std::isfinite(value);
        }
        return data->effect_ref && data->time_scale && data->inter.checkout_param && data->inter.checkin_param && checked_scalar(data,index,value);
    };
    if(!read(m::mode,mode))return false;enabled=mode==2 || mode==3;
    for(unsigned i=0;i<bank.size();++i)if(!read(m::path_prefix_length+int(i),bank[i]))return false;
    return motion_prefix::decode(bank,text);
}catch(...){return false;}

PF_Err edit_motion_prefix(PF_InData* data,PF_OutData* out) noexcept try {
    if(!data || !out || !data->effect_ref || !data->pica_basicP || !data->time_scale ||
        !data->inter.checkout_param || !data->inter.checkin_param || (data->in_flags&PF_InFlag_PROJECT_IS_RENDER_ONLY))return PF_Err_BAD_CALLBACK_PARAM;
    EffectUiExclusion scope;if(!scope)return PF_Err_BAD_CALLBACK_PARAM;
    Snapshot before;if(!capture(data,before))return PF_Err_BAD_CALLBACK_PARAM;
    Dialog state;if(!motion_prefix::decode(before.bank,state.text))return PF_Err_BAD_CALLBACK_PARAM;
    HWND owner{};
    {Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
        if(!utility.value || utility.value->AEGP_GetMainHWND(&owner) || !owner)return PF_Err_BAD_CALLBACK_PARAM;}
    const int chosen=choose(owner,state);if(chosen==0)return PF_Err_NONE;if(chosen!=1)return PF_Err_BAD_CALLBACK_PARAM;
    motion_prefix::Bank wanted;if(!motion_prefix::encode(state.text,wanted))return PF_Err_BAD_CALLBACK_PARAM;
    Snapshot after;if(!capture(data,after) || after!=before){
        std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield Starting With: target or prefix changed while the dialog was open. Please reopen it.");return PF_Err_BAD_CALLBACK_PARAM;}
    if(wanted==before.bank)return PF_Err_NONE;
    constexpr auto count=native_nodes::parameter_count(kind);
    auto values=std::make_unique<std::array<PF_ParamDef,count>>();std::array<PF_ParamDef*,count> pointers{};
    for(int i=0;i<count;++i){(*values)[i].param_type=PF_Param_FLOAT_SLIDER;pointers[i]=&(*values)[i];}
    for(unsigned i=0;i<before.uuid.size();++i)(*values)[uuid_first+i].u.fs_d.value=before.uuid[i];
    for(unsigned i=0;i<wanted.size();++i)(*values)[m::path_prefix_length+i].u.fs_d.value=wanted[i];
    auto owned=*data;owned.num_params=count;
    Suite<AEGP_UtilitySuite6> utility(data->pica_basicP,kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(!utility.value)return PF_Err_BAD_CALLBACK_PARAM;
    auto error=utility.value->AEGP_StartUndoGroup("Starfield: Starting With");if(error)return static_cast<PF_Err>(error);
    struct Undo {const AEGP_UtilitySuite6* s;bool open{true};~Undo(){if(open)s->AEGP_EndUndoGroup();}} undo{utility.value};
    error=sync_motion_prefix_graph_parameter(&owned,out,pointers.data(),before.bank,scope);
    const auto closed=utility.value->AEGP_EndUndoGroup();undo.open=false;if(!error)error=closed;
    if(!error)out->out_flags|=PF_OutFlag_REFRESH_UI|PF_OutFlag_FORCE_RERENDER;
    return static_cast<PF_Err>(error);
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}

PF_Err motion_prefix_event(PF_InData* data,PF_OutData* out,PF_ParamDef* params[],PF_EventExtra* event) noexcept try {
    if(!data || !out || !event || !event->contextH || !data->pica_basicP ||
        event->effect_win.index!=m::path_prefix_length ||
        event->effect_win.area!=PF_EA_CONTROL)return PF_Err_NONE;
    if(event->e_type!=PF_Event_DRAW && event->e_type!=PF_Event_DO_CLICK)return PF_Err_NONE;
    std::wstring prefix;bool enabled{};const bool valid=motion_prefix_preview(data,params,prefix,enabled);
    const auto& frame=event->effect_win.current_frame;
    if(event->e_type==PF_Event_DO_CLICK){
        event->evt_out_flags|=PF_EO_HANDLED_EVENT;
        const auto p=event->u.do_click.screen_point;
        if(event->effect_win.area==PF_EA_CONTROL && valid && enabled &&
            p.h>=frame.left && p.h<frame.right && p.v>=frame.top && p.v<frame.bottom)
            return edit_motion_prefix(data,out);
        return PF_Err_NONE;
    }
    // Drawbot references end with this draw callback; none enter the modal editor.
    auto* b=data->pica_basicP;
    Suite<PF_EffectCustomUISuite2> ui(b,kPFEffectCustomUISuite,kPFEffectCustomUISuiteVersion2);
    Suite<DRAWBOT_DrawbotSuite1> bot(b,kDRAWBOT_DrawSuite,kDRAWBOT_DrawSuite_Version1);
    Suite<DRAWBOT_SupplierSuite1> supplier(b,kDRAWBOT_SupplierSuite,kDRAWBOT_SupplierSuite_Version1);
    Suite<DRAWBOT_SurfaceSuite1> surface(b,kDRAWBOT_SurfaceSuite,kDRAWBOT_SurfaceSuite_Version1);
    if(!ui.value || !bot.value || !supplier.value || !surface.value)return PF_Err_NONE;
    DRAWBOT_DrawRef draw{};DRAWBOT_SupplierRef source{};DRAWBOT_SurfaceRef target{};
    if(ui.value->PF_GetDrawingReference(event->contextH,&draw) || !draw ||
        bot.value->GetSupplier(draw,&source) || !source || bot.value->GetSurface(draw,&target) || !target)return PF_Err_NONE;
    struct Objects {const DRAWBOT_SupplierSuite1* suite;DRAWBOT_FontRef font{};DRAWBOT_BrushRef brush{};
        ~Objects(){if(brush)suite->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(brush));
            if(font)suite->ReleaseObject(reinterpret_cast<DRAWBOT_ObjectRef>(font));}} objects{supplier.value};
    const float shade=valid && enabled?0.9f:0.5f;
    const DRAWBOT_ColorRGBA color{shade,shade,shade,1};
    if(supplier.value->NewDefaultFont(source,11,&objects.font) || !objects.font ||
        supplier.value->NewBrush(source,&color,&objects.brush) || !objects.brush)return PF_Err_NONE;
    const auto text=[&](const std::wstring& label,float y){
        const DRAWBOT_PointF32 origin{float(frame.left+4),y};
        surface.value->DrawString(target,objects.brush,objects.font,reinterpret_cast<const DRAWBOT_UTF16Char*>(label.c_str()),&origin,
            kDRAWBOT_TextAlignment_Left,kDRAWBOT_TextTruncation_End,float(frame.right-frame.left-8));};
    text(valid?enabled?L"Edit light name prefix...":L"Available in Light Path / Look At":L"Prefix unavailable",float(frame.top+14));
    if(valid && enabled)text(prefix.empty()?L"(all matching lights)":prefix,float(frame.top+34));
    event->evt_out_flags|=PF_EO_HANDLED_EVENT;return PF_Err_NONE;
}catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
