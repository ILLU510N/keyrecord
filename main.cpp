#include <windows.h>
#include <shellapi.h>
#include <sqlite3.h>
#include <string>
#include <chrono>
#include <map>

sqlite3* db = nullptr;
HHOOK hook = nullptr;
HWND hwnd = nullptr;
NOTIFYICONDATA nid = {};

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

std::map<DWORD, std::string> vkCodeToName;

void initKeyMap() {
    vkCodeToName[VK_BACK] = "Backspace";
    vkCodeToName[VK_TAB] = "Tab";
    vkCodeToName[VK_RETURN] = "Enter";
    vkCodeToName[VK_SHIFT] = "Shift";
    vkCodeToName[VK_CONTROL] = "Ctrl";
    vkCodeToName[VK_MENU] = "Alt";
    vkCodeToName[VK_PAUSE] = "Pause";
    vkCodeToName[VK_CAPITAL] = "CapsLock";
    vkCodeToName[VK_ESCAPE] = "Esc";
    vkCodeToName[VK_SPACE] = "Space";
    vkCodeToName[VK_PRIOR] = "PageUp";
    vkCodeToName[VK_NEXT] = "PageDown";
    vkCodeToName[VK_END] = "End";
    vkCodeToName[VK_HOME] = "Home";
    vkCodeToName[VK_LEFT] = "Left";
    vkCodeToName[VK_UP] = "Up";
    vkCodeToName[VK_RIGHT] = "Right";
    vkCodeToName[VK_DOWN] = "Down";
    vkCodeToName[VK_SNAPSHOT] = "PrintScreen";
    vkCodeToName[VK_INSERT] = "Insert";
    vkCodeToName[VK_DELETE] = "Delete";
    vkCodeToName[VK_LWIN] = "LeftWin";
    vkCodeToName[VK_RWIN] = "RightWin";
    vkCodeToName[VK_APPS] = "Menu";
    vkCodeToName[VK_NUMPAD0] = "Numpad0";
    vkCodeToName[VK_NUMPAD1] = "Numpad1";
    vkCodeToName[VK_NUMPAD2] = "Numpad2";
    vkCodeToName[VK_NUMPAD3] = "Numpad3";
    vkCodeToName[VK_NUMPAD4] = "Numpad4";
    vkCodeToName[VK_NUMPAD5] = "Numpad5";
    vkCodeToName[VK_NUMPAD6] = "Numpad6";
    vkCodeToName[VK_NUMPAD7] = "Numpad7";
    vkCodeToName[VK_NUMPAD8] = "Numpad8";
    vkCodeToName[VK_NUMPAD9] = "Numpad9";
    vkCodeToName[VK_MULTIPLY] = "Numpad*";
    vkCodeToName[VK_ADD] = "Numpad+";
    vkCodeToName[VK_SUBTRACT] = "Numpad-";
    vkCodeToName[VK_DECIMAL] = "Numpad.";
    vkCodeToName[VK_DIVIDE] = "Numpad/";
    vkCodeToName[VK_F1] = "F1";
    vkCodeToName[VK_F2] = "F2";
    vkCodeToName[VK_F3] = "F3";
    vkCodeToName[VK_F4] = "F4";
    vkCodeToName[VK_F5] = "F5";
    vkCodeToName[VK_F6] = "F6";
    vkCodeToName[VK_F7] = "F7";
    vkCodeToName[VK_F8] = "F8";
    vkCodeToName[VK_F9] = "F9";
    vkCodeToName[VK_F10] = "F10";
    vkCodeToName[VK_F11] = "F11";
    vkCodeToName[VK_F12] = "F12";
    vkCodeToName[VK_NUMLOCK] = "NumLock";
    vkCodeToName[VK_SCROLL] = "ScrollLock";
    vkCodeToName[VK_LSHIFT] = "LeftShift";
    vkCodeToName[VK_RSHIFT] = "RightShift";
    vkCodeToName[VK_LCONTROL] = "LeftCtrl";
    vkCodeToName[VK_RCONTROL] = "RightCtrl";
    vkCodeToName[VK_LMENU] = "LeftAlt";
    vkCodeToName[VK_RMENU] = "RightAlt";
    vkCodeToName[VK_OEM_1] = ";";
    vkCodeToName[VK_OEM_PLUS] = "=";
    vkCodeToName[VK_OEM_COMMA] = ",";
    vkCodeToName[VK_OEM_MINUS] = "-";
    vkCodeToName[VK_OEM_PERIOD] = ".";
    vkCodeToName[VK_OEM_2] = "/";
    vkCodeToName[VK_OEM_3] = "`";
    vkCodeToName[VK_OEM_4] = "[";
    vkCodeToName[VK_OEM_5] = "\\";
    vkCodeToName[VK_OEM_6] = "]";
    vkCodeToName[VK_OEM_7] = "'";
}

std::string getKeyName(DWORD vkCode) {
    if (vkCodeToName.find(vkCode) != vkCodeToName.end()) {
        return vkCodeToName[vkCode];
    }

    if ((vkCode >= 0x30 && vkCode <= 0x39) || (vkCode >= 0x41 && vkCode <= 0x5A)) {
        return std::string(1, (char)vkCode);
    }

    return "Unknown_" + std::to_string(vkCode);
}

void initDB() {
    sqlite3_open("keyrecord.db", &db);
    const char* sql = "CREATE TABLE IF NOT EXISTS keys("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "timestamp INTEGER,"
                      "date TEXT,"
                      "hour INTEGER,"
                      "vk_code INTEGER,"
                      "key_name TEXT)";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

void recordKey(DWORD vkCode, const std::string& keyName) {
    auto now = std::chrono::system_clock::now();
    auto epoch = now.time_since_epoch();
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(epoch).count();
    auto t = std::chrono::system_clock::to_time_t(now);
    tm local;
    localtime_s(&local, &t);
    char date[32];
    sprintf_s(date, "%04d-%02d-%02d", local.tm_year + 1900, local.tm_mon + 1, local.tm_mday);

    char sql[512];
    sprintf_s(sql, "INSERT INTO keys(timestamp,date,hour,vk_code,key_name) VALUES(%lld,'%s',%d,%lu,'%s')",
              ts, date, local.tm_hour, vkCode, keyName.c_str());
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

LRESULT CALLBACK KeyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*)lParam;
        DWORD vkCode = kb->vkCode;
        std::string keyName = getKeyName(vkCode);
        recordKey(vkCode, keyName);
    }
    return CallNextHookEx(hook, code, wParam, lParam);
}

void createTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
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
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
    removeTrayIcon();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuW(hMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
                SetForegroundWindow(hwnd);
                TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, nullptr);
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
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 创建隐藏窗口用于消息循环
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
    initDB();

    hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, nullptr, 0);
    if (!hook) {
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
