#pragma once
#include <InfosFileWnd.h>


class InfoDialog : public wxDialog {
public:
    InfoDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);
private:
   
    void OnSize(wxSizeEvent& event);
    wxStatusBar* m_statusBar;
    wxString m_filename;
    Regards::Control::CInfosFileWnd * infosFileWnd;
};
