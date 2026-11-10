
// ClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"
#include "sqlite3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

sqlite3* g_clientDb = nullptr;
bool RecvMessageFromSocket(SOCKET sock, CString& strMsgOut);
bool SendMessageToSocket(SOCKET sock, const CString& strMsg);

UINT __cdecl ClientReceiveThread(LPVOID pParam) {
	CClientDlg* pDlg = (CClientDlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();

	CString recvMsg;
	while (RecvMessageFromSocket(g_clientSocket, recvMsg)) {
		CString* pStr = new CString(recvMsg);
		::PostMessage(hWnd, WM_NET_MESSAGE, 0, (LPARAM)pStr);
	}

	if (::IsWindow(hWnd)) {
		AfxMessageBox(L"Disconnected to server!");
		pDlg->EndDialog(IDCANCEL);
	}
	return 0;
}

// On/Off Autorun
void RegisterAutorun(bool bEnable)
{
	// Registry entry for Windows for current user
	const wchar_t* kRunKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	const wchar_t* kValueName = L"MyChatClient";

	HKEY hKey = nullptr;
	LONG lRes = RegOpenKeyEx(HKEY_CURRENT_USER, kRunKey, 0, KEY_WRITE, &hKey);

	if (lRes != ERROR_SUCCESS) return;

	if (bEnable) {
		// Current entry for .exe
		wchar_t szExePath[MAX_PATH] = {};
		GetModuleFileName(nullptr, szExePath, MAX_PATH);

		// Write in Registry
		RegSetValueEx(hKey, kValueName, 0, REG_SZ,
			(const BYTE*)szExePath,
			(DWORD)((wcslen(szExePath) + 1) * sizeof(wchar_t)));
	}
	else {
		// Delete key
		RegDeleteValue(hKey, kValueName);
	}
	RegCloseKey(hKey);
}

// Check Autorun if on/off
bool IsAutorunEnabled()
{
	const wchar_t* kRunKey = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run";
	const wchar_t* kValueName = L"MyChatClient";

	HKEY hKey = nullptr;
	// Read only (KEY_READ)
	if (RegOpenKeyEx(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
		return false;

	wchar_t szBuf[MAX_PATH] = {};
	DWORD dwSize = sizeof(szBuf);
	DWORD dwType = 0;

	// Read value
	LONG lRes = RegQueryValueEx(hKey, kValueName, nullptr, &dwType, (LPBYTE)szBuf, &dwSize);
	RegCloseKey(hKey);

	return (lRes == ERROR_SUCCESS && dwType == REG_SZ);
}

// Send requested bytes
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

// Receive requestd bytes
bool RecvAll(SOCKET sock, void* pData, int nSize) {
	char* pBuf = (char*)pData;
	int nReceived = 0;
	while (nReceived < nSize) {
		int n = recv(sock, pBuf + nReceived, nSize - nReceived, 0);
		if (n == SOCKET_ERROR || n == 0) return false; // 0 = disconnect
		nReceived += n;
	}
	return true;
}

bool SendMessageToSocket(SOCKET sock, const CString& strMsg) {
	if (sock == INVALID_SOCKET) return false;

	// Get byte size of message
	uint32_t len = (strMsg.GetLength() + 1) * sizeof(wchar_t);

	// Convert to Network Byte Order (Big-Endian) to send to net
	uint32_t netLen = htonl(len);

	// Send first 4 bytes
	if (!SendAll(sock, &netLen, sizeof(netLen))) return false;

	// Send message conetent
	if (!SendAll(sock, strMsg.GetString(), len)) return false;

	return true;
}

bool RecvMessageFromSocket(SOCKET sock, CString& strMsgOut) {
	if (sock == INVALID_SOCKET) return false;

	uint32_t netLen = 0;
	// Read 4-bytes length
	if (!RecvAll(sock, &netLen, sizeof(netLen))) return false;

	// Convert from Network Byte Order to normal
	uint32_t payloadLen = ntohl(netLen);
	if (payloadLen == 0 || payloadLen > 1024 * 1024) return false;

	// Allocate memory size
	wchar_t* pBuffer = new wchar_t[payloadLen / sizeof(wchar_t)];

	// Read content
	if (!RecvAll(sock, pBuffer, payloadLen)) {
		delete[] pBuffer;
		return false;
	}

	// Add to CString
	strMsgOut = pBuffer;
	delete[] pBuffer;

	return true;
}

void SaveLocalHistory(const CString& peer, const CString& direction, const CString& content) {
	if (g_clientDb == nullptr) return;

	const char* sql = "INSERT INTO local_history (peer, direction, content, timestamp) VALUES (?, ?, ?, datetime('now', 'localtime'));";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(g_clientDb, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text16(stmt, 1, peer.GetString(), -1, SQLITE_STATIC);
		sqlite3_bind_text16(stmt, 2, direction.GetString(), -1, SQLITE_STATIC);
		sqlite3_bind_text16(stmt, 3, content.GetString(), -1, SQLITE_STATIC);
		sqlite3_step(stmt);
		sqlite3_finalize(stmt);
	}
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CClientDlg dialog



CClientDlg::CClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CLIENT_DIALOG, pParent)
	, m_strMsg(_T(""))
	, m_strCurrentPeer(_T(""))
	, m_bAutorun(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

CClientDlg::~CClientDlg()
{
	if (g_clientDb != nullptr) {
		sqlite3_close(g_clientDb);
		g_clientDb = nullptr;
	}
	if (g_clientSocket != INVALID_SOCKET) {
		closesocket(g_clientSocket);
		g_clientSocket = INVALID_SOCKET;
	}
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Control(pDX, IDC_LIST_MSG, m_listMsg);
	DDX_Check(pDX, IDC_CHK_AUTORUN, m_bAutorun);
	DDX_Control(pDX, IDC_TREE_USERS, m_treeUsers);
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CClientDlg::OnBnClickedBtnSend)
	ON_MESSAGE(WM_NET_STATUS, &CClientDlg::OnNetStatus)
	ON_MESSAGE(WM_NET_MESSAGE, &CClientDlg::OnNetMessage)
	ON_BN_CLICKED(IDC_CHK_AUTORUN, &CClientDlg::OnBnClickedChkAutorun)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_USERS, &CClientDlg::OnTvnSelchangedTreeUsers)
END_MESSAGE_MAP()


// CClientDlg message handlers

BOOL CClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	// init db
	CString dbPath = L"client_history_" + g_loggedInUser + L".db";
	if (sqlite3_open16(dbPath.GetString(), &g_clientDb) == SQLITE_OK) {
		const char* sql = "CREATE TABLE IF NOT EXISTS local_history ("
			"id INTEGER PRIMARY KEY AUTOINCREMENT,"
			"peer TEXT NOT NULL,"      
			"direction TEXT NOT NULL,"  // SENT/RECV
			"content TEXT NOT NULL,"
			"timestamp TEXT NOT NULL);";
		sqlite3_exec(g_clientDb, sql, nullptr, nullptr, nullptr);
	}

	m_listMsg.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listMsg.InsertColumn(0, L"Time", LVCFMT_LEFT, 75);
	m_listMsg.InsertColumn(1, L"From", LVCFMT_LEFT, 60);
	m_listMsg.InsertColumn(2, L"Message", LVCFMT_LEFT, 200);
	AdjustMessageColumnWidth();

	// Image list cho avatar/icon nguoi dung tren TreeView (+diem giao dien)
	m_treeImageList.Create(16, 16, ILC_COLOR32, 1, 1);
	m_treeImageList.Add(m_hIcon);
	m_treeUsers.SetImageList(&m_treeImageList, TVSIL_NORMAL);

	SetDlgItemText(IDC_STATIC_HELLO, L"Hello, " + g_loggedInUser);

	// Check if Autorun is on
	m_bAutorun = IsAutorunEnabled();
	UpdateData(FALSE); // Push status to CheckBox

	AfxBeginThread(ClientReceiveThread, this);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CClientDlg::OnNetStatus(WPARAM wParam, LPARAM lParam)
{
	return 0;
}

LRESULT CClientDlg::OnNetMessage(WPARAM wParam, LPARAM lParam)
{
	CString* pMsg = (CString*)lParam;

	CTime currentTime = CTime::GetCurrentTime();
	CString strTime = currentTime.Format(L"%H:%M:%S");

	CString cmd = pMsg->SpanExcluding(L"|");
	if (cmd == L"USER_LIST") {
		m_treeUsers.DeleteAllItems();
		HTREEITEM hRootOnline = m_treeUsers.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE, L"Online Users", 0, 0, 0, 0, 0, TVI_ROOT, TVI_LAST);
		HTREEITEM hRootOffline = m_treeUsers.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE, L"Offline Users", 0, 0, 0, 0, 0, TVI_ROOT, TVI_LAST);
		HTREEITEM hItemToReselect = NULL;

		bool bOnlineSection = true;
		int startPos = 10; // ignore USER_LIST|
		while (startPos < pMsg->GetLength()) {
			int endPos = pMsg->Find(L'|', startPos);
			CString token;
			if (endPos == -1) {
				token = pMsg->Mid(startPos);
				startPos = pMsg->GetLength();
			}
			else {
				token = pMsg->Mid(startPos, endPos - startPos);
				startPos = endPos + 1;
			}

			if (token == L"ONLINE") {
				bOnlineSection = true;
			}
			else if (token == L"OFFLINE") {
				bOnlineSection = false;
			}
			else if (!token.IsEmpty() && token != g_loggedInUser) { 
				HTREEITEM hParent = bOnlineSection ? hRootOnline : hRootOffline;
				HTREEITEM hItem = m_treeUsers.InsertItem(TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE, token, 0, 0, 0, 0, 0, hParent, TVI_LAST);
				if (!m_strCurrentPeer.IsEmpty() && token == m_strCurrentPeer) {
					hItemToReselect = hItem;
				}
			}
		}
		m_treeUsers.Expand(hRootOnline, TVE_EXPAND);
		m_treeUsers.Expand(hRootOffline, TVE_EXPAND);
		if (hItemToReselect != NULL) {
			m_treeUsers.SelectItem(hItemToReselect);
		}
	}
	// Check if message is forwarded from server
	else if (pMsg->Left(3) == L"MSG") {
		CString sender, content;
		AfxExtractSubString(sender, *pMsg, 1, L'|');
		AfxExtractSubString(content, *pMsg, 2, L'|');

		// 1. Luon luu vao lich su local database
		SaveLocalHistory(sender, L"RECV", content);

		// 2. Neu chua chon ai chat, tu dong chon nguoi vua nhan tin toi
		if (m_strCurrentPeer.IsEmpty()) {
			m_strCurrentPeer = sender;
			HTREEITEM hRoot = m_treeUsers.GetRootItem();
			if (hRoot) {
				HTREEITEM hChild = m_treeUsers.GetChildItem(hRoot);
				while (hChild) {
					if (m_treeUsers.GetItemText(hChild) == sender) {
						m_treeUsers.SelectItem(hChild);
						break;
					}
					hChild = m_treeUsers.GetNextSiblingItem(hChild);
				}
			}
		}

		// 3. Chi hien thi len ListView neu dang mo chat voi sender nay
		if (sender == m_strCurrentPeer) {
			int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
			m_listMsg.SetItemText(nIndex, 1, sender); 
			m_listMsg.SetItemText(nIndex, 2, content);
			m_listMsg.EnsureVisible(nIndex, FALSE);
			AdjustMessageColumnWidth();
		}
	}
	else if (*pMsg == L"CHAT_OK" || *pMsg == L"LOGIN_OK") {
		// Bo qua cac phan hoi he thong nay, khong in len ListView de tranh rac man hinh
	}
	else if (pMsg->Left(9) == L"CHAT_FAIL") {
		CString reason;
		AfxExtractSubString(reason, *pMsg, 1, L'|');
		AfxMessageBox(reason.IsEmpty() ? L"Cannot send message!" : reason);
	}
	else {
		// Cac thong bao he thong khac neu co
	}

	delete pMsg;
	return 0;
}

void CClientDlg::OnBnClickedBtnSend()
{
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty() || g_clientSocket == INVALID_SOCKET) return;

	HTREEITEM hSelected = m_treeUsers.GetSelectedItem();
	if (hSelected == NULL) {
		AfxMessageBox(L"Choose someone to chat!");
		return;
	}

	CString receiver = m_treeUsers.GetItemText(hSelected);
	if (receiver == L"Online Users" || receiver == L"Offline Users" || receiver.IsEmpty()) {
		AfxMessageBox(L"Choose specific person to chat!");
		return;
	}
	m_strCurrentPeer = receiver;

	// Pack and send
	CString cmdToSend = L"CHAT|" + receiver + L"|" + m_strMsg;

	if (SendMessageToSocket(g_clientSocket, cmdToSend)) {
		CTime currentTime = CTime::GetCurrentTime();
		CString strTime = currentTime.Format(L"%H:%M:%S");

		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
		m_listMsg.SetItemText(nIndex, 1, L"Me");
		m_listMsg.SetItemText(nIndex, 2, m_strMsg);
		m_listMsg.EnsureVisible(nIndex, FALSE);
		SaveLocalHistory(receiver, L"SENT", m_strMsg); // save chat 
		AdjustMessageColumnWidth(); 
	}

	m_strMsg = L"";
	UpdateData(FALSE);
}


