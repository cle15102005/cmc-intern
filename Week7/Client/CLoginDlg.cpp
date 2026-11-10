// CLoginDlg.cpp : implementation file
//

#include "pch.h"
#include "Client.h"
#include "CLoginDlg.h"
#include "CRegisterDlg.h" 
#include "afxdialogex.h"


// Send requested bytes
static bool SendAll(SOCKET sock, const void* pData, int nSize) {
	const char* pBuf = (const char*)pData;
	int nSent = 0;
	while (nSent < nSize) {
		int n = send(sock, pBuf + nSent, nSize - nSent, 0);
		if (n == SOCKET_ERROR) return false;
		nSent += n;
	}
	return true;
}

// Receive requestd bytes
static bool RecvAll(SOCKET sock, void* pData, int nSize) {
	char* pBuf = (char*)pData;
	int nReceived = 0;
	while (nReceived < nSize) {
		int n = recv(sock, pBuf + nReceived, nSize - nReceived, 0);
		if (n == SOCKET_ERROR || n == 0) return false; // 0 = disconnect
		nReceived += n;
	}
	return true;
}

static bool SendMessageToSocket(SOCKET sock, const CString& strMsg) {
	if (sock == INVALID_SOCKET) return false;

	// Get byte size of message
	uint32_t len = (strMsg.GetLength() + 1) * sizeof(wchar_t);

	// Convert to Network Byte Order (Big-Endian) to send to net
	uint32_t netLen = htonl(len);

	// Send first 4 bytes
	if (!SendAll(sock, &netLen, sizeof(netLen))) return false;

	// Send message conetent
	if (!SendAll(sock, strMsg.GetString(), len)) return false;

	return true;
}

static bool RecvMessageFromSocket(SOCKET sock, CString& strMsgOut) {
	if (sock == INVALID_SOCKET) return false;

	uint32_t netLen = 0;
	// Read 4-bytes length
	if (!RecvAll(sock, &netLen, sizeof(netLen))) return false;

	// Convert from Network Byte Order to normal
	uint32_t payloadLen = ntohl(netLen);
	if (payloadLen == 0 || payloadLen > 1024 * 1024) return false;

	// Allocate memory size
	wchar_t* pBuffer = new wchar_t[payloadLen / sizeof(wchar_t)];

	// Read content
	if (!RecvAll(sock, pBuffer, payloadLen)) {
		delete[] pBuffer;
		return false;
	}

	// Add to CString
	strMsgOut = pBuffer;
	delete[] pBuffer;

	return true;
}

// CLoginDlg dialog

IMPLEMENT_DYNAMIC(CLoginDlg, CDialogEx)

CLoginDlg::CLoginDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LOGIN_DIALOG, pParent)
{

}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBnClickedBtnLogin)
    ON_BN_CLICKED(IDC_BTN_REGISTER, &CLoginDlg::OnBnClickedBtnRegister)
END_MESSAGE_MAP()


// CLoginDlg message handlers

void CLoginDlg::OnBnClickedBtnLogin()
{
    CString user, pass;
    GetDlgItemText(IDC_EDIT_USER, user);
    GetDlgItemText(IDC_EDIT_PASS, pass);

    if (user.IsEmpty() || pass.IsEmpty()) {
        AfxMessageBox(L"Username or Password is empty!");
        return;
    }

    // Connect to local host
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        AfxMessageBox(L"Cannot connect to server!");
        return;
    }

    // Send LOGIN
    CString cmd = L"LOGIN|" + user + L"|" + pass;
    SendMessageToSocket(sock, cmd); 

    // Receive response
    CString response;
    if (RecvMessageFromSocket(sock, response)) {
        if (response == L"LOGIN_OK") {
            AfxMessageBox(L"Login successfully!");
            g_clientSocket = sock;
            g_loggedInUser = user;

            CDialogEx::OnOK(); 
        }
        else {
            AfxMessageBox(L"Wrong username or password!");
            closesocket(sock);
        }
    }
}



void CLoginDlg::OnBnClickedBtnRegister()
{
    // TODO: Add your control notification handler code here
    CRegisterDlg regDlg;
    regDlg.DoModal(); 
}
