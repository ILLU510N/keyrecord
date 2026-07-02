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

void localTime(std::tm& out, std::time_t value) {
#ifdef _WIN32
    localtime_s(&out, &value);
#else
    localtime_r(&value, &out);
#endif
}

void debugLog(const std::string& message) {
#ifdef _WIN32
    OutputDebugStringA(message.c_str());
#else
    std::fputs(message.c_str(), stderr);
#endif
}

} // namespace keyrecord
