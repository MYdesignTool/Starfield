#pragma once
#include <array>
#include <cmath>
#include <cstdint>

namespace starfield::adapter::particle_shapes {
// Public popup order follows the owner's complete reference menu. Core Model4
// predates this unpublished author popup and must not be mistaken for Face5.
struct Choice { const char16_t* label; std::uint32_t native; bool enabled; };
inline constexpr std::array<Choice,6> choices{{
    {u"Circle",1,true},{u"Rectangle",2,true},{u"Cloud",3,true},
    {u"Texture",4,true},{u"Face",5,false},{u"Model",6,true}}};
inline constexpr const char* popup_names="Circle|Rectangle|Cloud|Texture|Face|Model";
inline bool native_to_core(double native,std::uint32_t& core) noexcept {
    if(!std::isfinite(native) || std::floor(native)!=native)return false;
    if(native>=1 && native<=4){core=static_cast<std::uint32_t>(native)-1;return true;}
    if(native==6){core=4;return true;}
    return false; // Face has no OBJ face-emission implementation yet.
}
inline bool core_to_native(std::uint32_t core,std::uint32_t& native) noexcept {
    if(core>4)return false;
    native=core==4?6:core+1;return true;
}
}
