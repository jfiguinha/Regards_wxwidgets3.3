#include <header.h>
#include "CustomSlider.h"

wxDEFINE_EVENT(wxEVT_CUSTOM_SLIDER_CHANGED, wxCommandEvent);

// --- Table d'événements du conteneur principal ---
wxBEGIN_EVENT_TABLE(wxCustomSlider, wxControl)
EVT_BUTTON(ID_BTN_MINUS, wxCustomSlider::OnDecrement)
EVT_BUTTON(ID_BTN_PLUS, wxCustomSlider::OnIncrement)
wxEND_EVENT_TABLE()

wxCustomSlider::wxCustomSlider(wxWindow* parent, wxWindowID id, int value, int minValue, int maxValue,
    const wxPoint& pos, const wxSize& size)
    : wxControl(parent, id, pos, size, wxBORDER_NONE), m_value(value), m_min(minValue), m_max(maxValue)
{
    // 1. Création des sous-composants internes
    m_btnMinus = new wxButton(this, ID_BTN_MINUS, "-", wxDefaultPosition, wxSize(10, 10));
    m_canvas = new wxSliderCanvas(this);
    m_btnPlus = new wxButton(this, ID_BTN_PLUS, "+", wxDefaultPosition, wxSize(10, 10));

    // 2. Agencement interne automatique
    wxBoxSizer* internalSizer = new wxBoxSizer(wxHORIZONTAL);
    internalSizer->Add(m_btnMinus, 0, wxALIGN_CENTER_VERTICAL);
    internalSizer->Add(m_canvas, 1, wxEXPAND | wxLEFT | wxRIGHT, 5); // Le dessin prend tout le centre
    internalSizer->Add(m_btnPlus, 0, wxALIGN_CENTER_VERTICAL);

    SetSizer(internalSizer);
}

void wxCustomSlider::SetValue(int val) {
    int boundedValue = wxMax(m_min, wxMin(val, m_max));
    if (boundedValue != m_value) {
        m_value = boundedValue;
        m_canvas->Refresh(); // Demande le redessin de la piste au canvas
    }
}

void wxCustomSlider::OnDecrement(wxCommandEvent& event) {
    SetValue(m_value - 1);
    SendChangeEvent();
}

void wxCustomSlider::OnIncrement(wxCommandEvent& event) {
    SetValue(m_value + 1);
    SendChangeEvent();
}

void wxCustomSlider::SendChangeEvent() {
    wxCommandEvent evt(wxEVT_CUSTOM_SLIDER_CHANGED, GetId());
    evt.SetInt(m_value);
    evt.SetEventObject(this);
    ProcessWindowEvent(evt);
}


// --- Implémentation du Canvas de dessin interne ---
wxBEGIN_EVENT_TABLE(wxCustomSlider::wxSliderCanvas, wxWindow)
EVT_PAINT(wxCustomSlider::wxSliderCanvas::OnPaint)
EVT_LEFT_DOWN(wxCustomSlider::wxSliderCanvas::OnMouseDown)
EVT_MOTION(wxCustomSlider::wxSliderCanvas::OnMouseMove)
EVT_LEFT_UP(wxCustomSlider::wxSliderCanvas::OnMouseUp)
wxEND_EVENT_TABLE()

wxCustomSlider::wxSliderCanvas::wxSliderCanvas(wxCustomSlider* owner)
    : wxWindow(owner, wxID_ANY), m_owner(owner), m_isDragging(false)
{
    SetMinSize(wxSize(60, 30));
}

int wxCustomSlider::wxSliderCanvas::ValueToX(int val) {
    wxSize size = GetClientSize();
    return ((val - m_owner->m_min) * (size.x - 20)) / (m_owner->m_max - m_owner->m_min) + 10;
}

int wxCustomSlider::wxSliderCanvas::XToValue(int x) {
    wxSize size = GetClientSize();
    if (size.x <= 20) return m_owner->m_min;
    int val = m_owner->m_min + ((x - 10) * (m_owner->m_max - m_owner->m_min)) / (size.x - 20);
    return wxMax(m_owner->m_min, wxMin(val, m_owner->m_max));
}

void wxCustomSlider::wxSliderCanvas::OnPaint(wxPaintEvent& event) {
    wxPaintDC dc(this);
    wxSize size = GetClientSize();

    // Dessin de la piste
    dc.SetPen(wxPen(wxColour(200, 200, 200), 4));
    dc.DrawLine(10, size.y / 2, size.x - 10, size.y / 2);

    // Dessin du curseur (Thumb)
    int thumbX = ValueToX(m_owner->m_value);
    dc.SetPen(*wxTRANSPARENT_PEN);
    dc.SetBrush(wxBrush(wxColour(0, 120, 215)));
    dc.DrawCircle(thumbX, size.y / 2, 8);
}

void wxCustomSlider::wxSliderCanvas::OnMouseDown(wxMouseEvent& event) {
    m_isDragging = true;
    CaptureMouse();
    int newValue = XToValue(event.GetX());
    m_owner->SetValue(newValue);
    m_owner->SendChangeEvent();
}

void wxCustomSlider::wxSliderCanvas::OnMouseMove(wxMouseEvent& event) {
    if (m_isDragging && event.Dragging()) {
        int newValue = XToValue(event.GetX());
        m_owner->SetValue(newValue);
        m_owner->SendChangeEvent();
    }
}

void wxCustomSlider::wxSliderCanvas::OnMouseUp(wxMouseEvent& event) {
    if (m_isDragging) {
        m_isDragging = false;
        ReleaseMouse();
    }
}
