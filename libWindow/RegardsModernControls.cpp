#include <header.h>
#include "RegardsModernControls.h"

#include <wx/wx.h>
#include <wx/dcbuffer.h>
#include <wx/graphics.h>
#include <algorithm>

namespace Regards::UI
{
    namespace
    {
        void DrawRounded(wxGraphicsContext* gc, const wxRect& rect,
                        const wxColour& fill, const wxColour& outline,
                        double radius, double penWidth = 1.0)
        {
            if (!gc) return;
            gc->SetBrush(wxBrush(fill));
            gc->SetPen(wxPen(outline, penWidth));
            gc->DrawRoundedRectangle(rect.x + 0.5, rect.y + 0.5,
                                     std::max(0, rect.width - 1),
                                     std::max(0, rect.height - 1), radius);
        }

        wxColour Blend(const wxColour& a, const wxColour& b, double t)
        {
            t = std::clamp(t, 0.0, 1.0);
            return wxColour(
                static_cast<unsigned char>(a.Red()   + (b.Red()   - a.Red())   * t),
                static_cast<unsigned char>(a.Green() + (b.Green() - a.Green()) * t),
                static_cast<unsigned char>(a.Blue()  + (b.Blue()  - a.Blue())  * t));
        }
    }

    ModernTheme ModernTheme::Light()
    {
        ModernTheme t;
        t.background = wxColour(243, 245, 248);
        t.surface = wxColour(255, 255, 255);
        t.text = wxColour(32, 35, 40);
        t.secondaryText = wxColour(100, 107, 117);
        t.border = wxColour(210, 214, 220);
        t.accent = wxColour(30, 120, 220);
        t.accentHover = wxColour(20, 103, 200);
        t.hover = wxColour(232, 236, 241);
        t.pressed = wxColour(220, 225, 232);
        t.disabled = wxColour(160, 166, 174);
        t.dark = false;
        return t;
    }

    ModernTheme ModernTheme::Dark()
    {
        ModernTheme t;
        t.background = wxColour(30, 32, 35);
        t.surface = wxColour(39, 42, 46);
        t.text = wxColour(238, 240, 243);
        t.secondaryText = wxColour(165, 171, 180);
        t.border = wxColour(65, 70, 77);
        t.accent = wxColour(70, 150, 235);
        t.accentHover = wxColour(90, 165, 245);
        t.hover = wxColour(52, 57, 63);
        t.pressed = wxColour(62, 68, 75);
        t.disabled = wxColour(105, 110, 117);
        t.dark = true;
        return t;
    }

    // ---------------- CModernPanel ----------------

    CModernPanel::CModernPanel(wxWindow* parent, wxWindowID id,
                               const wxPoint& pos, const wxSize& size, long style)
        : wxPanel(parent, id, pos, size, style | wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetBackgroundColour(m_theme.surface);
        Bind(wxEVT_PAINT, &CModernPanel::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernPanel::OnEraseBackground, this);
    }

