#pragma once

namespace Regards
{
    namespace Control
    {
        class CListLayer;
    }
};

class CBitmapEditor;
class CLayerList;

class LayerDialog : public wxDialog {
public:
    LayerDialog(wxWindow* parent);
    void SetLayerList(CLayerList* listOfLayer);
   //void SetFile(const wxString& filename);

private:
    void OnSize(wxSizeEvent& event);
    wxStatusBar* m_statusBar;
    Regards::Control::CListLayer* listLayer;
};
