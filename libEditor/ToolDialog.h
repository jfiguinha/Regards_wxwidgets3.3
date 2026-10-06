#pragma once
#include "ToolbarTools.h"
using namespace Regards::Editor;

class CColorSelectorWidget;

class ToolDialog : public wxDialog {
public:
    ToolDialog(wxWindow* parent);
    wxColour GetColor1();
    wxColour GetColor2();
private:
    void OnColorChanged(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    CToolbarTools* toolbarTools = nullptr;
    CColorSelectorWidget* colorSelector = nullptr;
};
