#include "win_string.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace srm::platform::windows {

std::string narrow_or_empty(const wchar_t* wide) {
    if (wide == nullptr) {
        return {};
    }
    const int required = WideCharToMultiByte(CP_UTF8, 0, wide, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0) {
        return {};
    }
    // `required` includes the null terminator; size the buffer without it.
    std::string result(static_cast<std::size_t>(required - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide, -1, result.data(), required, nullptr, nullptr);
    return result;
}

} // namespace srm::platform::windows
