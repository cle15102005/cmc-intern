
// ServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"
#include "Database.h" 
#include <map>
#include <mutex>

std::map<CString, SOCKET> g_onlineUsers; //Online list
std::mutex g_onlineMutex; //Mutex

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct ClientThreadParam {
	SOCKET sock;
	CServerDlg* pDlg;
};

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
	uint32_t netLen = htonl(len);

	// Send 4-bytes length
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

// Handle Thread per Client
UINT __cdecl ClientHandlerThread(LPVOID pParam) {
	ClientThreadParam* pData = (ClientThreadParam*)pParam;
	SOCKET clientSock = pData->sock;     // Get socket of the client
	CServerDlg* pDlg = pData->pDlg;
	HWND hWnd = pDlg->GetSafeHwnd();
	delete pData;

	CString loggedInUser = L""; 

	// Receive message loop
	CString recvMsg;
	while (RecvMessageFromSocket(clientSock, recvMsg)) {
		// Extract command from message
		CString cmd = recvMsg.SpanExcluding(L"|");
		CString response = L"";

		if (cmd == L"REG") {
			CString user, pass;
			AfxExtractSubString(user, recvMsg, 1, L'|');
			AfxExtractSubString(pass, recvMsg, 2, L'|');

			if (RegisterUser(user, pass)) {
				response = L"REG_OK";
			}
			else {
				response = L"REG_FAIL|Username exists!";
			}
		}
		else if (cmd == L"LOGIN") {
			CString user, pass;
			AfxExtractSubString(user, recvMsg, 1, L'|');
			AfxExtractSubString(pass, recvMsg, 2, L'|');

			if (LoginUser(user, pass)) {
				loggedInUser = user; // Save successful logged in user

				// Save user to online list
				g_onlineMutex.lock();
				g_onlineUsers[user] = clientSock;
				g_onlineMutex.unlock();

				response = L"LOGIN_OK";
			}
			else {
				response = L"LOGIN_FAIL|Wrong username or password!";
			}
		}
		else if (cmd == L"CHAT") {
			CString receiver, content;
			AfxExtractSubString(receiver, recvMsg, 1, L'|');
			AfxExtractSubString(content, recvMsg, 2, L'|');

			if (loggedInUser.IsEmpty()) {
				response = L"CHAT_FAIL|You must login first!";
			}
			else {
				SaveMessage(loggedInUser, receiver, content);

				SOCKET recipientSock = INVALID_SOCKET;
				g_onlineMutex.lock();
				auto it = g_onlineUsers.find(receiver);
				if (it != g_onlineUsers.end()) {
					recipientSock = it->second;
				}
				g_onlineMutex.unlock();

				// Forward message if user is online
				if (recipientSock != INVALID_SOCKET) {
					CString fwdMsg = L"MSG|" + loggedInUser + L"|" + content;
					SendMessageToSocket(recipientSock, fwdMsg);
					response = L"CHAT_OK";
				}
				else {
					response = L"CHAT_FAIL|User is offline!";
				}
			}
		}
		else {
			response = L"ERROR|Invalid command";
		}

		// Send response to client
		if (!response.IsEmpty()) {
			SendMessageToSocket(clientSock, response); 
		}

		CString* pStr = new CString(recvMsg);
		::PostMessage(hWnd, WM_NET_MESSAGE, 0, (LPARAM)pStr);
	}

	// Delete user from online list, after disconnect
	if (!loggedInUser.IsEmpty()) {
		g_onlineMutex.lock();
		g_onlineUsers.erase(loggedInUser);
		g_onlineMutex.unlock();
	}

	closesocket(clientSock);
	return 0;
}

