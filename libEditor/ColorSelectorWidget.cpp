#include <header.h>
#include "ColorSelectorWidget.h"
#include <wx/dcbuffer.h>
#include "ColorPickerDialog.h"

wxDEFINE_EVENT(wxEVT_COLOR_SELECTOR_CHANGED, wxCommandEvent);

wxBEGIN_EVENT_TABLE(CColorSelectorWidget, wxWindow)
EVT_PAINT(CColorSelectorWidget::OnPaint)
EVT_LEFT_DOWN(CColorSelectorWidget::OnLeftDown)
EVT_ERASE_BACKGROUND(CColorSelectorWidget::OnEraseBackground)
wxEND_EVENT_TABLE()

CColorSelectorWidget::CColorSelectorWidget(wxWindow* parent,
    wxWindowID id,
    const wxPoint& pos,
    const wxSize& size,
    const wxColour& initialColor1,
    const wxColour& initialColor2)
    : wxWindow(parent, id, pos, size, wxFULL_REPAINT_ON_RESIZE),
    m_color1(initialColor1),
    m_color2(initialColor2)
{
    // Si aucune taille n'est spécifiée, on calcule une taille idéale par défaut
    if (size == wxDefaultSize)
    {
        int idealWidth = (m_padding * 2) + (m_squareSize * 2) + m_spacing;
        int idealHeight = (m_padding * 2) + m_squareSize;
        SetMinSize(wxSize(idealWidth, idealHeight));
        SetSize(wxSize(idealWidth, idealHeight));
    }

    m_color1 = wxColor(0, 0, 0);
    m_color2 = wxColor(255, 255, 255);
}

wxRect CColorSelectorWidget::GetRectSquare1() const
{
    // Premier carré positionné à gauche
    return wxRect(m_padding, m_padding, m_squareSize, m_squareSize);
}

wxRect CColorSelectorWidget::GetRectSquare2() const
{
    // Deuxième carré décalé de la largeur du premier + l'espace
    return wxRect(m_padding + m_squareSize + m_spacing, m_padding, m_squareSize, m_squareSize);
}

void CColorSelectorWidget::SetColor1(const wxColour& color)
{
    if (m_color1 != color)
    {
        m_color1 = color;
        Refresh();
    }
}

void CColorSelectorWidget::SetColor2(const wxColour& color)
{
    if (m_color2 != color)
    {
        m_color2 = color;
        Refresh();
    }
}

void CColorSelectorWidget::OnPaint(wxPaintEvent& event)
{
    // Utilisation du double-buffering automatique pour un rendu propre
    wxBufferedPaintDC dc(this);

    // Dessiner le fond (on prend la couleur de fond native du parent/système)
    dc.SetBackground(wxBrush(GetBackgroundColour()));
    dc.Clear();

    // Bordure discrète autour des carrés (gris foncé)
    dc.SetPen(wxPen(wxColour(100, 100, 100), 1, wxPENSTYLE_SOLID));

    // Dessin du Carré 1
    dc.SetBrush(wxBrush(m_color1));
    dc.DrawRectangle(GetRectSquare1());

    // Dessin du Carré 2
    dc.SetBrush(wxBrush(m_color2));
    dc.DrawRectangle(GetRectSquare2());
}

void CColorSelectorWidget::OnLeftDown(wxMouseEvent& event)
{
    wxPoint mousePos = event.GetPosition();
    bool colorChanged = false;

    // Détecter si le clic est sur le Carré 1 ou le Carré 2
    if (GetRectSquare1().Contains(mousePos))
    {
        CColorPickerDialog colorDialog(this, m_color1);
        if (colorDialog.ShowModal() == wxID_OK)
        {
            m_color1 = colorDialog.GetColour();
            colorChanged = true;
        }
    }
    else if (GetRectSquare2().Contains(mousePos))
    {
        CColorPickerDialog colorDialog(this, m_color2);
        if (colorDialog.ShowModal() == wxID_OK)
        {
            m_color2 = colorDialog.GetColour();
            colorChanged = true;
        }
        
    }

    if (colorChanged)
    {
        // Forcer le composant à se redessiner immédiatement
        Refresh();

        // Envoyer une notification à la fenêtre parente
        wxCommandEvent evt(wxEVT_COLOR_SELECTOR_CHANGED, GetId());
        evt.SetEventObject(this);
        ProcessWindowEvent(evt);
    }
}
