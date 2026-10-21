
// ServerDlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Server.h"
#include "ServerDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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

	// Send first 4-bytes length
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

UINT __cdecl ServerAcceptThread(LPVOID pParam) {
	CServerDlg* pDlg = (CServerDlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();

	SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listenSock == INVALID_SOCKET) return 1;

	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(9999);
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listenSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
		closesocket(listenSock);
		return 1;
	}

	listen(listenSock, SOMAXCONN);
	::PostMessage(hWnd, WM_NET_STATUS, 1, 0); // Wait for client noti

	SOCKET clientSock = accept(listenSock, NULL, NULL);
	if (clientSock != INVALID_SOCKET) {
		pDlg->m_connectedSock = clientSock; // Save socket to dialog
		::PostMessage(hWnd, WM_NET_STATUS, 2, 0); // Client connected noti

		// Receive message loop
		CString recvMsg;
		while (RecvMessageFromSocket(pDlg->m_connectedSock, recvMsg)) {
			CString* pStr = new CString(recvMsg);
			// Send message to UI to show
			::PostMessage(hWnd, WM_NET_MESSAGE, 0, (LPARAM)pStr);
		}

		::PostMessage(hWnd, WM_NET_STATUS, 3, 0); // Client disconnect noty
		closesocket(pDlg->m_connectedSock);
		pDlg->m_connectedSock = INVALID_SOCKET;
	}

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
