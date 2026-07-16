#include "tray_app.h"

#include "platform/platform_util.h"

#include <exception>
#include <string>

namespace {

int runKeyrecordAppSafely() {
    try {
        return runKeyrecordApp();
    } catch (const std::exception& ex) {
        keyrecord::showError(std::string("Unexpected startup failure: ") + ex.what());
        return 1;
    }
}

} // namespace

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE,
    _In_opt_ HINSTANCE,
    _In_ LPSTR,
    _In_ int) {
    return runKeyrecordAppSafely();
}
#else
int main() {
    return runKeyrecordAppSafely();
}
#endif
