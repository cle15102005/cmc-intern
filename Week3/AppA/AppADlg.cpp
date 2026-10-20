
// AppADlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "AppA.h"
#include "AppADlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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


// CAppADlg dialog



CAppADlg::CAppADlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_APPA_DIALOG, pParent)
	, m_strMsg(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_hPipe = INVALID_HANDLE_VALUE;
}

void CAppADlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MSG, m_listMsg);
	DDX_Text(pDX, IDC_EDIT_MSG, m_strMsg);
}

BEGIN_MESSAGE_MAP(CAppADlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CAppADlg::OnBnClickedBtnSend)
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_BTN_SERVER, &CAppADlg::OnBnClickedBtnServer)
	ON_MESSAGE(WM_PIPE_RECV, &CAppADlg::OnPipeRecv)
END_MESSAGE_MAP()

// CAppADlg message handlers

BOOL CAppADlg::OnInitDialog()
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
	
	// App A list view
	m_listMsg.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_listMsg.InsertColumn(0, L"From", LVCFMT_LEFT, 100);
	m_listMsg.InsertColumn(1, L"Message", LVCFMT_LEFT, 300);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CAppADlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CAppADlg::OnPaint()
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
HCURSOR CAppADlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


BOOL CAppADlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct)
{
	if (pCopyDataStruct->dwData == 1)
	{
		const wchar_t* pWideStr = (const wchar_t*)pCopyDataStruct->lpData;
		CString strMsg(pWideStr);

		// Show message to ListView
		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), L"Peer");
		m_listMsg.SetItemText(nIndex, 1, strMsg);

		// Roll down to latest line
		m_listMsg.EnsureVisible(nIndex, FALSE);
	}
	return TRUE;
}

void CAppADlg::OnBnClickedBtnSend()
{
	UpdateData(TRUE);
	if (m_strMsg.IsEmpty()) return;

	// Send throught pipe if opening
	if (m_hPipe != INVALID_HANDLE_VALUE) {
		DWORD bytesWritten;
		WriteFile(m_hPipe, m_strMsg.GetString(),
			(m_strMsg.GetLength() + 1) * sizeof(wchar_t),
			&bytesWritten, NULL);

		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), L"Me (Pipe)");
		m_listMsg.SetItemText(nIndex, 1, m_strMsg);
	}
	else {
		// else send through wm_copydata
		HWND hTargetWnd = ::FindWindow(NULL, L"App B");
		if (hTargetWnd != NULL) {
			COPYDATASTRUCT cds;
			cds.dwData = 1;
			cds.cbData = (m_strMsg.GetLength() + 1) * sizeof(wchar_t);
			cds.lpData = (void*)m_strMsg.GetString();
			::SendMessage(hTargetWnd, WM_COPYDATA, (WPARAM)GetSafeHwnd(), (LPARAM)&cds);

			int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), L"Me");
			m_listMsg.SetItemText(nIndex, 1, m_strMsg);
		}
	}

	m_strMsg = L"";
	UpdateData(FALSE);
}

// Server thread
UINT __cdecl PipeServerThread(LPVOID pParam) {
	CAppADlg* pDlg = (CAppADlg*)pParam;
	HWND hWnd = pDlg->GetSafeHwnd();

	// Create named pipe
	pDlg->m_hPipe = CreateNamedPipe(
		L"\\\\.\\pipe\\ChatPipeA",       // Pipe name
		PIPE_ACCESS_DUPLEX,              // rw
		PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
		1, 4096, 4096, 0, NULL);

	if (pDlg->m_hPipe == INVALID_HANDLE_VALUE) return 1;

	// Wait for client to connect
	if (ConnectNamedPipe(pDlg->m_hPipe, NULL)) {
		::PostMessage(hWnd, WM_PIPE_RECV, 1, 0);

		// Repeat reading data
		wchar_t buffer[1024];
		DWORD bytesRead;
		while (true) {
			BOOL success = ReadFile(pDlg->m_hPipe, buffer, sizeof(buffer) - sizeof(wchar_t), &bytesRead, NULL);
			if (!success || bytesRead == 0) break; // Disconenct

			buffer[bytesRead / sizeof(wchar_t)] = L'\0';
			CString* pStr = new CString(buffer);
			::PostMessage(hWnd, WM_PIPE_RECV, 2, (LPARAM)pStr); // wParam = 2: new message
		}
	}
	return 0;
}

LRESULT CAppADlg::OnPipeRecv(WPARAM wParam, LPARAM lParam)
{
	if (wParam == 1) {
		AfxMessageBox(L"Đã có client kết nối tới server!");
	}
	else if (wParam == 2) {
		CString* pMsg = (CString*)lParam;
		int nIndex = m_listMsg.InsertItem(m_listMsg.GetItemCount(), L"Peer (Pipe)");
		m_listMsg.SetItemText(nIndex, 1, *pMsg);
		m_listMsg.EnsureVisible(nIndex, FALSE);
		delete pMsg; // Delete memory after use
	}
	return 0;
}

void CAppADlg::OnBnClickedBtnServer()
{
	GetDlgItem(IDC_BTN_SERVER)->EnableWindow(FALSE); // Lock the button
	AfxBeginThread(PipeServerThread, this);          // Turn on server thread
}
