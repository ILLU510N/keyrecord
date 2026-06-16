#include "tray_app.h"

#include "key_event_writer.h"
#include "key_names.h"

#include <shellapi.h>

#include <chrono>

namespace {

HHOOK hook = nullptr;
HWND hwnd = nullptr;
NOTIFYICONDATA nid = {};
constexpr UINT WM_TRAYICON = WM_USER + 1;
constexpr UINT ID_TRAY_EXIT = 1001;

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        enqueueKeyEvent(kb->vkCode, std::chrono::system_clock::now());
    }
    return CallNextHookEx(hook, code, wParam, lParam);
}

void createTrayIcon(HWND windowHandle) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = windowHandle;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wcscpy_s(nid.szTip, 128, L"KeyRecord Running");
    Shell_NotifyIcon(NIM_ADD, &nid);
}

void removeTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

void cleanup() {
    if (hook) {
        UnhookWindowsHookEx(hook);
        hook = nullptr;
    }
    stopWriter();
    removeTrayIcon();
}

LRESULT CALLBACK WndProc(HWND windowHandle, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
                SetForegroundWindow(windowHandle);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, windowHandle, nullptr);
                DestroyMenu(hMenu);
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                PostQuitMessage(0);
            }
            break;
        case WM_DESTROY:
            cleanup();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(windowHandle, msg, wParam, lParam);
    }
    return 0;
}

} // namespace

int runTrayApp(HINSTANCE hInstance) {
    // 创建隐藏窗口用于接收托盘菜单和退出消息。
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"KeyRecordClass";
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(
        0,
        L"KeyRecordClass",
        L"KeyRecord",
        0,
        0, 0, 0, 0,
        HWND_MESSAGE,
        nullptr,
        hInstance,
        nullptr
    );

    initKeyMap();
    if (!startWriter()) {
        MessageBoxW(nullptr, L"Failed to initialize database writer", L"Error", MB_ICONERROR);
        return 1;
    }

    hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    if (!hook) {
        stopWriter();
        MessageBoxW(nullptr, L"Failed to set keyboard hook", L"Error", MB_ICONERROR);
        return 1;
    }

    createTrayIcon(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    cleanup();
    return 0;
}
