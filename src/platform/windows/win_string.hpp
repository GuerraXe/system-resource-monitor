#pragma once

// Small shared utility: several Win32 APIs (GetVolumeInformationW, the IP
// Helper API's interface descriptions, ...) return wide strings. This is
// the one place that dependency lives, kept out of every monitor header so
// none of them need <windows.h> or <string> wide-conversion boilerplate.

#include <string>

namespace srm::platform::windows {

// Converts a null-terminated wide string to UTF-8. Returns an empty string
// for a null pointer or a failed conversion -- callers treat "unknown" and
// "couldn't convert" the same way, since this is an internal utility rather
// than an OS query with its own error-reporting contract.
std::string narrow_or_empty(const wchar_t* wide);

} // namespace srm::platform::windows
