#pragma once

// Lightweight owner-drawn controls for wxWidgets 3.3.
// They intentionally avoid native wxButton/wxSlider rendering.
// Add RegardsModernControls.cpp to your project.

namespace Regards::UI
{
    struct ModernTheme
    {
        wxColour background{ 248, 249, 251 };
        wxColour surface{ 255, 255, 255 };
        wxColour text{ 30,  35,  42 };
        wxColour secondaryText{ 105, 112,  122 };
        wxColour border{ 218, 223, 230 };
        wxColour accent{ 0, 120, 215 };
        wxColour accentHover{ 0, 105, 190 };
        wxColour hover{ 240, 244, 248 };
        wxColour pressed{ 225, 232, 240 };
        wxColour disabled{ 160, 166, 174 };
        bool dark = false;

        static ModernTheme Light();
        static ModernTheme Dark();
    };

    // Use SetTheme() on controls to switch theme at runtime.
    class CModernPanel : public wxPanel
    {
    public:
        CModernPanel(wxWindow* parent, wxWindowID id = wxID_ANY,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxTAB_TRAVERSAL);

        void SetTheme(const ModernTheme& theme);
        const ModernTheme& GetTheme() const { return m_theme; }
        void SetCornerRadius(int radius);
        void SetBorder(bool enabled);

    protected:
        void OnPaint(wxPaintEvent& event);
        void OnEraseBackground(wxEraseEvent& event);

        ModernTheme m_theme;
        int m_radius = 10;
        bool m_border = true;
    };

    class CModernButton : public wxControl
    {
    public:
        CModernButton(wxWindow* parent, wxWindowID id, const wxString& label,
                     const wxPoint& pos = wxDefaultPosition,
                     const wxSize& size = wxDefaultSize,
                     long style = wxBORDER_NONE);

        void SetLabel(const wxString& label) override;
        wxString GetLabel() const override { return m_label; }

        void SetTheme(const ModernTheme& theme);
        void SetAccent(bool accent);
        void SetCornerRadius(int radius);

    protected:
        void OnPaint(wxPaintEvent& event);
        void OnEraseBackground(wxEraseEvent& event);
        void OnMouseEnter(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void OnLeftDown(wxMouseEvent& event);
        void OnLeftUp(wxMouseEvent& event);
        void OnCaptureLost(wxMouseCaptureLostEvent& event);
        void OnKeyDown(wxKeyEvent& event);
        void OnKeyUp(wxKeyEvent& event);
        void OnSetFocus(wxFocusEvent& event);
        void OnKillFocus(wxFocusEvent& event);

        wxString m_label;
        ModernTheme m_theme;
        bool m_accent = false;
        bool m_hover = false;
        bool m_pressed = false;
        bool m_focused = false;
        int m_radius = 8;
    };

    class CModernSlider : public wxControl
    {
    public:
        CModernSlider(wxWindow* parent, wxWindowID id = wxID_ANY,
                      int value = 50, int minValue = 0, int maxValue = 100,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize);

        int GetValue() const { return m_value; }
        int GetMin() const { return m_min; }
        int GetMax() const { return m_max; }
        void SetValue(int value);
        void SetRange(int minValue, int maxValue);
        void SetTheme(const ModernTheme& theme);

    protected:
        void OnPaint(wxPaintEvent& event);
        void OnEraseBackground(wxEraseEvent& event);
        void OnLeftDown(wxMouseEvent& event);
        void OnLeftUp(wxMouseEvent& event);
        void OnMotion(wxMouseEvent& event);
        void OnCaptureLost(wxMouseCaptureLostEvent& event);
        void OnKeyDown(wxKeyEvent& event);

        int ValueFromX(int x) const;
        int ThumbX() const;
        void UpdateFromMouse(int x);
        void SendChangedEvent();

        ModernTheme m_theme;
        int m_min = 0;
        int m_max = 100;
        int m_value = 50;
        bool m_dragging = false;
        bool m_hover = false;
    };

    // Simple custom radio option. Use several controls with a shared group id.
    // Selecting one emits wxEVT_RADIOBUTTON with its control ID.
    class CModernRadioButton : public wxControl
    {
    public:
        CModernRadioButton(wxWindow* parent, wxWindowID id, const wxString& label,
                          int groupId = 0, bool value = false,
                          const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize);

        bool GetValue() const { return m_value; }
        void SetValue(bool value);
        int GetGroupId() const { return m_groupId; }
        void SetTheme(const ModernTheme& theme);

    protected:
        void OnPaint(wxPaintEvent& event);
        void OnEraseBackground(wxEraseEvent& event);
        void OnLeftDown(wxMouseEvent& event);
        void OnKeyDown(wxKeyEvent& event);
        void OnMouseEnter(wxMouseEvent& event);
        void OnMouseLeave(wxMouseEvent& event);
        void Select();

        wxString m_label;
        ModernTheme m_theme;
        int m_groupId = 0;
        bool m_value = false;
        bool m_hover = false;
    };

    // Owner-drawn group-box-like panel. Place controls inside it using sizers.
    class CModernGroupBox : public wxPanel
    {
    public:
        CModernGroupBox(wxWindow* parent, const wxString& label,
                       wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize);

        void SetLabel(const wxString& label);
        wxString GetLabel() const { return m_label; }
        void SetTheme(const ModernTheme& theme);

    protected:
        void OnPaint(wxPaintEvent& event);
        void OnEraseBackground(wxEraseEvent& event);

        wxString m_label;
        ModernTheme m_theme;
    };
}
