#pragma once


namespace Regards
{
    namespace Control
    {
        class CInfosFileWnd;
    }
}

class InfoDialog : public wxDialog {
public:
    InfoDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);
    wxString GetFilename();

private:
   
    void OnSize(wxSizeEvent& event);
    wxStatusBar* m_statusBar;
    wxString m_filename = "";
    Regards::Control::CInfosFileWnd * infosFileWnd;
};
