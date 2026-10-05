#pragma once
#include <wx/wx.h>

class wxCustomSlider : public wxControl {
public:
    wxCustomSlider(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue,
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize);

    int GetValue() const { return m_value; }
    void SetValue(int val);

private:
    // Classe interne pour gérer uniquement le dessin de la ligne et du rond
    class wxSliderCanvas : public wxWindow {
    public:
        wxSliderCanvas(wxCustomSlider* owner);
    private:
        void OnPaint(wxPaintEvent& event);
        void OnMouseDown(wxMouseEvent& event);
        void OnMouseMove(wxMouseEvent& event);
        void OnMouseUp(wxMouseEvent& event);

        int ValueToX(int val);
        int XToValue(int x);

        wxCustomSlider* m_owner;
        bool m_isDragging;
        wxDECLARE_EVENT_TABLE();
    };

    void OnDecrement(wxCommandEvent& event);
    void OnIncrement(wxCommandEvent& event);
    void SendChangeEvent();

    int m_value;
    int m_min;
    int m_max;

    wxButton* m_btnMinus;
    wxSliderCanvas* m_canvas;
    wxButton* m_btnPlus;

    enum {
        ID_BTN_MINUS = 20001,
        ID_BTN_PLUS
    };

    wxDECLARE_EVENT_TABLE();
};

wxDECLARE_EVENT(wxEVT_CUSTOM_SLIDER_CHANGED, wxCommandEvent);
