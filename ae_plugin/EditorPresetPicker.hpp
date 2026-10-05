#pragma once
#include "AE_Effect.h"
#include "starfield/core/ColorGradient.hpp"
#include "starfield/core/Settings.hpp"
namespace starfield::adapter {
bool choose_gradient_preset(PF_InData*,core::ColorGradient&) noexcept;
bool choose_curve_preset(PF_InData*,core::AgeCurve&) noexcept;
}
