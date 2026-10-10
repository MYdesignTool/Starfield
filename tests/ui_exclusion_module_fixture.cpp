#ifndef STARFIELD_MISMATCHED_HOST
#include "UiExclusionHost.hpp"
extern "C" __declspec(dllexport) void SFLD_TestInitialize() noexcept {starfield::adapter::initialize_ui_exclusion();}
extern "C" __declspec(dllexport) void SFLD_TestStop() noexcept {starfield::adapter::stop_ui_exclusion();}
#else
extern "C" __declspec(dllexport) void SFLD_TestMismatchedHost() noexcept {}
#endif
