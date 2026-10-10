#include "PresetFileHost.hpp"
#define NOMINMAX
#include <Windows.h>
#include <commdlg.h>
#include <array>

namespace starfield::adapter {
PresetFileChoice choose_preset_file(std::uintptr_t owner,bool save,std::u16string& path) {
    std::array<wchar_t,32768> filename{};OPENFILENAMEW dialog{};
    dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=reinterpret_cast<HWND>(owner);
    dialog.lpstrFilter=save?L"Starfield presets (*.sfldpreset)\0*.sfldpreset\0\0":L"Starfield presets (*.sfldpreset; *.json)\0*.sfldpreset;*.json\0\0";
    dialog.lpstrFile=filename.data();dialog.nMaxFile=static_cast<DWORD>(filename.size());
    dialog.lpstrTitle=save?L"Save Starfield preset":L"Import Starfield preset";
    dialog.lpstrDefExt=save?L"sfldpreset":nullptr;
    dialog.Flags=OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_EXPLORER|(save?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    path.clear();
    if(!(save?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog)))return CommDlgExtendedError()?PresetFileChoice::failed:PresetFileChoice::cancelled;
    std::wstring selected(filename.data());
    if(save && (selected.size()<11 || _wcsicmp(selected.c_str()+selected.size()-11,L".sfldpreset"))) {
        selected+=L".sfldpreset";
        if(selected.size()>32767)return PresetFileChoice::failed;
        const auto attributes=GetFileAttributesW(selected.c_str());
        if(attributes!=INVALID_FILE_ATTRIBUTES) {
            if(attributes&FILE_ATTRIBUTE_DIRECTORY)return PresetFileChoice::failed;
            if(MessageBoxW(dialog.hwndOwner,(L"Replace the existing preset?\n"+selected).c_str(),L"Starfield",MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2)!=IDYES)return PresetFileChoice::cancelled;
        }else if(GetLastError()!=ERROR_FILE_NOT_FOUND && GetLastError()!=ERROR_PATH_NOT_FOUND)return PresetFileChoice::failed;
    }
    static_assert(sizeof(wchar_t)==sizeof(char16_t));
    for(const wchar_t c:selected)path+=static_cast<char16_t>(c);
    return PresetFileChoice::selected;
}
}
