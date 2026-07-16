#include "platform/capture_backend.h"

#include "platform/macos_keymap.h"
#include "platform/platform_util.h"

#include <ApplicationServices/ApplicationServices.h>
#import <AppKit/AppKit.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <string>

namespace {

constexpr CFTimeInterval STOP_TIMER_INTERVAL = 0.25;

std::atomic_flag stopRequested = ATOMIC_FLAG_INIT;
std::atomic<CFRunLoopRef> captureRunLoop = nullptr;
CFMachPortRef eventTap = nullptr;
const keyrecord::KeyEventCallback* eventCallback = nullptr;

void handleStopSignal(int) {
    stopRequested.test_and_set(std::memory_order_relaxed);
}

class SignalHandlerGuard {
public:
    bool install() {
        struct sigaction action = {};
        action.sa_handler = handleStopSignal;
        sigemptyset(&action.sa_mask);

        if (sigaction(SIGINT, &action, &oldInterrupt_) != 0) {
            return false;
        }
        interruptInstalled_ = true;
        if (sigaction(SIGTERM, &action, &oldTerminate_) != 0) {
            return false;
        }
        terminateInstalled_ = true;
        return true;
    }

    ~SignalHandlerGuard() {
        if (terminateInstalled_) {
            sigaction(SIGTERM, &oldTerminate_, nullptr);
        }
        if (interruptInstalled_) {
            sigaction(SIGINT, &oldInterrupt_, nullptr);
        }
    }

private:
    struct sigaction oldInterrupt_ = {};
    struct sigaction oldTerminate_ = {};
    bool interruptInstalled_ = false;
    bool terminateInstalled_ = false;
};

CGEventRef captureCallback(
    CGEventTapProxy,
    CGEventType type,
    CGEventRef event,
    void*) {
    if (type == kCGEventTapDisabledByTimeout ||
        type == kCGEventTapDisabledByUserInput) {
        if (eventTap) {
            CGEventTapEnable(eventTap, true);
        }
        return event;
    }

    if ((type == kCGEventKeyDown || type == kCGEventFlagsChanged) && eventCallback) {
        auto nativeCode = static_cast<int>(
            CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
        keyrecord::KeyCode vkCode = keyrecord::macVirtualKeyToVk(nativeCode);
        const bool isKeyDown = type == kCGEventKeyDown ||
            CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, nativeCode);
        if (vkCode != 0 && isKeyDown) {
            (*eventCallback)(vkCode, std::chrono::system_clock::now());
        }
    }
    return event;
}

void stopTimerCallback(CFRunLoopTimerRef, void*) {
    if (stopRequested.test(std::memory_order_relaxed)) {
        [NSApp stop:nil];
    }
}

} // namespace

namespace keyrecord {

int runCaptureLoop(const KeyEventCallback& callback) {
    stopRequested.clear(std::memory_order_relaxed);
    if (!CGPreflightListenEventAccess() && !CGRequestListenEventAccess()) {
        debugLog(
            "KeyRecord: Input Monitoring permission is required; enable KeyRecord "
            "in System Settings > Privacy & Security > Input Monitoring\n");
        return 1;
    }

    SignalHandlerGuard signalHandlers;
    if (!signalHandlers.install()) {
        debugLog("KeyRecord: failed to install signal handlers\n");
        return 1;
    }

    eventCallback = &callback;
    CGEventMask eventMask =
        CGEventMaskBit(kCGEventKeyDown) | CGEventMaskBit(kCGEventFlagsChanged);
    eventTap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionListenOnly,
        eventMask,
        captureCallback,
        nullptr);
    if (!eventTap) {
        debugLog(
            "KeyRecord: failed to create event tap; verify Input Monitoring permission\n");
        eventCallback = nullptr;
        return 1;
    }

    CFRunLoopSourceRef eventSource =
        CFMachPortCreateRunLoopSource(kCFAllocatorDefault, eventTap, 0);
    if (!eventSource) {
        debugLog("KeyRecord: failed to create event-tap run-loop source\n");
        CFRelease(eventTap);
        eventTap = nullptr;
        eventCallback = nullptr;
        return 1;
    }

    CFRunLoopTimerContext timerContext = {};
    CFRunLoopTimerRef stopTimer = CFRunLoopTimerCreate(
        kCFAllocatorDefault,
        CFAbsoluteTimeGetCurrent() + STOP_TIMER_INTERVAL,
        STOP_TIMER_INTERVAL,
        0,
        0,
        stopTimerCallback,
        &timerContext);
    if (!stopTimer) {
        debugLog("KeyRecord: failed to create stop timer\n");
        CFRelease(eventSource);
        CFRelease(eventTap);
        eventTap = nullptr;
        eventCallback = nullptr;
        return 1;
    }

    CFRunLoopRef runLoop = CFRunLoopGetCurrent();
    captureRunLoop.store(runLoop, std::memory_order_release);
    CFRunLoopAddSource(runLoop, eventSource, kCFRunLoopCommonModes);
    CFRunLoopAddTimer(runLoop, stopTimer, kCFRunLoopCommonModes);
    CGEventTapEnable(eventTap, true);
    // NSApplication 负责派发状态栏菜单事件，底层仍使用加入事件 Tap 的主 CFRunLoop。
    [NSApp run];

    captureRunLoop.store(nullptr, std::memory_order_release);
    CGEventTapEnable(eventTap, false);
    CFRunLoopRemoveTimer(runLoop, stopTimer, kCFRunLoopCommonModes);
    CFRunLoopRemoveSource(runLoop, eventSource, kCFRunLoopCommonModes);
    CFRelease(stopTimer);
    CFRelease(eventSource);
    CFRelease(eventTap);
    eventTap = nullptr;
    eventCallback = nullptr;
    return 0;
}

void requestStopCapture() {
    stopRequested.test_and_set(std::memory_order_relaxed);
    CFRunLoopRef runLoop = captureRunLoop.load(std::memory_order_acquire);
    if (runLoop) {
        [NSApp performSelectorOnMainThread:@selector(stop:)
                               withObject:nil
                            waitUntilDone:NO];
        CFRunLoopWakeUp(runLoop);
    }
}

} // namespace keyrecord
