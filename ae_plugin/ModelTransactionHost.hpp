#pragma once
#include "AEConfig.h"
#include "AE_GeneralPlug.h"
#include "SPBasic.h"

namespace starfield::adapter {
inline constexpr char model_transaction_command_name[]="Starfield Apply Model Graph Transaction";
void initialize_model_transaction_host(SPBasicSuite*,AEGP_PluginID) noexcept;
void queue_model_graph_transaction() noexcept;
bool step_model_transaction_host() noexcept;
void stop_model_transaction_host() noexcept;
}
