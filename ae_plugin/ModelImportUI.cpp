#include "ModelImportUI.hpp"
#include "NodeRecord.hpp"
#include "UiExclusionClient.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <array>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <memory>
#include <string>

namespace starfield::adapter {
PF_Err import_model_obj(PF_InData* data,PF_OutData* out,PF_ParamDef*[]) noexcept try {
    if(!data||!out||!data->pica_basicP||!data->effect_ref||!data->time_scale||
       !data->inter.checkout_param||!data->inter.checkin_param)return PF_Err_BAD_CALLBACK_PARAM;
    EffectUiExclusion exclusion;
    if(!exclusion){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: UI transaction is busy or the paired Host is unavailable.");return PF_Err_BAD_CALLBACK_PARAM;}
    std::array<std::uint16_t,8> identity{};bool nonzero{};
    for(A_long i=0;i<8;++i){
        const auto index=native_nodes::uuid_first_index(native_nodes::Kind::model)+i;
        PF_ParamDef checked{};
        const auto read=data->inter.checkout_param(data->effect_ref,index,data->current_time,0,data->time_scale,&checked);
        if(read){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: Model identity checkout failed (stream %ld, error %d).",long(index),int(read));return read;}
        const auto value=checked.u.fs_d.value;
        const bool valid=checked.param_type==PF_Param_FLOAT_SLIDER && std::isfinite(value) && value>=0 && value<=65535 && std::floor(value)==value;
        const auto released=data->inter.checkin_param(data->effect_ref,&checked);
        if(released || !valid){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: invalid checked Model identity (stream %ld, error %d).",long(index),int(released));
            return released?released:PF_Err_BAD_CALLBACK_PARAM;}
        identity[i]=static_cast<std::uint16_t>(value);nonzero|=identity[i]!=0;
    }
    if(!nonzero){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: missing Model identity before file chooser.");return PF_Err_BAD_CALLBACK_PARAM;}
    ModelImportCheckpoint checkpoint;ModelImportDiagnostic diagnostic;
    const auto captured=capture_model_import_checkpoint(data,checkpoint,&diagnostic,&identity);
    if(captured){std::snprintf(out->return_msg,sizeof(out->return_msg),
        "Starfield OBJ: %s before file chooser (stream %ld, count %ld, error %d).",
        diagnostic.stage,long(diagnostic.stream),long(diagnostic.count),int(captured));return captured;}
    const AEGP_UtilitySuite6* utility{};
    const auto error=data->pica_basicP->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,reinterpret_cast<const void**>(&utility));
    if(error||!utility){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: AE utility suite unavailable (error %d).",int(error));
        return static_cast<PF_Err>(error?error:PF_Err_BAD_CALLBACK_PARAM);}
    HWND owner{};const auto owner_error=utility->AEGP_GetMainHWND(&owner);
    data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    if(owner_error||!owner){std::snprintf(out->return_msg,sizeof(out->return_msg),"Starfield OBJ: AE main window unavailable (error %d).",int(owner_error));
        return static_cast<PF_Err>(owner_error?owner_error:PF_Err_BAD_CALLBACK_PARAM);}
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
    // Own the transaction's narrow PF-style inputs. No checkout, callback
    // array or AEGP handle survives the modal interval. Host layout/UUID/time
    // are checked by the checkpoint; num_params below describes OUR array.
    constexpr auto count=native_nodes::parameter_count(native_nodes::Kind::model);
    static_assert(count==std::tuple_size_v<decltype(checkpoint.numbers)>);
    auto owned=std::make_unique<std::array<PF_ParamDef,count>>();
    std::array<PF_ParamDef*,count> pointers{};
    for(A_long i=0;i<count;++i){auto& p=(*owned)[i];p.param_type=PF_Param_FLOAT_SLIDER;p.u.fs_d.value=checkpoint.numbers[i];pointers[i]=&p;}
    namespace layout=native_nodes::model_layout;
    (*owned)[layout::source].param_type=PF_Param_POPUP;(*owned)[layout::source].u.pd.value=static_cast<A_long>(checkpoint.numbers[layout::source]);
    (*owned)[layout::revision].param_type=PF_Param_SLIDER;(*owned)[layout::revision].u.sd.value=static_cast<A_long>(checkpoint.numbers[layout::revision]);
    (*owned)[layout::mesh]={};(*owned)[layout::mesh].param_type=PF_Param_ARBITRARY_DATA;
    auto transaction_data=*data;transaction_data.num_params=count;
    core::NeverCancelled cancel;return import_model_obj_text(&transaction_data,out,pointers.data(),text,cancel,&checkpoint,&exclusion);
} catch(const std::bad_alloc&){return PF_Err_OUT_OF_MEMORY;}catch(...){return PF_Err_INTERNAL_STRUCT_DAMAGED;}
}
