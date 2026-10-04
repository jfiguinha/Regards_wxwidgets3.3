#include <header.h>
#include "ColorSLPickerCtrl.h"
#include <wx/dcbuffer.h>
#include <wx/image.h>

#include <algorithm>
#include <cmath>



namespace {
    void HSLToRGB(double h, double s, double l, unsigned char& r, unsigned char& g,
        unsigned char& b) {
        h = std::fmod(h, 360.0);

        if (h < 0.0) h += 360.0;

        s = std::clamp(s, 0.0, 1.0);
        l = std::clamp(l, 0.0, 1.0);

        if (s == 0.0) {
            const unsigned char v = static_cast<unsigned char>(std::round(l * 255.0));

            r = v;
            g = v;
            b = v;

            return;
        }

        const double q = l < 0.5 ? l * (1.0 + s) : l + s - l * s;

        const double p = 2.0 * l - q;

        auto hueToRGB = [](double p, double q, double t) {
            if (t < 0.0) t += 1.0;

            if (t > 1.0) t -= 1.0;

            if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;

            if (t < 1.0 / 2.0) return q;

            if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;

            return p;
            };

        const double hk = h / 360.0;

        r = static_cast<unsigned char>(
            std::round(hueToRGB(p, q, hk + 1.0 / 3.0) * 255.0));

        g = static_cast<unsigned char>(std::round(hueToRGB(p, q, hk) * 255.0));

        b = static_cast<unsigned char>(
            std::round(hueToRGB(p, q, hk - 1.0 / 3.0) * 255.0));
    }
}  // namespace

wxBEGIN_EVENT_TABLE(CColorSLPickerCtrl, wxPanel)
EVT_PAINT(CColorSLPickerCtrl::OnPaint) EVT_SIZE(CColorSLPickerCtrl::OnSize)
EVT_LEFT_DOWN(CColorSLPickerCtrl::OnMouseDown)
EVT_LEFT_UP(CColorSLPickerCtrl::OnMouseUp)
EVT_MOTION(CColorSLPickerCtrl::OnMouseMove) wxEND_EVENT_TABLE()

CColorSLPickerCtrl::CColorSLPickerCtrl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size)
    : wxPanel(parent, id, pos, size) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    GenerateBitmap();
}

void CColorSLPickerCtrl::SetHue(double hue) {
    m_hue = std::fmod(hue, 360.0);

    if (m_hue < 0.0) m_hue += 360.0;

    GenerateBitmap();
    Refresh();
}

void CColorSLPickerCtrl::SetSaturation(double saturation) {
    m_saturation = std::clamp(saturation, 0.0, 1.0);

    Refresh();
}

void CColorSLPickerCtrl::SetLightness(double lightness) {
    m_lightness = std::clamp(lightness, 0.0, 1.0);

    Refresh();
}

void CColorSLPickerCtrl::SetHSL(double hue, double saturation,
    double lightness) {
    m_hue = hue;
    m_saturation = std::clamp(saturation, 0.0, 1.0);
    m_lightness = std::clamp(lightness, 0.0, 1.0);

    GenerateBitmap();
    Refresh();
}

double CColorSLPickerCtrl::GetHue() const { return m_hue; }

double CColorSLPickerCtrl::GetSaturation() const { return m_saturation; }

double CColorSLPickerCtrl::GetLightness() const { return m_lightness; }

void CColorSLPickerCtrl::OnSize(wxSizeEvent& event) {
    GenerateBitmap();

    event.Skip();
}

void CColorSLPickerCtrl::GenerateBitmap() {
    const wxSize size = GetClientSize();

    if (size.x <= 0 || size.y <= 0) return;

    wxImage image(size.x, size.y, true);

    unsigned char* data = image.GetData();

    if (!data) return;

    for (int y = 0; y < size.y; ++y) {
        const double lightness =
            1.0 -
            static_cast<double>(y) / static_cast<double>(std::max(1, size.y - 1));

        for (int x = 0; x < size.x; ++x) {
            const double saturation =
                static_cast<double>(x) / static_cast<double>(std::max(1, size.x - 1));

            unsigned char r;
            unsigned char g;
            unsigned char b;

            HSLToRGB(m_hue, saturation, lightness, r, g, b);

            const int index = (y * size.x + x) * 3;

            data[index + 0] = r;
            data[index + 1] = g;
            data[index + 2] = b;
        }
    }

    m_bitmap = wxBitmap(image);
}

void CColorSLPickerCtrl::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);

    dc.Clear();

    if (m_bitmap.IsOk()) {
        dc.DrawBitmap(m_bitmap, 0, 0, false);
    }

    const wxSize size = GetClientSize();

    const int x = static_cast<int>(
        std::round(m_saturation * static_cast<double>(std::max(0, size.x - 1))));

    const int y = static_cast<int>(std::round(
        (1.0 - m_lightness) * static_cast<double>(std::max(0, size.y - 1))));

    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    dc.SetPen(wxPen(*wxBLACK, 3));

    dc.DrawCircle(x, y, 7);

    dc.SetPen(wxPen(*wxWHITE, 1));

    dc.DrawCircle(x, y, 7);
}

void CColorSLPickerCtrl::UpdateFromMouse(const wxPoint& point) {
    const wxSize size = GetClientSize();

    if (size.x <= 1 || size.y <= 1) return;

    const double saturation = std::clamp(
        static_cast<double>(point.x) / static_cast<double>(size.x - 1), 0.0, 1.0);

    const double lightness = std::clamp(
        1.0 - static_cast<double>(point.y) / static_cast<double>(size.y - 1), 0.0,
        1.0);

    m_saturation = saturation;
    m_lightness = lightness;

    Refresh();

    wxCommandEvent event(wxEVT_COMMAND_SLIDER_UPDATED, GetId());

    event.SetInt(1);
    event.SetEventObject(this);

    ProcessWindowEvent(event);
}

void CColorSLPickerCtrl::OnMouseDown(wxMouseEvent& event) {
    m_dragging = true;

    CaptureMouse();

    UpdateFromMouse(event.GetPosition());
}

void CColorSLPickerCtrl::OnMouseMove(wxMouseEvent& event) {
    if (!m_dragging) return;

    UpdateFromMouse(event.GetPosition());
}

void CColorSLPickerCtrl::OnMouseUp(wxMouseEvent&) {
    m_dragging = false;

    if (HasCapture()) ReleaseMouse();
}