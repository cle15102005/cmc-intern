#pragma once


// CRegisterDlg dialog

class CRegisterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegisterDlg)

public:
	CRegisterDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRegisterDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REGISTER_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnRegister();
	afx_msg void OnBnClickedBtnCancel();

	// Get user name from register dialog
	CString GetUsername() const { return m_strUsername; }

private:
	CString m_strUsername;
	CString m_strPassword;
	CString m_strConfirmPassword;
};
