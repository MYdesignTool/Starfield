#include "CoreLoader.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <filesystem>
#include <fstream>
#include <array>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>

namespace starfield::adapter {
namespace {
std::mutex g_mutex;
std::shared_ptr<const CoreGeneration> g_current;
int g_module_anchor = 0;

std::optional<std::uint64_t> identity_for(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return std::nullopt;
    std::uint64_t hash = 14695981039346656037ull;
    for (const wchar_t character : path.wstring()) {
        hash ^= static_cast<std::uint64_t>(character);
        hash *= 1099511628211ull;
    }
    std::array<char, 65536> buffer{};
    while (stream.read(buffer.data(), buffer.size()) || stream.gcount() > 0) {
        for (std::streamsize i = 0; i < stream.gcount(); ++i) {
            hash ^= static_cast<unsigned char>(buffer[static_cast<std::size_t>(i)]);
            hash *= 1099511628211ull;
        }
    }
    if (!stream.eof()) return std::nullopt;
    return hash;
}

std::filesystem::path adapter_directory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&g_module_anchor), &module))
        return {};
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(module, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0 || length >= buffer.size()) return {};
    buffer.resize(length);
    return std::filesystem::path(buffer).parent_path();
}

bool valid_versioned_name(std::string_view name) noexcept {
    constexpr std::string_view prefix = "StarfieldCore-";
    constexpr std::string_view suffix = ".dll";
    if (!name.starts_with(prefix) || !name.ends_with(suffix) ||
        name.size() <= prefix.size() + suffix.size() || name.size() > 100) return false;
    for (const unsigned char character : name) {
        if (!((character >= 'A' && character <= 'Z') ||
              (character >= 'a' && character <= 'z') ||
              (character >= '0' && character <= '9') ||
              character == '-' || character == '_' || character == '.')) return false;
    }
    return name.find("..") == std::string_view::npos;
}

std::filesystem::path selected_core(std::string& error) {
    const auto directory = adapter_directory();
    if (directory.empty()) {
        error = "cannot locate the installed Starfield adapter";
        return {};
    }
    const auto runtime = directory / L"StarfieldRuntime";
    const auto manifest = runtime / L"current.txt";
    std::error_code fs_error;
    if (!std::filesystem::exists(manifest, fs_error)) {
        if (fs_error) error = "cannot inspect the core runtime manifest";
        return fs_error ? std::filesystem::path{} : directory / L"StarfieldCore.dll";
    }
    if (std::filesystem::file_size(manifest, fs_error) > 128 || fs_error) {
        error = "core runtime manifest is too large or unreadable";
        return {};
    }
    std::ifstream stream(manifest, std::ios::binary);
    if (!stream) {
        error = "cannot read the core runtime manifest";
        return {};
    }
    std::string filename;
    std::getline(stream, filename);
    if (!filename.empty() && filename.back() == '\r') filename.pop_back();
    if (!valid_versioned_name(filename) || stream.bad()) {
        error = "invalid core runtime manifest filename";
        return {};
    }
    return runtime / std::filesystem::path(filename);
}

CoreLoadResult load_selected(bool force_check) {
    CoreLoadResult result;
    std::string selection_error;
    const auto selected = selected_core(selection_error);
    if (selected.empty()) {
        std::lock_guard lock(g_mutex);
        result.generation = g_current;
        result.error = std::move(selection_error);
        return result;
    }
    const auto path = selected.wstring();
    {
        std::lock_guard lock(g_mutex);
        if (g_current && g_current->path() == path) {
            result.generation = g_current;
            return result;
        }
        if (!force_check && g_current) {
            result.generation = g_current;
            return result;
        }
    }
    const auto identity = identity_for(selected);
    if (!identity.has_value()) {
        std::lock_guard lock(g_mutex);
        result.generation = g_current;
        result.error = "cannot hash selected core DLL for the AE cache key";
        return result;
    }
    const HMODULE module = LoadLibraryExW(path.c_str(), nullptr,
                                          LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (module == nullptr) {
        std::lock_guard lock(g_mutex);
        result.generation = g_current;
        result.error = "cannot load selected StarfieldCore.dll";
        return result;
    }
    const auto get_api = reinterpret_cast<SfCoreGetApiFn>(GetProcAddress(module, "StarfieldCore_GetApi"));
    SfCoreApi api{};
    bool api_ok = false;
    try {
        api_ok = get_api != nullptr && get_api(SF_CORE_ABI_VERSION, sizeof(SfCoreApi), &api) == 1;
    } catch (...) {
        api_ok = false;
    }
    if (!api_ok ||
        api.struct_size != sizeof(SfCoreApi) || api.abi_version != SF_CORE_ABI_VERSION ||
        api.render == nullptr || api.release_render_result == nullptr || api.inspect == nullptr) {
        FreeLibrary(module);
        std::lock_guard lock(g_mutex);
        result.generation = g_current;
        result.error = "selected StarfieldCore.dll has an incompatible ABI";
        return result;
    }
    std::shared_ptr<const CoreGeneration> next;
    try {
        next = std::make_shared<CoreGeneration>(module, api, *identity, path);
        std::shared_ptr<const CoreGeneration> retired;
        {
            std::lock_guard lock(g_mutex);
            std::string current_selection_error;
            const auto still_selected = selected_core(current_selection_error);
            if (still_selected != selected) {
                result.generation = g_current;
                result.error = "core selection changed while loading; retry reload";
            } else if (g_current && g_current->path() == path) {
                result.generation = g_current;
            } else {
                retired = std::move(g_current);
                g_current = next;
                result.generation = std::move(next);
                result.changed = true;
            }
        }
        // Both the unused new module and retired generation unload outside the
        // loader mutex, after all outstanding pre-render/render leases release.
        return result;
    } catch (...) {
        if (!next) FreeLibrary(module);
        std::lock_guard lock(g_mutex);
        result.generation = g_current;
        result.error = "cannot allocate core generation lease";
        return result;
    }
}
} // namespace

CoreGeneration::CoreGeneration(void* module, SfCoreApi api, std::uint64_t identity,
                               std::wstring path) noexcept
    : module_(module), api_(api), cache_identity_(identity), path_(std::move(path)) {}

CoreGeneration::~CoreGeneration() noexcept {
    if (module_) FreeLibrary(static_cast<HMODULE>(module_));
}

CoreLoadResult acquire_core() noexcept {
    try {
        std::lock_guard lock(g_mutex);
        if (g_current) return CoreLoadResult{g_current, {}, false};
    } catch (...) { return CoreLoadResult{{}, "cannot acquire core generation lock", false}; }
    try { return load_selected(true); }
    catch (...) { return CoreLoadResult{{}, "cannot locate core runtime", false}; }
}

CoreLoadResult reload_core() noexcept {
    try { return load_selected(true); }
    catch (...) {
        try {
            std::lock_guard lock(g_mutex);
            return CoreLoadResult{g_current, "cannot reload core runtime", false};
        } catch (...) { return CoreLoadResult{{}, "cannot reload core runtime", false}; }
    }
}

} // namespace starfield::adapter
