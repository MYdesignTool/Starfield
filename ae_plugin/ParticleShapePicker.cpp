#include "ParticleShapeUI.hpp"
#include "ParticleShape.hpp"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#define NOMINMAX
#include <Windows.h>

namespace starfield::adapter {
bool choose_particle_shape(PF_InData* data,std::uint32_t current,std::uint32_t& selected) noexcept {
    if(!data || !data->pica_basicP)return false;
    const AEGP_UtilitySuite6* utility{};
    if(data->pica_basicP->AcquireSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6,
       reinterpret_cast<const void**>(&utility)) || !utility)return false;
    HWND owner{};const auto error=utility->AEGP_GetMainHWND(&owner);
    data->pica_basicP->ReleaseSuite(kAEGPUtilitySuite,kAEGPUtilitySuiteVersion6);
    POINT cursor{};if(error || !owner || !GetCursorPos(&cursor))return false;
    const HMENU menu=CreatePopupMenu();if(!menu)return false;
    struct Menu {HMENU handle;~Menu(){DestroyMenu(handle);}} owned{menu};
    for(const auto& choice:particle_shapes::choices) {
        const auto flags=MF_STRING|(choice.enabled?MF_ENABLED:MF_GRAYED)|
            (choice.native==current?MF_CHECKED:MF_UNCHECKED);
        if(!AppendMenuW(menu,flags,choice.native,reinterpret_cast<const wchar_t*>(choice.label)))return false;
    }
    const auto command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_NONOTIFY|TPM_LEFTALIGN|TPM_TOPALIGN,
                                    cursor.x,cursor.y,0,owner,nullptr);
    std::uint32_t core{};if(!particle_shapes::native_to_core(command,core))return false;
    selected=command;return true;
}
}
