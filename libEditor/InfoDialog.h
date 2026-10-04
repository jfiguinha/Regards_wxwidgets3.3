#include <wx/wx.h>

class InfoDialog : public wxDialog {
public:
    InfoDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);
private:
   
    wxString m_filename;

};
