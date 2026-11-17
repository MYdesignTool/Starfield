// Non-editor fake hosts never display native modal preset windows.
#include "EditorPresetPicker.hpp"
#if defined(STARFIELD_OVERLIFE_TEST)
#include "../artifacts/generated/EditorPresetCatalog.hpp"
#include "UiExclusionClient.hpp"
#include <iterator>
#endif
namespace starfield::adapter {
#if defined(STARFIELD_OVERLIFE_TEST)
int overlife_preset_index=-1;
bool choose_curve_preset(PF_InData*,core::AgeCurve& value) noexcept {
    EffectUiExclusion modal;
    if(!modal || overlife_preset_index<0 || static_cast<unsigned>(overlife_preset_index)>=std::size(editor_presets::curves))return false;
    value=editor_presets::curves[overlife_preset_index].value;return true;
}
#else
bool choose_curve_preset(PF_InData*, core::AgeCurve&) noexcept { return false; }
#endif
bool choose_gradient_preset(PF_InData*, core::ColorGradient&) noexcept { return false; }
}
