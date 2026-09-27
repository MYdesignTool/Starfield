#pragma once

#include <cstdint>
#include <utility>
#include <variant>

namespace starfield::core {

// Stable error taxonomy of the host-independent core. The AE adapter maps these
// codes onto host errors; core code never includes or returns PF_Err.
enum class ErrorCode : std::uint8_t {
    invalid_request,     // structurally invalid or oversized request geometry
    invalid_time,        // rational time could not be normalized, or overflowed
    unsupported_format,  // pixel format/bit depth this backend cannot produce
    allocation_failed,   // a bounded allocation failed
    work_limit_exceeded, // request would exceed the documented bounded-work budget
    cancelled,           // the host reported a user interrupt
    internal_failure,    // invariant violation inside the core
};

[[nodiscard]] constexpr const char* describe(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::invalid_request:
            return "invalid render request";
        case ErrorCode::invalid_time:
            return "invalid or overflowing rational time";
        case ErrorCode::unsupported_format:
            return "unsupported pixel format";
        case ErrorCode::allocation_failed:
            return "bounded allocation failed";
        case ErrorCode::work_limit_exceeded:
            return "bounded work budget exceeded";
        case ErrorCode::cancelled:
            return "render cancelled by the host";
        case ErrorCode::internal_failure:
            break;
    }
    return "internal failure";
}

struct CoreError {
    ErrorCode code{ErrorCode::internal_failure};
    // Static string owned by the core; never allocated, never freed by callers.
    const char* detail{"unspecified"};
};

[[nodiscard]] constexpr CoreError make_error(ErrorCode code, const char* detail) noexcept {
    return CoreError{code, detail != nullptr ? detail : describe(code)};
}

template <typename T>
class Result {
public:
    static Result success(T value) { return Result(Storage{std::in_place_type<T>, std::move(value)}); }
    static Result failure(CoreError error) { return Result(Storage{std::in_place_type<CoreError>, error}); }
    static Result failure(ErrorCode code, const char* detail) { return failure(make_error(code, detail)); }

    [[nodiscard]] bool has_value() const noexcept { return std::holds_alternative<T>(storage_); }
    [[nodiscard]] const T& value() const { return std::get<T>(storage_); }
    [[nodiscard]] T&& take_value() { return std::move(std::get<T>(storage_)); }
    [[nodiscard]] const CoreError& error() const { return std::get<CoreError>(storage_); }

private:
    using Storage = std::variant<T, CoreError>;
    explicit Result(Storage storage) : storage_(std::move(storage)) {}

    Storage storage_;
};

} // namespace starfield::core
