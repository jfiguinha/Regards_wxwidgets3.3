#pragma once

#include <wx/dialog.h>
#include <wx/colour.h>

class CColorWheelCtrl;
class CColorSLPickerCtrl;

class wxSpinCtrl;
class wxSlider;
class wxPanel;
class wxStaticText;

class CColorPickerDialog : public wxDialog
{
public:
    CColorPickerDialog(
        wxWindow* parent,
        const wxColour& colour = *wxBLACK,
        wxWindowID id = wxID_ANY,
        const wxString& title = "Sélection de couleur",
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(620, 500));

    void SetColour(const wxColour& colour);
    wxColour GetColour() const;

private:
    void CreateControls();
    void UpdateControlsFromColour();
    void UpdateColourFromHSL();
    void UpdateColourFromRGB();

    void UpdatePreview();

    void OnWheelChanged(wxCommandEvent& event);
    void OnSLChanged(wxCommandEvent& event);

    void OnRGBChanged(wxSpinEvent& event);
    void OnHSLChanged(wxSpinEvent& event);

    void OnAlphaChanged(wxCommandEvent& event);

    void OnOK(wxCommandEvent& event);

    wxColour m_colour;

    double m_hue = 0.0;
    double m_saturation = 0.0;
    double m_lightness = 0.0;

    CColorWheelCtrl* m_colorWheel = nullptr;
    CColorSLPickerCtrl* m_slPicker = nullptr;

    wxPanel* m_preview = nullptr;

    wxSpinCtrl* m_red = nullptr;
    wxSpinCtrl* m_green = nullptr;
    wxSpinCtrl* m_blue = nullptr;

    wxSpinCtrl* m_hueCtrl = nullptr;
    wxSpinCtrl* m_saturationCtrl = nullptr;
    wxSpinCtrl* m_lightnessCtrl = nullptr;

    wxSlider* m_alphaCtrl = nullptr;

    wxStaticText* m_alphaValue = nullptr;

    bool m_updating = false;

    wxDECLARE_EVENT_TABLE();
};