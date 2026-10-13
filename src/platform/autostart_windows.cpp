#include "platform/autostart_windows.h"

#include "platform/platform_util.h"

#include <vector>

namespace {

constexpr wchar_t RUN_KEY[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr wchar_t VALUE_NAME[] = L"KeyRecord";
constexpr size_t MAX_RUN_COMMAND_LENGTH = 260;

// 注册表句柄在所有返回路径上释放，不关闭调用方传入的预定义根句柄。
struct RegistryKey {
    HKEY value = nullptr;
    ~RegistryKey() {
        if (value) {
            RegCloseKey(value);
        }
    }
};

void clearError(std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }
}

bool fail(std::string* errorMessage, const char* action, LSTATUS error) {
    if (errorMessage) {
        *errorMessage = std::string(action) + " (Windows error " + std::to_string(error) + ")";
    }
    return false;
}

bool getExecutablePath(std::wstring& path, std::string* errorMessage) {
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    if (length == 0) {
        return fail(errorMessage, "Failed to resolve application path", GetLastError());
    }
    if (length >= buffer.size()) {
        return fail(errorMessage, "Application path is too long", ERROR_FILENAME_EXCED_RANGE);
    }
    path.assign(buffer.data(), length);
    return true;
}

bool updateRegistration(bool enabled, std::string* errorMessage) {
    std::wstring path;
    if (enabled && !getExecutablePath(path, errorMessage)) {
        return false;
    }

    RegistryKey key;
    // 开启才允许创建 Run 键；关闭不存在的键是成功的幂等操作。
    const LSTATUS result = enabled
        ? RegCreateKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, nullptr, 0,
                          KEY_SET_VALUE, nullptr, &key.value, nullptr)
        : RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &key.value);
    if (!enabled && result == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    if (result != ERROR_SUCCESS) {
        return fail(errorMessage, "Failed to open autostart registry key", result);
    }
    return keyrecord::autostart_detail::writeRegistration(key.value, path, enabled, errorMessage);
}

} // namespace

namespace keyrecord {
namespace autostart_detail {

bool queryRegistration(HKEY key, const std::wstring& executablePath,
                       bool& enabled, std::string* errorMessage) {
    clearError(errorMessage);
    enabled = false;
    // 本程序只写 REG_SZ。固定容量同时限制外部异常数据的读取量。
    wchar_t buffer[MAX_RUN_COMMAND_LENGTH + 2] = {};
    DWORD bytes = sizeof(buffer);
    DWORD type = 0;
    const LSTATUS result = RegQueryValueExW(key, VALUE_NAME, nullptr, &type,
                                           reinterpret_cast<BYTE*>(buffer), &bytes);
    if (result == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    if (result != ERROR_SUCCESS) {
        return fail(errorMessage, "Failed to read autostart registration", result);
    }
    // 非字符串、缺少终止符或内嵌终止符均不能被当作有效启动命令。
    if (type != REG_SZ || bytes < sizeof(wchar_t) || bytes % sizeof(wchar_t) != 0 ||
        buffer[bytes / sizeof(wchar_t) - 1] != L'\0') {
        return fail(errorMessage, "Invalid autostart registration", ERROR_INVALID_DATA);
    }
    const std::wstring command(buffer, bytes / sizeof(wchar_t) - 1);
    const std::wstring expected = L"\"" + executablePath + L"\"";
    enabled = command == expected;
    return true;
}

bool writeRegistration(HKEY key, const std::wstring& executablePath,
                       bool enabled, std::string* errorMessage) {
    clearError(errorMessage);
    if (!enabled) {
        const LSTATUS result = RegDeleteValueW(key, VALUE_NAME);
        return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND
            ? true : fail(errorMessage, "Failed to remove autostart registration", result);
    }

    const std::wstring command = L"\"" + executablePath + L"\"";
    // Run 命令最多 260 个字符（包含路径引号，不含结尾 NUL）；失败时保留旧值。
    if (command.size() > MAX_RUN_COMMAND_LENGTH) {
        if (errorMessage) {
            *errorMessage = "Autostart command exceeds the Windows limit of 260 characters";
        }
        return false;
    }
    const LSTATUS result = RegSetValueExW(key, VALUE_NAME, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()),
        static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t)));
    return result == ERROR_SUCCESS
        ? true : fail(errorMessage, "Failed to save autostart registration", result);
}

} // namespace autostart_detail

bool getAutostartEnabled(bool& enabled, std::string* errorMessage) {
    clearError(errorMessage);
    enabled = false;
    RegistryKey key;
    const LSTATUS result = RegOpenKeyExW(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_QUERY_VALUE, &key.value);
    if (result == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    if (result != ERROR_SUCCESS) {
        return fail(errorMessage, "Failed to open autostart registry key", result);
    }
    std::wstring path;
    return getExecutablePath(path, errorMessage) &&
        autostart_detail::queryRegistration(key.value, path, enabled, errorMessage);
}

bool setAutostartEnabled(bool enabled, std::string* errorMessage) {
    clearError(errorMessage);
    debugLog(std::string("Autostart change requested: ") + (enabled ? "enabled\n" : "disabled\n"));
    const bool success = updateRegistration(enabled, errorMessage);
    debugLog(success ? "Autostart change succeeded\n" : "Autostart change failed\n");
    return success;
}

} // namespace keyrecord
