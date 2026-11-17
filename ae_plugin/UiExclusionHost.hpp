#pragma once
#include <cstdint>
#include "SourceEditTelemetry.hpp"

// Private session-local C seam. The resident Host owns the state; effect DLLs
// only borrow these functions while a UI callback is active. No host handles.
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginUiExclusionV1() noexcept;
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginSourceUiExclusionV1() noexcept;
extern "C" __declspec(dllexport) bool SFLD_SourceUiExclusionActiveV1() noexcept;
extern "C" __declspec(dllexport) void SFLD_RecordSourceEditV1(const starfield::adapter::SourceEditTelemetry*) noexcept;
extern "C" __declspec(dllexport) void SFLD_EndUiExclusionV1(std::uint64_t) noexcept;
extern "C" __declspec(dllexport) bool SFLD_UiExclusionActiveV1() noexcept;
// A distinct, read-only render-thread fence for physical graph transactions.
// Odd means mutation in progress; an even value changes when it settles.
extern "C" __declspec(dllexport) std::uint64_t SFLD_GraphMutationEpochV1() noexcept;
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginGraphMutationV1() noexcept;
extern "C" __declspec(dllexport) void SFLD_EndGraphMutationV1(std::uint64_t) noexcept;

namespace starfield::adapter {
void initialize_ui_exclusion() noexcept;
void stop_ui_exclusion() noexcept;
[[nodiscard]] bool ui_exclusion_available() noexcept;
bool take_source_edit(SourceEditTelemetry&) noexcept;
class HostUiExclusion final {
    std::uint64_t token_{SFLD_BeginUiExclusionV1()};
public:
    ~HostUiExclusion(){if(token_)SFLD_EndUiExclusionV1(token_);}
    HostUiExclusion() noexcept = default;
    HostUiExclusion(const HostUiExclusion&) = delete;
    HostUiExclusion& operator=(const HostUiExclusion&) = delete;
    explicit operator bool() const noexcept{return token_!=0;}
};
class HostGraphMutation final {
    std::uint64_t token_{SFLD_BeginGraphMutationV1()};
public:
    ~HostGraphMutation(){if(token_)SFLD_EndGraphMutationV1(token_);}
    HostGraphMutation() noexcept = default;
    HostGraphMutation(const HostGraphMutation&) = delete;
    HostGraphMutation& operator=(const HostGraphMutation&) = delete;
    explicit operator bool() const noexcept{return token_!=0;}
};
}
