#include <string>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

int main()
{
    std::string s = "hello";
    std::wstring ws = L"hello";

    wxString test = "hello";

    return static_cast<int>(
        s.size() +
        ws.size() +
        test.size());
}
