
// Week2Dlg.cpp : implementation file
//

#include "pch.h"
#include "framework.h"
#include "Week2.h"
#include "Week2Dlg.h"
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


// CWeek2Dlg dialog



CWeek2Dlg::CWeek2Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CHAT_DIALOG, pParent)
	, m_strLoggedUser(_T(""))
	, m_strMessage(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWeek2Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TREE_CONTACTS, m_treeContacts);
	DDX_Control(pDX, IDC_LIST_MESSAGES, m_listMessages);
	DDX_Text(pDX, IDC_EDIT_MESSAGE, m_strMessage);
}

BEGIN_MESSAGE_MAP(CWeek2Dlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_SEND, &CWeek2Dlg::OnBnClickedBtnSend)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE_CONTACTS, &CWeek2Dlg::OnTvnSelchangedTreeContacts)
END_MESSAGE_MAP()


// CWeek2Dlg message handlers

BOOL CWeek2Dlg::OnInitDialog()
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
	
	// Image list: Avatars
	m_imageList.Create(16, 16, ILC_COLOR32 | ILC_MASK, 2, 2);
	m_imageList.Add(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION)); // Online icon 
	m_imageList.Add(AfxGetApp()->LoadStandardIcon(IDI_QUESTION));    // Offline icon 
	m_treeContacts.SetImageList(&m_imageList, TVSIL_NORMAL);

	// Read contacts from users.txt
	m_nContactCount = 0;
	HANDLE hUserFile = CreateFile(L"users.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hUserFile != INVALID_HANDLE_VALUE) {
		DWORD dwSize = GetFileSize(hUserFile, NULL);
		if (dwSize > 0) {
			BYTE* pBuf = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwSize + sizeof(wchar_t));
			if (pBuf) {
				DWORD dwRead = 0;
				if (ReadFile(hUserFile, pBuf, dwSize, &dwRead, NULL)) {
					CString strContent = (wchar_t*)pBuf;
					int nPos = 0;
					CString strLine = strContent.Tokenize(L"", nPos);
					while (!strLine.IsEmpty() && m_nContactCount < 100) {
						int nComma = strLine.Find(L',');
						if (nComma != -1) {
							CString strUser = strLine.Left(nComma);
							if (strUser != m_strLoggedUser) { // Don't show yourself in contacts
								m_arrContacts[m_nContactCount].strName = strUser;
								m_arrContacts[m_nContactCount].strStatus = (m_nContactCount % 2 == 0) ? L"Online" : L"Offline";
								m_arrContacts[m_nContactCount].strLastSeen = L"Just now";
								m_nContactCount++;
							}
						}
						strLine = strContent.Tokenize(L"", nPos);
					}
				}
				HeapFree(GetProcessHeap(), 0, pBuf);
			}
		}
		CloseHandle(hUserFile);
	}

	// Tree view: Contacts 
	// Populate the dynamic contacts tree
	PopulateDummyData(m_treeContacts, m_arrContacts, m_nContactCount);


	// List view: Chat box
	m_listMessages.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	m_listMessages.InsertColumn(0, L"Time", LVCFMT_LEFT, 70);
	m_listMessages.InsertColumn(1, L"Receiver", LVCFMT_LEFT, 150);
	m_listMessages.InsertColumn(2, L"Sender", LVCFMT_RIGHT, 150);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CWeek2Dlg::PopulateDummyData(CTreeCtrl& treeContacts, Contact* pContacts, int nContacts)
{	
	treeContacts.DeleteAllItems();
	
	HTREEITEM hRoot = treeContacts.InsertItem(L"Contacts", 0, 0, TVI_ROOT);
	HTREEITEM hOnline = treeContacts.InsertItem(L"Online", 0, 0, hRoot);
	HTREEITEM hOffline = treeContacts.InsertItem(L"Offline", 1, 1, hRoot);

	for (int i = 0; i < nContacts; i++) {
		HTREEITEM hNode;
		if (pContacts[i].strStatus == L"Online") {
			hNode = treeContacts.InsertItem(pContacts[i].strName, 0, 0, hOnline, TVI_LAST);
		}
		else {
			hNode = treeContacts.InsertItem(pContacts[i].strName, 1, 1, hOffline, TVI_LAST);
		}
		treeContacts.SetItemData(hNode, (DWORD_PTR)i);
	}
	
	treeContacts.Expand(hRoot, TVE_EXPAND);
	treeContacts.Expand(hOnline, TVE_EXPAND);
	treeContacts.Expand(hOffline, TVE_EXPAND);
}

