#include "platform/capture_backend.h"

#include "platform/linux_keymap.h"
#include "platform/platform_util.h"

#include <linux/input.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

namespace {

constexpr int POLL_TIMEOUT_MS = 250;
constexpr std::size_t BITS_PER_LONG = sizeof(unsigned long) * 8;

std::atomic_flag stopRequested = ATOMIC_FLAG_INIT;

struct InputDevice {
    int fd;
    std::string path;
};

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

template <std::size_t N>
bool isBitSet(int bit, const std::array<unsigned long, N>& bits) {
    auto index = static_cast<std::size_t>(bit) / BITS_PER_LONG;
    auto offset = static_cast<std::size_t>(bit) % BITS_PER_LONG;
    return index < bits.size() && (bits[index] & (1UL << offset)) != 0;
}

bool isKeyboardDevice(int fd) {
    std::array<unsigned long, (EV_MAX + BITS_PER_LONG) / BITS_PER_LONG> eventBits = {};
    if (ioctl(fd, EVIOCGBIT(0, sizeof(eventBits)), eventBits.data()) < 0 ||
        !isBitSet(EV_KEY, eventBits)) {
        return false;
    }

    std::array<unsigned long, (KEY_MAX + BITS_PER_LONG) / BITS_PER_LONG> keyBits = {};
    if (ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(keyBits)), keyBits.data()) < 0) {
        return false;
    }

    // 排除只上报鼠标按钮或多媒体键的设备，避免将非键盘 event 节点加入轮询。
    return isBitSet(KEY_A, keyBits) &&
           isBitSet(KEY_ENTER, keyBits) &&
           isBitSet(KEY_SPACE, keyBits);
}

std::vector<InputDevice> openKeyboardDevices() {
    std::vector<std::filesystem::path> eventPaths;
    std::error_code error;
    std::filesystem::directory_iterator iterator("/dev/input", error);
    std::filesystem::directory_iterator end;
    while (!error && iterator != end) {
        const auto& entry = *iterator;
        std::string filename = entry.path().filename().string();
        if (filename.starts_with("event")) {
            eventPaths.push_back(entry.path());
        }
        iterator.increment(error);
    }
    if (error) {
        keyrecord::debugLog(
            "KeyRecord: failed to enumerate /dev/input: " + error.message() + "\n");
        return {};
    }

    std::sort(eventPaths.begin(), eventPaths.end());
    std::vector<InputDevice> devices;
    for (const auto& path : eventPaths) {
        int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK | O_CLOEXEC);
        if (fd < 0) {
            continue;
        }
        if (!isKeyboardDevice(fd)) {
            close(fd);
            continue;
        }
        devices.push_back({fd, path.string()});
    }
    return devices;
}

void closeDevices(std::vector<InputDevice>& devices) {
    for (auto& device : devices) {
        if (device.fd >= 0) {
            close(device.fd);
            device.fd = -1;
        }
    }
}

std::chrono::system_clock::time_point eventTimePoint(const input_event& event) {
    return std::chrono::system_clock::time_point(
        std::chrono::seconds(event.time.tv_sec) +
        std::chrono::microseconds(event.time.tv_usec));
}

bool readAvailableEvents(InputDevice& device, const keyrecord::KeyEventCallback& callback) {
    std::array<input_event, 64> events = {};
    while (true) {
        ssize_t bytesRead = read(device.fd, events.data(), sizeof(events));
        if (bytesRead < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return true;
            }
            keyrecord::debugLog(
                "KeyRecord: failed to read " + device.path + ": " +
                std::strerror(errno) + "\n");
            return false;
        }
        if (bytesRead == 0) {
            return false;
        }
        if (bytesRead % static_cast<ssize_t>(sizeof(input_event)) != 0) {
            keyrecord::debugLog(
                "KeyRecord: incomplete input event received from " + device.path + "\n");
            return false;
        }

        auto eventCount = static_cast<std::size_t>(bytesRead) / sizeof(input_event);
        for (std::size_t index = 0; index < eventCount; ++index) {
            const auto& event = events[index];
            // evdev: 1 表示按下，2 表示自动重复；都对应 Windows 的 WM_KEYDOWN。
            if (event.type != EV_KEY || (event.value != 1 && event.value != 2)) {
                continue;
            }
            keyrecord::KeyCode vkCode = keyrecord::linuxEvdevToVk(event.code);
            if (vkCode != 0) {
                callback(vkCode, eventTimePoint(event));
            }
        }
    }
}

} // namespace

namespace keyrecord {

int runCaptureLoop(const KeyEventCallback& callback) {
    stopRequested.clear(std::memory_order_relaxed);
    auto devices = openKeyboardDevices();
    if (devices.empty()) {
        debugLog(
            "KeyRecord: no readable keyboard devices found under /dev/input; "
            "check input-group or udev permissions\n");
        return 1;
    }

    SignalHandlerGuard signalHandlers;
    if (!signalHandlers.install()) {
        debugLog(
            "KeyRecord: failed to install signal handlers: " +
            std::string(std::strerror(errno)) + "\n");
        closeDevices(devices);
        return 1;
    }

    int result = 0;
    while (!stopRequested.test(std::memory_order_relaxed)) {
        std::vector<pollfd> pollDescriptors;
        pollDescriptors.reserve(devices.size());
        for (const auto& device : devices) {
            pollDescriptors.push_back({device.fd, POLLIN, 0});
        }

        int pollResult = poll(pollDescriptors.data(), pollDescriptors.size(), POLL_TIMEOUT_MS);
        if (pollResult < 0) {
            if (errno == EINTR) {
                continue;
            }
            debugLog(
                "KeyRecord: input polling failed: " +
                std::string(std::strerror(errno)) + "\n");
            result = 1;
            break;
        }
        if (pollResult == 0) {
            continue;
        }

        for (std::size_t index = 0; index < pollDescriptors.size(); ++index) {
            short events = pollDescriptors[index].revents;
            if ((events & POLLIN) != 0 &&
                !readAvailableEvents(devices[index], callback)) {
                close(devices[index].fd);
                devices[index].fd = -1;
            }
            if ((events & (POLLERR | POLLHUP | POLLNVAL)) != 0 &&
                devices[index].fd >= 0) {
                close(devices[index].fd);
                devices[index].fd = -1;
            }
        }

        std::erase_if(devices, [](const InputDevice& device) {
            return device.fd < 0;
        });
        if (devices.empty()) {
            debugLog("KeyRecord: all keyboard input devices were disconnected\n");
            result = 1;
            break;
        }
    }

    closeDevices(devices);
    return result;
}

void requestStopCapture() {
    stopRequested.test_and_set(std::memory_order_relaxed);
}

} // namespace keyrecord
