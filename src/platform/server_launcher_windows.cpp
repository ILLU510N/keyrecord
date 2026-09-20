#include "platform/server_launcher.h"

#include "app_config.h"
#include "server_config.h"
#include "visualization_url.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <shellapi.h>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <string>
#include <vector>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "iphlpapi.lib")

namespace {

HANDLE serverProcess = nullptr;
HANDLE serverJob = nullptr;

void setError(std::string* errorMessage, const std::string& message) {
    if (errorMessage) {
        *errorMessage = message;
    }
}

std::string makeWindowsError(const char* action, DWORD error) {
    return std::string(action) + " (Windows error " + std::to_string(error) + ")";
}

void closeManagedServerHandles() {
    if (serverProcess) {
        CloseHandle(serverProcess);
        serverProcess = nullptr;
    }
    if (serverJob) {
        // Job 关闭后会终止其中的服务进程，避免主程序退出后留下后台服务。
        CloseHandle(serverJob);
        serverJob = nullptr;
    }
}

bool isManagedServerRunning() {
    if (!serverProcess) {
        return false;
    }
    if (WaitForSingleObject(serverProcess, 0) == WAIT_TIMEOUT) {
        return true;
    }
    closeManagedServerHandles();
    return false;
}

bool getCurrentExecutablePath(std::filesystem::path& path, std::string* errorMessage) {
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD copied = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (copied == 0) {
            setError(errorMessage, makeWindowsError("Failed to resolve application path", GetLastError()));
            return false;
        }
        if (copied < buffer.size()) {
            path = std::wstring(buffer.data(), copied);
            return true;
        }
        if (buffer.size() >= 32768) {
            setError(errorMessage, "Application path is too long");
            return false;
        }
        buffer.resize(buffer.size() * 2);
    }
}

bool createManagedServerProcess(std::string* errorMessage) {
    std::filesystem::path applicationPath;
    if (!getCurrentExecutablePath(applicationPath, errorMessage)) {
        return false;
    }

    const std::filesystem::path serverPath = applicationPath.parent_path() / L"keyrecord_server.exe";
    std::error_code filesystemError;
    if (!std::filesystem::is_regular_file(serverPath, filesystemError) || filesystemError) {
        setError(errorMessage, "Visualization server executable was not found next to the application");
        return false;
    }

    HANDLE job = CreateJobObjectW(nullptr, nullptr);
    if (!job) {
        setError(errorMessage, makeWindowsError("Failed to create visualization server job", GetLastError()));
        return false;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo = {};
    jobInfo.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation, &jobInfo, sizeof(jobInfo))) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        setError(errorMessage, makeWindowsError("Failed to configure visualization server job", error));
        return false;
    }

    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};
    const std::wstring workingDirectory = serverPath.parent_path().wstring();
    // 先挂起子进程并加入 Job，再恢复执行，防止服务在失败路径中脱离生命周期管理。
    if (!CreateProcessW(
            serverPath.c_str(),
            nullptr,
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED,
            nullptr,
            workingDirectory.c_str(),
            &startupInfo,
            &processInfo)) {
        const DWORD error = GetLastError();
        CloseHandle(job);
        setError(errorMessage, makeWindowsError("Failed to start visualization server", error));
        return false;
    }

    if (!AssignProcessToJobObject(job, processInfo.hProcess)) {
        const DWORD error = GetLastError();
        TerminateProcess(processInfo.hProcess, 1);
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        CloseHandle(job);
        setError(errorMessage, makeWindowsError("Failed to manage visualization server process", error));
        return false;
    }

    if (ResumeThread(processInfo.hThread) == static_cast<DWORD>(-1)) {
        const DWORD error = GetLastError();
        CloseHandle(processInfo.hThread);
        CloseHandle(processInfo.hProcess);
        CloseHandle(job);
        setError(errorMessage, makeWindowsError("Failed to resume visualization server", error));
        return false;
    }

    CloseHandle(processInfo.hThread);
    serverProcess = processInfo.hProcess;
    serverJob = job;
    return true;
}

bool openDefaultBrowser(const std::string& url, std::string* errorMessage) {
    if (url.empty()) {
        setError(errorMessage, "Visualization page URL is empty");
        return false;
    }

    const int wideLength = MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, nullptr, 0);
    if (wideLength <= 0) {
        setError(errorMessage, makeWindowsError("Failed to encode visualization page URL", GetLastError()));
        return false;
    }
    std::vector<wchar_t> wideUrl(static_cast<std::size_t>(wideLength));
    if (MultiByteToWideChar(CP_UTF8, 0, url.c_str(), -1, wideUrl.data(), wideLength) == 0) {
        setError(errorMessage, makeWindowsError("Failed to encode visualization page URL", GetLastError()));
        return false;
    }

    const auto result = reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wideUrl.data(), nullptr, nullptr, SW_SHOWNORMAL));
    if (result <= 32) {
        setError(errorMessage, "Failed to open visualization page (ShellExecute error " + std::to_string(result) + ")");
        return false;
    }
    return true;
}

} // namespace

