#pragma once

#include <wx/panel.h>
#include <wx/bitmap.h>

class CColorWheelCtrl : public wxPanel
{
public:
    CColorWheelCtrl(
        wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxSize(220, 220));

    void SetHue(double hue);
    double GetHue() const;

    void RefreshWheel();

private:
    void OnPaint(wxPaintEvent& event);
    void OnSize(wxSizeEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseMove(wxMouseEvent& event);
    void OnMouseUp(wxMouseEvent& event);

    void GenerateWheel();
    void UpdateHueFromMouse(const wxPoint& point);

    wxBitmap m_bitmap;

    double m_hue = 0.0;

    bool m_dragging = false;

    int m_centerX = 0;
    int m_centerY = 0;
    int m_radius = 0;
    int m_innerRadius = 0;

    wxDECLARE_EVENT_TABLE();
};