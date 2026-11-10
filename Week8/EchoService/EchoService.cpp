#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <atlstr.h>
#include <map>
#include <vector>
#include <mutex>
#include "sqlite3.h"

#pragma comment(lib, "ws2_32.lib")

// Global service variables
SERVICE_STATUS_HANDLE g_hSvcStatus = nullptr;
SERVICE_STATUS g_SvcStatus = {};
SOCKET g_listenSock = INVALID_SOCKET;
FILE* g_logFile = nullptr;
sqlite3* g_db = nullptr;

std::map<CStringW, SOCKET> g_onlineUsers;
std::mutex g_onlineMutex;

// Function declarations
void WINAPI ServiceMain(DWORD dwArgc, LPWSTR* lpszArgv);
void WINAPI ServiceCtrlHandler(DWORD dwControl);
void RunChatServer();
bool InitDatabase();
void WriteLog(const wchar_t* format, ...);

// Helper to write formatted log with timestamp
void WriteLog(const wchar_t* format, ...) {
    if (g_logFile == nullptr) return;

    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(g_logFile, L"[%04d-%02d-%02d %02d:%02d:%02d] ", 
        st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    va_list args;
    va_start(args, format);
    vfwprintf(g_logFile, format, args);
    va_end(args);
    fflush(g_logFile);
}

// Substring extraction helper (equivalent to AfxExtractSubString)
bool ExtractToken(CStringW& rString, const CStringW& fullString, int index, wchar_t sep = L'|') {
    int cur = 0;
    int start = 0;
    while (cur < index) {
        int pos = fullString.Find(sep, start);
        if (pos == -1) {
            rString.Empty();
            return false;
        }
        start = pos + 1;
        cur++;
    }
    int end = fullString.Find(sep, start);
    if (end == -1) {
        rString = fullString.Mid(start);
    } else {
        rString = fullString.Mid(start, end - start);
    }
    return true;
}

// Database Operations
bool InitDatabase() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wchar_t* pSlash = wcsrchr(exePath, L'\\');
    if (pSlash) {
        *(pSlash + 1) = L'\0';
    }
    wcscat_s(exePath, MAX_PATH, L"chat_server.db");

    int rc = sqlite3_open16(exePath, &g_db);
    if (rc != SQLITE_OK) {
        WriteLog(L"Failed to open SQLite database: %ls\n", exePath);
        return false;
    }
    WriteLog(L"SQLite database opened at: %ls\n", exePath);

    const char* sqlUsers =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password TEXT NOT NULL);";

    const char* sqlMessages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "sender TEXT NOT NULL,"
        "receiver TEXT NOT NULL,"
        "content TEXT NOT NULL,"
        "timestamp TEXT NOT NULL);";

    char* errMsg = nullptr;
    sqlite3_exec(g_db, sqlUsers, nullptr, nullptr, &errMsg);
    sqlite3_exec(g_db, sqlMessages, nullptr, nullptr, &errMsg);

    return true;
}

bool RegisterUser(const CStringW& user, const CStringW& pass) {
    if (g_db == nullptr) return false;
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text16(stmt, 1, user.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 2, pass.GetString(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

bool LoginUser(const CStringW& user, const CStringW& pass) {
    if (g_db == nullptr) return false;
    const char* sql = "SELECT id FROM users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text16(stmt, 1, user.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 2, pass.GetString(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_ROW);
}

bool SaveMessage(const CStringW& sender, const CStringW& receiver, const CStringW& content) {
    if (g_db == nullptr) return false;
    const char* sql = "INSERT INTO messages (sender, receiver, content, timestamp) VALUES (?, ?, ?, datetime('now', 'localtime'));";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text16(stmt, 1, sender.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 2, receiver.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 3, content.GetString(), -1, SQLITE_STATIC);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE);
}

std::vector<CStringW> GetAllUsers() {
    std::vector<CStringW> userList;
    if (g_db == nullptr) return userList;

    const char* sql = "SELECT username FROM users ORDER BY username ASC;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const wchar_t* pUser = (const wchar_t*)sqlite3_column_text16(stmt, 0);
            if (pUser) {
                userList.push_back(pUser);
            }
        }
        sqlite3_finalize(stmt);
    }
    return userList;
}

// Network Helpers
bool SendAll(SOCKET sock, const void* pData, int nSize) {
    const char* pBuf = (const char*)pData;
    int nSent = 0;
    while (nSent < nSize) {
        int n = send(sock, pBuf + nSent, nSize - nSent, 0);
        if (n == SOCKET_ERROR) return false;
        nSent += n;
    }
    return true;
}

