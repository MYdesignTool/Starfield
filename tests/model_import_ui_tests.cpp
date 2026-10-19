#include "ModelImportUI.hpp"
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>

namespace {
using namespace starfield::adapter;
namespace layout=native_nodes::model_layout;
unsigned checks{},checkouts{},checkins{},held{},dialogs{},captures{},validations{},imports{};
A_long fail_checkout=-1,fail_checkin=-1,bad_type=-1,bad_value=-1;
bool empty_uuid{},cancel_dialog{},stale{},capture_failure{};
constexpr auto first=native_nodes::uuid_first_index(native_nodes::Kind::model);
#define CHECK(x) do{++checks;if(!(x)){std::printf("FAIL %d: %s\n",__LINE__,#x);std::exit(1);}}while(false)
PF_Err checkout(PF_ProgPtr,PF_ParamIndex index,A_long time,A_long step,A_u_long scale,PF_ParamDef* out){
    CHECK(index>=first && index<first+8 && time==12 && step==0 && scale==24);++checkouts;
    if(index==fail_checkout)return 512;++held;*out={};out->param_type=index==bad_type?PF_Param_POPUP:PF_Param_FLOAT_SLIDER;
    out->u.fs_d.value=empty_uuid?0:double(index-first+1);if(index==bad_value)out->u.fs_d.value=std::numeric_limits<double>::quiet_NaN();return 0;
}
PF_Err checkin(PF_ProgPtr,PF_ParamDef*){CHECK(held==1);--held;++checkins;return A_long(first+checkins-1)==fail_checkin?512:0;}
AEGP_UtilitySuite6 test_utility{};
SPAPI SPErr acquire(const char* name,int32,const void** out){CHECK(!std::strcmp(name,kAEGPUtilitySuite));*out=&test_utility;return 0;}
SPAPI SPErr release(const char*,int32){return 0;}
BOOL WINAPI chooser(OPENFILENAMEW* dialog){CHECK(held==0 && captures==1 && ui_transaction_active());++dialogs;
    CHECK(dialog->hwndOwner==reinterpret_cast<HWND>(1));if(cancel_dialog)return FALSE;
    const wchar_t* file=L"artifacts/model-import-ui-tests/input.obj";std::wcscpy(dialog->lpstrFile,file);return TRUE;}
DWORD WINAPI chooser_error(){return 0;}
void reset(){checkouts=checkins=held=dialogs=captures=validations=imports=0;fail_checkout=fail_checkin=bad_type=bad_value=-1;
    empty_uuid=cancel_dialog=stale=capture_failure=false;}
}
namespace starfield::adapter {
PF_Err capture_model_import_checkpoint(PF_InData*,ModelImportCheckpoint& out,ModelImportDiagnostic*,const std::array<std::uint16_t,8>* id) noexcept {
    CHECK(held==0 && id);++captures;for(unsigned i=0;i<8;++i)CHECK((*id)[i]==i+1);
    if(capture_failure)return 516;out={};out.numbers[layout::source]=1;out.numbers[layout::revision]=7;
    for(unsigned i=0;i<8;++i)out.numbers[first+i]=(*id)[i];return 0;
}
PF_Err validate_model_import_checkpoint(PF_InData*,const ModelImportCheckpoint&) noexcept {CHECK(held==0);++validations;return stale?516:0;}
PF_Err import_model_obj_text(PF_InData* data,PF_OutData*,PF_ParamDef* params[],std::string_view text,
    const core::Cancellation&,const ModelImportCheckpoint* checkpoint,const EffectUiExclusion* owner) noexcept {
    CHECK(data->num_params==101 && checkpoint && owner && *owner && ui_transaction_active() && held==0);
    CHECK(params && params[100] && params[layout::source]->param_type==PF_Param_POPUP && params[layout::source]->u.pd.value==1);
    CHECK(params[layout::revision]->param_type==PF_Param_SLIDER && params[layout::revision]->u.sd.value==7);
    CHECK(params[layout::mesh]->param_type==PF_Param_ARBITRARY_DATA && !params[layout::mesh]->u.arb_d.value);
    CHECK(text=="v 0 0 0\n");for(unsigned i=0;i<8;++i)CHECK(params[first+i]->u.fs_d.value==i+1);++imports;return 0;
}
}
// Execute the production UI path; only the actual Windows chooser is replaced.
#define GetOpenFileNameW chooser
#define CommDlgExtendedError chooser_error
#include "../ae_plugin/ModelImportUI.cpp"
#undef GetOpenFileNameW
#undef CommDlgExtendedError
int main(){
    auto module=LoadLibraryW(L"artifacts/model-ui-exclusion-tests/paired/StarfieldHost.aex");CHECK(module);
    const auto initialize=std::bit_cast<void(*)() noexcept>(GetProcAddress(module,"SFLD_TestInitialize"));CHECK(initialize);initialize();
    std::ofstream("artifacts/model-import-ui-tests/input.obj",std::ios::binary)<<"v 0 0 0\n";
    test_utility.AEGP_GetMainHWND=[](void* out)->A_Err{*static_cast<HWND*>(out)=reinterpret_cast<HWND>(1);return 0;};
    SPBasicSuite basic{};basic.AcquireSuite=acquire;basic.ReleaseSuite=release;
    PF_InData data{};data.pica_basicP=&basic;data.effect_ref=reinterpret_cast<PF_ProgPtr>(1);data.time_scale=24;data.current_time=12;
    data.inter.checkout_param=checkout;data.inter.checkin_param=checkin;
    for(auto count:{0,1,100,101}){reset();data.num_params=count;PF_OutData out{};
        CHECK(starfield::adapter::import_model_obj(&data,&out,reinterpret_cast<PF_ParamDef**>(1))==0);
        CHECK(data.num_params==count && imports==1 && dialogs==1 && checkouts==8 && checkins==8 && !held && !ui_transaction_active());}
    for(A_long index=first;index<first+8;++index)for(int failure=0;failure<4;++failure){reset();
        if(failure==0)fail_checkout=index;if(failure==1)fail_checkin=index;if(failure==2)bad_type=index;if(failure==3)bad_value=index;
        PF_OutData out{};CHECK(starfield::adapter::import_model_obj(&data,&out,nullptr)!=0);
        CHECK(!held && dialogs==0 && imports==0 && !ui_transaction_active());}
    for(int failure=0;failure<4;++failure){reset();empty_uuid=failure==0;capture_failure=failure==1;cancel_dialog=failure==2;stale=failure==3;
        PF_OutData out{};const auto error=starfield::adapter::import_model_obj(&data,&out,nullptr);
        CHECK((error==0)==cancel_dialog);CHECK(imports==0 && !held && !ui_transaction_active());}
    CHECK(FreeLibrary(module));std::printf("Model import UI: %u checks, 0 failures; zero-count actual UI path, owned inputs, no real dialog or AE\n",checks);
}
