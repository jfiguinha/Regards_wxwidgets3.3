#pragma once

class CColorPickerDialog;

// Déclaration d'un type d'événement personnalisé pour notifier le parent
wxDECLARE_EVENT(wxEVT_COLOR_SELECTOR_CHANGED, wxCommandEvent);

class CColorSelectorWidget : public wxWindow
{
public:
    CColorSelectorWidget(wxWindow* parent,
        wxWindowID id = wxID_ANY,
        const wxPoint& pos = wxDefaultPosition,
        const wxSize& size = wxDefaultSize,
        const wxColour& initialColor1 = *wxBLACK,
        const wxColour& initialColor2 = *wxWHITE);

    ~CColorSelectorWidget() override = default;

    // Getters pour récupérer les couleurs sélectionnées
    wxColour GetColor1() const { return m_color1; }
    wxColour GetColor2() const { return m_color2; }

    // Setters pour modifier les couleurs par programmation
    void SetColor1(const wxColour& color);
    void SetColor2(const wxColour& color);

private:
    void OnPaint(wxPaintEvent& event);
    void OnLeftDown(wxMouseEvent& event);
    void OnEraseBackground(wxEraseEvent& event) {} // Évite les scintillements

    // Retourne la zone (wxRect) occupée par chaque carré
    wxRect GetRectSquare1() const;
    wxRect GetRectSquare2() const;

    wxColour m_color1;
    wxColour m_color2;

    const int m_squareSize = 40;  // Taille d'un côté du carré en pixels
    const int m_spacing = 15;     // Espace entre les deux carrés
    const int m_padding = 10;     // Marges intérieures autour des carrés


    CColorPickerDialog* colorPicker = nullptr;

    wxDECLARE_EVENT_TABLE();
};
