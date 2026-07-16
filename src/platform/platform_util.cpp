#include "platform/platform_util.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cstdio>
#endif

namespace keyrecord {

bool localTime(std::tm& out, std::time_t value) {
#ifdef _WIN32
    return localtime_s(&out, &value) == 0;
#else
    return localtime_r(&value, &out) != nullptr;
#endif
}

void debugLog(const std::string& message) {
#ifdef _WIN32
    OutputDebugStringA(message.c_str());
#else
    std::fputs(message.c_str(), stderr);
#endif
}

void showError(const std::string& message) {
#ifdef _WIN32
    MessageBoxA(nullptr, message.c_str(), "KeyRecord", MB_OK | MB_ICONERROR);
#else
    std::fputs(("KeyRecord: " + message + "\n").c_str(), stderr);
#endif
}

} // namespace keyrecord
