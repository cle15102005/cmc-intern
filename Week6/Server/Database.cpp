#include "pch.h"        
#include "Database.h"
#include "sqlite3.h"

sqlite3* g_db = nullptr;

// Init database
bool InitDatabase() {
    int rc = sqlite3_open16(L"chat_server.db", &g_db);
    if (rc != SQLITE_OK) {
        return false;
    }

    // Create Users
    const char* sqlUsers =
        "CREATE TABLE IF NOT EXISTS users ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "username TEXT NOT NULL UNIQUE,"
        "password TEXT NOT NULL);";

    // Create Messages
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

// Regist new user
bool RegisterUser(const CString& user, const CString& pass) {
    const char* sql = "INSERT INTO users (username, password) VALUES (?, ?);";
    sqlite3_stmt* stmt = nullptr;

    // Anti sql injection
    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    // Bind data
    sqlite3_bind_text16(stmt, 1, user.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 2, pass.GetString(), -1, SQLITE_STATIC);

    // Process
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt); // Clear memory

    // Return done if success
    return (rc == SQLITE_DONE);
}

// Login
bool LoginUser(const CString& user, const CString& pass) {
    const char* sql = "SELECT id FROM users WHERE username = ? AND password = ?;";
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(g_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text16(stmt, 1, user.GetString(), -1, SQLITE_STATIC);
    sqlite3_bind_text16(stmt, 2, pass.GetString(), -1, SQLITE_STATIC);

    // Process
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // If match login data
    return (rc == SQLITE_ROW);
}

// Save message
bool SaveMessage(const CString& sender, const CString& receiver, const CString& content) {
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

