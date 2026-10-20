
// Week1Dlg.h : header file
//

#pragma once


// CWeek1Dlg dialog
class CWeek1Dlg : public CDialogEx
{
// Construction
public:
	CWeek1Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_WEEK1_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	afx_msg void OnBnClickedBtnBrowse();
	afx_msg void OnBnClickedBtnSave();
	afx_msg void OnBnClickedBtnLoad();
	afx_msg void OnEnChangeEditText();
	CString m_strContent;		// Displayed in content box
	CString m_strSavePath;		// Displayed in chosen save path
	CString m_strFolderPath;	// Displayed in folder path
};
