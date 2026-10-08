#pragma once
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "AE_GeneralPlug.h"
#include <string>
#include <vector>
namespace starfield::adapter {
struct TransformLayerChoice {AEGP_LayerIDVal id{};std::u16string name;};
// Native Windows menu, opened only by an explicit selector click.
bool choose_transform_layer(PF_InData*,const std::vector<TransformLayerChoice>&,
                            AEGP_LayerIDVal current,AEGP_LayerIDVal& selected) noexcept;
PF_Err read_transform_layers(PF_InData*,std::vector<TransformLayerChoice>&,
                             AEGP_LayerIDVal& selected,bool full_inventory=true,A_long parameter_index=1) noexcept;
PF_Err set_transform_null_source(PF_InData*,PF_OutData*,PF_ParamDef*[],AEGP_LayerIDVal) noexcept;
PF_Err create_transform_null(PF_InData*,PF_OutData*,PF_ParamDef*[]) noexcept;
PF_Err transform_null_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
PF_Err texture_layer_event(PF_InData*,PF_OutData*,PF_ParamDef*[],PF_EventExtra*) noexcept;
}
