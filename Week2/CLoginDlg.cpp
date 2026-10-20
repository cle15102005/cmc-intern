// CLoginDlg.cpp : implementation file
//

#include "pch.h"
#include "Week2.h"
#include "CLoginDlg.h"
#include "afxdialogex.h"
#include "CRegisterDlg.h"

// CLoginDlg dialog

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DIALOG, pParent)
	, m_strUsername(_T(""))
	, m_strPassword(_T(""))
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUsername);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBnClickedBtnLogin)
	ON_BN_CLICKED(IDC_BTN_CANCEL, &CLoginDlg::OnBnClickedBtnCancel)
	ON_BN_CLICKED(IDC_BTN_REGISTER, &CLoginDlg::OnBnClickedBtnRegister)
END_MESSAGE_MAP()


// CLoginDlg message handlers


void CLoginDlg::OnBnClickedBtnLogin()
{
	UpdateData(TRUE);
	if (m_strUsername.IsEmpty() || m_strPassword.IsEmpty()) {
		AfxMessageBox(L"Username and Password cannot be empty.");
		return;	
	}

	// Read from users.txt (Week 1 style)
	HANDLE hFile = INVALID_HANDLE_VALUE;
	BYTE* pBuffer = NULL;
	DWORD dwFileSize = 0;
	DWORD dwBytesRead = 0;
	CString strFileContent;
	bool bFound = false;

	hFile = CreateFile(L"users.txt", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		AfxMessageBox(L"No users registered yet.");
		goto cleanup;
	}

	dwFileSize = GetFileSize(hFile, NULL);
	pBuffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, dwFileSize + sizeof(wchar_t));
	if (pBuffer == NULL) goto cleanup;

	if (ReadFile(hFile, pBuffer, dwFileSize, &dwBytesRead, NULL)) {
		strFileContent = (wchar_t*)pBuffer;
		
		// Parse line by line
		int nPos = 0;
		CString strLine = strFileContent.Tokenize(L"\r\n", nPos);
		while (!strLine.IsEmpty()) {
			int nCommaPos = strLine.Find(L',');
			if (nCommaPos != -1) {
				CString strUser = strLine.Left(nCommaPos);
				CString strPass = strLine.Mid(nCommaPos + 1);
				if (strUser == m_strUsername && strPass == m_strPassword) {
					bFound = true;
					break;
				}
			}
			strLine = strFileContent.Tokenize(L"\r\n", nPos);
		}
	}

	if (bFound) {
		CDialogEx::OnOK();
	} else {
		AfxMessageBox(L"Invalid username or password.");
	}

cleanup:
	if (pBuffer != NULL) HeapFree(GetProcessHeap(), 0, pBuffer);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}


void CLoginDlg::OnBnClickedBtnCancel()
{
	// CLose the Cancel dialog with IDCANCEL
	CDialogEx::OnCancel();
}


void CLoginDlg::OnBnClickedBtnRegister()
{
	// Model dialog: Register
	CRegisterDlg registerDlg;
	INT_PTR nRegisterResponse = registerDlg.DoModal();
	if (nRegisterResponse == IDOK) {
		CString strUsername = registerDlg.GetUsername();
		m_strUsername = strUsername;

		// Upadte data from register variable to login dialog
		UpdateData(FALSE);
	}
	else if (nRegisterResponse == IDCANCEL) {
		// Do nothing else
	}
}
