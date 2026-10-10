#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <bit>
#include <cstdint>

namespace starfield::adapter {
// Resolve the already loaded resident Host. Never load a library, register a
// hook, pump messages or probe scripting availability to acquire this scope.
// A missing or mismatched Host fails closed; publish only the full pairing.
class EffectUiExclusion final {
    using Begin=std::uint64_t(*)() noexcept;
    using End=void(*)(std::uint64_t) noexcept;
    End end_{};
    std::uint64_t token_{};
public:
    EffectUiExclusion() noexcept {
        const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return;
        const auto begin=std::bit_cast<Begin>(GetProcAddress(module,"SFLD_BeginUiExclusionV1"));
        end_=std::bit_cast<End>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));
        if(begin && end_)token_=begin();
    }
    ~EffectUiExclusion(){if(token_)end_(token_);}
    EffectUiExclusion(const EffectUiExclusion&) = delete;
    EffectUiExclusion& operator=(const EffectUiExclusion&) = delete;
    explicit operator bool() const noexcept{return token_!=0;}
};
}
