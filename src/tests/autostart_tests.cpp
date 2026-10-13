#include "platform/autostart_windows.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

// 临时键位于普通 Software 子项下，绝不触碰系统 Run 键。
struct TestRegistry {
    std::wstring path = L"Software\\KeyRecordAutostartTest_" +
        std::to_wstring(GetCurrentProcessId()) + L"_" + std::to_wstring(GetTickCount64());
    HKEY key = nullptr;
    HKEY readOnly = nullptr;
    HKEY writeOnly = nullptr;

    ~TestRegistry() {
        if (writeOnly) RegCloseKey(writeOnly);
        if (readOnly) RegCloseKey(readOnly);
        if (key) {
            RegCloseKey(key);
            const LSTATUS result = RegDeleteKeyW(HKEY_CURRENT_USER, path.c_str());
            if (result != ERROR_SUCCESS) {
                std::cerr << "Failed to remove test registry key: " << result << "\n";
            }
        }
    }
};

void runTests() {
    using keyrecord::autostart_detail::queryRegistration;
    using keyrecord::autostart_detail::writeRegistration;

    TestRegistry registry;
    expect(RegCreateKeyExW(HKEY_CURRENT_USER, registry.path.c_str(), 0, nullptr,
                          REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &registry.key, nullptr)
               == ERROR_SUCCESS, "Failed to create isolated test registry key");
    const std::wstring path = L"C:\\Program Files\\KeyRecord\\keyrecord.exe";
    std::string error = "stale error";
    bool enabled = true;
    expect(queryRegistration(registry.key, path, enabled, &error) && !enabled && error.empty(),
           "Missing registration must be disabled and clear stale errors");
    DWORD bytes = 0;
    expect(RegQueryValueExW(registry.key, L"KeyRecord", nullptr, nullptr, nullptr, &bytes)
               == ERROR_FILE_NOT_FOUND, "Query must not create a registration");

    expect(writeRegistration(registry.key, path, true, &error), "Failed to enable autostart");
    expect(queryRegistration(registry.key, path, enabled, &error) && enabled,
           "Saved registration must be enabled");
    wchar_t command[300] = {};
    DWORD type = 0;
    bytes = sizeof(command);
    expect(RegQueryValueExW(registry.key, L"KeyRecord", nullptr, &type,
                           reinterpret_cast<BYTE*>(command), &bytes) == ERROR_SUCCESS,
           "Failed to inspect saved command");
    expect(type == REG_SZ && std::wstring(command) == L"\"" + path + L"\"" &&
           bytes == (path.size() + 3) * sizeof(wchar_t),
           "Saved command must quote the full path and include a terminator");
    expect(writeRegistration(registry.key, path, true, &error), "Repeated enable must succeed");

    // 重新打开句柄验证持久化，同时使用真实受限句柄覆盖权限错误。
    expect(RegOpenKeyExW(HKEY_CURRENT_USER, registry.path.c_str(), 0, KEY_QUERY_VALUE,
                        &registry.readOnly) == ERROR_SUCCESS, "Failed to open read-only test key");
    expect(queryRegistration(registry.readOnly, path, enabled, &error) && enabled,
           "Registration must survive reopening the key");
    expect(!writeRegistration(registry.readOnly, path, true, &error) && !error.empty(),
           "Writing through a read-only handle must fail");
    expect(!writeRegistration(registry.readOnly, path, false, &error) && !error.empty(),
           "Deleting through a read-only handle must fail");
    expect(queryRegistration(registry.key, path, enabled, &error) && enabled,
           "Failed changes must preserve the existing registration");
    expect(RegOpenKeyExW(HKEY_CURRENT_USER, registry.path.c_str(), 0, KEY_SET_VALUE,
                        &registry.writeOnly) == ERROR_SUCCESS, "Failed to open write-only test key");
    expect(!queryRegistration(registry.writeOnly, path, enabled, &error) && !enabled && !error.empty(),
           "Reading through a write-only handle must fail and clear enabled state");

    const std::wstring movedPath = L"D:\\KeyRecord\\keyrecord.exe";
    expect(queryRegistration(registry.key, movedPath, enabled, &error) && !enabled,
           "A registration for another path must not be checked");
    expect(writeRegistration(registry.key, movedPath, true, &error) &&
           queryRegistration(registry.key, movedPath, enabled, &error) && enabled,
           "Enabling after a move must replace the saved path");

    // 使用 Unicode 转义测试中文目录，源代码中的字符串文案保持英文。
    const std::wstring unicodePath = L"C:\\\u7528\u6237\\Key Record\\keyrecord.exe";
    expect(writeRegistration(registry.key, unicodePath, true, &error) &&
           queryRegistration(registry.key, unicodePath, enabled, &error) && enabled,
           "Unicode paths with spaces must round-trip");
    const std::wstring maxPath = L"C:\\" + std::wstring(241, L'a') + L"\\keyrecord.exe";
    expect(maxPath.size() == 258, "Incorrect command-length boundary fixture");
    expect(writeRegistration(registry.key, maxPath, true, &error) &&
           queryRegistration(registry.key, maxPath, enabled, &error) && enabled,
           "A 260-character quoted command must be accepted");
    expect(!writeRegistration(registry.key, maxPath + L"a", true, &error) && !error.empty(),
           "Commands exceeding 260 characters must fail");
    expect(queryRegistration(registry.key, maxPath, enabled, &error) && enabled,
           "Overlong commands must not replace the existing registration");

    const DWORD unrelated = 42;
    expect(RegSetValueExW(registry.key, L"Unrelated", 0, REG_DWORD,
                         reinterpret_cast<const BYTE*>(&unrelated), sizeof(unrelated)) == ERROR_SUCCESS,
           "Failed to create unrelated value");
    expect(writeRegistration(registry.key, path, false, &error) &&
           queryRegistration(registry.key, path, enabled, &error) && !enabled,
           "Disabling must remove the registration");
    expect(writeRegistration(registry.key, path, false, &error), "Repeated disable must succeed");
    DWORD preserved = 0;
    bytes = sizeof(preserved);
    expect(RegQueryValueExW(registry.key, L"Unrelated", nullptr, nullptr,
                           reinterpret_cast<BYTE*>(&preserved), &bytes) == ERROR_SUCCESS && preserved == unrelated,
           "Disabling must preserve unrelated values");

    expect(RegSetValueExW(registry.key, L"KeyRecord", 0, REG_DWORD,
                         reinterpret_cast<const BYTE*>(&unrelated), sizeof(unrelated)) == ERROR_SUCCESS,
           "Failed to create malformed value");
    expect(!queryRegistration(registry.key, path, enabled, &error) && !enabled && !error.empty(),
           "Invalid value types must report an error");
    const wchar_t unterminated[] = {L'a', L'b'};
    expect(RegSetValueExW(registry.key, L"KeyRecord", 0, REG_SZ,
                         reinterpret_cast<const BYTE*>(unterminated), sizeof(unterminated)) == ERROR_SUCCESS,
           "Failed to create unterminated value");
    expect(!queryRegistration(registry.key, path, enabled, &error) && !enabled,
           "Unterminated registry strings must report an error");
}

} // namespace

int main() {
    try {
        runTests();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 1;
    }
}
