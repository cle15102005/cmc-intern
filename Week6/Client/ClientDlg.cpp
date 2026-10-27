
// ClientDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Client.h"
#include "ClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

UINT __cdecl ClientConnectThread(LPVOID pParam) {
	CClientDlg* pDlg = (CClientDlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();

	SOCKET clientSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSock == INVALID_SOCKET) return 1;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

	if (connect(clientSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(clientSock);
		::PostMessage(hWnd, WM_NET_STATUS, 4, 0); // Connection failed
		return 1;
	}

	// If success
	pDlg->m_connectedSock = clientSock;
	::PostMessage(hWnd, WM_NET_STATUS, 2, 0); // Connetion successfully

	// Message loop
	CString recvMsg;
	while (RecvMessageFromSocket(pDlg->m_connectedSock, recvMsg)) {
		CString* pStr = new CString(recvMsg);
		::PostMessage(hWnd, WM_NET_MESSAGE, 0, (LPARAM)pStr); // Send message to UI
	}

	// End loop
	::PostMessage(hWnd, WM_NET_STATUS, 3, 0); // Disconnect
	closesocket(pDlg->m_connectedSock);
	pDlg->m_connectedSock = INVALID_SOCKET;

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


// CClientDlg dialog



CClientDlg::CClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CLIENT_DIALOG, pParent)
	, m_strMsg(_T(""))
	, m_bAutorun(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
	DDX_Control(pDX, IDC_LIST_MSG, m_listMsg);
	DDX_Check(pDX, IDC_CHK_AUTORUN, m_bAutorun);
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_CONNECT, &CClientDlg::OnBnClickedBtnConnect)
	ON_BN_CLICKED(IDC_BTN_SEND, &CClientDlg::OnBnClickedBtnSend)
	ON_MESSAGE(WM_NET_STATUS, &CClientDlg::OnNetStatus)
	ON_MESSAGE(WM_NET_MESSAGE, &CClientDlg::OnNetMessage)
	ON_BN_CLICKED(IDC_CHK_AUTORUN, &CClientDlg::OnBnClickedChkAutorun)
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
	m_listMsg.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listMsg.InsertColumn(0, L"Time", LVCFMT_LEFT, 80);
	m_listMsg.InsertColumn(1, L"From", LVCFMT_LEFT, 80);
	m_listMsg.InsertColumn(2, L"Message", LVCFMT_LEFT, 250);

	// Check if Autorun is on
	m_bAutorun = IsAutorunEnabled();
	UpdateData(FALSE); // Push status to CheckBox

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
	if (wParam == 2) {
		AfxMessageBox(L"Connected successfully to the server!");
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(FALSE); // Lock connect button once connected
	}
	else if (wParam == 3) {
		AfxMessageBox(L"Server disconnected!");
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE); // Unlock connect button if failed
	}
	else if (wParam == 4) {
		AfxMessageBox(L"Connection failed! Server is not on or wrong port.");
		GetDlgItem(IDC_BTN_CONNECT)->EnableWindow(TRUE); // Unlock connect button to re-try
	}
	return 0;
}

LRESULT CClientDlg::OnNetMessage(WPARAM wParam, LPARAM lParam)
{
	CString* pMsg = (CString*)lParam;

	CTime currentTime = CTime::GetCurrentTime();
	CString strTime = currentTime.Format(L"%H:%M:%S");

	// Check if message is forwarded from server
	if (pMsg->Left(3) == L"MSG") {
		CString sender, content;
		AfxExtractSubString(sender, *pMsg, 1, L'|');
		AfxExtractSubString(content, *pMsg, 2, L'|');

		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
		m_listMsg.SetItemText(nIndex, 1, sender); 
		m_listMsg.SetItemText(nIndex, 2, content);
		m_listMsg.EnsureVisible(nIndex, FALSE);
	}
	else {
		// System response
		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
		m_listMsg.SetItemText(nIndex, 1, L"System");
		m_listMsg.SetItemText(nIndex, 2, *pMsg);
		m_listMsg.EnsureVisible(nIndex, FALSE);
	}

	delete pMsg;
	return 0;
}

void CClientDlg::OnBnClickedBtnConnect()
{
	AfxBeginThread(ClientConnectThread, this);
}


void CClientDlg::OnBnClickedBtnSend()
{
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty() || m_connectedSock == INVALID_SOCKET) return;

	CString cmdToSend;

	// Check user
	if (m_strMsg.Left(5) == L"LOGIN" || m_strMsg.Left(3) == L"REG") {
		cmdToSend = m_strMsg; // 
	}
	else {
		// if chat
		CString receiver = L"admin";
		cmdToSend = L"CHAT|" + receiver + L"|" + m_strMsg;
	}

	// send
	if (SendMessageToSocket(m_connectedSock, cmdToSend)) {
		CTime currentTime = CTime::GetCurrentTime();
		CString strTime = currentTime.Format(L"%H:%M:%S");

		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), strTime);
		m_listMsg.SetItemText(nIndex, 1, L"Me");
		m_listMsg.SetItemText(nIndex, 2, m_strMsg);
		m_listMsg.EnsureVisible(nIndex, FALSE);
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
