#include "header.h"
#include "TreeElementComboBox.h"
#include "TreeControl.h"
#include <TreeElementValue.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <RegardsConfigParam.h>
#include <ParamInit.h>
using namespace Regards::Window;
using namespace Regards::UI;

CTreeElementComboBox::CTreeElementComboBox(CTreeControl* parent, wxString exifKey)
    : m_selectionIndex(0), m_pComboBoxModern(nullptr), m_pParentWindow(nullptr), m_pParentControl(parent)
{
    this->exifKey = exifKey;
    m_width = DEFAULT_WIDTH;
    m_height = DEFAULT_HEIGHT;
    m_items.clear();

    if (auto* regardsParam = CParamInit::getInstance())
        skinMode = regardsParam->GetSkinWindowMode();

    isDarkTheme =
        wxSystemSettings::GetAppearance().IsDark();

}

CTreeElementComboBox::~CTreeElementComboBox()
{
    if (m_pComboBoxModern != nullptr)
    {
        m_pComboBoxModern->Destroy();
        m_pComboBoxModern = nullptr;
    }
}

void CTreeElementComboBox::SetZoneSize(const int& width, const int& height)
{
    if (width > DEFAULT_WIDTH)
        m_width = width;
    else
        m_width = DEFAULT_WIDTH;

    m_height = height;

    if (m_pComboBoxModern != nullptr)
    {
        m_pComboBoxModern->SetSize(m_width - 4, m_height - 2);
    }
}

int CTreeElementComboBox::GetWidth()
{
    return m_width;
}

int CTreeElementComboBox::GetHeight()
{
    return m_height;
}

void CTreeElementComboBox::SetItems(const std::vector<wxString>& items, const int& defaultSelection)
{
    m_items = items;
    m_selectionIndex = (defaultSelection >= 0 && defaultSelection < (int)items.size()) ? defaultSelection : 0;

    if (m_pComboBoxModern != nullptr)
    {
        m_pComboBoxModern->SetItems(items, m_selectionIndex);
    }
}

int CTreeElementComboBox::GetSelectionIndex() const
{
    return m_selectionIndex;
}

wxString CTreeElementComboBox::GetSelectionString() const
{
    if (m_selectionIndex >= 0 && m_selectionIndex < (int)m_items.size())
        return m_items[m_selectionIndex];
    return "";
}

void CTreeElementComboBox::SetSelection(const int& index)
{
    if (index >= 0 && index < (int)m_items.size())
    {
        m_selectionIndex = index;
        if (m_pComboBoxModern != nullptr)
        {
            m_pComboBoxModern->SetSelection(index);
        }
    }
}

void CTreeElementComboBox::ClickElement(wxWindow* window, const int& x, const int& y)
{
    if (window == nullptr || m_items.empty()) return;
    m_pParentWindow = window;

    wxPoint comboPos(x + 2, y + 1);
    wxSize comboSize(m_width - 4, m_height - 2);

    // Si le contrôle moderne n'existe pas encore, on le crée
    if (m_pComboBoxModern == nullptr)
    {
        Regards::UI::ModernTheme m_theme = IsDarkTheme() ? Regards::UI::ModernTheme::Dark() : Regards::UI::ModernTheme::Light();
        m_pComboBoxModern = new CModernCombo(window, wxID_ANY, m_items, m_selectionIndex, comboPos, comboSize);
        m_pComboBoxModern->SetTheme(m_theme); // Aligné sur votre thème sombre global
        m_pComboBoxModern->Bind(wxEVT_COMBOBOX, &CTreeElementComboBox::OnComboSelection, this);
    }
    else
    {
        // S'il existe déjà, on le repositionne et synchronise sa sélection
        m_pComboBoxModern->SetSize(comboPos.x, comboPos.y, comboSize.x, comboSize.y);
        m_pComboBoxModern->SetSelection(m_selectionIndex);
    }

    m_pComboBoxModern->Show(true);
    m_pComboBoxModern->SetFocus();

    // Simule immédiatement le clic pour ouvrir le menu contextuel pop-up du CModernCombo
    wxMouseEvent mockEvent(wxEVT_LEFT_DOWN);
    mockEvent.SetPosition(wxPoint(comboSize.x / 2, comboSize.y / 2));
    m_pComboBoxModern->GetEventHandler()->ProcessEvent(mockEvent);
}