bool RecvAll(SOCKET sock, void* pData, int nSize) {
    char* pBuf = (char*)pData;
    int nReceived = 0;
    while (nReceived < nSize) {
        int n = recv(sock, pBuf + nReceived, nSize - nReceived, 0);
        if (n == SOCKET_ERROR || n == 0) return false;
        nReceived += n;
    }
    return true;
}

bool SendMessageToSocket(SOCKET sock, const CStringW& strMsg) {
    if (sock == INVALID_SOCKET) return false;

    uint32_t len = (strMsg.GetLength() + 1) * sizeof(wchar_t);
    uint32_t netLen = htonl(len);

    if (!SendAll(sock, &netLen, sizeof(netLen))) return false;
    if (!SendAll(sock, strMsg.GetString(), len)) return false;

    return true;
}

bool RecvMessageFromSocket(SOCKET sock, CStringW& strMsgOut) {
    if (sock == INVALID_SOCKET) return false;

    uint32_t netLen = 0;
    if (!RecvAll(sock, &netLen, sizeof(netLen))) return false;
    uint32_t payloadLen = ntohl(netLen);

    if (payloadLen == 0 || payloadLen > 1024 * 1024) return false;

    wchar_t* pBuffer = new wchar_t[payloadLen / sizeof(wchar_t)];
    if (!RecvAll(sock, pBuffer, payloadLen)) {
        delete[] pBuffer;
        return false;
    }

    strMsgOut = pBuffer;
    delete[] pBuffer;
    return true;
}

// Broadcast Online/Offline user list to all online clients
void BroadcastUserList() {
    std::vector<CStringW> allUsers = GetAllUsers();
    CStringW onlinePart = L"";
    CStringW offlinePart = L"";

    SOCKET socksToSend[100];
    int count = 0;

    g_onlineMutex.lock();
    for (const auto& u : allUsers) {
        if (g_onlineUsers.find(u) != g_onlineUsers.end()) {
            onlinePart += L"|" + u;
        } else {
            offlinePart += L"|" + u;
        }
    }

    for (auto it = g_onlineUsers.begin(); it != g_onlineUsers.end(); ++it) {
        if (count < 100) {
            socksToSend[count++] = it->second;
        }
    }
    g_onlineMutex.unlock();

    CStringW userList = L"USER_LIST|ONLINE" + onlinePart + L"|OFFLINE" + offlinePart;
    for (int i = 0; i < count; ++i) {
        SendMessageToSocket(socksToSend[i], userList);
    }
    WriteLog(L"[BROADCAST] Broadcasted user list to %d clients.\n", count);
}

// Client Handler Thread
DWORD WINAPI ClientHandlerThread(LPVOID pParam) {
    SOCKET clientSock = (SOCKET)pParam;
    CStringW loggedInUser = L"";
    CStringW recvMsg;

    WriteLog(L"[CONNECT] New client socket %d connected.\n", (int)clientSock);

    while (RecvMessageFromSocket(clientSock, recvMsg)) {
        int barPos = recvMsg.Find(L'|');
        CStringW cmd = (barPos == -1) ? recvMsg : recvMsg.Left(barPos);
        CStringW response = L"";

        if (cmd == L"REG") {
            CStringW user, pass;
            ExtractToken(user, recvMsg, 1, L'|');
            ExtractToken(pass, recvMsg, 2, L'|');

            if (RegisterUser(user, pass)) {
                response = L"REG_OK";
                WriteLog(L"[REG] User registered successfully: %ls\n", user.GetString());
                BroadcastUserList();
            } else {
                response = L"REG_FAIL|Username exists!";
                WriteLog(L"[REG] User registration failed (already exists): %ls\n", user.GetString());
            }
        }
        else if (cmd == L"LOGIN") {
            CStringW user, pass;
            ExtractToken(user, recvMsg, 1, L'|');
            ExtractToken(pass, recvMsg, 2, L'|');

            if (LoginUser(user, pass)) {
                loggedInUser = user;
                SendMessageToSocket(clientSock, L"LOGIN_OK");
                response = L"";

                g_onlineMutex.lock();
                g_onlineUsers[user] = clientSock;
                g_onlineMutex.unlock();

                WriteLog(L"[LOGIN] User '%ls' logged in successfully.\n", user.GetString());
                BroadcastUserList();
            } else {
                response = L"LOGIN_FAIL|Wrong username or password!";
                WriteLog(L"[LOGIN] User '%ls' login failed: wrong credentials.\n", user.GetString());
            }
        }
        else if (cmd == L"CHAT") {
            CStringW receiver, content;
            ExtractToken(receiver, recvMsg, 1, L'|');
            ExtractToken(content, recvMsg, 2, L'|');

            if (loggedInUser.IsEmpty()) {
                response = L"CHAT_FAIL|You must login first!";
            } else {
                SaveMessage(loggedInUser, receiver, content);
                WriteLog(L"[CHAT] %ls -> %ls: %ls\n", loggedInUser.GetString(), receiver.GetString(), content.GetString());

                SOCKET recipientSock = INVALID_SOCKET;
                g_onlineMutex.lock();
                auto it = g_onlineUsers.find(receiver);
                if (it != g_onlineUsers.end()) {
                    recipientSock = it->second;
                }
                g_onlineMutex.unlock();

                if (recipientSock != INVALID_SOCKET) {
                    CStringW fwdMsg = L"MSG|" + loggedInUser + L"|" + content;
                    SendMessageToSocket(recipientSock, fwdMsg);
                    response = L"CHAT_OK";
                } else {
                    response = L"CHAT_FAIL|User is offline!";
                }
            }
        }
        else {
            response = L"ERROR|Invalid command";
        }

        if (!response.IsEmpty()) {
            SendMessageToSocket(clientSock, response);
        }
    }

    if (!loggedInUser.IsEmpty()) {
        g_onlineMutex.lock();
        g_onlineUsers.erase(loggedInUser);
        g_onlineMutex.unlock();

        WriteLog(L"[DISCONNECT] User '%ls' disconnected.\n", loggedInUser.GetString());
        BroadcastUserList();
    } else {
        WriteLog(L"[DISCONNECT] Anonymous client socket %d closed.\n", (int)clientSock);
    }

    closesocket(clientSock);
    return 0;
}

