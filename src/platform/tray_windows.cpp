#include "platform/tray.h"

#include "resource.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <utility>

namespace {

constexpr wchar_t WINDOW_CLASS_NAME[] = L"KeyRecordClass";
constexpr UINT WM_TRAYICON = WM_USER + 1;
constexpr UINT ID_TRAY_EXIT = 1001;

HWND trayWindow = nullptr;
NOTIFYICONDATAW trayIcon = {};
keyrecord::TrayExitCallback exitCallback;

HICON loadTrayIcon(HINSTANCE instance) {
    // 托盘使用小尺寸图标，优先从多尺寸 .ico 资源中取系统推荐尺寸。
    auto icon = static_cast<HICON>(LoadImageW(
        instance,
        MAKEINTRESOURCEW(IDI_KEYRECORD),
        IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON),
        GetSystemMetrics(SM_CYSMICON),
        LR_DEFAULTCOLOR | LR_SHARED));
    return icon ? icon : LoadIconW(nullptr, IDI_APPLICATION);
}

LRESULT CALLBACK trayWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT point = {};
                GetCursorPos(&point);
                HMENU menu = CreatePopupMenu();
                if (menu) {
                    AppendMenuW(menu, MF_STRING, ID_TRAY_EXIT, L"Exit");
                    SetForegroundWindow(window);
                    TrackPopupMenu(
                        menu,
                        TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                        point.x,
                        point.y,
                        0,
                        window,
                        nullptr);
                    DestroyMenu(menu);
                }
            }
            return 0;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT && exitCallback) {
                exitCallback();
            }
            return 0;
        default:
            return DefWindowProcW(window, message, wParam, lParam);
    }
}

} // namespace

namespace keyrecord {

bool initializeTray(TrayExitCallback callback) {
    HINSTANCE instance = GetModuleHandleW(nullptr);
    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = trayWindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WINDOW_CLASS_NAME;
    if (!RegisterClassW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    trayWindow = CreateWindowExW(
        0,
        WINDOW_CLASS_NAME,
        L"KeyRecord",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        instance,
        nullptr);
    if (!trayWindow) {
        return false;
    }

    exitCallback = std::move(callback);
    trayIcon.cbSize = sizeof(trayIcon);
    trayIcon.hWnd = trayWindow;
    trayIcon.uID = 1;
    trayIcon.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    trayIcon.uCallbackMessage = WM_TRAYICON;
    trayIcon.hIcon = loadTrayIcon(instance);
    wcscpy_s(trayIcon.szTip, L"KeyRecord Running");
    if (!Shell_NotifyIconW(NIM_ADD, &trayIcon)) {
        DestroyWindow(trayWindow);
        trayWindow = nullptr;
        exitCallback = {};
        return false;
    }
    return true;
}

void shutdownTray() {
    if (trayWindow) {
        Shell_NotifyIconW(NIM_DELETE, &trayIcon);
        DestroyWindow(trayWindow);
        trayWindow = nullptr;
    }
    trayIcon = {};
    exitCallback = {};
}

} // namespace keyrecord