void CTreeElementComboBox::SynchronizeComboPosition(const int& x, const int& y)
{
    if (m_pComboBoxModern == nullptr) return;

    const wxPoint newPos(x + 2, y + 1);
    const wxSize newSize(m_width - 4, m_height - 2);

    // Cellule sortie de la zone visible de l'arbre : on masque le contrôle
    if (m_pParentWindow != nullptr)
    {
        const wxRect visibleArea = m_pParentWindow->GetClientRect();
        if (!visibleArea.Intersects(wxRect(newPos, newSize)))
        {
            m_pComboBoxModern->Show(false);
            return;
        }
    }

    if (m_pComboBoxModern->GetPosition() != newPos || m_pComboBoxModern->GetSize() != newSize)
    {
        m_pComboBoxModern->SetSize(newPos.x, newPos.y, newSize.x, newSize.y);
    }
}

void CTreeElementComboBox::OnComboSelection(wxCommandEvent& event)
{
    if (m_pComboBoxModern == nullptr) return;

    // 1. Enregistrement du nouvel index sélectionné depuis le CModernCombo
    m_selectionIndex = event.GetInt();

    // 2. On masque le widget
    m_pComboBoxModern->Show(false);

    if (m_pParentWindow != nullptr)
    {
        m_pParentWindow->CallAfter([this]() {
            if (m_pParentControl != nullptr)
            {
                CTreeElementValueInt valueInt(m_selectionIndex);

                // Notification synchrone vers le filtre OpenCV
                m_pParentControl->SlidePosChange(this, m_selectionIndex, &valueInt, exifKey);
            }

            // Rafraîchissement complet de l'arbre graphique
            if (m_pParentWindow != nullptr)
            {
                m_pParentWindow->Refresh();
                m_pParentWindow->Update();
            }
            });
    }
}
void CTreeElementComboBox::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
    if (deviceContext == nullptr) return;

    xPos = x;
    yPos = y;

    // Si le contrôle moderne est visible, on le laisse dessiner par-dessus,
    // mais on le replace sur la cellule : x/y changent quand les barres de défilement bougent
    if (m_pComboBoxModern != nullptr && m_pComboBoxModern->IsShown())
    {
        SynchronizeComboPosition(x, y);
        return;
    }

    std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(*(wxWindowDC*)deviceContext));
    if (!gc) return;

    Regards::UI::ModernTheme theme = IsDarkTheme() ? Regards::UI::ModernTheme::Dark() : Regards::UI::ModernTheme::Light();

    // 1. Dessin du fond de la ligne de l'arbre
    if (backcolor.IsOk())
    {
        gc->SetBrush(wxBrush(backcolor));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(x, y, m_width, m_height);
    }

    // 2. Rendu au repos (Simule trait pour trait l'apparence graphique statique de CModernCombo)
    wxRect boxRect(x + 2, y + 1, m_width - 4, m_height - 2);

    gc->SetBrush(wxBrush(theme.surface));
    gc->SetPen(wxPen(theme.border, 1.0));
    gc->DrawRoundedRectangle(boxRect.x + 0.5, boxRect.y + 0.5, boxRect.width - 1, boxRect.height - 1, 4.0);

    // 3. Dessin de la flèche vectorielle épurée (Style chevron vers le bas)
    const int arrowX = boxRect.x + boxRect.width - 18;
    const int arrowY = boxRect.y + (boxRect.height / 2);

    gc->SetPen(wxPen(theme.text, 1.5, wxPENSTYLE_SOLID));
    gc->SetBrush(*wxTRANSPARENT_BRUSH);

    wxGraphicsPath path = gc->CreatePath();
    path.MoveToPoint(arrowX, arrowY - 2);
    path.AddLineToPoint(arrowX + 4, arrowY + 2);
    path.AddLineToPoint(arrowX + 8, arrowY - 2);
    gc->StrokePath(path);

    // 4. Rendu du texte de l'option courante sélectionnée avec troncature automatique
    wxString currentText = GetSelectionString();
    if (!currentText.IsEmpty())
    {
        gc->SetFont(wxFont(wxFontInfo(9).Family(wxFONTFAMILY_DEFAULT)), theme.text);

        wxDouble textW, textH;
        gc->GetTextExtent(currentText, &textW, &textH);
        int textY = boxRect.y + (boxRect.height - textH) / 2;
        int maxTextWidth = boxRect.width - 26;

        if (textW > maxTextWidth)
        {
            while (textW > maxTextWidth && currentText.Length() > 0)
            {
                currentText.RemoveLast();
                gc->GetTextExtent(currentText + "...", &textW, &textH);
            }
            currentText += "...";
        }

        gc->DrawText(currentText, boxRect.x + 8, textY);
    }
}