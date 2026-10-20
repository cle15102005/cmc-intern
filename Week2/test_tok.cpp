#include <afx.h>
#include <iostream>

int main() {
    CString strContent = L"admin,admin123\r\n";
    int nPos = 0;
    int count = 0;
    CString strLine = strContent.Tokenize(L"\r\n", nPos);
    while (!strLine.IsEmpty() && count < 100) {
        std::wcout << L"Line: " << (LPCWSTR)strLine << L" (nPos=" << nPos << L")\n";
        count++;
        strLine = strContent.Tokenize(L"\r\n", nPos);
    }
    std::wcout << L"Count: " << count << L"\n";
    return 0;
}
