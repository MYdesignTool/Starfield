#pragma once
#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"
#include <cstdint>
#include <string>

namespace starfield::adapter {
inline constexpr char preset_file_command_name[]="Starfield Choose Preset File";
void initialize_preset_file_host(SPBasicSuite*,AEGP_PluginID) noexcept;
void queue_preset_file_dialog() noexcept;
bool step_preset_file_host() noexcept;
void stop_preset_file_host() noexcept;
enum class PresetFileChoice : std::uint32_t {selected=0,cancelled=1,failed=2};
// Only owned text/numeric window identity crosses the modal boundary. This
// platform function must not execute scripts or retain AE suites/objects.
PresetFileChoice choose_preset_file(std::uintptr_t owner,bool save,std::u16string& path);
}
