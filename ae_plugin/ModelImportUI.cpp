#include "ModelImportUI.hpp"
#include "UiExclusionClient.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <array>
#include <cstdio>
#include <fstream>
#include <string>

namespace starfield::adapter {
PF_Err import_model_obj(PF_InData* data,PF_OutData* out,PF_ParamDef* params[]) noexcept try {
    if(!data||!out||!data->pica_basicP)return PF_Err_BAD_CALLBACK_PARAM;
    EffectUiExclusion exclusion;
    if(!exclusion){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: UI transaction is busy or the paired Host is unavailable.");return PF_Err_BAD_CALLBACK_PARAM;}
    ModelImportCheckpoint checkpoint;const auto captured=capture_model_import_checkpoint(data,checkpoint);
    if(captured)return captured;
    const AEGP_UtilitySuite6* utility{};
    const auto error=data->pica_basicP->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,reinterpret_cast<const void**>(&utility));
    if(error||!utility)return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);
    HWND owner{};const auto owner_error=utility->AEGP_GetMainHWND(&owner);
    data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(owner_error||!owner)return static_cast<PF_Err>(owner_error?owner_error:PF_Err_BAD_CALLBACK_PARAM);
    std::array<wchar_t,32768> filename{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=owner;
    dialog.lpstrFilter=L"Wavefront OBJ (*.obj)\0*.obj\0\0";dialog.lpstrFile=filename.data();dialog.nMaxFile=static_cast<DWORD>(filename.size());
    dialog.lpstrTitle=L"Starfield: Import OBJ";dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER;
    if(!GetOpenFileNameW(&dialog)){if(!CommDlgExtendedError())return PF_Err_NONE;
        std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: file chooser failed.");return PF_Err_BAD_CALLBACK_PARAM;}
    const auto checked=validate_model_import_checkpoint(data,checkpoint);
    if(checked){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: target or Model author changed while the file chooser was open. Import cancelled.");return checked;}
    std::ifstream file(filename.data(),std::ios::binary|std::ios::ate);
    constexpr std::streamoff limit=8*1024*1024;const auto size=file?file.tellg():std::streampos{-1};
    if(size<=0||size>limit){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: file must contain 1 byte to 8 MiB.");return PF_Err_BAD_CALLBACK_PARAM;}
    std::string text(static_cast<std::size_t>(size),'\0');file.seekg(0);file.read(text.data(),static_cast<std::streamsize>(text.size()));
    if(!file||file.peek()!=std::char_traits<char>::eof()){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: file changed or could not be read.");return PF_Err_BAD_CALLBACK_PARAM;}
    core::NeverCancelled cancel;return import_model_obj_text(data,out,params,text,cancel,&checkpoint);
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
