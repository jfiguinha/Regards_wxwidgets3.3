#pragma once

namespace Regards
{
    namespace Control
    {
        class CListLayer;
    }
};

class CBitmapEditor;
class CListLayer;

class LayerDialog : public wxDialog {
public:
    LayerDialog(wxWindow* parent);
    void SetBitmapEditor(CBitmapEditor* bitmapEditor);
    void SetFile(const wxString& filename);

private:
    void OnSize(wxSizeEvent& event);

    CBitmapEditor* bitmapEditor = nullptr;
    wxString filename;
    wxStatusBar* m_statusBar;
    Regards::Control::CListLayer* listLayer;
};
