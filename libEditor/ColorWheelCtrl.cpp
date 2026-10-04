#include <wx/dcbuffer.h>
#include <wx/image.h>

#include <algorithm>
#include <cmath>

#include "ColorWheelCtrl.h"

namespace {
    constexpr double PI = 3.14159265358979323846;

    void HSLToRGB(double h, double s, double l, unsigned char& r, unsigned char& g,
        unsigned char& b) {
        h = std::fmod(h, 360.0);

        if (h < 0.0) h += 360.0;

        s = std::clamp(s, 0.0, 1.0);
        l = std::clamp(l, 0.0, 1.0);

        if (s <= 0.0) {
            const unsigned char value =
                static_cast<unsigned char>(std::round(l * 255.0));

            r = value;
            g = value;
            b = value;

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

wxBEGIN_EVENT_TABLE(CColorWheelCtrl, wxPanel)
EVT_PAINT(CColorWheelCtrl::OnPaint) EVT_SIZE(CColorWheelCtrl::OnSize)
EVT_LEFT_DOWN(CColorWheelCtrl::OnMouseDown)
EVT_LEFT_UP(CColorWheelCtrl::OnMouseUp)
EVT_MOTION(CColorWheelCtrl::OnMouseMove) wxEND_EVENT_TABLE()

CColorWheelCtrl::CColorWheelCtrl(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size)
    : wxPanel(parent, id, pos, size) {
    SetBackgroundStyle(wxBG_STYLE_PAINT);

    GenerateWheel();
}

void CColorWheelCtrl::SetHue(double hue) {
    hue = std::fmod(hue, 360.0);

    if (hue < 0.0) hue += 360.0;

    m_hue = hue;

    Refresh();
}

double CColorWheelCtrl::GetHue() const { return m_hue; }

void CColorWheelCtrl::RefreshWheel() {
    GenerateWheel();
    Refresh();
}

void CColorWheelCtrl::OnSize(wxSizeEvent& event) {
    GenerateWheel();

    event.Skip();
}
void CColorWheelCtrl::GenerateWheel()
{
    const wxSize size = GetClientSize();

    if (size.x <= 0 || size.y <= 0)
        return;

    const int width = size.x;
    const int height = size.y;

    m_centerX = width / 2;
    m_centerY = height / 2;

    m_radius = std::max(
        1,
        std::min(width, height) / 2 - 4);

    wxImage image(width, height, true);
    image.SetAlpha();

    unsigned char* data = image.GetData();
    unsigned char* alpha = image.GetAlpha();

    if (!data || !alpha)
        return;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const double dx =
                static_cast<double>(x - m_centerX);

            const double dy =
                static_cast<double>(y - m_centerY);

            const double distance =
                std::sqrt(dx * dx + dy * dy);

            const int index =
                (y * width + x) * 3;

            const int alphaIndex =
                y * width + x;

            // Cercle plein
            if (distance <= m_radius)
            {
                double angle =
                    std::atan2(dy, dx) *
                    180.0 / PI;

                // Rouge en haut
                angle += 90.0;

                if (angle < 0.0)
                    angle += 360.0;

                if (angle >= 360.0)
                    angle -= 360.0;

                unsigned char r;
                unsigned char g;
                unsigned char b;

                HSLToRGB(
                    angle,
                    1.0,
                    0.5,
                    r,
                    g,
                    b);

                data[index + 0] = r;
                data[index + 1] = g;
                data[index + 2] = b;

                alpha[alphaIndex] = 255;
            }
            else
            {
                data[index + 0] = 0;
                data[index + 1] = 0;
                data[index + 2] = 0;

                alpha[alphaIndex] = 0;
            }
        }
    }

    m_bitmap = wxBitmap(image);
}

void CColorWheelCtrl::OnPaint(wxPaintEvent&) {
    wxAutoBufferedPaintDC dc(this);

    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    if (m_bitmap.IsOk()) {
        dc.DrawBitmap(m_bitmap, 0, 0, true);
    }

    const double angle =
        (m_hue - 90.0) * PI / 180.0;

    const double radius =
        static_cast<double>(m_radius - 8);

    const int x =
        static_cast<int>(
            std::round(
                m_centerX +
                std::cos(angle) * radius));

    const int y =
        static_cast<int>(
            std::round(
                m_centerY +
                std::sin(angle) * radius));

    dc.SetPen(wxPen(*wxBLACK, 3));
    dc.SetBrush(*wxTRANSPARENT_BRUSH);

    dc.DrawCircle(x, y, 7);

    dc.SetPen(wxPen(*wxWHITE, 1));

    dc.DrawCircle(x, y, 7);
}
void CColorWheelCtrl::UpdateHueFromMouse(const wxPoint& point) {
    const double dx = static_cast<double>(point.x - m_centerX);

    const double dy = static_cast<double>(point.y - m_centerY);

    const double distance = std::sqrt(dx * dx + dy * dy);

    // En dehors du cercle
    if (distance > static_cast<double>(m_radius)) return;

    // ---------------------------------------------------------
    // Centre du cercle
    // ---------------------------------------------------------
    //
    // Au centre, l'angle n'a pas de sens mathématique.
    // On conserve donc la teinte actuelle.
    //
    if (distance < 1.0) {
        Refresh();

        wxCommandEvent event(wxEVT_COMMAND_SLIDER_UPDATED, GetId());

        event.SetInt(static_cast<int>(std::round(m_hue)));

        event.SetEventObject(this);

        ProcessWindowEvent(event);

        return;
    }

    // ---------------------------------------------------------
    // Calcul de la teinte
    // ---------------------------------------------------------

    double angle = std::atan2(dy, dx) * 180.0 / PI;

    // Rouge en haut
    angle += 90.0;

    if (angle < 0.0) angle += 360.0;

    if (angle >= 360.0) angle -= 360.0;

    m_hue = angle;

    Refresh();

    wxCommandEvent event(wxEVT_COMMAND_SLIDER_UPDATED, GetId());

    event.SetInt(static_cast<int>(std::round(m_hue)));

    event.SetEventObject(this);

    ProcessWindowEvent(event);
}

void CColorWheelCtrl::OnMouseDown(wxMouseEvent& event) {
    m_dragging = true;

    CaptureMouse();

    UpdateHueFromMouse(event.GetPosition());
}

void CColorWheelCtrl::OnMouseMove(wxMouseEvent& event) {
    if (!m_dragging) return;

    UpdateHueFromMouse(event.GetPosition());
}

void CColorWheelCtrl::OnMouseUp(wxMouseEvent&) {
    m_dragging = false;

    if (HasCapture()) ReleaseMouse();
}