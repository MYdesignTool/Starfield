// Non-target resident services are deliberately inert in the bootstrap fixture.
// Their transactions have separate production fixtures. Unexpected commands fail.
#include "ModelAssetHost.hpp"
#include "ModelTransactionHost.hpp"
#include "PresetFileHost.hpp"
#include <cstdlib>
namespace starfield::adapter {
void initialize_model_asset_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_asset_export() noexcept {std::abort();}
bool step_model_asset_host() noexcept {return false;}
void stop_model_asset_host() noexcept {}
void initialize_model_transaction_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_model_graph_transaction() noexcept {std::abort();}
bool step_model_transaction_host() noexcept {return false;}
void stop_model_transaction_host() noexcept {}
void initialize_preset_file_host(SPBasicSuite*,AEGP_PluginID) noexcept {}
void queue_preset_file_dialog() noexcept {std::abort();}
bool step_preset_file_host() noexcept {return false;}
void stop_preset_file_host() noexcept {}
}