// Main TCP Listening Server
void RunChatServer() {
    g_listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSock == INVALID_SOCKET) {
        WriteLog(L"Socket creation failed: %d\n", WSAGetLastError());
        return;
    }

    // Set SO_REUSEADDR so service can rebind quickly
    BOOL opt = TRUE;
    setsockopt(g_listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        WriteLog(L"Bind failed on port 9999: %d\n", WSAGetLastError());
        closesocket(g_listenSock);
        g_listenSock = INVALID_SOCKET;
        return;
    }

    if (listen(g_listenSock, SOMAXCONN) == SOCKET_ERROR) {
        WriteLog(L"Listen failed: %d\n", WSAGetLastError());
        closesocket(g_listenSock);
        g_listenSock = INVALID_SOCKET;
        return;
    }

    WriteLog(L"Chat Server Service listening on port 9999...\n");

    while (true) {
        SOCKET clientSock = accept(g_listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) {
            WriteLog(L"Accept loop terminated (likely stopped by Service Control).\n");
            break;
        }

        HANDLE hThread = CreateThread(nullptr, 0, ClientHandlerThread, (LPVOID)clientSock, 0, nullptr);
        if (hThread) {
            CloseHandle(hThread);
        } else {
            closesocket(clientSock);
        }
    }
}

// Service Entry & Control
void WINAPI ServiceCtrlHandler(DWORD dwControl) {
    if (dwControl == SERVICE_CONTROL_STOP) {
        g_SvcStatus.dwCurrentState = SERVICE_STOP_PENDING;
        g_SvcStatus.dwWaitHint = 5000;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);

        WriteLog(L"Stop requested from Service Control Manager. Shutting down server...\n");
        if (g_listenSock != INVALID_SOCKET) {
            closesocket(g_listenSock);
            g_listenSock = INVALID_SOCKET;
        }
    }
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

    _wfopen_s(&g_logFile, L"C:\\echo_service.log", L"a, ccs=UTF-8");
    WriteLog(L"=== EchoService (Chat Server Windows Service) Starting ===\n");

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        WriteLog(L"WSAStartup failed.\n");
        g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
        return;
    }

    if (!InitDatabase()) {
        WriteLog(L"Warning: Database initialization failed or cannot open file.\n");
    }

    g_SvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    g_SvcStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
    WriteLog(L"Service is now RUNNING.\n");

    RunChatServer();

    WriteLog(L"Service is stopping and cleaning up resources...\n");
    if (g_db) {
        sqlite3_close(g_db);
        g_db = nullptr;
    }
    WSACleanup();

    WriteLog(L"=== EchoService Stopped ===\n\n");
    if (g_logFile) {
        fclose(g_logFile);
        g_logFile = nullptr;
    }

    g_SvcStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_hSvcStatus, &g_SvcStatus);
}

int wmain() {
    SERVICE_TABLE_ENTRYW dispatchTable[] = {
        { (LPWSTR)L"EchoService", (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { nullptr, nullptr }
    };

    StartServiceCtrlDispatcherW(dispatchTable);
    return 0;
}
