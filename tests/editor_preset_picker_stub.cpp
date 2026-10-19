// Non-editor fake hosts never display native modal preset windows.
#include "EditorPresetPicker.hpp"
namespace starfield::adapter {
bool choose_curve_preset(PF_InData*, core::AgeCurve&) noexcept { return false; }
bool choose_gradient_preset(PF_InData*, core::ColorGradient&) noexcept { return false; }
}