    void CModernPanel::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        SetBackgroundColour(theme.surface);
        Refresh();
    }

    void CModernPanel::SetCornerRadius(int radius)
    {
        m_radius = std::max(0, radius);
        Refresh();
    }

    void CModernPanel::SetBorder(bool enabled)
    {
        m_border = enabled;
        Refresh();
    }

    void CModernPanel::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.surface));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const wxSize s = GetClientSize();
        const wxColour border = m_border ? m_theme.border : m_theme.surface;
        DrawRounded(gc.get(), wxRect(0, 0, s.x, s.y), m_theme.surface, border, m_radius);
    }

    void CModernPanel::OnEraseBackground(wxEraseEvent&) {}

    // ---------------- CModernButton ----------------

    CModernButton::CModernButton(wxWindow* parent, wxWindowID id, const wxString& label,
                                 const wxPoint& pos, const wxSize& size, long style)
        : wxControl(parent, id, pos, size, style | wxBORDER_NONE),
          m_label(label)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetMinSize(size == wxDefaultSize ? wxSize(96, 34) : size);
        SetForegroundColour(m_theme.text);
        SetBackgroundColour(m_theme.surface);
        SetFont(wxFont(wxFontInfo(10).Family(wxFONTFAMILY_DEFAULT)));
        Bind(wxEVT_PAINT, &CModernButton::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernButton::OnEraseBackground, this);
        Bind(wxEVT_ENTER_WINDOW, &CModernButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &CModernButton::OnMouseLeave, this);
        Bind(wxEVT_LEFT_DOWN, &CModernButton::OnLeftDown, this);
        Bind(wxEVT_LEFT_UP, &CModernButton::OnLeftUp, this);
        Bind(wxEVT_MOUSE_CAPTURE_LOST, &CModernButton::OnCaptureLost, this);
        Bind(wxEVT_KEY_DOWN, &CModernButton::OnKeyDown, this);
        Bind(wxEVT_KEY_UP, &CModernButton::OnKeyUp, this);
        Bind(wxEVT_SET_FOCUS, &CModernButton::OnSetFocus, this);
        Bind(wxEVT_KILL_FOCUS, &CModernButton::OnKillFocus, this);
    }

    void CModernButton::SetLabel(const wxString& label)
    {
        m_label = label;
        Refresh();
    }

    void CModernButton::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        SetForegroundColour(theme.text);
        Refresh();
    }

    void CModernButton::SetAccent(bool accent)
    {
        m_accent = accent;
        Refresh();
    }

    void CModernButton::SetCornerRadius(int radius)
    {
        m_radius = std::max(0, radius);
        Refresh();
    }

    void CModernButton::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.background));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        wxRect r(GetClientRect());
        wxColour fill = m_theme.surface;
        wxColour border = m_theme.border;
        wxColour text = m_theme.text;

        if (!IsEnabled())
        {
            fill = m_theme.background;
            text = m_theme.disabled;
        }
        else if (m_accent)
        {
            fill = m_pressed ? m_theme.accentHover :
                   (m_hover ? m_theme.accentHover : m_theme.accent);
            border = fill;
            text = *wxWHITE;
        }
        else if (m_pressed)
            fill = m_theme.pressed;
        else if (m_hover)
            fill = m_theme.hover;

        DrawRounded(gc.get(), r, fill, border, m_radius);

        if (m_focused)
        {
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->SetPen(wxPen(m_theme.accent, 1, wxPENSTYLE_DOT));
            gc->DrawRoundedRectangle(3, 3, r.width - 6, r.height - 6,
                                     std::max(0, m_radius - 2));
        }

        gc->SetFont(GetFont(), text);
        wxDouble tw = 0, th = 0, descent = 0, leading = 0;
        gc->GetTextExtent(m_label, &tw, &th, &descent, &leading);
        gc->DrawText(m_label, (r.width - tw) / 2.0, (r.height - th) / 2.0);
    }

    void CModernButton::OnEraseBackground(wxEraseEvent&) {}
    void CModernButton::OnMouseEnter(wxMouseEvent&) { m_hover = true; Refresh(); }
    void CModernButton::OnMouseLeave(wxMouseEvent&) { m_hover = false; Refresh(); }

    void CModernButton::OnLeftDown(wxMouseEvent& e)
    {
        if (!IsEnabled()) return;
        m_pressed = true;
        CaptureMouse();
        SetFocus();
        Refresh();
    }

    void CModernButton::OnLeftUp(wxMouseEvent& e)
    {
        if (!m_pressed) return;
        m_pressed = false;
        if (HasCapture()) ReleaseMouse();
        Refresh();

        if (GetClientRect().Contains(e.GetPosition()) && IsEnabled())
        {
            wxCommandEvent event(wxEVT_BUTTON, GetId());
            event.SetEventObject(this);
            event.SetString(m_label);
            ProcessWindowEvent(event);
        }
    }

    void CModernButton::OnCaptureLost(wxMouseCaptureLostEvent&) { m_pressed = false; Refresh(); }

    void CModernButton::OnKeyDown(wxKeyEvent& e)
    {
        if (e.GetKeyCode() == WXK_SPACE || e.GetKeyCode() == WXK_RETURN)
        {
            if (!m_pressed) { m_pressed = true; Refresh(); }
        }
        else e.Skip();
    }

    void CModernButton::OnKeyUp(wxKeyEvent& e)
    {
        if ((e.GetKeyCode() == WXK_SPACE || e.GetKeyCode() == WXK_RETURN) && m_pressed)
        {
            m_pressed = false;
            Refresh();
            wxCommandEvent event(wxEVT_BUTTON, GetId());
            event.SetEventObject(this);
            event.SetString(m_label);
            ProcessWindowEvent(event);
        }
        else e.Skip();
    }

    void CModernButton::OnSetFocus(wxFocusEvent&) { m_focused = true; Refresh(); }
    void CModernButton::OnKillFocus(wxFocusEvent&) { m_focused = false; m_pressed = false; Refresh(); }

    // ---------------- CModernSlider ----------------

    CModernSlider::CModernSlider(wxWindow* parent, wxWindowID id,
                                 int value, int minValue, int maxValue,
                                 const wxPoint& pos, const wxSize& size)
        : wxControl(parent, id, pos, size, wxBORDER_NONE),
          m_min(minValue), m_max(std::max(minValue + 1, maxValue)),
          m_value(std::clamp(value, m_min, m_max))
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        //SetSize(size == wxDefaultSize ? wxSize(180, 28) : size);
        Bind(wxEVT_PAINT, &CModernSlider::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernSlider::OnEraseBackground, this);
        Bind(wxEVT_LEFT_DOWN, &CModernSlider::OnLeftDown, this);
        Bind(wxEVT_LEFT_UP, &CModernSlider::OnLeftUp, this);
        Bind(wxEVT_MOTION, &CModernSlider::OnMotion, this);
        Bind(wxEVT_MOUSE_CAPTURE_LOST, &CModernSlider::OnCaptureLost, this);
        Bind(wxEVT_KEY_DOWN, &CModernSlider::OnKeyDown, this);
    }

    void CModernSlider::SetValue(int value)
    {
        const int v = std::clamp(value, m_min, m_max);
        if (v == m_value) return;
        m_value = v;
        Refresh();
    }

    void CModernSlider::SetRange(int minValue, int maxValue)
    {
        m_min = minValue;
        m_max = std::max(minValue + 1, maxValue);
        m_value = std::clamp(m_value, m_min, m_max);
        Refresh();
    }

    void CModernSlider::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        Refresh();
    }

    void CModernSlider::OnEraseBackground(wxEraseEvent&) {}

    // ---------------- CModernSlider (Support Horizontal et Vertical automatique) ----------------

    int CModernSlider::ThumbX() const
    {
        const wxSize sz = GetClientSize();
        if (sz.y > sz.x) // Mode Vertical
        {
            const int top = 9, bottom = std::max(top + 1, sz.y - 9);
            // Progression du bas (max) vers le haut (min) pour le volume
            const double t = double(m_value - m_min) / double(m_max - m_min);
            return bottom - static_cast<int>((bottom - top) * t);
        }
        else // Mode Horizontal
        {
            const int left = 9, right = std::max(left + 1, sz.x - 9);
            const double t = double(m_value - m_min) / double(m_max - m_min);
            return left + static_cast<int>((right - left) * t);
        }
    }

    int CModernSlider::ValueFromX(int pos) const
    {
        const wxSize sz = GetClientSize();
        if (sz.y > sz.x) // Mode Vertical (pos représente le Y de la souris)
        {
            const int top = 9, bottom = std::max(top + 1, sz.y - 9);
            // Inversion pour que le clic en bas donne la valeur minimale
            const double t = std::clamp(double(bottom - pos) / double(bottom - top), 0.0, 1.0);
            return m_min + static_cast<int>(std::lround(t * (m_max - m_min)));
        }
        else // Mode Horizontal (pos représente le X de la souris)
        {
            const int left = 9, right = std::max(left + 1, sz.x - 9);
            const double t = std::clamp(double(pos - left) / double(right - left), 0.0, 1.0);
            return m_min + static_cast<int>(std::lround(t * (m_max - m_min)));
        }
    }

    void CModernSlider::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.background));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const wxSize sz = GetClientSize();
        const int thumb = ThumbX();

        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->SetBrush(wxBrush(m_theme.border));

        if (isVertical) // Dessin Vertical
        {
            const int x = sz.x / 2;
            const int top = 9, bottom = std::max(top + 1, sz.y - 9);

            // Rail complet arrière-plan
            gc->DrawRoundedRectangle(x - 2, top, 4, bottom - top, 2);

            // Rail coloré actif (du bas jusqu'au curseur)
            gc->SetBrush(wxBrush(m_theme.accent));
            gc->DrawRoundedRectangle(x - 2, thumb, 4, std::max(0, bottom - thumb), 2);

            // Curseur (Ellipse)
            const int radius = m_hover || m_dragging ? 8 : 7;
            gc->SetBrush(wxBrush(m_theme.surface));
            gc->SetPen(wxPen(m_theme.accent, 2));
            gc->DrawEllipse(x - radius, thumb - radius, radius * 2, radius * 2);
        }
        else // Dessin Horizontal original
        {
            const int y = sz.y / 2;
            const int left = 9, right = std::max(left + 1, sz.x - 9);

            gc->DrawRoundedRectangle(left, y - 2, right - left, 4, 2);

            gc->SetBrush(wxBrush(m_theme.accent));
            gc->DrawRoundedRectangle(left, y - 2, std::max(0, thumb - left), 4, 2);

            const int radius = m_hover || m_dragging ? 8 : 7;
            gc->SetBrush(wxBrush(m_theme.surface));
            gc->SetPen(wxPen(m_theme.accent, 2));
            gc->DrawEllipse(thumb - radius, y - radius, radius * 2, radius * 2);
        }
    }

    void CModernSlider::OnLeftDown(wxMouseEvent& e)
    {
        if (!IsEnabled()) return;
        m_dragging = true;
        CaptureMouse();
        SetFocus();
        const wxSize sz = GetClientSize();
        UpdateFromMouse(sz.y > sz.x ? e.GetY() : e.GetX());
    }

    void CModernSlider::OnLeftUp(wxMouseEvent& e)
    {
        if (!m_dragging) return;
        const wxSize sz = GetClientSize();
        UpdateFromMouse(sz.y > sz.x ? e.GetY() : e.GetX());
        m_dragging = false;
        if (HasCapture()) ReleaseMouse();
        Refresh();
    }

    void CModernSlider::OnMotion(wxMouseEvent& e)
    {
        m_hover = true;
        if (m_dragging && e.LeftIsDown())
        {
            const wxSize sz = GetClientSize();
            UpdateFromMouse(sz.y > sz.x ? e.GetY() : e.GetX());
        }
        Refresh();
    }

    void CModernSlider::OnKeyDown(wxKeyEvent& e)
    {
        int step = e.ShiftDown() ? 10 : 1;
        const wxSize sz = GetClientSize();

        // En mode vertical, la flèche HAUT augmente la valeur, la flèche BAS la diminue
        if (sz.y > sz.x)
        {
            if (e.GetKeyCode() == WXK_UP) SetValue(m_value + step);
            else if (e.GetKeyCode() == WXK_DOWN) SetValue(m_value - step);
            else if (e.GetKeyCode() == WXK_HOME) SetValue(m_min);
            else if (e.GetKeyCode() == WXK_END) SetValue(m_max);
            else { e.Skip(); return; }
        }
        else // Mode horizontal classique
        {
            if (e.GetKeyCode() == WXK_LEFT) SetValue(m_value - step);
            else if (e.GetKeyCode() == WXK_RIGHT) SetValue(m_value + step);
            else if (e.GetKeyCode() == WXK_HOME) SetValue(m_min);
            else if (e.GetKeyCode() == WXK_END) SetValue(m_max);
            else { e.Skip(); return; }
        }
        SendChangedEvent();
    }

    void CModernSlider::UpdateFromMouse(int pos)
    {
        const int v = ValueFromX(pos);
        if (v == m_value) return;
        m_value = v;
        Refresh();
        SendChangedEvent();
    }


    void CModernSlider::SendChangedEvent()
    {
        wxCommandEvent event(wxEVT_SLIDER, GetId());
        event.SetInt(m_value);
        event.SetEventObject(this);
        ProcessWindowEvent(event);
    }

    void CModernSlider::OnCaptureLost(wxMouseCaptureLostEvent&)
    {
        m_dragging = false;
        Refresh();
    }

    // ---------------- CModernRadioButton ----------------

    CModernRadioButton::CModernRadioButton(wxWindow* parent, wxWindowID id,
                                           const wxString& label, int groupId,
                                           bool value, const wxPoint& pos,
                                           const wxSize& size)
        : wxControl(parent, id, pos, size, wxBORDER_NONE),
          m_label(label), m_groupId(groupId), m_value(value)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetMinSize(size == wxDefaultSize ? wxSize(130, 28) : size);
        Bind(wxEVT_PAINT, &CModernRadioButton::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernRadioButton::OnEraseBackground, this);
        Bind(wxEVT_LEFT_DOWN, &CModernRadioButton::OnLeftDown, this);
        Bind(wxEVT_KEY_DOWN, &CModernRadioButton::OnKeyDown, this);
        Bind(wxEVT_ENTER_WINDOW, &CModernRadioButton::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &CModernRadioButton::OnMouseLeave, this);
    }

    void CModernRadioButton::SetValue(bool value)
    {
        m_value = value;
        Refresh();
    }

    void CModernRadioButton::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        Refresh();
    }

    void CModernRadioButton::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.background));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const int y = GetClientSize().y / 2;
        const int r = 8;
        gc->SetBrush(wxBrush(m_theme.surface));
        gc->SetPen(wxPen(m_value ? m_theme.accent : m_theme.border, 1.5));
        gc->DrawEllipse(4, y - r, r * 2, r * 2);

        if (m_value)
        {
            gc->SetBrush(wxBrush(m_theme.accent));
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawEllipse(8, y - 4, 8, 8);
        }

        gc->SetFont(GetFont(), IsEnabled() ? m_theme.text : m_theme.disabled);
        wxDouble tw = 0, th = 0;
        gc->GetTextExtent(m_label, &tw, &th);
        gc->DrawText(m_label, 28, (GetClientSize().y - th) / 2.0);
    }

    void CModernRadioButton::OnEraseBackground(wxEraseEvent&) {}
    void CModernRadioButton::OnMouseEnter(wxMouseEvent&) { m_hover = true; Refresh(); }
    void CModernRadioButton::OnMouseLeave(wxMouseEvent&) { m_hover = false; Refresh(); }
    void CModernRadioButton::OnLeftDown(wxMouseEvent&) { if (IsEnabled()) Select(); }
    void CModernRadioButton::OnKeyDown(wxKeyEvent& e)
    {
        if (e.GetKeyCode() == WXK_SPACE || e.GetKeyCode() == WXK_RETURN) Select();
        else e.Skip();
    }

    void CModernRadioButton::Select()
    {
        if (m_value) return;

        // Radio grouping is managed by the parent: controls with the same group id
        // are reset before this control is selected.
        if (GetParent())
        {
            for (wxWindow* child : GetParent()->GetChildren())
            {
                auto* radio = dynamic_cast<CModernRadioButton*>(child);
                if (radio && radio != this && radio->GetGroupId() == m_groupId)
                    radio->SetValue(false);
            }
        }

        m_value = true;
        Refresh();

        wxCommandEvent event(wxEVT_RADIOBUTTON, GetId());
        event.SetInt(1);
        event.SetEventObject(this);
        ProcessWindowEvent(event);
    }

    // ---------------- CModernGroupBox ----------------

    CModernGroupBox::CModernGroupBox(wxWindow* parent, const wxString& label,
                                     wxWindowID id, const wxPoint& pos,
                                     const wxSize& size)
        : wxPanel(parent, id, pos, size, wxBORDER_NONE), m_label(label)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetBackgroundColour(m_theme.background);
        Bind(wxEVT_PAINT, &CModernGroupBox::OnPaint, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernGroupBox::OnEraseBackground, this);
    }

    void CModernGroupBox::SetLabel(const wxString& label)
    {
        m_label = label;
        Refresh();
    }

    void CModernGroupBox::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        SetBackgroundColour(theme.background);
        Refresh();
    }

    void CModernGroupBox::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.background));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        wxDouble tw = 0, th = 0;
        gc->SetFont(GetFont(), m_theme.text);
        gc->GetTextExtent(m_label, &tw, &th);

        const double top = th / 2.0 + 7.0;
        gc->SetPen(wxPen(m_theme.border, 1));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->DrawRoundedRectangle(0.5, top, GetClientSize().x - 1.0,
                                 GetClientSize().y - top - 1.0, 8);

        // Small background patch behind the title, like a modern group header.
        gc->SetBrush(wxBrush(m_theme.background));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(12, 0, tw + 12, th + 3);
        gc->DrawText(m_label, 18, 1);
    }

    void CModernGroupBox::OnEraseBackground(wxEraseEvent&) {}

    // ---------------- CModernCombo ----------------

    CModernCombo::CModernCombo(wxWindow* parent, wxWindowID id, const std::vector<wxString>& items,
        int defaultSelection, const wxPoint& pos, const wxSize& size)
        : wxControl(parent, id, pos, size, wxBORDER_NONE)
    {
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetMinSize(size == wxDefaultSize ? wxSize(120, 24) : size);
        SetItems(items, defaultSelection);

        Bind(wxEVT_PAINT, &CModernCombo::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &CModernCombo::OnLeftDown, this);
        Bind(wxEVT_ERASE_BACKGROUND, &CModernCombo::OnEraseBackground, this);
        Bind(wxEVT_ENTER_WINDOW, [this](wxMouseEvent&) { m_hover = true; Refresh(); });
        Bind(wxEVT_LEAVE_WINDOW, [this](wxMouseEvent&) { m_hover = false; Refresh(); });
    }

    void CModernCombo::SetItems(const std::vector<wxString>& items, int defaultSelection)
    {
        m_items = items;
        m_selectionIndex = (defaultSelection >= 0 && defaultSelection < (int)items.size()) ? defaultSelection : 0;
        Refresh();
    }

    wxString CModernCombo::GetSelectionString() const
    {
        if (m_selectionIndex >= 0 && m_selectionIndex < (int)m_items.size())
            return m_items[m_selectionIndex];
        return "";
    }

    void CModernCombo::SetSelection(int index)
    {
        if (index >= 0 && index < (int)m_items.size())
        {
            m_selectionIndex = index;
            Refresh();
        }
    }

    void CModernCombo::SetTheme(const ModernTheme& theme)
    {
        m_theme = theme;
        Refresh();
    }

    void CModernCombo::OnPaint(wxPaintEvent&)
    {
        wxAutoBufferedPaintDC dc(this);
        dc.SetBackground(wxBrush(m_theme.background));
        dc.Clear();

        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc) return;

        const wxRect r = GetClientRect();
        const double radius = 4.0;

        // 1. Fond et bordure
        wxColour fill = m_theme.surface;
        wxColour border = m_hover ? m_theme.accent : m_theme.border;
        DrawRounded(gc.get(), r, fill, border, radius);

        // 2. Dessin de la flèche épurée à droite
        const int arrowWidth = 12;
        const int arrowX = r.width - 20;
        const int arrowY = r.height / 2;

        gc->SetPen(wxPen(m_theme.text, 1.5, wxPENSTYLE_SOLID));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        wxGraphicsPath path = gc->CreatePath();
        path.MoveToPoint(arrowX, arrowY - 2);
        path.AddLineToPoint(arrowX + 4, arrowY + 2);
        path.AddLineToPoint(arrowX + 8, arrowY - 2);
        gc->StrokePath(path);

        // 3. Rendu du texte sélectionné
        wxString currentText = GetSelectionString();
        if (!currentText.IsEmpty())
        {
            gc->SetFont(GetFont(), m_theme.text);
            wxDouble tw = 0, th = 0;
            gc->GetTextExtent(currentText, &tw, &th);

            // Découpage automatique avec "..." si le texte est trop long
            int maxTextWidth = r.width - 28;
            if (tw > maxTextWidth)
            {
                while (tw > maxTextWidth && currentText.Length() > 0)
                {
                    currentText.RemoveLast();
                    gc->GetTextExtent(currentText + "...", &tw, &th);
                }
                currentText += "...";
            }

            gc->DrawText(currentText, 8, (r.height - th) / 2.0);
        }
    }

    void CModernCombo::OnLeftDown(wxMouseEvent&)
    {
        if (m_items.empty()) return;

        // Utilisation d'un wxMenu natif pour simuler de façon ultra stable le comportement Popup d'une ComboBox moderne
        wxMenu menu;
        for (size_t i = 0; i < m_items.size(); ++i)
        {
            wxMenuItem* item = menu.AppendCheckItem(wxID_HIGHEST + i, m_items[i]);
            if ((int)i == m_selectionIndex)
                item->Check(true);
        }

        const wxRect r = GetClientRect();
        // Affiche le menu juste en dessous du bouton de contrôle
        int id = GetParent()->GetPopupMenuSelectionFromUser(menu, GetPosition().x, GetPosition().y + r.height);

        if (id >= wxID_HIGHEST && id < wxID_HIGHEST + (int)m_items.size())
        {
            m_selectionIndex = id - wxID_HIGHEST;
            Refresh();

            // Notification vers l'OS
            wxCommandEvent event(wxEVT_COMBOBOX, GetId());
            event.SetInt(m_selectionIndex);
            event.SetEventObject(this);
            ProcessWindowEvent(event);
        }
    }

    void CModernCombo::OnEraseBackground(wxEraseEvent&) {}

}
