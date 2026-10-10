#include "PresetFileHost.hpp"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
unsigned checks{},prompts{},attribute_reads{};bool save_expected{},accepted{};
std::wstring selected_name;DWORD extended_error{},observed_attributes{},last_error{};int prompt_result{};
#define CHECK(x) do{++checks;if(!(x)){std::cerr<<"chooser check "<<checks<<" line "<<__LINE__<<": "<<#x<<'\n';std::exit(1);}}while(false)
BOOL chooser(OPENFILENAMEW* dialog,bool save){CHECK(save==save_expected);CHECK(dialog->hwndOwner==reinterpret_cast<HWND>(1));
    CHECK(dialog->lStructSize==sizeof(*dialog));CHECK(dialog->nMaxFile==32768);CHECK(dialog->Flags&OFN_NOCHANGEDIR);CHECK(dialog->Flags&OFN_PATHMUSTEXIST);CHECK(dialog->Flags&OFN_EXPLORER);
    CHECK(!(dialog->Flags&OFN_ALLOWMULTISELECT));CHECK(bool(dialog->Flags&OFN_OVERWRITEPROMPT)==save);CHECK(bool(dialog->Flags&OFN_FILEMUSTEXIST)==!save);
    CHECK(save?std::wstring(dialog->lpstrDefExt)==L"sfldpreset":dialog->lpstrDefExt==nullptr);
    if(!accepted)return FALSE;CHECK(selected_name.size()<dialog->nMaxFile);std::wcscpy(dialog->lpstrFile,selected_name.c_str());return TRUE;}
BOOL fake_open(OPENFILENAMEW* dialog){return chooser(dialog,false);}BOOL fake_save(OPENFILENAMEW* dialog){return chooser(dialog,true);}
DWORD extended(){return extended_error;}DWORD file_observed_attributes(LPCWSTR path){++attribute_reads;CHECK(std::wstring(path).ends_with(L".sfldpreset"));return observed_attributes;}
DWORD error(){return last_error;}int message(HWND owner,LPCWSTR text,LPCWSTR title,UINT flags){++prompts;CHECK(owner==reinterpret_cast<HWND>(1));
    CHECK(std::wstring(title)==L"Starfield");CHECK(std::wstring(text).find(selected_name+L".sfldpreset")!=std::wstring::npos);
    CHECK(flags==(MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2));return prompt_result;}
void reset(bool is_save=true){save_expected=is_save;accepted=true;selected_name=L"D:/preset";extended_error=0;observed_attributes=INVALID_FILE_ATTRIBUTES;last_error=ERROR_FILE_NOT_FOUND;prompt_result=IDYES;prompts=attribute_reads=0;}
}
#define GetOpenFileNameW fake_open
#define GetSaveFileNameW fake_save
#define CommDlgExtendedError extended
#define GetFileAttributesW file_observed_attributes
#define GetLastError error
#define MessageBoxW message
#include "../ae_plugin/PresetFileChooser.cpp"
#undef GetOpenFileNameW
#undef GetSaveFileNameW
#undef CommDlgExtendedError
#undef GetFileAttributesW
#undef GetLastError
#undef MessageBoxW

int main(){using namespace starfield::adapter;std::u16string path;
    reset(false);selected_name=L"D:/\u6d4b\u8bd5/\U0001f600.json";CHECK(choose_preset_file(1,false,path)==PresetFileChoice::selected);CHECK(path==u"D:/\u6d4b\u8bd5/\U0001f600.json");CHECK(attribute_reads==0);CHECK(prompts==0);
    reset();CHECK(choose_preset_file(1,true,path)==PresetFileChoice::selected);CHECK(path==u"D:/preset.sfldpreset");CHECK(attribute_reads==1);CHECK(prompts==0);
    reset();selected_name=L"D:/preset.SfLdPrEsEt";CHECK(choose_preset_file(1,true,path)==PresetFileChoice::selected);CHECK(path==u"D:/preset.SfLdPrEsEt");CHECK(attribute_reads==0);CHECK(prompts==0);
    reset();observed_attributes=FILE_ATTRIBUTE_ARCHIVE;CHECK(choose_preset_file(1,true,path)==PresetFileChoice::selected);CHECK(prompts==1);
    for(int result:{IDNO,IDCANCEL,0}){reset();observed_attributes=FILE_ATTRIBUTE_ARCHIVE;prompt_result=result;CHECK(choose_preset_file(1,true,path)==PresetFileChoice::cancelled);CHECK(path.empty());CHECK(prompts==1);}
    reset();observed_attributes=FILE_ATTRIBUTE_DIRECTORY;CHECK(choose_preset_file(1,true,path)==PresetFileChoice::failed);CHECK(path.empty());CHECK(prompts==0);
    reset();last_error=ERROR_ACCESS_DENIED;CHECK(choose_preset_file(1,true,path)==PresetFileChoice::failed);CHECK(path.empty());
    reset();last_error=ERROR_PATH_NOT_FOUND;CHECK(choose_preset_file(1,true,path)==PresetFileChoice::selected);
    reset();selected_name=L"D:/"+std::wstring(32760,L'x');CHECK(choose_preset_file(1,true,path)==PresetFileChoice::failed);CHECK(path.empty());CHECK(attribute_reads==0);
    for(bool is_save:{false,true}){reset(is_save);accepted=false;path=u"old";CHECK(choose_preset_file(1,is_save,path)==PresetFileChoice::cancelled);CHECK(path.empty());CHECK(attribute_reads==0);
        extended_error=FNERR_BUFFERTOOSMALL;CHECK(choose_preset_file(1,is_save,path)==PresetFileChoice::failed);CHECK(path.empty());}
    std::cout<<"preset_file_chooser_tests: "<<checks<<" checks passed; actual chooser with fake Windows file/dialog APIs\n";
}
