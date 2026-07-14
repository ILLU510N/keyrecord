#include "tray_app.h"

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
    return runKeyrecordApp();
}
#else
int main() {
    return runKeyrecordApp();
}
#endif