void CWeek2Dlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CWeek2Dlg::OnPaint()
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
HCURSOR CWeek2Dlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CWeek2Dlg::OnBnClickedBtnSend()
{
	// Read data from edit box to m_strMessage
	UpdateData(TRUE);
	if (m_strMessage.IsEmpty() || m_strMessage.Trim().IsEmpty()) {
		AfxMessageBox(L"Type something meaningful before press the Send button!");
		return;
	}
	
	int nIndex = m_listMessages.GetItemCount();
	CTime time = CTime::GetCurrentTime();
	CString strTime = time.Format(L"%H:%M");
	
	m_listMessages.InsertItem(nIndex, strTime);         
	m_listMessages.SetItemText(nIndex, 1, L"");         
	m_listMessages.SetItemText(nIndex, 2, m_strMessage); 
	
	HTREEITEM hSelected = m_treeContacts.GetSelectedItem();
	if (hSelected != NULL && !m_treeContacts.ItemHasChildren(hSelected)) {
		int index = (int)m_treeContacts.GetItemData(hSelected);
		CString strContactName = m_arrContacts[index].strName;
		SaveChatMessage(strContactName, strTime, m_strMessage, true);
	}
	
	m_strMessage = L"";
	UpdateData(FALSE);
	m_listMessages.EnsureVisible(m_listMessages.GetItemCount() - 1, FALSE);
}


void CWeek2Dlg::OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMTREEVIEW pNMTreeView = reinterpret_cast<LPNMTREEVIEW>(pNMHDR);
	
	HTREEITEM hSelected = pNMTreeView->itemNew.hItem;

	if (hSelected != NULL) {
		// No updating if the user clicked title Contacts or Online
		if (!m_treeContacts.ItemHasChildren(hSelected)) {
			// If user
			int index = (int)m_treeContacts.GetItemData(hSelected);
			CString strName = m_arrContacts[index].strName;
			
			CString strUI;
			strUI.Format(L"Chat with: %s (%s, %s)", strName.GetString(), m_arrContacts[index].strStatus.GetString(), m_arrContacts[index].strLastSeen.GetString());
			SetDlgItemText(IDC_STATIC_CURRENT_CHAT, strUI);
			
			LoadChatHistory(strName);
		}
	}

	*pResult = 0;
}


void CWeek2Dlg::LoadChatHistory(CString strContact)
{
	m_listMessages.DeleteAllItems();

	CString strFileName;
	strFileName.Format(L"history_%s.txt", strContact.GetString());

	HANDLE hFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;

	DWORD dwFileSize = GetFileSize(hFile, NULL);
	if (dwFileSize > 0) {
		BYTE* pBuffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize + sizeof(wchar_t));
		if (pBuffer) {
			DWORD dwBytesRead = 0;
			if (ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL)) {
				CString strContent = (wchar_t*)pBuffer;
				int nPos = 0;
				CString strLine = strContent.Tokenize(L"\r\n", nPos);
				while (!strLine.IsEmpty()) {
					int nFirst = strLine.Find(L"|");
					if (nFirst != -1) {
						int nSecond = strLine.Find(L"|", nFirst + 1);
						if (nSecond != -1) {
							CString strTime = strLine.Left(nFirst);
							CString strRecv = strLine.Mid(nFirst + 1, nSecond - nFirst - 1);
							CString strSend = strLine.Mid(nSecond + 1);

							int nIndex = m_listMessages.GetItemCount();
							m_listMessages.InsertItem(nIndex, strTime); 
							m_listMessages.SetItemText(nIndex, 1, strRecv);
							m_listMessages.SetItemText(nIndex, 2, strSend);
						}
					}
					strLine = strContent.Tokenize(L"\r\n", nPos);
				}
			}
			HeapFree(GetProcessHeap(), 0, pBuffer);
		}
	}
	CloseHandle(hFile);
}

void CWeek2Dlg::SaveChatMessage(CString strContact, CString strTime, CString strMessage, bool bIsMe)
{
	CString strFileName;
	strFileName.Format(L"history_%s.txt", strContact.GetString());

	HANDLE hFile = CreateFile(strFileName, FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return;

	CString strLine;
	if (bIsMe) {
		strLine.Format(L"%s||%s\r\n", strTime.GetString(), strMessage.GetString());
	} else {
		strLine.Format(L"%s|%s|\r\n", strTime.GetString(), strMessage.GetString());
	}

	DWORD dwBytesToWrite = strLine.GetLength() * sizeof(wchar_t);
	DWORD dwBytesWritten = 0;
	WriteFile(hFile, strLine.GetString(), dwBytesToWrite, &dwBytesWritten, NULL);

	CloseHandle(hFile);
}
