#pragma once
#include "AE_Effect.h"
#include "AE_GeneralPlug.h"
#include "starfield/core/PluginApi.h"
#include <vector>
namespace starfield::adapter {
struct LightStaging {
    std::vector<SfModelLight> lights;
    double layer_to_world[16]{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    double camera_position[3]{};
    bool camera_enabled{};
};
PF_Err capture_lights(PF_InData*,AEGP_PluginID,bool needed,LightStaging&) noexcept;
void apply_lights(SfCoreRenderRequest&,const LightStaging&) noexcept;
}
