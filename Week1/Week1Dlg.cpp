
// Week1Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Week1.h"
#include "Week1Dlg.h"
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


// CWeek1Dlg dialog



CWeek1Dlg::CWeek1Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_WEEK1_DIALOG, pParent)
	, m_strContent(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWeek1Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TEXT, m_strContent);
}

BEGIN_MESSAGE_MAP(CWeek1Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_BROWSE, &CWeek1Dlg::OnBnClickedBtnBrowse)
	ON_BN_CLICKED(IDC_BTN_SAVE, &CWeek1Dlg::OnBnClickedBtnSave)
	ON_BN_CLICKED(IDC_BTN_LOAD, &CWeek1Dlg::OnBnClickedBtnLoad)
	ON_EN_CHANGE(IDC_EDIT_TEXT, &CWeek1Dlg::OnEnChangeEditText)
END_MESSAGE_MAP()


// CWeek1Dlg message handlers

BOOL CWeek1Dlg::OnInitDialog()
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

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWeek1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWeek1Dlg::OnPaint()
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
HCURSOR CWeek1Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWeek1Dlg::OnEnChangeEdit1()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}


void CWeek1Dlg::OnBnClickedBtnBrowse()
{
	CFileDialog dlg(
		FALSE,				                 // FALSE = Save dialog
		L"txt",				                 // default extension
		L"output",                           // default filename
		OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST,
		L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"
	);

	if (dlg.DoModal() == IDOK)
	{
		// Load path name
		m_strFolderPath = dlg.GetPathName();

		// Update the static label without going through DDX
		SetDlgItemText(IDC_STATIC_SAVE_PATH, m_strFolderPath);
	}
}


void CWeek1Dlg::OnBnClickedBtnSave()
{
	// Get content from edit box to m_strContent;
	UpdateData(TRUE);

	// File handler
	HANDLE hfile = CreateFile(
		m_strFolderPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
	);

	if (hfile == INVALID_HANDLE_VALUE)
	{
		MessageBox(L"Failed to open file");
	}
	else {
		// NOTE: no need to use WideCharToMultiByte, we can directly access the m_strContent
		//	     to avoid data loss from UTF_16 to UTF_8
		DWORD dwBytesToWrite = (DWORD)(m_strContent.GetLength()*sizeof(wchar_t)); 
		DWORD dwBytesWritten = 0;
		BOOL bOK = WriteFile(hfile, m_strContent.GetString(), dwBytesToWrite, &dwBytesWritten, NULL);

		if (!bOK || dwBytesWritten != dwBytesToWrite)
			AfxMessageBox(L"Write failed!");
		else
			AfxMessageBox(L"Saved!");

		CloseHandle(hfile);
	}
}


void CWeek1Dlg::OnBnClickedBtnLoad()
{
	// Declare ALL variables first with safe initial values
	// so goto can never skip an initialization
	HANDLE hfile = INVALID_HANDLE_VALUE;
	BYTE* pBuffer = NULL;
	DWORD  dwBytesToRead = 0;
	DWORD  dwBytesRead = 0;
	BOOL   bOK = FALSE;

	// File handler
	hfile = CreateFile(m_strFolderPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile == INVALID_HANDLE_VALUE)
	{
		AfxMessageBox(L"File does not exist or failed to open. \n Please browse the file path first!!");
		goto cleanup;
	}

	// Get file size
	dwBytesToRead = GetFileSize(hfile, NULL);
	if (dwBytesToRead == 0 || dwBytesToRead == INVALID_FILE_SIZE) {
		AfxMessageBox(L"File is empty or invalid!");
		goto cleanup;
	}

	// NOTE: Use HeapAlloc instead of allocate a vector
	pBuffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwBytesToRead + sizeof(wchar_t));
	if (pBuffer == NULL)
	{
		AfxMessageBox(L"Out of memory!");
		goto cleanup;
	}

	// Read file into pBuffer
	// NOTE: alway declare returned value for safety
	bOK = ReadFile(hfile, pBuffer, dwBytesToRead, &dwBytesRead, NULL);		
	if (!bOK) {
		AfxMessageBox(L"Read failed!");
		goto cleanup;
	}
	

	// NOTE: no need to use MultiByteToWideChar, we can directly access the m_strContent
	//		 to avoid data loss from UTF_16 to UTF_8
	m_strContent = (wchar_t*)pBuffer;

	// Update content from m_strContent to edit box;
	UpdateData(FALSE);

	//free allocated heap and file handle
	cleanup:
		if (pBuffer != NULL) HeapFree(GetProcessHeap(), 0, pBuffer);
		if (hfile != INVALID_HANDLE_VALUE) CloseHandle(hfile);
}


void CWeek1Dlg::OnEnChangeEditText()
{
	// TODO:  If this is a RICHEDIT control, the control will not
	// send this notification unless you override the CDialogEx::OnInitDialog()
	// function and call CRichEditCtrl().SetEventMask()
	// with the ENM_CHANGE flag ORed into the mask.

	// TODO:  Add your control notification handler code here
}
