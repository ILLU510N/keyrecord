#include "platform/server_launcher.h"

#include "app_config.h"
#include "visualization_url.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <string>
#include <vector>

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

bool openVisualizationPage(std::string* errorMessage) {
    if (errorMessage) {
        errorMessage->clear();
    }

    if (!isManagedServerRunning() && !createManagedServerProcess(errorMessage)) {
        return false;
    }

    const auto values = parseConfigFile(getDefaultConfigFilePath());
    return openDefaultBrowser(buildVisualizationPageUrl(values), errorMessage);
}

void shutdownVisualizationServer() {
    closeManagedServerHandles();
}

} // namespace keyrecord
