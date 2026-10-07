#pragma once

namespace Regards
{
    namespace Editor
    {
        class CToolbarTools;
    }
}

class CColorSelectorWidget;

class ToolDialog : public wxDialog {
public:
    ToolDialog(wxWindow* parent);
    wxColour GetColor1();
    wxColour GetColor2();
private:
    void OnColorChanged(wxCommandEvent& event);
    void OnSize(wxSizeEvent& event);
    Regards::Editor::CToolbarTools* toolbarTools = nullptr;
    CColorSelectorWidget* colorSelector = nullptr;
};
