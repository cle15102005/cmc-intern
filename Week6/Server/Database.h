#pragma once

#include "framework.h"
#include "sqlite3.h"

extern sqlite3* g_db;

bool InitDatabase();
bool RegisterUser(const CString& user, const CString& pass);
bool LoginUser(const CString& user, const CString& pass);
bool SaveMessage(const CString& sender, const CString& receiver, const CString& content);
