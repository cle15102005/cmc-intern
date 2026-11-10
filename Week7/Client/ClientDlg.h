
// ClientDlg.h : header file
//

#pragma once
#define WM_NET_STATUS (WM_USER + 10)
#define WM_NET_MESSAGE (WM_USER + 11)

// CClientDlg dialog
class CClientDlg : public CDialogEx
{
// Construction
public:
	CClientDlg(CWnd* pParent = nullptr);	// standard constructor
	virtual ~CClientDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CLIENT_DIALOG };
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
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnBnClickedChkAutorun();
	DECLARE_MESSAGE_MAP()
public:
	SOCKET m_connectedSock = INVALID_SOCKET;
	CString m_strMsg;
	CString m_strCurrentPeer;
	CListCtrl m_listMsg;
	CTreeCtrl m_treeUsers;
	BOOL m_bAutorun;
	afx_msg void OnLvnItemchangedListMsg(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnTvnSelchangedTreeUsers(NMHDR* pNMHDR, LRESULT* pResult);
	void AdjustMessageColumnWidth();
};