void CClientDlg::OnBnClickedChkAutorun()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE); // Get latest status from CheckBox
	RegisterAutorun(m_bAutorun == TRUE); // Call write/del Registry func
}


void CClientDlg::OnTvnSelchangedTreeUsers(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);

	HTREEITEM hSelected = pNMTreeView ? pNMTreeView->itemNew.hItem : NULL;
	if (hSelected == NULL) {
		hSelected = m_treeUsers.GetSelectedItem();
	}
	if (hSelected == NULL) {
		*pResult = 0;
		return;
	}

	CString peer = m_treeUsers.GetItemText(hSelected);
	if (peer == L"Online Users" || peer == L"Offline Users" || peer.IsEmpty()) {
		*pResult = 0;
		return;
	}

	// Clear chat screen 
	m_listMsg.DeleteAllItems();
	m_strCurrentPeer = peer; 

	// Load chat history 
	if (g_clientDb == nullptr) return;

	const char* sql = "SELECT direction, content, timestamp FROM local_history WHERE peer = ? ORDER BY id ASC;";
	sqlite3_stmt* stmt;
	if (sqlite3_prepare_v2(g_clientDb, sql, -1, &stmt, nullptr) == SQLITE_OK) {
		sqlite3_bind_text16(stmt, 1, peer.GetString(), -1, SQLITE_STATIC);

		while (sqlite3_step(stmt) == SQLITE_ROW) {
			CString direction = (const wchar_t*)sqlite3_column_text16(stmt, 0);
			CString content = (const wchar_t*)sqlite3_column_text16(stmt, 1);
			CString time = (const wchar_t*)sqlite3_column_text16(stmt, 2);

			// Rut gon 'YYYY-MM-DD HH:MM:SS' thanh 'HH:MM:SS' neu co
			if (time.GetLength() >= 19) {
				time = time.Mid(11, 8);
			}

			int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), time);
			m_listMsg.SetItemText(nIndex, 1, (direction == L"SENT") ? L"Me" : peer);
			m_listMsg.SetItemText(nIndex, 2, content);
			m_listMsg.EnsureVisible(nIndex, FALSE);
		}
		sqlite3_finalize(stmt);
		AdjustMessageColumnWidth();
	}

	*pResult = 0;
}

void CClientDlg::AdjustMessageColumnWidth()
{
	m_listMsg.SetColumnWidth(2, LVSCW_AUTOSIZE);
	int nAutoWidth = m_listMsg.GetColumnWidth(2);

	CRect rectList;
	m_listMsg.GetClientRect(&rectList);
	int nRemainingWidth = rectList.Width() - m_listMsg.GetColumnWidth(0) - m_listMsg.GetColumnWidth(1) - 4;

	if (nAutoWidth < nRemainingWidth) {
		m_listMsg.SetColumnWidth(2, nRemainingWidth);
	}
}
