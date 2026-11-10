// CRegisterDlg.cpp : implementation file
//

#include "pch.h"
#include "Client.h"
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

// CRegisterDlg dialog

IMPLEMENT_DYNAMIC(CRegisterDlg, CDialogEx)

CRegisterDlg::CRegisterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REGISTER_DIALOG, pParent)
{

}

CRegisterDlg::~CRegisterDlg()
{
}

void CRegisterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRegisterDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_REGISTER, &CRegisterDlg::OnBnClickedBtnRegister)
END_MESSAGE_MAP()


// CRegisterDlg message handlers


void CRegisterDlg::OnBnClickedBtnRegister()
{
	// TODO: Add your control notification handler code here
    CString user, pass, confirmPass;
    GetDlgItemText(IDC_EDIT_USER, user);
    GetDlgItemText(IDC_EDIT_PASS, pass);
    GetDlgItemText(IDC_EDIT_CF_PASS, confirmPass);

    if (user.IsEmpty() || pass.IsEmpty() || confirmPass.IsEmpty()) {
        AfxMessageBox(L"Please fill all information!");
        return;
    }

    if (pass != confirmPass) {
        AfxMessageBox(L"Confirm password does not match!");
        return;
    }

    if (pass.GetLength() < 6) {
        AfxMessageBox(L"Password must have at least 6 characters!");
        return;
    }

    // Connect to server
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9999);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        AfxMessageBox(L"Cannot connect to server!");
        return;
    }

    // Send REG to server
    CString cmd = L"REG|" + user + L"|" + pass;
    if (!SendMessageToSocket(sock, cmd)) {
        AfxMessageBox(L"Error sending message!");
        closesocket(sock);
        return;
    }

    // Get response from server
    CString response;
    if (RecvMessageFromSocket(sock, response)) {
        if (response == L"REG_OK") {
            AfxMessageBox(L"Register successfully! Please login.");

            CDialogEx::OnOK(); // 
        }
        else if (response.Left(8) == L"REG_FAIL") {
            CString reason;
            AfxExtractSubString(reason, response, 1, L'|');
            AfxMessageBox(L"Register failed: " + reason);
            
            //Clean up box
            SetDlgItemText(IDC_EDIT_PASS, L"");
            SetDlgItemText(IDC_EDIT_CF_PASS, L"");
        }
    }
    else {
        AfxMessageBox(L"No response from server!");
    }

    closesocket(sock);
}
