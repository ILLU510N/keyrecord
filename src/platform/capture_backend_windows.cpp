#include "platform/capture_backend.h"

#include "platform/platform_util.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <atomic>

namespace {

HHOOK keyboardHook = nullptr;
std::atomic<DWORD> captureThreadId = 0;
const keyrecord::KeyEventCallback* eventCallback = nullptr;

LRESULT CALLBACK keyboardProc(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) && eventCallback) {
        const auto* event = reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);
        (*eventCallback)(
            static_cast<keyrecord::KeyCode>(event->vkCode),
            std::chrono::system_clock::now());
    }
    return CallNextHookEx(keyboardHook, code, wParam, lParam);
}

} // namespace

namespace keyrecord {

int runCaptureLoop(const KeyEventCallback& callback) {
    captureThreadId.store(GetCurrentThreadId(), std::memory_order_release);
    eventCallback = &callback;
    keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, keyboardProc, nullptr, 0);
    if (!keyboardHook) {
        debugLog("KeyRecord: failed to set keyboard hook\n");
        showError("Failed to set keyboard hook");
        eventCallback = nullptr;
        captureThreadId.store(0, std::memory_order_release);
        return 1;
    }

    int result = 0;
    MSG message = {};
    while (true) {
        int getMessageResult = GetMessageW(&message, nullptr, 0, 0);
        if (getMessageResult == 0) {
            break;
        }
        if (getMessageResult == -1) {
            debugLog("KeyRecord: Windows message loop failed\n");
            result = 1;
            break;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    UnhookWindowsHookEx(keyboardHook);
    keyboardHook = nullptr;
    eventCallback = nullptr;
    captureThreadId.store(0, std::memory_order_release);
    return result;
}

void requestStopCapture() {
    const DWORD threadId = captureThreadId.load(std::memory_order_acquire);
    if (threadId != 0) {
        PostThreadMessageW(threadId, WM_QUIT, 0, 0);
    }
}

} // namespace keyrecord
