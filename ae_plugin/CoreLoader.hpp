#pragma once

#include "starfield/core/PluginApi.h"

#include <cstdint>
#include <memory>
#include <string>

namespace starfield::adapter {

// A generation is immutable after loading. A shared lease pins its DLL through
// pre-render, Smart Render and result release, including across a manual reload.
class CoreGeneration final {
public:
    CoreGeneration(const CoreGeneration&) = delete;
    CoreGeneration& operator=(const CoreGeneration&) = delete;
    // Constructed only after LoadLibraryExW and ABI validation in CoreLoader.cpp.
    CoreGeneration(void* module, SfCoreApi api, std::uint64_t identity, std::wstring path) noexcept;
    ~CoreGeneration() noexcept;

    [[nodiscard]] const SfCoreApi& api() const noexcept { return api_; }
    [[nodiscard]] std::uint64_t cache_identity() const noexcept { return cache_identity_; }
    [[nodiscard]] const std::wstring& path() const noexcept { return path_; }

private:
    void* module_{nullptr};
    SfCoreApi api_{};
    std::uint64_t cache_identity_{0};
    std::wstring path_;
};

struct CoreLoadResult {
    std::shared_ptr<const CoreGeneration> generation;
    std::string error;
    bool changed{false};
    [[nodiscard]] explicit operator bool() const noexcept { return generation != nullptr; }
};

// acquire_core loads on first use. reload_core reads current.txt again and swaps
// only after the new module has passed every ABI check. On failure, the prior
// generation stays active and the result carries its lease plus an error.
[[nodiscard]] CoreLoadResult acquire_core() noexcept;
[[nodiscard]] CoreLoadResult reload_core() noexcept;

} // namespace starfield::adapter
