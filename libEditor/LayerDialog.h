#pragma once
#include <ListLayer.h>
using namespace Regards::Control;

class LayerDialog : public wxDialog {
public:
    LayerDialog(wxWindow* parent);
    void SetFile(const wxString& filename);

private:
    void OnSize(wxSizeEvent& event);

    wxString filename;
    wxStatusBar* m_statusBar;
    CListLayer* listLayer;
};
