
// Week2Dlg.h : header file
//

// Define a struct to represent a contact
struct Contact {
	CString strName;
	CString strStatus;
	CString strLastSeen;
};

#pragma once


// CWeek2Dlg dialog
class CWeek2Dlg : public CDialogEx
{
// Construction
public:
	CWeek2Dlg(CWnd* pParent = nullptr);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CHAT_DIALOG };
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
	
private:
	CTreeCtrl m_treeContacts;
	CListCtrl m_listMessages;
	CImageList m_imageList;
	CString m_strLoggedUser;
	CString m_strMessage;

	Contact m_arrContacts[100];
	int m_nContactCount; 

	void PopulateDummyData(CTreeCtrl& treeContacts, Contact* pContacts, int nContacts);
	void LoadChatHistory(CString strContact);
	void SaveChatMessage(CString strContact, CString strTime, CString strMessage, bool bIsMe);

public:
	afx_msg void OnBnClickedBtnSend();
	afx_msg void OnTvnSelchangedTreeContacts(NMHDR* pNMHDR, LRESULT* pResult);
};

