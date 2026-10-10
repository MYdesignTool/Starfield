#pragma once
#include "AE_GeneralPlug.h"

namespace starfield::adapter {
// ExecuteScript's optional diagnostic is a string handle. A returned handle
// may own an empty string even on success; its address is not an error flag.
// Ownership stays with the caller and no lock survives this read.
inline bool script_diagnostic_empty(const AEGP_MemorySuite1* memory,AEGP_MemHandle diagnostic) noexcept {
    if(!diagnostic)return true;
    if(!memory || !memory->AEGP_GetMemHandleSize || !memory->AEGP_LockMemHandle || !memory->AEGP_UnlockMemHandle)return false;
    AEGP_MemSize size{};
    if(memory->AEGP_GetMemHandleSize(diagnostic,&size) || size>8192)return false;
    if(!size)return true;
    void* bytes{};
    if(memory->AEGP_LockMemHandle(diagnostic,&bytes))return false;
    const bool empty=bytes && *static_cast<const char*>(bytes)==0;
    return !memory->AEGP_UnlockMemHandle(diagnostic) && empty;
}
}