namespace keyrecord {

bool isProcessListeningOnPort(unsigned long processId, unsigned short port) {
    if (processId == 0) {
        return false;
    }

    // Check IPv4 listeners.
    for (int attempt = 0; attempt < 3; ++attempt) {
        DWORD size = 0;
        DWORD ret = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);
        if (size == 0) {
            break;
        }
        std::vector<unsigned char> buffer(size);
        ret = GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0);
        if (ret == NO_ERROR) {
            const auto* table = reinterpret_cast<const MIB_TCPTABLE_OWNER_PID*>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                if (row.dwOwningPid == processId && ntohs(static_cast<u_short>(row.dwLocalPort)) == port) {
                    return true;
                }
            }
            break;
        }
        if (ret != ERROR_INSUFFICIENT_BUFFER) {
            break;
        }
    }

    // Check IPv6 listeners.
    for (int attempt = 0; attempt < 3; ++attempt) {
        DWORD size = 0;
        DWORD ret = GetExtendedTcpTable(nullptr, &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0);
        if (size == 0) {
            break;
        }
        std::vector<unsigned char> buffer(size);
        ret = GetExtendedTcpTable(buffer.data(), &size, FALSE, AF_INET6, TCP_TABLE_OWNER_PID_LISTENER, 0);
        if (ret == NO_ERROR) {
            const auto* table = reinterpret_cast<const MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
            for (DWORD i = 0; i < table->dwNumEntries; ++i) {
                const auto& row = table->table[i];
                if (row.dwOwningPid == processId && ntohs(static_cast<u_short>(row.dwLocalPort)) == port) {
                    return true;
                }
            }
            break;
        }
        if (ret != ERROR_INSUFFICIENT_BUFFER) {
            break;
        }
    }

    return false;
}

bool waitForProcessListening(
    void* processHandle,
    unsigned short port,
    std::chrono::milliseconds timeout,
    std::string* errorMessage) {
    if (!processHandle) {
        setError(errorMessage, "Visualization server process handle is invalid");
        return false;
    }

    const HANDLE process = static_cast<HANDLE>(processHandle);
    const DWORD childPid = GetProcessId(process);
    if (childPid == 0) {
        setError(errorMessage, makeWindowsError("Failed to get visualization server process ID", GetLastError()));
        return false;
    }

    const auto startTime = std::chrono::steady_clock::now();
    constexpr DWORD pollIntervalMs = 25;

    for (;;) {
        const DWORD initialWait = WaitForSingleObject(process, 0);
        if (initialWait == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(process, &exitCode)) {
                setError(errorMessage, "Visualization server exited unexpectedly with code " + std::to_string(exitCode));
            } else {
                setError(errorMessage, makeWindowsError("Visualization server exited unexpectedly", GetLastError()));
            }
            return false;
        }

        if (isProcessListeningOnPort(childPid, port)) {
            // Re-verify child is still alive in case it exited immediately after entering listen state.
            if (WaitForSingleObject(process, 0) == WAIT_OBJECT_0) {
                DWORD exitCode = 0;
                if (GetExitCodeProcess(process, &exitCode)) {
                    setError(errorMessage, "Visualization server exited unexpectedly with code " + std::to_string(exitCode));
                } else {
                    setError(errorMessage, makeWindowsError("Visualization server exited unexpectedly", GetLastError()));
                }
                return false;
            }
            return true;
        }

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime);
        if (elapsed >= timeout) {
            setError(errorMessage, "Timed out waiting for visualization server to listen on port " + std::to_string(port));
            return false;
        }

        const auto remaining = timeout - elapsed;
        const DWORD waitMs = static_cast<DWORD>(
            std::min<long long>(pollIntervalMs, remaining.count()));
        const DWORD waitResult = WaitForSingleObject(process, waitMs);
        if (waitResult == WAIT_OBJECT_0) {
            DWORD exitCode = 0;
            if (GetExitCodeProcess(process, &exitCode)) {
                setError(errorMessage, "Visualization server exited unexpectedly with code " + std::to_string(exitCode));
            } else {
                setError(errorMessage, makeWindowsError("Visualization server exited unexpectedly", GetLastError()));
            }
            return false;
        }
    }
}

bool openVisualizationPage(std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    if (!isManagedServerRunning() && !createManagedServerProcess(errorMessage)) {
        return false;
    }

    const auto values = parseConfigFile(getDefaultConfigFilePath());
    const unsigned short port = values.port.value_or(DEFAULT_SERVER_PORT);

    if (!waitForProcessListening(serverProcess, port, std::chrono::milliseconds(5000), errorMessage)) {
        if (serverProcess) {
            TerminateProcess(serverProcess, 1);
        }
        closeManagedServerHandles();
        return false;
    }

    return openDefaultBrowser(buildVisualizationPageUrl(values), errorMessage);
}

void shutdownVisualizationServer() {
    closeManagedServerHandles();
}

} // namespace keyrecord
