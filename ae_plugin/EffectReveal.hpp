#pragma once
#include "AE_GeneralPlug.h"

namespace starfield::adapter {
// UI-only: un-hiding a stream without SKIP_REVEAL expands its parents and scrolls
// it into view. Restore visibility; never change values/keyframes/expressions.
inline A_Err reveal_stream(const AEGP_DynamicStreamSuite4* dynamic,AEGP_StreamRefH stream) noexcept {
    AEGP_DynStreamFlags flags{};
    auto error=dynamic->AEGP_GetDynamicStreamFlags(stream,&flags);
    if(error || (flags&(AEGP_DynStreamFlag_HIDDEN|AEGP_DynStreamFlag_ELIDED|
        AEGP_DynStreamFlag_SKIP_REVEAL_WHEN_UNHIDDEN)))return error;
    error=dynamic->AEGP_SetDynamicStreamFlag(stream,AEGP_DynStreamFlag_HIDDEN,FALSE,TRUE);
    if(!error) {
        error=dynamic->AEGP_SetDynamicStreamFlag(stream,AEGP_DynStreamFlag_HIDDEN,FALSE,FALSE);
        if(error)(void)dynamic->AEGP_SetDynamicStreamFlag(stream,AEGP_DynStreamFlag_HIDDEN,FALSE,FALSE);
    }
    return error;
}
}
