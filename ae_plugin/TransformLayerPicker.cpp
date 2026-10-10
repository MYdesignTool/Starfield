#include "TransformNullUI.hpp"
#include "SPBasic.h"
#define NOMINMAX
#include <Windows.h>
#include "UiExclusionClient.hpp"

namespace starfield::adapter {
bool choose_transform_layer(PF_InData* data,const std::vector<TransformLayerChoice>& choices,
                            AEGP_LayerIDVal current,AEGP_LayerIDVal& selected) noexcept try {
    if(!data || !data->pica_basicP || choices.empty() || choices.size()>4097)return false;
    EffectUiExclusion exclusion;if(!exclusion)return false;
    const AEGP_UtilitySuite6* utility{};
    if(data->pica_basicP->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,
       reinterpret_cast<const void**>(&utility)) || !utility)return false;
    HWND owner{};const auto error=utility->AEGP_GetMainHWND(&owner);
    data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    POINT cursor{};if(error || !owner || !GetCursorPos(&cursor))return false;
    const HMENU menu=CreatePopupMenu();if(!menu)return false;
    struct Menu {HMENU handle;~Menu(){DestroyMenu(handle);}} owned{menu};
    for(std::size_t i=0;i<choices.size();++i) {
        const auto flags=MF_STRING|(choices[i].id==current?MF_CHECKED:MF_UNCHECKED);
        std::wstring label;
        for(const auto character:choices[i].name){label+=static_cast<wchar_t>(character);if(character==u'&')label+=L'&';}
        if(!AppendMenuW(menu,flags,i+1,label.c_str()))return false;
    }
    const auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY|TPM_LEFTALIGN|TPM_TOPALIGN,
                                     cursor.x,cursor.y,0,owner,nullptr);
    if(command<1 || static_cast<std::size_t>(command)>choices.size())return false;
    selected=choices[command-1].id;return true;
}catch(...){return false;}
}