UINT __cdecl ServerAcceptThread(LPVOID pParam) {
	CServerDlg* pDlg = (CServerDlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();

	// Create socket
	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) return 1;

	// Bind socket to port 9999
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(listenSock);
		return 1;
	}

	// Start listening for incoming connections
	listen(listenSock, SOMAXCONN);
	::PostMessage(hWnd, WM_NET_STATUS, 1, 0); // Wait for client noty

	// Infinite loop to accept multiple clients simultaneously
	while (true) {
		SOCKET clientSock = accept(listenSock, NULL, NULL);

		// If accept fails, it means the listening socket was closed
		if (clientSock == INVALID_SOCKET) {
			break;
		}

		// Create a parameter struct to pass data to the new thread
		ClientThreadParam* pData = new ClientThreadParam();
		pData->sock = clientSock;
		pData->pDlg = pDlg;

		// Spawn a dedicated thread to handle this specific client
		AfxBeginThread(ClientHandlerThread, pData);

		// Connection noti
		::PostMessage(hWnd, WM_NET_STATUS, 2, 0);
	}

	// Cleanup
	closesocket(listenSock);
	return 0;
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


// CServerDlg dialog



CServerDlg::CServerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SERVER_DIALOG, pParent)
	, m_strMsg(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Control(pDX, IDC_LIST_MSG, m_listMsg);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_START_SERVER, &CServerDlg::OnBnClickedBtnStartServer)
	ON_MESSAGE(WM_NET_STATUS, &CServerDlg::OnNetStatus)
	ON_MESSAGE(WM_NET_MESSAGE, &CServerDlg::OnNetMessage)
	ON_BN_CLICKED(IDC_BTN_SEND, &CServerDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()


// CServerDlg message handlers

BOOL CServerDlg::OnInitDialog()
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
	m_listMsg.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listMsg.InsertColumn(0, L"Time", LVCFMT_LEFT, 80);
	m_listMsg.InsertColumn(1, L"From", LVCFMT_LEFT, 80);
	m_listMsg.InsertColumn(2, L"Message", LVCFMT_LEFT, 250);

	// Test database
	if (InitDatabase()) {
		// Register an account
		//if (RegisterUser(L"admin", L"123456")) {
		//	AfxMessageBox(L"Dummy admin success!");
		//}
		//else {
		//	AfxMessageBox(L"Register failed (admin may existed)!");
		//}

		// Login
		//if (LoginUser(L"admin", L"123456")) {
		//	AfxMessageBox(L"Logged in!");
		//}
		//else {
		//	AfxMessageBox(L"Logged failed!");
		//}
	}
	else {
		AfxMessageBox(L"Cannot initialize database!");
	}

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CServerDlg::OnPaint()
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
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


LRESULT CServerDlg::OnNetStatus(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 1) AfxMessageBox(L"Server is running. Waiting for client...");
	else if (wParam == 2) AfxMessageBox(L"Client connected!");
	else if (wParam == 3) AfxMessageBox(L"Client disconnected!");
	return 0;
}

LRESULT CServerDlg::OnNetMessage(WPARAM wParam, LPARAM lParam)
{
	CString* pMsg = (CString*)lParam;

	// Realtime
	CTime currentTime = CTime::GetCurrentTime();
	CString strTime = currentTime.Format(L"%H:%M:%S");

	// Add to ListView
	int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
	m_listMsg.SetItemText(nIndex, 1, L"Peer"); // Sender is Peer
	m_listMsg.SetItemText(nIndex, 2, *pMsg);

	// Roll to latest  
	m_listMsg.EnsureVisible(nIndex, FALSE);

	delete pMsg; // Clean up memory
	return 0;
}

void CServerDlg::OnBnClickedBtnStartServer()
{
	GetDlgItem(IDC_BTN_START_SERVER)->EnableWindow(FALSE); // Lock button
	AfxBeginThread(ServerAcceptThread, this);     // Turn on the thread
}

void CServerDlg::OnBnClickedBtnSend()
{
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty() || m_connectedSock == INVALID_SOCKET) return;
	if (SendMessageToSocket(m_connectedSock, m_strMsg)) {

		// Show on list view
		CTime currentTime = CTime::GetCurrentTime();
		CString strTime = currentTime.Format(L"%H:%M:%S");

		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
		m_listMsg.SetItemText(nIndex, 1, L"Me");
		m_listMsg.SetItemText(nIndex, 2, m_strMsg);
		m_listMsg.EnsureVisible(nIndex, FALSE);
	}

	// Clear edit box
	m_strMsg = L"";
	UpdateData(FALSE);
}
