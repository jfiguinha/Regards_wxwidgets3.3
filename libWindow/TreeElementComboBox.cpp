#include "header.h"
#include "TreeElementComboBox.h"
#include "TreeControl.h"
#include <TreeElementValue.h>
using namespace Regards::Window;

CTreeElementComboBox::CTreeElementComboBox(CTreeControl* parent, wxString exifKey)
	: m_selectionIndex(0), m_pComboBoxNative(nullptr), m_pParentWindow(nullptr), m_pParentControl(parent)
{
	this->exifKey = exifKey;
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
	if (width > DEFAULT_WIDTH)
		m_width = width;
	else
		m_width = DEFAULT_WIDTH;

	m_height = height;

	if (m_pComboBoxNative != nullptr)
	{
		m_pComboBoxNative->SetSize(m_width - 4, m_height);
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

	wxArrayString choices;
	for (const auto& item : m_items)
	{
		choices.Add(item);
	}

	wxPoint comboPos(x, y);
	wxSize comboSize(m_width - 4, m_height);

	// Si le contrôle n'existe pas encore, on le crée
	if (m_pComboBoxNative == nullptr)
	{
		m_pComboBoxNative = new wxComboBox(window, wxID_ANY, choices[m_selectionIndex],
			comboPos, comboSize, choices, wxCB_READONLY);

		m_pComboBoxNative->Bind(wxEVT_COMBOBOX, &CTreeElementComboBox::OnComboSelection, this);
	}
	else
	{
		// S'il existe déjà, on le repositionne et on le réaffiche
		m_pComboBoxNative->SetSize(comboPos.x, comboPos.y, comboSize.x, comboSize.y);
	}

	m_pComboBoxNative->Show(true);
	m_pComboBoxNative->Popup();
	m_pComboBoxNative->SetFocus();
}

void CTreeElementComboBox::OnComboSelection(wxCommandEvent& event)
{
	if (m_pComboBoxNative == nullptr) return;

	// 1. Enregistrement du nouvel index sélectionné
	m_selectionIndex = m_pComboBoxNative->GetSelection();

	// 2. On masque le widget natif
	m_pComboBoxNative->Show(false);

	if (m_pParentWindow != nullptr)
	{
		m_pParentWindow->CallAfter([this]() {
			if (m_pParentControl != nullptr)
			{
				CTreeElementValueInt valueInt(m_selectionIndex);


				// Notification synchrone vers le filtre OpenCV
				m_pParentControl->SlidePosChange(this, m_selectionIndex, &valueInt, exifKey);
			}

			// --- LA CORRECTION EST ICI ---
			// Il faut forcer l'arbre de contrôle principal à recalculer l'affichage (RenderMode::Update)
			// En appelant l'interface de contrôle liée à votre fenêtre Regards
			// Si la classe Regards possède une méthode sur son interface, on l'appelle :
			// (Par exemple, si eventControl est accessible ou via une méthode virtuelle de CTreeControl)

			if (m_pParentWindow != nullptr)
			{
				// Demande à l'OS de relancer le cycle complet de peinture, ce qui va forcer l'exécution de DrawElement !
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

	// Si le contrôle natif est actuellement ouvert et visible, on le laisse s'afficher par-dessus
	if (m_pComboBoxNative != nullptr && m_pComboBoxNative->IsShown())
	{
		return;
	}

	// 1. Dessin du fond de ligne standard
	if (backcolor.IsOk())
	{
		wxPen backPen(backcolor, 1, wxPENSTYLE_SOLID);
		wxBrush backBrush(backcolor, wxBRUSHSTYLE_SOLID);
		deviceContext->SetPen(backPen);
		deviceContext->SetBrush(backBrush);
		deviceContext->DrawRectangle(x, y, m_width, m_height);
	}

	// 2. Mode au repos : Rendu graphique simulé (Ultra stable, ne bloque pas l'affichage de l'arbre)
	int marginX = 2;
	int marginY = 1;
	int boxWidth = m_width - (marginX * 2);
	int boxHeight = m_height - (marginY * 2);

	// Dessin du champ blanc
	deviceContext->SetPen(wxPen(wxColour(180, 180, 180), 1, wxPENSTYLE_SOLID));
	deviceContext->SetBrush(*wxWHITE_BRUSH);
	deviceContext->DrawRectangle(x + marginX, y + marginY, boxWidth, boxHeight);

	// Dessin du bouton flèche gris à droite
	int arrowBoxWidth = 16;
	int arrowX = x + marginX + boxWidth - arrowBoxWidth;
	deviceContext->SetPen(wxPen(wxColour(180, 180, 180), 1, wxPENSTYLE_SOLID));
	deviceContext->SetBrush(wxBrush(wxColour(240, 240, 240), wxBRUSHSTYLE_SOLID));
	deviceContext->DrawRectangle(arrowX, y + marginY, arrowBoxWidth, boxHeight);

	// Dessin de la petite flèche noire
	deviceContext->SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID));
	deviceContext->SetBrush(wxBrush(wxColour(0, 0, 0), wxBRUSHSTYLE_SOLID));
	wxPoint arrowPoints[3];
	arrowPoints[0] = wxPoint(arrowX + 4, y + (boxHeight / 2) - 1);
	arrowPoints[1] = wxPoint(arrowX + 12, y + (boxHeight / 2) - 1);
	arrowPoints[2] = wxPoint(arrowX + 8, y + (boxHeight / 2) + 3);
	deviceContext->DrawPolygon(3, arrowPoints);

	// Rendu du texte de l'option courante sélectionnée
	wxString currentText = GetSelectionString();
	if (!currentText.IsEmpty())
	{
		deviceContext->SetTextForeground(wxColour(0, 0, 0));

		wxCoord textW, textH;
		deviceContext->GetTextExtent(currentText, &textW, &textH);
		int textY = y + (m_height - textH) / 2;
		int maxTextWidth = boxWidth - arrowBoxWidth - 6;

		wxString textToDraw = currentText;
		if (textW > maxTextWidth && textToDraw.Length() > 0)
		{
			while (textW > maxTextWidth && textToDraw.Length() > 0)
			{
				textToDraw.RemoveLast();
				deviceContext->GetTextExtent(textToDraw + "...", &textW, &textH);
			}
			textToDraw += "...";
		}

		deviceContext->DrawText(textToDraw, x + marginX + 4, textY);
	}

	deviceContext->SetBrush(wxNullBrush);
	deviceContext->SetPen(wxNullPen);
}
