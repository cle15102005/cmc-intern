// CRegisterDlg.cpp : implementation file
//

#include "pch.h"
#include "Week2.h"
#include "CRegisterDlg.h"
#include "afxdialogex.h"


// CRegisterDlg dialog

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REGISTER_DIALOG, pParent)
	, m_strUsername(_T(""))
	, m_strPassword(_T(""))
	, m_strConfirmPassword(_T(""))
{

}

CRegisterDlg::~CRegisterDlg()
{
}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUsername);
	DDX_Text(pDX, IDC_EDIT_PASSWORD, m_strPassword);
	DDX_Text(pDX, IDC_EDIT_CONFIRM_PASSWORD, m_strConfirmPassword);
}


BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_REGISTER, &CRegisterDlg::OnBnClickedBtnRegister)
	ON_BN_CLICKED(IDC_BTN_CANCEL, &CRegisterDlg::OnBnClickedBtnCancel)
END_MESSAGE_MAP()


// CRegisterDlg message handlers


void CRegisterDlg::OnBnClickedBtnRegister()
{
	UpdateData(TRUE);

	if (m_strUsername.IsEmpty()) 
	{ 
		AfxMessageBox(L"Username must not be empty!"); 
		return; 
	}
	else if (m_strPassword.GetLength() < 6) 
	{ 
		AfxMessageBox(L"Password must have at least 6 characters"); 
		return; 
	}
	else if (m_strPassword.Compare(m_strConfirmPassword) != 0) 
	{ 
		AfxMessageBox(L"Confirm password does not match password"); 
		return; 
	}
	
	// Write to users.txt (Week 1 style)
	HANDLE hFile = INVALID_HANDLE_VALUE;
	CString strOutput;
	DWORD dwBytesToWrite = 0;
	DWORD dwBytesWritten = 0;

	hFile = CreateFile(L"users.txt", FILE_APPEND_DATA, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		AfxMessageBox(L"Failed to open users.txt");
		goto cleanup;
	}

	strOutput.Format(L"%s,%s\r\n", m_strUsername.GetString(), m_strPassword.GetString());
	dwBytesToWrite = strOutput.GetLength() * sizeof(wchar_t);

	if (!WriteFile(hFile, strOutput.GetString(), dwBytesToWrite, &dwBytesWritten, NULL) || dwBytesWritten != dwBytesToWrite) {
		AfxMessageBox(L"Failed to write to users.txt");
	}
	else {
		AfxMessageBox(L"Register successfully!");
		CDialogEx::OnOK();
	}

cleanup:
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
}


void CRegisterDlg::OnBnClickedBtnCancel()
{
	CDialogEx::OnCancel();
}
