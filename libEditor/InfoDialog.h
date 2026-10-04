#pragma once
#include <InfosFileWnd.h>


class InfoDialog : public wxDialog {
public:
    InfoDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);
private:
   
    void OnSize(wxSizeEvent& event);

    wxString m_filename;
    Regards::Control::CInfosFileWnd * infosFileWnd;
};
