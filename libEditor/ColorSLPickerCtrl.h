#pragma once

#include <wx/panel.h>
#include <wx/bitmap.h>

class CColorSLPickerCtrl : public wxPanel
{
public:
    CColorSLPickerCtrl(
        wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(220, 160));

    void SetHue(double hue);
    void SetSaturation(double saturation);
    void SetLightness(double lightness);

    double GetHue() const;
    double GetSaturation() const;
    double GetLightness() const;

    void SetHSL(
        double hue,
        double saturation,
        double lightness);

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);

    void GenerateBitmap();
    void UpdateFromMouse(const wxPoint& point);

    wxBitmap m_bitmap;

    double m_hue = 0.0;
    double m_saturation = 1.0;
    double m_lightness = 0.5;

    bool m_dragging = false;

    wxDECLARE_EVENT_TABLE();
};