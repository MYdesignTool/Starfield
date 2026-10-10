#pragma once
#include <cstdint>

// Private session-local C seam. The resident Host owns the state; effect DLLs
// only borrow these functions while a UI callback is active. No host handles.
extern "C" __declspec(dllexport) std::uint64_t SFLD_BeginUiExclusionV1() noexcept;
extern "C" __declspec(dllexport) void SFLD_EndUiExclusionV1(std::uint64_t) noexcept;

namespace starfield::adapter {
void initialize_ui_exclusion() noexcept;
void stop_ui_exclusion() noexcept;
[[nodiscard]] bool ui_exclusion_available() noexcept;
class HostUiExclusion final {
    std::uint64_t token_{SFLD_BeginUiExclusionV1()};
public:
    ~HostUiExclusion(){if(token_)SFLD_EndUiExclusionV1(token_);}
    HostUiExclusion() noexcept = default;
    HostUiExclusion(const HostUiExclusion&) = delete;
    HostUiExclusion& operator=(const HostUiExclusion&) = delete;
    explicit operator bool() const noexcept{return token_!=0;}
};
}
