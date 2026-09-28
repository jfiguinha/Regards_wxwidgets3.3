#include "header.h"
#include "TreeElementComboBox.h"

using namespace Regards::Window;

CTreeElementComboBox::CTreeElementComboBox()
	: m_selectionIndex(0), m_pComboBoxNative(nullptr), m_pParentWindow(nullptr)
{
	m_width = DEFAULT_WIDTH;
	m_height = DEFAULT_HEIGHT;
	m_items.clear();
}

CTreeElementComboBox::~CTreeElementComboBox()
{
	if (m_pComboBoxNative != nullptr)
	{
		m_pComboBoxNative->Destroy();
		m_pComboBoxNative = nullptr;
	}
}

void CTreeElementComboBox::SetZoneSize(const int& width, const int& height)
{
	m_width = width;
	m_height = height;
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
	m_selectionIndex = (defaultSelection >= 0 && defaultSelection < items.size()) ? defaultSelection : 0;
}

int CTreeElementComboBox::GetSelectionIndex() const
{
	return m_selectionIndex;
}

wxString CTreeElementComboBox::GetSelectionString() const
{
	if (m_selectionIndex >= 0 && m_selectionIndex < m_items.size())
		return m_items[m_selectionIndex];
	return "";
}

void CTreeElementComboBox::SetSelection(const int& index)
{
	if (index >= 0 && index < m_items.size())
		m_selectionIndex = index;
}

void CTreeElementComboBox::ClickElement(wxWindow* window, const int& x, const int& y)
{
	if (window == nullptr || m_items.empty()) return;
	m_pParentWindow = window;

	// Si une combobox est déjà affichée, on ne fait rien
	if (m_pComboBoxNative != nullptr) return;

	// On prépare la liste pour l'objet natif wxWidgets
	wxArrayString choices;
	for (const auto& item : m_items)
	{
		choices.Add(item);
	}

	// Calcul de la position absolue de l'élément par rapport au panel parent de l'arbre
	// xPos et yPos proviennent de la classe de base CTreeElement
	wxPoint comboPos(xPos, yPos);
	wxSize comboSize(m_width - 4, m_height);

	// Instanciation de la ComboBox native wxWidgets en mode lecture seule (dropdown non éditable)
	m_pComboBoxNative = new wxComboBox(window, wxID_ANY, choices[m_selectionIndex],
		comboPos, comboSize, choices, wxCB_READONLY);

	// Liaison de l'événement de sélection
	m_pComboBoxNative->Bind(wxEVT_COMBOBOX, &CTreeElementComboBox::OnComboSelection, this);

	// Déclenchement automatique de l'ouverture du menu déroulant
	m_pComboBoxNative->Popup();
	m_pComboBoxNative->SetFocus();
}

void CTreeElementComboBox::OnComboSelection(wxCommandEvent& event)
{
	if (m_pComboBoxNative == nullptr) return;

	// Enregistrement du nouvel index sélectionné
	m_selectionIndex = m_pComboBoxNative->GetSelection();

	// Simulation d'une perte de focus ou fin d'édition : on détruit le contrôle éphémère
	m_pComboBoxNative->Destroy();
	m_pComboBoxNative = nullptr;

	if (m_pParentWindow != nullptr)
	{
		// Force le rafraîchissement global de l'arbre pour afficher le nouveau texte
		m_pParentWindow->Refresh();

		// Optionnel : Vous pouvez poster ici un événement personnalisé pour notifier 
		// l'arbre parent (CFiltreEffect) du changement de paramètre.
	}
}

void CTreeElementComboBox::DrawElement(wxDC* deviceContext, const int& x, const int& y)
{
	if (deviceContext == nullptr) return;

	// 1. Dessin du fond de ligne standard
	if (backcolor.IsOk())
	{
		wxPen backPen(backcolor, 1, wxPENSTYLE_SOLID);
		wxBrush backBrush(backcolor, wxBRUSHSTYLE_SOLID);
		deviceContext->SetPen(backPen);
		deviceContext->SetBrush(backBrush);
		deviceContext->DrawRectangle(x, y, m_width, m_height);
	}

	// Si le contrôle natif est visible au-dessus, c'est l'OS qui le dessine.
	// Sinon (au repos), on simule l'affichage graphique d'un champ de sélection :
	if (m_pComboBoxNative == nullptr)
	{
		int marginX = 2;
		int marginY = 1;
		int boxWidth = m_width - (marginX * 2);
		int boxHeight = m_height - (marginY * 2);

		// Dessin du rectangle de contour (Style champ de texte)
		deviceContext->SetPen(wxPen(wxColour(180, 180, 180), 1, wxPENSTYLE_SOLID));
		deviceContext->SetBrush(*wxWHITE_BRUSH);
		deviceContext->DrawRectangle(x + marginX, y + marginY, boxWidth, boxHeight);

		// Dessin de la flèche ComboBox sur la droite de la cellule
		int arrowBoxWidth = 16;
		int arrowX = x + marginX + boxWidth - arrowBoxWidth;
		deviceContext->SetPen(wxPen(wxColour(180, 180, 180), 1, wxPENSTYLE_SOLID));
		deviceContext->SetBrush(wxBrush(wxColour(240, 240, 240), wxBRUSHSTYLE_SOLID));
		deviceContext->DrawRectangle(arrowX, y + marginY, arrowBoxWidth, boxHeight);

		// Petite flèche noire vers le bas
		deviceContext->SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID));
		deviceContext->SetBrush(wxBrush(wxColour(0, 0, 0), wxBRUSHSTYLE_SOLID));
		wxPoint arrowPoints[3];
		arrowPoints[0] = wxPoint(arrowX + 4, y + (boxHeight / 2) - 1);
		arrowPoints[1] = wxPoint(arrowX + 12, y + (boxHeight / 2) - 1);
		arrowPoints[2] = wxPoint(arrowX + 8, y + (boxHeight / 2) + 3);
		deviceContext->DrawPolygon(3, arrowPoints);

		// Rendu textuel de l'option courante sélectionnée
		wxString currentText = GetSelectionString();
		if (!currentText.IsEmpty())
		{
			deviceContext->SetFont(deviceContext->GetFont()); // Utilise la police par défaut du DC
			deviceContext->SetTextForeground(wxColour(0, 0, 0));

			// On tronque le texte s'il dépasse de la zone de la ComboBox
			wxCoord textW, textH;
			deviceContext->GetTextExtent(currentText, &textW, &textH);
			int textY = y + (m_height - textH) / 2;

			deviceContext->DrawText(currentText, x + marginX + 4, textY);
		}
	}

	// Nettoyage standard du DC
	deviceContext->SetBrush(wxNullBrush);
	deviceContext->SetPen(wxNullPen);
}
