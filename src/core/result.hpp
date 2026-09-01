#pragma once

// A minimal Result<T> type for fallible metric collection.
//
// Why not exceptions for this? Every monitor (CPU, memory, disk, ...) can
// fail independently at runtime for reasons that are entirely expected --
// a removable drive was unplugged, a process exited between enumeration and
// query, a counter isn't exposed on this machine -- and the presentation
// layer needs to keep rendering the *other* rows when one fails. Modeling
// that as a return value keeps the "this metric is unavailable" path in the
// type system instead of relying on callers to remember a try/catch around
// every single sample() call.
//
// Why not std::expected<T, E>? That's a C++23 library feature; this project
// targets C++20, and the interface boundary (platform/interfaces.hpp) is
// meant to compile with any reasonably current C++20 compiler, not just
// ones that have shipped std::expected.

#include <stdexcept>
#include <string>
#include <utility>
#include <variant>

namespace srm::core {

enum class ErrorCode {
    Unavailable,        // metric not supported/exposed on this platform
    PermissionDenied,   // insufficient privilege to query
    NotFound,           // the queried process/volume/interface no longer exists
    PlatformApiFailure, // the underlying OS call failed unexpectedly
    InvalidArgument,    // caller passed a value the API can't act on
};

inline const char* to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Unavailable: return "Unavailable";
        case ErrorCode::PermissionDenied: return "PermissionDenied";
        case ErrorCode::NotFound: return "NotFound";
        case ErrorCode::PlatformApiFailure: return "PlatformApiFailure";
        case ErrorCode::InvalidArgument: return "InvalidArgument";
    }
    return "Unknown";
}

struct Error {
    ErrorCode code;
    std::string message;
};

template <typename T>
class Result {
public:
    static Result Ok(T value) {
        return Result(std::in_place_index<0>, std::move(value));
    }
    static Result Fail(Error error) {
        return Result(std::in_place_index<1>, std::move(error));
    }

    [[nodiscard]] bool has_value() const noexcept { return storage_.index() == 0; }
    [[nodiscard]] explicit operator bool() const noexcept { return has_value(); }

    // Throws std::logic_error if this is an error result -- a programmer
    // mistake (not checking has_value() first), not a modeled failure mode.
    [[nodiscard]] const T& value() const& {
        require_value();
        return std::get<0>(storage_);
    }
    [[nodiscard]] T& value() & {
        require_value();
        return std::get<0>(storage_);
    }
    [[nodiscard]] T&& value() && {
        require_value();
        return std::move(std::get<0>(storage_));
    }

    [[nodiscard]] T value_or(T fallback) const& {
        return has_value() ? std::get<0>(storage_) : std::move(fallback);
    }

    [[nodiscard]] const Error& error() const {
        if (has_value()) {
            throw std::logic_error("Result::error() called on a success result");
        }
        return std::get<1>(storage_);
    }

private:
    Result(std::in_place_index_t<0>, T value) : storage_(std::in_place_index<0>, std::move(value)) {}
    Result(std::in_place_index_t<1>, Error error) : storage_(std::in_place_index<1>, std::move(error)) {}

    void require_value() const {
        if (!has_value()) {
            throw std::logic_error("Result::value() called on an error result: " +
                                    std::string(to_string(std::get<1>(storage_).code)) + ": " +
                                    std::get<1>(storage_).message);
        }
    }

    std::variant<T, Error> storage_;
};

} // namespace srm::core
