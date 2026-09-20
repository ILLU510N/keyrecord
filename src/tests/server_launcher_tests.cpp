#include "../platform/server_launcher.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << message << "\n";
        return false;
    }
    return true;
}

bool expectContains(const std::string& actual, const char* expected, const char* message) {
    if (actual.find(expected) == std::string::npos) {
        std::cerr << message << "\nMissing: " << expected << "\nActual: " << actual << "\n";
        return false;
    }
    return true;
}

bool testInvalidHandle() {
    std::string errorMessage;
    const bool ok = keyrecord::waitForProcessListening(
        nullptr, 1234, std::chrono::milliseconds(100), &errorMessage);
    return expect(!ok, "Null process handle should fail") &&
           expectContains(errorMessage, "Visualization server process handle is invalid",
                          "Should report invalid process handle");
}

bool testProcessListeningDetection() {
    WSADATA wsaData = {};
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }

    const SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        WSACleanup();
        std::cerr << "socket creation failed\n";
        return false;
    }

    sockaddr_in bindAddr = {};
    bindAddr.sin_family = AF_INET;
    bindAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    bindAddr.sin_port = 0; // Bind to ephemeral port

    if (bind(listenSocket, reinterpret_cast<const sockaddr*>(&bindAddr), sizeof(bindAddr)) != 0) {
        closesocket(listenSocket);
        WSACleanup();
        std::cerr << "bind failed\n";
        return false;
    }

    if (listen(listenSocket, SOMAXCONN) != 0) {
        closesocket(listenSocket);
        WSACleanup();
        std::cerr << "listen failed\n";
        return false;
    }

    sockaddr_in localAddr = {};
    int localAddrLen = sizeof(localAddr);
    if (getsockname(listenSocket, reinterpret_cast<sockaddr*>(&localAddr), &localAddrLen) != 0) {
        closesocket(listenSocket);
        WSACleanup();
        std::cerr << "getsockname failed\n";
        return false;
    }
    const unsigned short boundPort = ntohs(localAddr.sin_port);

    const DWORD currentPid = GetCurrentProcessId();
    bool ok = true;
    ok = expect(keyrecord::isProcessListeningOnPort(currentPid, boundPort),
                "Should detect current process listening on bound port") && ok;
    ok = expect(!keyrecord::isProcessListeningOnPort(currentPid + 999999, boundPort),
                "Should return false for different process ID") && ok;
    ok = expect(!keyrecord::isProcessListeningOnPort(currentPid, 0),
                "Should return false for unused port 0") && ok;

    // Test waitForProcessListening with current process which is already listening
    std::string errorMessage;
    ok = expect(keyrecord::waitForProcessListening(
                    GetCurrentProcess(), boundPort, std::chrono::milliseconds(1000), &errorMessage),
                "waitForProcessListening should succeed for already listening process") && ok;
    ok = expect(errorMessage.empty(),
                "Error message should be empty on success") && ok;

    closesocket(listenSocket);

    ok = expect(!keyrecord::isProcessListeningOnPort(currentPid, boundPort),
                "Should not detect listening after socket is closed") && ok;

    WSACleanup();
    return ok;
}

bool testWaitForProcessExit() {
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    std::wstring cmdLine = L"cmd.exe /c exit 19";
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    if (!CreateProcessW(
            nullptr,
            cmdBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo)) {
        std::cerr << "Failed to create exit test process\n";
        return false;
    }
    CloseHandle(processInfo.hThread);

    std::string errorMessage;
    const bool okWait = keyrecord::waitForProcessListening(
        processInfo.hProcess, 59123, std::chrono::milliseconds(3000), &errorMessage);
    CloseHandle(processInfo.hProcess);

    bool ok = true;
    ok = expect(!okWait, "Terminated child process should fail wait") && ok;
    ok = expectContains(errorMessage, "Visualization server exited unexpectedly with code 19",
                        "Should report child process exit code") && ok;
    return ok;
}

bool testWaitForProcessTimeout() {
    STARTUPINFOW startupInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo = {};

    std::wstring cmdLine = L"cmd.exe /c ping 127.0.0.1 -n 3 >nul";
    std::vector<wchar_t> cmdBuffer(cmdLine.begin(), cmdLine.end());
    cmdBuffer.push_back(L'\0');

    if (!CreateProcessW(
            nullptr,
            cmdBuffer.data(),
            nullptr,
            nullptr,
            FALSE,
            CREATE_NO_WINDOW,
            nullptr,
            nullptr,
            &startupInfo,
            &processInfo)) {
        std::cerr << "Failed to create timeout test process\n";
        return false;
    }
    CloseHandle(processInfo.hThread);

    std::string errorMessage;
    const bool okWait = keyrecord::waitForProcessListening(
        processInfo.hProcess, 59124, std::chrono::milliseconds(100), &errorMessage);

    TerminateProcess(processInfo.hProcess, 1);
    CloseHandle(processInfo.hProcess);

    bool ok = true;
    ok = expect(!okWait, "Non-listening child process should time out") && ok;
    ok = expectContains(errorMessage, "Timed out waiting for visualization server to listen on port 59124",
                        "Should report timeout message") && ok;
    return ok;
}

} // namespace

int main() {
    bool ok = true;
    ok = testInvalidHandle() && ok;
    ok = testProcessListeningDetection() && ok;
    ok = testWaitForProcessExit() && ok;
    ok = testWaitForProcessTimeout() && ok;
    return ok ? 0 : 1;
}
