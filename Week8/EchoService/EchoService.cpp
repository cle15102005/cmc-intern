#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment(lib, "ws2_32.lib")

// Global variables
SERVICE_STATUS_HANDLE g_hSvcStatus = nullptr;
SERVICE_STATUS g_SvcStatus = {};
SOCKET g_listenSock = INVALID_SOCKET;
FILE* g_logFile = nullptr;

// Functions definition
void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);
void WINAPI ServiceCtrlHandler(DWORD dwControl);
void RunEchoServer();

// Write log into file
void WriteLog(const char* format, ...) {
    if (g_logFile == nullptr) return;
    va_list args;
    va_start(args, format);
    vfprintf(g_logFile, format, args);
    va_end(args);
    fflush(g_logFile); // Write immediately
}

// Main function
int wmain() {
    // Service declaration and main 
    SERVICE_TABLE_ENTRYW dispatchTable[] = {
        { (LPWSTR)L"EchoService", (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { nullptr, nullptr }
    };

    // Wait for Windows to call ServiceMain
    StartServiceCtrlDispatcher(dispatchTable);
    return 0;
}

void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv) {
    ZeroMemory(&g_SvcStatus, sizeof(g_SvcStatus));
    g_SvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_SvcStatus.dwCurrentState = SERVICE_START_PENDING;
    g_SvcStatus.dwControlsAccepted = 0;
    g_SvcStatus.dwWaitHint = 3000;

    g_hSvcStatus = RegisterServiceCtrlHandler(L"EchoService", ServiceCtrlHandler);
    if (g_hSvcStatus == nullptr) return;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);

    // Open log file
    fopen_s(&g_logFile, "C:\\echo_service.log", "a");
    WriteLog("Service is starting...\n");

    // Init WinSock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        WriteLog("WSAStartup failed.\n");
        g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
        return;
    }

    // Update status: ready for Stop
    g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);

    // Run server 
    RunEchoServer();

    // Clean up
    WriteLog("Service is stopping...\n");
    WSACleanup();
    if (g_logFile) fclose(g_logFile);

    g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
}

// Handle stop from Win
void WINAPI ServiceCtrlHandler(DWORD dwControl) {
    if (dwControl == SERVICE_CONTROL_STOP) {
        g_SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_SvcStatus.dwWaitHint = 5000;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);

        WriteLog("Stop requested. Closing socket...\n");
        if (g_listenSock != INVALID_SOCKET) {
            closesocket(g_listenSock);
            g_listenSock = INVALID_SOCKET;
        }
    }
}

// TCP Server
void RunEchoServer() {
    g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSock == INVALID_SOCKET) return;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        WriteLog("Bind failed: %d\n", WSAGetLastError());
        return;
    }

    listen(g_listenSock, SOMAXCONN);
    WriteLog("Server listening on port 9999...\n");

    while (true) {
        SOCKET clientSock = accept(g_listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            WriteLog("Accept failed (likely stopped by user).\n");
            break; 
        }

        WriteLog("Client connected!\n");

        // Echo logic
        char buf[1024];
        int bytesRead;
        while ((bytesRead = recv(clientSock, buf, sizeof(buf), 0)) > 0) {
            send(clientSock, buf, bytesRead, 0);
        }

        WriteLog("Client disconnected.\n");
        closesocket(clientSock);
    }
}