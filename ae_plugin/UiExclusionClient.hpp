#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <bit>
#include <cstdint>
#include "SourceEditTelemetry.hpp"

namespace starfield::adapter {
// Read-only, session-local query; no acquisition, SDK call or message pumping.
inline bool ui_transaction_active() noexcept {
    using Query=bool(*)() noexcept;
    const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return false;
    const auto query=std::bit_cast<Query>(GetProcAddress(module,"SFLD_UiExclusionActiveV1"));
    return query && query();
}
inline std::uint64_t graph_mutation_epoch() noexcept {
    using Query=std::uint64_t(*)() noexcept;
    const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return 0;
    const auto query=std::bit_cast<Query>(GetProcAddress(module,"SFLD_GraphMutationEpochV1"));
    return query?query():0;
}
inline bool source_ui_transaction_active() noexcept {
    using Query=bool(*)() noexcept;
    const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return false;
    const auto query=std::bit_cast<Query>(GetProcAddress(module,"SFLD_SourceUiExclusionActiveV1"));
    return query && query();
}
inline void record_source_edit(const SourceEditTelemetry& value) noexcept {
    using Record=void(*)(const SourceEditTelemetry*) noexcept;
    const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return;
    const auto record=std::bit_cast<Record>(GetProcAddress(module,"SFLD_RecordSourceEditV1"));
    if(record)record(&value);
}
// Resolve the already loaded resident Host. Never load a library, register a
// hook, pump messages or probe scripting availability to acquire this scope.
// A missing or mismatched Host fails closed; publish only the full pairing.
class EffectUiExclusion final {
    using Begin=std::uint64_t(*)() noexcept;
    using End=void(*)(std::uint64_t) noexcept;
    End end_{};
    std::uint64_t token_{};
public:
    explicit EffectUiExclusion(bool source=false) noexcept {
        const auto module=GetModuleHandleW(L"StarfieldHost.aex");if(!module)return;
        const auto begin=std::bit_cast<Begin>(GetProcAddress(module,source?"SFLD_BeginSourceUiExclusionV1":"SFLD_BeginUiExclusionV1"));
        end_=std::bit_cast<End>(GetProcAddress(module,"SFLD_EndUiExclusionV1"));
        if(begin && end_)token_=begin();
    }
    ~EffectUiExclusion(){if(token_)end_(token_);}
    EffectUiExclusion(const EffectUiExclusion&) = delete;
    EffectUiExclusion& operator=(const EffectUiExclusion&) = delete;
    explicit operator bool() const noexcept{return token_!=0;}
};
}
