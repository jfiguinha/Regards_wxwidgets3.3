#include <wx/wx.h>

class EffectsDialog : public wxDialog {
public:
    EffectsDialog(wxWindow* parent);
    void SetFilename(const wxString& filename);
private:
    wxString m_filename;
};
