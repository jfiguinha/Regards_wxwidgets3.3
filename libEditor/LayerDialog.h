#pragma once
#include <ListLayer.h>
using namespace Regards::Control;

class LayerDialog : public wxDialog {
public:
    LayerDialog(wxWindow* parent);

private:
    void OnSize(wxSizeEvent& event);

    wxStatusBar* m_statusBar;
    CListLayer* listLayer;
};
