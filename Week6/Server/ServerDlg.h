
// ServerDlg.h : header file
//

#pragma once
#define WM_NET_STATUS (WM_USER + 10)
#define WM_NET_MESSAGE (WM_USER + 11)

// CServerDlg dialog
class CServerDlg : public CDialogEx
{
// Construction
public:
	CServerDlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_SERVER_DIALOG };
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

	afx_msg LRESULT OnNetStatus(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnNetMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnBnClickedBtnStartServer();
	afx_msg void OnBnClickedBtnSend();
	DECLARE_MESSAGE_MAP()
public:
	CString m_strMsg;
	SOCKET m_connectedSock = INVALID_SOCKET;
	CListCtrl m_listMsg;
};
